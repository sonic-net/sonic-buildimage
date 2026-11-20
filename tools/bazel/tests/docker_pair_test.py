#!/usr/bin/env python3
"""Execute VS installer dependency selection and the real grouped Bazel bridge."""

import json
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))

from tools.bazel.tests.docker_test import ArchiveRecipeFixture
from tools.bazel.ci.private_server_diagnostics import snapshot
from tools.bazel.ci.verify_vs_image_tags import batch_archives
from tools.bazel.build_container_batch import go_proxy_timed_out


OTEL_MODULE = "github.com/open-telemetry/opentelemetry-collector-contrib/receiver/otlpjsonfilereceiver"
GAZELLE_TIMEOUT = (
    "otelcol-contrib++go_deps+com_github_open_telemetry_opentelemetry_collector_contrib_receiver_"
    f'otlpjsonfilereceiver: fetch_repo: {OTEL_MODULE}@v0.144.0: Get '
    f'"https://proxy.golang.org/{OTEL_MODULE}/@v/v0.144.0.info": net/http: TLS handshake timeout'
)


def failure_events(description):
    return [
        {"id": {"unconfiguredLabel": {"label": "//external:" + description.split(":")[0]}},
         "aborted": {"reason": "LOADING_FAILURE", "description": description}},
        {"id": {"targetConfigured": {"label": "//dockers/docker-sonic-otel:docker-sonic-otel.gz"}},
         "aborted": {"reason": "ANALYSIS_FAILURE"}},
        {"id": {"buildFinished": {}}, "finished": {"exitCode": {"code": 1},
         "failureDetail": {"packageLoading": {"code": "PACKAGE_MISSING"}, "message": description}}},
        {"id": {"buildMetrics": {}}, "lastMessage": True},
    ]


