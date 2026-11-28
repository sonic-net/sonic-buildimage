"""Exercise the root kernel package command across a trusted build and a fresh job."""

import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import unittest
from unittest.mock import Mock, patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import kernel_artifacts
from tools.bazel.kernel import packages, prepare, remote_bundle, root_request, root_resolution
from tools.bazel.tests import kernel_test, kernel_cache_root_resolution_test as proof


class RootPackagesTest(unittest.TestCase):
    def setUp(self):
        fixture = kernel_test.KernelBundleTest()
        fixture.setUp()
        self.addCleanup(fixture.doCleanups)
        self.fixture = fixture
        self.workspace, self.directory = fixture.root, fixture.bundle
        self.architecture = fixture.architecture
        self.lock, self.responses = proof.resolution_fixture(self.workspace)
        self.enterContext(proof.fixture_policy(self.workspace, self.lock))
        self.downloads = []
        self.blobs = {}
        self.log = self.workspace / 'bazel-calls'
        outputs = self.workspace / 'bazel-outs'
        outputs.mkdir()
        paths = []
        for name in fixture.packages:
            # The registry producer emits a directory artifact for each sonic_deb.
            parent = outputs / (name + 's')
            parent.mkdir()
            (self.directory / name).rename(parent / name)
            paths.append(str(parent))
        (self.directory / packages.MANIFEST).rename(outputs / packages.MANIFEST)
        paths.append(str(outputs / packages.MANIFEST))
        shutil.rmtree(self.directory)
        self.bazel = self.workspace / 'fixture-bazel'
        self.bazel.write_text(
            '#!' + sys.executable + '\nimport json, sys\nfrom pathlib import Path\n'
            "command = sys.argv[1]\nwith Path('bazel-calls').open('a') as log: print(command, file=log)\n"
            "Path(command + '-argv').write_text(json.dumps(sys.argv[2:]))\n"
            "if command == 'build': Path('MODULE.bazel.lock').write_bytes(Path('producer.lock').read_bytes())\n"
            'elif command == "cquery": print(' + repr('\n'.join(paths)) + ')\n'
            'else: raise ValueError(command)\n')
        self.bazel.chmod(0o755)
        (self.workspace / 'producer.lock').write_bytes(self.lock)

        def download(digest, size, dest):
            self.downloads.append(dest.name)
            dest.write_bytes(self.blobs[digest, size])

        def upload(digest, size, src):
            data = src.read_bytes()
            self.assertEqual((digest, size), (hashlib.sha256(data).hexdigest(), len(data)))
            self.blobs[digest, size] = data

        cache = Mock(download=download, upload=upload, missing=lambda items: set(items) - self.blobs.keys())
        self.enterContext(patch.object(remote_bundle.cache_transport, 'BlobCache',
                                       return_value=contextlib.nullcontext(cache)))
        self.enterContext(patch.object(root_request, 'request_key', return_value='a' * 64))
        self.enterContext(patch.dict(os.environ, {'SONIC_VS_KERNEL_REMOTE_REUSE': '1', 'BAZEL': str(self.bazel),
                                                  'BAZEL_CONTAINER_ARGS': ''}))
        real_validate = root_resolution.validate
        self.enterContext(patch.object(root_resolution, 'validate',
                                      side_effect=lambda data, workspace: real_validate(
                                          data, workspace, fetch=self.responses.__getitem__, environment={})))
        self.build(upload=True)
        self.assertEqual(self.log.read_text().splitlines(), ['build', 'cquery'])
        self.original = {p.name: p.read_bytes() for p in self.directory.iterdir() if not p.name.startswith('.')}
        shutil.rmtree(self.directory)
        self.log.unlink()
        (self.workspace / 'MODULE.bazel.lock').unlink()
        self.index = self.workspace / remote_bundle.INPUT_INDEX
        self.index.parent.mkdir(parents=True)
        (self.workspace / remote_bundle.OUTPUT_INDEX).rename(self.index)

    def build(self, upload=False, *, platform=None):
        args = ['packages.py', 'build', '--workspace', str(self.workspace), '--debs-directory', str(self.directory),
                '--architecture', self.architecture, '--remote-cache', 'grpc://fixture:9092']
        if platform is not None:
            args.extend(['--kernel-config', 'CONFIGURED_PLATFORM=' + platform])
        if upload:
            args.append('--upload-local-results')
        with patch.object(sys, 'argv', args):
            packages.main()

    def test_kernel_timing_covers_restore_and_fallback(self):
        start = '[ vs timing ] [ kernel-prep ] [ start ]'
        end = '[ vs timing ] [ kernel-prep ] [ end ]'
        log = io.StringIO()
        with patch.dict(os.environ, {'SONIC_VS_IMAGE_TIMINGS': '1'}), contextlib.redirect_stderr(log):
            self.build()
            self.assertFalse(self.log.exists())
            self.index.unlink()
            self.build(platform='vs')
            self.assertEqual(self.log.read_text().splitlines(), ['build', 'cquery'])
            with patch.dict(os.environ, {'BAZEL': str(self.workspace / 'missing-bazel')}):
                with self.assertRaises(FileNotFoundError):
                    self.build(platform='vs')
            self.assertEqual(log.getvalue().splitlines(), [start, end, start, end, start])

    def test_kernel_timing_depends_only_on_the_timing_flag(self):
        markers = ['[ vs timing ] [ kernel-prep ] [ start ]', '[ vs timing ] [ kernel-prep ] [ end ]']
        for platform in (None, 'broadcom', 'vs'):
            for timings in (None, '0', '1'):
                with self.subTest(platform=platform, timings=timings), patch.dict(os.environ), \
                        contextlib.redirect_stderr(io.StringIO()) as log, patch.object(packages, 'build_packages'):
                    if timings is None:
                        os.environ.pop('SONIC_VS_IMAGE_TIMINGS', None)
                    else:
                        os.environ['SONIC_VS_IMAGE_TIMINGS'] = timings
                    self.build(platform=platform)
                    self.assertEqual(log.getvalue().splitlines(), markers if timings == '1' else [])

    def test_registry_audit_standalone_and_make_share_the_same_configured_target(self):
        """Both native graphs must address the same five DEBs on the first Make call."""
        for architecture in ('amd64', 'arm64'):
            args = ['packages.py', 'build', '--workspace', str(self.workspace), '--debs-directory',
                    str(self.directory), '--architecture', architecture]
            explicit = prepare.effective_config(self.workspace, architecture)
            explicit.update(CROSS_BUILD_ENVIRON='', MULTIARCH_QEMU_ENVIRON='', SECURE_UPGRADE_MODE='"no_sign"')
            make_flags = [arg for key, value in explicit.items() for arg in ('--kernel-config', f'{key}={value}')]
            machine = {'amd64': 'x86_64', 'arm64': 'aarch64'}[architecture]
            with self.subTest(architecture=architecture), patch.object(packages.os, 'uname', return_value=Mock(machine=machine)), \
                    patch.object(packages, 'build_packages') as build:
                for extra in ([], make_flags):
                    with patch.object(sys, 'argv', [*args, *extra]):
                        packages.main()
                initial, repeated = [call.args for call in build.call_args_list]
                self.assertEqual(initial[3], repeated[3])
                self.assertEqual(initial[5], repeated[5])
                output = io.StringIO()
                with patch.object(sys, 'argv', [args[0], 'bazel-config', *args[2:]]), contextlib.redirect_stdout(output):
                    packages.main()
                self.assertIn('--@sonic_linux_kernel//tools/bazel:kernel_config=' + output.getvalue().strip(),
                              initial[5])
                for key, value in (('KERNEL_FEATURESET', 'another'), ('SECURE_UPGRADE_MODE', 'unsupported'),
                                   ('SECURE_UPGRADE_KERNEL_CAFILE', '/key.pem'), ('EXTERNAL_KERNEL_PATCH_URL', 'alternate'),
                                   ('EXTERNAL_KERNEL_PATCH_LOC', '/patches')):
                    with patch.object(sys, 'argv', [*args, '--kernel-config', key + '=' + value]):
                        packages.main()
                    # The producer sees the requested non-default value and can reject unsupported settings.
                    self.assertEqual(build.call_args.args[3][key], value)
                    self.assertNotEqual(initial[5], build.call_args.args[5])

    def test_cross_build_keeps_the_owner_default_and_explicit_request(self):
        """AMD64 targeting ARM64 selects cross build unless Make explicitly requests otherwise."""
        args = ['packages.py', 'bazel-config', '--workspace', str(self.workspace), '--architecture', 'arm64']
        with patch.object(packages.os, 'uname', return_value=Mock(machine='x86_64')):
            for extra, expected in (([], 'y'), (['--kernel-config', 'CROSS_BUILD_ENVIRON='], 'n')):
                output = io.StringIO()
                with patch.object(sys, 'argv', [*args, *extra]), contextlib.redirect_stdout(output):
                    packages.main()
                self.assertEqual(json.loads(output.getvalue())['CROSS_BUILD_ENVIRON'], expected)

    def test_only_requested_cache_fallback_options_reach_build_and_cquery(self):
        """Make can recover from cache outages without changing kernel flags or cache permissions."""
        self.index.unlink()
        original = {command: json.loads((self.workspace / (command + '-argv')).read_text())
                    for command in ('build', 'cquery')}
        local = '--remote_local_fallback'
        compatible = '--incompatible_remote_local_fallback_for_remote_cache'
        unrelated = '--config=other --define="quoted --remote_local_fallback" --remote_upload_local_results=false'
        for requested in ([], [local], [local, compatible], [local, '--noremote_local_fallback', compatible + '=false']):
            with self.subTest(requested=requested), patch.dict(os.environ, {
                'BAZEL_CONTAINER_ARGS': ' '.join([unrelated, *requested]),
            }):
                self.build(upload=True)
                for command, baseline in original.items():
                    argv = json.loads((self.workspace / (command + '-argv')).read_text())
                    self.assertEqual([arg for arg in argv if arg in requested], requested)
                    self.assertEqual([arg for arg in argv if arg not in requested], baseline)

    def test_fresh_job_restores_original_five_debs_and_receipt_without_invoking_bazel(self):
        self.build()
        self.assertFalse(self.log.exists())
        self.assertEqual(self.original, {p.name: p.read_bytes() for p in self.directory.iterdir()
                                         if not p.name.startswith('.')})
        self.assertEqual(5, sum(name.endswith('.deb') for name in self.downloads))
        self.assertFalse((self.workspace / remote_bundle.OUTPUT_INDEX).exists())
        provenance = json.loads(self.original[packages.PROVENANCE])
        self.assertEqual(provenance['resolution']['lock_sha256'], hashlib.sha256(self.lock).hexdigest())

    def collect_handoff(self):
        handoff = self.workspace / 'vs-handoff'
        summary = kernel_artifacts.collect(self.workspace, None, handoff, debs_directory=self.directory, vs_handoff=True)
        output = self.workspace / 'tested-packages'
        self.assertEqual(summary, kernel_artifacts.collect(self.workspace, None, output, debs_directory=handoff,
                                                           from_vs_handoff=True))
        self.assertEqual(summary['resolution_sha256'], {'MODULE.bazel.lock': hashlib.sha256(self.lock).hexdigest()})
        self.assertEqual((handoff / packages.PROVENANCE).read_bytes(), (self.directory / packages.PROVENANCE).read_bytes())
        return handoff

    def test_restored_root_handoff_binds_verified_master_index_and_vs_revision(self):
        self.build()
        handoff = self.collect_handoff()
        self.assertFalse(self.log.exists())
        restored = self.workspace / remote_bundle.RESTORED_INDEX
        self.assertEqual(restored.read_bytes(), self.index.read_bytes())
        self.index.write_text('{}')
        with self.assertRaisesRegex(ValueError, 'trusted master index'):
            kernel_artifacts.collect(self.workspace, None, self.workspace / 'untrusted', debs_directory=self.directory,
                                     vs_handoff=True)
        receipt = handoff / kernel_artifacts.VS_RECEIPT
        value = json.loads(receipt.read_bytes())
        value['vs_revision'] = '0' * 40
        receipt.write_text(json.dumps(value))
        with self.assertRaisesRegex(ValueError, 'kernel handoff differs'):
            kernel_artifacts.collect(self.workspace, None, self.workspace / 'wrong-revision', debs_directory=handoff,
                                     from_vs_handoff=True)

    def test_make_preserves_verified_same_run_handoff_without_bazel(self):
        self.build()
        for name in ('slave.mk', 'rules/functions'):
            shutil.copyfile(ROOT / name, self.workspace / name)
        subprocess.run(['git', 'add', 'MODULE.bazel', 'slave.mk', 'rules/functions'],
                       cwd=self.workspace, check=True)
        self.fixture.commit(self.workspace, 'Track the VS inputs')
        handoff = self.collect_handoff()
        receipt = handoff / kernel_artifacts.VS_RECEIPT
        original = receipt.read_bytes()
        digest = hashlib.sha256(original).hexdigest()
        environment = dict(os.environ, SONIC_BAZEL_KERNEL_VS_HANDOFF_SHA256=digest, PYTHONPATH=str(ROOT))
        smoke = subprocess.run([sys.executable, '-B', str(ROOT / 'tools/bazel/ci/kernel_make_smoke.py'),
                                '--architecture', self.architecture, '--debs-directory', str(handoff)],
                               cwd=self.workspace, env=environment, text=True, capture_output=True)
        self.assertEqual(smoke.returncode, 0, smoke.stdout + smoke.stderr)
        self.assertEqual(smoke.stdout.count('Make consumed the verified same-run VS kernel DEBs.'), 4)
        self.assertFalse(self.log.exists())

        argv = ['packages.py', 'build', '--workspace', str(self.workspace), '--debs-directory',
                str(handoff), '--architecture', self.architecture, '--vs-handoff-sha256', digest]
        with patch.object(sys, 'argv', argv + ['--kernel-config', 'SECURE_UPGRADE_MODE=unsupported']):
            with self.assertRaisesRegex(ValueError, 'configuration differs'):
                packages.main()
        # A changed transitive registry pin in the checkout cannot authorize a
        # handoff built at the recorded VS revision, even if the git HEAD agrees.
        module = self.workspace / 'MODULE.bazel'
        tracked = module.read_bytes()
        module.write_bytes(tracked + b'\nbazel_dep(name="patchelf", version="0.18.1")\n')
        with patch.object(sys, 'argv', argv), self.assertRaisesRegex(ValueError, 'original checkout'):
            packages.main()
        module.write_bytes(tracked)
        provenance = handoff / packages.PROVENANCE
        record = json.loads(provenance.read_bytes())
        changed_lock = json.loads(self.lock)
        url = next(url for url in changed_lock['registryFileHashes']
                   if '/patchelf/' in url and url.endswith('/source.json'))
        changed_lock['registryFileHashes'][url] = '0' * 64
        changed_digest = hashlib.sha256(json.dumps(changed_lock).encode()).hexdigest()
        record['resolution']['lock_sha256'] = changed_digest
        provenance.write_text(json.dumps(record))
        with patch.object(sys, 'argv', argv), self.assertRaisesRegex(ValueError, 'handoff differs'):
            packages.main()
        # Rewriting the receipt to match tampered provenance must also fail.
        forged = json.loads(original)
        forged['summary']['provenance_sha256'] = prepare.sha256(provenance)
        forged['summary']['resolution_sha256']['MODULE.bazel.lock'] = changed_digest
        receipt.write_text(json.dumps(forged))
        with patch.object(sys, 'argv', argv), self.assertRaisesRegex(ValueError, 'receipt digest differs'):
            packages.main()
        self.assertFalse(self.log.exists())

    def test_plain_root_build_handoff_binds_lock_even_without_cache_publication(self):
        self.index.unlink()
        self.build()
        self.assertEqual(self.log.read_text().splitlines(), ['build', 'cquery'])
        (self.workspace / 'MODULE.bazel.lock').write_text('lock from another target')
        self.collect_handoff()
        lock = self.directory / prepare.RESOLUTION_DIRECTORY / hashlib.sha256(self.lock).hexdigest()
        lock.write_text('tampered retained lock')
        with self.assertRaisesRegex(ValueError, 'kernel dependency record differs'):
            kernel_artifacts.collect(self.workspace, None, self.workspace / 'corrupt', debs_directory=self.directory,
                                     vs_handoff=True)

    def test_transitive_change_or_lookup_failure_uses_bazel_before_fetching_debs(self):
        url = next(url for url in self.responses if '/patchelf/' in url and url.endswith('/source.json')
                   and self.responses[url] is not None)
        changed_source = json.loads(self.responses[url])
        changed_source['integrity'] = 'sha256-' + 'B' * 43 + '='
        for response in (json.dumps(changed_source).encode(), None):
            with self.subTest(response_missing=response is None):
                self.responses[url] = response
                self.build()
                self.assertEqual(self.log.read_text().splitlines(), ['build', 'cquery'])
                self.assertFalse(any(name.endswith('.deb') for name in self.downloads))
                self.log.unlink()

    def test_evicted_package_and_legacy_index_use_normal_root_build(self):
        index = json.loads(self.index.read_text())
        pkg = json.loads(self.original[packages.MANIFEST])['packages'][0]
        self.blobs.pop((pkg['sha256'], pkg['size']))
        for schema in (index['schema'], 1):
            with self.subTest(schema=schema):
                self.index.write_text(json.dumps({**index, 'schema': schema}))
                self.build()
                self.assertEqual(self.log.read_text().splitlines(), ['build', 'cquery'])
                self.assertFalse(any(name.endswith('.deb') for name in self.downloads))
                self.log.unlink()


if __name__ == '__main__':
    unittest.main()
