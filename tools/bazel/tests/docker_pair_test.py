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
from tools.bazel.ci.summarize_vs_image_timings import read_owner


class VSDockerPairTest(ArchiveRecipeFixture, unittest.TestCase):
    archives = ("docker-orchagent.gz", "docker-orchagent-dbg.gz",
                "docker-syncd-vs.gz", "docker-syncd-vs-dbg.gz")
    labels = {name: ("//dockers/docker-orchagent:" if "orchagent" in name
                     else "//platform/vs/docker-syncd-vs:") + name for name in archives}

    def setUp(self):
        super().setUp()
        (self.root / "platform").symlink_to(ROOT / "platform", target_is_directory=True)
        inputs = self.root / "inputs"
        inputs.mkdir()
        self.prerequisites = {}
        for name in self.archives:
            (inputs / (name + ".source")).write_text(name)
            self.prerequisites[name] = ["inputs/" + name + ".ready",
                                        "target/.container-build-method/" + name]
        self.makefile += """
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
            self.makefile += f"{name}_BAZEL_ARGS = --define=owner={'swss' if 'orchagent' in name else 'syncd'}\n"
        self.makefile += """
include platform/vs/kvm-image.mk
include platform/vs/one-image.mk
include tools/bazel/docker.mk
INPUT_ROOT = inputs
inputs/%.ready: inputs/%.source
	@cp "$<" "$@"
# Exercise the actual installer's selected Docker prerequisite list.
target/sonic-vs.img.gz: $$(addprefix target/,$$(sonic-vs.img.gz_DOCKERS))
	@printf '%s\\n' '$(sonic-vs.img.gz_DOCKERS)' > installed-containers
target/sonic-vs.bin: $$(addprefix target/,$$(sonic-vs.bin_DOCKERS))
	@printf '%s\\n' '$(sonic-vs.bin_DOCKERS)' > installed-containers
