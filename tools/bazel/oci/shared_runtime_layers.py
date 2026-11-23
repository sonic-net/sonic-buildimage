"""Factor identical regular files into a shared layer without merging image receipts."""

import argparse
from contextlib import contextmanager
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import tarfile


def is_gzip(path):
    with path.open("rb") as stream:
        return stream.read(2) == b"\x1f\x8b"


def open_layer(path):
    # Only accept encodings that the lossless fallback can emit as a gzip TAR.
    mode = "r|gz" if is_gzip(path) else "r|"
    try:
        return tarfile.open(path, mode)
    except tarfile.TarError as exc:
        raise ValueError(f"{path}: expected a plain tar or gzip-compressed tar") from exc


def path_name(name):
    path = PurePosixPath(name)
    if path.is_absolute() or ".." in path.parts:
        raise ValueError("unsafe layer path: " + name)
    return path.as_posix()


def member_metadata(member):
    return (member.type, member.size, member.mode, member.uid, member.gid,
            member.uname, member.gname, member.mtime, member.linkname,
            member.devmajor, member.devminor,
            tuple(sorted((key, value) for key, value in member.pax_headers.items()
                         if key not in ("path", "linkpath"))))


def inventory(path, directories=None):
    entries, blocked = {}, set()
    fallback = None
    with open_layer(path) as archive:
        for member in archive:
            name = path_name(member.name)
            if name in entries:
                fallback = "duplicate paths"
            if PurePosixPath(name).name.startswith(".wh."):
                fallback = "whiteouts"
            if member.issparse():
                fallback = "sparse files"
            if member.islnk():
                try:
                    blocked.add(path_name(member.linkname.lstrip("/")))
                except ValueError:
                    fallback = "complex hardlinks"
            digest = None
            if member.isfile() and not member.issparse():
                with archive.extractfile(member) as source:
                    digest = hashlib.file_digest(source, "sha256").hexdigest()
            entries[name] = (member_metadata(member), digest)
            if member.isdir() and directories is not None:
                directories[name] = member
    return entries, blocked, fallback


def movable_files(entries, base_non_dirs):
    """Find files that can move earlier and the directory headers they need."""
    seen = {}
    used_as_directory = set()
    early_dirs = set()
    files = {}

    def known_directory(name):
        if name == ".":
            return True
        if name in seen:
            return seen[name] == tarfile.DIRTYPE
        # An absent path under real directories can be created implicitly.
        return base_non_dirs is not None and name not in base_non_dirs

    for name, (metadata, digest) in entries.items():
        kind = metadata[0]
        parents = [str(parent) for parent in PurePosixPath(name).parents]
        accessed = parents
        if kind == tarfile.LNKTYPE:
            target = path_name(metadata[8].lstrip("/"))
            # Keep reads of inherited links and forward hardlinks in place.
            if seen.get(target) not in tarfile.REGULAR_TYPES:
                break
            accessed = parents + [str(parent) for parent in PurePosixPath(target).parents]
        # An operation through a symlink can read or overwrite an unrelated
        # path. Keep it and everything after it in original extraction order.
        if not all(known_directory(parent) for parent in accessed):
            break
        if name not in used_as_directory:
            if kind == tarfile.DIRTYPE:
                early_dirs.add(name)
            elif digest is not None:
                files[name] = {parent for parent in parents if parent in seen}
        used_as_directory.update(accessed)
        seen[name] = kind
    return files, early_dirs


@contextmanager
def gzip_output(path, pigz):
    with path.open("wb") as output, subprocess.Popen(
            [pigz, "-n", "-6", "-p", "2", "-c"],
            stdin=subprocess.PIPE, stdout=output) as process:
        with process.stdin:
            yield process.stdin
        if process.wait():
            raise ValueError("failed to compress shared runtime layer")


@contextmanager
def compressed_tar(path, pigz):
    with gzip_output(path, pigz) as output, tarfile.open(
            fileobj=output, mode="w|", format=tarfile.PAX_FORMAT) as archive:
        yield archive


