#!/usr/bin/env python3
"""Load verified DASH engine archives and exercise their installed software."""

import argparse
import json
from pathlib import Path
import platform
import subprocess
import sys
import uuid

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as check, validate_runtime
from tools.bazel.ci.dash_engine_image import IMAGES, TARGETS, clean_source, source_identity
from tools.bazel.ci.public_artifacts import safe_json


def verify_artifacts(directory, report):
    check.require(set(report["artifacts"]) == set(TARGETS), "incomplete static artifact inventory")
    for name, expected in report["artifacts"].items():
        path = directory / name
        check.require(path.is_file() and path.stat().st_size == expected["bytes"]
                      and check.sha(path) == expected["sha256"], "validated artifact changed: " + name)


def container_command(tag, name, report_path, debug):
    probe = ROOT / "platform/vs/docker-dash-engine/bazel/runtime_probe.py"
    command = ["docker", "run", "--rm", "--name", name, "--network=none", "--cap-drop=ALL",
               "--cap-add=NET_ADMIN", "--cap-add=NET_RAW", "--security-opt=no-new-privileges",
               "--read-only", "--user=0:0", "--pids-limit=128", "--memory=4g", "--cpus=2",
               "--tmpfs", "/tmp:rw,nosuid,nodev,noexec,size=1g,mode=1777",
               "--tmpfs", "/run:rw,nosuid,nodev,noexec,size=32m,mode=0755",
               "--tmpfs", "/var/log:rw,nosuid,nodev,noexec,size=32m,mode=0755",
               "--mount", "type=bind,src=" + str(probe) + ",dst=/probe/runtime_probe.py,readonly",
               "--mount", "type=bind,src=" + str(ROOT / "tools/bazel/oci/runtime_smoke.py") + ",dst=/probe/runtime_smoke.py,readonly",
               "--mount", "type=bind,src=" + str(report_path) + ",dst=/probe/validation.json,readonly",
               "--entrypoint", "/usr/bin/python3", tag, "-I", "-B", "/probe/runtime_probe.py",
               "--validation", "/probe/validation.json"]
    if debug:
        command.append("--debug")
    return command


