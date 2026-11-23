#!/usr/bin/env python3
"""Write an unsigned ONIE header and hash the tar payload in one pass."""

import argparse
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import tempfile

HEADER_SIZE = 4096
MARKER = b"exit_marker\n"


def write_payload(directory, template, destination):
    directory, template, destination = map(Path, (directory, template, destination))
    body = template.read_bytes()
    if (not body.endswith(b"\n" + MARKER)
            or body.count(b"\n" + MARKER) != 1
            or body.count(b"%%IMAGE_SHA1%%") != 1
            or body.count(b"%%PAYLOAD_IMAGE_SIZE%%") != 1):
        raise ValueError("Unexpected ONIE header template")
    # Reserve room for a SHA1 checksum, the decimal payload size and a comment.
    if len(body) + 64 > HEADER_SIZE:
        raise ValueError("ONIE header exceeds reserved space")
    if destination.resolve().is_relative_to((directory / "installer").resolve()):
        raise ValueError("ONIE destination must be outside the payload tree")
    try:
        previous = destination.lstat()
    except FileNotFoundError:
        mask = os.umask(0)
        os.umask(mask)
        mode = stat.S_IMODE(template.stat().st_mode) & ~mask
    else:
        if not stat.S_ISREG(previous.st_mode):
            raise ValueError("ONIE destination must be a regular file")
        mode = stat.S_IMODE(previous.st_mode)
    pending = None
    process = None
    try:
        with tempfile.NamedTemporaryFile(
                mode="w+b", dir=destination.parent, prefix=".onie-payload-", delete=False) as output:
            pending = Path(output.name)
            output.write(b"\0" * HEADER_SIZE)
            digest = hashlib.sha1(usedforsecurity=False)
            size = 0
            process = subprocess.Popen(
                ["tar", "-C", str(directory), "-cf", "-", "installer"],
                stdout=subprocess.PIPE)
            try:
                while chunk := process.stdout.read(1024 * 1024):
                    output.write(chunk)
                    digest.update(chunk)
                    size += len(chunk)
                process.stdout.close()
                if process.wait() != 0:
                    raise RuntimeError("ONIE payload tar failed")
            finally:
                process.stdout.close()
                if process.poll() is None:
                    process.terminate()
                    process.wait()
            header = body.replace(b"%%IMAGE_SHA1%%", digest.hexdigest().encode("ascii"))
            header = header.replace(b"%%PAYLOAD_IMAGE_SIZE%%", str(size).encode("ascii"))
            padding = HEADER_SIZE - len(header)
            if padding < 2:
                raise ValueError("ONIE header exceeds reserved space")
            comment = b"#" + b" " * (padding - 2) + b"\n"
            header = header[:-len(MARKER)] + comment + MARKER
            output.seek(0)
            output.write(header)
            os.fchmod(output.fileno(), mode)
        pending.replace(destination)
    finally:
        if pending is not None:
            pending.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("template", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    write_payload(args.directory, args.template, args.destination)


if __name__ == "__main__":
    main()
