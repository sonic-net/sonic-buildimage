#!/usr/bin/env python3
"""Remove one VS run checkout, never shared caches or Docker image storage."""

import argparse
import json
from pathlib import Path
import re
import subprocess


def checkout_path(workspace, run_id, run_attempt):
    if not workspace.is_absolute() or not workspace.is_dir():
        raise ValueError("Cleanup workspace must be an existing absolute directory")
    if not all(re.fullmatch(r"[1-9][0-9]*", value) for value in (run_id, run_attempt)):
        raise ValueError("Cleanup requires positive numeric run ID and attempt")
    checkout = workspace.resolve(strict=True) / f"vs-{run_id}-{run_attempt}"
    if checkout.is_symlink():
        raise ValueError(f"Refusing redirected VS checkout: {checkout}")
    if checkout.exists() and not checkout.is_dir():
        raise ValueError(f"VS checkout is not a directory: {checkout}")
    return checkout


def check_mounts(checkout):
    for line in Path("/proc/self/mountinfo").read_text().splitlines():
        mount = Path(re.sub(r"\\([0-7]{3})", lambda match: chr(int(match[1], 8)), line.split()[4]))
        if mount.is_relative_to(checkout):
            raise ValueError(f"Refusing cleanup while a mount remains in the cleanup path: {mount}")


def check_container_users(checkout):
    containers = subprocess.check_output(
        ["docker", "ps", "--all", "--quiet", "--no-trunc"], text=True).split()
    if not containers:
        return
    metadata = subprocess.check_output(
        ["docker", "inspect", "--format", "{{json .Mounts}}", *containers], text=True)
    for line in metadata.splitlines():
        for mount in json.loads(line):
            source = mount.get("Source")
            if not source:
                continue
            source = Path(source).resolve()
            if source.is_relative_to(checkout) or checkout.is_relative_to(source):
                raise ValueError(f"A Docker container still references the cleanup path: {source}")


def cleanup(workspace, run_id, run_attempt):
    checkout = checkout_path(workspace, run_id, run_attempt)
    if not checkout.exists():
        print(f"No VS checkout to remove: {checkout}")
        return
    identity = checkout.stat()
    check_container_users(checkout)
    check_mounts(checkout)
    current = checkout_path(workspace, run_id, run_attempt).stat()
    if (identity.st_dev, identity.st_ino) != (current.st_dev, current.st_ino):
        raise ValueError("VS checkout changed during cleanup checks")
    print(f"Removing this run's VS checkout: {checkout}", flush=True)
    # Builders may leave root-owned files. Do not chmod files that could be
    # hardlinked into retained caches, and do not cross filesystem boundaries.
    subprocess.run(
        ["sudo", "-n", "rm", "--recursive", "--force", "--one-file-system", "--", str(checkout)],
        check=True)
    if checkout.exists() or checkout.is_symlink():
        raise ValueError(f"VS checkout still exists after cleanup: {checkout}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workspace", type=Path, required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--run-attempt", required=True)
    args = parser.parse_args()
    try:
        cleanup(args.workspace, args.run_id, args.run_attempt)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"VS checkout cleanup failed: {error}\n")


if __name__ == "__main__":
    main()