def parse_probe(stdout, debug, expected_count):
    records = [json.loads(line) for line in stdout.splitlines() if line.startswith('{"status":')]
    check.require(len(records) == 1, "installed probe did not return exactly one receipt")
    report = records[0]
    check.require(report["status"] == "passed" and report["architecture"] == "amd64"
                  and report["variant"] == ("debug" if debug else "runtime"), "installed probe identity differs")
    check.require(report["loader_checks"] == report["source_elf_count"] == expected_count and expected_count > 0,
                  "incomplete installed ELF validation")
    checks = report["debug_checks"]
    check.require(len(checks) == (3 if debug else 0), "incomplete installed symbol validation")
    if debug:
        check.require({(item["path"], item["symbol"]) for item in checks} == {
            ("usr/bin/simple_switch_grpc", "main"), ("usr/lib/python3/dist-packages/_nnpy.so", "_cffi_f_nn_socket"),
            ("usr/bin/pi_CLI_dummy", "main")},
            "installed debug targets differ")
        check.require(all(not item["without_symbols"].get("found") and item["with_symbols"].get("found")
                          for item in checks), "missing installed GDB positive/negative evidence")
        launcher = next(item for item in checks if item["path"] == "usr/bin/pi_CLI_dummy")
        check.require(all(launcher.get("debug_forwarding", {}).get(key) == value for key, value in {
            "provider": "usr/lib/x86_64-linux-gnu/libpicli.so.0", "symbol": "main", "source": "main.c"}.items()),
            "PI installed debug forwarding differs")
        found = launcher["with_symbols"]
        check.require(found.get("function") == "main" and found.get("source") == "main.c"
                      and found.get("line", 0) > 0 and found.get("type"),
                      "PI implementation lacks installed source/line/type evidence")
    startup = report["startup"]
    check.require(startup["entrypoint"] == "/usr/bin/start.sh" and startup["stable_installed_process"]
                  and startup["interfaces_up"] == ["eth1", "eth2"], "original startup was not verified")
    check.require(startup["switch_arguments"] == ["simple_switch_grpc", "--interface", "0@eth1", "--interface",
                  "1@eth2", "--log-console", "--no-p4"], "installed switch arguments differ")
    check.require(startup["p4runtime"] == {"api_version": "1.3.0", "device_id": 0,
        "primary_arbitration": "passed", "no_pipeline_get_config": "FAILED_PRECONDITION"},
        "missing real no-pipeline P4Runtime evidence")
    check.require(startup["nanomsg_clients"] == {
        "bm_nanomsg_events": {"packet_in_decoded": True, "config_change_observed": True},
        "bm_p4dbg": {"attach_detach": True, "request_types": [18, 19]}}, "installed client protocol evidence differs")
    safe_json(report)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", type=Path, required=True)
    args = parser.parse_args()
    directory = args.artifacts.resolve()
    check.require(platform.machine() == "x86_64", "loaded DASH engine requires native AMD64")
    clean_source(directory)
    identity = source_identity()
    report_path = directory / "validation.json"
    report_hash = check.sha(report_path)
    report = json.loads(report_path.read_text())
    check.require(report["status"] == "passed" and report["architecture"] == "amd64"
                  and report["revision"] == identity["revision"], "runtime checkout differs from validated source")
    verify_artifacts(directory, report)
    results = {}
    for name in IMAGES:
        expected_path = directory / (name + ".validation.json")
        expected_hash = check.sha(expected_path)
        expected = json.loads(expected_path.read_text())
        check.require(expected["status"] == "passed", "image static validation did not pass")
        archive = directory / (name + ".gz")
        identifier = uuid.uuid4().hex
        tag, container = "sonic-dash-engine-check:" + identifier, "sonic-dash-engine-probe-" + identifier
        debug = name.endswith("-dbg")
        try:
            loaded, config, _ = validate_runtime.load_archive(archive, tag, expected["config_digest"])
            validate_runtime.verify_loaded_image(loaded, config, expected["config_digest"])
            check.require(loaded["Os"] == "linux" and loaded["Architecture"] == "amd64", "loaded platform differs")
            check.require(config["rootfs"] == expected["rootfs"], "loaded layers differ from validated OCI")
            completed = subprocess.run(container_command(tag, container, report_path, debug),
                                       text=True, capture_output=True, timeout=900)
            (directory / (name + ".runtime.log")).write_text(completed.stdout + completed.stderr)
            check.require(completed.returncode == 0, name + " installed probe failed:\n" + completed.stdout + completed.stderr)
            count = len(report["native"] if debug else report["runtime_native"])
            result = parse_probe(completed.stdout, debug, count)
            check.require(check.sha(archive) == report["artifacts"][archive.name]["sha256"]
                          and check.sha(report_path) == report_hash and check.sha(expected_path) == expected_hash,
                          "validated inputs changed during installed checks")
            results[name] = {**result, "revision": identity["revision"], "config_and_layers_verified": True,
                             "archive_sha256": check.sha(archive), "static_validation_sha256": expected_hash}
            safe_json(results[name])
            (directory / (name + ".runtime.json")).write_text(json.dumps(results[name], indent=2) + "\n")
        finally:
            # Stop only this private probe on a timeout; retain all shared caches.
            subprocess.run(["docker", "rm", "--force", container], capture_output=True, check=False, timeout=30)
            subprocess.run(["docker", "image", "rm", tag], capture_output=True, check=False, timeout=30)
    clean_source(directory)
    check.require(source_identity() == identity, "runtime checkout changed during verification")
    result = {"status": "passed", **identity, "architecture": "amd64", "images": results}
    safe_json(result)
    (directory / "loaded-validation.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"status": "passed", "images": list(results)}))


if __name__ == "__main__":
    main()
