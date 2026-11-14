#!/usr/bin/env python3
"""Require the three Broadcom SAI binaries to report the same OCP API version."""

import argparse
import ctypes
from pathlib import Path
import re
import subprocess
import sys
import uuid


class VersionCheckError(Exception):
    pass


def parse_version(text, source):
    if not re.fullmatch(r"[1-9]\d*", text.strip()):
        raise VersionCheckError(f"Invalid SAI API version from {source}: {text!r}")
    return int(text.strip())


def linked_library(dependencies, name):
    matches = re.findall(
        r"^\s*" + re.escape(name) + r"\.so(?:\.[^\s]+)?\s+=>\s+(/[^\s]+)\s+\(",
        dependencies, re.MULTILINE,
    )
    if len(matches) != 1:
        raise VersionCheckError(f"Cannot uniquely resolve {name} in syncd dependencies:\n{dependencies}")
    return matches[0]


def query_runtime(syncd):
    dependencies = subprocess.run(
        ["ldd", str(syncd)], check=True, capture_output=True, text=True, timeout=15,
    ).stdout
    vendor = ctypes.CDLL(linked_library(dependencies, "libsai"))
    vendor.sai_query_api_version.argtypes = [ctypes.POINTER(ctypes.c_uint64)]
    vendor.sai_query_api_version.restype = ctypes.c_int32
    version = ctypes.c_uint64()
    status = vendor.sai_query_api_version(ctypes.byref(version))
    if status != 0:
        raise VersionCheckError(f"sai_query_api_version failed: status={status}")
    return parse_version(str(version.value), syncd)


def query_image(image):
    image_id = subprocess.run(
        ["docker", "inspect", "--format", "{{.Id}}", image],
        check=True, capture_output=True, text=True, timeout=30,
    ).stdout.strip()
    if not re.fullmatch(r"sha256:[0-9a-f]{64}", image_id):
        raise VersionCheckError(f"Invalid image ID for {image}: {image_id!r}")
    name = "sai-api-check-" + uuid.uuid4().hex
    print(f"Checking {image} ({image_id})", flush=True)
    probe_succeeded = False
    try:
        result = subprocess.run(
            ["docker", "run", "--name", name, "--network", "none", "-i",
             "--entrypoint", "python3", image_id, "-", "--syncd", "/usr/bin/syncd"],
            input=Path(__file__).read_text(encoding="utf-8"), text=True, capture_output=True, check=True, timeout=60,
        )
        probe_succeeded = True
    finally:
        try:
            subprocess.run(["docker", "rm", "--force", "--volumes", name], check=True, timeout=30)
        except (OSError, subprocess.SubprocessError) as error:
            if probe_succeeded:
                raise
            print(f"SAI probe container cleanup failed for {name}: {error}", file=sys.stderr)
    versions = re.findall(r"^SAI_API_VERSION=(\d+)$", result.stdout, re.MULTILINE)
    if len(versions) != 1:
        raise VersionCheckError(f"Expected one SAI API version from {image}:\n{result.stdout}")
    version = parse_version(versions[0], image)
    print(f"SAI API version: {image}, vendor={version}", flush=True)
    return version


def compare_versions(results):
    versions = [parse_version(path.read_text(encoding="utf-8"), path) for path in results]
    print("Broadcom SAI API versions: " + ", ".join(
        f"{path.name}={version}" for path, version in zip(results, versions)
    ), flush=True)
    if len(set(versions)) != 1:
        raise VersionCheckError("Broadcom XGS, legacy-TH and DNX vendor API versions must be equal")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--image")
    source.add_argument("--compare", nargs=3, type=Path)
    source.add_argument("--syncd", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if bool(args.image) != bool(args.output):
        parser.error("--image and --output must be specified together")
    try:
        if args.image:
            args.output.write_text(f"{query_image(args.image)}\n", encoding="utf-8")
        elif args.compare:
            compare_versions(args.compare)
        else:
            print(f"SAI_API_VERSION={query_runtime(args.syncd)}", flush=True)
    except (VersionCheckError, OSError, AttributeError, subprocess.SubprocessError) as error:
        print(f"SAI API version check failed: {error}", file=sys.stderr)
        if isinstance(error, subprocess.CalledProcessError) and error.stderr:
            print(error.stderr, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
