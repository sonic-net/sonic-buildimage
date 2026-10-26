"""Select imported split-debug files matching the final runtime OCI image."""

import argparse
import bisect
import copy
import hashlib
import json
from pathlib import Path, PurePosixPath
import posixpath
import shutil
import tarfile
import tempfile
import zlib

from tools.bazel.ci.artifact_validation import path_name, require, sha
from tools.bazel.oci.elf import inspect_elf
from tools.bazel.oci.oci_layout import validate_layout


def identity(path):
    return {"sha256": sha(path), "size": path.stat().st_size}


def file_info(stream, prefix, temporary):
    """Read one ELF through a temporary seekable file, keeping memory bounded."""
    with tempfile.TemporaryFile(dir=temporary) as saved:
        saved.write(prefix)
        shutil.copyfileobj(stream, saved)
        info = inspect_elf(saved)
        saved.seek(0)
        info["sha256"] = hashlib.file_digest(saved, "sha256").hexdigest()
        return info


def runtime_files(layers, temporary):
    """Apply ordered OCI layers, including whiteouts and file replacement."""
    files = {}
    for layer in layers:
        entries, whiteouts = {}, []
        with tarfile.open(layer, "r|*") as archive:
            for member in archive:
                name = path_name(member.name)
                pure = PurePosixPath(name)
                if pure.name.startswith(".wh."):
                    opaque = pure.name == ".wh..wh..opq"
                    target = str(pure.parent if opaque else pure.parent / pure.name[4:])
                    whiteouts.append((target, opaque))
                    continue
                item = {"kind": "directory" if member.isdir() else "other"}
                if member.issym() or member.islnk():
                    item.update(kind="symlink" if member.issym() else "hardlink", link=member.linkname)
                elif member.isfile():
                    require(member.sparse is None, "sparse runtime member is unsupported: " + name)
                    with archive.extractfile(member) as stream:
                        prefix = stream.read(4)
                        if prefix == b"\x7fELF":
                            item.update(kind="elf", elf=file_info(stream, prefix, temporary))
                entries[name] = item
        for name, opaque in whiteouts:
            prefix = "" if name == "." else name + "/"
            for previous in list(files):
                if previous.startswith(prefix) or (not opaque and previous == name):
                    del files[previous]
        previous_names = sorted(files)
        for name, item in entries.items():
            if item["kind"] != "directory":
                index = bisect.bisect_left(previous_names, name + "/")
                while index < len(previous_names) and previous_names[index].startswith(name + "/"):
                    files.pop(previous_names[index], None)
                    index += 1
            files[name] = item
    return files


def resolve(name, files):
    """Resolve links inside an image inventory without using host paths."""
    seen = set()
    for _ in range(40):
        require(name not in seen, "cycle in image links: " + name)
        seen.add(name)
        parts = PurePosixPath(name).parts
        for index in range(1, len(parts) + 1):
            prefix = "/".join(parts[:index])
            item = files.get(prefix, {})
            if item.get("kind") not in ("symlink", "hardlink"):
                continue
            target = item["link"]
            if item["kind"] == "symlink" and not target.startswith("/"):
                target = posixpath.join(posixpath.dirname(prefix), target)
            name = posixpath.normpath(posixpath.join(target.lstrip("/"), *parts[index:]))
            path_name(name)
            break
        else:
            return name
    raise ValueError("too many image links: " + name)


def candidates(tars, temporary):
    files, sources = {}, []
    for index, path in enumerate(tars):
        source = {"index": index, **identity(path)}
        sources.append(source)
        with tarfile.open(path, "r|*") as archive:
            for member in archive:
                name = path_name(member.name)
                if not name.startswith("usr/lib/debug/") or member.isdir():
                    continue
                require(member.isfile() and member.sparse is None,
                        "imported symbols must be regular files: " + name)
                target = temporary / str(len(files))
                with archive.extractfile(member) as stream, target.open("wb") as output:
                    shutil.copyfileobj(stream, output)
                with target.open("rb") as stream:
                    info = inspect_elf(stream)
                require(info is not None, "imported symbol is not ELF: " + name)
                require(name not in files, "duplicate imported symbol path: " + name)
                files[name] = {"file": target, "member": member, "elf": info,
                               "source": index, **identity(target)}
    return files, sources


