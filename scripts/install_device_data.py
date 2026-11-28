#!/usr/bin/env python3
"""Install the VS device-data TAR into an offline host root and register its dpkg data."""

import argparse
from contextlib import ExitStack
import fcntl
import hashlib
import os
from pathlib import Path
import subprocess
import tarfile
from tempfile import TemporaryDirectory


PACKAGE = "sonic-device-data"
DEBIAN = Path(__file__).resolve().parents[1] / "src" / PACKAGE / "src/debian"
PAYLOAD = (Path("usr/share/sonic/device"), Path("usr/share/doc") / PACKAGE)
PARENTS = {parent for directory in PAYLOAD for parent in directory.parents}


def owned_path(path):
    return any(path.is_relative_to(directory) for directory in PAYLOAD)


def stage_payload(archive_path, stage):
    directories = {Path("."): 0o755}
    checksums = {}
    seen = set()
    file_kib = 0
    with tarfile.open(archive_path, mode="r|*") as archive:
        for member in archive:
            path = Path(member.name)
            if (path.is_absolute() or ".." in path.parts or path in seen or
                    any(c in member.name for c in "\r\n\\") or member.uid or member.gid or
                    member.mode & ~0o777 or not (owned_path(path) or member.isdir() and path in PARENTS)):
                raise ValueError(f"Unexpected device-data member or metadata: {member.name}")
            seen.add(path)
            for parent in path.parents:
                directories.setdefault(parent, 0o755)
            if member.isdir():
                directories[path] = member.mode
                continue
            if not member.isreg() or path in directories:
                raise ValueError(f"Expected regular device data: {path}")
            output = stage / path
            output.parent.mkdir(parents=True, exist_ok=True)
            digest = hashlib.md5(usedforsecurity=False)  # dpkg's md5sums format
            with archive.extractfile(member) as source, output.open("xb") as target:
                while chunk := source.read(1024 * 1024):
                    target.write(chunk)
                    digest.update(chunk)
            output.chmod(member.mode)
            os.utime(output, (member.mtime, member.mtime))
            checksums[path] = digest.hexdigest()
            file_kib += (member.size + 1023) // 1024
    for required in ("usr/share/sonic/device/x86_64-kvm_x86_64-r0/platform_asic",
                     f"usr/share/doc/{PACKAGE}/copyright", f"usr/share/doc/{PACKAGE}/changelog.Debian.gz"):
        if Path(required) not in checksums:
            raise ValueError(f"Incomplete device-data TAR: missing {required}")
    # dpkg-gencontrol counts each directory, including the package root and the
    # DEBIAN control directory present during Debian packaging, and rounds each file up.
    return directories, checksums, file_kib + len(directories) + 1


def status_with_package(status, installed_size, stage):
    record = subprocess.check_output([
        "dpkg-gencontrol", "-O", "-p" + PACKAGE, "-c" + str(DEBIAN / "control"),
        "-l" + str(DEBIAN / "changelog"), "-P" + str(stage), "-f" + str(stage / "files"),
        "-Vmisc:Depends=", f"-VInstalled-Size={installed_size}", "-DStatus=install ok installed",
    ], cwd=stage).strip()
    package_line = b"Package: " + PACKAGE.encode()
    others = [p for p in status.split(b"\n\n") if p.strip() and package_line not in p.splitlines()]
    return b"\n\n".join([*others, record]) + b"\n\n"


def install(root, archive):
    if os.geteuid() or os.getegid():
        raise PermissionError("Installing root-owned device data requires root")
    root = root.resolve(strict=True)
    if root == Path("/"):
        raise ValueError("An offline installer root is required")
    db = root / "var/lib/dpkg"
    status = db / "status"
    file_list = db / "info" / (PACKAGE + ".list")
    md5sums = file_list.with_suffix(".md5sums")
    for path in (db / "lock-frontend", db / "lock", db / "updates", status, file_list, md5sums):
        if path.resolve() != path:
            raise ValueError(f"Unexpected link in offline dpkg database: {path}")
    with ExitStack() as locks, TemporaryDirectory(dir=root, prefix=".device-data-") as temporary:
        # Use the same POSIX record locks as apt and dpkg before changing their database.
        for name in ("lock-frontend", "lock"):
            handle = locks.enter_context((db / name).open("ab"))
            fcntl.lockf(handle, fcntl.LOCK_EX)
        if any(p.name.isdecimal() for p in (db / "updates").iterdir()):
            raise ValueError("The offline root has pending dpkg database updates")
        stage = Path(temporary)
        directories, checksums, size = stage_payload(archive, stage)
        paths = set(directories) | checksums.keys()
        previous = set()
        if file_list.exists():
            for name in file_list.read_text().splitlines():
                path = Path(name.removeprefix("/"))
                if not name.startswith("/") or path.is_absolute() or ".." in path.parts or not (owned_path(path) or path in PARENTS):
                    raise ValueError(f"Unexpected previous {PACKAGE} path: {name}")
                previous.add(path)
        # Debian walks the tree depth-first for .list, but sorts full filenames in .md5sums.
        new_list = stage / "list"
        new_list.write_text("".join(f"/{p}\n" for p in sorted(paths, key=lambda p: p.parts)))
        new_sums = stage / "md5sums"
        new_sums.write_text("".join(f"{checksums[p]}  {p}\n" for p in sorted(checksums, key=os.fspath)))
        new_status = stage / "status"
        new_status.write_bytes(status_with_package(status.read_bytes(), size, stage))
        for output in (new_list, new_sums, new_status):
            output.chmod(0o644)

        # Leave pre-existing shared directories and unrelated files intact.
        for path in sorted(directories.keys() | {d for p in previous for d in p.parents}, key=lambda p: p.parts):
            dest = root / path
            if dest.is_symlink() or dest.exists() and not dest.is_dir():
                raise ValueError(f"Expected a real directory in the offline root: {path}")
            if path in directories:
                if not dest.exists():
                    dest.mkdir()
                    dest.chmod(directories[path])
                elif path in previous and owned_path(path):
                    dest.chmod(directories[path])
        collisions = {"/" + str(p) for p in checksums.keys() - previous
                      if (root / p).exists() or (root / p).is_symlink()}
        if collisions:
            for listing in (db / "info").glob("*.list"):
                conflict = collisions.intersection(listing.read_text().splitlines())
                if conflict:
                    raise ValueError(f"{listing.stem} already owns {min(conflict)}")
        for path in checksums:
            (stage / path).replace(root / path)
        # Remove data retired by a later source revision. Interrupted operations
        # can be replayed; publishing status last marks the completed installation.
        for path in sorted(previous - paths, key=lambda p: p.parts, reverse=True):
            if owned_path(path):
                dest = root / path
                if dest.is_dir() and not dest.is_symlink():
                    if not any(dest.iterdir()):
                        dest.rmdir()
                else:
                    dest.unlink(missing_ok=True)
        new_list.replace(file_list)
        new_sums.replace(md5sums)
        new_status.replace(status)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--tar", type=Path, required=True)
    args = parser.parse_args()
    install(args.root, args.tar)
