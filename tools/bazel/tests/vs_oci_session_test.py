"""Exercise P4 Make dependencies and in-session verification without a builder."""

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]
ARCHIVES = ["docker-orchagent.gz", "docker-orchagent-dbg.gz",
            "docker-syncd-vs.gz", "docker-syncd-vs-dbg.gz"]
PACKAGES = ["sonic-p4rt_0.0.1_amd64.deb", "sonic-p4rt-dbgsym_0.0.1_amd64.deb"]
DEB_PATHS = ["target/debs/trixie/" + name for name in PACKAGES]
BASE_ARCHIVE = "target/docker-config-engine-trixie.gz"


class VsOciSessionTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        for source in ("tools/bazel/ci/verify_vs_oci.sh", "tools/bazel/build_helpers.py",
                       "tools/bazel/cache/credentials.py"):
            destination = self.directory / source
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / source, destination)
        (self.directory / "tools/bazel/ci/verify_p4rt_debug.py").write_text(
            "import json, os, pathlib, sys\n"
            "pathlib.Path('debug-arguments.json').write_text(json.dumps(sys.argv[1:]))\n"
            "print('debug verification output')\n"
            "sys.exit(int(os.environ.get('DEBUG_STATUS', '0')))\n")
        self.bazel = self.directory / "bazel"
        self.bazel.write_text("""#!/usr/bin/env python3
import json, os, pathlib, sys
args = sys.argv[1:]
with pathlib.Path('bazel-arguments.jsonl').open('a') as output:
    output.write(json.dumps(args) + '\\n')
command = args[0]
if command == 'test':
    target = next(arg.split(':')[1] for arg in args if arg.startswith('//'))
    output = pathlib.Path('bazel-testlogs/tools/bazel/tests') / target
    for name in ('test.log', 'test.xml', 'test.outputs/validation.json'):
        path = output / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(name)
    print('image verification output')
    sys.exit(int(os.environ.get('IMAGE_STATUS', '0')))
if command == 'build':
    label = next(arg for arg in args if arg.startswith('//'))
    pathlib.Path('policy.json').write_text(label)
if command == 'cquery':
    print('policy.json')
""")
        self.bazel.chmod(0o755)
        self.make = self.directory / "make command"
        self.make.write_text("#!" + sys.executable + "\n" +
                            "import json, os, pathlib, sys\n"
                            "with pathlib.Path('make-calls.jsonl').open('a') as output:\n"
                            "    output.write(json.dumps(sys.argv[1:]) + '\\n')\n" +
                            "os.execv(" + repr(shutil.which("make")) + ", ['make', *sys.argv[1:]])\n")
        self.make.chmod(0o755)
        self.settings = {
            "BUILD_WITH_BAZEL_WHEN_AVAILABLE": "y", "BLDENV": "trixie", "CONFIGURED_PLATFORM": "vs",
            "CONFIGURED_ARCH": "amd64", "INCLUDE_FIPS": "y", "INCLUDE_VS_DASH_SAI": "y",
            "TARGET_PATH": "target", "DEBS_PATH": "target/debs/$(BLDENV)",
            "DOCKER_CONFIG_ENGINE_TRIXIE": "docker-config-engine-trixie.gz",
            "SONIC_BAZEL_DOCKER_IMAGES": " ".join([*ARCHIVES[::2], "docker-config-engine-trixie.gz"]),
            "SONIC_BAZEL_DBG_DOCKER_IMAGES": " ".join(ARCHIVES[1::2]),
            "SONIC_IMAGE_VERSION": "master.42-test",
            "SONIC_P4RT_VERSION": "0.0.1",
            "SONIC_P4RT": "sonic-p4rt_$(SONIC_P4RT_VERSION)_$(CONFIGURED_ARCH).deb",
            "SONIC_P4RT_DBG": "sonic-p4rt-dbgsym_$(SONIC_P4RT_VERSION)_$(CONFIGURED_ARCH).deb",
        }
        (self.directory / "input.source").write_text("first source revision\n")
        (self.directory / "slave.mk").write_text(
            "\n".join(name + " = " + value for name, value in self.settings.items()) + "\n" + r'''
.DEFAULT_GOAL := forbidden-default
.PHONY: forbidden-default
forbidden-default:
	@echo 'The metadata query must not build the default goal' >&2; exit 39
input.ready: input.source
	@cp "$<" "$@"
$(addprefix $(DEBS_PATH)/,$(SONIC_P4RT) $(SONIC_P4RT_DBG)) $(TARGET_PATH)/$(DOCKER_CONFIG_ENGINE_TRIXIE): input.ready
	@mkdir recipe-lock
	@printf '%s\n' '$@' >> producers.log
	@sleep 0.02
	@rmdir recipe-lock
	@test '$@' != '$(FAIL_TARGET)'
	@mkdir -p '$(@D)'
	@if test '$@' = '$(OMIT_OUTPUT)'; then :; \
	elif test '$@' = '$(EMPTY_OUTPUT)'; then : > '$@'; \
	else cp input.ready '$@'; fi
# Existing image archives are validation inputs, never package-stage goals.
$(addprefix target/,docker-orchagent.gz docker-orchagent-dbg.gz docker-syncd-vs.gz docker-syncd-vs-dbg.gz):
	@echo 'Validation must not request container producers' >&2; exit 41
''')
        self.environment = dict(os.environ, BAZEL=str(self.bazel))
        for name in ("MAKEFLAGS", "MFLAGS", "MAKEOVERRIDES", "MAKELEVEL",
                     "BAZEL_CONTAINER_ARGS", "BAZEL_SWSS_ARGS", "BAZEL_CONTAINER_CACHE_DIR", "BAZEL_SWSS_CACHE_DIR"):
            self.environment.pop(name, None)

    def run_verifier(self, *, settings=None, **environment):
        arguments = [str(self.make), "--no-print-directory", "-j8", "-f", "slave.mk"]
        arguments += [key + "=" + value for key, value in (settings or {}).items()]
        return subprocess.run(
            ["bash", "tools/bazel/ci/verify_vs_oci.sh", *arguments],
            cwd=self.directory, env=dict(self.environment, **environment),
            text=True, capture_output=True, check=False)

    def producers(self):
        path = self.directory / "producers.log"
        return path.read_text().splitlines() if path.exists() else []

    def make_calls(self):
        path = self.directory / "make-calls.jsonl"
        return [json.loads(line) for line in path.read_text().splitlines()] if path.exists() else []

    def test_separate_validation_reuses_the_published_config_engine(self):
        switch = {"SONIC_VS_REUSE_BAZEL_PRODUCERS": "1"}
        base = self.directory / BASE_ARCHIVE
        base.parent.mkdir()
        base.write_bytes(b"base used by the installer")
        result = self.run_verifier(**switch)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.producers(), DEB_PATHS)
        self.assertEqual(base.read_bytes(), b"base used by the installer")
        self.assertTrue((self.directory / "artifacts/vs/syncd-installed-images-policy.json").is_file())
        self.assertTrue((self.directory / "artifacts/vs/swss-installed-images-policy.json").is_file())
        arguments = [json.loads(line) for line in
                     (self.directory / "bazel-arguments.jsonl").read_text().splitlines()]
        self.assertFalse(any(arg.startswith(("--output_base=", "--output_user_root="))
                             for args in arguments for arg in args))
        self.assertFalse(any("shutdown" in args for args in arguments))

        # A Bazel test or policy output copied during the first attempt is read-only.
        for name in ("swss-images/test.log", "syncd-installed-images-policy.json"):
            (self.directory / "artifacts/vs" / name).chmod(0o444)
        result = self.run_verifier(**switch)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse((self.directory / "target/.vs-bazel").exists())

    def test_reference_images_use_the_release_recorded_inside_the_installer(self):
        version = self.directory / "artifacts/vs/sonic-version.json"
        version.parent.mkdir(parents=True)
        version.write_text(json.dumps({"build_version": "master.42-built"}))
        result = self.run_verifier(settings={"SONIC_IMAGE_VERSION": "master.42-later"})
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        calls = (self.directory / "bazel-arguments.jsonl").read_text().splitlines()
        for call in map(json.loads, calls):
            self.assertIn("--//tools/bazel/image/vs/metadata:use_make_metadata=true", call)
            self.assertIn("--//tools/bazel/oci:image_version=master.42-built", call)

    def test_verification_keeps_archive_options_logs_and_deb_inputs(self):
        cache = self.directory / "cache with spaces"
        result = self.run_verifier(BAZEL_CONTAINER_CACHE_DIR=str(cache),
                                   BAZEL_CONTAINER_ARGS="--distdir='/input with spaces' --jobs=3")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        arguments = [json.loads(line) for line in (self.directory / "bazel-arguments.jsonl").read_text().splitlines()]
        target_calls = [args for args in arguments if any(arg.startswith("//") for arg in args)]
        self.assertEqual(len(target_calls), 6)
        self.assertFalse(any(arg.startswith(("--output_base=", "--output_user_root="))
                             for args in target_calls for arg in args))
        test_calls = [args[args.index("test"):] for args in target_calls if "test" in args]
        common = [f"--repository_cache={cache}/repository_cache", f"--disk_cache={cache}/disk_cache",
                  "--distdir=/input with spaces", "--jobs=3", "--//tools/bazel/image/vs/metadata:use_make_metadata=true",
                  "--//tools/bazel/oci:image_version=master.42-test",
                  "--platforms=@sonic_build_infra//platforms:x86_64_trixie",
                  "--host_platform=@sonic_build_infra//platforms:x86_64_trixie"]
        for index, (target, flags) in enumerate((("swss_images_test", []),
                                               ("syncd_p4_images_test", ["--config=syncd-dash"]))):
            self.assertEqual(test_calls[index], ["test", *common, *flags, "//tools/bazel/tests:" + target,
                "--nocache_test_results", "--nozip_undeclared_test_outputs"])
        for family in ("swss-images", "syncd-p4-images"):
            for name in ("test.log", "test.xml", "validation.json"):
                self.assertTrue((self.directory / "artifacts/vs" / family / name).is_file())
        for family in ("swss", "syncd"):
            policy = self.directory / "artifacts/vs" / (family + "-installed-images-policy.json")
            self.assertEqual(policy.read_text(), "//tools/bazel/tests:" + family + "_installed_images_policy")
        self.assertIn("image verification output", (self.directory / "artifacts/vs/syncd-p4-images.log").read_text())
        debug = json.loads((self.directory / "debug-arguments.json").read_text())
        self.assertEqual(debug[:4], ["--runtime-deb", DEB_PATHS[0], "--debug-deb", DEB_PATHS[1]])
        self.assertIn("debug verification output", (self.directory / "artifacts/vs/p4rt-debug-verification.log").read_text())

    def test_image_test_failure_retains_evidence_and_stops_debug_verification(self):
        result = self.run_verifier(IMAGE_STATUS="7")
        self.assertEqual(result.returncode, 7, result.stdout + result.stderr)
        self.assertTrue((self.directory / "artifacts/vs/syncd-p4-images/test.xml").is_file())
        self.assertFalse((self.directory / "debug-arguments.json").exists())

    def test_debug_verification_failure_is_not_hidden_by_tee(self):
        result = self.run_verifier(DEBUG_STATUS="9")
        self.assertEqual(result.returncode, 9, result.stdout + result.stderr)

    def test_resolved_package_paths_use_the_passed_make_and_serial_producers(self):
        """A no-op query selects actual Make variables before fresh package builds."""
        settings = {"SONIC_P4RT_VERSION": "7.4.9", "DEBS_PATH": "target/selected-debs"}
        result = self.run_verifier(settings=settings)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        expected = ["target/selected-debs/" + name.replace("0.0.1", "7.4.9") for name in PACKAGES]
        self.assertEqual(self.producers(), [*expected, BASE_ARCHIVE])
        calls = self.make_calls()
        self.assertEqual(len(calls), 2)
        self.assertIn("-", calls[0])
        self.assertNotIn("-", calls[1])
        self.assertIn("-j1", calls[1])
        self.assertEqual(calls[1][-3:], [*expected, BASE_ARCHIVE])
        for call in calls:
            self.assertIn("SONIC_P4RT_VERSION=7.4.9", call)
            self.assertIn("DEBS_PATH=target/selected-debs", call)
        debug = json.loads((self.directory / "debug-arguments.json").read_text())
        self.assertEqual(debug[:4], ["--runtime-deb", expected[0], "--debug-deb", expected[1]])

    def test_existing_outputs_still_check_prerequisite_freshness(self):
        result = self.run_verifier()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(self.producers(), [*DEB_PATHS, BASE_ARCHIVE])
        result = self.run_verifier()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(self.producers(), [*DEB_PATHS, BASE_ARCHIVE])
        (self.directory / "input.source").write_text("changed source revision\n")
        result = self.run_verifier()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(self.producers(), [*DEB_PATHS, BASE_ARCHIVE] * 2)
        for name in DEB_PATHS:
            self.assertEqual((self.directory / name).read_text(), "changed source revision\n")

    def test_build_failure_stops_later_outputs_and_verification(self):
        dependencies = [*DEB_PATHS, BASE_ARCHIVE]
        for index, target in enumerate(dependencies):
            with self.subTest(target=target):
                shutil.rmtree(self.directory / "target", ignore_errors=True)
                (self.directory / "producers.log").unlink(missing_ok=True)
                result = self.run_verifier(settings={"FAIL_TARGET": target})
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(self.producers(), dependencies[:index + 1])
                self.assertFalse((self.directory / "bazel-arguments.jsonl").exists())
                self.assertFalse((self.directory / "debug-arguments.json").exists())

    def test_missing_or_empty_package_outputs_stop_before_image_validation(self):
        for mode in ("OMIT_OUTPUT", "EMPTY_OUTPUT"):
            for target in [*DEB_PATHS, BASE_ARCHIVE]:
                with self.subTest(mode=mode, target=target):
                    shutil.rmtree(self.directory / "target", ignore_errors=True)
                    result = self.run_verifier(settings={mode: target})
                    self.assertNotEqual(result.returncode, 0)
                    self.assertFalse((self.directory / "bazel-arguments.jsonl").exists())
                    self.assertFalse((self.directory / "debug-arguments.json").exists())

    def test_unsupported_profiles_fail_before_any_producer(self):
        for overrides in ({"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"}, {"CONFIGURED_ARCH": "arm64"},
                          {"BLDENV": "bookworm"}, {"INCLUDE_FIPS": "n"}, {"INCLUDE_VS_DASH_SAI": "n"},
                          {"CONFIGURED_PLATFORM": "broadcom"}, {"ENABLE_ASAN": "y"},
                          {"ENABLE_SYNCD_RPC": "y"}, {"CROSS_BUILD_ENVIRON": "y"},
                          {"MULTIARCH_QEMU_ENVIRON": "y"},
                          {"SONIC_BAZEL_DOCKER_IMAGES": "docker-orchagent.gz"},
                          {"SONIC_BAZEL_DOCKER_IMAGES": " ".join(ARCHIVES[::2])},
                          {"SONIC_BAZEL_DBG_DOCKER_IMAGES": "docker-syncd-vs-dbg.gz"},
                          {"SONIC_P4RT": ""}, {"SONIC_P4RT_DBG": ""},
                          {"TARGET_PATH": ""}, {"DOCKER_CONFIG_ENGINE_TRIXIE": ""}):
            with self.subTest(overrides=overrides):
                result = self.run_verifier(settings=overrides)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(self.producers(), [])
                self.assertFalse((self.directory / "bazel-arguments.jsonl").exists())

    def test_missing_make_command_fails_without_validation(self):
        result = subprocess.run(["bash", "tools/bazel/ci/verify_vs_oci.sh"], cwd=self.directory,
                                env=self.environment, text=True, capture_output=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.producers(), [])
        self.assertFalse((self.directory / "bazel-arguments.jsonl").exists())


if __name__ == "__main__":
    unittest.main()
