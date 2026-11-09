#!/usr/bin/env python3
"""Require the selected header and final container's SAI API versions to match."""

import argparse
import ctypes
from pathlib import Path
import re
import subprocess
import sys
import uuid


class VersionCheckError(Exception):
    pass


def header_version(header):
    text = header.read_text(encoding="utf-8")
    parts = []
    for name in ("MAJOR", "MINOR", "REVISION"):
        values = re.findall(r"^\s*#\s*define\s+SAI_" + name + r"\s+(\d+)\s*$", text, re.MULTILINE)
        if len(values) != 1:
            raise VersionCheckError(f"Expected one literal SAI_{name} in {header}")
        parts.append(int(values[0]))
    if parts[1] >= 100 or parts[2] >= 100:
        raise VersionCheckError(f"Invalid SAI version components in {header}: {parts}")
    return parts[0] * 10000 + parts[1] * 100 + parts[2]


def linked_library(dependencies, name):
    matches = re.findall(
        r"^\s*" + re.escape(name) + r"\.so(?:\.[^\s]+)?\s+=>\s+(/[^\s]+)\s+\(",
        dependencies, re.MULTILINE,
    )
    if len(matches) != 1:
        raise VersionCheckError(f"Cannot uniquely resolve {name} in syncd dependencies:\n{dependencies}")
    return matches[0]


def check_runtime(expected, syncd):
    dependencies = subprocess.run(
        ["ldd", str(syncd)], check=True, capture_output=True, text=True, timeout=15,
    ).stdout
    metadata = ctypes.CDLL(linked_library(dependencies, "libsaimetadata"))
    vendor = ctypes.CDLL(linked_library(dependencies, "libsai"))
    metadata.sai_metadata_query_api_version.argtypes = []
    metadata.sai_metadata_query_api_version.restype = ctypes.c_uint64
    vendor.sai_query_api_version.argtypes = [ctypes.POINTER(ctypes.c_uint64)]
    vendor.sai_query_api_version.restype = ctypes.c_int32
    compiled = metadata.sai_metadata_query_api_version()
    version = ctypes.c_uint64()
    status = vendor.sai_query_api_version(ctypes.byref(version))
    if status != 0:
        raise VersionCheckError(f"sai_query_api_version failed: status={status}")
    print(f"SAI API versions: header={expected}, metadata={compiled}, vendor={version.value}", flush=True)
    if min(expected, compiled, version.value) < 10900:
        raise VersionCheckError("SAI API versions must be at least 1.9.0")
    if expected != compiled or expected != version.value:
        raise VersionCheckError("SAI header, compiled metadata and vendor API versions must be equal")


def check_image(image, expected):
    image_id = subprocess.run(
        ["docker", "inspect", "--format", "{{.Id}}", image],
        check=True, capture_output=True, text=True, timeout=30,
    ).stdout.strip()
    if not re.fullmatch(r"sha256:[0-9a-f]{64}", image_id):
        raise VersionCheckError(f"Invalid image ID for {image}: {image_id!r}")
    name = "sai-api-check-" + uuid.uuid4().hex
    print(f"Checking {image} ({image_id})", flush=True)
    try:
        subprocess.run(
            ["docker", "run", "--name", name, "--network", "none", "-i",
             "--entrypoint", "python3", image_id, "-", "--expected-version", str(expected)],
            input=Path(__file__).read_text(encoding="utf-8"), text=True, check=True, timeout=60,
        )
    finally:
        subprocess.run(["docker", "rm", "--force", "--volumes", name], check=True, timeout=30)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--header", type=Path)
    source.add_argument("--expected-version", type=int)
    parser.add_argument("--image")
    parser.add_argument("--syncd", type=Path, default=Path("/usr/bin/syncd"))
    args = parser.parse_args()
    if bool(args.header) != bool(args.image):
        parser.error("--header and --image must be specified together")
    try:
        if args.header:
            check_image(args.image, header_version(args.header))
        else:
            check_runtime(args.expected_version, args.syncd)
    except (VersionCheckError, OSError, AttributeError, subprocess.SubprocessError) as error:
        print(f"SAI API version check failed: {error}", file=sys.stderr)
        if isinstance(error, subprocess.CalledProcessError) and error.stderr:
            print(error.stderr, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
