#!/usr/bin/env python3
"""Exercise the shared Make/Bazel container bridge without Docker or Bazel."""

import json
import os
import pathlib
import shlex
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))


class ArchiveRecipeFixture:
    """Use production logging/publication; replace only the Bazel executable."""

    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = pathlib.Path(temporary.name)
        (self.root / "tools").symlink_to(ROOT / "tools", target_is_directory=True)
        self.target = self.root / "target"
        self.target.mkdir()
        for archive in self.archives:
            (self.target / archive).write_bytes(b"previous image")
        self.calls = self.root / "bazel-calls.jsonl"
        self.fake_bazel = self.root / "bazel executable"
        self.fake_bazel.write_text("#!/usr/bin/env python3\n" + r"""
import hashlib, json, os, pathlib, sys
arguments = sys.argv[1:]
operation = arguments[0]
label = arguments[-1]
archive = json.loads(os.environ['FAKE_BAZEL_LABELS'])[label]
prerequisites = json.loads(os.environ['FAKE_BAZEL_PREREQUISITES'])[archive]
record = {'arguments': arguments,
          'prerequisites_ready': all(pathlib.Path(path).is_file() for path in prerequisites),
          'prerequisite_sha256': {path: hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()
                                  for path in prerequisites if pathlib.Path(path).is_file()}}
with pathlib.Path(os.environ['FAKE_BAZEL_CALLS']).open('a') as output:
    output.write(json.dumps(record) + '\n')
if not record['prerequisites_ready']:
    raise SystemExit('Make prerequisites were not prepared before Bazel')
mode = os.environ.get('FAKE_BAZEL_MODE', '')
source = pathlib.Path('bazel-bin/archive outputs') / archive
if operation == 'build':
    if mode == 'build_failure':
        raise SystemExit('fixture build failure')
    source.parent.mkdir(parents=True, exist_ok=True)
    if mode == 'directory_output':
        source.mkdir(exist_ok=True)
    else:
        source.write_bytes(b'' if mode == 'empty_output' else
                           os.environ.get('FAKE_BAZEL_PAYLOAD', 'new ' + archive).encode())
elif operation == 'cquery':
    print('INFO: fixture query diagnostic', file=sys.stderr)
    if mode == 'query_failure':
        print(source)
        raise SystemExit('fixture query failure')
    if mode == 'no_outputs':
        pass
    elif mode == 'multiple_outputs':
        print(source)
        print('bazel-bin/unexpected-second-output')
    elif mode == 'missing_output':
        print('bazel-bin/missing-output')
    else:
        print(source)
else:
    raise SystemExit('unexpected Bazel operation: ' + operation)
""")
        self.fake_bazel.chmod(0o755)
        # The logger succeeds even when its producer fails: the real LOG status
        # check must propagate the build/query/export failure through the pipe.
        scripts = self.root / "scripts"
        scripts.mkdir()
        logger = scripts / "process_log.sh"
        logger.write_text("#!/bin/sh\ncat\n")
        logger.chmod(0o755)
        error_screen = self.root / "update_screen.sh"
        error_screen.write_text("#!/bin/sh\nprintf 'error\\n' >> logging-errors\n")
        error_screen.chmod(0o755)
        log_setting = next(line for line in (ROOT / "rules/functions").read_text().splitlines()
                           if line.startswith("LOG = "))
        self.markers = self.root / "publication-markers"
        # These execution settings match slave.mk. Individual tests add owner
        # metadata and include the actual shared recipe, rather than copying it.
        self.makefile = """SHELL = /bin/bash
.SHELLFLAGS += -e
.ONESHELL:
.SECONDEXPANSION:
TARGET_PATH = target
CONFIGURED_ARCH = amd64
.PHONY: .platform
""" + f"PROJECT_ROOT = {self.root}\n" + log_setting + "\n" + r"""
sbom_emit_fragment = printf 'sbom %s %s %s\n' '$(1)' '$(2)' '$(3)' >> publication-markers
FOOTER = printf 'footer %s\n' '$@' >> publication-markers
"""

    def run_make(self, *archives, environment=None, parallel=False, goals=None):
        variables = dict(os.environ)
        # Test fallback and unset-versus-empty behavior independently of the
        # developer's shell settings or a pre-existing GNU Make invocation.
        for name in ("BAZEL_CONTAINER_ARGS", "BAZEL_CONTAINER_CACHE_DIR",
                     "BAZEL_SWSS_ARGS", "BAZEL_SWSS_CACHE_DIR", "MAKEFLAGS", "MFLAGS"):
            variables.pop(name, None)
        variables.update({
            "BAZEL": str(self.fake_bazel),
            "FAKE_BAZEL_CALLS": str(self.calls),
            "FAKE_BAZEL_MODE": "",
            "FAKE_BAZEL_LABELS": json.dumps({label: archive for archive, label in self.labels.items()}),
            "FAKE_BAZEL_PREREQUISITES": json.dumps(self.prerequisites),
        })
        variables.update(environment or {})
        return subprocess.run(
            ["make", "--no-print-directory", "-j2" if parallel else "-j1", "-f", "-",
             *(goals if goals is not None else
               ["target/" + archive for archive in (archives or self.archives[:1])])],
            input=self.makefile, cwd=self.root, env=variables,
            text=True, capture_output=True, check=False,
        )

    def recorded_calls(self):
        return [json.loads(line) for line in self.calls.read_text().splitlines()] if self.calls.exists() else []

    def assert_success(self, result):
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


