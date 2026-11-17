"""Inspect OCI layer contents and reject unsafe paths in added layers."""

from pathlib import PurePosixPath
import posixpath

from tools.bazel.ci.validation_inputs import cached_inventory
import tarfile

from tools.bazel.ci.artifact_validation import file_metadata, metadata, path_name, require


def resolve_path(name, files):
    """Follow image-local links without consulting the build host's filesystem."""
    seen = set()
    for _ in range(40):
        if name in seen:
            return None
        seen.add(name)
        parts = PurePosixPath(name).parts
        for index in range(1, len(parts) + 1):
            prefix = "/".join(parts[:index])
            item = files.get(prefix, {})
            if item.get("kind") not in ("symlink", "hardlink"):
                continue
            target = item["linkname"]
            if item["kind"] == "symlink" and not target.startswith("/"):
                target = posixpath.join(posixpath.dirname(prefix), target)
            name = posixpath.normpath(posixpath.join(target.lstrip("/"), *parts[index:]))
            if name == ".." or name.startswith("../"):
                return None
            break
        else:
            return name
    return None


def assert_overlay_paths(entries, files):
    """Require added paths to traverse directories, preserving base directory links."""
    elf_parents = set()
    for name, item in files.items():
        if "elf_machine" in item:
            parts = PurePosixPath(name).parts
            elf_parents.update("/".join(parts[:index]) for index in range(1, len(parts)))
    for name, item in entries.items():
        previous = files.get(name, {})
        require(name not in elf_parents or item["kind"] == "directory",
                "added OCI layer hides a base ELF directory: " + name)
        if previous.get("kind") in ("symlink", "hardlink"):
            target = resolve_path(name, files)
            resolved = files.get(target, {})
            if "elf_machine" in resolved:
                require(item == previous, "added OCI layer changes a base ELF link: " + name)
            if target is not None and (resolved.get("kind") == "directory" or target == "." or
                                       any(path.startswith(target + "/") for path in files)):
                require(item == previous, "added OCI layer changes a base directory link: " + name)
        require(not (item["kind"] == "directory" and files.get(name, {}).get("kind") == "symlink"),
                "added OCI layer replaces a directory symlink: " + name)
        parts = PurePosixPath(name).parts
        for index in range(1, len(parts)):
            parent = "/".join(parts[:index])
            previous = entries.get(parent, files.get(parent))
            require(previous is None or previous["kind"] == "directory",
                    "added OCI layer path crosses a non-directory: " + name + " via " + parent)


def apply_layer(path, files, *, checked_overlay=False, normalize_member=None):
    """Apply tar metadata, contents and OCI whiteouts to an in-memory inventory.

    Owners may normalize known legacy paths before inspection. Added package
    layers may not contain whiteouts or cross inherited directory symlinks.
    """
    entries, whiteouts = cached_inventory(
        path, ("oci_layer", normalize_member), lambda: _layer_entries(path, normalize_member))
    if checked_overlay:
        require(not whiteouts, "added OCI layer contains a whiteout")
        assert_overlay_paths(entries, files)
    for name, opaque in whiteouts:
        prefix = "" if name == "." else name + "/"
        for current in list(files):
            if current.startswith(prefix) or (not opaque and current == name):
                del files[current]
    files.update(entries)
    return entries


def _layer_entries(path, normalize_member):
    entries, whiteouts = {}, []
    with tarfile.open(path, "r:*") as archive:
        for member in archive:
            normalized = normalize_member(member) if normalize_member else member
            if normalized is None:
                continue
            name = path_name(normalized.name)
            pure = PurePosixPath(name)
            if pure.name == ".wh..wh..opq":
                whiteouts.append((str(pure.parent), True))
                continue
            if pure.name.startswith(".wh."):
                whiteouts.append((str(pure.parent / pure.name[4:]), False))
                continue
            item = metadata(normalized)
            if normalized.issym():
                # Ordinary Linux symlinks always have mode 0777 (symlink(7)),
                # even when the TAR header stores 0000, as protobuf's source
                # package does. Compare installed semantics; retain the link's
                # owner and target and every other member's original mode.
                item["mode"] = 0o777
            if member.isfile():
                with archive.extractfile(member) as stream:
                    item.update(file_metadata(stream, name))
            entries[name] = item
    return entries, whiteouts
