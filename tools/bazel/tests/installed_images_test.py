"""Exercise shared Docker image checks, including real native split symbols."""

import copy
import hashlib
import json
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import validate_runtime as subject
from tools.bazel.oci import runtime_smoke

ARCHITECTURE = {'x86_64': 'amd64', 'aarch64': 'arm64'}.get(platform.machine(), 'amd64')
TRIPLET = {'amd64': 'x86_64-linux-gnu', 'arm64': 'aarch64-linux-gnu'}[ARCHITECTURE]
COMMON = 'usr/lib/' + TRIPLET + '/libswsscommon.so.0.0.0'
DATABASE = 'run/redis/sonic-db/database_config.json'
ARCHIVE_BYTES = b'Standard Docker owns archive parsing'


class InstalledFilesTest(unittest.TestCase):
    def setUp(self):
        names = ('config', 'service', 'service-debug')
        self.policy = {'architecture': ARCHITECTURE, 'images': [], 'symbol_pairs': [
            {'runtime': 'service', 'debug': 'service-debug', 'paths': [COMMON]}]}
        for index, name in enumerate(names):
            spec = {'name': name, 'archive': name + '.gz', 'tag': name + ':latest',
                    'files': {COMMON: {}, DATABASE: {'mode': 0o644, 'uid': 0, 'gid': 0}},
                    'directory_aliases': {'var/run': '/run'},
                    'archive_sha256': hashlib.sha256(ARCHIVE_BYTES).hexdigest(),
                    'config_digest': 'sha256:' + hashlib.sha256(name.encode()).hexdigest(),
                    'expected_config': {},
                    'expected_rootfs': {'type': 'layers', 'diff_ids': list(names[:index + 1])}}
            if index:
                spec.update(parent=names[index - 1], inherit_files=[COMMON, DATABASE])
            self.policy['images'].append(spec)
        self.images = {name: {'Os': 'linux', 'Architecture': ARCHITECTURE, 'Config': {},
                             'Id': self.policy['images'][index]['config_digest'],
                             'RootFS': {'Type': 'layers', 'Layers': list(names[:index + 1])}}
                       for index, name in enumerate(names)}
        self.reports = {name: copy.deepcopy({
            'files': {COMMON: {'kind': 'file', 'sha256': 'common', 'mode': 0o555, 'uid': 0, 'gid': 0},
                      DATABASE: {'kind': 'file', 'sha256': 'config', 'mode': 0o644, 'uid': 0, 'gid': 0}},
            'directory_aliases': {'var/run': '/run'}}) for name in names}

    def test_declared_images_retain_source_base(self):
        subject.check_images_policy(self.policy)
        subject.verify_image_reports(self.policy, self.images, self.reports)

    def test_base_images_need_no_artificial_debug_variant(self):
        policy = copy.deepcopy(self.policy)
        policy['images'] = policy['images'][:1]
        policy['symbol_pairs'] = []
        subject.check_images_policy(policy)
        subject.verify_image_reports(policy, self.images, self.reports)

    def test_separately_configured_owners_share_one_identical_base(self):
        second = copy.deepcopy(self.policy)
        for spec in second['images'][1:]:
            spec['name'] = spec['name'].replace('service', 'syncd')
            spec['archive'] = spec['archive'].replace('service', 'syncd')
            spec['tag'] = spec['tag'].replace('service', 'syncd')
            spec['parent'] = spec['parent'].replace('service', 'syncd')
        second['symbol_pairs'] = [{'runtime': 'syncd', 'debug': 'syncd-debug', 'paths': [COMMON]}]
        for policies in ([self.policy], [self.policy, second, copy.deepcopy(self.policy)]):
            with self.subTest(count=len(policies)), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                arguments = ['validate_runtime.py', '--target', str(root / 'target'),
                             '--output', str(root / 'result.json')]
                for index, policy in enumerate(policies):
                    path = root / (str(index) + '.json')
                    path.write_text(json.dumps(policy))
                    arguments.extend(['--images-policy', str(path)])
                with mock.patch.object(sys, 'argv', arguments), mock.patch('builtins.print'), \
                        mock.patch.object(subject, 'validate_images', return_value={'status': 'passed'}) as validate:
                    self.assertEqual(subject.main(), 0)
                merged, target = validate.call_args.args
                self.assertEqual(target, root / 'target')
                self.assertEqual([spec['name'] for spec in merged['images']],
                                 ['config', 'service', 'service-debug'] +
                                 (['syncd', 'syncd-debug'] if len(policies) > 1 else []))
                self.assertEqual(merged['symbol_pairs'], self.policy['symbol_pairs'] +
                                 (second['symbol_pairs'] if len(policies) > 1 else []))
                self.assertEqual(json.loads((root / 'result.json').read_text()), {'status': 'passed'})

    def test_conflicting_shared_base_or_architecture_fails_before_docker(self):
        changes = (
            lambda p: p['images'][0].update(archive_sha256='0' * 64),
            lambda p: p['images'][0].update(config_digest='sha256:' + '0' * 64),
            lambda p: p['images'][0].update(expected_config={'Env': ['OTHER=1']}),
            lambda p: p['images'][0]['expected_rootfs'].update(diff_ids=['other-base']),
            lambda p: p['images'][0]['files'][DATABASE].update(mode=0o600),
            lambda p: p.update(architecture='arm64' if ARCHITECTURE == 'amd64' else 'amd64'),
        )
        for change in changes:
            second = copy.deepcopy(self.policy)
            change(second)
            with self.subTest(change=change), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                for name, policy in [('first', self.policy), ('second', second)]:
                    (root / (name + '.json')).write_text(json.dumps(policy))
                arguments = ['validate_runtime.py', '--images-policy', str(root / 'first.json'),
                             '--images-policy', str(root / 'second.json'), '--output', str(root / 'result.json')]
                with mock.patch.object(sys, 'argv', arguments), mock.patch.object(subject, 'docker') as docker, \
                        self.assertRaisesRegex(ValueError, 'disagree on shared image|different architectures'):
                    subject.main()
                docker.assert_not_called()
                self.assertFalse((root / 'result.json').exists())

    def test_wrong_base_or_debug_parent_fails(self):
        for name in ('service', 'service-debug'):
            images = copy.deepcopy(self.images)
            images[name]['RootFS']['Layers'][0] = 'unrelated-image'
            with self.subTest(image=name), self.assertRaisesRegex(ValueError, "parent's exact layers"):
                subject.verify_image_reports(self.policy, images, self.reports)

    def test_changed_inherited_files_or_metadata_fail(self):
        for name in ('service', 'service-debug'):
            for path in (COMMON, DATABASE):
                for field, value in (('sha256', 'overwritten'), ('mode', 0o777), ('uid', 1000), ('gid', 1000)):
                    reports = copy.deepcopy(self.reports)
                    reports[name]['files'][path][field] = value
                    with self.subTest(image=name, path=path, field=field), \
                            self.assertRaisesRegex(ValueError, 'inherited file|metadata differs'):
                        subject.verify_image_reports(self.policy, self.images, reports)

    def test_expected_hash_binds_base_files_to_static_validation(self):
        policy = copy.deepcopy(self.policy)
        policy['images'][0]['files'][COMMON]['sha256'] = 'expected-source-bytes'
        with self.assertRaisesRegex(ValueError, 'metadata differs'):
            subject.verify_image_reports(policy, self.images, self.reports)

    def test_wrong_directory_alias_and_platform_fail(self):
        self.reports['service']['directory_aliases']['var/run'] = '../elsewhere'
        with self.assertRaisesRegex(ValueError, 'directory aliases differ'):
            subject.verify_image_reports(self.policy, self.images, self.reports)
        for field, value in (('Os', 'windows'), ('Architecture', 'armhf')):
            images = copy.deepcopy(self.images)
            images['config'][field] = value
            with self.subTest(field=field), self.assertRaisesRegex(ValueError, 'platform differs'):
                subject.verify_image_reports(self.policy, images, self.reports)

    def test_native_architectures_and_invalid_declarations_fail_before_docker(self):
        for arch, machine in (('amd64', 'x86_64'), ('arm64', 'aarch64')):
            policy = dict(self.policy, architecture=arch)
            with mock.patch.object(subject.platform, 'machine', return_value=machine):
                subject.check_images_policy(policy)
            with mock.patch.object(subject.platform, 'machine', return_value='wrong-host'), \
                    self.assertRaisesRegex(ValueError, 'Native host'):
                subject.check_images_policy(policy)
        changes = (
            lambda p: p['images'][0].update(archive='../other.gz'),
            lambda p: p['images'][1].update(parent='absent'),
            lambda p: p['images'][0]['files'].update({'../etc/passwd': {}}),
            lambda p: p['images'].append(copy.deepcopy(p['images'][0])),
            lambda p: p['symbol_pairs'][0].update(paths=['undeclared/elf']),
            lambda p: p['images'][0].pop('archive_sha256'),
        )
        for change in changes:
            policy = copy.deepcopy(self.policy)
            change(policy)
            with self.subTest(change=change), mock.patch.object(subject, 'docker') as docker, \
                    self.assertRaises(ValueError):
                subject.validate_images(policy, Path('/unused'))
            docker.assert_not_called()

    def test_changed_archive_is_rejected_before_loading(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for spec in self.policy['images']:
                (root / spec['archive']).write_bytes(b'other build')
            with mock.patch.object(subject, 'docker') as docker, \
                    self.assertRaisesRegex(ValueError, 'archive differs from its static validation'):
                subject.validate_images(self.policy, root)
            docker.assert_not_called()

    def test_failed_docker_start_or_probe_exit_cleans_only_created_containers(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for spec in self.policy['images']:
                (root / spec['archive']).write_bytes(ARCHIVE_BYTES)
            for failure in ('docker', 'probe'):
                def docker(*arguments, **kwargs):
                    if arguments[0] == 'load':
                        return 'loaded'
                    if arguments[0] == 'image':
                        return json.dumps([self.images['config']])
                    if arguments[0] == 'create':
                        mount = next(arg for arg in arguments if arg.startswith('type=bind,'))
                        self.assertIn('src=' + str(ROOT / 'tools/bazel/oci/runtime_smoke.py'), mount)
                        return 'this-check-only'
                    if arguments[0] == 'start':
                        if failure == 'docker':
                            raise ValueError('Docker command failed')
                        return json.dumps(self.reports['config'])
                    if arguments[0] == 'inspect':
                        return json.dumps([{'State': {'ExitCode': 23}}])
                    self.fail(arguments)

                with self.subTest(failure=failure), mock.patch.object(subject, 'docker', side_effect=docker), \
                        mock.patch.object(subject.subprocess, 'run') as cleanup, self.assertRaisesRegex(
                            ValueError, 'Docker command failed|file probe failed'):
                    subject.validate_images(self.policy, root)
                self.assertEqual([call.args[0] for call in cleanup.call_args_list],
                                 [['docker', 'rm', '--force', 'this-check-only']])

    def test_actual_file_probe_rejects_directory_alias_and_nonregular_file(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for name in (COMMON, DATABASE):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b'installed payload')
            (root / 'var').mkdir()
            (root / 'var/run').symlink_to('/run')
            report = runtime_smoke.inspect_files([COMMON, DATABASE], ['var/run'], root=root)
            self.assertEqual(report['directory_aliases'], {'var/run': '/run'})
            self.assertEqual(report['files'][COMMON]['sha256'], subject.sha(root / COMMON))
            inputs = {'files': [str(root / COMMON)], 'directory_aliases': [str(root / 'var/run')]}
            command = [sys.executable, '-B', str(ROOT / 'tools/bazel/oci/runtime_smoke.py'),
                       '--inspect-files', json.dumps(inputs)]
            installed = json.loads(subprocess.check_output(command, text=True))
            self.assertEqual(installed['files'][str(root / COMMON)], report['files'][COMMON])
            (root / 'var/run').unlink()
            (root / 'var/run').mkdir()
            with self.assertRaises(OSError):
                runtime_smoke.inspect_files([COMMON], ['var/run'], root=root)
            self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)
            (root / COMMON).unlink()
            (root / COMMON).symlink_to('/unrelated')
            with self.assertRaisesRegex(ValueError, 'regular file'):
                runtime_smoke.inspect_files([COMMON], [], root=root)


@unittest.skipUnless(platform.machine() in ('x86_64', 'aarch64') and
                     all(shutil.which(tool) for tool in ('cc', 'readelf', 'objcopy')),
                     'native AMD64/ARM64 C compiler and binutils required')
class InstalledSymbolsTest(unittest.TestCase):
    setUp = InstalledFilesTest.setUp

    def test_copied_native_elf_symbols_crc_and_cleanup(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / 'common.c'
            source.write_text('int inherited_common(void) { return 42; }\n')
            binary = root / 'common.so'
            command = subject.artifact_validation.command
            command(['cc', '-g', '-shared', '-fPIC', '-Wl,--build-id=sha1', str(source), '-o', str(binary)])
            identifier = re.search(r'Build ID: ([0-9a-f]+)', command(['readelf', '-n', str(binary)]))[1]
            symbols = root / (identifier[2:] + '.debug')
            command(['objcopy', '--only-keep-debug', str(binary), str(symbols)])
            command(['objcopy', '--strip-debug', '--add-gnu-debuglink=' + str(symbols), str(binary)])
            for report in self.reports.values():
                report['files'][COMMON]['sha256'] = subject.sha(binary)
            for spec in self.policy['images']:
                (root / spec['archive']).write_bytes(ARCHIVE_BYTES)
            created, commands = {}, []
            names = list(self.images)

            def docker(*arguments, **kwargs):
                commands.append(arguments)
                operation = arguments[0]
                if operation == 'load':
                    self.assertEqual(arguments[1], '--input')
                    return 'loaded'
                if operation == 'image':
                    name = arguments[-1].removesuffix(':latest')
                    return json.dumps([self.images[name]])
                if operation == 'create':
                    for flag in ('--network=none', '--read-only', '--cap-drop=ALL',
                                 '--security-opt=no-new-privileges', '--pids-limit=32', '--memory=256m'):
                        self.assertIn(flag, arguments)
                    self.assertTrue(any(arg.endswith('dst=/probe/runtime_smoke.py,readonly') for arg in arguments))
                    name = next(name for name in names if self.images[name]['Id'] in arguments)
                    created[name] = 'container-' + name
                    return created[name]
                if operation == 'start':
                    return json.dumps(self.reports[arguments[-1].removeprefix('container-')])
                if operation == 'inspect':
                    return json.dumps([{'State': {'ExitCode': 0}}])
                if operation == 'cp':
                    shutil.copyfile(symbols if '/usr/lib/debug/' in arguments[1] else binary, arguments[2])
                    return ''
                self.fail(arguments)

            actual_run = subprocess.run

            def cleanup_or_tool(arguments, **kwargs):
                if arguments[:3] == ['docker', 'rm', '--force']:
                    return subprocess.CompletedProcess(arguments, 0)
                return actual_run(arguments, **kwargs)

            for corrupt in (False, True):
                if corrupt:
                    # Keep the same ELF/build ID but alter the copied symbol bytes.
                    symbols.write_bytes(symbols.read_bytes() + b'changed debug image payload')
                created.clear()
                with mock.patch.object(subject, 'docker', side_effect=docker), \
                        mock.patch.object(subject.subprocess, 'run', side_effect=cleanup_or_tool) as run:
                    if corrupt:
                        with self.assertRaisesRegex(ValueError, 'debuglink CRC differs'):
                            subject.validate_images(self.policy, root)
                    else:
                        result = subject.validate_images(self.policy, root)
                        self.assertEqual(result['debug_pairs'][0]['build_id'], identifier)
                        self.assertEqual(result['debug_pairs'][0]['runtime'], 'service')
                cleaned = [call.args[0][-1] for call in run.call_args_list
                           if call.args[0][:3] == ['docker', 'rm', '--force']]
                self.assertEqual(cleaned, list(created.values()))
            self.assertEqual(len([args for args in commands if args[0] == 'load']), 6)


if __name__ == '__main__':
    unittest.main()
