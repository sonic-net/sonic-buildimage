"""Check shared archive publication and optional cache settings."""

import errno
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

from tools.bazel import build_helpers
from tools.bazel.image import file_copy
from tools.bazel.oci import boot_layout
from tools.bazel.tests.oci_base_fixture import image_fixture, oci_files, write_layout


class BuildHelpersTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.destination = self.root / "target/container.gz"
        self.destination.parent.mkdir()
        self.destination.write_bytes(b"previous image")

    def test_failed_copy_preserves_previous_image(self):
        """Keep the last good image and remove staging files when archive copying fails."""
        source = self.root / "source"
        source.write_bytes(b"new image")
        with patch.object(build_helpers, "copy_file", side_effect=OSError("disk full")):
            with self.assertRaises(OSError):
                build_helpers.export_archive(source, self.destination)
        self.assertEqual(self.destination.read_bytes(), b"previous image")
        self.assertEqual(list(self.destination.parent.glob("container.gz.*")), [])

    def test_reflink_publication_has_an_independent_inode_and_preserves_source(self):
        source = self.root / "source"
        source.write_bytes(b"new image")

        def clone(destination, operation, original):
            os.write(destination, os.read(original, 4096))

        with patch.object(file_copy.fcntl, "ioctl", side_effect=clone) as ioctl, \
                patch.object(file_copy.shutil, "copyfile") as fallback:
            build_helpers.export_archive(source, self.destination)
        ioctl.assert_called_once()
        fallback.assert_not_called()
        self.assertFalse(source.samefile(self.destination))
        self.assertEqual(self.destination.stat().st_mode & 0o777, 0o644)
        self.destination.write_bytes(b"changed")
        self.assertEqual(source.read_bytes(), b"new image")

    def test_unsupported_reflink_falls_back_and_copy_failure_is_atomic(self):
        source = self.root / "source"
        source.write_bytes(b"new image")
        for error in (errno.EXDEV, errno.EOPNOTSUPP, errno.ENOTTY, errno.EINVAL):
            with self.subTest(error=error):
                with patch.object(file_copy.fcntl, "ioctl", side_effect=OSError(error, "unsupported")):
                    build_helpers.export_archive(source, self.destination)
                self.assertEqual(self.destination.read_bytes(), source.read_bytes())
                self.destination.write_bytes(b"previous image")
        with patch.object(file_copy.fcntl, "ioctl", side_effect=OSError(errno.EOPNOTSUPP, "unsupported")), \
                patch.object(file_copy.shutil, "copyfile", side_effect=OSError(errno.ENOSPC, "full")):
            with self.assertRaises(OSError):
                build_helpers.export_archive(source, self.destination)
        self.assertEqual(self.destination.read_bytes(), b"previous image")
        self.assertEqual(list(self.destination.parent.glob("container.gz.*")), [])

    def test_reflink_storage_errors_do_not_retry_and_samefile_is_not_truncated(self):
        source = self.root / "source"
        source.write_bytes(b"new image")
        with patch.object(file_copy.fcntl, "ioctl", side_effect=OSError(errno.ENOSPC, "full")), \
                patch.object(file_copy.shutil, "copyfile") as fallback:
            with self.assertRaises(OSError):
                build_helpers.export_archive(source, self.destination)
        fallback.assert_not_called()
        self.assertEqual(self.destination.read_bytes(), b"previous image")
        with self.assertRaises(file_copy.shutil.SameFileError):
            file_copy.copy_file(source, source)
        self.assertEqual(source.read_bytes(), b"new image")

    def test_unchanged_archive_keeps_timestamp(self):
        """Avoid invalidating downstream Make targets when the exported archive bytes are
        unchanged.
        """
        source = self.root / "source"
        source.write_bytes(self.destination.read_bytes())
        before = self.destination.stat().st_mtime_ns
        build_helpers.export_archive(source, self.destination)
        self.assertEqual(self.destination.stat().st_mtime_ns, before)

    def test_optional_cache_does_not_change_normal_bazel_defaults(self):
        """Leave Bazel cache and output settings untouched when no shared cache is requested."""
        self.assertEqual(build_helpers.cache_options(None), [])
        self.assertEqual(build_helpers.cache_options(""), [])

    def test_direct_handoff_is_bound_to_selected_archive_and_build(self):
        layout = self.root / "oci"
        write_layout(layout, oci_files(*image_fixture()))
        source = self.root / "source"
        source.write_bytes(b"new archive")
        for session in ("first", "second"):  # new export, then existing identical bytes
            with patch.dict(os.environ, SONIC_VS_BOOT_HANDOFF=session), patch.object(
                    boot_layout, "sha256", side_effect=AssertionError("reread exported archive")):
                build_helpers.export_image(source, self.destination, layout)
            selected = boot_layout.direct_layout_for_archive(self.destination, session)
            self.assertIsNotNone(selected)
            self.assertEqual((selected / "index.json").read_bytes(), (layout / "index.json").read_bytes())
        self.assertIsNone(boot_layout.direct_layout_for_archive(self.destination, "first"))

        # Even a same-size write with a restored mtime invalidates the binding.
        status = self.destination.stat()
        self.destination.write_bytes(b"bad archive")
        os.utime(self.destination, ns=(status.st_atime_ns, status.st_mtime_ns))
        self.assertIsNone(boot_layout.direct_layout_for_archive(self.destination, "second"))

    def test_replaced_export_cannot_publish_a_binding_for_the_new_owner(self):
        layout = self.root / "oci"
        write_layout(layout, oci_files(*image_fixture()))
        source = self.root / "source"
        source.write_bytes(b"first archive")
        published = build_helpers.export_archive(source, self.destination, bind_identity=True)
        other = self.root / "other"
        other.write_bytes(b"other archive")
        other.replace(self.destination)
        boot_layout.publish_layout(layout, source, self.destination, handoff="first", published=published)
        self.assertIsNone(boot_layout.direct_layout_for_archive(self.destination, "first"))

    def test_mismatched_or_pruned_layout_cannot_be_handed_off(self):
        layout = self.root / "oci"
        write_layout(layout, oci_files(*image_fixture()))
        with patch.dict(os.environ, SONIC_VS_BOOT_HANDOFF="build"):
            build_helpers.export_image(self.destination, self.destination, layout)
        generation = boot_layout.direct_layout_for_archive(self.destination, "build")
        (generation / "index.json").write_bytes(b"{}")
        self.assertIsNone(boot_layout.direct_layout_for_archive(self.destination, "build"))
        (generation / "index.json").unlink()
        self.assertIsNone(boot_layout.direct_layout_for_archive(self.destination, "build"))


if __name__ == "__main__":
    unittest.main()
