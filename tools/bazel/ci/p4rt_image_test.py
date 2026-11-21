"""Keep private test-runtime materialization faithful to the declared TARs."""

from contextlib import redirect_stderr
from pathlib import Path
import io
import json
import struct
import tarfile
import tempfile
import os
import subprocess
import sys
from unittest.mock import patch
from types import SimpleNamespace
import unittest

from tools.bazel.ci import p4rt_image as image
from tools.bazel import p4rt_common_sdk as sdk


class SdkTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.elf = b'\x7fELF\x02\x01' + bytes(12) + struct.pack('<H', 62)
        self.common = self.archive('common.tar', {
            'lib/libswsscommon.so.0': self.elf,
            'include/swss/dbconnector.h': b'// database',
            'include/swss/cfg_schema.h': b'// schema',
        })
        self.members = {'usr/include/libnl3/netlink/netlink.h': b'// source-owned header'}
        for stem in sdk.LIBNL_STEMS:
            name = 'usr/lib/x86_64-linux-gnu/' + stem
            self.members[name + '.so.200.26.0'] = self.elf + stem.encode()
            self.members[name + '.so.200'] = stem + '.so.200.26.0'
        self.parent = self.archive('parent.tar', self.members)
        self.yang_members = {
            'usr/lib/x86_64-linux-gnu/libyang.so.3.9.1': self.elf + b'libyang',
            'usr/lib/x86_64-linux-gnu/libyang.so.3': 'libyang.so.3.9.1',
            'usr/lib/x86_64-linux-gnu/libyang.so': 'libyang.so.3',
        }
        self.yang = self.archive('libyang.tar', self.yang_members)
        self.archive('parent.tar', {**self.members, **self.yang_members})
        # Runtime parents need not install the development linker aliases.
        for stem in sdk.LIBNL_STEMS:
            self.members['usr/lib/x86_64-linux-gnu/' + stem + '.so'] = stem + '.so.200'
        self.inputs = {'@libnl3//:libnl-nf-3-dev_pkg': self.archive('nf.tar', self.members),
                       '@libnl3//:libnl-genl-3-dev_pkg': self.archive('genl.tar', self.members)}
        self.output = self.root / 'sdk'

    def archive(self, name, members):
        path = self.root / name
        with tarfile.open(path, 'w') as archive:
            for name, contents in members.items():
                member = tarfile.TarInfo(name)
                member.mode = 0o644
                if isinstance(contents, str):
                    member.type, member.linkname = tarfile.SYMTYPE, contents
                    archive.addfile(member)
                else:
                    member.size = len(contents)
                    archive.addfile(member, io.BytesIO(contents))
        return path

    def prepare(self):
        sdk.prepare_repository(self.common, self.output, 'amd64', {},
                               libnl_archives=self.inputs, libyang_archive=self.yang,
                               parent_archive=self.parent)

    def test_declared_libraries_headers_links_and_parent_identity(self):
        self.prepare()
        self.assertIn('_DECLARED_LIBNL = True', (self.output / 'BUILD.bazel').read_text())
        for stem in sdk.LIBNL_STEMS:
            library = self.output / ('lib/' + stem + '.so.200')
            self.assertTrue(library.is_symlink())
            self.assertEqual(library.read_bytes(), self.elf + stem.encode())
            development = self.output / ('lib/' + stem + '.so')
            self.assertEqual(development.readlink().as_posix(), stem + '.so.200')
            self.assertEqual(development.read_bytes(), library.read_bytes())
            self.assertNotIn('usr/lib/x86_64-linux-gnu/' + stem + '.so', image.check.payload(self.parent))
            self.assertEqual(self.template_targets()[stem]['shared_library'], 'lib/' + stem + '.so')
        self.assertEqual((self.output / 'include/libnl3/netlink/netlink.h').read_bytes(),
                         self.members['usr/include/libnl3/netlink/netlink.h'])
        receipt = json.loads((self.output / 'sdk-build.json').read_text())['libnl']
        self.assertEqual(receipt['parent_source_tar_sha256'], image.check.sha(self.parent))
        self.assertEqual(receipt['producer_targets'], {key: image.check.sha(path) for key, path in self.inputs.items()})
        for stem in sdk.LIBNL_STEMS:
            self.assertEqual(receipt['files']['usr/lib/x86_64-linux-gnu/' + stem + '.so']['linkname'],
                             stem + '.so.200')

    def test_development_alias_absence_and_wrong_kind_are_rejected(self):
        self.prepare()
        before = (self.output / 'sdk-build.json').read_bytes()
        name = 'usr/lib/x86_64-linux-gnu/libnl-nf-3.so'
        for value in (None, self.elf + b'not an alias'):
            with self.subTest(value=value):
                changed = dict(self.members)
                if value is None:
                    del changed[name]
                else:
                    changed[name] = value
                for path in self.inputs.values():
                    self.archive(path.name, changed)
                with self.assertRaisesRegex(ValueError, 'Incomplete libnl SDK|Unexpected libnl SDK library kind'):
                    self.prepare()
                self.assertEqual((self.output / 'sdk-build.json').read_bytes(), before)

    def test_development_alias_rejects_escaping_wrong_and_bypassed_soname(self):
        self.prepare()
        before = (self.output / 'sdk-build.json').read_bytes()
        for target in ('/tmp/ambient.so', '../libnl-nf-3.so.200', 'libnl-3.so.200',
                       'libnl-nf-3.so.200.26.0', 'libnl-nf-3.so'):
            with self.subTest(target=target):
                changed = dict(self.members, **{'usr/lib/x86_64-linux-gnu/libnl-nf-3.so': target})
                for path in self.inputs.values():
                    self.archive(path.name, changed)
                with self.assertRaisesRegex(ValueError, 'Unexpected libnl development link'):
                    self.prepare()
                self.assertEqual((self.output / 'sdk-build.json').read_bytes(), before)

    def test_legacy_sdk_keeps_original_dependency_contract(self):
        sdk.prepare_repository(self.common, self.output, 'amd64', {})
        self.assertIn('_DECLARED_LIBNL = False', (self.output / 'BUILD.bazel').read_text())
        self.assertNotIn('libnl', json.loads((self.output / 'sdk-build.json').read_text()))
        self.assertIn('_DECLARED_LIBYANG = False', (self.output / 'BUILD.bazel').read_text())
        self.assertNotIn('libyang', json.loads((self.output / 'sdk-build.json').read_text()))
        self.assertFalse((self.output / 'lib/libyang.so.3').exists())
        targets = self.template_targets()
        self.assertIn('-lyang', targets['common']['linkopts'])
        self.assertNotIn('libyang', targets)

    def template_targets(self):
        targets = {}
        def target(**arguments):
            targets[arguments['name']] = arguments
        namespace = {'package': lambda **unused: None, 'cc_import': target,
                     'cc_library': target, 'alias': target, 'glob': lambda unused: []}
        exec(compile((self.output / 'BUILD.bazel').read_text(), 'SDK BUILD.bazel', 'exec'), namespace)
        return targets

    def test_libyang_soname_and_exact_parent_bytes_are_declared(self):
        self.prepare()
        library = self.output / 'lib/libyang.so.3'
        self.assertEqual(library.readlink().as_posix(), 'libyang.so.3.9.1')
        self.assertEqual(library.read_bytes(), self.elf + b'libyang')
        self.assertFalse((self.output / 'lib/libyang.so').exists())
        receipt = json.loads((self.output / 'sdk-build.json').read_text())['libyang']
        self.assertEqual(receipt['parent_source_tar_sha256'], image.check.sha(self.parent))
        self.assertEqual(receipt['producer_targets'], {'@libyang//:libyang_pkg': image.check.sha(self.yang)})
        self.assertEqual(set(receipt['files']), set(self.yang_members) - {'usr/lib/x86_64-linux-gnu/libyang.so'})
        targets = self.template_targets()
        self.assertEqual(targets['libyang']['shared_library'], 'lib/libyang.so.3')
        self.assertIn(':libyang', targets['swsscommon_library']['deps'])
        self.assertNotIn('-lyang', targets['common']['linkopts'])

    def test_libyang_rejects_missing_wrong_architecture_and_soname_inputs(self):
        library = 'usr/lib/x86_64-linux-gnu/libyang.so.3.9.1'
        soname = 'usr/lib/x86_64-linux-gnu/libyang.so.3'
        wrong_arch = self.elf[:18] + struct.pack('<H', 183) + b'libyang'
        for changed in ({}, {**self.yang_members, library: wrong_arch},
                        {**self.yang_members, library: 'other.so'},
                        {**self.yang_members, soname: '/tmp/ambient.so'},
                        {**self.yang_members, soname: '../libyang.so.3.9.1'},
                        {**self.yang_members, soname: self.elf}):
            with self.subTest(changed=changed):
                self.archive('libyang.tar', changed)
                with self.assertRaisesRegex(ValueError, 'libyang SDK'):
                    self.prepare()
                self.assertFalse(self.output.exists())

    def test_libyang_parent_drift_preserves_previous_sdk(self):
        self.prepare()
        before = (self.output / 'sdk-build.json').read_bytes()
        library = 'usr/lib/x86_64-linux-gnu/libyang.so.3.9.1'
        soname = 'usr/lib/x86_64-linux-gnu/libyang.so.3'
        for changed in ({**self.yang_members, library: self.elf + b'other'},
                        {**self.yang_members, soname: 'other.so'}, {}):
            with self.subTest(changed=changed):
                self.archive('parent.tar', {**self.members, **changed})
                with self.assertRaisesRegex(ValueError, 'libyang SDK'):
                    self.prepare()
                self.assertEqual((self.output / 'sdk-build.json').read_bytes(), before)

    def test_libyang_flattened_paths_and_links_are_normalized(self):
        self.archive('libyang.tar', {'./' + name: './' + value if isinstance(value, str) else value
                                    for name, value in self.yang_members.items()})
        self.prepare()
        self.assertEqual((self.output / 'lib/libyang.so.3').readlink().as_posix(), 'libyang.so.3.9.1')

    def test_libyang_requires_native_image_parent(self):
        for arch, parent in (('arm64', self.parent), ('amd64', None)):
            with self.subTest(arch=arch, parent=parent):
                with self.assertRaisesRegex(ValueError, 'AMD64 image parent'):
                    sdk.prepare_libyang(self.root / 'staging', self.yang, parent, arch)

    def test_flattened_relative_soname_prefix_is_normalized(self):
        flattened = {'./' + name: './' + value if isinstance(value, str) else value
                     for name, value in self.members.items()}
        self.archive('nf.tar', flattened)
        self.prepare()
        for stem in sdk.LIBNL_STEMS:
            self.assertEqual((self.output / ('lib/' + stem + '.so.200')).readlink().as_posix(),
                             stem + '.so.200.26.0')
            self.assertEqual((self.output / ('lib/' + stem + '.so')).readlink().as_posix(),
                             stem + '.so.200')

    def test_conflicting_header_does_not_replace_existing_sdk(self):
        self.prepare()
        before = (self.output / 'sdk-build.json').read_bytes()
        changed = dict(self.members, **{'usr/include/libnl3/netlink/netlink.h': b'wrong header'})
        self.archive('genl.tar', changed)
        with self.assertRaisesRegex(ValueError, 'Conflicting libnl SDK member'):
            self.prepare()
        self.assertEqual((self.output / 'sdk-build.json').read_bytes(), before)

    def test_parent_with_different_library_is_rejected(self):
        changed = dict(self.members, **{'usr/lib/x86_64-linux-gnu/libnl-3.so.200.26.0': self.elf + b'wrong'})
        self.archive('parent.tar', changed)
        with self.assertRaisesRegex(ValueError, 'differs from the selected parent runtime'):
            self.prepare()
        self.assertFalse(self.output.exists())

    def test_escaping_soname_and_incomplete_sdk_are_rejected(self):
        changed = dict(self.members, **{'usr/lib/x86_64-linux-gnu/libnl-3.so.200': '/tmp/ambient.so'})
        self.archive('nf.tar', changed)
        with self.assertRaisesRegex(ValueError, 'Unexpected libnl SONAME link'):
            self.prepare()
        self.inputs = {'@libnl3//:incomplete': self.archive('incomplete.tar', {})}
        with self.assertRaisesRegex(ValueError, 'Incomplete libnl SDK'):
            self.prepare()


class RuntimeTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.library = self.root / 'usr/lib/libfixture.so.1.0'
        self.library.parent.mkdir(parents=True)
        self.library.write_bytes(b'\x7fELFdeclared fixture library')
        self.alias = self.library.with_name('libfixture.so.1')
        self.alias.symlink_to(self.library.name)
        self.selected = {
            'usr/lib/libfixture.so.1.0': {'kind': 'file', 'elf_machine': 62,
                                        'sha256': image.check.sha(self.library)},
            'usr/lib/libfixture.so.1': {'kind': 'symlink', 'linkname': self.library.name},
        }

    def test_accepts_real_payload_and_declared_soname_alias(self):
        actual = image.runtime_libraries(self.root, self.selected)
        self.assertEqual(set(actual), {str(self.library), str(self.alias)})
        self.assertEqual(set(actual.values()), {image.check.sha(self.library)})

    def test_loader_report_hashes_actual_resolved_files(self):
        libraries = image.runtime_libraries(self.root, self.selected)
        output = f'\t{self.alias.name} => {self.alias} (0x1234)\n'
        self.assertEqual(image.loaded_libraries(output, libraries),
                         {self.alias.name: image.check.sha(self.library)})

    def test_loader_report_rejects_changed_or_undeclared_libraries(self):
        libraries = image.runtime_libraries(self.root, self.selected)
        for output in (f'\tlibundeclared.so.1 => {self.alias} (0x1234)\n',
                       f'\t{self.alias.name} => {self.alias} (0x1234)\n'):
            with self.subTest(output=output):
                if 'libundeclared' not in output:
                    self.library.write_bytes(b'\x7fELFwrong runtime')
                with self.assertRaisesRegex(ValueError, 'resolved an ambient library'):
                    image.loaded_libraries(output, libraries)

    def test_unrecorded_or_changed_library_is_rejected(self):
        extra = self.library.with_name('libambient.so')
        extra.write_bytes(b'\x7fELFambient library')
        with self.assertRaisesRegex(ValueError, 'differ from the declared'):
            image.runtime_libraries(self.root, self.selected)
        extra.unlink()
        self.library.write_bytes(b'\x7fELFwrong version')
        with self.assertRaisesRegex(ValueError, 'differ from the declared'):
            image.runtime_libraries(self.root, self.selected)

    def test_make_cache_options_preserve_native_platform_and_quoted_arguments(self):
        with patch.dict(os.environ, {'BAZEL_CONTAINER_CACHE_DIR': str(self.root / 'cache'),
                'BAZEL_CONTAINER_ARGS': '--jobs=1 --define="fixture=value with spaces" --platforms=other'}, clear=True):
            options = image.root_options()
        self.assertIn('--repository_cache=' + str(self.root / 'cache/repository_cache'), options)
        self.assertIn('--disk_cache=' + str(self.root / 'cache/disk_cache'), options)
        self.assertIn('--define=fixture=value with spaces', options)
        self.assertEqual([value for value in options if value.startswith('--jobs=')][-1], '--jobs=1')
        self.assertEqual([value for value in options if value.startswith('--platforms=')][-1], image.OPTIONS[-2])

    def test_root_and_owner_builds_leave_job_scheduling_to_bazel(self):
        with patch.dict(os.environ, {}, clear=True):
            options = image.root_options()
        self.assertEqual([value for value in options if value.startswith(('--jobs=', '--local_resources='))], [])
        argv = ['p4rt_image.py', 'build', '--directory', str(self.root), '--owner-source', str(self.root),
                '--owner-revision', 'a' * 40, '--owner-output-base', str(self.root / 'output')]
        with patch('sys.argv', argv), patch.object(image.os, 'sched_getaffinity', return_value={0, 1, 2, 3, 4, 5}), \
                patch.object(image, 'build_images') as build:
            image.main()
        self.assertEqual(build.call_args.args[0].jobs, 6)

    def test_make_legacy_fallback_and_empty_container_override(self):
        with patch.dict(os.environ, {'BAZEL_SWSS_ARGS': '--jobs=3'}, clear=True):
            self.assertIn('--jobs=3', image.root_options())
        with patch.dict(os.environ, {'BAZEL_SWSS_ARGS': '--jobs=3', 'BAZEL_CONTAINER_ARGS': ''}, clear=True):
            self.assertNotIn('--jobs=3', image.root_options())

    def test_owner_profile_cannot_disable_the_recorded_yang_feature(self):
        flag = '--@sonic_swss_common//tools/bazel:yang_modules='
        for variable in ('BAZEL_CONTAINER_ARGS', 'BAZEL_SWSS_ARGS'):
            with self.subTest(variable=variable), patch.dict(os.environ, {
                    variable: flag + 'False --jobs=1'}, clear=True):
                options = image.root_options()
                self.assertEqual([value for value in options if value.startswith(flag)][-1],
                                 flag + 'True')
                self.assertEqual([value for value in options if value.startswith('--jobs=')][-1],
                                 '--jobs=1')

    def test_clean_checkout_renders_manifests_before_the_first_owner_graph_audit(self):
        events = []
        args = SimpleNamespace(directory=self.root / 'artifacts', owner_source=self.root / 'owner',
                               owner_revision='a' * 40, bazel='root-bazel')
        def execute(command, *unused, **keywords):
            if '--version' in command:
                return 'bazel 8.5.1\n'
            self.assertEqual(command[:3], ['make', '-f', 'tools/bazel/prepare_manifests.mk'])
            self.assertIn('MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk rules/docker-p4rt.mk', command)
            events.append('manifests')
        def audit(*unused, **keywords):
            self.assertEqual(events, ['manifests'], 'A fresh graph cannot load before metadata exists')
            events.append('audit')
            raise RuntimeError('stop before any compilation')
        with patch.object(image, 'native'), patch.object(image, 'source_identity', return_value='b' * 40), \
                patch.object(image.p4rt_handoff, 'source_identity'), \
                patch.object(image.command_log, 'execute', side_effect=execute), \
                patch.object(image.bazel_commands, 'audit_targets', side_effect=audit):
            with patch.dict(os.environ, {'SONIC_VS_IMAGE_TIMINGS': '1'}), redirect_stderr(io.StringIO()) as log, \
                    self.assertRaisesRegex(RuntimeError, 'stop before any compilation'):
                image.prepare_owner(args)
        self.assertEqual(log.getvalue(), '')
        self.assertEqual(events, ['manifests', 'audit'])

    def test_owner_preparation_passes_the_audited_libyang_runtime_tar_to_sdk(self):
        args = SimpleNamespace(directory=self.root / 'artifacts', owner_source=self.root / 'owner',
                               owner_revision='a' * 40, bazel='root-bazel')
        def collect(root, directory, receipt, targets, **unused):
            self.assertEqual(targets['libyang-runtime.tar'], '@libyang//:libyang_pkg')
            self.assertEqual(len(targets), 10)
            (directory / 'common.MODULE.bazel').write_text('module(version="0.0.9-' + image.COMMON_REVISION + '")')
        with patch.object(image, 'native'), patch.object(image, 'source_identity', return_value='b' * 40), \
                patch.object(image.p4rt_handoff, 'source_identity'), \
                patch.object(image.command_log, 'execute', return_value='bazel 8.5.1\n'), \
                patch.object(image.bazel_commands, 'audit_targets', return_value={}) as audit, \
                patch.object(image.build, 'collect_archives', side_effect=collect), \
                patch.object(image, 'prepare_repository', side_effect=RuntimeError('stop before owner')) as prepare, \
                patch.object(image, 'run_owner') as owner:
            with self.assertRaisesRegex(RuntimeError, 'stop before owner'):
                image.prepare_owner(args)
        preparation = args.directory / 'owner-preparation'
        self.assertIn('@libyang//:libyang_pkg', audit.call_args.args[2])
        self.assertEqual(prepare.call_args.kwargs['libyang_archive'], preparation / 'libyang-runtime.tar')
        self.assertEqual(prepare.call_args.kwargs['parent_archive'], preparation / 'config-engine.source.tar')
        owner.assert_not_called()

    def test_runtime_link_cannot_escape_private_root(self):
        outside = self.root.parent / (self.root.name + '-outside.so')
        outside.write_bytes(b'\x7fELFoutside library')
        self.addCleanup(outside.unlink)
        self.alias.unlink()
        self.alias.symlink_to(outside)
        with self.assertRaisesRegex(ValueError, 'escapes runtime root'):
            image.runtime_libraries(self.root, self.selected)

    def test_existing_make_and_ci_evidence_can_coexist(self):
        with patch.object(image, 'ROOT', self.root), patch.object(image.subprocess, 'run') as run, \
                patch.object(image.subprocess, 'check_output', side_effect=[
                    b'artifacts/vs/timing.json\0artifacts/archive/revision.json\0', 'a' * 40 + '\n']):
            self.assertEqual(image.source_identity(self.root / 'artifacts/p4rt'), 'a' * 40)
        run.assert_called_once_with(['git', 'diff', '--exit-code', '--ignore-submodules=none', 'HEAD'],
                                    cwd=self.root, check=True)

    def test_make_records_the_base_commit_with_local_edits_and_generated_outputs(self):
        checkout = self.root / 'make-checkout'
        checkout.mkdir()
        def git(*args):
            return subprocess.check_output(['git', *args], cwd=checkout, text=True).strip()
        git('init', '-q')
        (checkout / 'input').write_text('pinned')
        git('add', 'input')
        git('-c', 'user.name=Fixture', '-c', 'user.email=test@example.org', '-c', 'commit.gpgsign=false',
            'commit', '-qm', 'fixture')
        revision = git('rev-parse', 'HEAD')
        evidence = checkout / 'artifacts/p4rt'
        config = checkout / 'sonic-slave-trixie/files/apt/apt.conf.d/81norecommends'
        config.parent.mkdir(parents=True)
        config.write_text('generated by Make')
        with patch.object(image, 'ROOT', checkout):
            self.assertEqual(image.checkout_revision(), revision)
            with self.assertRaisesRegex(ValueError, 'undeclared inputs'):
                image.source_identity(evidence)
            (checkout / 'input').write_text('local edit')
            self.assertEqual(image.checkout_revision(), revision)
            with self.assertRaises(subprocess.CalledProcessError):
                image.source_identity(evidence)

    def test_make_uses_existing_manifests_and_cannot_supply_a_ci_receipt(self):
        args = SimpleNamespace(directory=self.root / 'artifacts', owner_source=self.root / 'owner',
                               owner_revision='a' * 40, bazel='root-bazel')
        revision = 'b' * 40
        def collect(root, directory, receipt, targets, **unused):
            (directory / 'common.MODULE.bazel').write_text('module(version="0.0.9-' + image.COMMON_REVISION + '")')
        with patch.object(image, 'native'), patch.object(image.p4rt_handoff, 'source_identity') as owner_identity, \
                patch.object(image, 'checkout_revision', return_value=revision) as identity, \
                patch.object(image.command_log, 'execute', return_value='bazel 8.5.1\n'), \
                patch.object(image, 'prepare_manifests') as render, \
                patch.object(image.bazel_commands, 'audit_targets'), \
                patch.object(image.build, 'collect_archives', side_effect=collect), \
                patch.object(image, 'prepare_repository'), patch.object(image.p4rt_handoff, 'same_common_library'), \
                patch.object(image, 'run_owner'), patch.object(image.p4rt_handoff, 'stage'), patch.object(image, 'write') as write:
            command = ['p4rt_image.py', 'prepare-make-owner', '--directory', str(args.directory),
                       '--owner-source', str(args.owner_source), '--owner-revision', args.owner_revision,
                       '--owner-output-base', str(self.root / 'owner-output'), '--bazel', args.bazel]
            with patch.object(sys, 'argv', command), patch.dict(os.environ, {'SONIC_VS_IMAGE_TIMINGS': '1'}), \
                    redirect_stderr(io.StringIO()) as log:
                image.main()
        phases = ('p4rt-audit', 'p4rt-sdk', 'p4rt-owner', 'p4rt-handoff')
        children = [f'[ vs timing ] [ {phase} ] [ {event} ]' for phase in phases for event in ('start', 'end')]
        self.assertEqual(log.getvalue().splitlines(),
                         ['[ vs timing ] [ p4rt-prep ] [ start ]', *children, '[ vs timing ] [ p4rt-prep ] [ end ]'])
        owner_identity.assert_called_once_with(args.owner_source, args.owner_revision)
        self.assertEqual(identity.call_count, 2)
        render.assert_not_called()
        path, receipt = write.call_args.args
        self.assertEqual(receipt['status'], 'prepared-for-make')
        self.assertEqual(receipt['revision'], revision)
        path.write_text(json.dumps({'status': receipt['status'], 'revision': revision}))
        with patch.object(image, 'source_identity', return_value=revision), \
                self.assertRaisesRegex(ValueError, 'different inputs'):
            image.prepared(args)

    def test_untracked_source_remains_rejected(self):
        with patch.object(image, 'ROOT', self.root), patch.object(image.subprocess, 'run'), \
                patch.object(image.subprocess, 'check_output', return_value=b'artifacts/vs/timing.json\0src/untracked.cc\0'):
            with self.assertRaisesRegex(ValueError, 'undeclared inputs'):
                image.source_identity(self.root / 'artifacts/p4rt')

    def test_tracked_changes_fail_before_evidence_exemption(self):
        with patch.object(image, 'ROOT', self.root), \
                patch.object(image.subprocess, 'run', side_effect=subprocess.CalledProcessError(1, 'git diff')), \
                patch.object(image.subprocess, 'check_output') as read:
            with self.assertRaises(subprocess.CalledProcessError):
                image.source_identity(self.root / 'artifacts/p4rt')
        read.assert_not_called()


if __name__ == '__main__':
    unittest.main()