class GenericDockerRecipeTest(ArchiveRecipeFixture, unittest.TestCase):
    archives = ("docker-telemetry.gz", "docker-routing.gz",
                "docker-telemetry-dbg.gz", "docker-routing-dbg.gz")
    labels = dict(zip(archives, ("//services/telemetry:release_tar", "//routing:installer_payload",
                                "//services/telemetry:debug_tar", "//routing:symbols_payload")))
    paths = dict(zip(archives, ("services/telemetry", "containers/routing",
                               "services/telemetry/debug", "containers/routing/debug")))

    def setUp(self):
        super().setUp()
        self.prerequisites = {archive: ["inputs/" + archive + ".ready"] for archive in self.archives}
        inputs = self.root / "inputs"
        inputs.mkdir()
        for archive in self.archives:
            (inputs / (archive + ".source")).write_text(archive)
        self.makefile += "SONIC_BAZEL_DOCKER_IMAGES = " + " ".join(self.archives[:2]) + "\n"
        self.makefile += "SONIC_BAZEL_DBG_DOCKER_IMAGES = " + " ".join(self.archives[2:]) + "\n"
        for archive in self.archives:
            self.makefile += f"{archive}_BAZEL_TARGET = {self.labels[archive]}\n"
            self.makefile += f"{archive}_PATH = {self.paths[archive]}\n"
            # INPUT_ROOT is deliberately defined after the include. These are
            # complete owner paths, not filenames prefixed by TARGET_PATH.
            self.makefile += f"{archive}_BAZEL_DEPENDS = $(INPUT_ROOT)/{archive}.ready\n"
        self.makefile += """include tools/bazel/docker.mk
INPUT_ROOT = inputs
inputs/%.ready: inputs/%.source
	@cp "$<" "$@"
.PHONY: registered
registered: $(SONIC_TARGET_LIST)
	@printf 'registered=%s\\n' '$(SONIC_TARGET_LIST)'
"""

    def test_runtime_and_debug_export_custom_targets_after_deferred_prerequisites(self):
        """Honor owner-supplied targets and deferred inputs without requiring SWSS files or
        imposing cache settings.
        """
        for archive in self.archives:
            with self.subTest(archive=archive):
                self.calls.unlink(missing_ok=True)
                self.assert_success(self.run_make(archive))
                destination = self.target / archive
                self.assertEqual(destination.read_bytes(), ("new " + archive).encode())
                self.assertEqual(destination.stat().st_mode & 0o777, 0o644)
                calls = self.recorded_calls()
                self.assertEqual([call['arguments'][0] for call in calls], ["build", "cquery"])
                self.assertTrue(all(call['prerequisites_ready'] for call in calls))
                for call in calls:
                    self.assertEqual(call['arguments'][-1], self.labels[archive])
                    self.assertFalse(any(option.startswith(("--repository_cache=", "--disk_cache=", "--output_base="))
                                         for option in call['arguments']))
        self.assertFalse((self.target / "docker-config-engine-trixie.oci").exists())
        self.assertFalse((self.target / "python-wheels").exists())
        self.assertFalse((self.target / ".swss-build-method").exists())

    def test_configured_native_architecture_reaches_build_and_query(self):
        """An ARM64 Make build must not inherit the root RC's AMD64 platforms."""
        original = self.makefile
        for architecture, platform in (("amd64", "x86_64_trixie"), ("arm64", "aarch64_trixie")):
            with self.subTest(architecture=architecture):
                self.makefile = original + "\nCONFIGURED_ARCH = " + architecture + "\n"
                self.calls.unlink(missing_ok=True)
                self.assert_success(self.run_make(environment={
                    "BAZEL_CONTAINER_ARGS": "--platforms=//wrong:target --host_platform=//wrong:host",
                }))
                calls = self.recorded_calls()
                self.assertEqual([call["arguments"][0] for call in calls], ["build", "cquery"])
                for call in calls:
                    for flag in ("--platforms=", "--host_platform="):
                        selected = [arg for arg in call["arguments"] if arg.startswith(flag)]
                        self.assertEqual(selected[-1], flag + "@sonic_build_infra//platforms:" + platform)
        self.makefile = original

    def test_unsupported_native_architecture_fails_without_publishing_an_image(self):
        """An unsupported architecture must never silently produce an AMD64 image."""
        original = self.makefile
        for architecture in ("armhf", "riscv64", ""):
            with self.subTest(architecture=architecture):
                self.makefile = original + "\nCONFIGURED_ARCH = " + architecture + "\n"
                result = self.run_make()
                self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn("Unsupported native Bazel container architecture:",
                              (self.target / (self.archives[0] + ".log")).read_text())
                self.assertEqual(self.recorded_calls(), [])
                self.assertEqual((self.target / self.archives[0]).read_bytes(), b"previous image")
                self.assertFalse(self.markers.exists())
        self.makefile = original

    def test_image_source_options_apply_to_build_and_query_without_leaking(self):
        """A DASH image selects its feature flags without changing another image."""
        selected = self.archives[0]
        self.makefile += f"{selected}_BAZEL_ARGS = --define=image=dashboard\n"
        self.assert_success(self.run_make(*self.archives, parallel=True, environment={
            "BAZEL_CONTAINER_ARGS": "--jobs=2 --define=image=default",
        }))
        calls = self.recorded_calls()
        self.assertEqual(len(calls), 2 * len(self.archives))
        for call in calls:
            args = call["arguments"]
            self.assertIn("--jobs=2", args)
            if args[-1] == self.labels[selected]:
                self.assertLess(args.index("--define=image=default"),
                                args.index("--define=image=dashboard"))
            else:
                self.assertNotIn("--define=image=dashboard", args)
    def test_both_lists_register_buildable_targets_and_keep_owner_sbom_paths(self):
        """Keep runtime and debug archives buildable together, with owner-specific SBOM
        attribution and normal logging.
        """
        result = self.run_make(goals=["registered"], parallel=True)
        self.assert_success(result)
        registered = next(line.removeprefix("registered=") for line in result.stdout.splitlines()
                          if line.startswith("registered="))
        self.assertCountEqual(registered.split(), ["target/" + archive for archive in self.archives])
        markers = self.markers.read_text().splitlines()
        for archive in self.archives:
            sbom = f"sbom target/{archive} DOCKER_IMAGE {self.paths[archive]}"
            footer = f"footer target/{archive}"
            self.assertIn(sbom, markers)
            self.assertLess(markers.index(sbom), markers.index(footer))
            self.assertIn('INFO: fixture query diagnostic', (self.target / (archive + '.log')).read_text())
        self.assertFalse((self.root / 'logging-errors').exists())
        self.assertEqual(len(self.recorded_calls()), 2 * len(self.archives))

    def test_containers_have_independent_prerequisites(self):
        """A missing input must preserve its container archive without blocking an unrelated
        container.
        """
        broken, healthy = self.archives[:2]
        (self.root / ("inputs/" + broken + ".source")).unlink()
        self.assert_success(self.run_make(healthy))
        self.assertEqual({call['arguments'][-1] for call in self.recorded_calls()}, {self.labels[healthy]})
        self.calls.unlink()
        result = self.run_make(broken)
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(self.recorded_calls(), [])
        self.assertEqual((self.target / broken).read_bytes(), b'previous image')

    def test_slave_filters_selected_archives_and_registers_runtime_and_debug_loads(self):
        """Assign each selected archive to one builder while retaining the normal Docker-load
        targets.
        """
        slave = (ROOT / 'slave.mk').read_text()
        filters = '\n'.join(line for line in slave.splitlines()
                            if line.startswith(('DOCKER_IMAGES := $(filter-out ',
                                                'DOCKER_DBG_IMAGES := $(filter-out ')))
        load_start = slave.index('DOCKER_LOAD_TARGETS = ')
        load_definition = slave[load_start:slave.index('\n\n', load_start)]
        self.makefile += """
DOCKER_IMAGES = legacy.gz $(SONIC_BAZEL_DOCKER_IMAGES)
DOCKER_DBG_IMAGES = legacy-dbg.gz $(SONIC_BAZEL_DBG_DOCKER_IMAGES)
SONIC_BAZEL_OCI_BASES = bases/telemetry.oci bases/routing.oci
""" + filters + '\n' + load_definition + """
.PHONY: selection
selection:
	@printf 'runtime=%s\\n' '$(DOCKER_IMAGES)'
	@printf 'debug=%s\\n' '$(DOCKER_DBG_IMAGES)'
	@printf 'loads=%s\\n' '$(DOCKER_LOAD_TARGETS)'
"""
        result = self.run_make(goals=['selection'])
        self.assert_success(result)
        values = dict(line.split('=', 1) for line in result.stdout.splitlines())
        self.assertEqual(values['runtime'], 'legacy.gz')
        self.assertEqual(values['debug'], 'legacy-dbg.gz')
        self.assertCountEqual(values['loads'].split(),
                              ['target/' + archive + '-load'
                               for archive in (*self.archives, 'legacy.gz', 'legacy-dbg.gz')])
        self.assertEqual(self.recorded_calls(), [])

    def test_missing_target_or_sbom_path_is_rejected_before_bazel(self):
        """Reject incomplete owner declarations before building an archive without a target or
        SBOM attribution.
        """
        original = self.makefile
        archive = self.archives[-1]
        for field, value in (("BAZEL_TARGET", self.labels[archive]), ("PATH", self.paths[archive])):
            with self.subTest(field=field):
                self.makefile = original.replace(f"{archive}_{field} = {value}\n", "")
                result = self.run_make(archive)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(f"{archive}_{field} is required", result.stderr)
                self.assertEqual(self.recorded_calls(), [])
        self.makefile = original

    def test_local_packages_reject_each_selected_runtime_and_debug_archive(self):
        """Prevent the local-package path from accepting Bazel exports whose latest-only tags do
        not meet its contract.
        """
        original = self.makefile
        for archive in self.archives:
            with self.subTest(archive=archive):
                self.makefile = f"SONIC_PACKAGES_LOCAL = {archive}\n" + original
                result = self.run_make(archive)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("cannot be in SONIC_PACKAGES_LOCAL: " + archive, result.stderr)
                self.assertEqual(self.recorded_calls(), [])
                self.assertEqual((self.target / archive).read_bytes(), b"previous image")
        self.makefile = original

    def test_build_and_query_share_cache_and_quoted_options_without_shell_expansion(self):
        """Keep build and query configurations identical while preserving literal arguments and
        avoiding shared output bases.
        """
        cache = self.root / "builder's persistent cache"
        options = ["--jobs=2", "--define=message=two words", "--define=owner=builder's cache",
                   "--define=literal=$(touch SHOULD_NOT_EXIST)"]
        self.assert_success(self.run_make(environment={
            "BAZEL_CONTAINER_CACHE_DIR": str(cache), "BAZEL_CONTAINER_ARGS": shlex.join(options),
        }))
        calls = self.recorded_calls()
        self.assertEqual(len(calls), 2)
        for call in calls:
            arguments = call['arguments']
            for option in [*options, f"--repository_cache={cache}/repository_cache", f"--disk_cache={cache}/disk_cache"]:
                self.assertIn(option, arguments)
            self.assertFalse(any(option.startswith(("--output_base=", "--output_user_root=")) for option in arguments))
        self.assertEqual(calls[0]['arguments'][1:],
                         [option for option in calls[1]['arguments'][1:] if option != '--output=files'])
        self.assertTrue((cache / 'repository_cache').is_dir())
        self.assertTrue((cache / 'disk_cache').is_dir())
        self.assertFalse((self.root / 'SHOULD_NOT_EXIST').exists())

    def test_legacy_environment_fallback_and_explicit_new_overrides(self):
        """Retain legacy cache settings unless generic settings override them, including an
        explicit empty opt-out.
        """
        legacy_cache = self.root / 'legacy cache'
        for new_values, expected_cache, expected_option in (
            ({}, legacy_cache, '--jobs=1'),
            ({'BAZEL_CONTAINER_CACHE_DIR': str(self.root / 'new cache'),
              'BAZEL_CONTAINER_ARGS': '--jobs=2'}, self.root / 'new cache', '--jobs=2'),
            ({'BAZEL_CONTAINER_CACHE_DIR': '', 'BAZEL_CONTAINER_ARGS': ''}, None, None),
        ):
            with self.subTest(new_values=new_values):
                self.calls.unlink(missing_ok=True)
                self.assert_success(self.run_make(environment={
                    'BAZEL_SWSS_CACHE_DIR': str(legacy_cache), 'BAZEL_SWSS_ARGS': '--jobs=1', **new_values,
                }))
                for call in self.recorded_calls():
                    arguments = call['arguments']
                    self.assertEqual([option for option in arguments if option.startswith('--jobs=')],
                                     [expected_option] if expected_option else [])
                    self.assertEqual([option for option in arguments if option.startswith('--repository_cache=')],
                                     [f'--repository_cache={expected_cache}/repository_cache'] if expected_cache else [])
                    self.assertEqual([option for option in arguments if option.startswith('--disk_cache=')],
                                     [f'--disk_cache={expected_cache}/disk_cache'] if expected_cache else [])

    def test_failed_build_query_or_invalid_output_preserves_previous_archive(self):
        """Ensure production logging exposes failures without replacing the last image or reaching
        SBOM and success hooks.
        """
        archive = self.archives[0]
        for mode in ('build_failure', 'query_failure', 'no_outputs', 'multiple_outputs',
                     'missing_output', 'empty_output', 'directory_output'):
            with self.subTest(mode=mode):
                self.calls.unlink(missing_ok=True)
                source = self.root / 'bazel-bin/archive outputs' / archive
                if source.is_file():
                    source.unlink()
                destination = self.target / archive
                previous = destination.read_bytes(), destination.stat().st_mtime_ns
                result = self.run_make(environment={'FAKE_BAZEL_MODE': mode})
                self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual((destination.read_bytes(), destination.stat().st_mtime_ns), previous)
                self.assertEqual([call['arguments'][0] for call in self.recorded_calls()],
                                 ['build'] if mode == 'build_failure' else ['build', 'cquery'])
                self.assertFalse([path for path in self.target.glob(archive + '.*') if path.suffix != '.log'])
                self.assertFalse(self.markers.exists(), 'failure continued to SBOM or footer')
                self.assertTrue((self.root / 'logging-errors').is_file(),
                                'production LOG did not surface the producer failure')

    def test_unchanged_output_keeps_archive_timestamp_while_bazel_still_checks_inputs(self):
        """Ask Bazel to check inputs on every request without triggering downstream Make rebuilds
        for identical bytes.
        """
        destination = self.target / self.archives[0]
        previous = destination.stat().st_mtime_ns
        for _ in range(2):
            self.assert_success(self.run_make(environment={'FAKE_BAZEL_PAYLOAD': 'previous image'}))
            self.assertEqual(destination.read_bytes(), b'previous image')
            self.assertEqual(destination.stat().st_mtime_ns, previous)
        self.assertEqual([call['arguments'][0] for call in self.recorded_calls()],
                         ['build', 'cquery', 'build', 'cquery'])

    def test_unusable_cache_fails_before_bazel_and_preserves_archive(self):
        """Reject an invalid cache path before invoking Bazel or modifying the existing image and
        cache-path file.
        """
        cache = self.root / 'cache-is-file'
        cache.write_bytes(b'not a directory')
        result = self.run_make(environment={'BAZEL_CONTAINER_CACHE_DIR': str(cache)})
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(self.recorded_calls(), [])
        self.assertEqual((self.target / self.archives[0]).read_bytes(), b'previous image')
        self.assertEqual(cache.read_bytes(), b'not a directory')
        self.assertFalse(self.markers.exists(), 'failure continued to SBOM or footer')


if __name__ == '__main__':
    unittest.main()
