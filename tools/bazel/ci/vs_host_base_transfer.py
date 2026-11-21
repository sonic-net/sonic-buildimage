"""Move one eligible Bazel disk-cache key into or out of a workflow artifact."""

import argparse
import fcntl
import os
from pathlib import Path
import re
import stat


KEY = re.compile(r"[0-9a-f]{64}")


def owned_directory(path, *, create=False):
    path = Path(path).absolute()
    # A self-hosted runner may mount its workspace through a symlink. Resolve
    # the parent, then use lstat on the cache or artifact directory itself.
    path = path.parent.resolve(strict=True) / path.name
    if create:
        path.mkdir(mode=0o700, exist_ok=True)
    info = path.lstat()
    if not stat.S_ISDIR(info.st_mode) or info.st_uid != os.geteuid():
        raise ValueError(f"expected a directory owned by the build user: {path}")
    return path


def validate_tree(path):
    with os.scandir(path) as entries:
        for entry in entries:
            if entry.is_dir(follow_symlinks=False):
                validate_tree(entry.path)
            elif not entry.is_file(follow_symlinks=False):
                raise ValueError(f"unsupported cache entry: {entry.path}")


def separate_directories(left, right):
    if left.is_relative_to(right) or right.is_relative_to(left):
        raise ValueError("cache and artifact directories must be disjoint")


def pack(cache_dir, key, publish_dir):
    if not KEY.fullmatch(key):
        raise ValueError("invalid host-base key")
    root = owned_directory(cache_dir)
    selected = owned_directory(root / key)
    if any(stat.S_IMODE(path.stat().st_mode) != 0o700 for path in (root, selected)):
        raise ValueError("cache and key directories must be private")
    separate_directories(root, Path(publish_dir).resolve())
    validate_tree(selected)
    publish = owned_directory(publish_dir, create=True)
    if any(publish.iterdir()):
        raise ValueError("artifact publication directory must be empty")
    publish.chmod(0o700)
    # Both directories belong to this job's workspace. rename never copies blobs.
    selected.rename(publish / key)


def restore(cache_dir, download_dir):
    download = owned_directory(download_dir)
    separate_directories(Path(cache_dir).resolve(), download)
    children = list(download.iterdir())
    if len(children) != 1 or not KEY.fullmatch(children[0].name):
        raise ValueError("artifact must contain exactly one host-base key directory")
    selected = owned_directory(children[0])
    validate_tree(selected)
    # GitHub downloads directories with mode 0755. The cached image and its
    # Bazel metadata stay private as a unit after the directory is moved.
    root = owned_directory(cache_dir, create=True)
    root.chmod(0o700)
    selected.chmod(0o700)
    fd = os.open(root, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    try:
        fcntl.flock(fd, fcntl.LOCK_EX)
        target = root / selected.name
        if os.path.lexists(target):
            existing = owned_directory(target)
            if stat.S_IMODE(existing.stat().st_mode) != 0o700:
                raise ValueError("existing host-base key directory must be private")
            validate_tree(existing)
        else:
            selected.rename(target)
    finally:
        os.close(fd)
    return selected.name


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="operation", required=True)
    write = commands.add_parser("pack")
    write.add_argument("cache_dir", type=Path)
    write.add_argument("key")
    write.add_argument("publish_dir", type=Path)
    read = commands.add_parser("restore")
    read.add_argument("cache_dir", type=Path)
    read.add_argument("download_dir", type=Path)
    args = parser.parse_args()
    if args.operation == "pack":
        pack(args.cache_dir, args.key, args.publish_dir)
    else:
        print(restore(args.cache_dir, args.download_dir))


if __name__ == "__main__":
    main()
