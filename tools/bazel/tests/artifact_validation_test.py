#!/usr/bin/env python3
"""Validate archive metadata and real ELF/debug pairs without building packages."""

import hashlib
import io
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tarfile
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

from tools.bazel.ci import artifact_validation as validation


class PayloadTest(unittest.TestCase):
    """Inspect synthetic tar members without extracting their payloads."""

    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.archive = Path(temporary.name) / "payload.tar"

    def write_archive(self, entries):
        with tarfile.open(self.archive, "w") as archive:
            for member, content in entries:
                member.size = len(content) if member.isfile() else 0
                archive.addfile(member, io.BytesIO(content) if member.isfile() else None)

    def test_preserves_bytes_modes_links_and_elf_architecture(self):
        """Keep content hashes, install metadata, normalized links and ELF machine IDs."""
        directory = tarfile.TarInfo("./usr/lib")
        directory.type = tarfile.DIRTYPE
        directory.mode = 0o755
        binary = tarfile.TarInfo("./usr/lib/libexample.so.1")
        binary.mode = 0o755
        content = b"\x7fELF\x02\x01" + bytes(12) + struct.pack("<H", 183)
        symlink = tarfile.TarInfo("usr/lib/libexample.so")
        symlink.type = tarfile.SYMTYPE
        symlink.linkname = "./libexample.so.1"
        hardlink = tarfile.TarInfo("usr/lib/alias.so")
        hardlink.type = tarfile.LNKTYPE
        hardlink.linkname = "./usr/lib/libexample.so.1"
        self.write_archive([(directory, b""), (binary, content), (symlink, b""), (hardlink, b"")])
        result = validation.payload(self.archive)
        self.assertEqual(validation.elf_header(content, "usr/lib/libexample.so.1"),
                         {"elf_type": 0, "elf_machine": 183})
        self.assertEqual(result["usr/lib/libexample.so.1"], {
            "kind": "file", "mode": 0o755, "uid": 0, "gid": 0,
            "size": len(content), "sha256": hashlib.sha256(content).hexdigest(), "elf_machine": 183,
        })
        self.assertEqual(result["usr/lib"]["kind"], "directory")
        self.assertEqual(result["usr/lib/libexample.so"]["linkname"], "libexample.so.1")
        self.assertEqual(result["usr/lib/alias.so"]["kind"], "hardlink")
        self.assertEqual(result["usr/lib/alias.so"]["linkname"], "usr/lib/libexample.so.1")

    def test_rejects_absolute_and_parent_archive_paths(self):
        """Reject member paths that could write outside an extraction directory."""
        for name in ("/etc/passwd", "usr/../../outside"):
            with self.subTest(name=name):
                self.write_archive([(tarfile.TarInfo(name), b"data")])
                with self.assertRaisesRegex(ValueError, "unsafe archive path"):
                    validation.payload(self.archive)

    def test_rejects_duplicate_normalized_paths(self):
        """Reject distinct tar names that resolve to the same installed path."""
        self.write_archive([(tarfile.TarInfo("./usr/file"), b"first"),
                            (tarfile.TarInfo("usr/file"), b"second")])
        with self.assertRaisesRegex(ValueError, "duplicate package member"):
            validation.payload(self.archive)

    def test_non_root_ownership_requires_explicit_opt_out(self):
        """Require root ownership by default while preserving explicitly allowed IDs."""
        member = tarfile.TarInfo("usr/file")
        member.uid, member.gid = 1000, 2000
        self.write_archive([(member, b"data")])
        with self.assertRaisesRegex(ValueError, "non-root package owner"):
            validation.payload(self.archive)
        item = validation.payload(self.archive, require_root=False)["usr/file"]
        self.assertEqual((item["uid"], item["gid"]), (1000, 2000))

    def test_accepts_identical_directories_from_flattened_archives(self):
        entries = []
        for name in ("./usr/lib/debug/.build-id", "usr/lib/debug/.build-id"):
            member = tarfile.TarInfo(name)
            member.type, member.mode = tarfile.DIRTYPE, 0o755
            entries.append((member, b""))
        self.write_archive(entries)
        self.assertEqual(validation.payload(self.archive), {
            "usr/lib/debug/.build-id": {"kind": "directory", "mode": 0o755, "uid": 0, "gid": 0},
        })

    def test_rejects_conflicting_directory_metadata_or_kind(self):
        for attribute, value in (("mode", 0o700), ("uid", 12), ("gid", 34), ("type", tarfile.REGTYPE)):
            with self.subTest(attribute=attribute):
                first, second = tarfile.TarInfo("usr/lib"), tarfile.TarInfo("./usr/lib")
                first.type = second.type = tarfile.DIRTYPE
                setattr(second, attribute, value)
                self.write_archive([(first, b""), (second, b"")])
                with self.assertRaisesRegex(ValueError, "duplicate package member"):
                    validation.payload(self.archive, require_root=False)

    def test_rejects_special_files(self):
        """Reject device nodes instead of treating them as ordinary package files."""
        member = tarfile.TarInfo("dev/device")
        member.type = tarfile.CHRTYPE
        self.write_archive([(member, b"")])
        with self.assertRaisesRegex(ValueError, "unsupported package member"):
            validation.payload(self.archive)

    def test_rejects_wrong_elf_class_and_endianness(self):
        """Reject ELF32 and big-endian headers before recording architecture metadata."""
        for header in (b"\x7fELF\x01\x01", b"\x7fELF\x02\x02"):
            with self.subTest(header=header):
                self.write_archive([(tarfile.TarInfo("usr/bin/tool"), header + bytes(20))])
                with self.assertRaisesRegex(ValueError, "expected little-endian ELF64"):
                    validation.payload(self.archive)


