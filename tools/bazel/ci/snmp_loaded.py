#!/usr/bin/env python3
"""Verify the SNMP archive handoff and query the installed runtime and debug agents."""

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
from tools.bazel.ci.snmp_image import IMAGES


def run_loaded(directory):
    directory = directory.resolve()
    report = directory / "validation.json"
    receipt = json.loads(report.read_text())
    check.require(receipt["status"] == "passed" and receipt["architecture"] == "amd64",
                  "SNMP static validation did not pass on AMD64")
    check.require(receipt["images"] == list(IMAGES), "expected both SNMP runtime and debug variants")
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    check.require(receipt["revision"] == revision, "SNMP archives were built from a different revision")
    # Read the same version lists as the Make image, including its inherited YAML.
    versions = ROOT / "files/build/versions-public/dockers"
    snmp_pins = dict(line.split("==") for line in (versions / "docker-snmp/versions-py3").read_text().splitlines())
    base_pins = dict(line.split("==") for line in (versions / "docker-base-trixie/versions-py3").read_text().splitlines())
    snmp_pins["pyyaml"] = base_pins["pyyaml"]
    probe = ROOT / "dockers/docker-snmp/bazel/runtime_probe.py"
    redis = directory / "test-redis.tar"
    check.require(check.sha(redis) == receipt["artifacts"][redis.name]["sha256"], "Redis fixture changed")
    with tempfile.TemporaryDirectory(prefix="snmp-redis-") as temporary:
        fixture = Path(temporary)
        with tarfile.open(redis) as data:
            data.extractall(fixture, filter="data")
        # The installed account runs as UID 101, outside the fixture owner's UID.
        fixture.chmod(0o755)
        for name in IMAGES:
            expected = json.loads((directory / (name + ".validation.json")).read_text())
            check.require(expected["status"] == "passed", "missing OCI contents validation for " + name)
            archive = directory / (name + ".gz")
            digest = check.sha(archive)
            check.require(digest == receipt["artifacts"][archive.name]["sha256"], "SNMP archive changed: " + name)
            probe_id = uuid.uuid4().hex
            tag = "sonic-snmp-check:" + probe_id
            container = "sonic-snmp-probe-" + probe_id
            try:
                loaded, config, _ = validate_runtime.load_archive(archive, tag, expected["config_digest"])
                validate_runtime.verify_loaded_image(loaded, config, expected["config_digest"])
                check.require(config["rootfs"] == expected["rootfs"], "loaded SNMP layers differ from OCI")
                check.require((loaded["Os"], loaded["Architecture"]) == ("linux", "amd64"), "wrong SNMP platform")
                command = ["docker", "run", "--rm", "--name", container, "--network=none", "--cap-drop=ALL",
                           "--security-opt=no-new-privileges", "--read-only", "--user=101:102",
                           "--pids-limit=128", "--memory=4g", "--cpus=2",
                           "--tmpfs", "/tmp:rw,nosuid,nodev,noexec,size=256m",
                           "--mount", "type=bind,src=" + str(probe) + ",dst=/probe/runtime_probe.py,readonly",
                           "--mount", "type=bind,src=" + str(fixture) + ",dst=/probe/redis,readonly",
                           "--mount", "type=bind,src=" + str(ROOT / "tools/bazel/oci/runtime_smoke.py") + ",dst=/probe/runtime_smoke.py,readonly",
                           "--mount", "type=bind,src=" + str(report) + ",dst=/probe/validation.json,readonly",
                           "--entrypoint", "/usr/bin/python3", tag, "-B", "/probe/runtime_probe.py", name, json.dumps(snmp_pins)]
                completed = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=600)
                check.require(completed.returncode == 0, name + " probe failed:\n" + completed.stdout + completed.stderr)
                result = json.loads(completed.stdout)
                check.require(result["status"] == "passed" and [q["program"] for q in result["queries"]]
                              == ["snmpget", "snmpgetnext"], "missing installed SNMP query results")
                result.update(revision=revision, image=name, archive_sha256=digest, config_digest=expected["config_digest"])
                safe_json(result)
                (directory / (name + ".runtime.json")).write_text(json.dumps(result, indent=2) + "\n")
                print(json.dumps(result))
            finally:
                subprocess.run(["docker", "rm", "--force", container], capture_output=True, check=False)
                subprocess.run(["docker", "image", "rm", tag], capture_output=True, check=False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", type=Path, required=True)
    run_loaded(parser.parse_args().artifacts)
