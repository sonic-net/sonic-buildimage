#!/usr/bin/env python3
"""Build and inspect the SWSS source layers on a native AMD64 Trixie runner."""

import argparse
from functools import partial
import json
from pathlib import Path
import platform
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(Path(__file__).resolve().parent))
from tools.bazel.ci import artifact_validation, bazel_commands, build, command_log, resolution
import package_contract


execute = partial(command_log.execute, cwd=ROOT)
# These targets create tar archives, never Debian packages. Make supplies the
# config-engine base and Scapy wheel when it assembles the complete OCI image.
TARGETS = {
    "swss.tar": "@sonic_swss//dist:swss_pkg",
    "protobuf.tar": "@sonic_dash_api//:protobuf_runtime_pkg",
    "rdeps.tar": "//dockers/docker-orchagent:rdeps",
    "config.tar": "//dockers/docker-orchagent/config:files",
    "debug-symbols.tar": "//dockers/docker-orchagent:rdeps_debug_symbols",
}
OPTIONS = [
    "--jobs=4", "--local_resources=cpu=4", "--local_resources=memory=10000",
    "--lockfile_mode=update", "--noshow_progress", "--color=no", "--curses=no",
]
TESTS = [
    "//tools/bazel/tests:apt_selection_test",
    "//tools/bazel/tests:select_apt_payloads_test",
    "//tools/bazel/tests:sonic_py_common_test",
    "//tools/bazel/tests:sonic_py_common_full_test",
    "//tools/bazel/tests:sonic_config_engine_full_test",
    "//tools/bazel/tests:sonic_python_wheels_test",
    "//tools/bazel/tests:containercfgd_full_test",
    "//tools/bazel/tests:containercfgd_wheel_test",
    "//src/sonic-config-engine:sonic_cfggen_cli_test",
    "//tools/bazel/tests:swss_common_rust_test",
    "//tools/bazel/tests:test_dpkg_patterns_up_to_date",
    "//tools/bazel/tests:manifest_labels_test",
    "//tools/bazel/tests:manifest_labels_action_test",
    "//tools/bazel/tests:swss_render_test",
]


def verify_packages(paths, source):
    runtime = artifact_validation.payload(paths["swss.tar"])
    programs = package_contract.swss_contract(source, runtime)
    dependencies = artifact_validation.payload(paths["rdeps.tar"])
    for name, metadata in runtime.items():
        if metadata["kind"] != "directory":
            artifact_validation.require(dependencies.get(name) == metadata, "runtime layer changed SWSS payload: " + name)
    protobuf = artifact_validation.payload(paths["protobuf.tar"], require_root=False)
    package_contract.source_protobuf_contract(dependencies, protobuf)
    debug = artifact_validation.debug_archives(
        paths["rdeps.tar"], paths["debug-symbols.tar"], expected_machine=62)
    required_debug = set(programs) | {"usr/lib/libdashapi.so", "usr/lib/python3/dist-packages/dash_api/_utils.so", package_contract.PROTOBUF_RUNTIME}
    artifact_validation.require(required_debug <= {pair["path"] for pair in debug["debug_pairs"]}, "incomplete SWSS/DASH/protobuf debug coverage")
    configuration = artifact_validation.payload(paths["config.tar"])
    artifact_validation.require(configuration.get("usr/bin/docker-init.sh", {}).get("mode") == 0o755,
                     "missing executable rendered SWSS entrypoint")
    return {"programs": programs, **debug,
            "source_contract": {name: artifact_validation.sha(source / name)
                                for name in package_contract.SWSS_CONTRACT_INPUTS},
            "scope": "Package payload, architecture, build IDs, DWARF and debuglink CRC; no container or boot execution."}


def source_directory(bazel, options, directory, receipt):
    source = build.source_directory(ROOT, directory, receipt, TARGETS["swss.tar"],
                                    bazel=bazel, options=options, name="swss-source")
    if not (source / "dist/BUILD.bazel").is_file():
        raise ValueError("Resolved SWSS source lacks its install declarations: " + str(source))
    return source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--bazel", default="bazel")
    args = parser.parse_args()
    directory = args.artifacts.resolve()
    if directory.exists() and any(directory.iterdir()):
        parser.error("artifact directory must be empty: " + str(directory))
    directory.mkdir(parents=True, exist_ok=True)
    receipt = {"status": "running", "commands": [],
               "scope": "SWSS source layers and matching debug symbols; no complete OCI image or VS installer."}
    started = time.monotonic()
    try:
        if platform.machine() != "x86_64" or platform.freedesktop_os_release().get("VERSION_CODENAME") != "trixie":
            raise ValueError("SWSS source-layer CI requires native AMD64 Debian Trixie")
        receipt["platform"] = platform.platform()
        receipt["revision"] = execute(["git", "rev-parse", "HEAD"], directory, receipt, "revision").strip()
        version = execute([args.bazel, "--version"], directory, receipt, "version").strip()
        if version != "bazel " + (ROOT / ".bazelversion").read_text().strip():
            raise ValueError("Bazel version does not match .bazelversion: " + version)
        receipt["bazel_version"] = version
        if (ROOT / "MODULE.bazel.lock").exists():
            raise ValueError("CI must start without a preexisting MODULE.bazel.lock")
        execute([
            "make", "-f", "tools/bazel/prepare_manifests.mk",
            "MANIFEST_METADATA=rules/docker-orchagent.mk",
            "BUILD_WITH_BAZEL_WHEN_AVAILABLE=y", "BLDENV=trixie", "CONFIGURED_PLATFORM=vs",
            "CONFIGURED_ARCH=amd64", "ENABLE_ASAN=n", "DBG_IMAGE_MARK=dbg",
            "DOCKERS_PATH=dockers",
        ], directory, receipt, "make-manifests")
        options = OPTIONS
        receipt["action_audit"] = bazel_commands.audit_targets(
            args.bazel, options, [*TESTS, *TARGETS.values()],
            workspace=ROOT, output=directory / "action-audit.json")
        execute([args.bazel, "test", *options, "--nocache_test_results",
                 "--build_event_json_file=" + str(directory / "test-events.jsonl"),
                 *TESTS], directory, receipt, "bazel-tests")
        receipt["tests"] = {target: "passed" for target in TESTS}
        paths = build.collect_archives(ROOT, directory, receipt, TARGETS,
                                       bazel=args.bazel, options=options)
        source = source_directory(args.bazel, options, directory, receipt)
        receipt["validation"] = verify_packages(paths, source)
        receipt["resolution"] = resolution.collect(ROOT, directory, bazel=[args.bazel])
        receipt["architecture"] = "amd64"
        receipt["status"] = "passed"
    except Exception as error:
        receipt.update(status="failed", error=str(error))
        raise
    finally:
        receipt["elapsed_seconds"] = time.monotonic() - started
        (directory / "receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
