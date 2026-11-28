"""Test native Bazel 8 P4RT build/owner-test lifecycle and review launcher checks."""
import json
import os
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

    def test_verify_owner_source_requires_clean_matching_head(self):
        owner = self.root / 'owner'
        owner.mkdir()

        def git(*args):
            return subprocess.check_output(['git', *args], cwd=owner, text=True).strip()

        git('init', '-q')
        (owner / 'BUILD.bazel').write_text('# fixture\n')
        git('add', 'BUILD.bazel')
        git('-c', 'user.name=Fixture', '-c', 'user.email=test@example.org',
            '-c', 'commit.gpgsign=false', 'commit', '-qm', 'init')
        revision = git('rev-parse', 'HEAD')
        self.assertEqual(image.verify_owner_source(owner, revision), revision)
        with self.assertRaisesRegex(ValueError, 'differs from expected'):
            image.verify_owner_source(owner, '0' * 40)
        (owner / 'untracked.txt').write_text('dirty\n')
        with self.assertRaisesRegex(ValueError, 'untracked files'):
            image.verify_owner_source(owner, revision)

    def test_owner_tests_requires_matching_built_receipt(self):
        directory = self.root / 'artifacts'
        directory.mkdir()
        (directory / 'build-receipt.json').write_text(json.dumps({
            'status': 'failed', 'revision': 'a' * 40, 'owner_revision': 'b' * 40,
        }))
        args = SimpleNamespace(directory=directory, bazel='bazel', bazel_arg=[],
                               owner_source=self.root / 'owner', owner_revision='b' * 40)
        with patch.object(image, 'native'), patch.object(image, 'source_identity', return_value='a' * 40):
            with self.assertRaisesRegex(ValueError, 'Built P4RT receipt'):
                image.owner_tests(args)


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
