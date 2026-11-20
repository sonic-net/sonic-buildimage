"""Keep the root and owner JVMs separate without relaxing original acceptance."""
import json
import os
import shlex
from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from tools.bazel.ci import p4rt_image as image


class LifecycleTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.args = SimpleNamespace(bazel='root-bazel', owner_bazel='owner-bazel',
            owner_source=self.root / 'owner', owner_output_base=self.root / 'output', jobs=1)
        self.receipt = {'commands': []}
        self.calls = []
        self.names = []
        self.before = {'state': 'S', 'started': 42, 'rss_pages': 900}

    def execute(self, command, *unused, **kwargs):
        self.calls.append(command)
        self.names.append(unused[2])
        return '123\n' if command == ['root-bazel', 'info', 'server_pid'] else 'ok'

    def test_make_build_preserves_shared_server_and_forwards_only_owner_cache_settings(self):
        backend = self.root / 'configured owner bazel'
        backend.write_text('#!/bin/sh\nprintf "%s\\n" "$@"\n')
        backend.chmod(0o755)
        self.args.owner_bazel = str(backend)
        endpoint = image.PRIVATE_CACHE_ENDPOINT
        for policy, upload in (('y', 'true'), ('n', 'false')):
            with self.subTest(upload=policy):
                self.check_make_owner_cache_settings(endpoint, policy, upload)
        self.assertEqual(list(self.root.glob('p4rt-owner-client-*')), [])

    def check_make_owner_cache_settings(self, endpoint, policy, upload):
        # The owner follows the image producers' upload policy: only builds that
        # may write the selected cache (master pushes) upload owner results.
        permitted = {'SONIC_BAZEL_REMOTE_CACHE': endpoint, 'SONIC_BAZEL_REMOTE_UPLOAD': policy,
                     'BAZEL_CONTAINER_ARGS': '--experimental_remote_downloader=' + endpoint +
                     ' --experimental_remote_downloader_local_fallback --remote_header=X=private --local_resources=cpu=2'}
        def execute(command, directory, receipt, name, **unused):
            self.assertEqual(name, 'owner-build')
            launcher = next(value.removeprefix('--bazel=') for value in command if value.startswith('--bazel='))
            request = [launcher, '--output_base=/owner-base', 'build', '--config=p4rt-runtime', '//p4rt_app:runtime']
            recorded = subprocess.check_output(request, text=True).splitlines()
            self.assertEqual(recorded[1:], request[1:])
            self.assertTrue(recorded[0].startswith('--bazelrc='))
            rc = Path(recorded[0].split('=', 1)[1])
            settings = [shlex.split(line) for line in rc.read_text().splitlines()]
            self.assertEqual(settings, [['common', '--remote_cache=' + endpoint],
                                        ['common', '--experimental_remote_downloader=' + endpoint],
                                        ['common', '--experimental_remote_downloader_local_fallback'],
                                        ['common', '--experimental_downloader_config=' +
                                         str(image.ROOT / 'src/sonic-p4rt/source_mirrors.cfg')],
                                        ['build', '--remote_upload_local_results=' + upload]])
            return 'OK'
        with patch.dict(os.environ, permitted, clear=True), patch.object(image.command_log, 'execute', side_effect=execute) as call:
            self.assertEqual(image.run_owner(self.args, self.root, self.root, 'build',
                                             self.root, self.receipt, make_build=True), 'OK')
            self.assertEqual(call.call_count, 1)
            self.assertIn('--remote_cache=' + endpoint, image.root_options(make_build=True))
            self.assertIn('--remote_upload_local_results=' + upload, image.root_options(make_build=True))
            self.assertNotIn('--remote_cache=' + endpoint, image.root_options())

    def test_make_with_no_selected_service_uses_the_local_owner_only(self):
        with patch.dict(os.environ, {'BAZEL_CONTAINER_ARGS': '--experimental_remote_downloader=' +
                                     image.PRIVATE_CACHE_ENDPOINT}, clear=True):
            image.make_owner_launcher(self.root, 'bazel')
        settings = (self.root / 'make-owner.bazelrc').read_text()
        self.assertNotIn('grpc://', settings)
        self.assertNotIn('--remote_cache=', settings)
        self.assertIn('--experimental_downloader_config=', settings)
        self.assertIn('--remote_upload_local_results=false', settings)

    def run_phase(self, phase='build', after=None, shutdown_error=None):
        def execute(command, *args, **kwargs):
            if command == ['root-bazel', 'shutdown'] and shutdown_error:
                self.calls.append(command)
                raise subprocess.CalledProcessError(shutdown_error, command)
            return self.execute(command, *args, **kwargs)
        with patch.object(image, 'server_process', side_effect=[self.before, after]), \
                patch.object(image.command_log, 'execute', side_effect=execute):
            return image.run_owner(self.args, self.root / 'sdk', self.root / 'artifacts', phase,
                                   self.root, self.receipt,
                                   self.root / 'runtime.json' if phase == 'test' else None)

    def test_build_and_test_release_root_before_owner(self):
        for phase in ('build', 'test'):
            self.calls.clear()
            self.run_phase(phase)
            self.assertEqual(self.calls[:2], [['root-bazel', 'info', 'server_pid'], ['root-bazel', 'shutdown']])
            self.assertEqual(self.names[-1], 'owner-tests' if phase == 'test' else 'owner-build')
            self.assertIn('--phase=' + phase, self.calls[2])
            self.assertIn('--bazel=owner-bazel', self.calls[2])
            self.assertEqual('--runtime-receipt=' + str(self.root / 'runtime.json') in self.calls[2], phase == 'test')
            self.assertTrue(self.receipt['root_server_shutdown']['caches_and_outputs_retained'])

    def test_live_root_prevents_owner_execution(self):
        with self.assertRaisesRegex(ValueError, 'remains alive'):
            self.run_phase(after=self.before)
        self.assertEqual(len(self.calls), 2)

    def test_wait_failure_only_allows_identical_zero_rss_zombie(self):
        self.run_phase(after={'state': 'Z', 'started': 42, 'rss_pages': 0}, shutdown_error=36)
        self.assertEqual(len(self.calls), 3)

    def test_reused_pid_or_resident_zombie_is_rejected(self):
        for after in ({'state': 'Z', 'started': 99, 'rss_pages': 0},
                      {'state': 'Z', 'started': 42, 'rss_pages': 1}):
            self.calls.clear()
            with self.assertRaisesRegex(ValueError, 'remains alive'):
                self.run_phase(after=after, shutdown_error=36)
            self.assertEqual(len(self.calls), 2)

    def test_unrelated_shutdown_error_remains_fatal(self):
        with self.assertRaises(subprocess.CalledProcessError):
            self.run_phase(shutdown_error=2)
        self.assertEqual(len(self.calls), 2)

    def test_missing_initial_process_is_rejected(self):
        with patch.object(image, 'server_process', return_value=None), \
                patch.object(image.command_log, 'execute', side_effect=self.execute):
            with self.assertRaisesRegex(ValueError, 'identity is unavailable'):
                image.run_owner(self.args, self.root, self.root, 'build', self.root, self.receipt)
        self.assertEqual(len(self.calls), 1)


