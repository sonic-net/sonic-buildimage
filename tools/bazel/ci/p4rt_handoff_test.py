"""Check that an image cannot silently consume a stale or unrelated owner TAR."""

import io
import json
from pathlib import Path
import tarfile
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from tools.bazel.ci import p4rt_handoff as handoff


class HandoffTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.sdk, self.output, self.destination = [self.root / name for name in ('sdk', 'output', 'repository')]
        (self.sdk / 'lib').mkdir(parents=True)
        self.output.mkdir()
        (self.sdk / 'lib/libswsscommon.so.0').write_bytes(b'actual selected Common library')
        for name in handoff.FILES:
            (self.output / name).write_bytes(('owner bytes: ' + name).encode())
        self.revision = 'a' * 40
        self.api = SimpleNamespace(verify_build=lambda *args: {'sdk_inventory': {'headers': 'verified by owner'}})
        self.owner_patch = patch.object(handoff, 'owner_api', return_value=self.api)
        self.owner_patch.start()
        self.addCleanup(self.owner_patch.stop)

    def archive(self, name, data):
        path = self.root / name
        with tarfile.open(path, 'w') as stream:
            entry = tarfile.TarInfo('usr/lib/x86_64-linux-gnu/libswsscommon.so.0.0.0')
            entry.size = len(data)
            entry.mode = 0o755
            stream.addfile(entry, io.BytesIO(data))
        return path

    def test_common_sdk_must_equal_both_owner_and_parent_bytes(self):
        data = (self.sdk / 'lib/libswsscommon.so.0').read_bytes()
        runtime, parent = self.archive('runtime.tar', data), self.archive('parent.tar', data)
        proof = handoff.same_common_library(self.sdk, runtime, parent)
        self.assertEqual(proof['sha256'], handoff.check.sha(self.sdk / 'lib/libswsscommon.so.0'))
        self.archive('parent.tar', b'another Common build')
        with self.assertRaisesRegex(ValueError, 'differs from the selected parent'):
            handoff.same_common_library(self.sdk, runtime, parent)

    def test_declared_repository_exports_exact_owner_files_without_module(self):
        receipt = handoff.stage(self.root, self.revision, self.sdk, self.output, self.destination, {})
        self.assertEqual(set(receipt['files']), set(handoff.FILES))
        self.assertFalse((self.destination / 'MODULE.bazel').exists())
        self.assertIn('handoff.json', (self.destination / 'BUILD.bazel').read_text())
        for name in handoff.FILES:
            self.assertEqual((self.destination / name).read_bytes(), (self.output / name).read_bytes())
        handoff.verify_stage(self.destination, self.root, self.revision, self.sdk, self.output)

    def test_tampered_payload_extra_file_and_link_are_rejected(self):
        handoff.stage(self.root, self.revision, self.sdk, self.output, self.destination, {})
        path = self.destination / handoff.FILES[0]
        original = path.read_bytes()
        path.write_bytes(b'wrong cached TAR')
        with self.assertRaisesRegex(ValueError, 'output changed'):
            handoff.verify_stage(self.destination, self.root, self.revision, self.sdk, self.output)
        path.write_bytes(original)
        extra = self.destination / 'ambient.tar'
        extra.write_bytes(b'undeclared')
        with self.assertRaisesRegex(ValueError, 'Unexpected files'):
            handoff.verify_stage(self.destination, self.root, self.revision, self.sdk, self.output)
        extra.unlink()
        path.unlink()
        path.symlink_to(self.output / handoff.FILES[0])
        with self.assertRaisesRegex(ValueError, 'output changed'):
            handoff.verify_stage(self.destination, self.root, self.revision, self.sdk, self.output)

    def test_modified_generated_build_interface_is_rejected(self):
        handoff.stage(self.root, self.revision, self.sdk, self.output, self.destination, {})
        (self.destination / 'BUILD.bazel').write_text('genrule(name="ambient", cmd="false")')
        with self.assertRaisesRegex(ValueError, 'Generated owner interface changed'):
            handoff.verify_stage(self.destination, self.root, self.revision, self.sdk, self.output)

    def test_existing_unowned_directory_is_preserved(self):
        self.destination.mkdir()
        marker = self.destination / 'valuable-file'
        marker.write_text('keep')
        with self.assertRaisesRegex(ValueError, 'not generated'):
            handoff.stage(self.root, self.revision, self.sdk, self.output, self.destination, {})
        self.assertEqual(marker.read_text(), 'keep')


if __name__ == '__main__':
    unittest.main()
