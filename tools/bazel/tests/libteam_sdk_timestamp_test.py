#!/usr/bin/env python3
"""Keep generated file timestamps out of the installed libteam SDK."""

import importlib.util
import io
from pathlib import Path
import tarfile
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location(
    "libteam_verify_runtime", ROOT / "src/libteam/verify_runtime.py")
VERIFY = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(VERIFY)


class DevelopmentTimestampTest(unittest.TestCase):
    def archive(self, directory, *, mtime=0, pax_mtime=None, empty=False):
        path = Path(directory) / "sdk.tar"
        with tarfile.open(path, "w", format=tarfile.PAX_FORMAT) as archive:
            if not empty:
                entry = tarfile.TarInfo("usr/lib/x86_64-linux-gnu/pkgconfig/libteam.pc")
                contents = b"prefix=/usr\n"
                entry.size = len(contents)
                entry.mtime = mtime
                if pax_mtime is not None:
                    entry.pax_headers = {"mtime": pax_mtime}
                archive.addfile(entry, io.BytesIO(contents))
                link = tarfile.TarInfo("usr/lib/x86_64-linux-gnu/libteam.so")
                link.type = tarfile.SYMTYPE
                link.linkname = "libteam.so.5.6.1"
                archive.addfile(link)
        return path

    def test_zero_timestamp_files_and_links(self):
        with tempfile.TemporaryDirectory() as directory:
            self.assertEqual(VERIFY.verify_development_timestamps(self.archive(directory)), 2)

    def test_rejects_build_time_and_pax_override(self):
        for options in ({"mtime": 1793813010}, {"mtime": -1}, {"pax_mtime": "1793813010.5"}):
            with self.subTest(options=options), tempfile.TemporaryDirectory() as directory:
                with self.assertRaisesRegex(ValueError, "non-reproducible development timestamp: .*libteam.pc"):
                    VERIFY.verify_development_timestamps(self.archive(directory, **options))

    def test_rejects_empty_archive(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, "empty development archive"):
                VERIFY.verify_development_timestamps(self.archive(directory, empty=True))


if __name__ == "__main__":
    unittest.main()
