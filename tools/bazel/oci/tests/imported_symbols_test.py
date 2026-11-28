"""Check that imported symbols follow deployed bytes, including inherited DWZ.

Synthetic ELF metadata makes corruption and both ELF byte orders reproducible.
The separate native test exercises the same reader against compiled binaries.
"""

import hashlib
import io
import json
from pathlib import Path
import struct
import tarfile
import tempfile
import unittest
import zlib

from tools.bazel.oci.elf import inspect_elf
from tools.bazel.oci.imported_symbols import build
from tools.bazel.tests.oci_base_fixture import digest, oci_files, write_layout


def elf(identifier, *, dwarf=False, debuglink=None, alternate=None, endian="<", wide=True, machine=62):
    sections = [("", 0, b"")]
    note = struct.pack(endian + "III", 4, len(bytes.fromhex(identifier)), 3) + b"GNU\0" + bytes.fromhex(identifier)
    note += b"\0" * (-len(note) % 4)
    sections.append((".note.gnu.build-id", 7, note))
    if dwarf:
        sections.append((".debug_info", 1, b"example dwarf"))
    if debuglink is not None:
        name, crc = debuglink
        data = name.encode() + b"\0"
        data += b"\0" * (-len(data) % 4)
        sections.append((".gnu_debuglink", 1, data + struct.pack(endian + "I", crc)))
    if alternate:
        sections.append((".gnu_debugaltlink", 1, alternate[0].encode() + b"\0" + bytes.fromhex(alternate[1])))
    names = b"\0" + b"".join(name.encode() + b"\0" for name, _, _ in sections[1:]) + b".shstrtab\0"
    sections.append((".shstrtab", 3, names))
    header_size, section_size = (64, 64) if wide else (52, 40)
    data, headers = bytearray(b"\0" * header_size), []
    for name, kind, contents in sections:
        headers.append((names.index(name.encode() + b"\0"), kind, 0, 0, len(data), len(contents), 0, 0, 1, 0))
        data.extend(contents)
    section_offset = len(data)
    for header in headers:
        data.extend(struct.pack(endian + ("IIQQQQIIQQ" if wide else "IIIIIIIIII"), *header))
    data[:16] = b"\x7fELF" + bytes([2 if wide else 1, 1 if endian == "<" else 2, 1]) + b"\0" * 9
    struct.pack_into(endian + ("HHIQQQIHHHHHH" if wide else "HHIIIIIHHHHHH"), data, 16,
                     3, machine, 1, 0, 0, section_offset, 0, header_size, 0, 0, section_size, len(headers), len(headers) - 1)
    return bytes(data)


def archive(path, files):
    with tarfile.open(path, "w") as output:
        for name, contents in files.items():
            member = tarfile.TarInfo(name)
            member.mode, member.uid, member.gid, member.mtime = 0o640, 123, 456, 987
            member.size = len(contents)
            output.addfile(member, io.BytesIO(contents))


def layout(root, layers, architecture="amd64"):
    """Reuse buildimage's existing OCI fixture encoder for synthetic images."""
    payloads = [layer.read_bytes() for layer in layers]
    config = {"os": "linux", "architecture": architecture,
              "rootfs": {"type": "layers", "diff_ids": [digest(layer) for layer in payloads]}}
    write_layout(root, oci_files(json.dumps(config).encode(), payloads))


class ImportedSymbolsTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.identifier = "12" * 20
        self.dwz_id = "34" * 20
        self.path = "usr/lib/libinherited.so"
        self.debug = "usr/lib/debug/.build-id/12/" + "12" * 19 + ".debug"
        self.dwz = "usr/lib/debug/.dwz/example.debug"
        self.symbols = elf(self.identifier, dwarf=True, alternate=("../../.dwz/example.debug", self.dwz_id))
        self.runtime = elf(self.identifier, debuglink=(Path(self.debug).name, zlib.crc32(self.symbols)))
        self.candidates = {self.debug: self.symbols, self.dwz: elf(self.dwz_id, dwarf=True)}

    def run_build(self, runtime=None, candidates=None, extra_layers=(), required=True):
        first = self.root / "runtime.tar"
        archive(first, {self.path: self.runtime} if runtime is None else runtime)
        additional = []
        for index, files in enumerate(extra_layers):
            path = self.root / f"extra{index}.tar"
            archive(path, files)
            additional.append(path)
        layout(self.root / "image", [first, *additional])
        debug = self.root / "debug.tar"
        archive(debug, self.candidates if candidates is None else candidates)
        return build(self.root / "image", [debug], "linux/amd64", [self.path] if required else [],
                     self.root / "selected.tar", self.root / "receipt.json")

    def test_retains_matching_pair_and_dwz_without_package_docs_or_stale_symbols(self):
        """The inherited pair survives while overwritten libraries' symbols and docs stay out."""
        stale = "usr/lib/debug/.build-id/56/" + "56" * 19 + ".debug"
        result = self.run_build(candidates={**self.candidates, stale: elf("56" * 20, dwarf=True),
                                           "usr/share/doc/example/copyright": b"license"})
        self.assertEqual(result["selected_paths"], sorted([self.debug, self.dwz]))
        self.assertEqual(result["excluded_paths"], [stale])
        self.assertEqual(result["pairs"][0]["runtime_sha256"], hashlib.sha256(self.runtime).hexdigest())
        with tarfile.open(self.root / "selected.tar") as output:
            for member in output:
                self.assertEqual((member.mode, member.uid, member.gid, member.mtime), (0o640, 123, 456, 0))
                self.assertEqual(output.extractfile(member).read(), self.candidates[member.name.removeprefix("./")])

    def test_runtime_replacement_excludes_old_symbols(self):
        """Match the final image rather than a base that later source layers replace."""
        result = self.run_build(extra_layers=[{self.path: elf("56" * 20)}], required=False)
        self.assertEqual(result["selected_paths"], [])

    def test_required_coverage_rejects_missing_new_companion(self):
        """A base rebuild must not silently discard required inherited debug coverage."""
        with self.assertRaisesRegex(ValueError, "required runtime"):
            self.run_build(extra_layers=[{self.path: elf("56" * 20)}])

    def test_whiteout_removes_inherited_runtime(self):
        """Removed runtime files must not pull obsolete symbol files into the image."""
        result = self.run_build(extra_layers=[{"usr/lib/.wh.libinherited.so": b""}], required=False)
        self.assertEqual(result["pairs"], [])

    def test_wrong_crc_is_rejected_even_with_same_build_id(self):
        """Build IDs alone cannot prove that a companion matches the deployed bytes."""
        with self.assertRaisesRegex(ValueError, "CRC"):
            self.run_build(candidates={**self.candidates, self.debug: self.symbols + b"changed"})

    def test_wrong_companion_identity_is_rejected(self):
        """A build-ID filename cannot disguise an ELF from another build."""
        with self.assertRaisesRegex(ValueError, "build ID"):
            self.run_build(candidates={**self.candidates, self.debug: elf("56" * 20, dwarf=True)})

    def test_wrong_elf_platform_is_rejected(self):
        """The same build-ID string cannot bind a foreign-architecture companion."""
        with self.assertRaisesRegex(ValueError, "platform"):
            self.run_build(candidates={**self.candidates, self.debug: elf(self.identifier, dwarf=True, machine=183)})

    def test_missing_dwz_is_rejected(self):
        """A companion is incomplete without the supplement named by debugaltlink."""
        with self.assertRaisesRegex(ValueError, "missing DWZ"):
            self.run_build(candidates={self.debug: self.symbols})

    def test_wrong_dwz_build_id_is_rejected(self):
        """A supplement from a different package build cannot satisfy the link."""
        with self.assertRaisesRegex(ValueError, "DWZ supplement identity"):
            self.run_build(candidates={**self.candidates, self.dwz: elf("56" * 20, dwarf=True)})

    def test_absolute_dwz_link_stays_inside_the_image(self):
        """Debian DWZ may store absolute image paths; never consult the host path."""
        symbols = elf(self.identifier, dwarf=True, alternate=("/" + self.dwz, self.dwz_id))
        runtime = elf(self.identifier, debuglink=(Path(self.debug).name, zlib.crc32(symbols)))
        receipt = self.run_build(runtime={self.path: runtime}, candidates={**self.candidates, self.debug: symbols})
        self.assertEqual(receipt["pairs"][0]["supplements"][0]["path"], self.dwz)

    def test_path_traversal_is_rejected(self):
        """Archive paths never escape the image namespace or touch host files."""
        with self.assertRaisesRegex(ValueError, "unsafe archive"):
            self.run_build(candidates={"../outside": b"bad"})

    def test_platform_mismatch_is_rejected(self):
        """The actual OCI config must match the explicitly requested platform."""
        self.run_build()
        with self.assertRaisesRegex(ValueError, "platform"):
            build(self.root / "image", [], "linux/arm64", [], self.root / "other.tar", self.root / "other.json")

    def test_corrupt_oci_layer_is_rejected(self):
        """Validate descriptor hashes before interpreting any runtime contents."""
        self.run_build()
        digest = hashlib.sha256((self.root / "runtime.tar").read_bytes()).hexdigest()
        (self.root / "image/blobs/sha256" / digest).write_bytes(b"corrupt")
        with self.assertRaisesRegex(ValueError, "digest or size"):
            build(self.root / "image", [], "linux/amd64", [], self.root / "other.tar", self.root / "other.json")

    def test_elf_class_and_byte_order_come_from_elf_header(self):
        """Decode both ELF widths and endian variants without using host metadata."""
        for wide in (False, True):
            for endian in ("<", ">"):
                with self.subTest(wide=wide, endian=endian):
                    info = inspect_elf(io.BytesIO(elf(self.identifier, wide=wide, endian=endian, debuglink=("a.debug", 0x12345678))))
                    self.assertEqual(info["build_id"], self.identifier)
                    self.assertEqual(info["debuglink"], {"name": "a.debug", "crc": 0x12345678})

    def test_truncated_elf_metadata_is_rejected(self):
        """Malformed section offsets fail closed rather than truncating the inventory."""
        with self.assertRaisesRegex(ValueError, "section table"):
            inspect_elf(io.BytesIO(self.runtime[:-1]))


if __name__ == "__main__":
    unittest.main()
