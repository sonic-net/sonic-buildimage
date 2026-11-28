"""Match source-compiled split symbols against a real rules_oci image.

The unit cases cover inherited DWZ, corrupt metadata and missing coverage. This
case proves that the public rule also accepts the configured native toolchain's
ELF sections and GNU debuglink checksum, with repeatable selected outputs.
"""

import json
from pathlib import Path
import sys
import tempfile
import unittest

from tools.bazel.oci.imported_symbols import build

RUNTIME, DEBUG, SELECTED, RECEIPT = map(Path, sys.argv[1:5])
ARCHITECTURE = sys.argv[5]
del sys.argv[1:]


class NativeSymbolsTest(unittest.TestCase):
    def test_compiled_elf_matches_runtime_image_and_selected_receipt(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            receipt = build(RUNTIME, [DEBUG], "linux/" + ARCHITECTURE, ["usr/bin/hello"],
                            root / "symbols.tar", root / "symbols.json")
            self.assertEqual(len(receipt["pairs"]), 1)
            self.assertEqual(receipt["excluded_paths"], [])
            self.assertEqual(receipt, json.loads(RECEIPT.read_text()))
            self.assertEqual((root / "symbols.tar").read_bytes(), SELECTED.read_bytes())


if __name__ == "__main__":
    unittest.main()
