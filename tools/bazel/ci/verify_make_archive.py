#!/usr/bin/env python3
"""Exercise the shared Make export rule with real, source-only Bazel fixtures."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.tests.oci_base_fixture import docker_save, image_fixture

FIXTURES = {
    "docker-fixture.gz": "//tools/bazel/tests:archive-fixture.gz",
    "docker-fixture-dbg.gz": "//tools/bazel/tests:archive-fixture.gz",
    "docker-overlay.gz": "//tools/bazel/tests:oci-consumer-first.gz",
}
BASES = {"fixture-amd64.oci": "amd64", "fixture-arm64.oci": "arm64"}


def base_snapshot(directory):
    return {name: {
        "generation": os.readlink(directory / name),
        "link_mtime_ns": (directory / name).lstat().st_mtime_ns,
        "files": {str(path.relative_to(directory / name)): {
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "mtime_ns": path.stat().st_mtime_ns,
        } for path in sorted((directory / name).rglob("*")) if path.is_file()},
    } for name in BASES}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--bazel", default="bazel")
    parser.add_argument("--bazel-arg", action="append", default=[])
    args = parser.parse_args()
    architecture = {"x86_64": "amd64", "aarch64": "arm64"}.get(platform.machine())
    if architecture is None:
        parser.error("Make archive checks require a native AMD64 or ARM64 runner")
    directory = args.artifacts.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    if any(character.isspace() or character in "$#" for character in str(directory)):
        parser.error("Make fixture output directory must not contain whitespace, $ or #")

    # These archives test routing and publication, not production debug symbols.
    # No fixture consumes SWSS inputs or builds a Debian package.
    makefile = directory / "fixture.mk"
    lines = [
        "SHELL = /bin/bash",
        ".SHELLFLAGS += -e",
        ".ONESHELL:",
        ".SECONDEXPANSION:",
        f"TARGET_PATH = {directory}",
        f"CONFIGURED_ARCH = {architecture}",
        "SONIC_BAZEL_DOCKER_IMAGES = docker-fixture.gz docker-overlay.gz",
        "SONIC_BAZEL_DBG_DOCKER_IMAGES = docker-fixture-dbg.gz",
        "SONIC_BAZEL_OCI_BASES = " + " ".join(BASES),
        ".PHONY: .platform",
        ".platform:",
    ]
    for name, architecture in BASES.items():
        # Existing archives model a Make package-cache hit: no Docker rebuild
        # recipe runs before the shared rule validates and publishes the base.
        archive = directory / (name + ".gz")
        docker_save(archive, *image_fixture(architecture=architecture))
        lines += [f"{name}_OCI_ARCHIVE = {archive}",
                  f"{name}_OCI_PLATFORM = linux/{architecture}"]
    for archive, label in FIXTURES.items():
        base = "fixture-arm64.oci" if archive == "docker-overlay.gz" else "fixture-amd64.oci"
        lines += [f"{archive}_BAZEL_TARGET = {label}",
                  f"{archive}_PATH = tools/bazel/tests",
                  f"{archive}_BAZEL_DEPENDS = $(TARGET_PATH)/{base}"]
    lines.append("include tools/bazel/docker.mk")
    makefile.write_text("\n".join(lines) + "\n")
    environment = dict(os.environ, BAZEL=args.bazel,
                       BAZEL_CONTAINER_ARGS=shlex.join(args.bazel_arg),
                       BAZEL_CONTAINER_CACHE_DIR="")
    command = ["make", "--no-print-directory", "-f", str(makefile),
               *[str(directory / archive) for archive in FIXTURES]]
    (directory / "command.json").write_text(json.dumps(command, indent=2) + "\n")
    snapshots = []
    bases = []
    for phase in ("first", "repeat"):
        with (directory / f"{phase}.log").open("w") as log:
            subprocess.run(command, cwd=ROOT, env=environment, check=True,
                           stdout=log, stderr=subprocess.STDOUT)
        snapshots.append({name: {
            "sha256": hashlib.sha256((directory / name).read_bytes()).hexdigest(),
            "size": (directory / name).stat().st_size,
            "mtime_ns": (directory / name).stat().st_mtime_ns,
        } for name in FIXTURES})
        bases.append(base_snapshot(directory))
    if snapshots[0] != snapshots[1] or not all(item["size"] for item in snapshots[0].values()):
        raise RuntimeError("repeated Make export changed an archive or its timestamp")
    if bases[0] != bases[1] or not all(item["files"] for item in bases[0].values()):
        raise RuntimeError("repeated Make preparation changed an OCI base or its timestamps")
    receipt = {"targets": FIXTURES, "first": snapshots[0], "repeat": snapshots[1],
               "bases": {"platforms": BASES, "first": bases[0], "repeat": bases[1]}}
    (directory / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print("Shared Make rules prepared two OCI bases and exported three archives; repeat timestamps match.")


if __name__ == "__main__":
    main()
