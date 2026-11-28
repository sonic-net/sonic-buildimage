"""Derive pip uninstall whiteouts from installed distribution RECORD files."""
from contextlib import ExitStack
import csv
from email.parser import BytesParser
import io
import json
from pathlib import Path, PurePosixPath
import posixpath
import re
import sys
import tarfile

from merge_layers import Dir, Tree, apply_layer
from tar_filesystem import TarFilesystem


def canonical(name):
    return re.sub(r"[-_.]+", "-", name).lower()


def uninstall(spec, output, audit):
    tree = Tree()
    with ExitStack() as stack:
        for path in spec["layers"]:
            apply_layer(tree, stack.enter_context(tarfile.open(path, "r:*")), path)
        fs = TarFilesystem(tree)
        files, directories = set(), {}
        def walk(directory, prefix=""):
            contents = set()
            for name, node in directory.children.items():
                path = prefix + name
                if isinstance(node, Dir):
                    contents.update(walk(node, path + "/"))
                else:
                    files.add(path)
                    contents.add(path)
            if prefix:
                directories[prefix.rstrip("/")] = contents
            return contents
        walk(tree.root)
        selected = {canonical(name) for name in spec["distributions"]}
        removed, records, owned_roots = set(), {}, set()
        for path in sorted(files):
            if not path.endswith(".dist-info/METADATA"):
                continue
            metadata = BytesParser().parsebytes(fs.read(path))
            name = canonical(metadata["Name"])
            if name not in selected:
                continue
            if name in records:
                raise ValueError("ambiguous installed distribution: " + name)
            info = PurePosixPath(path).parent
            site = str(info.parent)
            record = fs.read(str(info / "RECORD")).decode()
            members = set()
            for row in csv.reader(io.StringIO(record)):
                if not row:
                    continue
                member = posixpath.normpath(site + "/" + row[0])
                if row[0].startswith("/") or member == ".." or member.startswith("../"):
                    raise ValueError("installed RECORD escapes rootfs: " + row[0])
                if member in files:
                    members.add(member)
                    if member.startswith(site + "/"):
                        first = member[len(site) + 1:].split("/", 1)[0]
                        owned_roots.add(site + "/" + first)
                if member.endswith(".py"):
                    source = PurePosixPath(member)
                    for optimize in ("", ".opt-1", ".opt-2"):
                        cached = str(source.parent / "__pycache__" / (source.stem + "." + spec["cache_tag"] + optimize + ".pyc"))
                        if cached in files:
                            members.add(cached)
            removed.update(members)
            records[name] = {"version": metadata["Version"], "record": str(info / "RECORD"), "files": sorted(members)}
        missing = selected - records.keys()
        if missing:
            raise ValueError("requested installed distributions are missing: " + ", ".join(sorted(missing)))
        empty = {path for path, members in directories.items() if members and members <= removed
                 and any(path == root or path.startswith(root + "/") for root in owned_roots)}
        targets = set(removed) | empty
        targets = {path for path in targets if not any(str(parent) in empty for parent in PurePosixPath(path).parents)}
        with tarfile.open(output, "w", format=tarfile.PAX_FORMAT) as archive:
            for path in sorted(targets):
                source = PurePosixPath(path)
                member = tarfile.TarInfo(str(source.parent / (".wh." + source.name)))
                member.mode = 0
                archive.addfile(member)
    Path(audit).write_text(json.dumps({"distributions": records, "whiteouts": sorted(targets)}, indent=2) + "\n")


if __name__ == "__main__":
    uninstall(json.loads(Path(sys.argv[1]).read_text()), *sys.argv[2:])
