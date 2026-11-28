#!/usr/bin/env python3
"""Exercise real GDB .build-id split-symbol lookup; these fixtures do not build DEBs."""

from pathlib import Path
import json
import platform
import shutil
import struct
import subprocess
import sys
import tarfile
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

from tools.bazel.ci.verify_p4rt_debug import elf_sections, verify_pair


@unittest.skipUnless(platform.system() == "Linux" and platform.machine() == "x86_64"
                     and all(shutil.which(tool) for tool in ("gcc", "objcopy", "strip", "gdb")),
                     "Requires native AMD64 GCC, binutils and GDB")
class P4rtDebugTest(unittest.TestCase):
    """A populated .build-id debug ELF must independently supply matching function debug data."""

    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.source = self.root / "p4rt_app/p4rt.cc"
        self.source.parent.mkdir()
        self.source.write_text("int main(int argc, char** argv) {\n"
                               "  return argc + (argv[0][0] == 'x');\n}\n")
        self.binary, self.symbols, self.unstripped = self.compile(self.root / "build")

    def tool(self, *arguments):
        subprocess.run(arguments, cwd=self.root, check=True, capture_output=True, timeout=30)

    def compile(self, directory):
        directory.mkdir()
        unstripped = directory / "p4rt.unstripped"
        binary, symbols = directory / "p4rt", directory / "p4rt.debug"
        self.tool("gcc", "-x", "c++", "-O2", "-g", "-Wl,--build-id=sha1",
                  str(self.source), "-o", str(unstripped))
        self.tool("objcopy", "--only-keep-debug", str(unstripped), str(symbols))
        shutil.copyfile(unstripped, binary)
        self.tool("strip", "--strip-all", str(binary))
        return binary, symbols, unstripped

    def test_matching_build_id_debug_supplies_function_types_and_source_line(self):
        """Resolve main through .build-id debug directory with stripped runtime binary."""
        receipt = verify_pair(self.binary, self.symbols, self.source)
        self.assertEqual(receipt["status"], "passed")
        self.assertFalse(receipt["without_symbols"]["found"])
        self.assertTrue(receipt["with_symbols"]["file"].endswith("p4rt_app/p4rt.cc"))
        self.assertEqual(receipt["debug_path"],
                         f"usr/lib/debug/.build-id/{receipt['build_id'][:2]}/{receipt['build_id'][2:]}.debug")

    def test_unstripped_runtime_binary_is_rejected(self):
        """Reject embedded symbols in the runtime binary instead of false-passing."""
        with self.assertRaisesRegex(ValueError, "not stripped"):
            verify_pair(self.unstripped, self.symbols, self.source)

    def test_empty_debug_file_is_not_a_valid_debug_package(self):
        """Catch zero-byte debug symbol artifacts."""
        self.symbols.write_bytes(b"")
        with self.assertRaisesRegex(ValueError, "nonempty AMD64 ELF"):
            verify_pair(self.binary, self.symbols, self.source)

    def test_unrelated_nonempty_debug_file_cannot_supply_matching_symbols(self):
        """Nonempty DWARF with a mismatched build ID must be rejected."""
        self.source.write_text("int main(int count, char** values) {\n"
                               "  volatile long different = 42;\n  return count + different;\n}\n")
        _, other_symbols, _ = self.compile(self.root / "other")
        with self.assertRaisesRegex(ValueError, "build ID does not match"):
            verify_pair(self.binary, other_symbols, self.source)

    def test_missing_debug_info_is_rejected(self):
        """Reject a debug ELF whose .debug_info payload was stripped."""
        self.tool("objcopy", "--remove-section=.debug_info", str(self.symbols))
        with self.assertRaisesRegex(ValueError, "lacks populated DWARF"):
            verify_pair(self.binary, self.symbols, self.source)

    def test_wrong_elf_architecture_is_rejected(self):
        """The existing P4RT package contract is AMD64."""
        with self.symbols.open("r+b") as stream:
            stream.seek(18)
            stream.write(struct.pack("<H", 183))
        with self.assertRaisesRegex(ValueError, "AMD64 ELF"):
            verify_pair(self.binary, self.symbols, self.source)

    def test_tar_cli_checks_payload_inventory_and_isolated_gdb(self):
        """Exercise TAR loading through the public CLI with real matching .build-id symbols."""
        from tools.bazel.ci.verify_p4rt_debug import RUNTIME_FILES

        bid = elf_sections(self.binary)["build_id"]
        runtime, debug = self.root / "runtime.tar.bz2", self.root / "debug.tar.bz2"
        with tarfile.open(runtime, "w:bz2") as archive:
            for name in sorted(RUNTIME_FILES):
                member = archive.gettarinfo(self.binary, arcname="./" + name)
                member.mode, member.uid, member.gid = 0o755, 0, 0
                with self.binary.open("rb") as stream:
                    archive.addfile(member, stream)
        debug_paths = [f"usr/lib/debug/.build-id/{bid[:2]}/{bid[2:]}.debug"] + [
            f"usr/lib/debug/.build-id/{i:02x}/{'0' * 38}.debug" for i in range(1, len(RUNTIME_FILES))
        ]
        with tarfile.open(debug, "w:bz2") as archive:
            for name in sorted(debug_paths):
                member = archive.gettarinfo(self.symbols, arcname="./" + name)
                member.mode, member.uid, member.gid = 0o644, 0, 0
                with self.symbols.open("rb") as stream:
                    archive.addfile(member, stream)
        output = self.root / "receipt.json"
        self.tool(sys.executable, str(Path(__file__).resolve().parents[1] / "ci/verify_p4rt_debug.py"),
                  "--runtime-tar", str(runtime), "--debug-tar", str(debug),
                  "--source-root", str(self.root), "--output", str(output))
        receipt = json.loads(output.read_text())
        self.assertEqual(receipt["status"], "passed")
        self.assertEqual(receipt["payloads"]["runtime"], sorted(RUNTIME_FILES))
        self.assertEqual(receipt["payloads"]["debug"], sorted(debug_paths))
        self.assertFalse(receipt["without_symbols"]["found"])
        self.assertTrue(receipt["with_symbols"]["found"])


if __name__ == "__main__":
    unittest.main()
