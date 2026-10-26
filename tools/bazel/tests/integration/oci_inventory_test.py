#!/usr/bin/env python3
"""Protect shared OCI layer semantics without depending on a container policy.

Both container CI suites run these cases against the same inventory functions
used by APT selection and image validation. Inputs are small TARs and metadata
fixtures; no image, native package or Debian package is built.
"""

from pathlib import Path
import tempfile
import unittest

from tools.bazel.oci import oci_inventory as subject
from tools.bazel.tests.oci_base_fixture import tar_entries as layer


class OciInventoryTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="oci-inventory-test-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)

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
