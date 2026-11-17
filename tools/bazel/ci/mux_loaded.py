#!/usr/bin/env python3
"""Load the accepted MUX archives and run their installed service probes."""

import argparse
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import uuid

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import mux_image, validate_runtime
from tools.bazel.ci.mux_image import IMAGES
from tools.bazel.ci.public_artifacts import safe_json


def verify_inputs(directory):
    """Check both archives and their source identity before running Docker."""
    mux_image.clean_source(directory)
    source = mux_image.source_identity()
    report = json.loads((directory / "validation.json").read_text())
    check.require(report.get("status") == "passed" and report.get("architecture") == "amd64",
                  "static validation did not pass")
    check.require(report.get("revision") == source["revision"] and
                  report.get("tested_revision_parents") == source["tested_revision_parents"],
                  "runtime checkout differs from built source")
    check.require(set(report.get("images", {})) == set(mux_image.IMAGES),
                  "MUX acceptance requires both runtime and debug images")
    for name in mux_image.IMAGES:
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
    report, identity = verify_inputs(directory)
    revision = identity["revision"]
    probe = ROOT / "dockers/docker-mux/bazel/runtime_probe.py"
    results = {}
    with tempfile.TemporaryDirectory(prefix="mux-redis-") as temporary:
        fixture = Path(temporary)
        test_redis = directory / "test-redis.tar"
        check.require(check.sha(test_redis) == report["artifacts"][test_redis.name]["sha256"], "Redis fixture changed")
        with tarfile.open(test_redis) as source:
            source.extractall(fixture, filter="data")
        # Keep the private fixture readable inside the isolated probe container.
        fixture.chmod(0o755)
        owner = directory / "config-engine-source.tar"
        check.require(check.sha(owner) == report["artifacts"][owner.name]["sha256"], "database fixture owner changed")
        with tarfile.open(owner) as archive:
            members = [member for member in archive.getmembers()
                       if str(Path(member.name)) == "run/redis/sonic-db/database_config.json"]
            check.require(len(members) == 1 and members[0].isfile(), "missing owner database fixture")
            (fixture / "database_config.json").write_bytes(archive.extractfile(members[0]).read())
        for name, expected in report["images"].items():
            archive = directory / (name + ".gz")
            check.require(check.sha(archive) == report["artifacts"][archive.name]["sha256"], "archive changed")
            tag = "sonic-mux-check:" + uuid.uuid4().hex
            container_name = "sonic-mux-probe-" + tag.rsplit(":", 1)[1]
            try:
                loaded, config, diagnostics = validate_runtime.load_archive(archive, tag, expected["config_digest"])
                validate_runtime.verify_loaded_image(loaded, config, expected["config_digest"])
                check.require(config["rootfs"] == expected["rootfs"], "loaded layers differ from validated OCI")
                command = ["docker", "run", "--rm", "--name", container_name, "--network=none", "--cap-drop=ALL",
                           "--cap-add=NET_ADMIN", "--cap-add=NET_RAW", "--cap-add=SYS_NICE", "--cap-add=SETUID", "--cap-add=SETGID", "--cap-add=CHOWN",
                           "--security-opt=no-new-privileges", "--read-only", "--user=0:0",
                           "--pids-limit=128", "--memory=4g", "--cpus=2",
                           "--tmpfs", "/tmp:rw,nosuid,nodev,noexec,size=256m,mode=1777",
                           "--tmpfs", "/run:rw,nosuid,nodev,noexec,size=32m,mode=0755",
                           "--tmpfs", "/var/log:rw,nosuid,nodev,noexec,size=64m,mode=0755",
                           "--tmpfs", "/var/spool/rsyslog:rw,nosuid,nodev,noexec,size=4m,mode=0755",
                           "--tmpfs", "/etc/sonic:rw,nosuid,nodev,noexec,size=4m,mode=0755",
                           "--mount", "type=bind,src=" + str(probe) + ",dst=/probe/runtime_probe.py,readonly",
                           "--mount", "type=bind,src=" + str(ROOT / "tools/bazel/oci/runtime_smoke.py") + ",dst=/probe/runtime_smoke.py,readonly",
                           "--mount", "type=bind,src=" + str(report_path) + ",dst=/probe/validation.json,readonly",
                           "--mount", "type=bind,src=" + str(fixture) + ",dst=/probe/redis,readonly",
                           "--entrypoint", "/usr/bin/python3", tag, "-B", "/probe/runtime_probe.py",
                           "/probe/validation.json", name]
                completed = subprocess.run(command, text=True, capture_output=True, timeout=600)
                if completed.returncode:
                    raise AssertionError(name + " installed probe failed:\n" + completed.stdout + completed.stderr)
                records = []
                for line in completed.stdout.splitlines():
                    if line.startswith("{"):
                        value = json.loads(line)
                        if isinstance(value, dict) and "status" in value:
                            records.append(value)
                check.require(len(records) == 1 and records[0]["status"] == "passed", "missing runtime receipt")
                results[name] = {**records[0], "config_and_layers_verified": True, "archive_sha256": check.sha(archive)}
                safe_json(results[name])
                (directory / (name + ".runtime.json")).write_text(json.dumps(results[name], indent=2) + "\n")
            finally:
                # Stop only this probe if its Docker client timed out; --rm alone
                # does not stop a container when the client is killed.
                subprocess.run(["docker", "rm", "--force", container_name], capture_output=True, check=False)
                # Remove only this probe's unique tag; keep all prior images/caches.
                subprocess.run(["docker", "image", "rm", tag], capture_output=True, check=False)
    result = {"status": "passed", "revision": revision, "images": results}
    safe_json(result)
    mux_image.clean_source(directory)
    check.require(mux_image.source_identity() == identity, "loaded source identity changed")
    (directory / "loaded-validation.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
