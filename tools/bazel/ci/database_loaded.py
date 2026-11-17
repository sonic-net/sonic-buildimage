#!/usr/bin/env python3
"""Load accepted database archives and exercise their installed services."""

import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import uuid

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import database_image, validate_runtime
from tools.bazel.ci.public_artifacts import safe_json


def verify_inputs(directory):
    """Check both archives and their source identity before running Docker."""
    database_image.clean_source(directory)
    source = database_image.identity()
    report = json.loads((directory / "validation.json").read_text())
    check.require(report.get("status") == "passed" and report.get("architecture") == "amd64",
                  "static validation did not pass")
    check.require(report.get("revision") == source["revision"] and
                  report.get("tested_revision_parents") == source["tested_revision_parents"],
                  "runtime checkout differs from built source")
    check.require(set(report.get("images", {})) == set(database_image.IMAGES),
                  "Database acceptance requires both runtime and debug images")
    for name in database_image.IMAGES:
        archive = directory / (name + ".gz")
        check.require(check.sha(archive) == report["artifacts"][archive.name]["sha256"],
                      "archive changed: " + archive.name)
    return report, source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", type=Path, required=True)
    args = parser.parse_args()
    directory = args.artifacts.resolve()
    report_path = directory / "validation.json"
    report, source = verify_inputs(directory)
    probe = ROOT / "dockers/docker-database/bazel/runtime_probe.py"
    results = {}
    for name, expected in report["images"].items():
        archive = directory / (name + ".gz")
        check.require(check.sha(archive) == report["artifacts"][archive.name]["sha256"], "archive changed")
        tag = "sonic-database-check:" + uuid.uuid4().hex
        container_name = "sonic-database-probe-" + tag.rsplit(":", 1)[1]
        try:
            loaded, config, diagnostics = validate_runtime.load_archive(archive, tag, expected["config_digest"])
            validate_runtime.verify_loaded_image(loaded, config, expected["config_digest"])
            check.require(config["rootfs"] == expected["rootfs"], "loaded layers differ from validated OCI")
            profiles = {}
            for profile in ("single", "multi"):
                command = ["docker", "run", "--rm", "--name", container_name, "--network=none", "--cap-drop=ALL",
                           "--cap-add=CHOWN", "--cap-add=SETUID", "--cap-add=SETGID", "--cap-add=DAC_OVERRIDE",
                           "--security-opt=no-new-privileges", "--read-only", "--user=0:0",
                           "--pids-limit=256", "--memory=4g", "--cpus=2"]
                for path, size, mode in (("/tmp", "256m", "1777"), ("/run", "64m", "0755"),
                                         ("/var/lib/redis", "32m", "0750"), ("/var/lib/redis_bmp", "32m", "0750"),
                                         ("/var/log", "32m", "0755"), ("/var/spool/rsyslog", "8m", "0755"),
                                         ("/etc/sonic", "8m", "0755"), ("/etc/supervisor/conf.d", "8m", "0755")):
                    command += ["--tmpfs", path + ":rw,nosuid,nodev,noexec,size=" + size + ",mode=" + mode]
                if profile == "multi":
                    for index in range(1, 5):
                        command += ["--tmpfs", "/var/lib/redis" + str(index) + ":rw,nosuid,nodev,noexec,size=32m,mode=0750"]
                command += ["--mount", "type=bind,src=" + str(probe) + ",dst=/probe/runtime_probe.py,readonly",
                            "--mount", "type=bind,src=" + str(ROOT / "tools/bazel/oci/runtime_smoke.py") + ",dst=/probe/runtime_smoke.py,readonly",
                            "--mount", "type=bind,src=" + str(report_path) + ",dst=/probe/validation.json,readonly",
                            "--entrypoint", "/usr/bin/python3", tag, "-B", "/probe/runtime_probe.py",
                            "/probe/validation.json", name, profile]
                # Only the generated service-list file needs a writable mount;
                # retain the actual inherited supervisord.conf in the image.
                with tempfile.NamedTemporaryFile(prefix="sonic-database-critical-") as critical:
                    command[2:2] = ["--mount", "type=bind,src=" + critical.name + ",dst=/etc/supervisor/critical_processes"]
                    completed = subprocess.run(command, text=True, capture_output=True, timeout=600)
                if completed.returncode:
                    raise AssertionError(name + " " + profile + " installed probe failed:\n" + completed.stdout + completed.stderr)
                records = [json.loads(line) for line in completed.stdout.splitlines() if line.startswith('{"status":')]
                check.require(len(records) == 1 and records[0]["status"] == "passed", "missing runtime receipt")
                profiles[profile] = records[0]
            results[name] = {"profiles": profiles, "config_and_layers_verified": True, "archive_sha256": check.sha(archive)}
            safe_json(results[name])
            (directory / (name + ".runtime.json")).write_text(json.dumps(results[name], indent=2) + "\n")
        finally:
            # Stop only this probe if its Docker client timed out; --rm alone
            # does not stop a container when the client is killed.
            subprocess.run(["docker", "rm", "--force", container_name], capture_output=True, check=False)
            # Remove only this probe's unique tag, preserving other images/caches.
            subprocess.run(["docker", "image", "rm", tag], capture_output=True, check=False)
    database_image.clean_source(directory)
    check.require(database_image.identity() == source, "loaded source identity changed")
    result = {"status": "passed", **source, "images": results}
    safe_json(result)
    (directory / "loaded-validation.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
