#!/usr/bin/env python3
"""Single-build handoff of a freshly created native VS rootfs."""

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import stat
import tempfile
import time


def session_directory(value):
    path = Path(value)
    if (path.is_symlink() or path.parent.resolve() != Path.cwd().resolve()
            or not re.fullmatch(r"\.vs-rootfs-handoff\.[A-Za-z0-9]+", path.name)):
        raise ValueError("invalid handoff directory")
    if not path.is_dir() or stat.S_IMODE(path.stat().st_mode) != 0o700:
        raise ValueError("handoff directory must be private")
    return path.resolve()


def has_mounts(path):
    for line in Path("/proc/self/mountinfo").read_text().splitlines():
        mount = Path(re.sub(r"\\([0-7]{3})", lambda match: chr(int(match[1], 8)), line.split()[4]))
        if mount == path or path in mount.parents:
            return True
    return False


def write_state(session, value):
    fd, temporary = tempfile.mkstemp(dir=session)
    try:
        with os.fdopen(fd, "w") as stream:
            json.dump(value, stream)
        os.replace(temporary, session / "state.json")
    finally:
        Path(temporary).unlink(missing_ok=True)


def read_state(session):
    with (session / "state.json").open() as stream:
        value = stream.read(65537)
    if len(value) > 65536:
        raise ValueError("oversized handoff state")
    value = json.loads(value)
    if not isinstance(value, dict) or value.get("status") not in ("stored", "restored", "miss"):
        raise ValueError("invalid handoff state")
    return value


def file_stat(info):
    return [info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns, info.st_ctime_ns]


def archive_identity(path):
    if path.is_symlink() or not path.is_file():
        raise ValueError("invalid rootfs archive")
    before = path.stat()
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    after = path.stat()
    if file_stat(before) != file_stat(after):
        raise ValueError("rootfs archive changed")
    return {"stat": file_stat(after), "sha256": digest.hexdigest()}


def handoff(action, directory, root, archive, host_base_receipt=None):
    started = time.monotonic()
    session = session_directory(directory)
    root, archive = Path(root), Path(archive)
    if (root.name != "fsroot-vs" or root.parent.resolve() != Path.cwd().resolve()
            or root.is_symlink() or archive.name != "sonic-vs.bin__vs__rfs.squashfs"):
        raise ValueError("unsupported rootfs handoff")
    root = root.absolute()
    payload = session / "root"
    value = {"status": "miss", "reason": "unavailable"}
    success = False
    try:
        if action == "store":
            if (session / "state.json").exists() or not root.is_dir() or has_mounts(root):
                return False
            producer = None
            if host_base_receipt is not None:
                with Path(host_base_receipt).open() as stream:
                    producer = json.load(stream)
                if not isinstance(producer, dict) or producer.get("cache_hit") is not False:
                    return False
            identity = archive_identity(archive)
            if producer is not None and producer["outputs"]["image"] != {
                    "sha256": identity["sha256"], "bytes": identity["stat"][2]}:
                return False
            value = {"status": "stored", "archive": identity,
                     "archive_path": str(archive.resolve())}
            root.rename(payload)
            info = payload.stat()
            value["root_inode"] = [info.st_dev, info.st_ino]
            success = True
        else:
            value = read_state(session)
            if value.get("status") != "stored":
                return False
            # Consume before validation: a failed or interrupted attempt cannot retry.
            value["status"] = "miss"
            value["reason"] = "profile-mismatch"
            write_state(session, value)
            if (root.exists() or root.is_symlink() or payload.is_symlink()
                    or not payload.is_dir() or has_mounts(payload)
                    or value["archive_path"] != str(archive.resolve())
                    or value["archive"] != archive_identity(archive)):
                return False
            info = payload.stat()
            if value["root_inode"] != [info.st_dev, info.st_ino]:
                return False
            payload.rename(root)
            value.update(status="restored", reason=None)
            success = True
        return success
    except (OSError, ValueError, KeyError, TypeError):
        value.update(status="miss", reason="unavailable")
        return False
    finally:
        value[action + "_seconds"] = round(time.monotonic() - started, 6)
        write_state(session, value)
        print("[ installer rootfs handoff ] " + value["status"], flush=True)


def cleanup(directory):
    session = session_directory(directory)
    if has_mounts(session):
        raise ValueError("refusing to clean mounted handoff data")
    try:
        value = read_state(session)
    except (OSError, ValueError):
        value = {}
    summary = {"status": value.get("status", "unused")}
    if summary["status"] not in ("unused", "stored", "restored", "miss"):
        summary["status"] = "miss"
    for name in ("store_seconds", "restore_seconds"):
        if isinstance(value.get(name), (int, float)) and math.isfinite(value[name]):
            summary[name] = value[name]
    shutil.rmtree(session)
    print("[ installer rootfs handoff ] " + json.dumps(summary), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("store", "restore", "cleanup"))
    parser.add_argument("--session", required=True)
    parser.add_argument("--root")
    parser.add_argument("--archive")
    parser.add_argument("--host-base-receipt",
                        help="Require a cache-miss receipt matching the archive before storing.")
    args = parser.parse_args()
    try:
        if args.action == "cleanup":
            cleanup(args.session)
            return 0
        if not args.root or not args.archive:
            parser.error("--root and --archive are required")
        success = handoff(args.action, args.session, args.root, args.archive, args.host_base_receipt)
        return 0 if success or args.action == "store" else 1
    except (OSError, ValueError):
        print("[ installer rootfs handoff ] unavailable", flush=True)
        return 1 if args.action != "store" else 0


if __name__ == "__main__":
    raise SystemExit(main())