# This innocuous goal tests the target propagated by the outer Make invocation.
installer-dispatch: $$(addprefix target/,$$(sonic-vs.bin_DOCKERS))
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

    def assert_pairs(self):
        calls = self.recorded_calls()
        builds = [call for call in calls if call["arguments"][0] == "build"]
        queries = [call for call in calls if call["arguments"][0] == "cquery"]
        self.assertEqual(len(builds), 2, calls)
        self.assertEqual(len(queries), 4, calls)
        for pair, owner in ((self.archives[:2], "swss"), (self.archives[2:], "syncd")):
            selected = next(call for call in builds if self.labels[pair[0]] in call["arguments"])
            self.assertEqual(selected["arguments"][-2:], [self.labels[name] for name in pair])
            self.assertIn("--define=owner=" + owner, selected["arguments"])
            self.assertTrue(selected["prerequisites_ready"])
            self.assertEqual(set(selected["prerequisite_sha256"]),
                             {path for name in pair for path in self.prerequisites[name]})
        for name in self.archives:
            self.assertEqual((self.target / name).read_bytes(), ("new " + name).encode())
            self.assertEqual((self.target / ".container-build-method" / name).read_text(), "bazel\n")
            self.assertIn(f"sbom target/{name} DOCKER_IMAGE services/{name}",
                          self.markers.read_text().splitlines())

    def test_normal_vs_builds_both_pairs_and_installs_only_runtime(self):
        self.assertEqual(self.installer(), list(self.archives[::2]))
        self.assert_pairs()
        self.assertFalse((self.root / "artifacts/vs/image-timings").exists())

    def test_timed_pair_profiles_each_bazel_command_and_times_both_exports(self):
        self.installer(SONIC_VS_IMAGE_TIMINGS="1")
        self.assert_pairs()
        timings = self.root / "artifacts/vs/image-timings"
        self.assertEqual({path.name for path in timings.iterdir()}, set(self.archives[::2]))
        expected_profiles = []
        for owner, debug in (self.archives[:2], self.archives[2:]):
            directory, = (timings / owner).iterdir()
            profiles = ["build", "cquery-" + owner, "cquery-" + debug]
            commands = [*profiles, "shutdown"]
            exports = ["export-" + owner, "export-" + debug]
            self.assertEqual(json.loads((directory / "recipe.json").read_text()),
                             {"bazel": commands, "profiles": profiles, "exports": exports})
            for phase in commands + exports:
                record = json.loads((directory / (phase + ".timing.json")).read_text())
                self.assertEqual((record["phase"], record["exit_code"]), (phase, 0))
                self.assertGreater(record["elapsed_seconds"], 0)
            self.assertIn("fixture query diagnostic", (directory / "make.log").read_text())
            self.assertFalse((directory / "startup-diagnostics.json").exists())
            expected_profiles.extend(f"--profile={directory}/{phase}.profile.json.gz" for phase in profiles)
        profiles = []
        for call in self.recorded_calls():
            args = call["arguments"]
            self.assertIn("--noslim_profile", args)
            profiles.extend(arg for arg in args if arg.startswith("--profile="))
        self.assertCountEqual(profiles, expected_profiles)

    def test_timed_standalone_archive_preserves_every_invocation(self):
        owner = self.archives[0]
        for _ in range(2):
            self.assert_success(self.run_make(owner, environment={"SONIC_VS_IMAGE_TIMINGS": "1"}))
        instances = list((self.root / "artifacts/vs/image-timings" / owner).iterdir())
        self.assertEqual(len(instances), 2)
        for directory in instances:
            recipe = json.loads((directory / "recipe.json").read_text())
            self.assertEqual(recipe["bazel"], ["build", "cquery-" + owner, "shutdown"])
            self.assertEqual(recipe["exports"], ["export-" + owner])
            self.assertTrue((directory / "build.timing.json").is_file())

    def test_failed_server_retains_sanitized_snapshot_before_cleanup(self):
        owner = self.archives[0]
        result = self.run_make(owner, environment={"SONIC_VS_IMAGE_TIMINGS": "1",
                                                   "FAKE_BAZEL_MODE": "server_startup_failure"})
        self.assertNotEqual(result.returncode, 0)
        build, shutdown = self.recorded_calls(("build", "shutdown"))
        self.assertEqual(build["output_base"], shutdown["output_base"])
        self.assertFalse(pathlib.Path(build["output_base"]).exists())

        owner_dir = self.root / "artifacts/vs/image-timings" / owner
        recipe, = owner_dir.iterdir()
        self.assertEqual(json.loads((recipe / "build.timing.json").read_text())["exit_code"], 37)
        receipt = recipe / "startup-diagnostics.json"
        data = json.loads(receipt.read_text())
        self.assertEqual(data["jvm"], {"available": True, "bytes_scanned": 16384,
                                       "error_phrases": ["OutOfMemoryError", "unable to create native thread"]})
        self.assertNotIn("synthetic-private-value", receipt.read_text())
        self.assertEqual(len(data["load_average_1_5_15_min"]), 3)
        self.assertGreaterEqual(data["output_base_disk"]["available_bytes"], 0)
        self.assertLess(receipt.stat().st_size, 8192)
        # Older receipts have no snapshot. Both formats must give the same dashboard row.
        summary = read_owner(owner_dir)
        self.assertTrue(summary[0]["failed"])
        receipt.unlink()
        self.assertEqual(read_owner(owner_dir), summary)

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

    def test_timing_preserves_failed_query_and_preceding_build(self):
        result = self.run_make(goals=["target/" + self.archives[2]], environment={
            "SONIC_BUILD_TARGET": "target/sonic-vs.bin",
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

    def test_debug_vs_builds_both_pairs_and_installs_only_debug(self):
        self.assertEqual(self.installer(INSTALL_DEBUG_TOOLS="y"), list(self.archives[1::2]))
        self.assert_pairs()

    def test_onie_installer_pairs_for_direct_and_propagated_targets(self):
        for propagated in (False, True):
            for debug in (False, True):
                with self.subTest(propagated=propagated, debug=debug):
                    self.calls.unlink(missing_ok=True)
                    environment = {"INSTALL_DEBUG_TOOLS": "y" if debug else "n"}
                    goal = "target/sonic-vs.bin"
                    if propagated:
                        environment["SONIC_BUILD_TARGET"] = goal
                        goal = "installer-dispatch"
                    result = self.run_make(goals=[goal], parallel=True, environment=environment)
                    self.assert_success(result)
                    self.assertEqual((self.root / "installed-containers").read_text().split(),
                                     list(self.archives[1::2] if debug else self.archives[::2]))
                    self.assert_pairs()

    def test_parallel_installer_and_explicit_variants_execute_once_per_pair(self):
        # Another Bazel caller can own the workspace convenience symlink.
        other_outputs = self.root / "other-server/bazel-out"
        (other_outputs / "archive outputs").mkdir(parents=True)
        (self.root / "bazel-out").symlink_to(other_outputs, target_is_directory=True)
        for name in self.archives:
            (other_outputs / "archive outputs" / name).write_bytes(b"other server's archive")

        cache = "--remote_cache=grpc://fixture:9092"
        result = self.run_make(goals=["target/sonic-vs.img.gz"] +
                               ["target/" + name for name in reversed(self.archives)],
                               parallel=True, environment={"BAZEL_CONTAINER_ARGS": cache})
        self.assert_success(result)
        self.assert_pairs()
        builds = self.recorded_calls(("build",))
        self.assertEqual(len({call["output_base"] for call in builds}), 2)
        for build in builds:
            labels = build["arguments"][-2:]
            queries = [call for call in self.recorded_calls(("cquery",))
                       if call["arguments"][-1] in labels]
            self.assertEqual(len(queries), 2)
            for call in [build, *queries]:
                self.assertEqual(call["output_base"], build["output_base"])
                self.assertIn(cache, call["arguments"])
                self.assertIn("--symlink_prefix=/", call["arguments"])
        self.assertCountEqual([call["output_base"] for call in self.recorded_calls(("shutdown",))],
                              [call["output_base"] for call in builds])
        self.assertTrue(all(not pathlib.Path(call["output_base"]).exists() for call in builds))

    def test_missing_debug_outputs_are_recreated_without_changing_runtime(self):
        self.installer()
        before = {name: (self.target / name).stat().st_mtime_ns for name in self.archives[::2]}
        for name in self.archives[1::2]:
            (self.target / name).unlink()
        self.calls.unlink()
        self.installer()
        self.assert_pairs()
        self.assertEqual(before, {name: (self.target / name).stat().st_mtime_ns
                                  for name in self.archives[::2]})

    def test_missing_runtime_outputs_are_recreated_for_debug_installer(self):
        self.installer(INSTALL_DEBUG_TOOLS="y")
        for name in self.archives[::2]:
            (self.target / name).unlink()
        self.calls.unlink()
        self.installer(INSTALL_DEBUG_TOOLS="y")
        self.assert_pairs()

    def test_bazel_failure_preserves_previous_archives_and_stops_installer(self):
        result = self.run_make(goals=["target/sonic-vs.img.gz"], environment={
            "FAKE_BAZEL_MODE": "build_failure"})
        self.assertNotEqual(result.returncode, 0)
        for name in self.archives:
            self.assertEqual((self.target / name).read_bytes(), b"previous image")
        self.assertFalse((self.root / "installed-containers").exists())
        self.assertFalse(self.markers.exists())

    def test_missing_debug_prerequisite_cannot_reuse_stale_pair(self):
        (self.root / "inputs" / (self.archives[1] + ".source")).unlink()
        result = self.run_make(goals=["target/sonic-vs.img.gz"])
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.recorded_calls(), [])
        self.assertEqual((self.target / self.archives[0]).read_bytes(), b"previous image")
        self.assertFalse((self.root / "installed-containers").exists())

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
        self.assertEqual([call["arguments"][0] for call in self.recorded_calls()], ["build", "cquery"])
        self.assertEqual((self.target / self.archives[1]).read_bytes(), b"previous image")

    def test_unregistered_debug_variant_keeps_runtime_recipe_independent(self):
        self.makefile = self.makefile.replace("include tools/bazel/docker.mk", """
SONIC_BAZEL_DBG_DOCKER_IMAGES = docker-syncd-vs-dbg.gz
include tools/bazel/docker.mk""")
        self.installer()
        builds = [call["arguments"] for call in self.recorded_calls()
                  if call["arguments"][0] == "build"]
        swss = next(args for args in builds if self.labels[self.archives[0]] in args)
        self.assertNotIn(self.labels[self.archives[1]], swss)
        self.assertEqual((self.target / self.archives[1]).read_bytes(), b"previous image")

    def test_changed_debug_input_is_seen_before_pair_build(self):
        self.installer()
        old = next(call["prerequisite_sha256"]["inputs/docker-orchagent-dbg.gz.ready"]
                   for call in self.recorded_calls()
                   if call["arguments"][0] == "build" and
                   self.labels[self.archives[0]] in call["arguments"])
        (self.root / "inputs/docker-orchagent-dbg.gz.source").write_text("changed debug metadata")
        self.calls.unlink()
        self.installer()
        new = next(call["prerequisite_sha256"]["inputs/docker-orchagent-dbg.gz.ready"]
                   for call in self.recorded_calls()
                   if call["arguments"][0] == "build" and
                   self.labels[self.archives[0]] in call["arguments"])
        self.assertNotEqual(old, new)
        self.assert_pairs()

    def test_non_vs_and_other_builder_phase_do_not_pair(self):
        for environment in ({"CONFIGURED_PLATFORM": "broadcom"}, {"BLDENV": "bookworm"}):
            with self.subTest(environment=environment):
                self.calls.unlink(missing_ok=True)
                self.installer(**environment)
                builds = [call["arguments"] for call in self.recorded_calls()
                          if call["arguments"][0] == "build"]
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

    def test_native_arm64_uses_only_owner_registered_pairs(self):
        self.makefile = self.makefile.replace("CONFIGURED_ARCH = amd64", "CONFIGURED_ARCH = arm64")
        self.installer()
        self.assert_pairs()
        for call in self.recorded_calls():
            self.assertIn("--platforms=@sonic_build_infra//platforms:aarch64_trixie", call["arguments"])
            self.assertIn("--host_platform=@sonic_build_infra//platforms:aarch64_trixie", call["arguments"])


if __name__ == "__main__":
    unittest.main()
