"""Verify native Bazel 8 P4RT root options, manifests, and clean-checkout checks."""

import os
from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from tools.bazel.ci import p4rt_image as image


class RuntimeTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def test_make_cache_options_preserve_native_platform_and_quoted_arguments(self):
        with patch.dict(os.environ, {'BAZEL_CONTAINER_CACHE_DIR': str(self.root / 'cache'),
                'BAZEL_CONTAINER_ARGS': '--jobs=1 --define="fixture=value with spaces" --platforms=other'}, clear=True):
            options = image.root_options()
        self.assertNotIn('--jobs=2', image.OPTIONS)
        self.assertEqual(image.TEST_SCOPE, '@sonic_pins//p4rt_app:runtime_package_test')
        self.assertIn('--repository_cache=' + str(self.root / 'cache/repository_cache'), options)
        self.assertIn('--disk_cache=' + str(self.root / 'cache/disk_cache'), options)
        self.assertIn('--define=fixture=value with spaces', options)
        self.assertEqual([value for value in options if value.startswith('--jobs=')][-1], '--jobs=1')
        self.assertEqual([value for value in options if value.startswith('--platforms=')][-1], image.OPTIONS[-2])

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

    def test_clean_checkout_renders_manifests_before_the_first_graph_audit(self):
        events = []
        args = SimpleNamespace(directory=self.root / 'artifacts', owner_source=self.root / 'owner',
                               owner_revision='a' * 40, bazel='root-bazel', bazel_arg=[])

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
                patch.object(image, 'verify_owner_source'), \
                patch.object(image.command_log, 'execute', side_effect=execute), \
                patch.object(image.bazel_commands, 'audit_targets', side_effect=audit):
            with self.assertRaisesRegex(RuntimeError, 'stop before any compilation'):
                image.build_images(args)
        self.assertEqual(events, ['manifests', 'audit'])

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
