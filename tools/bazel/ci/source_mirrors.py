#!/usr/bin/env python3
"""Reuse the registry's verified source mirrors with Bazel's normal downloader."""

import argparse
import hashlib
from pathlib import Path
from urllib.request import urlopen

ROOT = Path(__file__).resolve().parents[3]
URL = "https://raw.githubusercontent.com/securely1g/sonic-bazel-registry/9697f3db62fc58a7a70b0d876abe0a849133a00d/ci/source_mirrors.cfg"
SHA256 = "622830b8adf42d229a53b60ea0956de4a4a3c119d0639479be78817994d67327"
SNAPSHOT_FIRST = (rb"rewrite snapshot\.debian\.org/(archive/(?:debian|debian-security)/"
                  rb"[0-9]{8}T[0-9]{6}Z/.+) https://snapshot.debian.org/$1" + b"\n")


def verify(configuration):
    """Accept only the selected shared policy; source hashes remain Bazel inputs."""
    if hashlib.sha256(configuration).hexdigest() != SHA256:
        raise ValueError("Source mirror configuration checksum mismatch")
    return configuration


def download():
    with urlopen(URL, timeout=60) as response:
        return verify(response.read())


def prepare(output, local=ROOT / "tools/bazel/debian_mirrors.cfg", *, prefer_debian_snapshot=False):
    """Keep every local and registry fallback for the pinned inputs."""
    shared = download()
    configuration = local.read_bytes() + b"\n" + shared
    if prefer_debian_snapshot:
        configuration = SNAPSHOT_FIRST + configuration
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(configuration)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--prefer-debian-snapshot", action="store_true",
                        help="Try the original timestamped Debian and security URLs first.")
    args = parser.parse_args()
    prepare(args.output, prefer_debian_snapshot=args.prefer_debian_snapshot)


if __name__ == "__main__":
    main()
