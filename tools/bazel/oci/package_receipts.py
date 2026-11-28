#!/usr/bin/env python3
"""Record retained Debian package policy after BUILD-declared BSD tar actions.

Archive extraction and PAX assembly belong to the owning BUILD file. This
validator only reads their outputs, checks filesystem metadata and records the
original package/control identities required by SONiC's FIPS and debug policy.
It does not invoke tools, create archives, install files, or execute scripts.
"""

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import tarfile

from tools.bazel.ci.artifact_validation import path_name, require, sha


def inspect_archive(path, *, payload):
    """Read ordered filesystem facts and the original decompressed TAR digest."""
    entries = []
    controls = {}
    with tarfile.open(path, "r:*") as archive:
        for member in archive:
            name = path_name(member.name)
            require(member.isfile() or member.isdir() or member.issym() or member.islnk(),
                    "unsupported package member: " + name)
            require(member.sparse is None, "sparse package member: " + name)
            require(not payload or not PurePosixPath(name).name.startswith(".wh."),
                    "reserved OCI whiteout: " + name)
            if member.islnk():
                path_name(member.linkname)
            content_digest = None
            if member.isfile():
                with archive.extractfile(member) as stream:
                    if payload:
                        content_digest = hashlib.file_digest(stream, "sha256").hexdigest()
                    else:
                        require(name not in controls, "duplicate control entry: " + name)
                        controls[name] = stream.read()
                        content_digest = hashlib.sha256(controls[name]).hexdigest()
            elif not payload:
                require(member.isdir(), "non-file control entry: " + name)
            entries.append({
                "path": name, "kind": "file" if member.isfile() else member.type.decode("ascii"),
                "mode": member.mode, "uid": member.uid, "gid": member.gid,
                "uname": member.uname, "gname": member.gname, "mtime": member.mtime,
                "size": member.size, "linkname": member.linkname, "sha256": content_digest,
                "extended": {key: value for key, value in member.pax_headers.items()
                             if key not in {"path", "linkpath", "size", "uid", "gid", "uname", "gname", "mtime"}},
            })
        require(entries, "empty package archive")
        # Validate the end-of-archive boundary before hashing the entire original
        # decompressed stream, including padding. These hashes are existing pins.
        archive.fileobj.seek(archive.offset)
        require(archive.fileobj.read(1024) == b"\0" * 1024, "missing TAR end markers")
        while trailing := archive.fileobj.read(1024 * 1024):
            require(not trailing.strip(b"\0"), "data after TAR end markers")
        size = archive.fileobj.tell()
        archive.fileobj.seek(0)
        original_sha = hashlib.file_digest(archive.fileobj, "sha256").hexdigest()
    return entries, controls, original_sha, size


def control_fields(contents):
    """Keep complete control fields while rejecting ambiguous duplicate names."""
    fields, seen, current = {}, set(), None
    for line in contents.decode("utf-8").splitlines():
        if line.startswith((" ", "\t")):
            require(current is not None, "orphan control continuation")
            fields[current] += "\n" + line
        elif line:
            name, separator, value = line.partition(":")
            require(separator and re.fullmatch(r"[!-9;-~]+", name) and name.lower() not in seen,
                    "invalid or duplicate control field: " + name)
            seen.add(name.lower())
            fields[name] = value.lstrip()
            current = name
    require(all(fields.get(name) for name in ("Package", "Version", "Architecture")),
            "missing package control identity")
    return fields


def receipt(packages, context, payload, runtime_manifest=None, *, architecture):
    """Validate extracted package facts and return the image's retained-package receipt."""
    reserved = {"schema", "architecture", "packages", "payload", "runtime_manifest_sha256"}
    require(isinstance(context, dict) and not reserved.intersection(context), "context overrides package facts")
    records, expected, names = [], [], set()
    for paths in packages:
        source = Path(paths["source"])
        with source.open("rb") as stream:
            require(stream.read(8) == b"!<arch>\n", "input is not a Debian ar archive")
        require(Path(paths["version"]).read_bytes() == b"2.0\n", "unsupported Debian archive version")
        listed = Path(paths["members"]).read_text().splitlines()
        suffixes = ("", ".gz", ".xz", ".bz2")
        require(len(listed) == 3 and listed[0] == "debian-binary" and
                listed[1] in ["control.tar" + suffix for suffix in suffixes] and
                listed[2] in ["data.tar" + suffix for suffix in suffixes], "unsupported Debian archive members")
        _, controls, control_sha, _ = inspect_archive(Path(paths["control"]), payload=False)
        require("control" in controls, "package lacks control file")
        fields = control_fields(controls["control"])
        require(fields["Architecture"] in (architecture, "all"), "foreign Debian architecture")
        name = fields["Package"]
        require(name not in names, "duplicate Debian package: " + name)
        names.add(name)
        members, _, payload_sha, payload_size = inspect_archive(Path(paths["data"]), payload=True)
        expected.extend(members)
        records.append({
            "package": name, "version": fields["Version"], "architecture": fields["Architecture"],
            "source_deb": source.name, "source_label": paths["source_label"],
            "source_sha256": sha(source), "source_size": source.stat().st_size,
            "control_sha256": control_sha, "control_fields": fields,
            "control_files": {name: hashlib.sha256(data).hexdigest() for name, data in controls.items()},
            "payload_sha256": payload_sha, "payload_size": payload_size, "payload_members": len(members),
        })
    require(records and set(context.get("required_packages", [])).issubset(names), "missing required package")
    actual, _, _, _ = inspect_archive(payload, payload=True)
    require(len(actual) == len(expected), "aggregate package member count differs")
    for before, after in zip(expected, actual):
        require(all(after[key] == value for key, value in before.items() if key != "extended"),
                "aggregate changes package content, order or metadata: " + before["path"])
        require(all(after["extended"].get(key) == value for key, value in before["extended"].items()),
                "aggregate loses extended metadata: " + before["path"])
    result = context | {"schema": 1, "architecture": architecture, "packages": records,
                        "payload": {"path": payload.name, "sha256": sha(payload),
                                    "size": payload.stat().st_size, "members": len(actual)}}
    if runtime_manifest is not None:
        previous = json.loads(runtime_manifest.read_bytes())
        require(previous.get("schema") == 1 and previous.get("architecture") == architecture,
                "incompatible runtime package manifest")
        runtime = {record["package"]: record for record in previous["packages"]}
        require(all(record["package"] not in runtime or
                    runtime[record["package"]]["source_sha256"] == record["source_sha256"] for record in records),
                "debug input changes a runtime package")
        result["runtime_manifest_sha256"] = sha(runtime_manifest)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--architecture", required=True)
    parser.add_argument("--package", type=json.loads, action="append", required=True)
    parser.add_argument("--context", type=json.loads, default={})
    parser.add_argument("--payload", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--runtime-manifest", type=Path)
    args = parser.parse_args()
    result = receipt(args.package, args.context, args.payload, args.runtime_manifest, architecture=args.architecture)
    args.out.write_text(json.dumps(result, sort_keys=True, indent=2) + "\n")


if __name__ == "__main__":
    main()