@unittest.skipUnless(sys.platform.startswith("linux") and
                     all(shutil.which(tool) for tool in ("cc", "readelf", "objcopy")),
                     "ELF tests require a native Linux C compiler and binutils")
class ElfDebugTest(unittest.TestCase):
    """Check archive policy and split symbols using a tiny native C executable."""

    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.name = "usr/bin/example"
        self.binary = self.root / self.name
        self.binary.parent.mkdir(parents=True)
        source = self.root / "example.c"
        source.write_text("int main(void) { return 0; }\n")
        self.run_tool("cc", "-g", "-Wl,--build-id=sha1", str(source), "-o", str(self.binary))
        notes = self.run_tool("readelf", "-n", str(self.binary))
        self.identifier = re.search(r"Build ID: ([0-9a-f]+)", notes)[1]
        self.debug_name = ("usr/lib/debug/.build-id/" + self.identifier[:2]
                           + "/" + self.identifier[2:] + ".debug")
        self.symbols = self.root / self.debug_name
        self.symbols.parent.mkdir(parents=True)
        self.unstripped = self.root / "unstripped"
        shutil.copyfile(self.binary, self.unstripped)
        self.run_tool("objcopy", "--only-keep-debug", str(self.binary), str(self.symbols))
        self.run_tool("objcopy", "--strip-debug", "--add-gnu-debuglink=" + str(self.symbols),
                      str(self.binary))
        machine = struct.unpack_from("<H", self.binary.read_bytes(), 18)[0]
        self.expected = {self.name: {"elf_machine": machine}, self.debug_name: {"elf_machine": machine}}

    def run_tool(self, *arguments):
        return subprocess.run(arguments, check=True, capture_output=True, text=True, timeout=30).stdout

    def archives(self, *, runtime_name=None, extra_runtime=(), symbol_name=None):
        runtime = self.root / "runtime.tar"
        symbols = self.root / "symbols.tar"
        entries = ((runtime, [(runtime_name or self.name, self.binary.read_bytes()), *extra_runtime]),
                   (symbols, [(symbol_name or self.debug_name, self.symbols.read_bytes())]))
        for path, members in entries:
            with tarfile.open(path, "w") as archive:
                for name, content in members:
                    info = tarfile.TarInfo(name)
                    info.mode = 0o755 if path == runtime else 0o644
                    info.size = len(content)
                    archive.addfile(info, io.BytesIO(content))
        return runtime, symbols

    def test_debug_archives_support_unrelated_runtime_install_paths(self):
        """Match runtime and debug archives without assuming a component's install paths."""
        for name in ("usr/bin/telemetry-service", "opt/routing/lib/libroute.so"):
            with self.subTest(name=name):
                runtime, symbols = self.archives(runtime_name=name)
                result = validation.debug_archives(runtime, symbols,
                                                   expected_machine=self.expected[self.name]["elf_machine"])
                self.assertEqual(result, {
                    "elf_count": 2,
                    "debug_pairs": [{"path": name, "build_id": self.identifier, "debug_path": self.debug_name}],
                    "prebuilt_debug_gaps": [],
                })

    def test_debug_archives_reject_foreign_symbol_paths(self):
        """Reject debug archives containing files outside the build-ID directory layout."""
        runtime, symbols = self.archives(symbol_name="usr/share/unrelated-data")
        with self.assertRaisesRegex(ValueError, "unexpected debug-symbol payload"):
            validation.debug_archives(runtime, symbols, expected_machine=self.expected[self.name]["elf_machine"])

    def test_debug_archives_reject_runtime_symbol_collisions(self):
        """Prevent a debug archive from replacing different bytes in the runtime layer."""
        runtime, symbols = self.archives(extra_runtime=[(self.debug_name, b"different runtime bytes")])
        with self.assertRaisesRegex(ValueError, "debug symbols change runtime payload"):
            validation.debug_archives(runtime, symbols, expected_machine=self.expected[self.name]["elf_machine"])

    def test_debug_archives_enforce_the_callers_architecture(self):
        """Reject otherwise valid ELF pairs when they target a different machine."""
        runtime, symbols = self.archives()
        wrong_machine = 183 if self.expected[self.name]["elf_machine"] == 62 else 62
        with self.assertRaisesRegex(ValueError, "expected only ELF machine " + str(wrong_machine)):
            validation.debug_archives(runtime, symbols, expected_machine=wrong_machine)

    def test_debug_archives_prevent_links_escaping_the_extraction_directory(self):
        """Reject an escaping symlink and leave its external target unchanged."""
        runtime, symbols = self.archives()
        outside = self.root / "outside"
        outside.write_bytes(b"unchanged")
        with tarfile.open(runtime, "a") as archive:
            link = tarfile.TarInfo("usr/share/outside")
            link.type = tarfile.SYMTYPE
            link.linkname = str(outside)
            archive.addfile(link)
        with self.assertRaises(tarfile.FilterError):
            validation.debug_archives(runtime, symbols, expected_machine=self.expected[self.name]["elf_machine"])
        self.assertEqual(outside.read_bytes(), b"unchanged")

    def test_accepts_matching_split_debug_symbols(self):
        """Recognize a stripped executable and its matching build-ID debug file."""
        pairs, gaps = validation.elf_debug(self.root, self.expected, set())
        self.assertEqual(pairs, [{"path": self.name, "build_id": self.identifier,
                                  "debug_path": self.debug_name}])
        self.assertEqual(gaps, [])

    def test_read_only_runtime_is_never_rewritten_during_symbol_validation(self):
        """Inspect a packaged read-only library without requiring a writable ELF."""
        self.binary.chmod(0o555)
        os.utime(self.binary, (1700000000, 1700000000))
        before = self.binary.stat()
        contents = self.binary.read_bytes()
        pairs, gaps = validation.elf_debug(self.root, self.expected, set())
        self.assertEqual(len(pairs), 1)
        self.assertFalse(gaps)
        self.assertEqual(self.binary.read_bytes(), contents)
        after = self.binary.stat()
        self.assertEqual((after.st_mode, after.st_mtime_ns), (before.st_mode, before.st_mtime_ns))

    def test_rejects_unstripped_runtime(self):
        """Fail when the runtime executable still contains its debug information."""
        shutil.copyfile(self.unstripped, self.binary)
        with self.assertRaisesRegex(ValueError, "unstripped runtime ELF"):
            validation.elf_debug(self.root, self.expected, set())

    def test_rejects_missing_debug_payload(self):
        """Require the matching debug file to appear in the declared payload inventory."""
        del self.expected[self.debug_name]
        with self.assertRaisesRegex(ValueError, "missing embedded debug file"):
            validation.elf_debug(self.root, self.expected, set())

    def test_rejects_changed_debug_bytes_with_same_build_id(self):
        """Use the debuglink CRC to catch altered symbol bytes despite an unchanged ID."""
        with self.symbols.open("ab") as output:
            output.write(b"changed debug artifact")
        with self.assertRaisesRegex(ValueError, "debuglink CRC differs"):
            validation.elf_debug(self.root, self.expected, set())

    def test_rejects_debug_file_without_dwarf(self):
        """Require useful DWARF data rather than accepting a debug file by name alone."""
        self.run_tool("objcopy", "--strip-debug", str(self.symbols))
        with self.assertRaisesRegex(ValueError, "debug file lacks DWARF"):
            validation.elf_debug(self.root, self.expected, set())

    def test_requires_exact_prebuilt_gap_declarations(self):
        """Allow declared missing symbols while rejecting stale or unmatched exceptions."""
        with self.assertRaisesRegex(ValueError, "prebuilt debug-gap declaration is stale"):
            validation.elf_debug(self.root, self.expected, {self.name})
        del self.expected[self.debug_name]
        self.symbols.unlink()
        pairs, gaps = validation.elf_debug(self.root, self.expected, {self.name})
        self.assertEqual(pairs, [])
        self.assertEqual(gaps, [{"path": self.name, "build_id": self.identifier,
                                "reason": "producer supplies no matching debug artifact"}])
        with self.assertRaisesRegex(ValueError, "unmatched prebuilt gap declaration"):
            validation.elf_debug(self.root, {}, {"usr/bin/missing"})


if __name__ == "__main__":
    unittest.main()
