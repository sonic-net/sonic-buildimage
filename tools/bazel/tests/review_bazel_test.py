"""Exercise review command forwarding and setup-bazel's persistent caches."""

import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest

from tools.bazel.ci import review_bazel_rc as rc

ROOT = Path(__file__).resolve().parents[3]
WRAPPER = ROOT / 'tools/bazel/ci/review_bazel.sh'
REVISION = '1234567890abcdef1234567890abcdef12345678'


class ReviewBazelTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='sonic-review-test-')
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.workspace = self.directory / 'source checkout'
        (self.workspace / 'tools/bazel').mkdir(parents=True)
        self.common = (ROOT / 'tools/bazel/common.bazelrc').read_text()
        (self.workspace / 'tools/bazel/common.bazelrc').write_text(self.common)
        self.home = self.directory / 'user configuration'
        self.home.mkdir()
        self.record = self.directory / 'called.json'
        self.fake = self.directory / 'bazel executable'
        self.fake.write_text('''#!/usr/bin/env python3
import json, os, pathlib, sys
args=sys.argv[1:]
rc=next((x.split('=',1)[1] for x in args if x.startswith('--bazelrc=')),None)
pathlib.Path(os.environ['REVIEW_TEST_RECORD']).write_text(json.dumps({
    'arguments':args,'configuration':pathlib.Path(rc).read_text() if rc else '', 'rc_path':rc}))
sys.stdout.write(os.environ.get('REVIEW_TEST_STDOUT',''))
sys.stderr.write(os.environ.get('REVIEW_TEST_STDERR',''))
sys.exit(int(os.environ.get('REVIEW_TEST_EXIT','0')))
''')
        self.fake.chmod(0o755)

    def invoke(self, arguments, **changes):
        environment = dict(os.environ, HOME=str(self.home), SONIC_BAZEL_REGISTRY_REVISION=REVISION,
                           BAZEL_REAL=str(self.fake), REVIEW_TEST_RECORD=str(self.record))
        environment.update(changes)
        return subprocess.run([str(WRAPPER), *arguments], cwd=self.workspace,
                              env=environment, capture_output=True, text=True)

    def test_actual_command_and_startup_argument_forwarding(self):
        for command in ('build', 'aquery', 'cquery', 'test', 'query', 'fetch', 'info', 'mod'):
            with self.subTest(command=command):
                startup = ['--batch', '--output_base', str(self.directory / 'cached output'),
                           '--host_jvm_args=-Xmx3g', '--max_idle_secs=90']
                arguments = {'info': ['release'], 'mod': ['graph']}.get(command, ['//owner:runtime'])
                if command in ('build', 'test'):
                    arguments = ['--jobs=2', '--remote_upload_local_results=true',
                                 '--disk_cache=' + str(self.directory / 'action cache'), *arguments]
                result = self.invoke([*startup, command, *arguments])
                self.assertEqual(result.returncode, 0, result.stderr)
                record = json.loads(self.record.read_text())
                actual = record['arguments']
                self.assertEqual(actual[:len(startup)], startup)
                # Build-family commands are read-only even if a caller enables uploads.
                command_args = actual[actual.index(command) + 1:]
                if command in ('build', 'aquery', 'cquery', 'test', 'fetch', 'info'):
                    self.assertEqual(command_args.pop(), '--remote_upload_local_results=false')
                self.assertEqual(command_args[-len(arguments):], arguments)
                self.assertEqual([x for x in actual if x.startswith('--registry=')], [
                    '--registry=https://raw.githubusercontent.com/securely1g/sonic-bazel-registry/' + REVISION,
                    '--registry=https://bcr.bazel.build'])
                self.assertEqual(actual[actual.index(command)-1], '--bazelrc=' + record['rc_path'])
                self.assertIn('--nohome_rc', actual)
                self.assertNotIn('try-import', record['configuration'])
                self.assertIn('--host_platform=', record['configuration'])
                self.assertFalse(Path(record['rc_path']).exists(), 'temporary rc leaked')

    def test_standalone_wrapper_keeps_canonical_and_base_registry_selection(self):
        wrapper = self.directory / 'standalone-bazel'
        shutil.copy2(WRAPPER.with_name('libteam_draft_bazel.sh'), wrapper)
        (self.directory / 'bazel').symlink_to(self.fake)
        for base_revision in ('', 'b' * 40):
            with self.subTest(base_revision=base_revision):
                env = dict(os.environ, PATH=str(self.directory) + os.pathsep + os.environ['PATH'],
                           REVIEW_TEST_RECORD=str(self.record), BAZEL_REAL='/unused/worker/launcher',
                           SONIC_BAZEL_REGISTRY_REVISION=REVISION, SONIC_BASE_REVIEW_REGISTRY=base_revision)
                result = subprocess.run([str(wrapper), 'build', '//owner:runtime'], cwd=self.workspace,
                                        env=env, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                arguments = json.loads(self.record.read_text())['arguments']
                self.assertEqual([arg for arg in arguments if arg.startswith('--registry=')], [
                    '--registry=https://raw.githubusercontent.com/securely1g/sonic-bazel-registry/' +
                    (base_revision or 'main'), '--registry=https://bcr.bazel.build'])

    def test_read_only_flag_precedes_end_of_options(self):
        result = self.invoke(['build', '--remote_upload_local_results=true', '--', '//owner:runtime'])
        self.assertEqual(result.returncode, 0, result.stderr)
        arguments = json.loads(self.record.read_text())['arguments']
        self.assertEqual(arguments[-3:], ['--remote_upload_local_results=false', '--', '//owner:runtime'])

    def test_home_and_local_cache_values_survive(self):
        (self.home / '.bazelrc').write_text('''build --disk_cache='/persistent/action cache'
common --repository_cache=/persistent/downloads
common --registry=https://example.invalid/catalog
build --override_module=owner=/not-accepted
''')
        (self.workspace / '.bazelrc.user').write_text('''common --distdir '/verified source downloads'
common --repository_cache=/existing/local-downloads
common --experimental_repository_cache_hardlinks
common --registry=https://example.invalid/second-catalog
try-import /not-imported
''')
        result = rc.configuration(self.workspace, self.home)
        parsed = [shlex.split(line, comments=True) for line in result.splitlines()]
        self.assertIn(['build', '--disk_cache=/persistent/action cache'], parsed)
        self.assertIn(['common', '--repository_cache=/persistent/downloads'], parsed)
        self.assertLess(parsed.index(['common', '--repository_cache=/persistent/downloads']),
                        parsed.index(['common', '--repository_cache=/existing/local-downloads']))
        self.assertIn(['common', '--distdir=/verified source downloads'], parsed)
        self.assertIn(['common', '--experimental_repository_cache_hardlinks'], parsed)
        self.assertNotIn('--registry', result)
        self.assertNotIn('--override_module', result)
        self.assertNotIn('try-import', result)
        self.assertEqual((self.workspace / 'tools/bazel/common.bazelrc').read_text(), self.common)

    def test_worker_cache_settings_preserve_reads_and_disable_uploads(self):
        from tools.bazel.ci.preserve_setup_bazel_cache import preserve

        (self.home / '.bazelrc').write_text(
            'common --repository_cache=/worker/downloads\nbuild --disk_cache=/worker/actions\n')
        preserve(self.workspace, self.home, private_remote_cache=True)
        user_rc = self.workspace / '.bazelrc.user'
        with user_rc.open('a') as out:
            out.write('common --remote_upload_local_results=true\n')
        result = self.invoke(['build', '--remote_upload_local_results=true', '//owner:runtime'])
        self.assertEqual(result.returncode, 0, result.stderr)
        record = json.loads(self.record.read_text())
        for line in user_rc.read_text().splitlines():
            if line and '--remote_upload_local_results' not in line:
                self.assertIn(line, record['configuration'])
        self.assertNotIn('--remote_upload_local_results=true', record['configuration'])
        self.assertEqual(record['arguments'][-1], '--remote_upload_local_results=false')

    def test_verified_downloader_configuration_survives(self):
        spellings = ('--downloader_config', '--experimental_downloader_config')
        for home_option in spellings:
            for local_option in spellings:
                for separator in ('=', ' '):
                    with self.subTest(home=home_option, local=local_option, separator=separator):
                        (self.home / '.bazelrc').write_text(
                            'common ' + home_option + '=/home/source-mirrors.cfg\n')
                        (self.workspace / '.bazelrc.user').write_text(
                            'common ' + local_option + separator + "'/verified mirrors/source.cfg'\n"
                            'common --registry=https://example.invalid/not-a-mirror\n')
                        result = self.invoke(['build', '//owner:runtime'], HOME=str(self.home))
                        self.assertEqual(result.returncode, 0, result.stderr)
                        record = json.loads(self.record.read_text())
                        parsed = [shlex.split(line, comments=True)
                                  for line in record['configuration'].splitlines()]
                        values = [line for line in parsed if len(line) == 2 and
                                  line[1].split('=', 1)[0] in spellings]
                        self.assertEqual(values, [
                            ['common', '--downloader_config=tools/bazel/debian_mirrors.cfg'],
                            ['common', home_option + '=/home/source-mirrors.cfg'],
                            ['common', local_option + '=/verified mirrors/source.cfg']])
                        self.assertNotIn('example.invalid', record['configuration'])
                        self.assertEqual(len([arg for arg in record['arguments']
                                              if arg.startswith('--registry=')]), 2)
        for option in spellings:
            with self.subTest(missing_value=option):
                (self.workspace / '.bazelrc.user').write_text('common ' + option + '\n')
                with self.assertRaisesRegex(ValueError, 'missing cache option value'):
                    rc.configuration(self.workspace, self.home)

    def test_hardlink_choices_keep_home_then_local_precedence(self):
        positive = '--experimental_repository_cache_hardlinks'
        negative = '--noexperimental_repository_cache_hardlinks'
        for home_option, local_option in ((positive, negative), (negative, positive)):
            with self.subTest(home=home_option, local=local_option):
                (self.home / '.bazelrc').write_text('common ' + home_option + '\n')
                (self.workspace / '.bazelrc.user').write_text(
                    'common ' + local_option + ' --repository_cache=/verified/downloads\n'
                    'build --override_module=owner=/not-accepted\n')
                result = rc.configuration(self.workspace, self.home)
                parsed = [shlex.split(line, comments=True) for line in result.splitlines()]
                values = [line for line in parsed if len(line) == 2 and
                          line[1] in (positive, negative)]
                self.assertEqual(values, [['common', home_option], ['common', local_option]])
                self.assertIn(['common', '--repository_cache=/verified/downloads'], parsed)
                self.assertNotIn('--override_module', result)

    def test_missing_or_nonimmutable_catalog_fails_before_bazel(self):
        for revision in ('', 'main', '../main', 'a'*39, 'a'*41, 'A'*40):
            with self.subTest(revision=revision):
                result = self.invoke(['build', '//owner:runtime'], SONIC_BAZEL_REGISTRY_REVISION=revision)
                self.assertEqual(result.returncode, 2)
                self.assertFalse(self.record.exists())

    def test_extra_registries_and_rc_overrides_are_rejected(self):
        for arguments in (['--bazelrc=/tmp/other', 'build'], ['--home_rc', 'build'],
                          ['--ignore_all_rc_files', 'build'], ['build', '--registry=https://example.invalid'],
                          ['build', '--registry', 'https://example.invalid'], ['clean'], []):
            with self.subTest(arguments=arguments):
                self.assertEqual(self.invoke(arguments).returncode, 2)
                self.assertFalse(self.record.exists())

    def test_bazel_failure_and_version_are_preserved(self):
        result = self.invoke(['build', '//owner:missing'], REVIEW_TEST_EXIT='37')
        self.assertEqual(result.returncode, 37)
        self.assertFalse(Path(json.loads(self.record.read_text())['rc_path']).exists())
        result = self.invoke(['--version'], SONIC_BAZEL_REGISTRY_REVISION='')
        self.assertEqual(result.returncode, 0)
        self.assertEqual(json.loads(self.record.read_text())['arguments'], ['version', '--gnu_format'])

        from tools.bazel.ci import source_image_worker as worker
        launcher = self.directory / 'resource launcher'
        profile = {'jobs': 2, 'cpu': 2, 'scheduler_memory_mb': 8000, 'jvm_heap_gib': 3}
        base = self.directory / 'private output base'
        launcher.write_text(worker.launcher_text(str(self.fake), base, profile))
        launcher.chmod(0o755)
        self.fake.write_text(self.fake.read_text().replace(
            "sys.exit(int(os.environ.get('REVIEW_TEST_EXIT','0')))",
            "if args[-2:] == ['version', '--gnu_format']: print('bazel 8.5.1')\n"
            "sys.exit(int(os.environ.get('REVIEW_TEST_EXIT','0')))"))
        result = self.invoke(['--version'], BAZEL_REAL=str(launcher),
                             SONIC_BAZEL_REGISTRY_REVISION='')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.splitlines(), ['bazel 8.5.1'])
        self.assertEqual(json.loads(self.record.read_text())['arguments'], [
            '--output_base=' + str(base), '--host_jvm_args=-Xmx3g',
            '--host_jvm_args=-Djavax.net.ssl.trustStore=/etc/ssl/certs/java/cacerts',
            'version', '--gnu_format'])

    def test_configuration_drift_fails_closed(self):
        common = self.workspace / 'tools/bazel/common.bazelrc'
        for extra in ('common --registry=https://example.invalid\n', 'import /other/config\n'):
            common.write_text(self.common + extra)
            with self.assertRaises(ValueError):
                rc.configuration(self.workspace, self.home)
        common.write_text(self.common.replace('try-import %workspace%/.bazelrc.user', ''))
        with self.assertRaises(ValueError):
            rc.configuration(self.workspace, self.home)

    def test_version_removes_only_the_leading_bazelisk_banner(self):
        cases = (
            ('plain', 'bazel 8.5.1\n', 'bazel 8.5.1\n', 0),
            ('bazelisk', 'Bazelisk v1.29.0\nbazel 8.5.1\n', 'bazel 8.5.1\n', 0),
            ('later-banner', 'bazel 8.5.1\nBazelisk later output\n',
             'bazel 8.5.1\nBazelisk later output\n', 0),
            ('other-output', 'warning\nBazelisk v1.29.0\nbazel 8.5.1\n',
             'warning\nBazelisk v1.29.0\nbazel 8.5.1\n', 0),
            ('backend-failure', 'Bazelisk v1.29.0\nbackend output\n',
             'backend output\n', 37),
        )
        for name, output, expected, exit_code in cases:
            with self.subTest(name=name):
                result = self.invoke(
                    ['--version'], SONIC_BAZEL_REGISTRY_REVISION='',
                    REVIEW_TEST_STDOUT=output, REVIEW_TEST_STDERR='backend diagnostic\n',
                    REVIEW_TEST_EXIT=str(exit_code))
                self.assertEqual(result.returncode, exit_code)
                self.assertEqual(result.stdout, expected)
                self.assertEqual(result.stderr, 'backend diagnostic\n')
                self.assertEqual(json.loads(self.record.read_text())['arguments'],
                                 ['version', '--gnu_format'])


if __name__ == '__main__':
    unittest.main()
