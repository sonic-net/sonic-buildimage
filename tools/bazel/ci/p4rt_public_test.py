"""Exercise the public/private boundary for actual P4RT evidence shapes."""

import json
from pathlib import Path
import tempfile
import unittest

from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import p4rt_public


class PublicEvidenceTest(unittest.TestCase):
    def test_selects_only_hashed_artifact_and_safe_receipt_fields(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            artifact = root / 'p4rt.source.tar'
            artifact.write_bytes(b'producer-owned archive bytes')
            (root / 'build-receipt.json').write_text(json.dumps({
                'status': 'built', 'revision': 'a' * 40, 'owner_revision': 'b' * 40,
                'commands': [{'argv': ['bazel', '--output_base=/home/builder/.cache/private']}],
                'artifacts': {artifact.name: {'target': '//dockers/docker-sonic-p4rt:source_runtime',
                                            'sha256': check.sha(artifact), 'bytes': artifact.stat().st_size}},
            }))
            (root / 'prepared-owner-runtime.json').write_text('{"library_path":["/home/builder/.cache/private"]}')
            public = p4rt_public.prepare(root)
            self.assertEqual({path.name for path in public.iterdir()},
                             {'p4rt.source.tar', 'public-receipt.json', 'build-summary.json'})
            self.assertEqual(check.sha(public / artifact.name), check.sha(artifact))
            self.assertNotIn('commands', json.loads((public / 'public-receipt.json').read_text()))

    def test_rejects_changed_output(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'p4rt.source.tar').write_bytes(b'changed bytes')
            (root / 'build-receipt.json').write_text(json.dumps({
                'status': 'built', 'artifacts': {'p4rt.source.tar': {'sha256': '0' * 64}},
            }))
            with self.assertRaisesRegex(ValueError, 'artifact changed'):
                p4rt_public.prepare(root)

    def test_rejects_private_paths_in_public_validation(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'validation.json').write_text('{"path":"/home/builder/.cache/private"}')
            with self.assertRaisesRegex(ValueError, 'local-path'):
                p4rt_public.prepare(root)

    def test_does_not_publish_stale_previous_evidence(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'public').mkdir()
            with self.assertRaises(FileExistsError):
                p4rt_public.prepare(root)


if __name__ == '__main__':
    unittest.main()
