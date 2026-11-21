#!/usr/bin/env python3
"""Explicitly refresh DHCP's wheel-derived runtime lock using the shared tool."""

import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import bazel_commands


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bazel", default="bazel")
    parser.add_argument("--uv", default="uv")
    args = parser.parse_args()
    target = "//dockers/docker-dhcp-relay/python:plan"
    evidence = ROOT / "artifacts/dhcp-relay-python-lock"
    bazel_commands.audit_targets(args.bazel, [], [target, "@sonic_build_infra//python:runtime_lock"],
                                 workspace=ROOT, output=evidence / "action-audit.json")
    subprocess.run([args.bazel, "build", target], cwd=ROOT, check=True)
    outputs = subprocess.check_output([args.bazel, "cquery", target, "--output=files"],
                                      cwd=ROOT, text=True).splitlines()
    if len(outputs) != 1:
        raise ValueError("runtime plan must have exactly one output")
    # Another output base may repoint the workspace's bazel-out symlink.
    # This location-only request needs no target platform; Bazel info does not
    # resolve the workspace's apparent external platform repository aliases.
    execution_root = Path(subprocess.check_output(
        [args.bazel, "info", "execution_root", "--platforms=",
         "--host_platform=@@local_config_platform//:host"], cwd=ROOT, text=True).strip())
    directory = Path(__file__).resolve().parent
    subprocess.run([args.bazel, "run", "@sonic_build_infra//python:runtime_lock", "--", "update",
                    "--plan", str(execution_root / outputs[0]), "--uv", args.uv,
                    "--requirements", str(directory / "requirements.in"),
                    "--lock", str(directory / "requirements_lock.txt"),
                    "--inputs", str(directory / "requirements_lock.inputs.json")], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
