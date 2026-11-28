#!/usr/bin/env python3
"""Bound Bazel 8.5.1 prepared repositories on an idle, dedicated VS agent."""

import argparse
from contextlib import ExitStack
from dataclasses import dataclass
import fcntl
import json
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import time
import uuid

from .cleanup_vs_workspace import check_container_users, check_mounts


GIB = 1024 ** 3
SCOPES = ("master", "review", "dispatch")


@dataclass
class Entry:
    contents: Path
    marker: Path
    last_used: float


def checked_path(path, *, directory):
    metadata = path.lstat()
    expected = stat.S_ISDIR if directory else stat.S_ISREG
    if path.resolve() != path or not expected(metadata.st_mode):
        raise ValueError(f"Refusing unexpected repository cache path: {path}")
    return metadata


def acquire_lock(stack, scope):
    lock = scope / "gc_lock"
    created = False
    try:
        descriptor = os.open(
            lock, os.O_RDWR | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
        created = True
    except FileExistsError:
        descriptor = os.open(lock, os.O_RDWR | os.O_NOFOLLOW)
    stack.callback(os.close, descriptor)
    if created and os.geteuid() == 0:
        # A root supervisor must not leave its new lock owned by root.
        owner = scope.stat()
        os.fchown(descriptor, owner.st_uid, owner.st_gid)
    metadata = os.fstat(descriptor)
    if not stat.S_ISREG(metadata.st_mode) or metadata.st_nlink != 1:
        raise ValueError(f"Refusing unexpected repository cache lock: {lock}")
    try:
        # Java FileChannel locks use POSIX record locks, not BSD flock.
        fcntl.lockf(descriptor, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError as error:
        raise ValueError(f"Repository cache is in use: {scope}") from error
    if metadata.st_ino != lock.lstat().st_ino:
        raise ValueError(f"Repository cache lock changed: {lock}")


def inventory(scopes):
    entries, trash = [], []
    for scope in scopes:
        for group in sorted(scope.iterdir()):
            if group.name == "gc_lock":
                checked_path(group, directory=False)
                continue
            checked_path(group, directory=True)
            if group.name == "_trash":
                trash.append(group)
                continue
            if not re.fullmatch(r"[0-9a-f]+", group.name):
                raise ValueError(f"Unknown Bazel 8.5.1 cache directory: {group}")
            for child in sorted(group.iterdir()):
                if child.name in ("counter", "lock"):
                    checked_path(child, directory=False)
                elif re.fullmatch(r"[0-9]+", child.name):
                    checked_path(child, directory=True)
                    marker = child.with_name(child.name + ".recorded_inputs")
                    if not marker.exists() and not marker.is_symlink():
                        entries.append(Entry(child, marker, 0))
                elif re.fullmatch(r"[0-9]+\.recorded_inputs", child.name):
                    metadata = checked_path(child, directory=False)
                    contents = child.with_name(child.name.removesuffix(".recorded_inputs"))
                    if contents.exists() or contents.is_symlink():
                        checked_path(contents, directory=True)
                    entries.append(Entry(
                        contents, child, metadata.st_mtime if contents.exists() else 0))
                else:
                    raise ValueError(f"Unknown Bazel 8.5.1 cache entry: {child}")
    return sorted(entries, key=lambda entry: (entry.last_used, str(entry.contents))), trash


def allocated_sizes(paths):
    if not paths:
        return {}
    result = subprocess.run(
        ["du", "--summarize", "--block-size=1", "--one-file-system",
         "--null", "--files0-from=-"],
        input=b"".join(os.fsencode(path) + b"\0" for path in paths),
        capture_output=True, check=True)
    return {
        Path(os.fsdecode(path)): int(size)
        for record in result.stdout.split(b"\0") if record
        for size, path in [record.split(b"\t", 1)]
    }


def remove_path(path):
    identity = path.lstat()
    checked_path(path, directory=stat.S_ISDIR(identity.st_mode))
    check_container_users(path)
    check_mounts(path)
    current = path.lstat()
    if (identity.st_dev, identity.st_ino) != (current.st_dev, current.st_ino):
        raise ValueError(f"Repository cache path changed during cleanup checks: {path}")
    subprocess.run(
        ["sudo", "-n", "rm", "--recursive", "--force", "--one-file-system", "--", str(path)],
        check=True)
    if path.exists() or path.is_symlink():
        raise ValueError(f"Repository cache path still exists after cleanup: {path}")


def evict(entry):
    group = entry.contents.parent
    trash = group.parent / "_trash"
    trash.mkdir(mode=0o700, exist_ok=True)
    checked_path(trash, directory=True)
    if entry.contents.exists():
        checked_path(entry.contents, directory=True)
        entry.contents.rename(trash / str(uuid.uuid4()))
    if entry.marker.exists():
        checked_path(entry.marker, directory=False)
        entry.marker.rename(trash / str(uuid.uuid4()))


def reclaim(scopes, workspace, pending):
    for path in sorted(set(pending)):
        remove_path(path)
    return sum(allocated_sizes(scopes).values()), shutil.disk_usage(workspace).free


def check_free_space(workspace, min_free_bytes):
    if min_free_bytes <= 0:
        raise ValueError("Minimum free space must be positive")
    if not workspace.is_absolute() or not workspace.is_dir():
        raise ValueError("Runner workspace must be an existing absolute directory")
    free = shutil.disk_usage(workspace).free
    print(f"Workspace free: {free / GIB:.2f} GiB; required: {min_free_bytes / GIB:.2f} GiB")
    if free < min_free_bytes:
        raise ValueError("Insufficient workspace free space before build")
    return free


def write_summary(report):
    if summary := os.environ.get("GITHUB_STEP_SUMMARY"):
        with Path(summary).open("a") as output:
            output.write(
                "### Repository-contents cache retention\n\n"
                f"Cache: {report['before_bytes'] / GIB:.2f} -> "
                f"{report['after_bytes'] / GIB:.2f} GiB "
                f"(budget {report['max_bytes'] / GIB:g} GiB). "
                f"Evicted {report['evicted_entries']} entries "
                f"({report['expired_entries']} expired). "
                f"Free: {report['after_free_bytes'] / GIB:.2f} GiB "
                f"(required {report['min_free_bytes'] / GIB:g} GiB)."
                f"{' Dry-run estimates only.' if report['dry_run'] else ''}\n\n")


def retain(workspace, *, max_bytes, max_age_days, min_free_bytes, dry_run=False):
    if min(max_bytes, max_age_days, min_free_bytes) <= 0:
        raise ValueError("Retention limits must be positive")
    if not workspace.is_absolute() or not workspace.is_dir():
        raise ValueError("Runner workspace must be an existing absolute directory")
    workspace = workspace.resolve(strict=True)
    root = workspace / "vs-cache" / "repo-contents"
    if root.resolve() != root:
        raise ValueError(f"Refusing redirected repository cache: {root}")
    if root.exists():
        checked_path(root, directory=True)
        unknown = set(path.name for path in root.iterdir()) - set(SCOPES)
        if unknown:
            raise ValueError(f"Unknown repository cache scopes: {sorted(unknown)}")
    scopes = [root / scope for scope in SCOPES
              if (root / scope).exists() or (root / scope).is_symlink()]
    identities = {}
    for scope in scopes:
        identities[scope] = checked_path(scope, directory=True)
        if identities[scope].st_dev != workspace.stat().st_dev:
            raise ValueError(f"Repository cache is on a different filesystem: {scope}")
    report = {
        "dry_run": dry_run, "max_bytes": max_bytes, "max_age_days": max_age_days,
        "min_free_bytes": min_free_bytes, "evicted_entries": 0, "expired_entries": 0,
    }
    with ExitStack() as stack:
        for scope in scopes:
            acquire_lock(stack, scope)
        if scopes:
            check_container_users(root)
            check_mounts(root)
        for scope, identity in identities.items():
            current = checked_path(scope, directory=True)
            if (identity.st_dev, identity.st_ino) != (current.st_dev, current.st_ino):
                raise ValueError(f"Repository cache scope changed during cleanup checks: {scope}")
        entries, trash = inventory(scopes)
        before_sizes = allocated_sizes(scopes)
        used = sum(before_sizes.values())
        free = shutil.disk_usage(workspace).free
        report.update(before_bytes=used, before_free_bytes=free,
                      before_scope_bytes={path.name: size for path, size in before_sizes.items()})
        cutoff = time.time() - max_age_days * 86400
        entry_sizes = allocated_sizes([entry.contents for entry in entries
                                       if entry.contents.exists()])
        trash_bytes = sum(allocated_sizes(trash).values())
        if dry_run:
            used -= trash_bytes
            free += trash_bytes
        else:
            used, free = reclaim(scopes, workspace, trash)
        pending = []

        def select(entry, expired):
            nonlocal used, free
            report["evicted_entries"] += 1
            report["expired_entries"] += int(expired)
            size = entry_sizes.get(entry.contents, 0)
            print(json.dumps({
                "entry": str(entry.contents.relative_to(root)),
                "reason": "expired" if expired else "disk-pressure",
                "estimated_bytes": size, "dry_run": dry_run,
            }), flush=True)
            if not dry_run:
                evict(entry)
                pending.append(entry.contents.parent.parent / "_trash")
            used -= size
            free += size

        expired_entries = [entry for entry in entries if entry.last_used < cutoff]
        for entry in expired_entries:
            select(entry, True)
        if not dry_run and pending:
            used, free = reclaim(scopes, workspace, pending)
            pending.clear()
        for entry in entries:
            if entry.last_used < cutoff:
                continue
            if used <= max_bytes and free >= min_free_bytes:
                break
            select(entry, False)
            # Re-measure after each eviction batch: hardlinks and other disk
            # users mean estimated cache bytes need not become free space.
            if not dry_run and (used <= max_bytes and free >= min_free_bytes):
                used, free = reclaim(scopes, workspace, pending)
                pending.clear()
        if not dry_run:
            used, free = reclaim(scopes, workspace, pending)
            after_sizes = allocated_sizes(scopes)
            used = sum(after_sizes.values())
            free = shutil.disk_usage(workspace).free
            report["after_scope_bytes"] = {path.name: size for path, size in after_sizes.items()}
        report.update(after_bytes=max(used, 0), after_free_bytes=free)
        print(json.dumps(report, sort_keys=True), flush=True)
        write_summary(report)
        if used > max_bytes or free < min_free_bytes:
            raise ValueError("Repository cache retention cannot satisfy budget/free-space limits")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner-workspace", type=Path, required=True)
    parser.add_argument("--max-gib", type=int, default=70)
    parser.add_argument("--max-age-days", type=int, default=14)
    parser.add_argument("--min-free-gib", type=int, default=220)
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--check-free-space-only", action="store_true",
                        help="Check headroom without inspecting, locking or changing caches")
    args = parser.parse_args()
    try:
        if args.check_free_space_only:
            check_free_space(args.runner_workspace, args.min_free_gib * GIB)
        else:
            retain(args.runner_workspace, max_bytes=args.max_gib * GIB,
                   max_age_days=args.max_age_days, min_free_bytes=args.min_free_gib * GIB,
                   dry_run=args.dry_run)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Repository cache retention failed: {error}\n")


if __name__ == "__main__":
    main()
