#!/usr/bin/env python3
"""Publish Make's event service selection as a declared OCI environment input."""

import argparse
import os
from pathlib import Path
import tempfile


def publish(output, selection):
    if selection not in ("y", "n"):
        raise ValueError("INCLUDE_SYSTEM_EVENTD must be y or n")
    content = ("DEBIAN_FRONTEND=noninteractive\nINCLUDE_SYSTEM_EVENTD=" + selection + "\n").encode("utf-8")
    if output.exists() and output.read_bytes() == content:
        return
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=output.parent, prefix=".environment-", delete=False) as stream:
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
    publish(args.output, os.environ.get("INCLUDE_SYSTEM_EVENTD", "n"))


if __name__ == "__main__":
    main()
