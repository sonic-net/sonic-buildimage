#!/usr/bin/env python3
"""Check packaged P4RT .build-id split symbols with GDB, without running the executable."""

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import struct
import subprocess
import tarfile
import tempfile


RUNTIME_FILES = {
    "usr/local/bin/" + name for name in (
        "p4rt", "p4rt_program_table", "p4rt_read",
        "p4rt_set_forwarding_pipeline", "p4rt_write",
    )
}
BUILD_ID_DEBUG_RE = re.compile(r"usr/lib/debug/\.build-id/([0-9a-f]{2})/([0-9a-f]{30,})\.debug")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def elf_sections(path):
    """Read ELF64 section metadata and optional NT_GNU_BUILD_ID."""
    with path.open("rb") as stream:
        header = stream.read(64)
        require(len(header) == 64 and header[:6] == b"\x7fELF\x02\x01"
                and struct.unpack_from("<H", header, 18)[0] == 62,
                f"Expected nonempty AMD64 ELF: {path.name}")
        offset = struct.unpack_from("<Q", header, 40)[0]
        entry_size, count, names_index = struct.unpack_from("<HHH", header, 58)
        require(entry_size == 64 and 0 < names_index < count, "Unsupported ELF section table")
        require(offset + count * entry_size <= path.stat().st_size, "Truncated ELF section table")
        stream.seek(offset)
        entries = [struct.unpack("<IIQQQQIIQQ", stream.read(64)) for _ in range(count)]
        names_entry = entries[names_index]
        stream.seek(names_entry[4])
        names = stream.read(names_entry[5])
        sections = {}
        for entry in entries:
            name = names[entry[0]:].split(b"\0", 1)[0].decode("ascii")
            sections[name] = (entry[4], entry[5])
        note = sections.get(".note.gnu.build-id")
        if note and note[1] >= 16:
            stream.seek(note[0])
            raw = stream.read(note[1])
            pos = 0
            while pos + 12 <= len(raw):
                namesz, descsz, note_type = struct.unpack_from("<III", raw, pos)
                name_start = pos + 12
                desc_start = name_start + ((namesz + 3) & ~3)
                desc_end = desc_start + descsz
                if desc_end > len(raw):
                    break
                if note_type == 3 and raw[name_start:name_start + namesz] == b"GNU\0" and descsz >= 16:
                    sections["build_id"] = raw[desc_start:desc_end].hex()
                    break
                pos = desc_end + ((descsz + 3) & ~3)
        return sections


def elf_build_id(path):
    bid = elf_sections(path).get("build_id")
    require(bid, f"Missing NT_GNU_BUILD_ID in ELF: {path.name}")
    return bid


def extract_deb_member(package, member_name, destination):
    """Copy one regular member; never install a package or extract its links."""
    found = False
    with tempfile.TemporaryFile() as errors:
        process = subprocess.Popen(["dpkg-deb", "--fsys-tarfile", str(package)],
                                   stdout=subprocess.PIPE, stderr=errors)
        try:
            with tarfile.open(fileobj=process.stdout, mode="r|") as archive:
                for member in archive:
                    if str(PurePosixPath(member.name)) != member_name:
                        continue
                    require(not found and member.isfile(), "Duplicate or non-regular " + member_name)
                    with archive.extractfile(member) as source, destination.open("wb") as output:
                        shutil.copyfileobj(source, output)
                    found = True
            process.stdout.close()
            require(process.wait(timeout=60) == 0, "Cannot decode DEB: " + str(package))
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
    require(found, "Package is missing " + member_name)


def extract_member(package, member_name, destination):
    """Copy one regular TAR member without installing or following archive links."""
    found = False
    with tarfile.open(package, mode="r:*") as archive:
        for member in archive:
            if str(PurePosixPath(member.name)) != member_name:
                continue
            require(not found and member.isfile(), "Duplicate or non-regular " + member_name)
            with archive.extractfile(member) as source, destination.open("wb") as output:
                shutil.copyfileobj(source, output)
            found = True
    require(found, "Package is missing " + member_name)


