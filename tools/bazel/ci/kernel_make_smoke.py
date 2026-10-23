#!/usr/bin/env python3
"""Verify Make consumes normal kernel DEBs without rewriting unchanged files."""

import argparse
import json
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--architecture", required=True, choices=("amd64", "arm64"))
    parser.add_argument("--debs-directory", type=Path, default=Path("target/debs/trixie"))
    args = parser.parse_args()
    workspace = Path.cwd()
    packages = args.debs_directory.resolve(strict=True)
    manifest = json.loads((packages / "kernel-packages.json").read_text())
    names = [item["name"] for item in manifest["packages"]]
    if len(names) != 5 or manifest["architecture"] != args.architecture:
        raise ValueError("Make smoke requires the five selected native kernel packages")
    slave = (workspace / "slave.mk").read_text()
    derived_recipe = slave[slave.index("# Rules for derived debian packages"):
                           slave.index("# Rules for extra debian packages")]
    functions = (workspace / "rules/functions").read_text()
    start = functions.index("define add_derived_package\n")
    derived_macro = functions[start:functions.index("endef", start) + len("endef")]
    with tempfile.TemporaryDirectory(prefix="kernel-make-smoke-", dir=workspace / "target") as temporary, \
            tempfile.NamedTemporaryFile(prefix="unrelated-make-smoke-", suffix=".deb", dir=packages) as unrelated:
        unrelated.write(b"Unrelated package must survive kernel preparation.\n")
        unrelated.flush()
        unrelated_path = Path(unrelated.name)
        unrelated_bytes = unrelated_path.read_bytes()
        before = {path.name: (path.stat().st_size, path.stat().st_mtime_ns)
                  for path in packages.iterdir() if path.is_file()}
        directory = Path(temporary)
        (directory / ".platform").touch()
        makefile = directory / "smoke.mk"
        makefile.write_text(
            ".DEFAULT_GOAL := all\nSHELL := /bin/bash\n.SHELLFLAGS := -ec\n.ONESHELL:\n.SECONDEXPANSION:\n"
            "vpath .platform " + str(directory) + "\n"
            "SRC_PATH := src\nTARGET_PATH := target\nDEBS_PATH := " + str(packages) + "\n"
            "CONFIGURED_ARCH := " + args.architecture + "\nCONFIGURED_PLATFORM := vs\nBLDENV := trixie\n"
            "SECURE_UPGRADE_MODE := no_sign\nBUILD_WITH_BAZEL_WHEN_AVAILABLE := y\n"
            + derived_macro + "\ninclude rules/linux-kernel.mk\ninclude rules/linux-kernel.dep\n"
            "all: $(SONIC_BAZEL_KERNEL_INPUTS)\n"
            "\t@test -z '$(SONIC_MAKE_DEBS)$(SONIC_COPY_DEBS)$(SONIC_DERIVED_DEBS)'\n"
            "\t@test '$(BUILD_WITH_BAZEL_WHEN_AVAILABLE)' = y\n"
            + derived_recipe)
        command = ["make", "--no-print-directory", "--no-builtin-rules", "-j4", "-f", str(makefile)]
        for _ in range(2):
            subprocess.run(command, cwd=workspace, check=True)
        concurrent = [subprocess.Popen(command, cwd=workspace) for _ in range(2)]
        results = [process.wait() for process in concurrent]
        if any(results):
            raise ValueError("Concurrent Make kernel verification failed: " + str(results))
        after = {path.name: (path.stat().st_size, path.stat().st_mtime_ns)
                 for path in packages.iterdir() if path.is_file()}
        if before != after or unrelated_path.read_bytes() != unrelated_bytes:
            raise ValueError("Make changed retained kernel outputs or unrelated packages")
    print("Make uses normal DEBs; repeated and concurrent requests preserve their timestamps and unrelated packages.")


if __name__ == "__main__":
    main()
