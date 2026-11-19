#!/usr/bin/env python3
"""Check that P4RT exports retain test coverage without private XML/log data."""

import hashlib
import copy
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
        self.owner = self.directory / 'owner-output'
        self.owner.mkdir()
        self.targets = ['//p4rt_app:runtime_package_test', '//p4rt_app/sonic:packetio_impl_test']
        self.revision = 'a' * 40
        self.source = {'commit': self.revision, 'bazel': '6.4.0', 'architecture': 'amd64',
                       'test_targets': self.targets, 'sdk_inventory': {},
                       'tar_sha256': {}, 'contract_sha256': {}}
        self.sdk_fixture()
        self.write(self.owner / 'source.json', self.source)
        self.write(self.directory / 'owner-tests.json', {'status': 'passed', 'owner_revision': self.revision})
        self.write(self.owner / 'test-results.json', dict.fromkeys(self.targets, 'PASSED'))
        (self.owner / 'test-targets.txt').write_text('\n'.join(self.targets) + '\n')
        self.xml = []
        for target, count in zip(self.targets, (1, 3)):
            path = self.owner / 'test-results' / target.removeprefix('//').replace(':', '/') / 'test.xml'
            path.parent.mkdir(parents=True)
            cases = ''.join(f'<testcase name="case-{i}"/>' for i in range(count))
            path.write_text(f'<testsuites><testsuite tests="{count}" failures="0" errors="0">'
                            f'{cases}<system-out>/private/worker/token</system-out></testsuite></testsuites>')
            self.xml.append(path)

    def write(self, path, value):
        path.write_text(json.dumps(value))

    def sdk_fixture(self):
        """Shape the JSON emitted by p4rt_image, prepare_repository and the owner."""
        targets = {
            'common-sdk.tar': '@sonic_swss_common//dist:libswsscommon_sdk',
            'libnl-nf-sdk.tar': '@libnl3//:libnl-nf-3-dev_pkg',
            'libnl-genl-sdk.tar': '@libnl3//:libnl-genl-3-dev_pkg',
            'libyang-runtime.tar': '@libyang//:libyang_pkg',
            'common-runtime.tar': '@sonic_swss_common//dist:libswsscommon_pkg',
            'common-runtime-runner.py': '//dockers/docker-sonic-p4rt:owner_runtime_runner',
            'common.MODULE.bazel': '//dockers/docker-sonic-p4rt:owner_common_module',
            'owner-test-runtime.tar': '//dockers/docker-sonic-p4rt:owner_test_runtime',
            'runtime-patchelf': '//dockers/docker-sonic-p4rt:owner_patchelf',
            'config-engine.source.tar': '//dockers/docker-config-engine-trixie:source_runtime',
        }
        artifacts = {name: {'target': target, 'bytes': 1024, 'sha256': hashlib.sha256(name.encode()).hexdigest()}
                     for name, target in targets.items()}
        parent_sha = artifacts['config-engine.source.tar']['sha256']
        self.audit = {'schema': 1, 'action_count': 400, 'output_count': 600,
                      'deb_outputs': [], 'packaging_wrappers': [], 'targets': list(targets.values())}
        self.sdk = {'kind': 'sonic-swss-common-bazel-sdk', 'architecture': 'amd64',
                    'producer_bazel_version': '8.5.1', 'producer_target': targets['common-sdk.tar'],
                    'archive_sha256': artifacts['common-sdk.tar']['sha256'],
                    'common_module_sha256': artifacts['common.MODULE.bazel']['sha256'],
                    'common_source_commit': 'b' * 40, 'image_source_revision': 'c' * 40,
                    'yang_modules': True, 'common_source_has_tracked_changes': False}
        inventory = {'lib/libswsscommon.so.0': {'kind': 'file', 'sha256': 'd' * 64, 'mode': 493}}
        for component, names, stems, suffix in (
            ('libnl', ('libnl-nf-sdk.tar', 'libnl-genl-sdk.tar'),
             ('libnl-genl-3', 'libnl-nf-3', 'libnl-route-3', 'libnl-3'), '.so.200'),
            ('libyang', ('libyang-runtime.tar',), ('libyang',), '.so.3'),
        ):
            files = {}
            for stem in stems:
                soname = stem + suffix
                library = soname + ('.26.0' if component == 'libnl' else '.9.1')
                sha = hashlib.sha256(library.encode()).hexdigest()
                files['usr/lib/x86_64-linux-gnu/' + library] = {'kind': 'file', 'elf_machine': 62, 'sha256': sha, 'mode': 493}
                files['usr/lib/x86_64-linux-gnu/' + soname] = {'kind': 'symlink', 'linkname': library, 'mode': 511}
                inventory['lib/' + library] = {'kind': 'file', 'sha256': sha, 'mode': 493}
                inventory['lib/' + soname] = {'kind': 'symlink', 'target': library, 'mode': 511}
            if component == 'libnl':
                files['usr/include/libnl3/netlink/netlink.h'] = {'kind': 'file', 'sha256': 'e' * 64}
                inventory['include/libnl3/netlink/netlink.h'] = {'kind': 'file', 'sha256': 'e' * 64, 'mode': 420}
            self.sdk[component] = {'producer_targets': {targets[name]: artifacts[name]['sha256'] for name in names},
                                   'parent_source_tar_sha256': parent_sha, 'files': files}
        self.source['sdk_inventory'] = inventory
        parent = {'path': 'usr/lib/x86_64-linux-gnu/libswsscommon.so.0.0.0', 'sha256': 'd' * 64,
                  'common_runtime_tar_sha256': artifacts['common-runtime.tar']['sha256'],
                  'parent_source_tar_sha256': parent_sha}
        self.prepared = {'schema': 1, 'status': 'prepared', 'revision': 'c' * 40, 'architecture': 'amd64',
                         'owner_revision': self.revision, 'artifacts': artifacts, 'action_audit': self.audit,
                         'parent_common': parent, 'owner_handoff': {'source_revision': self.revision,
                         'sdk_inventory': inventory, 'parent_common': parent},
                         'commands': [{'argv': ['tool', '/home/worker/code/private'], 'environment': {'TOKEN': 'private'}}]}
        for name in ('owner-sdk', 'owner-preparation'):
            (self.directory / name).mkdir(exist_ok=True)
        self.save_sdk_fixture()

    def save_sdk_fixture(self):
        path = self.directory / 'owner-sdk/sdk-build.json'
        self.write(path, self.sdk)
        self.source['sdk_inventory']['sdk-build.json'] = {
            'kind': 'file', 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'mode': 420}
        self.write(self.directory / 'owner-preparation.json', self.prepared)
        self.write(self.directory / 'owner-preparation/action-audit.json', self.audit)
        self.write(self.owner / 'source.json', self.source)

    def test_sdk_summary_binds_audit_parent_and_inventory_without_private_fields(self):
        self.sdk['environment'] = {'TOKEN': 'private'}
        self.save_sdk_fixture()
        before = {p: p.read_bytes() for p in self.directory.rglob('*.json')}
        result = p4rt_public.sdk_preparation_summary(self.directory, self.source)
        self.assertEqual(len(result['action_audit']['targets']), 10)
        self.assertEqual(result['sdk_build']['libyang']['parent_source_tar_sha256'],
                         result['artifacts']['config-engine.source.tar']['sha256'])
        self.assertEqual(result['original_sha256']['owner-sdk/sdk-build.json'],
                         self.source['sdk_inventory']['sdk-build.json']['sha256'])
        self.assertNotIn('private', json.dumps(result))
        self.assertNotIn('environment', json.dumps(result))
        self.assertEqual(before, {p: p.read_bytes() for p in before})

    def test_sdk_linker_symlink_must_be_declared_and_match_inventory(self):
        name = 'usr/lib/x86_64-linux-gnu/libnl-nf-3.so'
        self.sdk['libnl']['files'][name] = {'kind': 'symlink', 'linkname': 'libnl-nf-3.so.200'}
        self.source['sdk_inventory']['lib/libnl-nf-3.so'] = {'kind': 'symlink', 'target': 'libnl-nf-3.so.200', 'mode': 511}
        self.save_sdk_fixture()
        result = p4rt_public.sdk_preparation_summary(self.directory, self.source)
        self.assertEqual(result['sdk_build']['libnl']['files'][name]['linkname'], 'libnl-nf-3.so.200')
        for target in ('../private', 'libnl-route-3.so.200'):
            with self.subTest(target=target):
                self.sdk['libnl']['files'][name]['linkname'] = target
                self.source['sdk_inventory']['lib/libnl-nf-3.so']['target'] = target
                self.save_sdk_fixture()
                with self.assertRaisesRegex(ValueError, 'SONAME linkage'):
                    p4rt_public.sdk_preparation_summary(self.directory, self.source)

    def test_sdk_changed_linkages_are_rejected(self):
        cases = ('parent', 'archive', 'module', 'yang', 'library', 'soname', 'owner', 'metadata')
        for case in cases:
            with self.subTest(case=case):
                self.sdk_fixture()
                if case == 'parent': self.sdk['libyang']['parent_source_tar_sha256'] = 'f' * 64
                if case == 'archive': self.sdk['libyang']['producer_targets']['@libyang//:libyang_pkg'] = 'f' * 64
                if case == 'module': self.sdk['common_module_sha256'] = 'f' * 64
                if case == 'yang': self.sdk['yang_modules'] = False
                if case == 'library': self.sdk['libyang']['files']['usr/lib/x86_64-linux-gnu/libyang.so.3.9.1']['sha256'] = 'f' * 64
                if case == 'soname': self.sdk['libyang']['files']['usr/lib/x86_64-linux-gnu/libyang.so.3']['linkname'] = '../libyang.so.3.9.1'
                if case == 'owner': self.prepared['owner_revision'] = 'f' * 40
                self.save_sdk_fixture()
                if case == 'metadata': self.source['sdk_inventory']['sdk-build.json']['sha256'] = 'f' * 64
                with self.assertRaises(ValueError):
                    p4rt_public.sdk_preparation_summary(self.directory, self.source)

    def test_sdk_missing_or_changed_audit_is_rejected(self):
        for case in ('missing', 'extra', 'duplicate', 'deb', 'wrapper', 'count', 'different'):
            with self.subTest(case=case):
                self.sdk_fixture()
                audit = copy.deepcopy(self.audit)
                if case == 'extra': audit['targets'].append('//unexpected:target')
                if case == 'duplicate': audit['targets'][-1] = audit['targets'][0]
                if case == 'deb': audit['deb_outputs'] = ['fixture.deb']
                if case == 'wrapper': audit['packaging_wrappers'] = [{'mnemonic': 'DebBuild'}]
                if case == 'count': audit['action_count'] = True
                if case != 'different': self.prepared['action_audit'] = audit
                self.write(self.directory / 'owner-preparation.json', self.prepared)
                if case == 'different': audit['action_count'] += 1
                path = self.directory / 'owner-preparation/action-audit.json'
                self.write(path, audit)
                if case == 'missing': path.unlink()
                with self.assertRaises(ValueError):
                    p4rt_public.sdk_preparation_summary(self.directory, self.source)

    def test_sdk_unsafe_or_escaping_input_is_rejected(self):
        for case in ('path', 'hash', 'revision', 'symlink'):
            with self.subTest(case=case):
                self.sdk_fixture()
                if case == 'path': self.sdk['libnl']['files']['usr/include/libnl3/../../../private'] = {'kind': 'file', 'sha256': 'e' * 64}
                if case == 'hash': self.sdk['libyang']['files']['usr/lib/x86_64-linux-gnu/libyang.so.3.9.1']['sha256'] = 'ghp_' + 'x' * 40
                if case == 'revision': self.sdk['common_source_commit'] = '/home/worker/code/private'
                self.save_sdk_fixture()
                if case == 'symlink':
                    path = self.directory / 'owner-sdk/sdk-build.json'
                    path.rename(self.directory / 'external.json')
                    path.symlink_to(self.directory / 'external.json')
                with self.assertRaises(ValueError):
                    p4rt_public.sdk_preparation_summary(self.directory, self.source)
    def test_complete_summary_counts_actual_cases_without_xml_contents(self):
        result = p4rt_public.owner_test_summary(self.directory)
        self.assertEqual((result['target_count'], result['testcase_count']), (2, 4))
        self.assertEqual(result['tests'][self.targets[1]]['tests'], 3)
        self.assertEqual(result['tests'][self.targets[0]]['xml_sha256'],
                         hashlib.sha256(self.xml[0].read_bytes()).hexdigest())
        self.assertNotIn('/private/', json.dumps(result))
        self.assertNotIn(str(self.directory), json.dumps(result))

    def test_owner_revision_must_match(self):
        self.write(self.directory / 'owner-tests.json', {'status': 'passed', 'owner_revision': 'b' * 40})
        with self.assertRaisesRegex(ValueError, 'differs from the built source'):
            p4rt_public.owner_test_summary(self.directory)

    def test_bep_statuses_must_cover_exact_selection_and_pass(self):
        for statuses in ({self.targets[0]: 'PASSED'},
                         {**dict.fromkeys(self.targets, 'PASSED'), '//p4rt_app:extra': 'PASSED'},
                         {self.targets[0]: 'PASSED', self.targets[1]: 'FAILED'}):
            with self.subTest(statuses=statuses):
                self.write(self.owner / 'test-results.json', statuses)
                with self.assertRaisesRegex(ValueError, 'passing BEP status'):
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

    def test_export_includes_reviewed_owner_and_sdk_evidence_only(self):
        unrelated = ('private-environment.json', 'docker-sonic-p4rt.runtime.json', 'loaded-validation.json')
        for name in unrelated:
            self.write(self.directory / name, {'environment': {'TOKEN': 'private'}})
        public = p4rt_public.prepare(self.directory)
        for name in ('owner-test-summary.json', 'sdk-preparation.json'):
            self.assertTrue((public / name).is_file(), name)
        for name in unrelated:
            self.assertFalse((public / name).exists(), name)
        self.assertFalse(list(public.rglob('*.xml')))


if __name__ == '__main__':
    unittest.main()