def verify_inventory(package, expected, *, mode=0o755):
    """Check the owner's payload paths, modes and root ownership."""
    found = set()
    with tarfile.open(package, mode="r:*") as archive:
        for member in archive:
            path = PurePosixPath(member.name)
            require(not path.is_absolute() and ".." not in path.parts,
                    "Unsafe TAR member: " + member.name)
            if member.isdir():
                continue
            name = str(path)
            require(member.isfile() and name not in found and name in expected,
                    "Unexpected, duplicate or non-regular TAR member: " + name)
            require(member.mode == mode and member.uid == 0 and member.gid == 0,
                    "Unexpected TAR permissions or ownership: " + name)
            require(member.size > 0, "Empty TAR member: " + name)
            found.add(name)
    require(found == expected, "Missing TAR members: " + ", ".join(sorted(expected - found)))
    return sorted(found)


def verify_debug_inventory(package, expected_count=len(RUNTIME_FILES)):
    """Check .build-id debug files, permissions (0644), and root ownership in TAR."""
    found = set()
    with tarfile.open(package, mode="r:*") as archive:
        for member in archive:
            path = PurePosixPath(member.name)
            require(not path.is_absolute() and ".." not in path.parts,
                    "Unsafe TAR member: " + member.name)
            if member.isdir():
                continue
            name = str(path)
            require(member.isfile() and name not in found and BUILD_ID_DEBUG_RE.fullmatch(name),
                    "Unexpected, duplicate or non-regular debug TAR member: " + name)
            require(member.mode == 0o644 and member.uid == 0 and member.gid == 0,
                    "Unexpected debug TAR permissions or ownership: " + name)
            require(member.size > 0, "Empty debug TAR member: " + name)
            found.add(name)
    require(len(found) == expected_count,
            f"Expected {expected_count} .build-id debug files, found {len(found)}")
    return sorted(found)


def lookup_main(binary, directory, debug_directory=None):
    """A fresh GDB process must load real function/type DIEs, not minimal symbols."""
    debug_dir = directory / "empty" if debug_directory is None else debug_directory
    script = directory / "lookup.gdb"
    script.write_text("""python
import gdb, json
try:
    location = gdb.decode_line("main")[1][0]
    block = gdb.block_for_pc(location.pc)
    while block is not None and block.function is None:
        block = block.superblock
    symbol = block.function if block is not None else None
    if symbol is None or symbol.symtab is None:
        raise ValueError("main has no debug symbol")
    if symbol.name.split("(", 1)[0] != "main":
        raise ValueError("main address resolves to a different function")
    parameters = [str(field.type) for field in symbol.type.fields()]
    if len(parameters) != 2 or parameters[0] != "int":
        raise ValueError("main has no expected parameter type information")
    result = {"found": True, "file": location.symtab.filename,
              "line": location.line, "parameters": parameters}
except Exception as error:
    result = {"found": False, "error": str(error)}
print("P4RT_DEBUG_LOOKUP=" + json.dumps(result))
end
""")
    environment = dict(os.environ, LC_ALL="C", DEBUGINFOD_URLS="")
    result = subprocess.run([
        "gdb", "-nx", "-nh", "--batch", "-iex", "set auto-load off",
        "-iex", "set debuginfod enabled off", "-iex", "set pagination off",
        "-iex", "set debug-file-directory " + str(debug_dir),
        "-ex", "file " + json.dumps(str(binary)), "-x", str(script),
    ], cwd=directory, env=environment, text=True, capture_output=True, timeout=240)
    require(result.returncode == 0, "GDB failed:\nstdout:\n" + result.stdout[-4000:]
            + "\nstderr:\n" + result.stderr[-4000:])
    matches = [line.removeprefix("P4RT_DEBUG_LOOKUP=") for line in result.stdout.splitlines()
               if line.startswith("P4RT_DEBUG_LOOKUP=")]
    require(len(matches) == 1, "GDB produced no unique debug lookup result")
    lookup = json.loads(matches[0])
    if not lookup["found"]:
        lookup["diagnostics"] = {"stdout": result.stdout[-4000:], "stderr": result.stderr[-4000:]}
    return lookup


