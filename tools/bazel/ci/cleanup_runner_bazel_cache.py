#!/usr/bin/env python3
"""Reclaim selected local build caches on an idle, dedicated host agent."""

import argparse
import os
from pathlib import Path
import subprocess

from .cleanup_vs_workspace import check_container_users, check_mounts
from .retain_repo_contents_cache import GIB, retain


CACHE_NAMES = ("bazel", "bazel-disk", "bazel-repo")


def cleanup(runner_temp, runner_workspace, *, remove_package_cache=False,
            repo_contents_policy=None):
    for label, root in (("RUNNER_TEMP", runner_temp), ("runner workspace", runner_workspace)):
        if not root.is_absolute() or not root.is_dir():
            raise ValueError(f"{label} must be an existing absolute directory")
    cache_groups = (
        (runner_temp.resolve(strict=True) / "_github_home" / ".cache", CACHE_NAMES),
        (runner_workspace.resolve(strict=True) / "vs-cache",
         ("bazel", "packages") if remove_package_cache else ("bazel",)),
    )
    paths = []
    for cache_root, names in cache_groups:
        if cache_root.resolve() != cache_root:
            raise ValueError(f"Refusing redirected cache directory: {cache_root}")
        for name in names:
            path = cache_root / name
            if path.is_symlink() or (path.exists() and not path.is_dir()):
                raise ValueError(f"Refusing unexpected local cache path: {path}")
            if path.exists():
                paths.append(path)
    if not paths:
        print("No selected local build caches to remove")
    for path in paths:
        check_container_users(path)
        check_mounts(path)
    for path in paths:
        if path.resolve() != path or not path.is_dir():
            raise ValueError(f"Local cache path changed during cleanup checks: {path}")
        print(f"Removing local build cache: {path}", flush=True)
        subprocess.run(
            ["sudo", "-n", "rm", "--recursive", "--force", "--one-file-system",
             "--", str(path)], check=True)
        if path.exists() or path.is_symlink():
            raise ValueError(f"Local cache still exists after cleanup: {path}")
    if repo_contents_policy is not None:
        return retain(runner_workspace, **repo_contents_policy)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner-workspace", type=Path, required=True)
    parser.add_argument("--remove-package-cache", action="store_true",
                        help="Also remove vs-cache/packages after NFS cache validation")
    parser.add_argument("--retain-repo-contents", action="store_true",
                        help="Apply bounded retention to prepared repositories after cleanup")
    parser.add_argument("--repo-contents-max-gib", type=int, default=70)
    parser.add_argument("--repo-contents-max-age-days", type=int, default=14)
    parser.add_argument("--repo-contents-min-free-gib", type=int, default=220)
    args = parser.parse_args()
    policy = None
    if args.retain_repo_contents:
        if min(args.repo_contents_max_gib, args.repo_contents_max_age_days,
               args.repo_contents_min_free_gib) <= 0:
            parser.error("Repository-contents retention limits must be positive")
        policy = dict(max_bytes=args.repo_contents_max_gib * GIB,
                      max_age_days=args.repo_contents_max_age_days,
                      min_free_bytes=args.repo_contents_min_free_gib * GIB)
    try:
        cleanup(Path(os.environ["RUNNER_TEMP"]), args.runner_workspace,
                remove_package_cache=args.remove_package_cache, repo_contents_policy=policy)
    except (KeyError, OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Runner local cache cleanup failed: {error}\n")


if __name__ == "__main__":
    main()
