#!/usr/bin/env python3
"""Record fixed-label installer timings without capturing command arguments or output."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time


STEPS = ("rootfs-restore", "rootfs-extract", "extensions", "initramfs",
         "version-collection", "remove-gcc", "remove-eatmydata", "apt-autoremove",
         "apt-clean", "cleanup-files", "size-scan", "size-optimize", "squashfs")


def run(step, command, directory):
    started = time.monotonic()
    result = subprocess.run(command)
    elapsed = time.monotonic() - started
    temporary = None
    try:
        directory = Path(directory)
        directory.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(mode="w", dir=directory, prefix=step + "-",
                                         suffix=".tmp", delete=False) as stream:
            temporary = Path(stream.name)
            json.dump({"step": step, "seconds": elapsed, "returncode": result.returncode}, stream)
            stream.write("\n")
        temporary.replace(temporary.with_suffix(".json"))
    except OSError:
        pass
    finally:
        if temporary is not None:
            try:
                temporary.unlink(missing_ok=True)
            except OSError:
                pass
    return result.returncode if result.returncode >= 0 else 128 - result.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("step", choices=STEPS)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if not args.command:
        parser.error("a command is required")
    raise SystemExit(run(args.step, args.command, os.environ["SONIC_INSTALLER_EVIDENCE_DIR"]))


if __name__ == "__main__":
    main()
