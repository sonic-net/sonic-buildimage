"""Execute the separate VS build and validation steps with controlled producers."""

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import yaml


ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.ci.build_dashboard.breakdown import parse_recipe_log


class VsBuildWorkflowTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="vs-build-workflow-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.scripts = self.root / "tools/bazel/ci"
        self.scripts.mkdir(parents=True)
        (self.root / "scripts").mkdir()
        (self.root / "scripts/run_with_retry").symlink_to(ROOT / "scripts/run_with_retry")
        self.commands = self.root / "commands"
        self.commands.mkdir()
        self.revision = self.root / "revision"
        self.revision.write_text("a" * 40 + "\n")
        for name in ("build_vs.sh", "validate_vs.sh"):
            shutil.copyfile(ROOT / "tools/bazel/ci" / name, self.scripts / name)
        recorder = "#!" + sys.executable + "\n" + r'''
import json, os, pathlib, sys
args = sys.argv[1:]
name = pathlib.Path(sys.argv[0]).name
action = "make" if name == "make" else args[0]
goal = next((arg for arg in args if arg in (
    "target/sonic-vs.bin", "bookworm", "sonic-slave-run", "docker-cleanup")), None)
with pathlib.Path("calls.jsonl").open("a") as output:
    output.write(json.dumps({"action": action, "goal": goal, "args": args,
        "bazel_args": os.environ.get("BAZEL_CONTAINER_ARGS"),
        "override_build_vars": os.environ.get("SONIC_OVERRIDE_BUILD_VARS"),
        "build_vars": os.environ.get("SONIC_BUILD_VARS"),
        "build_revision": pathlib.Path("artifacts/vs/build-revision.txt").read_text().strip()}) + "\n")
print("controlled " + action)
if os.environ.get("FAIL_ONCE_GOAL") == goal and goal is not None:
    failed = pathlib.Path("failed-once")
    if not failed.exists():
        failed.touch()
        sys.exit(17)
if os.environ.get("CHANGE_REVISION_ACTION") and os.environ["CHANGE_REVISION_ACTION"] in (action, args[0], goal):
    pathlib.Path("revision").write_text("b" * 40 + "\n")
if os.environ.get("FAIL_ACTION") and os.environ["FAIL_ACTION"] in (action, args[0], goal):
    sys.exit(17)
'''
        (self.commands / "make").write_text(recorder)
        (self.commands / "make").chmod(0o755)
        (self.scripts / "vs_archive_reuse.py").write_text(recorder)
        # The build records Git identity before Make; Python checks and timing
        # belong exclusively to the separate validation helper.
        gate = "#!" + sys.executable + "\n" + r'''
import os, pathlib, sys
name = pathlib.Path(sys.argv[0]).name
if name != "git" and os.environ.get("FORBID_VALIDATION") == "1":
    sys.exit("The build step must not call " + name)
if name == "git":
    assert sys.argv[1:] == ["rev-parse", "HEAD"]
    print(pathlib.Path("revision").read_text().strip())
else:
    os.execv(sys.executable, [sys.executable, *sys.argv[1:]])
'''
        for name in ("git", "python3"):
            path = self.commands / name
            path.write_text(gate)
            path.chmod(0o755)
        self.environment = dict(
            os.environ, PATH=str(self.commands) + os.pathsep + os.environ["PATH"],
            SONIC_CI_CONTAINER_LABEL="vs-fixture-label", BAZEL="fixture-bazel",
            BAZEL_CONTAINER_ARGS="--distdir='/source downloads' --jobs=3",
            SONIC_BUILD_RETRY_COUNT="0",
        )
        for name in ("MAKEFLAGS", "MFLAGS", "MAKEOVERRIDES", "MAKELEVEL", "FAIL_ACTION",
                     "FORBID_VALIDATION", "CHANGE_REVISION_ACTION", "NOBOOKWORM", "NOTRIXIE",
                     "SONIC_OVERRIDE_BUILD_VARS", "SONIC_BUILD_VARS", "FAIL_ONCE_GOAL",
                     "SONIC_VS_IMAGE_TIMINGS", "SONIC_VS_RETRY_GO_FETCH", "SONIC_ENABLE_DEBUG_LAYER",
                     "DOCKER_LOAD_ON_BOOT", "SONIC_VS_DEFER_BUILDER_CLEANUP"):
            self.environment.pop(name, None)

    def run_script(self, name, *args, **environment):
        env = dict(self.environment)
        for key, value in environment.items():
            if value is None:
                env.pop(key, None)
            else:
                env[key] = value
        return subprocess.run(["bash", str(self.scripts / name), *args], cwd=self.root,
                              env=env, text=True, capture_output=True, check=False)

    def calls(self):
        path = self.root / "calls.jsonl"
        return [json.loads(line) for line in path.read_text().splitlines()] if path.exists() else []

    def prepare_build(self):
        result = self.run_script("build_vs.sh", FORBID_VALIDATION="1", SONIC_ENABLE_DEBUG_LAYER="y")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        (self.root / "calls.jsonl").unlink()

    def profile_options(self):
        return [
            "BUILD_WITH_BAZEL_WHEN_AVAILABLE=y", "INCLUDE_VS_DASH_SAI=y", "INCLUDE_FIPS=y",
            "ENABLE_ASAN=n", "ENABLE_SYNCD_RPC=n",
            "SONIC_BUILDER_EXTRA_CMDLINE=--label vs-fixture-label -e BAZEL=fixture-bazel -e BAZEL_CONTAINER_ARGS",
        ]

    def assert_profile(self, call, target, debug_layer, load_on_boot="n"):
        self.assertEqual(call["args"], [target, *self.profile_options(), f"DOCKER_LOAD_ON_BOOT={load_on_boot}",
                                        f"SONIC_ENABLE_DEBUG_LAYER={debug_layer}"])
        self.assertEqual(call["bazel_args"], self.environment["BAZEL_CONTAINER_ARGS"])

    def test_build_records_revision_before_one_make_without_validation_receipts(self):
        result = self.run_script("build_vs.sh", FORBID_VALIDATION="1")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual([call["action"] for call in self.calls()], ["make"])
        self.assert_profile(self.calls()[0], "target/sonic-vs.bin", "n")
        self.assertEqual(self.calls()[0]["build_revision"], "a" * 40)
        self.assertEqual([path.name for path in (self.root / "artifacts/vs").iterdir()], ["build-revision.txt"])

    def test_deferred_cleanup_reaches_installer_make(self):
        result = self.run_script("build_vs.sh", SONIC_VS_DEFER_BUILDER_CLEANUP="y")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("SONIC_VS_DEFER_BUILDER_CLEANUP=y", self.calls()[0]["args"])

    def test_explicit_debug_layer_reaches_installer_make(self):
        result = self.run_script("build_vs.sh", SONIC_ENABLE_DEBUG_LAYER="y")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assert_profile(self.calls()[0], "target/sonic-vs.bin", "y")

    def test_ci_builds_and_verifies_the_boot_bundle(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())
        jobs = workflow["jobs"]
        builders = [name for name, job in jobs.items()
                    if any("bash tools/bazel/ci/build_vs.sh" in step.get("run", "")
                           for step in job.get("steps", []))]
        self.assertEqual(builders, ["vs_container"])
        build = next(step for step in jobs["vs_container"]["steps"] if step.get("id") == "native_build")
        self.assertEqual(build["env"]["DOCKER_LOAD_ON_BOOT"], "y")
        self.assertEqual(build["env"]["SONIC_VS_DEFER_BUILDER_CLEANUP"], "y")
        verify = jobs["installer_validation"]
        self.assertEqual(verify["needs"], "vs_container")
        self.assertTrue(any("--expect-boot-bundle" in step.get("run", "") for step in verify["steps"]))
        result = self.run_script("build_vs.sh", DOCKER_LOAD_ON_BOOT="y")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assert_profile(self.calls()[0], "target/sonic-vs.bin", "n", load_on_boot="y")
        timed = self.run_script("build_vs.sh", DOCKER_LOAD_ON_BOOT="y", SONIC_VS_IMAGE_TIMINGS="1")
        self.assertEqual(timed.returncode, 0, timed.stderr)
        self.assertIn("-e SONIC_BOOT_BUNDLE_EVIDENCE_DIR=artifacts/vs/image-timings/boot-bundle",
                      " ".join(self.calls()[-1]["args"]))

    def test_runtime_only_installer_and_validation_use_only_runtime_goals(self):
        result = self.run_script("build_vs.sh", SONIC_ENABLE_DEBUG_LAYER="n")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assert_profile(self.calls()[0], "target/sonic-vs.bin", "n")
        result = self.run_script("validate_vs.sh", "--runtime-only")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual([call["goal"] for call in self.calls()], ["target/sonic-vs.bin", "sonic-slave-run"])
        self.assertIn("SONIC_RUN_CMDS=bash tools/bazel/ci/verify_vs_oci.sh --runtime-only $(SONIC_BUILD_INSTRUCTION)",
                      self.calls()[1]["args"])

    def test_build_failure_propagates_without_a_followup(self):
        result = self.run_script("build_vs.sh", FAIL_ACTION="make", FORBID_VALIDATION="1")
        self.assertEqual(result.returncode, 17, result.stdout + result.stderr)
        self.assertEqual([call["action"] for call in self.calls()], ["make"])
        self.assertEqual([path.name for path in (self.root / "artifacts/vs").iterdir()], ["build-revision.txt"])

    def test_profile_opt_in_reaches_the_builder(self):
        result = self.run_script("build_vs.sh", SONIC_VS_IMAGE_TIMINGS="1")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(self.profile_options()[-1] + " -e SONIC_VS_IMAGE_TIMINGS", self.calls()[0]["args"])

    def test_go_fetch_retry_reaches_the_full_installer_builder(self):
        result = self.run_script("build_vs.sh", SONIC_VS_RETRY_GO_FETCH="1")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(self.profile_options()[-1] + " -e SONIC_VS_RETRY_GO_FETCH", self.calls()[0]["args"])
        workflows = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())["jobs"]
        enabled = [step["id"] for job in workflows.values() for step in job.get("steps", [])
                   if step.get("env", {}).get("SONIC_VS_RETRY_GO_FETCH") == "1"]
        self.assertCountEqual(enabled, ["native_build"])

    def test_native_handoff_is_forwarded_to_the_build_and_separate_validation_container(self):
        option = self.profile_options()[-1] + " -e SONIC_VS_REUSE_BAZEL_PRODUCERS"
        for script, goal in (("build_vs.sh", "target/sonic-vs.bin"), ("validate_vs.sh", "sonic-slave-run")):
            result = self.run_script(script, SONIC_VS_REUSE_BAZEL_PRODUCERS="1")
            self.assertEqual(result.returncode, 0, result.stderr)
            call = next(call for call in self.calls() if call["goal"] == goal)
            self.assertIn(option, call["args"])

    def test_extra_arguments_and_old_calibration_are_rejected_before_make(self):
        for name in ("build_vs.sh", "validate_vs.sh"):
            for argument in ("--measure-redundant-request", "target/docker-orchagent.gz"):
                with self.subTest(name=name, argument=argument):
                    result = self.run_script(name, argument)
                    self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                    self.assertEqual(self.calls(), [])

    def test_build_requires_the_existing_container_label_and_bazel_setting(self):
        for name in ("SONIC_CI_CONTAINER_LABEL", "BAZEL"):
            with self.subTest(name=name):
                result = self.run_script("build_vs.sh", **{name: None})
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(self.calls(), [])

    def test_validation_captures_side_outputs_before_followup_then_verifies_them(self):
        self.prepare_build()
        result = self.run_script("validate_vs.sh")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        calls = self.calls()
        self.assertEqual([call["action"] for call in calls], ["capture-pairs", "make", "make", "make", "verify-pairs"])
        for call, build_env, goal in zip(calls[1:4], ["bookworm", "trixie", "bookworm"],
                                          ["bookworm", "sonic-slave-run", "docker-cleanup"]):
            expected = ["-f", "Makefile.work", "BLDENV=" + build_env, goal, *self.profile_options()]
            if goal == "sonic-slave-run":
                expected.append("SONIC_RUN_CMDS=bash tools/bazel/ci/verify_vs_oci.sh $(SONIC_BUILD_INSTRUCTION)")
            self.assertEqual(call["args"], expected)
            self.assertEqual(call["bazel_args"], self.environment["BAZEL_CONTAINER_ARGS"])
        self.assertEqual(calls[0]["args"][1:], calls[-1]["args"][1:])
        self.assertIn("--receipt", calls[0]["args"])
        self.assertIn("--orchagent-receipt", calls[0]["args"])
        timing = json.loads((self.root / "artifacts/vs/runtime-debug-pairs-timing.json").read_text())
        self.assertEqual(timing["schema"], 2)
        self.assertEqual(timing["revision"], "a" * 40)
        self.assertFalse(timing["profile"]["install_debug_tools"])
        self.assertGreaterEqual(timing["p4_validation_seconds"], 0)
        self.assertGreaterEqual(timing["pair_validation_seconds"], 0)
        self.assertAlmostEqual(timing["validation_seconds"],
                               timing["p4_validation_seconds"] + timing["pair_validation_seconds"])
        self.assertEqual(set(timing), {"schema", "revision", "profile", "p4_validation_seconds",
                                       "pair_validation_seconds", "validation_seconds", "scope"})
        self.assertFalse((self.root / "artifacts/vs/orchagent-reuse-timing.json").exists())

    def test_validation_failures_stop_before_later_checks_or_success_timing(self):
        self.prepare_build()
        actions = ["capture-pairs", "bookworm", "sonic-slave-run", "docker-cleanup", "verify-pairs"]
        for index, action in enumerate(actions):
            with self.subTest(action=action):
                (self.root / "calls.jsonl").unlink(missing_ok=True)
                result = self.run_script("validate_vs.sh", FAIL_ACTION=action)
                self.assertEqual(result.returncode, 17, result.stdout + result.stderr)
                self.assertEqual([call["goal"] or call["action"] for call in self.calls()], actions[:index + 1])
                self.assertFalse((self.root / "artifacts/vs/runtime-debug-pairs-timing.json").exists())

    def test_validation_can_skip_bookworm_preparation_but_keeps_cleanup(self):
        self.prepare_build()
        result = self.run_script("validate_vs.sh", NOBOOKWORM="1")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual([call["goal"] or call["action"] for call in self.calls()],
                         ["capture-pairs", "sonic-slave-run", "docker-cleanup", "verify-pairs"])

    def test_caller_overrides_are_combined_in_the_environment_for_every_builder(self):
        self.prepare_build()
        existing = "KEEP_SETTING=first PACKAGE_REVISION=legacy"
        configured = "USER_FEATURE=enabled CUSTOM_SOURCE=/configured/source"
        result = self.run_script("validate_vs.sh", SONIC_OVERRIDE_BUILD_VARS=existing,
                                 SONIC_BUILD_VARS=configured)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        make_calls = [call for call in self.calls() if call["action"] == "make"]
        self.assertEqual([call["goal"] for call in make_calls], ["bookworm", "sonic-slave-run", "docker-cleanup"])
        for call in make_calls:
            self.assertEqual(call["override_build_vars"], existing + " " + configured)
            self.assertEqual(call["build_vars"], configured)
            self.assertFalse(any(arg.startswith("SONIC_OVERRIDE_BUILD_VARS=") for arg in call["args"]),
                             "A command-line assignment would suppress Makefile.work's version-cache additions")

    def test_validation_rejects_disabled_trixie_before_checks_or_builders(self):
        self.prepare_build()
        result = self.run_script("validate_vs.sh", NOTRIXIE="1")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Trixie", result.stderr)
        self.assertEqual(self.calls(), [])

    def test_validation_retries_the_failed_builder_without_recapturing_archives(self):
        self.prepare_build()
        result = self.run_script("validate_vs.sh", FAIL_ONCE_GOAL="sonic-slave-run",
                                 SONIC_BUILD_RETRY_COUNT="1", SONIC_BUILD_RETRY_INTERVAL="1")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual([call["goal"] or call["action"] for call in self.calls()],
                         ["capture-pairs", "bookworm", "sonic-slave-run", "sonic-slave-run",
                          "docker-cleanup", "verify-pairs"])
        self.assertIn("Waiting 1 to run again", result.stderr)

    def test_missing_build_revision_stops_before_any_validation_or_make(self):
        result = self.run_script("validate_vs.sh")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Missing VS build revision record", result.stderr)
        self.assertEqual(self.calls(), [])

    def test_revision_change_during_build_or_between_steps_stops_validation(self):
        for when in ("during-build", "between-steps"):
            with self.subTest(when=when):
                self.revision.write_text("a" * 40 + "\n")
                (self.root / "calls.jsonl").unlink(missing_ok=True)
                result = self.run_script("build_vs.sh", CHANGE_REVISION_ACTION=(
                    "target/sonic-vs.bin" if when == "during-build" else ""))
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(self.calls()[0]["build_revision"], "a" * 40)
                if when == "between-steps":
                    self.revision.write_text("b" * 40 + "\n")
                result = self.run_script("validate_vs.sh")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Source revision changed since the VS build started", result.stderr)
                self.assertEqual([call["action"] for call in self.calls()], ["make"])
                self.assertFalse((self.root / "artifacts/vs/runtime-debug-pairs-timing.json").exists())

    def test_revision_change_during_validation_cannot_produce_success_timing(self):
        self.prepare_build()
        result = self.run_script("validate_vs.sh", CHANGE_REVISION_ACTION="verify-pairs")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Source revision changed during VS validation", result.stderr)
        self.assertEqual([call["action"] for call in self.calls()], ["capture-pairs", "make", "make", "make", "verify-pairs"])
        self.assertFalse((self.root / "artifacts/vs/runtime-debug-pairs-timing.json").exists())

    def test_vs_installer_retains_all_bazel_recipe_diagnostics_before_validation(self):
        job = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())["jobs"]["vs_container"]
        build = next(step for step in job["steps"] if step.get("id") == "native_build")
        self.assertEqual(build["env"]["SONIC_VS_IMAGE_TIMINGS"], "1")
        self.assertNotIn("SONIC_VS_IMAGE_TIMINGS", job["env"])
        self.assertEqual(job["env"]["SONIC_VS_REUSE_BAZEL_PRODUCERS"], "1")

        upload = next(step for step in job["steps"] if step.get("id") == "vs_image_timings_upload")
        self.assertEqual(upload["if"], "always()")
        self.assertTrue(upload["continue-on-error"])
        self.assertEqual(upload["with"]["name"], "sonic-vs-bazel-timings-amd64")
        checkout = job["defaults"]["run"]["working-directory"] + "/"
        self.assertEqual(upload["with"]["path"], checkout + "artifacts/vs/image-timings/")
        summary = next(step for step in job["steps"] if "summarize_vs_image_timings.py" in step.get("run", ""))
        validation = next(step for step in job["steps"] if step.get("id") == "bundles")
        self.assertLess(job["steps"].index(build), job["steps"].index(summary))
        self.assertLess(job["steps"].index(summary), job["steps"].index(upload))
        self.assertLess(job["steps"].index(upload), job["steps"].index(validation))
        self.assertIn("steps.vs_image_timings_upload.outcome == 'success'", job["steps"][-1]["if"])

    def test_dashboard_reads_measurements_from_the_real_summary_step(self):
        job = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())["jobs"]["vs_container"]
        command = next(step["run"] for step in job["steps"] if step["name"] == "Summarize VS Bazel build timings")
        name = "summarize_vs_image_timings.py"
        (self.scripts / name).symlink_to(ROOT / "tools/bazel/ci" / name)
        timings = self.root / "artifacts/vs/image-timings"
        directory = timings / "docker-database.gz/run1"
        directory.mkdir(parents=True)
        phase = "export-docker-database.gz"
        (directory / "recipe.json").write_text(json.dumps({
            "bazel": ["build", "cquery-runtime", "cquery-debug"],
            "profiles": ["build", "cquery-runtime", "cquery-debug"], "exports": [phase, "export-debug"]}))
        (directory / (phase + ".timing.json")).write_text(json.dumps({
            "phase": phase, "elapsed_seconds": 1.25, "exit_code": 0,
        }))
        summary = self.root / "step_summary"
        result = subprocess.run(["bash", "-c", command], cwd=self.root, text=True, capture_output=True,
                                env=dict(os.environ, GITHUB_STEP_SUMMARY=str(summary)), check=True)
        self.assertEqual(summary.read_text(), (timings / "summary.md").read_text())
        self.assertIn("1.25 (partial)", summary.read_text())
        stamp = "2026-11-14T17:05:00Z"
        lines = ["##[group]Run set -euo pipefail", *command.splitlines(), "##[endgroup]",
                 *result.stdout.splitlines()]
        log = "".join(f"{stamp} {line}\n" for line in lines).encode()
        detail = parse_recipe_log(log, {"started_at": stamp, "completed_at": stamp,
                                        "name": "Summarize VS Bazel build timings"})
        receipt = json.loads((timings / "summary.json").read_text())
        self.assertEqual(detail, dict(receipt, status="available"))
        database, = detail["recipes"]
        self.assertEqual(database["owner"], "docker-database.gz")
        self.assertEqual((database["exports"], database["export_seconds"], database["compile_seconds"]),
                         (1, 1.25, None))


if __name__ == "__main__":
    unittest.main()