def verify_pair(binary, symbols, source):
    """Require stripped runtime ELF and matching .build-id debug ELF; reject fallbacks."""
    runtime_sections = elf_sections(binary)
    bid = runtime_sections.get("build_id")
    require(bid, "Runtime ELF lacks NT_GNU_BUILD_ID")
    require(".symtab" not in runtime_sections and not any(s.startswith(".debug_") for s in runtime_sections),
            "Runtime binary is not stripped")
    sym_sections = elf_sections(symbols)
    require(sym_sections.get("build_id") == bid, "Debug symbol build ID does not match runtime binary")
    require(sym_sections.get(".debug_info", (0, 0))[1] > 0 and sym_sections.get(".debug_abbrev", (0, 0))[1] > 0,
            "Debug symbol file lacks populated DWARF sections")
    lines = source.read_text().splitlines()
    definitions = [number for number, line in enumerate(lines, 1)
                   if re.match(r"\s*int\s+main\s*\(", line)]
    require(len(definitions) == 1, "Expected one main definition in p4rt_app/p4rt.cc")
    with tempfile.TemporaryDirectory(prefix="p4rt-debug-check-") as temporary:
        directory = Path(temporary)
        (directory / "empty").mkdir()
        isolated_binary = directory / "p4rt"
        shutil.copyfile(binary, isolated_binary)
        without = lookup_main(isolated_binary, directory, directory / "empty")
        require(not without["found"], "Debug lookup succeeds without split debug symbols")
        debug_dir = directory / "debug"
        debug_path = debug_dir / ".build-id" / bid[:2] / (bid[2:] + ".debug")
        debug_path.parent.mkdir(parents=True)
        shutil.copyfile(symbols, debug_path)
        with_symbols = lookup_main(isolated_binary, directory, debug_dir)
        require(with_symbols["found"], "Split debug symbols cannot resolve main: "
                + with_symbols.get("error", "unknown")
                + "\n" + json.dumps(with_symbols.get("diagnostics", {}), indent=2))
        require(with_symbols["file"].replace("\\", "/").endswith("p4rt_app/p4rt.cc"),
                "GDB resolved main from an unexpected source file")
        line = with_symbols["line"]
        require(definitions[0] <= line <= len(lines), "GDB source line is outside the supplied main function")
    return {"status": "passed", "runtime_sha256": sha256(binary), "debug_sha256": sha256(symbols),
            "debug_size": symbols.stat().st_size, "build_id": bid,
            "debug_path": f"usr/lib/debug/.build-id/{bid[:2]}/{bid[2:]}.debug",
            "without_symbols": {key: value for key, value in without.items() if key != "diagnostics"},
            "with_symbols": with_symbols,
            "source_sha256": sha256(source), "source_line": lines[line - 1],
            "scope": "Offline GDB .build-id symbol/type/source-line lookup; executable not run"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    runtime = parser.add_mutually_exclusive_group(required=True)
    runtime.add_argument("--runtime-tar", type=Path)
    runtime.add_argument("--runtime-deb", type=Path)
    debug = parser.add_mutually_exclusive_group(required=True)
    debug.add_argument("--debug-tar", type=Path)
    debug.add_argument("--debug-deb", type=Path)
    parser.add_argument("--source-root", required=True, type=Path,
                        help="Matching sonic-pins checkout containing p4rt_app/p4rt.cc")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if bool(args.runtime_tar) != bool(args.debug_tar):
        parser.error("runtime and debug inputs must use the same archive format")
    runtime_path = args.runtime_tar or args.runtime_deb
    debug_path = args.debug_tar or args.debug_deb
    payloads = None
    if args.runtime_tar:
        payloads = {
            "runtime": verify_inventory(runtime_path, RUNTIME_FILES, mode=0o755),
            "debug": verify_debug_inventory(debug_path, expected_count=len(RUNTIME_FILES)),
        }
    extract = extract_member if args.runtime_tar else extract_deb_member
    with tempfile.TemporaryDirectory(prefix="p4rt-debug-packages-") as temporary:
        directory = Path(temporary)
        binary, symbols = directory / "p4rt", directory / "p4rt.debug"
        extract(runtime_path, "usr/local/bin/p4rt", binary)
        bid = elf_build_id(binary)
        debug_member = f"usr/lib/debug/.build-id/{bid[:2]}/{bid[2:]}.debug"
        if payloads is not None:
            require(debug_member in payloads["debug"], "Debug TAR misses " + debug_member)
        extract(debug_path, debug_member, symbols)
        receipt = verify_pair(binary, symbols, args.source_root / "p4rt_app/p4rt.cc")
    receipt["packages"] = {"runtime_sha256": sha256(runtime_path),
                           "debug_sha256": sha256(debug_path)}
    if payloads is not None:
        receipt["payloads"] = payloads
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