class VSContainerBatchTest(ArchiveRecipeFixture, unittest.TestCase):
    archives = ("docker-orchagent.gz", "docker-orchagent-dbg.gz",
                "docker-syncd-vs.gz", "docker-syncd-vs-dbg.gz")
    labels = {name: ("//dockers/docker-orchagent:" if "orchagent" in name
                     else "//platform/vs/docker-syncd-vs:") + name for name in archives}

    def setUp(self):
        super().setUp()
        (self.root / "platform").symlink_to(ROOT / "platform", target_is_directory=True)
        (self.root / "rules").symlink_to(ROOT / "rules", target_is_directory=True)
        inputs = self.root / "inputs"
        inputs.mkdir()
        self.prerequisites = {}
        for name in self.archives:
            (inputs / (name + ".source")).write_text(name)
            self.prerequisites[name] = ["inputs/" + name + ".ready",
                                        "target/.container-build-method/" + name]
        self.makefile += """
include rules/config
CONFIGURED_PLATFORM ?= vs
BLDENV ?= trixie
DBG_IMAGE_MARK = dbg
SELECT_BAZEL ?= y
SONIC_INSTALL_DOCKER_IMAGES = docker-orchagent.gz docker-syncd-vs.gz
SONIC_INSTALL_DOCKER_DBG_IMAGES = docker-orchagent-dbg.gz docker-syncd-vs-dbg.gz
SONIC_BAZEL_SWITCHABLE_IMAGES = $(SONIC_INSTALL_DOCKER_IMAGES) $(SONIC_INSTALL_DOCKER_DBG_IMAGES)
ifeq ($(SELECT_BAZEL),y)
SONIC_BAZEL_DOCKER_IMAGES = $(SONIC_INSTALL_DOCKER_IMAGES)
SONIC_BAZEL_DBG_DOCKER_IMAGES = $(SONIC_INSTALL_DOCKER_DBG_IMAGES)
endif
"""
        for name in self.archives:
            self.makefile += f"{name}_BAZEL_TARGET = {self.labels[name]}\n"
            self.makefile += f"{name}_PATH = services/{name}\n"
            self.makefile += f"{name}_BAZEL_DEPENDS = $(INPUT_ROOT)/{name}.ready\n"
            if "syncd" in name:
                self.makefile += f"{name}_BAZEL_ARGS = --config=syncd-dash\n"
                self.makefile += f"{name}_BAZEL_SHARED_ARGS = $({name}_BAZEL_ARGS)\n"
        self.makefile += """
include platform/vs/kvm-image.mk
include platform/vs/one-image.mk
include tools/bazel/docker.mk
INPUT_ROOT = inputs
inputs/%.ready: inputs/%.source
	@cp "$<" "$@"
# Exercise the actual installer's selected Docker prerequisite list.
target/sonic-vs.img.gz: $$(addprefix target/,$$(sonic-vs.img.gz_DOCKERS) $$(SONIC_PACKAGES_LOCAL))
	@printf '%s\\n' '$(sonic-vs.img.gz_DOCKERS)' > installed-containers
target/sonic-vs.bin: $$(addprefix target/,$$(sonic-vs.bin_DOCKERS) $$(SONIC_PACKAGES_LOCAL))
	@printf '%s\\n' '$(sonic-vs.bin_DOCKERS)' > installed-containers
# This innocuous goal tests the target propagated by the outer Make invocation.
installer-dispatch: $$(addprefix target/,$$(sonic-vs.bin_DOCKERS) $$(SONIC_PACKAGES_LOCAL))
	@printf '%s\\n' '$(sonic-vs.bin_DOCKERS)' > installed-containers
LEGACY_IMAGES = $(filter-out $(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES),$(SONIC_BAZEL_SWITCHABLE_IMAGES))
$(addprefix target/,$(LEGACY_IMAGES)): target/%.gz:
	@printf 'legacy %s' '$(@F)' > "$@"
"""

    def installer(self, **environment):
        result = self.run_make(goals=["target/sonic-vs.img.gz"], parallel=True,
                               environment=environment)
        self.assert_success(result)
        self.assertNotIn("overriding recipe", result.stderr)
        self.assertNotIn("given more than once", result.stderr)
        return (self.root / "installed-containers").read_text().split()

    def assert_batch(self, archives=None):
        if archives is None:
            archives = self.archives
        calls = self.recorded_calls()
        self.assertEqual([c["operation"] for c in calls], ["build"], calls)
        build, = calls
        startup = build["arguments"][:build["arguments"].index("build")]
        self.assertTrue(any(arg.startswith("--invocation_policy=") for arg in startup), startup)
        self.assertCountEqual(build["archives"], archives)
        self.assertCountEqual([arg for arg in build["arguments"] if arg.startswith("//")],
                              [self.labels[name] for name in archives])
        self.assertTrue(build["prerequisites_ready"])
        self.assertEqual(set(build["prerequisite_sha256"]),
                         {path for name in archives for path in self.prerequisites[name]})
        for name in archives:
            self.assertEqual((self.target / name).read_bytes(), ("new " + name).encode())
            self.assertEqual((self.target / ".container-build-method" / name).read_text(), "bazel\n")
            self.assertIn(f"sbom target/{name} DOCKER_IMAGE services/{name}",
                          self.markers.read_text().splitlines())

    def test_normal_vs_builds_one_batch_and_installs_only_runtime(self):
        self.assertEqual(self.installer(), list(self.archives[::2]))
        self.assert_batch()
        self.assertFalse((self.root / "artifacts/vs/image-timings").exists())

    def test_onie_build_with_debug_layer_enabled_builds_paired_archives(self):
        result = self.run_make(goals=["target/sonic-vs.bin"],
                               environment={"SONIC_ENABLE_DEBUG_LAYER": "y"})
        self.assert_success(result)
        self.assert_batch()

    def test_default_onie_build_skips_debug_archives_and_prerequisites(self):
        for name in self.archives[1::2]:
            (self.root / "inputs" / (name + ".source")).unlink()
            (self.target / name).unlink()
        result = self.run_make(goals=["target/sonic-vs.bin"], parallel=True)
        self.assert_success(result)
        self.assert_batch(self.archives[::2])
        self.assertEqual((self.root / "installed-containers").read_text().split(), list(self.archives[::2]))
        for name in self.archives[1::2]:
            self.assertFalse((self.target / name).exists())

    def test_runtime_only_onie_build_honors_an_explicit_debug_goal(self):
        extra = self.archives[3]
        result = self.run_make(goals=["target/sonic-vs.bin", "target/" + extra], parallel=True,
                               environment={"SONIC_ENABLE_DEBUG_LAYER": "n"})
        self.assert_success(result)
        self.assert_batch((*self.archives[::2], extra))
        self.assertEqual((self.root / "installed-containers").read_text().split(), list(self.archives[::2]))

    def test_runtime_only_onie_build_rejects_installing_debug_images(self):
        result = self.run_make(goals=["target/sonic-vs.bin"],
                               environment={"SONIC_ENABLE_DEBUG_LAYER": "n", "INSTALL_DEBUG_TOOLS": "y"})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("SONIC_ENABLE_DEBUG_LAYER=n requires INSTALL_DEBUG_TOOLS", result.stderr)
        self.assertFalse(self.recorded_calls())

    def test_make_only_onie_installer_can_still_install_debug_images(self):
        result = self.run_make(goals=["target/sonic-vs.bin"],
                               environment={"SELECT_BAZEL": "n", "INSTALL_DEBUG_TOOLS": "y"})
        self.assert_success(result)
        self.assertEqual((self.root / "installed-containers").read_text().split(),
                         list(self.archives[1::2]))
        self.assertFalse(self.recorded_calls())
        for name in self.archives[1::2]:
            self.assertEqual((self.target / name).read_bytes(), ("legacy " + name).encode())

    def build_with_failures(self, failures, retry="1"):
        (self.root / "fixture-failures.json").write_text(json.dumps(failures))
        return self.run_make(goals=["target/sonic-vs.bin"], environment={
            "FAKE_BAZEL_MODE": "scripted_failures", "SONIC_VS_IMAGE_TIMINGS": "1",
            "SONIC_VS_RETRY_GO_FETCH": retry, "SONIC_ENABLE_DEBUG_LAYER": "y",
        })

    def test_go_proxy_timeout_retries_once_and_keeps_profiles_and_membership(self):
        self.assert_success(self.build_with_failures([(1, failure_events(GAZELLE_TIMEOUT))]))
        calls = self.recorded_calls()
        self.assertEqual(len(calls), 2)
        timings = self.root / "artifacts/vs/image-timings"
        self.assertEqual(batch_archives(timings), sorted(self.archives))
        codes = []
        for call in calls:
            self.assertCountEqual(call["archives"], self.archives)
            profile_arg, = [arg for arg in call["arguments"] if arg.startswith("--profile=")]
            profile = pathlib.Path(profile_arg.removeprefix("--profile="))
            self.assertGreater(profile.stat().st_size, 0)
            self.assertEqual(set(json.loads((profile.parent / "batch-targets.json").read_text())), set(self.archives))
            codes.append(json.loads((profile.parent / "build.timing.json").read_text())["exit_code"])
        self.assertEqual(codes, [1, 0])
        self.assertEqual(len(list(timings.rglob("export-*.timing.json"))), len(self.archives))
        log = (self.target / "vs-container-images.log").read_text()
        self.assertIn(GAZELLE_TIMEOUT, log)
        self.assertEqual(log.count("Retrying the container batch once."), 1)

    def test_persistent_timeout_exhausts_one_retry(self):
        failure = (1, failure_events(GAZELLE_TIMEOUT))
        result = self.build_with_failures([failure, failure, failure])
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(len(self.recorded_calls()), 2)
        self.assertEqual((self.target / self.archives[0]).read_bytes(), b"previous image")
        self.assertFalse((self.root / "installed-containers").exists())

    def test_nonretryable_failures_stop_immediately(self):
        timeout = failure_events(GAZELLE_TIMEOUT)
        mismatch = failure_events("fetch_repo: checksum mismatch")
        for failure, retry in [((1, mismatch), "1"), ((1, timeout), "0"), ((-15, timeout), "1")]:
            with self.subTest(code=failure[0], retry=retry):
                self.calls.unlink(missing_ok=True)
                result = self.build_with_failures([failure, failure], retry)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(len(self.recorded_calls()), 1)
                timings = self.root / "artifacts/vs/image-timings"
                latest = max((json.loads(p.read_text()) for p in timings.rglob("build.timing.json")),
                             key=lambda item: item["start_epoch_seconds"])
                self.assertEqual(latest["exit_code"], 143 if failure[0] == -15 else 1)

    def test_incomplete_malformed_and_mixed_build_events_do_not_retry(self):
        path = self.root / "events.json"
        timeout = failure_events(GAZELLE_TIMEOUT)
        mismatch = failure_events("checksum mismatch")
        self.assertFalse(go_proxy_timed_out(path))
        timeout[2]["finished"]["failureDetail"]["message"] = "Analysis of target failed; build aborted: Analysis failed"
        path.write_text("".join(json.dumps(event) + "\n" for event in timeout))
        self.assertTrue(go_proxy_timed_out(path))
        for events in [timeout[:-1], timeout[:1] + timeout[-1:],
                       [*timeout[:1], mismatch[0], *timeout[1:]], [*timeout[:2], *mismatch[2:]],
                       [*timeout[:1], {"action": {"success": False}}, *timeout[1:]]]:
            path.write_text("".join(json.dumps(event) + "\n" for event in events))
            self.assertFalse(go_proxy_timed_out(path))
        path.write_text("".join(json.dumps(event) + "\n" for event in timeout) + "{malformed")
        self.assertFalse(go_proxy_timed_out(path))

    def test_source_flags_reach_one_build_of_the_original_image_targets(self):
        # Exercise real FRR/gNMI registrations, including a nondefault dial-out setting.
        frr = ("docker-fpm-frr.gz", "docker-fpm-frr-dbg.gz")
        gnmi = ("docker-sonic-gnmi.gz", "docker-sonic-gnmi-dbg.gz")
        make_metadata = "--//tools/bazel/image/vs/metadata:use_make_metadata=true"
        version = "--//tools/bazel/oci:image_version=202705.42"
        features = ["--//src/sonic-frr:tcmalloc=true", "--//src/sonic-frr:snmp=true"]
        self.labels = dict(self.labels)
        for name in (*frr, *gnmi):
            self.labels[name] = "//dockers/" + name.removesuffix(".gz").removesuffix("-dbg") + ":" + name
            self.prerequisites[name] = ["target/.container-build-method/" + name]
        self.makefile = self.makefile.replace("include platform/vs/kvm-image.mk", """
BUILD_WITH_BAZEL_WHEN_AVAILABLE = y
INCLUDE_FIPS = y
ENABLE_FRR_TCMALLOC = y
ENABLE_FRR_SNMP_AGENT = y
FRR_USER_UID = 300
FRR_USER_GID = 300
SONIC_IMAGE_VERSION = 202705.42
DOCKERS_PATH = dockers
ENABLE_TRANSLIB_WRITE = y
ENABLE_NATIVE_WRITE = y
ENABLE_DIALOUT = n
include rules/docker-fpm-frr.mk
include rules/docker-gnmi.mk
# slave.mk inherits the source directory for generic debug images.
$(DOCKER_GNMI_DBG)_PATH = $($(DOCKER_GNMI)_PATH)
# The owner integration tests exercise the manifest prerequisites.
$(DOCKER_FPM_FRR)_BAZEL_DEPENDS =
$(DOCKER_FPM_FRR_DBG)_BAZEL_DEPENDS =
$(DOCKER_GNMI)_BAZEL_DEPENDS =
$(DOCKER_GNMI_DBG)_BAZEL_DEPENDS =
SONIC_PACKAGES_LOCAL += $(DOCKER_FPM_FRR) $(DOCKER_GNMI)
include platform/vs/kvm-image.mk""")
        self.installer()
        build, = self.recorded_calls()
        archives = (*self.archives, *frr, *gnmi)
        self.assertCountEqual(build["archives"], archives)
        self.assertCountEqual([arg for arg in build["arguments"] if arg.startswith("//")],
                              [self.labels[name] for name in archives])
        for option in [make_metadata, version, *features, "--@sonic_gnmi//tools/bazel:enable_dialout=false",
                       "--@sonic_sairedis//:enable_dashsai=true",
                       "--@sonic_sairedis//:dash_sai=@sonic_dash//:sdk",
                       "--@sonic_sairedis//:dash_runtime_metadata=@sonic_dash//:runtime_packages.json"]:
            self.assertEqual(build["arguments"].count(option), 1, option)
        self.assertEqual([arg for arg in build["arguments"] if ":image_version=" in arg], [version])
        # A standalone FRR build and its output query still receive every flag.
        self.calls.unlink()
        self.assert_success(self.run_make(frr[1]))
        build, query = self.recorded_calls()
        for call in (build, query):
            for option in [make_metadata, *features, version]:
                self.assertEqual(call["arguments"].count(option), 1)

    def test_profile_shared_build_once_and_time_each_export(self):
        self.installer(SONIC_VS_IMAGE_TIMINGS="1")
        self.assert_batch()
        timings = self.root / "artifacts/vs/image-timings"
        self.assertEqual({path.name for path in timings.iterdir()},
                         {"vs-container-images", *self.archives[::2]})
        shared, = (timings / "vs-container-images").iterdir()
        recipe = json.loads((shared / "recipe.json").read_text())
        self.assertEqual(recipe, {"bazel": ["build"], "profiles": ["build"], "exports": []})
        build, = self.recorded_calls()
        self.assertIn("--experimental_profile_include_target_label", build["arguments"])
        self.assertEqual(json.loads((shared / "batch-targets.json").read_text()),
                         {name: [name.replace("-dbg.gz", ".gz"), self.labels[name]] for name in self.archives})
        self.assertIn("--noslim_profile", build["arguments"])
        self.assertIn(f"--profile={shared}/build.profile.json.gz", build["arguments"])
        self.assertEqual(json.loads((shared / "build.timing.json").read_text())["exit_code"], 0)
        for owner, debug in (self.archives[:2], self.archives[2:]):
            directory, = (timings / owner).iterdir()
            recipe = json.loads((directory / "recipe.json").read_text())
            self.assertEqual(recipe["bazel"], [])
            self.assertEqual(recipe["profiles"], [])
            self.assertCountEqual(recipe["exports"], ["export-" + owner, "export-" + debug])
            for phase in recipe["exports"]:
                record = json.loads((directory / (phase + ".timing.json")).read_text())
                self.assertEqual((record["phase"], record["exit_code"]), (phase, 0))
                self.assertGreater(record["elapsed_seconds"], 0)

    def test_timed_standalone_archive_preserves_every_invocation(self):
        owner = self.archives[0]
        for _ in range(2):
            self.assert_success(self.run_make(owner, environment={"SONIC_VS_IMAGE_TIMINGS": "1"}))
        instances = list((self.root / "artifacts/vs/image-timings" / owner).iterdir())
        self.assertEqual(len(instances), 2)
        for directory in instances:
            recipe = json.loads((directory / "recipe.json").read_text())
            self.assertEqual(recipe["bazel"], ["build", "cquery-" + owner])
            self.assertEqual(recipe["exports"], ["export-" + owner])
            self.assertTrue((directory / "build.timing.json").is_file())

    def test_failure_snapshot_reads_the_process_and_enclosing_container_limits(self):
        proc = self.root / "proc"
        (proc / "self").mkdir(parents=True)
        (proc / "self/cgroup").write_text("0::/leaf\n")
        mount = self.root / "cgroup"
        leaf = mount / "leaf"
        leaf.mkdir(parents=True)
        (leaf / "memory.events").write_text("oom_kill 2\n")
        (leaf / "pids.current").write_text("91\n")
        (mount / "pids.max").write_text("128\n")
        (mount / "memory.max").write_text("4294967296\n")
        info = snapshot(self.root, proc, mount)
        self.assertFalse(info["jvm"]["available"])
        self.assertEqual(info["cgroup_v2"], {
            "process": {"memory.events": "oom_kill 2", "pids.current": "91"},
            "mount_root": {"memory.max": "4294967296", "pids.max": "128"}})

    def test_standalone_timing_preserves_failed_query_and_preceding_build(self):
        result = self.run_make(goals=["target/" + self.archives[2]], environment={
            "SONIC_VS_IMAGE_TIMINGS": "1", "FAKE_BAZEL_MODE": "query_failure"})
        self.assertNotEqual(result.returncode, 0)
        owner = self.archives[2]
        records, = (self.root / "artifacts/vs/image-timings" / owner).iterdir()
        self.assertIn("fixture query failure", (records / "make.log").read_text())
        self.assertEqual(json.loads((records / "build.timing.json").read_text())["exit_code"], 0)
        failed = json.loads((records / f"cquery-{owner}.timing.json").read_text())
        self.assertNotEqual(failed["exit_code"], 0)
        self.assertFalse(list(records.glob("export-*.timing.json")))
        self.assertEqual((self.target / owner).read_bytes(), b"previous image")

    def test_debug_vs_builds_one_batch_and_installs_only_debug(self):
        self.assertEqual(self.installer(INSTALL_DEBUG_TOOLS="y"), list(self.archives[1::2]))
        self.assert_batch()

    def test_onie_installer_batches_for_direct_and_propagated_targets(self):
        for propagated in (False, True):
            for debug in (False, True):
                with self.subTest(propagated=propagated, debug=debug):
                    self.calls.unlink(missing_ok=True)
                    environment = {"INSTALL_DEBUG_TOOLS": "y" if debug else "n"}
                    if debug:
                        environment["SONIC_ENABLE_DEBUG_LAYER"] = "y"
                    goal = "target/sonic-vs.bin"
                    if propagated:
                        environment["SONIC_BUILD_TARGET"] = goal
                        goal = "installer-dispatch"
                    result = self.run_make(goals=[goal], parallel=True, environment=environment)
                    self.assert_success(result)
                    self.assertEqual((self.root / "installed-containers").read_text().split(),
                                     list(self.archives[1::2] if debug else self.archives[::2]))
                    self.assert_batch(self.archives if debug else self.archives[::2])

    def test_parallel_installer_and_explicit_variants_run_one_build(self):
        cache = "--remote_cache=grpc://fixture:9092"
        result = self.run_make(goals=["target/sonic-vs.img.gz"] +
                               ["target/" + name for name in reversed(self.archives)],
                               parallel=True, environment={"BAZEL_CONTAINER_ARGS": cache})
        self.assert_success(result)
        self.assert_batch()
        build, = self.recorded_calls()
        self.assertIn(cache, build["arguments"])
        self.assertFalse(any(arg.startswith(("--output_base=", "--output_user_root="))
                             for arg in build["arguments"]))

    def test_only_reachable_bazel_bases_and_local_packages_join_the_batch(self):
        # Model legacy Dockerfile FROM edges as slave.mk's archive -> base-load
        # -> base archive rules. Registered Bazel archives build their bases in
        # Bazel, so their legacy _LOAD_DOCKERS should not pull extra Make outputs.
        self.makefile = self.makefile.replace("include platform/vs/kvm-image.mk", """
SONIC_BAZEL_DOCKER_IMAGES := $(SONIC_BAZEL_DOCKER_IMAGES)
SONIC_BAZEL_DBG_DOCKER_IMAGES := $(SONIC_BAZEL_DBG_DOCKER_IMAGES)
SONIC_BAZEL_SWITCHABLE_IMAGES := $(SONIC_BAZEL_SWITCHABLE_IMAGES)
SONIC_INSTALL_DOCKER_IMAGES = legacy-service.gz
SONIC_INSTALL_DOCKER_DBG_IMAGES =
legacy-service.gz_LOAD_DOCKERS = legacy-base.gz
legacy-base.gz_LOAD_DOCKERS = docker-orchagent.gz
docker-orchagent.gz_LOAD_DOCKERS = docker-syncd-vs.gz
include platform/vs/kvm-image.mk""")
        self.makefile += """
target/legacy-service.gz target/legacy-base.gz: target/%.gz: $$(addprefix target/,$$(addsuffix -load,$$($$*.gz_LOAD_DOCKERS)))
\t@touch "$@"
target/%.gz-load: target/%.gz
\t@touch "$@"
"""
        self.assertEqual(self.installer(), ["legacy-service.gz"])
        self.assert_batch(self.archives[:2])
        for name in self.archives[2:]:
            self.assertEqual((self.target / name).read_bytes(), b"previous image")
        self.calls.unlink()
        self.assertEqual(self.installer(SONIC_PACKAGES_LOCAL="docker-syncd-vs.gz"),
                         ["legacy-service.gz"])
        self.assert_batch()

    def test_native_vs_keeps_the_shared_server_when_ci_scratch_is_available(self):
        scratch = self.root / "registered-ci-job"
        scratch.mkdir()
        self.installer(SONIC_BAZEL_JOB_TEMP=str(scratch))
        self.assert_batch()
        self.assertEqual(list(scratch.iterdir()), [])
        for call in self.recorded_calls():
            self.assertFalse(any(arg.startswith(("--output_base=", "--output_user_root="))
                                 for arg in call["arguments"]))
            self.assertNotIn("shutdown", call["arguments"])

    def test_missing_debug_outputs_are_recreated_without_changing_runtime(self):
        self.installer()
        before = {name: (self.target / name).stat().st_mtime_ns for name in self.archives[::2]}
        for name in self.archives[1::2]:
            (self.target / name).unlink()
        self.calls.unlink()
        self.installer()
        self.assert_batch()
        self.assertEqual(before, {name: (self.target / name).stat().st_mtime_ns
                                  for name in self.archives[::2]})

    def test_missing_runtime_outputs_are_recreated_for_debug_installer(self):
        self.installer(INSTALL_DEBUG_TOOLS="y")
        for name in self.archives[::2]:
            (self.target / name).unlink()
        self.calls.unlink()
        self.installer(INSTALL_DEBUG_TOOLS="y")
        self.assert_batch()

    def test_bazel_failure_preserves_previous_archives_and_stops_installer(self):
        result = self.run_make(goals=["target/sonic-vs.img.gz"], environment={
            "FAKE_BAZEL_MODE": "build_failure"})
        self.assertNotEqual(result.returncode, 0)
        for name in self.archives:
            self.assertEqual((self.target / name).read_bytes(), b"previous image")
        self.assertFalse((self.root / "installed-containers").exists())
        self.assertFalse(self.markers.exists())

    def test_invalid_archive_output_fails_before_publishing_any_batch_member(self):
        result = self.run_make(goals=["target/sonic-vs.img.gz"], environment={
            "FAKE_BAZEL_MODE": "missing_output"})
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue(self.recorded_calls(("build",)))
        for name in self.archives:
            self.assertEqual((self.target / name).read_bytes(), b"previous image")
        self.assertFalse((self.root / "installed-containers").exists())
        self.assertFalse(self.markers.exists())
        # Retry the same Make request with intact Bazel outputs.
        self.calls.unlink()
        self.installer()
        self.assert_batch()

    def test_missing_debug_prerequisite_cannot_reuse_stale_pair(self):
        (self.root / "inputs" / (self.archives[1] + ".source")).unlink()
        result = self.run_make(goals=["target/sonic-vs.img.gz"])
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.recorded_calls(), [])
        self.assertEqual((self.target / self.archives[0]).read_bytes(), b"previous image")
        self.assertFalse((self.root / "installed-containers").exists())

    def test_unshared_owner_option_keeps_both_variants_on_the_standalone_recipe(self):
        runtime, debug = self.archives[:2]
        private = "--define=owner=orchagent"
        self.makefile = self.makefile.replace("include tools/bazel/docker.mk", f"""
{runtime}_BAZEL_ARGS = {private}
{debug}_BAZEL_ARGS = {private}
include tools/bazel/docker.mk""")
        self.assertEqual(self.installer(), list(self.archives[::2]))
        builds = self.recorded_calls(("build",))
        batched, = [call for call in builds if self.archives[2] in call["archives"]]
        self.assertCountEqual(batched["archives"], self.archives[2:])
        self.assertNotIn(private, batched["arguments"])
        standalone = [call for call in builds if runtime in call["archives"] or debug in call["archives"]]
        self.assertCountEqual([name for call in standalone for name in call["archives"]], [runtime, debug])
        queries = self.recorded_calls(("cquery",))
        self.assertCountEqual([name for call in queries for name in call["archives"]], [runtime, debug])
        for call in [*standalone, *queries]:
            self.assertEqual(call["arguments"].count(private), 1)
            self.assertNotIn("--config=syncd-dash", call["arguments"])
        for name in self.archives:
            self.assertEqual((self.target / name).read_bytes(), ("new " + name).encode())
            self.assertIn(f"sbom target/{name} DOCKER_IMAGE services/{name}",
                          self.markers.read_text().splitlines())

    def test_conflicting_shared_owner_values_fail_before_any_archive_is_published(self):
        additions = []
        for name in self.archives:
            value = "swss" if name in self.archives[:2] else "syncd"
            additions += [f"{name}_BAZEL_ARGS = --//fixture:mode={value}",
                          f"{name}_BAZEL_SHARED_ARGS = $({name}_BAZEL_ARGS)"]
        self.makefile = self.makefile.replace("include tools/bazel/docker.mk",
                                             "\n".join([*additions, "include tools/bazel/docker.mk"]))
        result = self.run_make(goals=["target/sonic-vs.bin"], parallel=True)
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("//fixture:mode", result.stdout + result.stderr)
        self.assertEqual(self.recorded_calls(), [])
        for name in self.archives:
            self.assertEqual((self.target / name).read_bytes(), b"previous image")
        self.assertFalse(self.markers.exists())

    def test_differing_owner_flags_are_rejected_instead_of_changing_runtime_config(self):
        self.makefile = self.makefile.replace("include tools/bazel/docker.mk", """
docker-orchagent-dbg.gz_BAZEL_ARGS = --define=owner=wrong
include tools/bazel/docker.mk""")
        result = self.run_make(goals=["target/sonic-vs.img.gz"])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("requires identical Bazel arguments", result.stderr)
        self.assertEqual(self.recorded_calls(), [])

    def test_standalone_archive_does_not_build_unrequested_debug_variant(self):
        result = self.run_make(self.archives[0])
        self.assert_success(result)
        self.assertEqual([call["operation"] for call in self.recorded_calls()], ["build", "cquery"])
        self.assertEqual((self.target / self.archives[1]).read_bytes(), b"previous image")

    def test_unregistered_debug_variant_is_excluded_from_batch(self):
        self.makefile = self.makefile.replace("include tools/bazel/docker.mk", """
SONIC_BAZEL_DBG_DOCKER_IMAGES = docker-syncd-vs-dbg.gz
include tools/bazel/docker.mk""")
        self.installer()
        self.assert_batch((self.archives[0], *self.archives[2:]))
        self.assertEqual((self.target / self.archives[1]).read_bytes(), b"previous image")

    def test_changed_debug_input_is_seen_before_batched_build(self):
        self.installer()
        old = next(call["prerequisite_sha256"]["inputs/docker-orchagent-dbg.gz.ready"]
                   for call in self.recorded_calls()
                   if call["operation"] == "build")
        (self.root / "inputs/docker-orchagent-dbg.gz.source").write_text("changed debug metadata")
        self.calls.unlink()
        self.installer()
        new = next(call["prerequisite_sha256"]["inputs/docker-orchagent-dbg.gz.ready"]
                   for call in self.recorded_calls()
                   if call["operation"] == "build")
        self.assertNotEqual(old, new)
        self.assert_batch()

    def test_non_vs_and_other_builder_phase_build_standalone(self):
        for environment in ({"CONFIGURED_PLATFORM": "broadcom"}, {"BLDENV": "bookworm"}):
            with self.subTest(environment=environment):
                self.calls.unlink(missing_ok=True)
                self.installer(**environment)
                builds = [call["arguments"] for call in self.recorded_calls()
                          if call["operation"] == "build"]
                self.assertEqual(len(builds), 2)
                self.assertTrue(all(sum(arg in self.labels.values() for arg in args) == 1
                                    for args in builds))
                for name in self.archives[1::2]:
                    self.assertEqual((self.target / name).read_bytes(), b"previous image")

    def test_make_mode_keeps_original_selection_and_switches_both_variants_back(self):
        self.installer()
        self.calls.unlink()
        self.assertEqual(self.installer(SELECT_BAZEL="n"), list(self.archives[::2]))
        self.assertEqual(self.recorded_calls(), [])
        for name in self.archives[::2]:
            self.assertEqual((self.target / name).read_bytes(), ("legacy " + name).encode())
            self.assertEqual((self.target / ".container-build-method" / name).read_text(), "make\n")
        self.assertEqual(self.installer(SELECT_BAZEL="n", INSTALL_DEBUG_TOOLS="y"),
                         list(self.archives[1::2]))
        for name in self.archives[1::2]:
            self.assertEqual((self.target / name).read_bytes(), ("legacy " + name).encode())
            self.assertEqual((self.target / ".container-build-method" / name).read_text(), "make\n")

    def test_native_arm64_uses_the_configured_host_and_target(self):
        self.makefile = self.makefile.replace("CONFIGURED_ARCH = amd64", "CONFIGURED_ARCH = arm64")
        self.installer()
        self.assert_batch()
        for call in self.recorded_calls():
            self.assertIn("--platforms=@sonic_build_infra//platforms:aarch64_trixie", call["arguments"])
            self.assertIn("--host_platform=@sonic_build_infra//platforms:aarch64_trixie", call["arguments"])


if __name__ == "__main__":
    unittest.main()
