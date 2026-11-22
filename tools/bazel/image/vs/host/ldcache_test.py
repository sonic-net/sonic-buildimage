"""Validate the TAR-backed cache against a real locked AMD64 glibc ELF."""
import sys
import tarfile
import unittest

from ldcache import cache_entries, generate_cache_from_fs
from merge_layers import Tree, apply_layer
from tar_filesystem import TarFilesystem

PAYLOAD = sys.argv.pop(1)


class CacheTest(unittest.TestCase):
    def test_glibc_soname_remains_resolvable(self):
        tree = Tree()
        with tarfile.open(PAYLOAD, "r:*") as archive:
            apply_layer(tree, archive, "locked Debian libc6")
            fs = TarFilesystem(tree)
            entries = cache_entries(generate_cache_from_fs(fs, "amd64"))
            libc = [entry for entry in entries if entry["soname"] == "libc.so.6"]
            self.assertEqual(len(libc), 1)
            self.assertEqual(libc[0]["path"], "/usr/lib/x86_64-linux-gnu/libc.so.6")
            self.assertTrue(fs.read(libc[0]["path"]).startswith(b"\x7fELF"))
            self.assertEqual(libc[0]["flags"], 0x303)


if __name__ == "__main__":
    unittest.main()
