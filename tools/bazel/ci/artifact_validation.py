"""Inspect archive payloads and verify their matching split ELF debug symbols."""

import hashlib
from pathlib import Path, PurePosixPath
import re
import struct
import subprocess
import tarfile
import tempfile
import zlib


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def path_name(value):
    path = PurePosixPath(value)
    require(not path.is_absolute() and ".." not in path.parts, "unsafe archive path: " + value)
    return str(path)


def command(arguments, timeout=300, **kwargs):
    result = subprocess.run(arguments, capture_output=True, text=True, timeout=timeout, **kwargs)
    require(result.returncode == 0, "command failed: " + repr(arguments[:8]) + "\n" + result.stdout + result.stderr)
    return result.stdout


def metadata(member):
    kind = ("file" if member.isfile() else "directory" if member.isdir() else
            "symlink" if member.issym() else "hardlink" if member.islnk() else "other")
    result = {"kind": kind, "mode": member.mode, "uid": member.uid, "gid": member.gid}
    if kind in ("symlink", "hardlink"):
        # Layer flattening may add a harmless ./ prefix to link targets.
        # PurePosixPath removes dot components while retaining .. components.
        result["linkname"] = str(PurePosixPath(member.linkname))
    return result


def elf_header(prefix, name):
    """Inspect the common ELF header without reading the rest of a file."""
    if not prefix.startswith(b"\x7fELF"):
        return {}
    require(len(prefix) >= 20 and prefix[:6] == b"\x7fELF\x02\x01", "expected little-endian ELF64: " + name)
    elf_type, elf_machine = struct.unpack_from("<HH", prefix, 16)
    return {"elf_type": elf_type, "elf_machine": elf_machine}


def file_metadata(stream, name):
    """Hash a file stream in bounded memory and record its ELF header when present."""
    prefix = stream.read(64)
    hasher = hashlib.sha256(prefix)
    size = len(prefix)
    for chunk in iter(lambda: stream.read(1024 * 1024), b""):
        hasher.update(chunk)
        size += len(chunk)
    return {"sha256": hasher.hexdigest(), "size": size, **elf_header(prefix, name)}


def payload(path, require_root=True):
    result = {}
    with tarfile.open(path) as archive:
        for member in archive:
            name = path_name(member.name)
            item = metadata(member)
            require(item["kind"] != "other", "unsupported package member: " + name)
            require(not require_root or (member.uid, member.gid) == (0, 0), "non-root package owner: " + name)
            if name in result:
                # Flattened component archives can repeat their shared parent
                # directories. Files and conflicting install metadata may not.
                require(item["kind"] == "directory" and result[name] == item,
                        "duplicate package member: " + name)
                continue
            if member.isfile():
                with archive.extractfile(member) as stream:
                    contents = file_metadata(stream, name)
                # Preserve this public inventory schema for existing consumers.
                contents.pop("elf_type", None)
                item.update(contents)
            result[name] = item
    return result


def elf_info(path, *, readelf="readelf", dynamic=False, timeout=300):
    """Read build ID and debug sections, optionally including runtime dependencies."""
    notes = command([readelf, "-n", str(path)], timeout=timeout)
    sections = command([readelf, "-SW", str(path)], timeout=timeout)
    identifier = re.search(r"Build ID: ([0-9a-f]+)", notes)
    result = {"build_id": identifier[1] if identifier else None,
              "has_dwarf": ".debug_info" in sections or ".zdebug_info" in sections,
              "has_debuglink": ".gnu_debuglink" in sections}
    if dynamic:
        entries = command([readelf, "-d", str(path)], timeout=timeout)
        soname = re.search(r"\(SONAME\).*\[([^]]+)\]", entries)
        result.update(soname=soname[1] if soname else None,
                      needed=re.findall(r"\(NEEDED\).*\[([^]]+)\]", entries))
    return result


