"""Shared mirror setup preserves source integrity and existing download policy."""

import hashlib
import io
from pathlib import Path
import re
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import source_mirrors


class SourceMirrorsTest(unittest.TestCase):
    def test_changed_shared_policy_does_not_replace_existing_configuration(self):
        """An unexpected policy cannot redirect downloads, even with an old output."""
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / 'mirrors.cfg'
            output.write_bytes(b'previous verified policy\n')
            with patch.object(source_mirrors, 'urlopen', return_value=io.BytesIO(b'changed policy')), \
                    self.assertRaisesRegex(ValueError, 'checksum mismatch'):
                source_mirrors.prepare(output)
            self.assertEqual(output.read_bytes(), b'previous verified policy\n')

    def test_verified_shared_policy_keeps_local_debian_priority(self):
        """Hosted jobs retain local mirrors and fallbacks ahead of shared choices."""
        shared = b'# verified registry policy\nrewrite example/source https://mirror.example/source\n'
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            local = root / 'debian.cfg'
            local.write_bytes(b'rewrite snapshot.example/(.*) https://sonic.example/$1\n'
                              b'rewrite snapshot.example/(.*) https://snapshot.example/$1\n')
            output = root / 'generated' / 'mirrors.cfg'
            with patch.object(source_mirrors, 'SHA256', hashlib.sha256(shared).hexdigest()), \
                    patch.object(source_mirrors, 'urlopen', return_value=io.BytesIO(shared)) as fetch:
                source_mirrors.prepare(output, local)
            fetch.assert_called_once_with(source_mirrors.URL, timeout=60)
            self.assertEqual(output.read_bytes(), local.read_bytes() + b'\n' + shared)

    def test_native_policy_adds_only_the_original_timestamped_snapshot_first(self):
        shared = (Path(__file__).parent / 'fixtures/source-mirrors-9697f3db.cfg').read_bytes()
        with tempfile.TemporaryDirectory() as temporary, \
                patch.object(source_mirrors, 'urlopen', side_effect=lambda *_a, **_k: io.BytesIO(shared)):
            output = Path(temporary) / 'mirrors.cfg'
            source_mirrors.prepare(output)
            default = output.read_bytes()
            source_mirrors.prepare(output, prefer_debian_snapshot=True)
            rule, fallbacks = output.read_bytes().split(b'\n', 1)
            self.assertEqual(fallbacks, default)
            operation, pattern, replacement = rule.decode().split()
            self.assertEqual(operation, 'rewrite')
            for archive in ('debian', 'debian-security'):
                url = f'snapshot.debian.org/archive/{archive}/20260726T121236Z/pool/locked.deb'
                match = re.fullmatch(pattern, url)
                self.assertIsNotNone(match)
                self.assertEqual(replacement.replace('$1', match[1]), 'https://' + url)
            self.assertIsNone(re.fullmatch(pattern, 'snapshot.debian.org/file/abc'))


if __name__ == '__main__':
    unittest.main()
