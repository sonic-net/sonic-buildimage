#!/usr/bin/env python3
"""Prepare short, runner-exclusive kernel scratch storage and clean fixed state."""

import argparse
import hashlib
import os
from pathlib import Path
import re
import stat
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.kernel.prepare import remove_build_tree, require
from tools.bazel.kernel import prepare as kernel


DIRECTORIES = (
    "kernel-bundle-state", "kernel-cache-hit", "kernel-unsupported",
    "kernel-cache", "kernel-cache-proof", "kernel-bundle-artifacts",
)
FILES = (
    "kernel-before-rejection.sha256", "kernel-unsupported.log", "kernel-bundle-stage.log",
)


def check_path_budget(root, version):
    source = (root / "kernel-bundle-state/build/output" / ("0" * 32)
              / "sandbox/linux-sandbox/2147483647/execroot/_main/kernel-action-12345678/source"
              / ("linux-" + version) / "debian/build/source_sonic")
    longest = source / "drivers/accessibility/speakup/spk_priv_keyinfo.h"
    length = len(os.fsencode(longest))
    require(length <= 255,
            f"kernel scratch path exceeds Speakup's 255-byte limit ({length} bytes): {longest}")
    print(f"Kernel Speakup path budget: {length}/255 bytes", flush=True)


def prepare_root(root, runner_temp, workspace, runner_name):
    require(root.is_absolute() and root.resolve() == root,
            "kernel scratch root must be a physical absolute path, not a symlink")
    require(root.parent.is_dir(), "kernel scratch parent must already exist on the build disk")
    runner_temp = runner_temp.resolve(strict=True)
    workspace = workspace.resolve(strict=True)
    for other in (runner_temp, workspace):
        require(root != other and root not in other.parents and other not in root.parents,
                "kernel scratch root must not overlap RUNNER_TEMP or the workspace")
    require(root.parent.stat().st_dev == runner_temp.stat().st_dev,
            "kernel scratch must be on the same filesystem as RUNNER_TEMP")
    root.mkdir(mode=0o700, exist_ok=True)
    metadata = root.lstat()
    require(stat.S_ISDIR(metadata.st_mode) and metadata.st_uid == os.geteuid()
            and not metadata.st_mode & 0o077,
            "kernel scratch root must be owned by this agent and private (mode 0700)")
    require(metadata.st_dev == runner_temp.stat().st_dev,
            "kernel scratch root must not redirect to another filesystem")
    require(bool(runner_name), "kernel scratch requires RUNNER_NAME")
    identity = hashlib.sha256(os.fsencode(runner_name + "\n" + str(runner_temp))).hexdigest()
    owner = root / ".runner-owner"
    try:
        descriptor = os.open(owner, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
    except FileExistsError:
        require(not owner.is_symlink() and owner.is_file(),
                "invalid kernel scratch runner identity")
        require(owner.read_text() == identity,
                "kernel scratch root belongs to a different runner; provision a separate short root")
    else:
        with os.fdopen(descriptor, "w") as stream:
            stream.write(identity)
    print("Kernel scratch root: " + str(root), flush=True)


def cleanup(temporary, workspace):
    require(temporary.is_absolute(), "kernel temporary directory must be absolute")
    temporary = temporary.resolve(strict=True)
    require(temporary.is_dir() and temporary != temporary.parent,
            "refusing invalid kernel temporary directory")
    workspace = workspace.resolve(strict=True)
    mounts = [
        Path(re.sub(r'\\([0-7]{3})', lambda match: chr(int(match[1], 8)), line.split()[4]))
        for line in Path("/proc/self/mountinfo").read_text().splitlines()
    ]
    targets = []
    for name in DIRECTORIES + FILES:
        path = temporary / name
        require(not path.is_symlink(), "refusing redirected kernel scratch path: " + str(path))
        require(path != workspace and path not in workspace.parents and workspace not in path.parents,
                "kernel scratch path must not overlap the workspace")
        require(not any(mount == path or path in mount.parents for mount in mounts),
                "refusing mounted kernel scratch path: " + str(path))
        if not path.exists():
            continue
        mode = path.lstat().st_mode
        require(stat.S_ISDIR(mode) if name in DIRECTORIES else stat.S_ISREG(mode),
                "unexpected kernel scratch path type: " + str(path))
        targets.append(path)

    pid_file = temporary / "kernel-cache/server.pid"
    require(not pid_file.is_symlink(), "refusing redirected kernel cache PID file")
    if pid_file.exists():
        require(pid_file.is_file(), "kernel cache PID file must be regular")
        value = pid_file.read_text().strip()
        require(re.fullmatch(r"[1-9][0-9]*", value) and int(value) > 1,
                "invalid kernel cache PID")
        try:
            os.kill(int(value), 0)
        except ProcessLookupError:
            pass
        else:
            raise ValueError("kernel cache process is still running; refusing scratch cleanup")

    for path in targets:
        print("Removing kernel scratch path: " + str(path), flush=True)
        if path.name in DIRECTORIES:
            remove_build_tree(path)
        else:
            path.unlink()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("prepare", "cleanup"))
    parser.add_argument("--architecture", choices=("amd64", "arm64"))
    args = parser.parse_args()
    root = Path(os.environ["KERNEL_RUN_TEMP"])
    workspace = Path(os.environ["GITHUB_WORKSPACE"])
    if args.command == "prepare":
        configuration, _ = kernel.contract(workspace, args.architecture or kernel.native_architecture())
        check_path_budget(root, configuration["kernel_version"])
        prepare_root(root, Path(os.environ["RUNNER_TEMP"]), workspace, os.environ["RUNNER_NAME"])
    else:
        cleanup(root, workspace)


if __name__ == "__main__":
    main()