def verify_debuglink(binary, symbols, name, *, expected_name=None, objcopy="objcopy", timeout=300):
    """Require the runtime debug link to name and checksum the selected symbols."""
    with tempfile.TemporaryDirectory(prefix="sonic-debuglink-") as temporary:
        link = Path(temporary) / "debuglink"
        # objcopy otherwise rewrites its input even when only dumping a section.
        command([objcopy, "--dump-section", ".gnu_debuglink=" + str(link), str(binary),
                 str(Path(temporary) / "elf-copy")], timeout=timeout)
        raw = link.read_bytes()
    end = raw.index(0)
    require(raw[:end].decode() == (expected_name or symbols.name), "debuglink filename differs: " + name)
    checksum = 0
    with symbols.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            checksum = zlib.crc32(chunk, checksum)
    offset = (end + 4) & ~3
    require(len(raw) >= offset + 4 and struct.unpack_from("<I", raw, offset)[0] == checksum,
            "debuglink CRC differs: " + name)


def elf_debug(extracted, expected, prebuilt, *, readelf="readelf", objcopy="objcopy"):
    pairs, gaps = [], []
    for name, item in expected.items():
        if "elf_machine" not in item or name.startswith("usr/lib/debug/"):
            continue
        binary = extracted / name
        info = elf_info(binary, readelf=readelf)
        identifier = info["build_id"]
        require(identifier is not None, "missing runtime ELF build ID: " + name)
        debug_name = "usr/lib/debug/.build-id/" + identifier[:2] + "/" + identifier[2:] + ".debug"
        require(not info["has_dwarf"], "unstripped runtime ELF: " + name)
        if name in prebuilt:
            require(debug_name not in expected, "prebuilt debug-gap declaration is stale: " + name)
            gaps.append({"path": name, "build_id": identifier, "reason": "producer supplies no matching debug artifact"})
            continue
        require(debug_name in expected, "missing embedded debug file: " + name)
        symbols = extracted / debug_name
        symbol_info = elf_info(symbols, readelf=readelf)
        require(symbol_info["build_id"] == identifier, "debug build ID differs: " + name)
        require(symbol_info["has_dwarf"], "debug file lacks DWARF: " + name)
        verify_debuglink(binary, symbols, name, objcopy=objcopy)
        pairs.append({"path": name, "build_id": identifier, "debug_path": debug_name})
    require(set(prebuilt) == {item["path"] for item in gaps}, "unmatched prebuilt gap declaration")
    return pairs, gaps


def debug_archives(runtime_archive, symbols_archive, *, expected_machine, prebuilt=(),
                   readelf="readelf", objcopy="objcopy"):
    """Validate split-symbol archives; callers retain their package inventory policy."""
    runtime = payload(runtime_archive)
    symbols = payload(symbols_archive)
    require(symbols and all(
        item["kind"] == "directory" or
        re.fullmatch(r"usr/lib/debug/\.build-id/[0-9a-f]{2}/[0-9a-f]+\.debug", name)
        for name, item in symbols.items()), "unexpected debug-symbol payload")
    combined = dict(runtime)
    for name, item in symbols.items():
        require(name not in combined or combined[name] == item,
                "debug symbols change runtime payload: " + name)
        combined[name] = item
    elfs = [name for name, item in combined.items() if "elf_machine" in item]
    require(elfs and all(combined[name]["elf_machine"] == expected_machine for name in elfs),
            "expected only ELF machine " + str(expected_machine))
    with tempfile.TemporaryDirectory(prefix="sonic-ci-symbols-") as temporary:
        extracted = Path(temporary)
        for path in (runtime_archive, symbols_archive):
            with tarfile.open(path) as archive:
                archive.extractall(extracted, filter="data")
        pairs, gaps = elf_debug(extracted, combined, set(prebuilt), readelf=readelf, objcopy=objcopy)
    return {"elf_count": len(elfs), "debug_pairs": pairs, "prebuilt_debug_gaps": gaps}
