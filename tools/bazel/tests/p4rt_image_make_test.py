"""Exercise real P4RT Make selection and the explicit owner handoff, without builds."""

import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
VALUES = dict(BLDENV='trixie', CONFIGURED_PLATFORM='vs', CONFIGURED_ARCH='amd64', INCLUDE_FIPS='y',
              DBG_IMAGE_MARK='dbg', BUILD_WITH_BAZEL_WHEN_AVAILABLE='y', INCLUDE_P4RT='y',
              DOCKERS_PATH='dockers', TARGET_PATH='target', SONIC_P4RT='p4rt.deb',
              SONIC_P4RT_DBG='p4rt-dbg.deb', LIBSWSSCOMMON_DBG='common-dbg.deb',
              LIBSAIREDIS_DBG='sairedis-dbg.deb', DOCKER_CONFIG_ENGINE_TRIXIE='docker-config-engine-trixie.gz')


class P4rtMakeTest(unittest.TestCase):
    def evaluate(self, **changes):
        values = {**VALUES, **changes}
        variables = ('SONIC_BAZEL_DOCKER_IMAGES', 'SONIC_BAZEL_DBG_DOCKER_IMAGES',
                     'docker-sonic-p4rt.gz_BAZEL_TARGET', 'docker-sonic-p4rt-$(DBG_IMAGE_MARK).gz_BAZEL_TARGET',
                     'docker-sonic-p4rt-$(DBG_IMAGE_MARK).gz_BAZEL_DEPENDS',
                     'docker-sonic-p4rt.gz_DEPENDS', 'docker-sonic-p4rt.gz_DBG_DEPENDS',
                     'docker-sonic-p4rt.gz_LOAD_DOCKERS', 'docker-sonic-p4rt.gz_RUN_OPT',
                     'docker-sonic-p4rt.gz_WARM_SHUTDOWN_BEFORE', 'docker-sonic-p4rt.gz_FAST_SHUTDOWN_BEFORE',
                     'SONIC_INSTALL_DOCKER_IMAGES', 'SONIC_INSTALL_DOCKER_DBG_IMAGES')
        recipe = 'include rules/docker-p4rt.mk\n.PHONY: result\nresult:\n\t@printf \'%s\\n\' '
        recipe += ' '.join("'$(" + name + ")'" for name in variables) + '\n'
        output = subprocess.check_output(['make', '--no-print-directory', '-f', '-', 'result',
            *[key + '=' + value for key, value in values.items()]], input=recipe, cwd=ROOT, text=True)
        return [line.strip() for line in output.splitlines()]

    def test_native_selection_has_all_manifests_without_owner_prep(self):
        rows = self.evaluate()
        self.assertEqual(rows[:4], ['docker-sonic-p4rt.gz', 'docker-sonic-p4rt-dbg.gz',
            '//dockers/docker-sonic-p4rt:docker-sonic-p4rt.gz', '//dockers/docker-sonic-p4rt:docker-sonic-p4rt-dbg.gz'])
        self.assertEqual(set(rows[4].split()), {
            'target/bazel-manifests/' + name + '/manifest.json' for name in
            ('docker-base-trixie', 'docker-config-engine-trixie', 'docker-sonic-p4rt', 'docker-sonic-p4rt-dbg')})

    def test_legacy_packages_mounts_shutdown_and_install_conditions_are_preserved(self):
        rows = self.evaluate()
        self.assertEqual(rows[5:8], ['p4rt.deb', 'p4rt-dbg.deb common-dbg.deb sairedis-dbg.deb', 'docker-config-engine-trixie.gz'])
        self.assertEqual(rows[8].split(), ['-t', '-v', '/etc/sonic:/etc/sonic:rw', '-v',
            '/etc/localtime:/etc/localtime:ro', '-v', '/zmq_swss:/zmq_swss:rw'])
        self.assertEqual(rows[9:], ['swss', 'swss', 'docker-sonic-p4rt.gz', 'docker-sonic-p4rt-dbg.gz'])
        self.assertEqual(self.evaluate(INCLUDE_P4RT='n')[-2:], ['', ''])

    def test_unsupported_profiles_keep_legacy_producers(self):
        baseline = self.evaluate()
        for changes in ({'BLDENV': 'bookworm'}, {'CONFIGURED_ARCH': 'arm64'}, {'CONFIGURED_ARCH': 'armhf'},
                        {'CONFIGURED_PLATFORM': 'broadcom'}, {'BUILD_WITH_BAZEL_WHEN_AVAILABLE': 'n'},
                        {'INCLUDE_FIPS': 'n'}, {'ENABLE_ASAN': 'y'}, {'CROSS_BUILD_ENVIRON': 'y'},
                        {'MULTIARCH_QEMU_ENVIRON': 'y'}, {'DBG_IMAGE_MARK': 'debug'}):
            with self.subTest(changes=changes):
                rows = self.evaluate(**changes)
                self.assertEqual(rows[:5], ['', '', '', '', ''])
                self.assertEqual(rows[5:12], baseline[5:12])
                self.assertEqual(rows[12], 'docker-sonic-p4rt-' + changes.get('DBG_IMAGE_MARK', 'dbg') + '.gz')

    def test_parallel_archive_prerequisites_only_require_manifests(self):
        with tempfile.TemporaryDirectory(prefix='p4rt-make-') as temporary:
            root = Path(temporary)
            recorder = root / 'python3'
            calls = root / 'calls.jsonl'
            recorder.write_text('#!/usr/bin/python3\nimport json,os,sys\n'
                "with open(os.environ['P4RT_TEST_CALLS'],'a') as f:f.write(json.dumps(sys.argv[1:])+'\\n')\n")
            recorder.chmod(0o755)
            manifests = root / 'manifests'
            for name in ('docker-base-trixie', 'docker-config-engine-trixie', 'docker-sonic-p4rt', 'docker-sonic-p4rt-dbg'):
                path = manifests / 'bazel-manifests' / name / 'manifest.json'
                path.parent.mkdir(parents=True)
                path.write_text('{}\n')
            recipe = ('include rules/docker-p4rt.mk\n.PHONY: all release debug\nall: release debug\n'
                      'release: $(docker-sonic-p4rt.gz_BAZEL_DEPENDS)\n'
                      'debug: $(docker-sonic-p4rt-dbg.gz_BAZEL_DEPENDS)\n')
            values = {**VALUES, 'TARGET_PATH': str(manifests)}
            subprocess.run(['make', '--no-print-directory', '-j2', '-f', '-', 'all',
                *[key + '=' + value for key, value in values.items()]], input=recipe, cwd=ROOT, text=True,
                env={**os.environ, 'PATH': str(root) + ':' + os.environ['PATH'], 'P4RT_TEST_CALLS': str(calls)},
                check=True, capture_output=True)
            self.assertFalse(calls.exists())

    def test_metadata_only_manifest_generation_never_prepares_the_owner(self):
        with tempfile.TemporaryDirectory(prefix='p4rt-manifests-') as temporary:
            values = {**VALUES, 'TARGET_PATH': temporary,
                      'MANIFEST_METADATA': 'rules/docker-p4rt.mk'}
            result = subprocess.run(['make', '--no-print-directory', '-f', 'tools/bazel/prepare_manifests.mk',
                'manifests', *[key + '=' + value for key, value in values.items()]], cwd=ROOT,
                text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertNotIn('prepare-owner', result.stdout + result.stderr)
            for name in ('docker-sonic-p4rt', 'docker-sonic-p4rt-dbg'):
                self.assertTrue((Path(temporary) / 'bazel-manifests' / name / 'manifest.json').is_file())


if __name__ == '__main__':
    unittest.main()
