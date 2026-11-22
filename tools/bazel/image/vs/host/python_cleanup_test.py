"""Check pip uninstall's installed RECORD contract with a real enum34 wheel."""
from pathlib import Path
import sys
import tarfile
import tempfile
import unittest

from merge_layers import Tree, apply_layer
from python_cleanup import uninstall

PAYLOAD = sys.argv.pop(1)


class CleanupTest(unittest.TestCase):
    def test_removes_wheel_owned_modules_and_metadata_only(self):
        with tempfile.TemporaryDirectory() as directory:
            output, audit = [str(Path(directory) / name) for name in ("cleanup.tar", "audit.json")]
            uninstall({"layers": [PAYLOAD], "distributions": ["enum34"], "cache_tag": "cpython-313"}, output, audit)
            tree = Tree()
            with tarfile.open(PAYLOAD) as original, tarfile.open(output) as cleanup:
                names = [member.name for member in original if member.isfile()]
                self.assertTrue(any(name.endswith("/enum/__init__.py") for name in names))
                apply_layer(tree, original, "actual installed enum34")
                apply_layer(tree, cleanup, "original Make pip uninstall enum34")
                self.assertIsNotNone(tree.lookup(["usr", "local", "lib", "python3.13", "dist-packages"]))
                remaining = tree.resolve_dir(["usr", "local", "lib", "python3.13", "dist-packages"], False)
                self.assertEqual(list(remaining.children), [])
            self.assertIn('"enum34"', Path(audit).read_text())


if __name__ == "__main__":
    unittest.main()
