#!/usr/bin/env python3
"""Load accepted Sysmgr archives and exercise the installed software handoff."""

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
from tools.bazel.ci import validate_runtime
from tools.bazel.ci.public_artifacts import safe_json
from tools.bazel.ci.sysmgr_image import clean_source, source_identity


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", type=Path, required=True)
    args = parser.parse_args()
    directory = args.artifacts.resolve()
    clean_source(directory)
    checkout_identity = source_identity()
    report_path = directory / "validation.json"
    report = json.loads(report_path.read_text())
    check.require(report["status"] == "passed" and report["architecture"] == "amd64", "static validation did not pass")
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    check.require(report["revision"] == revision, "runtime checkout differs from built source")
    probe = ROOT / "dockers/docker-sysmgr/bazel/runtime_probe.py"
    mock = directory / "dbus-mock"
    check.require(check.sha(mock) == report["artifacts"][mock.name]["sha256"], "D-Bus mock changed")
    # Actions preserves artifact bytes, but normalizes raw file permissions.
    mock.chmod(0o555)
    results = {}
    with tempfile.TemporaryDirectory(prefix="sysmgr-redis-") as temporary:
        fixture = Path(temporary)
        test_services = directory / "test-services.tar"
        check.require(check.sha(test_services) == report["artifacts"][test_services.name]["sha256"], "Redis fixture changed")
        with tarfile.open(test_services) as source:
            source.extractall(fixture, filter="data")
        fixture.chmod(0o755)
        owner = directory / "config-engine.source.tar"
        check.require(check.sha(owner) == report["artifacts"][owner.name]["sha256"], "database fixture owner changed")
        with tarfile.open(owner) as archive:
            members = [member for member in archive.getmembers()
                       if str(Path(member.name)) == "run/redis/sonic-db/database_config.json"]
            check.require(len(members) == 1 and members[0].isfile(), "missing owner database fixture")
            (fixture / "database_config.json").write_bytes(archive.extractfile(members[0]).read())
        for name in ("docker-sysmgr", "docker-sysmgr-dbg"):
            expected = json.loads((directory / (name + ".validation.json")).read_text())
            check.require(expected["status"] == "passed", "missing accepted OCI validation")
            archive = directory / (name + ".gz")
            check.require(check.sha(archive) == report["artifacts"][archive.name]["sha256"], "archive changed")
            probe_id = uuid.uuid4().hex
            tag = "sonic-sysmgr-check:" + probe_id
            container_name = "sonic-sysmgr-probe-" + probe_id
            try:
                loaded, config, _ = validate_runtime.load_archive(archive, tag, expected["config_digest"])
                validate_runtime.verify_loaded_image(loaded, config, expected["config_digest"])
                check.require(loaded["Os"] == "linux" and loaded["Architecture"] == "amd64", "loaded platform differs")
                check.require(config["rootfs"] == expected["rootfs"], "loaded layers differ from validated OCI")
                command = ["docker", "run", "--rm", "--name", container_name, "--network=none", "--cap-drop=ALL",
                           "--security-opt=no-new-privileges",
                           "--read-only", "--user=0:0", "--pids-limit=128", "--memory=4g", "--cpus=2",
                           "--tmpfs", "/tmp:rw,nosuid,nodev,noexec,size=256m,mode=1777",
                           "--tmpfs", "/run:rw,nosuid,nodev,noexec,size=32m,mode=0755",
                           "--tmpfs", "/var/warmboot:rw,nosuid,nodev,noexec,size=16m,mode=0755",
                           "--tmpfs", "/var/log:rw,nosuid,nodev,noexec,size=32m,mode=0755",
                           "--tmpfs", "/var/spool/rsyslog:rw,nosuid,nodev,noexec,size=16m,mode=0755",
                           "--mount", "type=bind,src=" + str(probe) + ",dst=/probe/runtime_probe.py,readonly",
                           "--mount", "type=bind,src=" + str(ROOT / "tools/bazel/oci/runtime_smoke.py") + ",dst=/probe/runtime_smoke.py,readonly",
                           "--mount", "type=bind,src=" + str(ROOT / "src/sonic-sysmgr/tests/installed_reboot_test.py") + ",dst=/probe/installed_reboot_test.py,readonly",
                           "--mount", "type=bind,src=" + str(mock) + ",dst=/probe/dbus-mock,readonly",
                           "--mount", "type=bind,src=" + str(report_path) + ",dst=/probe/validation.json,readonly",
                           "--mount", "type=bind,src=" + str(fixture) + ",dst=/probe/services,readonly",
                           "--entrypoint", "/usr/bin/python3", tag, "-B", "/probe/runtime_probe.py",
                           "--validation", "/probe/validation.json"]
                if name.endswith("-dbg"):
                    command.append("--debug")
                completed = subprocess.run(command, text=True, capture_output=True, timeout=600)
                check.require(completed.returncode == 0,
                              name + " installed probe failed:\n" + completed.stdout + completed.stderr)
                records = [json.loads(line) for line in completed.stdout.splitlines() if line.startswith('{"status":')]
                check.require(len(records) == 1 and records[0]["status"] == "passed", "missing runtime receipt")
                results[name] = {**records[0], "config_and_layers_verified": True,
                                 "archive_sha256": check.sha(archive), "revision": revision}
                safe_json(results[name])
                (directory / (name + ".runtime.json")).write_text(json.dumps(results[name], indent=2) + "\n")
            finally:
                subprocess.run(["docker", "rm", "--force", container_name], capture_output=True, check=False)
                subprocess.run(["docker", "image", "rm", tag], capture_output=True, check=False)
    clean_source(directory)
    check.require(source_identity() == checkout_identity, "runtime checkout changed during verification")
    print(json.dumps({"status": "passed", "images": list(results)}))


if __name__ == "__main__":
    main()
