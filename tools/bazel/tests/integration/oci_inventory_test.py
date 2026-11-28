#!/usr/bin/env python3
"""Protect shared OCI layer semantics without depending on a container policy.

Both container CI suites run these cases against the same inventory functions
used by APT selection and image validation. Inputs are small TARs and metadata
fixtures; no image, native package or Debian package is built.
"""

from pathlib import Path
import tarfile
import tempfile
import unittest

from tools.bazel.oci import oci_inventory as subject
from tools.bazel.tests.oci_base_fixture import tar_entries as layer


class OciInventoryTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="oci-inventory-test-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)

    def protobuf_link(self, filename, *, mode, uid=0, gid=0,
                      target="libprotobuf.so.32.0.12", kind=tarfile.SYMTYPE):
        path = self.root / filename
        with tarfile.open(path, "w") as archive:
            member = tarfile.TarInfo("usr/lib/x86_64-linux-gnu/libprotobuf.so.32")
            member.type, member.mode = kind, mode
            member.uid, member.gid, member.linkname = uid, gid, target
            archive.addfile(member)
        return path

    def protobuf_base(self):
        path = self.root / "protobuf-base.tar"
        elf = b"\x7fELF\x02\x01" + b"\0" * 10 + b"\x03\0\x3e\0"
        path.write_bytes(layer([("usr/lib/x86_64-linux-gnu/libprotobuf.so.32.0.12", elf, 0o644)]))
        files = {}
        subject.apply_layer(path, files)
        subject.apply_layer(self.protobuf_link("debian-link.tar", mode=0o777), files)
        return files

    def test_source_protobuf_link_matches_installed_debian_link(self):
        """The source TAR's 0000 link and Debian's 0777 link install identically on Linux."""
        files = self.protobuf_base()
        before = dict(files)
        source = self.protobuf_link("source-link.tar", mode=0)
        subject.apply_layer(source, files, checked_overlay=True)
        self.assertEqual(files, before)
        self.assertEqual(files["usr/lib/x86_64-linux-gnu/libprotobuf.so.32"]["mode"], 0o777)
        with tarfile.open(source) as archive:
            self.assertEqual(archive.getmembers()[0].mode, 0)

    def test_symlink_mode_normalization_keeps_target_owner_and_kind_checks(self):
        """Only header mode is irrelevant; a redirected, reowned or replaced ELF link still fails."""
        base = self.protobuf_base()
        replacements = (
            {"target": "libprotobuf.so.other"},
            {"uid": 123},
            {"gid": 456},
            {"kind": tarfile.LNKTYPE, "target": "usr/lib/x86_64-linux-gnu/libprotobuf.so.32.0.12"},
        )
        for replacement in replacements:
            with self.subTest(replacement=replacement):
                source = self.protobuf_link("changed-link.tar", mode=0, **replacement)
                with self.assertRaisesRegex(ValueError, "changes a base ELF link"):
                    subject.apply_layer(source, dict(base), checked_overlay=True)

    def test_hardlink_modes_remain_significant(self):
        """Hardlinks retain their TAR permissions and cannot change an inherited ELF link's mode."""
        files = self.protobuf_base()
        target = "usr/lib/x86_64-linux-gnu/libprotobuf.so.32.0.12"
        subject.apply_layer(self.protobuf_link("hardlink.tar", mode=0o644, kind=tarfile.LNKTYPE,
                                              target=target), files)
        self.assertEqual(files["usr/lib/x86_64-linux-gnu/libprotobuf.so.32"]["mode"], 0o644)
        changed = self.protobuf_link("changed-hardlink.tar", mode=0o777, kind=tarfile.LNKTYPE, target=target)
        with self.assertRaisesRegex(ValueError, "changes a base ELF link"):
            subject.apply_layer(changed, files, checked_overlay=True)

    def test_opaque_whiteout_removes_only_lower_layer_entries(self):
        """Apply an opaque whiteout to older contents while keeping files added in the same layer."""
        first = self.root / "whiteout-base.tar"
        first.write_bytes(layer([("dir/old", b"old", 0o644)]))
        second = self.root / "whiteout-update.tar"
        second.write_bytes(layer([("dir/new", b"new", 0o644), ("dir/.wh..wh..opq", b"", 0o000)]))
        files = {}
        subject.apply_layer(first, files)
        subject.apply_layer(second, files)
        self.assertNotIn("dir/old", files)
        self.assertIn("dir/new", files)

    def test_checked_overlay_rejects_parent_symlink_changes(self):
        """Reject overlays that replace or traverse an inherited directory symlink."""
        base = {"lib": {"kind": "symlink", "linkname": "usr/lib"},
                "usr": {"kind": "directory"}, "usr/lib": {"kind": "directory"}}
        with self.assertRaisesRegex(ValueError, "changes a base directory link"):
            subject.assert_overlay_paths({"lib": {"kind": "directory"}}, base)
        with self.assertRaisesRegex(ValueError, "crosses a non-directory"):
            subject.assert_overlay_paths({"lib/library.so": {"kind": "file"}}, base)
        subject.assert_overlay_paths({"usr/lib/library.so": {"kind": "file"}}, base)

    def test_checked_overlay_preserves_links_to_inherited_elfs(self):
        """Keep links to deployed binaries intact for relative, absolute and hardlink targets."""
        elf = {"kind": "file", "elf_machine": 62, "sha256": "a" * 64}
        for kind, target in (("hardlink", "usr/lib/library.so.1"),
                             ("symlink", "library.so.1"),
                             ("symlink", "/usr/lib/library.so.1")):
            with self.subTest(kind=kind, target=target):
                link = {"kind": kind, "linkname": target}
                base = {"usr/lib/library.so.1": elf, "usr/lib/library.so": link}
                subject.assert_overlay_paths({"usr/lib/library.so": link}, base)
                for replacement in ({**elf, "sha256": "b" * 64},
                                    {"kind": "symlink", "linkname": "other.so"}):
                    with self.assertRaisesRegex(ValueError, "changes a base ELF link"):
                        subject.assert_overlay_paths({"usr/lib/library.so": replacement}, base)

    def test_inherited_elf_links_resolve_image_directory_aliases(self):
        """Resolve image directory aliases when protecting ELF links and tolerate unrelated cycles."""
        base = {"lib": {"kind": "symlink", "linkname": "usr/lib"},
                "usr/lib/library.so.1": {"kind": "file", "elf_machine": 62},
                "usr/lib/library.so": {"kind": "symlink", "linkname": "/lib/library.so.1"},
                "etc/cycle": {"kind": "symlink", "linkname": "cycle"}}
        with self.assertRaisesRegex(ValueError, "changes a base ELF link"):
            subject.assert_overlay_paths({"usr/lib/library.so": {"kind": "symlink", "linkname": "other.so"}}, base)
        subject.assert_overlay_paths({"etc/cycle": {"kind": "file"}}, base)

    def test_checked_overlay_preserves_directories_and_aliases_containing_elfs(self):
        """Reject replacing an ELF-containing directory or redirecting its directory alias."""
        base = {"lib": {"kind": "symlink", "linkname": "usr/lib"},
                "usr/lib": {"kind": "directory"},
                "usr/lib/library.so.1": {"kind": "file", "elf_machine": 62}}
        subject.assert_overlay_paths({"lib": base["lib"], "usr/lib": base["usr/lib"]}, base)
        with self.assertRaisesRegex(ValueError, "changes a base directory link"):
            subject.assert_overlay_paths({"lib": {"kind": "symlink", "linkname": "opt/lib"}}, base)
        for kind in ("symlink", "file"):
            with self.subTest(kind=kind):
                with self.assertRaisesRegex(ValueError, "hides a base ELF directory"):
                    subject.assert_overlay_paths({"usr/lib": {"kind": kind, "linkname": "elsewhere"}}, base)


if __name__ == "__main__":
    unittest.main()