class ReviewShutdownTest(unittest.TestCase):
    def test_shutdown_and_standalone_version_preserve_arguments(self):
        wrapper = Path(image.__file__).with_name('review_bazel.sh')
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            fake = root / 'bazel'
            fake.write_text('#!/bin/sh\nprintf "%s\\n" "$@"\n')
            fake.chmod(0o755)
            env = dict(os.environ, BAZEL_REAL=str(fake), SONIC_BAZEL_REGISTRY_REVISION='a' * 40)
            for args in (['--version'], ['--output_base=' + str(root / 'base'), '--host_jvm_args=-Xmx3g', 'shutdown']):
                result = subprocess.run(['bash', str(wrapper), *args], env=env, text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                actual = result.stdout.splitlines()
                if args == ['--version']:
                    self.assertEqual(actual, ['version', '--gnu_format'])
                else:
                    self.assertEqual(actual[:2], args[:2])
                    self.assertEqual(actual[2:5], ['--nosystem_rc', '--nohome_rc', '--noworkspace_rc'])
                    self.assertTrue(actual[5].startswith('--bazelrc='))
                    self.assertFalse(Path(actual[5].split('=', 1)[1]).exists())
                    self.assertEqual(actual[6:], ['shutdown'])
            result = subprocess.run(['bash', str(wrapper), 'shutdown', '--registry=wrong'], env=env, text=True, capture_output=True)
            self.assertEqual(result.returncode, 2)
            self.assertEqual(result.stdout, '')

    def test_shutdown_uses_the_same_rc_selection_as_info_despite_home_startup(self):
        wrapper = Path(image.__file__).with_name('review_bazel.sh')
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            home = root / 'home'
            home.mkdir()
            (home / '.bazelrc').write_text(
                'startup --output_base=' + str(root / 'wrong-server') + '\n'
                'common --disk_cache=' + str(root / 'cache') + '\n')
            common = root / 'tools/bazel/common.bazelrc'
            common.parent.mkdir(parents=True)
            common.write_text('try-import %workspace%/.bazelrc.user\n')
            fake = root / 'bazel'
            fake.write_text('#!/usr/bin/env python3\n'
                'import json, pathlib, sys\n'
                'args = sys.argv[1:]\n'
                'rc = next(arg.split("=", 1)[1] for arg in args if arg.startswith("--bazelrc="))\n'
                'print(json.dumps({"args": args, "rc": pathlib.Path(rc).read_text()}))\n')
            fake.chmod(0o755)
            env = dict(os.environ, HOME=str(home), BAZEL_REAL=str(fake),
                       SONIC_BAZEL_REGISTRY_REVISION='a' * 40)
            records = []
            for command in (['info', 'server_pid'], ['shutdown']):
                result = subprocess.run(['bash', str(wrapper), '--host_jvm_args=-Xmx512m', *command],
                    cwd=root, env=env, text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                record = json.loads(result.stdout)
                args = record['args']
                self.assertEqual(args[:4], ['--host_jvm_args=-Xmx512m', '--nosystem_rc',
                                          '--nohome_rc', '--noworkspace_rc'])
                self.assertTrue(args[4].startswith('--bazelrc='))
                self.assertFalse(Path(args[4].split('=', 1)[1]).exists())
                self.assertNotIn('startup', record['rc'])
                self.assertIn('--disk_cache=' + str(root / 'cache'), record['rc'])
                records.append(record)
            self.assertEqual(records[0]['rc'], records[1]['rc'])
            self.assertEqual(records[1]['args'][5:], ['shutdown'])


if __name__ == '__main__':
    unittest.main()
