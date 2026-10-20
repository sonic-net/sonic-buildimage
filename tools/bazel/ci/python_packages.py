#!/usr/bin/env python3
"""Retain the Python wheels and evidence from their native package checks."""

import argparse
import json
from pathlib import Path
import platform
import shutil
import sys
import xml.etree.ElementTree as ElementTree
import zipfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation, build


WHEELS = {
    "sonic_containercfgd-1.0-py3-none-any.whl": "//src/sonic-containercfgd:sonic_containercfgd_wheel",
    "sonic_py_common-1.0-py3-none-any.whl": "//src/sonic-py-common:sonic_py_common_wheel",
    "sonic_config_engine-1.0-py3-none-any.whl": "//src/sonic-config-engine:sonic_config_engine_wheel",
}
TESTS = [
    "//tools/bazel/tests:installed_wheel_fixture_test",
    "//tools/bazel/tests:wheel_contract_test",
    "//tools/bazel/tests:containercfgd_full_test",
    "//tools/bazel/tests:containercfgd_wheel_test",
    "//tools/bazel/tests:sonic_py_common_test",
    "//tools/bazel/tests:sonic_py_common_full_test",
    "//tools/bazel/tests:sonic_config_engine_full_test",
    "//tools/bazel/tests:sonic_python_wheels_test",
    "//src/sonic-config-engine:sonic_cfggen_cli_test",
]
WHEEL_CHECKS = [
    ("sonic_python_wheels_test", "wheels.json", "installed_wheels", [
        "sonic_py_common-1.0-py3-none-any.whl", "sonic_config_engine-1.0-py3-none-any.whl",
    ]),
    ("containercfgd_wheel_test", "containercfgd-wheels.json", "installed_containercfgd", [
        "sonic_containercfgd-1.0-py3-none-any.whl",
    ]),
]
MACHINES = {"amd64": "x86_64", "arm64": "aarch64"}


def test_output(source, filename):
    """Read one required undeclared test output with either Bazel zip setting."""
    outputs = source / "test.outputs"
    if (outputs / filename).is_file():
        return (outputs / filename).read_bytes()
    with zipfile.ZipFile(outputs / "outputs.zip") as archive:
        return archive.read(filename)


def file_evidence(path, directory):
    return {"path": str(path.relative_to(directory)), "sha256": artifact_validation.sha(path)}


def test_result(path):
    """Require a complete, successful JUnit report without assuming test counts."""
    report = ElementTree.parse(path).getroot()
    suites = [suite for suite in report.iter("testsuite") if not suite.findall(".//testsuite")]
    counts = {name: sum(int(suite.get(name, "0")) for suite in suites)
              for name in ("tests", "failures", "errors", "skipped")}
    if (not suites or counts["tests"] <= 0 or any(value < 0 for value in counts.values())
            or counts["failures"] or counts["errors"]
            or report.findall(".//failure") or report.findall(".//error")
            or len(report.findall(".//testcase")) != counts["tests"]):
        raise ValueError(f"Incomplete or unsuccessful test report: {path}")
    return counts


def collect(workspace, directory, revision, architecture, *, bazel="bazel", options=()):
    """Export wheels and require matching successful native test evidence."""
    directory.mkdir(parents=True, exist_ok=True)
    receipt = {"status": "running", "revision": revision,
               "architecture": architecture, "commands": [], "tests": {}}
    try:
        if platform.machine() != MACHINES[architecture]:
            raise ValueError("Python package evidence must be collected on its native architecture")
        if revision["architecture"] != architecture:
            raise ValueError("Revision evidence has a different architecture")
        registered = [name for _, _, _, expected in WHEEL_CHECKS for name in expected]
        if len(registered) != len(WHEELS) or set(registered) != set(WHEELS):
            raise ValueError("Each retained wheel must have exactly one installed-wheel check")
        for target, _, _, _ in WHEEL_CHECKS:
            if "//tools/bazel/tests:" + target not in TESTS:
                raise ValueError("Installed-wheel check has no required test evidence: " + target)
        paths = build.collect_archives(workspace, directory, receipt, WHEELS,
                                       bazel=bazel, options=options)
        hashes = {name: artifact_validation.sha(path) for name, path in paths.items()}
        (directory / "wheels-sha256.txt").write_text("".join(
            f"{digest}  {name}\n" for name, digest in sorted(hashes.items())
        ))
        for target in TESTS:
            package, name = target.removeprefix("//").split(":")
            source = workspace / "bazel-testlogs" / package / name
            destination = directory / "tests" / name
            destination.mkdir(parents=True, exist_ok=True)
            evidence = {}
            for filename in ("test.log", "test.xml"):
                path = source / filename
                if not path.is_file() or not path.stat().st_size:
                    raise ValueError(f"Missing required {target} evidence: {filename}")
                output = destination / filename
                shutil.copyfile(path, output)
                evidence[filename] = file_evidence(output, directory)
            evidence["result"] = test_result(destination / "test.xml")
            if name.endswith("_full_test"):
                inventory = test_output(source, "pytest-inventory.json")
                collected = json.loads(inventory)
                if (collected["collected"] != len(collected["tests"])
                        or collected["collected"] != evidence["result"]["tests"]):
                    raise ValueError(f"Collected test inventory does not match the XML report: {target}")
                if not collected["python"].startswith("3.13."):
                    raise ValueError(f"Full source suite used a different Python version: {target}")
                output = destination / "pytest-inventory.json"
                output.write_bytes(inventory)
                evidence["inventory"] = file_evidence(output, directory)
            receipt["tests"][target] = evidence
        checked = set()
        for target, filename, receipt_key, expected in WHEEL_CHECKS:
            wheel_receipt = test_output(workspace / "bazel-testlogs/tools/bazel/tests" / target, "wheels.json")
            installed = json.loads(wheel_receipt)
            if installed["status"] != "passed" or installed["architecture"] != MACHINES[architecture]:
                raise ValueError("Installed-wheel receipt does not describe this successful native run: " + target)
            if not installed["python"].startswith("3.13."):
                raise ValueError("Installed-wheel test used a different Python version: " + target)
            if "python" in receipt and receipt["python"] != installed["python"]:
                raise ValueError("Installed-wheel checks used different Python versions")
            installed_hashes = {wheel["filename"]: wheel["sha256"] for wheel in installed["wheels"]}
            if (len(installed_hashes) != len(installed["wheels"])
                    or installed_hashes != {name: hashes[name] for name in expected}):
                raise ValueError("Retained wheels differ from the installed-wheel test inputs: " + target)
            (directory / filename).write_bytes(wheel_receipt)
            receipt[receipt_key] = file_evidence(directory / filename, directory)
            receipt["python"] = installed["python"]
            checked.update(expected)
        if checked != set(hashes):
            raise ValueError("A retained wheel has no installed-wheel check")
        receipt["status"] = "passed"
    except Exception as error:
        receipt.update(status="failed", error=str(error))
        raise
    finally:
        (directory / "receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--revision", type=Path, required=True)
    parser.add_argument("--architecture", choices=MACHINES, required=True)
    parser.add_argument("--bazel", default="bazel")
    parser.add_argument("--bazel-arg", action="append", default=[])
    args = parser.parse_args()
    collect(ROOT, args.artifacts.resolve(), json.loads(args.revision.read_text()),
            args.architecture, bazel=args.bazel, options=["--lockfile_mode=update", *args.bazel_arg])


if __name__ == "__main__":
    main()
