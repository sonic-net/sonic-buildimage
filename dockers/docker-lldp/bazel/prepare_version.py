#!/usr/bin/env python3
"""Publish Make's selected image version as a declared OCI environment input."""

import argparse
import os
from pathlib import Path
import tempfile


def publish(output, version):
    # rules_oci expands shell-style variable references in environment inputs.
    # Reject them before publication so the image cannot silently change Make's
    # version, and preserve the previous declared input on failure.
    if not version or any(char in version for char in "\r\n\0$"):
        raise ValueError("SONIC_IMAGE_VERSION must be nonempty, single-line, and contain no dollar signs")
    content = ("DEBIAN_FRONTEND=noninteractive\nIMAGE_VERSION=" + version + "\n").encode("utf-8")
    if output.exists() and output.read_bytes() == content:
        return
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=output.parent, prefix=".version-", delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(content)
            stream.flush()
            os.fsync(stream.fileno())
        temporary.chmod(0o644)
        temporary.replace(output)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    publish(args.output, os.environ.get("SONIC_IMAGE_VERSION", ""))


if __name__ == "__main__":
    main()
