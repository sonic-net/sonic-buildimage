#!/usr/bin/env python3
"""Restore default container startup ownership and recover corrupt Bazel installs."""

import argparse
from contextlib import ExitStack
import os
from pathlib import Path
import re
import stat
import subprocess
import sys


DIRECTORY_FLAGS = os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW
CORRUPT_INSTALLATION = re.compile(
    r"^FATAL: corrupt installation: file '([^'\n]+)' is missing or modified\.  "
    r"Please remove '([^'\n]+)' and try again\.$", re.MULTILINE
)


def open_directory(path, descriptors):
    if not path.is_absolute():
        raise ValueError("The startup directory must be an absolute path")
    descriptor = os.open("/", DIRECTORY_FLAGS)
    descriptors.callback(os.close, descriptor)
    for component in path.parts[1:]:
        if component == "..":
            raise ValueError("Refusing a redirected startup directory")
        descriptor = os.open(component, DIRECTORY_FLAGS, dir_fd=descriptor)
        descriptors.callback(os.close, descriptor)
    return descriptor


def mount_id(descriptor):
    for line in Path(f"/proc/self/fdinfo/{descriptor}").read_text().splitlines():
        if line.startswith("mnt_id:"):
            return int(line.split()[1])
    raise ValueError("Cannot determine the startup directory's mount")


def repair_directory(descriptor, device, mount, uid, gid):
    os.fchown(descriptor, uid, gid)
    with os.scandir(descriptor) as entries:
        for entry in entries:
            if not entry.is_dir(follow_symlinks=False):
                continue
            child = os.open(entry.name, DIRECTORY_FLAGS, dir_fd=descriptor)
            try:
                if os.fstat(child).st_dev == device and mount_id(child) == mount:
                    repair_directory(child, device, mount, uid, gid)
            finally:
                os.close(child)


def repair_startup(home):
    if not home.is_absolute():
        raise ValueError("The container home must be an absolute path")
    with ExitStack() as descriptors:
        descriptor = open_directory(home, descriptors)
        mount = mount_id(descriptor)
        device = os.fstat(descriptor).st_dev
        for component in (".cache", "bazel", "_bazel_root"):
            try:
                os.mkdir(component, 0o755, dir_fd=descriptor)
            except FileExistsError:
                pass
            descriptor = os.open(component, DIRECTORY_FLAGS, dir_fd=descriptor)
            descriptors.callback(os.close, descriptor)
            if os.fstat(descriptor).st_dev != device or mount_id(descriptor) != mount:
                raise ValueError("Refusing startup repair on a redirected cache mount")
        repair_directory(descriptor, device, mount, os.geteuid(), os.getegid())
    return home / ".cache/bazel/_bazel_root"


def remove_install_directory(descriptor, device, mount):
    if os.fstat(descriptor).st_dev != device or mount_id(descriptor) != mount:
        raise ValueError("Refusing corrupt-install cleanup across a mount")
    mode = stat.S_IMODE(os.fstat(descriptor).st_mode)
    os.fchmod(descriptor, mode | stat.S_IRWXU)
    with os.scandir(descriptor) as entries:
        for entry in entries:
            if entry.is_dir(follow_symlinks=False):
                child = os.open(entry.name, DIRECTORY_FLAGS, dir_fd=descriptor)
                try:
                    remove_install_directory(child, device, mount)
                finally:
                    os.close(child)
                os.rmdir(entry.name, dir_fd=descriptor)
            else:
                os.unlink(entry.name, dir_fd=descriptor)


def remove_corrupt_installation(state, installation, damaged_file):
    if installation.parent != state / "install" or not re.fullmatch(
            r"[0-9a-f]{32}", installation.name):
        raise ValueError("Refusing cleanup outside a hashed default Bazel installation")
    relative = damaged_file.relative_to(installation)
    if not relative.parts or ".." in relative.parts:
        raise ValueError("Corrupt file must be inside the reported installation")
    with ExitStack() as descriptors:
        state_descriptor = open_directory(state, descriptors)
        device = os.fstat(state_descriptor).st_dev
        mount = mount_id(state_descriptor)
        parent = os.open("install", DIRECTORY_FLAGS, dir_fd=state_descriptor)
        descriptors.callback(os.close, parent)
        if os.fstat(parent).st_dev != device or mount_id(parent) != mount:
            raise ValueError("Refusing a redirected Bazel installation mount")
        descriptor = os.open(installation.name, DIRECTORY_FLAGS, dir_fd=parent)
        descriptors.callback(os.close, descriptor)
        print(f"Removing Bazel-reported corrupt installation: {installation}", flush=True)
        remove_install_directory(descriptor, device, mount)
        os.rmdir(installation.name, dir_fd=parent)


def verify_startup(state):
    command = [
        "bazel", "--batch", "--nosystem_rc", "--nohome_rc", "--noworkspace_rc",
        "info", "output_base",
    ]

    def probe():
        result = subprocess.run(command, capture_output=True, text=True)
        sys.stdout.write(result.stdout)
        sys.stderr.write(result.stderr)
        return result

    result = probe()
    corruption = CORRUPT_INSTALLATION.search(result.stderr)
    if result.returncode == 36 and corruption:
        remove_corrupt_installation(state, Path(corruption[2]), Path(corruption[1]))
        print("Retrying Bazel startup once to re-extract its installation.", flush=True)
        result = probe()
    result.check_returncode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    try:
        if os.environ.get("GITHUB_ACTIONS") != "true":
            raise ValueError("Startup repair is restricted to GitHub Actions")
        if os.environ.get("HOME") != "/github/home":
            raise ValueError("Expected the container home /github/home")
        if os.geteuid() != 0 or not Path("/.dockerenv").is_file():
            raise ValueError("Startup repair requires a root job container")
        state = repair_startup(Path("/github/home"))
        print(f"Restored startup directory ownership to {os.geteuid()}:{os.getegid()}: {state}")
        print("Ownership repair left regular files, symlinks and repository/disk caches unchanged.")
        verify_startup(state)
    except subprocess.CalledProcessError as error:
        parser.exit(error.returncode, "Bazel startup verification failed.\n")
    except (OSError, ValueError) as error:
        parser.exit(1, "Cannot repair Bazel startup ownership: " + str(error) + "\n")
    print("Bazel startup verified at the default path.")


if __name__ == "__main__":
    main()
