#!/usr/bin/env python3
"""Check generic Make OCI-base preparation with actual Docker-save fixtures."""

import hashlib
import json
import os
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))

from tools.bazel.tests.docker_test import ArchiveRecipeFixture
from tools.bazel.tests.oci_base_fixture import docker_save, image_fixture


class OciBaseMakeTest(ArchiveRecipeFixture, unittest.TestCase):
    archives = ("telemetry.gz", "telemetry-dbg.gz", "routing.gz", "routing-dbg.gz")
    labels = {name: "//services:" + name for name in archives}
    bases = {"bases/telemetry.oci": "amd64", "bases/routing.oci": "arm64"}

    def setUp(self):
        super().setUp()
        cache = self.root / 'make-cache'
        cache.mkdir()
        self.sources = {base: cache / (architecture + '-saved-image.tar.gz')
                        for base, architecture in self.bases.items()}
        self.prerequisites = {name: ['target/bases/' + name.split('-')[0].split('.')[0] + '.oci/index.json']
                              for name in self.archives}
        for base, architecture in self.bases.items():
            docker_save(self.sources[base], *image_fixture(architecture))
        self.makefile += """
SONIC_BAZEL_DOCKER_IMAGES = telemetry.gz routing.gz
SONIC_BAZEL_DBG_DOCKER_IMAGES = telemetry-dbg.gz routing-dbg.gz
SONIC_BAZEL_OCI_BASES = bases/telemetry.oci bases/routing.oci bases/telemetry.oci
bases/telemetry.oci_OCI_ARCHIVE = $(TELEMETRY_ARCHIVE)
bases/telemetry.oci_OCI_PLATFORM = linux/amd64
bases/routing.oci_OCI_ARCHIVE = $(ROUTING_ARCHIVE)
bases/routing.oci_OCI_PLATFORM = linux/arm64
"""
        for name in self.archives:
            owner = name.split('-')[0].split('.')[0]
            self.makefile += f"{name}_BAZEL_TARGET = {self.labels[name]}\n"
            self.makefile += f"{name}_BAZEL_DEPENDS = $(TARGET_PATH)/bases/{owner}.oci\n"
            self.makefile += f"{name}_PATH = services/{owner}\n"
        # Source paths are resolved only after the shared include. The existing
        # archives model Make cache hits: their producer recipes must not run.
        self.makefile += """include tools/bazel/docker.mk
TELEMETRY_ARCHIVE = make-cache/amd64-saved-image.tar.gz
ROUTING_ARCHIVE = make-cache/arm64-saved-image.tar.gz
$(TELEMETRY_ARCHIVE) $(ROUTING_ARCHIVE):
	@echo unexpected-archive-rebuild >> archive-rebuilds
	@exit 1
"""

    def snapshot(self, base):
        layout = self.target / base
        return {
            'link': os.readlink(layout),
            'link_mtime_ns': layout.lstat().st_mtime_ns,
            'files': {path.relative_to(layout).as_posix(): (path.read_bytes(), path.stat().st_mtime_ns)
                      for path in sorted(layout.rglob('*')) if path.is_file()},
        }

    def config(self, base):
        layout = self.target / base
        index = json.loads((layout / 'index.json').read_text())
        manifest_digest = index['manifests'][0]['digest'].replace(':', '/')
        manifest = json.loads((layout / 'blobs' / manifest_digest).read_text())
        config_digest = manifest['config']['digest'].replace(':', '/')
        return json.loads((layout / 'blobs' / config_digest).read_text())

    def test_cached_two_architecture_bases_prepare_once_for_parallel_runtime_and_debug(self):
        """Share base preparation across parallel consumers while keeping platforms independent
        and reusing Make archives.
        """
        previous = None
        source_mtimes = {base: source.stat().st_mtime_ns for base, source in self.sources.items()}
        for _ in range(2):
            self.calls.unlink(missing_ok=True)
            result = self.run_make(*self.archives, parallel=True)
            self.assert_success(result)
            # Four consumers share two bases: one preparation per base per Make.
            self.assertEqual(result.stdout.count('python3 tools/bazel/oci/prepare_oci_base.py'), 2)
            current = {base: self.snapshot(base) for base in self.bases}
            if previous is not None:
                self.assertEqual(current, previous)
            previous = current
            for base, architecture in self.bases.items():
                self.assertEqual(self.config(base)['architecture'], architecture)
                self.assertEqual(self.config(base)['os'], 'linux')
                self.assertEqual(self.sources[base].stat().st_mtime_ns, source_mtimes[base])
            calls = self.recorded_calls()
            self.assertEqual(len(calls), 8)
            self.assertTrue(all(call['prerequisites_ready'] for call in calls))
            for name in self.archives:
                self.assertEqual([call['arguments'][0] for call in calls
                                  if call['arguments'][-1] == self.labels[name]], ['build', 'cquery'])
                self.assertEqual((self.target / name).read_bytes(), ('new ' + name).encode())
            self.assertFalse((self.root / 'archive-rebuilds').exists())
        self.assertFalse((self.target / 'docker-config-engine-trixie.oci').exists())
        self.assertFalse((self.target / '.swss-build-method').exists())

    def test_base_can_be_requested_without_building_a_container(self):
        """Allow a base-only request without duplicate recipes, unrelated layouts, or container
        builds.
        """
        self.makefile += """
.PHONY: registration
registration:
	@printf 'registered=%s\\n' '$(SONIC_TARGET_LIST)'
"""
        result = self.run_make(goals=['target/bases/routing.oci', 'registration'])
        self.assert_success(result)
        registered = next(line.removeprefix('registered=') for line in result.stdout.splitlines()
                          if line.startswith('registered='))
        self.assertCountEqual(registered.split(),
                              ['target/' + name for name in (*self.archives, *self.bases)])
        self.assertNotIn('given more than once', result.stderr)
        self.assertNotIn('overriding recipe', result.stderr)
        self.assertEqual(self.config('bases/routing.oci')['architecture'], 'arm64')
        self.assertFalse((self.target / 'bases/telemetry.oci').exists())
        self.assertEqual(self.recorded_calls(), [])
        self.assertFalse(self.markers.exists())

    def test_changed_archive_with_preserved_mtime_refreshes_layout_before_consumers(self):
        """Detect changed base bytes despite stable mtimes, publish before consumers run, and
        preserve prior generations.
        """
        self.assert_success(self.run_make(*self.archives, parallel=True))
        base = 'bases/telemetry.oci'
        before = {name: self.snapshot(name) for name in self.bases}
        old_generation = (self.target / base).resolve()
        source = self.sources[base]
        old_stat = source.stat()
        config, layers = image_fixture('amd64')
        changed_config = json.loads(config)
        changed_config['config']['Env'].append('REVISION=updated')
        docker_save(source, json.dumps(changed_config).encode(), layers)
        os.utime(source, ns=(old_stat.st_atime_ns, old_stat.st_mtime_ns))
        self.calls.unlink()
        self.assert_success(self.run_make('telemetry.gz', 'telemetry-dbg.gz', parallel=True))
        after = self.snapshot(base)
        self.assertNotEqual(after['link'], before[base]['link'])
        self.assertIn('REVISION=updated', self.config(base)['config']['Env'])
        self.assertEqual(self.snapshot('bases/routing.oci'), before['bases/routing.oci'])
        self.assertEqual((old_generation / 'index.json').read_bytes(), before[base]['files']['index.json'][0])
        index = 'target/' + base + '/index.json'
        expected = hashlib.sha256((self.root / index).read_bytes()).hexdigest()
        calls = self.recorded_calls()
        self.assertEqual(len(calls), 4)
        self.assertTrue(all(call['prerequisite_sha256'][index] == expected for call in calls))
        self.assertFalse((self.root / 'archive-rebuilds').exists())

    def test_replacing_archive_path_revalidates_bytes_even_when_new_source_is_older(self):
        """Revalidate a replacement source regardless of age while retaining the layout when its
        OCI content is identical.
        """
        self.assert_success(self.run_make('routing.gz'))
        base = 'bases/routing.oci'
        before = self.snapshot(base)
        replacement = self.root / 'make-cache/replacement.tar.gz'
        # Different archive bytes, identical OCI files: keep the published layout.
        docker_save(replacement, *image_fixture('arm64'), mtime=17)
        os.utime(replacement, ns=(0, 0))
        self.makefile += '\nROUTING_ARCHIVE = make-cache/replacement.tar.gz\n'
        self.calls.unlink()
        self.assert_success(self.run_make('routing.gz'))
        self.assertEqual(self.snapshot(base), before)
        receipt = json.loads((self.target / 'bases/.routing.oci.source.json').read_text())
        self.assertEqual(receipt['archive_sha256'], hashlib.sha256(replacement.read_bytes()).hexdigest())
        self.assertEqual(len(self.recorded_calls()), 2)

    def test_mismatched_platform_stops_before_bazel_and_preserves_existing_outputs(self):
        """Reject the wrong architecture through normal logging without replacing the last valid
        base or container.
        """
        self.assert_success(self.run_make('telemetry.gz'))
        base = 'bases/telemetry.oci'
        layout = self.snapshot(base)
        output = self.target / 'telemetry.gz'
        archive = output.read_bytes(), output.stat().st_mtime_ns
        self.calls.unlink()
        self.markers.unlink()
        self.makefile += '\nbases/telemetry.oci_OCI_PLATFORM = linux/arm64\n'
        result = self.run_make('telemetry.gz')
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("OCI image platform 'linux/amd64' does not match 'linux/arm64'",
                      (self.target / 'bases/telemetry.oci.log').read_text())
        self.assertEqual(self.recorded_calls(), [])
        self.assertEqual(self.snapshot(base), layout)
        self.assertEqual((output.read_bytes(), output.stat().st_mtime_ns), archive)
        self.assertFalse(self.markers.exists())
        self.assertTrue((self.root / 'logging-errors').is_file())

    def test_missing_base_metadata_stops_before_preparation_or_bazel(self):
        """Require an explicit source archive and platform before publishing a base or running its
        consumers.
        """
        original = self.makefile
        for field in ('OCI_ARCHIVE', 'OCI_PLATFORM'):
            with self.subTest(field=field):
                self.makefile = original + f'\nbases/telemetry.oci_{field} =\n'
                result = self.run_make('telemetry.gz')
                self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn(f'bases/telemetry.oci_{field} is required', result.stderr)
                self.assertEqual(self.recorded_calls(), [])
                self.assertFalse((self.target / 'bases/telemetry.oci').exists())
                self.assertEqual((self.target / 'telemetry.gz').read_bytes(), b'previous image')
        self.makefile = original


if __name__ == '__main__':
    unittest.main()