def original_layer(source, destination, pigz):
    if is_gzip(source):
        shutil.copyfile(source, destination)
    else:
        with source.open("rb") as stream, gzip_output(destination, pigz) as output:
            shutil.copyfileobj(stream, output, 1024 * 1024)


def split(first, second, common_out, first_out, second_out, receipt, pigz, base_non_dirs=None):
    outputs = [path.resolve() for path in (common_out, first_out, second_out, receipt)]
    if len(set(outputs)) != len(outputs) or set(outputs) & {first.resolve(), second.resolve()}:
        raise ValueError("layer outputs must be distinct from each other and the inputs")
    directories = {}
    first_entries, first_blocked, first_fallback = inventory(first, directories)
    second_entries, second_blocked, second_fallback = inventory(second)
    blocked = first_blocked | second_blocked
    common = set()
    common_dirs = set()
    fallback = first_fallback or second_fallback
    if not fallback:
        first_files, first_dirs = movable_files(first_entries, base_non_dirs)
        second_files, second_dirs = movable_files(second_entries, base_non_dirs)
        early_dirs = first_dirs & second_dirs
        for name in first_files.keys() & second_files.keys():
            if name in blocked or first_entries[name] != second_entries[name]:
                continue
            # Leave late directory headers at their original position. Share
            # files before those headers only if both inputs have that order.
            required_dirs = first_files[name] | second_files[name]
            if not required_dirs <= early_dirs:
                continue
            parents = {str(parent) for parent in PurePosixPath(name).parents if str(parent) != "."}
            if any(parent not in directories or first_entries[parent] != second_entries.get(parent)
                   for parent in parents | required_dirs):
                continue
            common.add(name)
            common_dirs.update(required_dirs)
    if not common:
        fallback = fallback or "no identical movable files"
        with compressed_tar(common_out, pigz):
            pass
        original_layer(first, first_out, pigz)
        original_layer(second, second_out, pigz)
    else:
        with compressed_tar(common_out, pigz) as shared, compressed_tar(first_out, pigz) as delta:
            for name in sorted(common_dirs, key=lambda name: (len(PurePosixPath(name).parts), name)):
                shared.addfile(directories[name])
            with open_layer(first) as archive:
                for member in archive:
                    target = shared if path_name(member.name) in common else delta
                    if member.isfile():
                        with archive.extractfile(member) as source:
                            target.addfile(member, source)
                    else:
                        target.addfile(member)
        with compressed_tar(second_out, pigz) as delta, open_layer(second) as archive:
            for member in archive:
                if path_name(member.name) in common:
                    continue
                if member.isfile():
                    with archive.extractfile(member) as source:
                        delta.addfile(member, source)
                else:
                    delta.addfile(member)
    receipt.write_text(json.dumps({
        "schema": 1,
        "common_files": len(common),
        "common_file_bytes": sum(first_entries[name][0][1] for name in common),
        "fallback": fallback,
        "output_bytes": {
            "common": common_out.stat().st_size,
            "first_delta": first_out.stat().st_size,
            "second_delta": second_out.stat().st_size,
        },
    }, sort_keys=True) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("first", "second", "common-out", "first-out", "second-out", "receipt"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--pigz", required=True)
    parser.add_argument("--base-paths", type=Path)
    args = parser.parse_args()
    base_non_dirs = None
    if args.base_paths is not None:
        paths = json.loads(args.base_paths.read_text())
        if paths is not None:
            if not isinstance(paths, list) or not all(
                isinstance(path, str) and path_name(path) == path for path in paths
            ):
                raise ValueError("expected a list of canonical base paths or null")
            base_non_dirs = set(paths)
    split(args.first, args.second, args.common_out, args.first_out, args.second_out,
          args.receipt, args.pigz, base_non_dirs)


if __name__ == "__main__":
    main()
