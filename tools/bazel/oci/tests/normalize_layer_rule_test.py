"""Check the actual normalizer action and its public output/receipt interface."""
import hashlib
import json
from pathlib import Path
import sys
import tarfile
import unittest

OUTPUT, RECEIPT = map(Path, sys.argv[1:])
del sys.argv[1:]


class NormalizeLayerRuleTest(unittest.TestCase):
    def test_normalized_tar_and_bound_receipt(self):
        record = json.loads(RECEIPT.read_text())
        self.assertEqual(record['output_sha256'], hashlib.sha256(OUTPUT.read_bytes()).hexdigest())
        self.assertEqual(record['rewritten_members'], 1)
        with tarfile.open(OUTPUT) as archive:
            entry = archive.getmember('./usr/lib/example')
            self.assertEqual((entry.uid, entry.gid, entry.mode), (101, 102, 0o640))
            self.assertEqual(archive.extractfile(entry).read(), b'value')


if __name__ == '__main__':
    unittest.main()