def match(runtime, imported, required_paths):
    selected, pairs = set(), []
    for name, item in sorted(runtime.items()):
        if item["kind"] != "elf":
            continue
        info = item["elf"]
        identifier = info["build_id"]
        if not identifier:
            continue
        companion = "usr/lib/debug/.build-id/" + identifier[:2] + "/" + identifier[2:] + ".debug"
        if companion not in imported:
            continue
        candidate = imported[companion]
        symbols = candidate["elf"]
        require(symbols["build_id"] == identifier and symbols["has_dwarf"],
                "debug companion lacks matching build ID or DWARF: " + name)
        require(all(info[key] == symbols[key] for key in ("class", "encoding", "machine")),
                "debug companion ELF platform differs: " + name)
        link = info["debuglink"]
        require(link is not None and link["name"] == PurePosixPath(companion).name,
                "runtime GNU debuglink does not name the companion: " + name)
        crc = 0
        with candidate["file"].open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                crc = zlib.crc32(chunk, crc)
        require(link["crc"] == crc, "debuglink CRC differs: " + name)
        selected.add(companion)
        pair = {"runtime_path": name, "runtime_sha256": info["sha256"], "build_id": identifier,
                "debug_path": companion, "debug_sha256": candidate["sha256"], "source": candidate["source"]}
        current, visited = companion, set()
        while imported[current]["elf"]["debugaltlink"] is not None:
            require(current not in visited, "cycle in DWZ supplement links: " + current)
            visited.add(current)
            alternate = imported[current]["elf"]["debugaltlink"]
            target = path_name(posixpath.normpath(posixpath.join(posixpath.dirname(current), alternate["name"])).lstrip("/"))
            require(target.startswith("usr/lib/debug/") and target in imported,
                    "missing DWZ supplement: " + target)
            supplement = imported[target]["elf"]
            require(supplement["build_id"] == alternate["build_id"] and supplement["has_dwarf"] and
                    all(info[key] == supplement[key] for key in ("class", "encoding", "machine")),
                    "DWZ supplement identity or DWARF differs: " + target)
            selected.add(target)
            pair.setdefault("supplements", []).append({"path": target, "build_id": supplement["build_id"],
                                                       "sha256": imported[target]["sha256"]})
            current = target
        pairs.append(pair)
    matched = {pair["runtime_path"] for pair in pairs}
    for name in required_paths:
        require(resolve(path_name(name), runtime) in matched,
                "required runtime file has no matching imported symbols: " + name)
    return selected, pairs


def build(runtime, tars, expected_platform, required_paths, output, receipt):
    layout = validate_layout(runtime, expected_platform)
    with tempfile.TemporaryDirectory(prefix="sonic-imported-symbols-") as name:
        temporary = Path(name)
        deployed = runtime_files(layout.layers, temporary)
        imported, sources = candidates(tars, temporary)
        selected, pairs = match(deployed, imported, required_paths)
        with tarfile.open(output, "w", format=tarfile.GNU_FORMAT) as archive:
            for name in sorted(selected):
                item = imported[name]
                member = copy.copy(item["member"])
                member.name, member.mtime, member.pax_headers = "./" + name, 0, {}
                with item["file"].open("rb") as stream:
                    archive.addfile(member, stream)
        result = {"schema": 1, "runtime_manifest": layout.descriptor["digest"],
                  "expected_platform": expected_platform, "sources": sources,
                  "required_paths": sorted(required_paths), "pairs": pairs,
                  "selected_paths": sorted(selected), "excluded_paths": sorted(set(imported) - selected),
                  "output": identity(output)}
        receipt.write_text(json.dumps(result, sort_keys=True, indent=2) + "\n")
        return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--tar", action="append", type=Path, default=[])
    parser.add_argument("--expected-platform", required=True)
    parser.add_argument("--required-path", action="append", default=[])
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    args = parser.parse_args()
    build(args.runtime, args.tar, args.expected_platform, args.required_path, args.output, args.receipt)


if __name__ == "__main__":
    main()
