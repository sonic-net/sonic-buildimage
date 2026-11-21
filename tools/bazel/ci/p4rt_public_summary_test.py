#!/usr/bin/env python3
"""Check that P4RT exports retain test coverage without private XML/log data."""

import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import p4rt_public


class P4rtPublicTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.targets = ['@@sonic-pins+//p4rt_app:runtime_package_test', '@@sonic-pins+//p4rt_app/sonic:packetio_impl_test']
        self.revision = 'a' * 40
        self.results = {target: 'passed' for target in self.targets}
        self.receipt = {'status': 'passed', 'owner_revision': self.revision,
                        'targets': self.targets, 'results': self.results}
        self.write(self.directory / 'owner-test-results.json', self.receipt)
        self.xml = []
        for target, count in zip(self.targets, (1, 3)):
            name = target.rsplit(':', 1)[1]
            path = self.directory / 'tests' / name / 'test.xml'
            path.parent.mkdir(parents=True)
            cases = ''.join(f'<testcase name="case-{i}"/>' for i in range(count))
            path.write_text(f'<testsuites><testsuite tests="{count}" failures="0" errors="0">'
                            f'{cases}<system-out>/private/worker/token</system-out></testsuite></testsuites>')
            self.xml.append(path)

    def write(self, path, value):
        path.write_text(json.dumps(value))

    def test_complete_summary_counts_actual_cases_without_xml_contents(self):
        result = p4rt_public.owner_test_summary(self.directory)
        self.assertEqual((result['target_count'], result['testcase_count']), (2, 4))
        self.assertEqual(result['tests'][self.targets[1]]['tests'], 3)
        self.assertEqual(result['tests'][self.targets[0]]['xml_sha256'],
                         hashlib.sha256(self.xml[0].read_bytes()).hexdigest())
        self.assertNotIn('/private/', json.dumps(result))
        self.assertNotIn(str(self.directory), json.dumps(result))

    def test_failed_status_or_incomplete_results_are_rejected(self):
        for results in ({self.targets[0]: 'passed'},
                        {self.targets[0]: 'passed', self.targets[1]: 'failed'}):
            with self.subTest(results=results):
                self.write(self.directory / 'owner-test-results.json', {**self.receipt, 'results': results})
                with self.assertRaisesRegex(ValueError, 'passing status'):
                    p4rt_public.owner_test_summary(self.directory)

    def test_failed_skipped_or_incomplete_xml_cannot_produce_passed_summary(self):
        for xml in ('<testsuite tests="1"><testcase><failure/></testcase></testsuite>',
                    '<testsuite tests="1" skipped="1"><testcase><skipped/></testcase></testsuite>',
                    '<testsuite tests="2"><testcase/></testsuite>'):
            with self.subTest(xml=xml):
                self.xml[0].write_text(xml)
                with self.assertRaises(ValueError):
                    p4rt_public.owner_test_summary(self.directory)

    def test_external_xml_symlink_is_rejected(self):
        external = self.directory / 'unrelated.xml'
        self.xml[0].rename(external)
        self.xml[0].symlink_to(external)
        with self.assertRaisesRegex(ValueError, 'escaped its retained directory'):
            p4rt_public.owner_test_summary(self.directory)

    def test_export_includes_reviewed_owner_evidence_only(self):
        unrelated = ('private-environment.json', 'docker-sonic-p4rt.runtime.json', 'loaded-validation.json')
        for name in unrelated:
            self.write(self.directory / name, {'environment': {'TOKEN': 'private'}})
        public = p4rt_public.prepare(self.directory)
        for name in ('owner-tests.json', 'owner-test-summary.json'):
            self.assertTrue((public / name).is_file(), name)
        for name in unrelated:
            self.assertFalse((public / name).exists(), name)
        self.assertFalse(list(public.rglob('*.xml')))


if __name__ == '__main__':
    unittest.main()
