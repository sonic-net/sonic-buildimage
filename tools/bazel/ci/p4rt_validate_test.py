"""Reject stale stage receipts and undeclared changes to installed image files."""

import ast
import copy
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

from tools.bazel.ci import p4rt_loaded, p4rt_validate as validation


class IncrementalOwnerOutputTest(unittest.TestCase):
    def test_external_owner_tar_uses_the_recorded_execution_root(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            workspace, execution, directory = (root / name for name in ('workspace', 'execroot', 'artifacts'))
            for path in (workspace, execution, directory):
                path.mkdir()
            source = workspace / 'dockers/docker-sonic-p4rt/p4rt_vars.j2'
            source.parent.mkdir(parents=True)
            source.write_bytes(b'original template\n')
            original = source.read_bytes()
            names = [*(name + '.gz' for name in validation.IMAGES), 'p4rt.source.tar', 'p4rt.debug.tar']
            paths = {name: ('external/+_repo_rules4+p4rt_owner_payload/p4rt_runtime.tar'
                            if name == 'p4rt.source.tar' else 'bazel-out/config/bin/' + name)
                     for name in names}
            for name, path in paths.items():
                output = execution / path
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_bytes(name.encode())
            handoff = workspace / '.bazel-inputs/p4rt-owner/handoff.json'
            handoff.parent.mkdir(parents=True)
            handoff.write_text('{}\n')
            receipt = {
                'status': 'built', 'revision': 'reviewed-revision',
                'layouts': {name: 'layouts/' + name for name in (*validation.IMAGES, validation.SYMBOLS)},
                'base_layout': 'layouts/base',
                'artifacts': {name: {'sha256': validation.check.sha(execution / path)}
                              for name, path in paths.items()},
            }
            for path in (*receipt['layouts'].values(), receipt['base_layout']):
                (execution / path).mkdir(parents=True)
            (directory / 'build-receipt.json').write_text(json.dumps(receipt))
            (directory / 'build-events.jsonl').write_text(json.dumps({
                'workspaceInfo': {'localExecRoot': str(execution)}}) + '\n')
            labels = {validation.p4rt_image.TARGETS[name]: path for name, path in paths.items()}

            def queried_output(command, **kwargs):
                self.assertEqual(command[1], 'cquery')
                return labels[command[-1]] + '\n'

            def build_outputs(command, **kwargs):
                self.assertEqual(command[1], 'build')
                edited = source.read_bytes() != original
                for name, path in paths.items():
                    (execution / path).write_bytes(name.encode() + (b' edited' if edited and name.endswith('.gz') else b''))

            def layout(path, platform):
                self.assertEqual(platform, 'linux/amd64')
                changed = source.read_bytes() != original and path.name != 'base'
                return SimpleNamespace(descriptor={'digest': path.name + ('-edited' if changed else '')})

            with (mock.patch.object(validation, 'ROOT', workspace),
                  mock.patch.object(validation, 'clean_source', return_value='reviewed-revision'),
                  mock.patch.object(validation.subprocess, 'check_output', side_effect=queried_output),
                  mock.patch.object(validation.subprocess, 'run', side_effect=build_outputs),
                  mock.patch.object(validation.oci_layout, 'validate_layout', side_effect=layout)):
                validation.incremental(SimpleNamespace(directory=directory, bazel='bazel'))
            result = json.loads((directory / 'incremental.json').read_text())
            self.assertEqual(result['status'], 'passed')
            self.assertEqual(source.read_bytes(), original)
            self.assertFalse((workspace / paths['p4rt.source.tar']).exists())
            self.assertEqual(result['baseline']['outputs'], result['restored']['outputs'])
            self.assertEqual(result['baseline']['outputs']['p4rt.source.tar'],
                             result['changed']['outputs']['p4rt.source.tar'])


class DownloadedImagesTest(unittest.TestCase):
    def test_release_intermediates_only_after_all_artifacts_match(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            artifacts = {}
            for name in validation.p4rt_image.TARGETS:
                path = directory / name
                path.write_text(name)
                artifacts[name] = {'sha256': validation.check.sha(path)}
            p4rt_loaded.verify_download(directory, artifacts, False)
            expected = {p.name for p in directory.iterdir()}
            self.assertEqual(expected, set(artifacts))
            changed = {**artifacts, 'p4rt.debug.tar': {'sha256': '0' * 64}}
            with self.assertRaisesRegex(ValueError, 'artifact changed'):
                p4rt_loaded.verify_download(directory, changed, True)
            self.assertEqual({p.name for p in directory.iterdir()}, expected)
            p4rt_loaded.verify_download(directory, artifacts, True)
            self.assertEqual({p.name for p in directory.iterdir()}, expected - {'p4rt.debug.tar', 'p4rt.source.tar'})


class DebugSaiTest(unittest.TestCase):
    def setUp(self):
        self.payloads = {}
        self.debug = {}
        self.metadata = {'source_packages': []}
        self.hashes = {'libsairedis': 'a' * 64, 'libsaimetadata': 'b' * 64}
        self.owner = {'schema': 1, 'module': 'sonic-sairedis', 'architecture': 'amd64', 'packages': {}}
        for package, target, names in (
                ('libsairedis', '//lib:libsairedis_pkg', ['libsairedis.so.0.0.0']),
                ('libsaimetadata', '//meta:libsaimetadata_pkg', ['libsaimeta.so.0.0.0', 'libsaimetadata.so.0.0.0'])):
            files = {'usr/lib/x86_64-linux-gnu/' + name: {
                'kind': 'file', 'uid': 0, 'gid': 0, 'mode': 0o644, 'sha256': name, 'elf_machine': 62}
                for name in names}
            self.payloads[package] = files
            self.debug.update(copy.deepcopy(files))
            self.metadata['source_packages'].append({'package': package, 'target': '@@sonic-sairedis+' + target,
                'source_revision': 'a' * 40, 'payload_sha256': self.hashes[package], 'version': '1.0.0',
                'native_files': [{'path': name, **row} for name, row in files.items()]})
            self.owner['packages'][package] = {'runtime_target': target,
                'payload_sha256': self.hashes[package], 'control_fields': {'Version': '1.0.0'}}

    def verify(self, runtime=None):
        validation.verify_debug_sai(runtime or {}, {}, self.debug, self.metadata, self.payloads, self.hashes, self.owner)

    def test_declared_debug_closure_and_owner_metadata_are_required(self):
        self.verify()
        self.metadata['source_packages'].pop()
        with self.assertRaisesRegex(ValueError, 'missing source-owned'):
            self.verify()

    def test_owner_metadata_is_bound_to_the_exact_retained_tars(self):
        self.verify()
        self.owner['packages']['libsairedis']['payload_sha256'] = 'c' * 64
        with self.assertRaisesRegex(ValueError, 'differs from retained source TAR'):
            self.verify()
        self.owner['packages']['libsairedis']['payload_sha256'] = self.hashes['libsairedis']
        self.owner['packages']['syncd-vs'] = {}
        with self.assertRaisesRegex(ValueError, 'only the two SAI library packages'):
            self.verify()

    def test_installed_target_must_be_the_canonical_sairedis_owner(self):
        self.verify()
        record = self.metadata['source_packages'][0]
        target = self.owner['packages'][record['package']]['runtime_target']
        for wrong in (target, '@sonic_sairedis' + target, '@@another-module+' + target,
                      '@@sonic-sairedis+//lib:another_target'):
            with self.subTest(target=wrong):
                record['target'] = wrong
                with self.assertRaisesRegex(ValueError, 'missing source-owned'):
                    self.verify()

    def test_missing_changed_or_runtime_sai_libraries_are_rejected(self):
        name = next(iter(self.debug))
        original = self.debug[name]
        with self.assertRaisesRegex(ValueError, 'leaked into an earlier stage'):
            self.verify({name: original})
        self.debug[name] = {**original, 'mode': 0o777}
        with self.assertRaisesRegex(ValueError, 'payload changed'):
            self.verify()
        del self.debug[name]
        with self.assertRaisesRegex(ValueError, 'payload changed'):
            self.verify()

    def test_extra_native_library_and_duplicate_metadata_are_rejected(self):
        first = self.metadata['source_packages'][0]
        self.metadata['source_packages'].append(copy.deepcopy(first))
        with self.assertRaisesRegex(ValueError, 'duplicate debug source'):
            self.verify()
        self.metadata['source_packages'].pop()
        self.payloads['libsairedis']['usr/lib/x86_64-linux-gnu/unexpected.so'] = next(iter(self.debug.values()))
        with self.assertRaisesRegex(ValueError, 'native inventory differs'):
            self.verify()


class LoadedSaiSelectionTest(unittest.TestCase):
    def test_debug_profile_requires_all_three_sai_libraries_and_matching_pairs(self):
        path = Path(__file__).resolve().parents[3] / 'dockers/docker-sonic-p4rt/bazel/runtime_probe.py'
        tree = ast.parse(path.read_text())
        selected = [item for item in tree.body if isinstance(item, ast.FunctionDef) and
                    item.name in ('require', 'source_debug_pairs')]
        self.assertEqual(len(selected), 2)
        namespace = {}
        exec(compile(ast.Module(body=selected, type_ignores=[]), str(path), 'exec'), namespace)
        choose = namespace['source_debug_pairs']
        common = 'usr/lib/x86_64-linux-gnu/libswsscommon.so.0.0.0'
        sai = ['usr/lib/x86_64-linux-gnu/' + name for name in
               ('libsairedis.so.0.0.0', 'libsaimeta.so.0.0.0', 'libsaimetadata.so.0.0.0')]
        report = {'source_debug_pairs': [{'path': name} for name in [common, *sai]]}
        self.assertEqual(choose(report, {common: {}}, False), [{'path': common}])
        native = {name: {} for name in [common, *sai]}
        self.assertEqual(len(choose(report, native, True)), 4)
        with self.assertRaisesRegex(AssertionError, 'missing debug SAI runtime'):
            choose(report, {common: {}}, True)
        report['source_debug_pairs'].pop()
        with self.assertRaisesRegex(AssertionError, 'missing or duplicate'):
            choose(report, native, True)
        report['source_debug_pairs'].append({'path': common})
        with self.assertRaisesRegex(AssertionError, 'missing or duplicate'):
            choose(report, native, True)

    def installed_exec_readiness(self, read_bytes):
        path = Path(__file__).resolve().parents[3] / 'dockers/docker-sonic-p4rt/bazel/runtime_probe.py'
        tree = ast.parse(path.read_text())
        selected = [item for item in ast.walk(tree) if isinstance(item, ast.FunctionDef) and
                    item.name in ('require', 'wait_for', 'p4rt_executed')]
        self.assertEqual({item.name for item in selected}, {'require', 'wait_for', 'p4rt_executed'})
        self.assertEqual(len(selected), 3)
        ticks, sleeps = [0], []

        def sleep(seconds):
            self.assertEqual(seconds, 0.2)
            sleeps.append(seconds)
            ticks[0] += 1

        class FakeRedisError(Exception):
            pass

        namespace = {
            'argv': SimpleNamespace(read_bytes=read_bytes),
            'children': {},
            'time': SimpleNamespace(monotonic=lambda: ticks[0] / 5, sleep=sleep),
            'redis': SimpleNamespace(exceptions=SimpleNamespace(RedisError=FakeRedisError)),
        }
        exec(compile(ast.Module(body=selected, type_ignores=[]), str(path), 'exec'), namespace)
        return namespace, ticks, sleeps

    def test_waits_for_the_installed_daemon_to_exec(self):
        wrapper = b'/bin/bash\0/usr/local/bin/p4rt.sh\0'
        daemon = (b'/usr/local/bin/p4rt\0--p4rt_grpc_port=9559\0--use_genetlink=false\0'
                  b'--p4rt_unix_socket=/run/p4rt.sock\0--use_insecure_server_credentials\0')
        read_bytes = mock.Mock(side_effect=[wrapper, daemon])
        namespace, ticks, sleeps = self.installed_exec_readiness(read_bytes)
        command = namespace['wait_for'](namespace['p4rt_executed'])
        self.assertEqual(command, daemon.split(b'\0'))
        self.assertEqual(read_bytes.call_count, 2)
        self.assertEqual(ticks, [1])
        self.assertEqual(sleeps, [0.2])

    def test_wrapper_cannot_satisfy_the_default_readiness_deadline(self):
        read_bytes = mock.Mock(return_value=b'/bin/bash\0/usr/local/bin/p4rt.sh\0')
        namespace, ticks, sleeps = self.installed_exec_readiness(read_bytes)
        with self.assertRaisesRegex(AssertionError, '^timed out waiting for the installed service$'):
            namespace['wait_for'](namespace['p4rt_executed'])
        self.assertEqual(read_bytes.call_count, 300)
        self.assertEqual(ticks[0] / 5, 60)
        self.assertEqual(sleeps, [0.2] * 300)


class StageBindingTest(unittest.TestCase):
    def setUp(self):
        self.apt = {'image': 'docker-sonic-p4rt', 'group': 'debug',
                    'base_manifest_digest': 'sha256:symbols', 'dependency_check': {'status': 'satisfied'}}
        self.metadata = {'architecture': 'amd64', 'base_manifest_digest': 'sha256:symbols',
                         'apt_selection_sha256': 'apt-hash', 'dependency_check': {'status': 'satisfied'}}

    def verify(self):
        validation.stage_binding('debug', self.apt, self.metadata, base_digest='sha256:symbols', apt_sha='apt-hash')

    def test_final_debug_requires_the_completed_symbols_base(self):
        self.verify()
        for record in (self.apt, self.metadata):
            original = record['base_manifest_digest']
            record['base_manifest_digest'] = 'sha256:runtime'
            with self.assertRaisesRegex(ValueError, 'different completed OCI base'):
                self.verify()
            record['base_manifest_digest'] = original

    def test_unrelated_apt_receipt_or_unsatisfied_dependencies_are_rejected(self):
        self.metadata['apt_selection_sha256'] = 'earlier-stage'
        with self.assertRaisesRegex(ValueError, 'checked APT selection'):
            self.verify()
        self.metadata['apt_selection_sha256'] = 'apt-hash'
        self.apt['dependency_check']['status'] = 'missing'
        with self.assertRaisesRegex(ValueError, 'unsatisfied stage'):
            self.verify()

    def test_workspace_owner_cannot_be_relabelled_as_a_bzlmod_source(self):
        self.metadata['source_contract_sha256'] = 'contract'
        validation.stage_binding('symbols', self.apt, self.metadata, base_digest='sha256:symbols',
                                 apt_sha='apt-hash', contract_sha='contract')
        self.metadata['source_modules'] = {'sonic-pins': {'version': 'invented'}}
        with self.assertRaisesRegex(ValueError, 'real owner contract'):
            validation.stage_binding('symbols', self.apt, self.metadata, base_digest='sha256:symbols',
                                     apt_sha='apt-hash', contract_sha='contract')


class InheritanceTest(unittest.TestCase):
    def setUp(self):
        self.parent = {name: {'kind': 'file', 'uid': 0, 'gid': 0, 'mode': 0o644, 'sha256': name}
                       for name in ('etc/group', 'etc/gshadow', 'etc/passwd', 'usr/bin/service')}
        self.child = copy.deepcopy(self.parent)
        self.rows = ['root:x:0:', 'redis:x:103:']
        self.debug_rows = self.rows + ['_ssh:x:104:']

    def verify(self):
        validation.verify_groups(self.parent, self.child, self.rows, self.debug_rows)

    def test_only_the_declared_ssh_group_may_be_appended(self):
        self.verify()
        for rows in ([self.rows[1], self.rows[0], '_ssh:x:104:'],
                     self.debug_rows + ['root:x:900:'], ['root:x:0:', 'redis:x:103:other', '_ssh:x:104:']):
            with self.subTest(rows=rows), self.assertRaises(ValueError):
                validation.verify_groups(self.parent, self.child, self.rows, rows)

    def test_group_metadata_and_shadow_state_cannot_be_exempted(self):
        for name, key, value in (('etc/group', 'mode', 0o777), ('etc/group', 'uid', 1000),
                                 ('etc/gshadow', 'sha256', 'changed')):
            with self.subTest(name=name, key=key):
                old = self.child[name][key]
                self.child[name][key] = value
                with self.assertRaises(ValueError):
                    self.verify()
                self.child[name][key] = old

    def test_symbol_stage_preserves_every_runtime_file(self):
        validation.inherited_files(self.parent, self.child)
        self.child['usr/bin/service']['mode'] = 0o777
        with self.assertRaisesRegex(ValueError, 'undeclared inherited file change'):
            validation.inherited_files(self.parent, self.child)


class OwnerPackageTest(unittest.TestCase):
    def test_native_inventory_and_provenance_must_match_the_real_owner(self):
        payload = {'usr/local/bin/p4rt': {'elf_machine': 62, 'sha256': 'actual', 'mode': 0o755, 'uid': 0, 'gid': 0}}
        definition = {'control_fields': {'Version': '1.0'}, 'target': '//p4rt_app:p4rt_runtime',
                      'source_revision': 'a' * 40, 'payload_sha256': 'tar'}
        contract = {'packages': {'sonic-p4rt': definition}}
        row = {'package': 'sonic-p4rt', 'version': '1.0', **{k: definition[k] for k in
               ('target', 'source_revision', 'payload_sha256')},
               'native_files': [{'path': 'usr/local/bin/p4rt', **payload['usr/local/bin/p4rt']}]}
        metadata = {'source_packages': [row]}
        validation.owner_package_state(metadata, contract, payload)
        row['native_files'].append(copy.deepcopy(row['native_files'][0]))
        with self.assertRaisesRegex(ValueError, 'native inventory'):
            validation.owner_package_state(metadata, contract, payload)
        row['native_files'].pop()
        row['source_revision'] = 'b' * 40
        with self.assertRaisesRegex(ValueError, 'provenance'):
            validation.owner_package_state(metadata, contract, payload)


class ConfigTest(unittest.TestCase):
    def test_only_declared_startup_environment_and_manifest_change(self):
        parent = {'User': '0', 'WorkingDir': '/', 'Cmd': ['/bin/sh'], 'Env': ['PATH=/usr/bin'],
                  'Labels': {'inherited': 'keep'}}
        runtime = {'User': '0', 'WorkingDir': '/', 'Entrypoint': ['/usr/local/bin/supervisord'],
                   'Env': ['PATH=/usr/bin', 'DEBIAN_FRONTEND=noninteractive'],
                   'Labels': {'inherited': 'keep', 'com.azure.sonic.manifest': '{"name":"p4rt"}'}}
        validation.verify_config(runtime, parent, runtime=True, manifest={'name': 'p4rt'})
        for key, changed in (('User', '1000'), ('WorkingDir', '/tmp'), ('Cmd', ['unexpected']),
                             ('Env', runtime['Env'] + ['LD_PRELOAD=/override.so'])):
            with self.subTest(key=key), self.assertRaises(ValueError):
                validation.verify_config({**runtime, key: changed}, parent, runtime=True, manifest={'name': 'p4rt'})
        validation.verify_config(runtime, runtime)


if __name__ == '__main__':
    unittest.main()
