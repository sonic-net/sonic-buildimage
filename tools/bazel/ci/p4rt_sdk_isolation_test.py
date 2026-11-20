"""Keep parallel SDK validations from overwriting one another's raw evidence."""

import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from tools.bazel.ci import p4rt_common_sdk as validation


class EvidenceIsolationTest(unittest.TestCase):
    def test_sibling_output_bases_use_separate_action_records(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            sdk = root / 'sdk'
            sdk.mkdir()
            (sdk / 'sdk-build.json').write_text('{}\n')
            records = []

            def run(command, **kwargs):
                phase = command[2]
                if phase == 'version':
                    return subprocess.CompletedProcess(command, 0, 'bazel 6.4.0\n')
                if phase == 'query':
                    return subprocess.CompletedProcess(command, 0, '//p4rt_app:example_test\n')
                if phase == 'aquery':
                    records.append(Path(kwargs['stdout'].name))
                    json.dump({}, kwargs['stdout'])
                    return subprocess.CompletedProcess(command, 0)
                if phase == 'build':
                    raise RuntimeError('stop before compilation')
                if phase == 'shutdown':
                    return subprocess.CompletedProcess(command, 0)
                raise AssertionError('Unexpected external command: ' + phase)

            for name in ('first-output', 'second-output'):
                argv = ['sdk-check', '--source', str(root), '--sdk', str(sdk),
                        '--output-base', str(root / name), '--output', str(root / (name + '-public')),
                        '--arch', 'amd64']
                with (patch('sys.argv', argv),
                      patch.object(validation.subprocess, 'run', side_effect=run),
                      patch.object(validation, 'audit_graph', return_value={'deb_outputs': 0})):
                    with self.assertRaisesRegex(RuntimeError, 'stop before compilation'):
                        validation.main()

            self.assertEqual(len(records), 2)
            self.assertNotEqual(records[0].parent, records[1].parent)
            # Both validators retain a separate private evidence directory.
            for record in records:
                self.assertTrue(record.parent.is_dir())
                self.assertTrue(record.parent.is_relative_to(root))


if __name__ == '__main__':
    unittest.main()
