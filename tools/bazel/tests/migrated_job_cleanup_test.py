"""Exercise migrated-runner cleanup without accessing a runner or Docker daemon."""

import contextlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

import yaml

from tools.bazel.ci import cleanup_container_bazel as subject


ROOT = Path(__file__).resolve().parents[3]
ACTION = ROOT / ".github/actions/bazel-post-job-cleanup"


class MigratedCleanupTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="migrated cleanup ")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name).resolve()
        self.workspace = self.root / "work/project/project"
        self.workspace.mkdir(parents=True)
        self.runner_temp = self.root / "work/_temp"
        self.runner_temp.mkdir()
        self.home = self.root / "home"
        self.cache = self.home / ".cache/bazel"
        self.cache.mkdir(parents=True)
        (self.cache / "shared").write_text("keep")
        environment = mock.patch.dict(os.environ, {
            "GITHUB_ACTIONS": "true", "RUNNER_ENVIRONMENT": "self-hosted",
            "GITHUB_RUN_ID": "123", "GITHUB_RUN_ATTEMPT": "2", "GITHUB_JOB": "checks",
            "GITHUB_REPOSITORY": "securely1g/project",
            "GITHUB_WORKFLOW_REF":
                "securely1g/project/.github/workflows/bazel-make-cache.yml@refs/heads/test",
            "GITHUB_WORKSPACE": str(self.workspace), "RUNNER_TEMP": str(self.runner_temp),
            "HOME": str(self.home), "BAZEL_CLEANUP_JOB_STATUS": "success",
            "BAZEL_CLEANUP_UPLOAD_STEPS": '["upload"]',
        })
        environment.start()
        self.addCleanup(environment.stop)
        self.directory = subject.prepare(self.workspace, self.runner_temp)
        self.artifacts = self.workspace / "artifacts"
        self.artifacts.mkdir()
        (self.artifacts / "evidence").write_text("keep until uploaded")
        self.steps = {"upload": {"outcome": "success"}}

    def authorize(self):
        with contextlib.redirect_stdout(io.StringIO()):
            subject.mark_artifacts(self.workspace, self.runner_temp, self.directory,
                                   self.steps, migrated=True)

    def cleanup(self, host=True):
        with mock.patch.object(subject, "repair_workspace") as repair, \
                mock.patch.object(subject, "servers", return_value=[]), \
                mock.patch.object(subject, "check_container_users"), \
                mock.patch.object(subject, "cleanup_host_bases") as bases, \
                contextlib.redirect_stdout(io.StringIO()):
            subject.cleanup(self.home, self.workspace, self.runner_temp, self.directory,
                            host=host, migrated=True)
            repair.assert_called_once_with(self.workspace, self.runner_temp, host)
            bases.assert_not_called()

    def test_success_only_removes_authorized_outputs_and_registered_temp(self):
        retained = [self.workspace / "source", self.root / "nfs-cache",
                    self.runner_temp / "other-job", self.workspace / "target"]
        for path in retained:
            path.mkdir()
            (path / "keep").write_text("keep")
        self.authorize()
        self.cleanup()
        self.assertFalse(self.artifacts.exists())
        self.assertFalse(self.directory.exists())
        self.assertTrue(all((path / "keep").is_file() for path in retained))
        self.assertEqual((self.cache / "shared").read_text(), "keep")
        self.assertTrue(self.workspace.is_dir())

    def test_failed_cancelled_or_skipped_upload_retains_evidence(self):
        for outcome in ("failure", "cancelled", "skipped"):
            with self.subTest(outcome=outcome):
                (self.directory / subject.AUTHORIZATION).unlink(missing_ok=True)
                self.steps["upload"]["outcome"] = outcome
                self.authorize()
                self.cleanup()
                self.assertTrue((self.artifacts / "evidence").is_file())
                self.assertTrue(self.directory.is_dir())

    def test_failed_job_retains_uploaded_evidence_but_removes_private_container_cache(self):
        with mock.patch.dict(os.environ, BAZEL_CLEANUP_JOB_STATUS="failure"):
            self.authorize()
        self.cleanup(host=False)
        self.assertTrue(self.artifacts.is_dir())
        self.assertTrue(self.directory.is_dir())
        self.assertFalse(self.cache.exists())

    def test_every_upload_must_complete_and_optional_skips_need_a_successful_upload(self):
        for steps, expected in (
                ({"a": {"outcome": "success"}, "b": {"outcome": "failure"}}, False),
                ({"a": {"outcome": "success"}, "b": {"outcome": "skipped"}}, True),
                ({"a": {"outcome": "skipped"}, "b": {"outcome": "skipped"}}, False)):
            self.assertEqual(subject.migrated_publication_status(
                steps, ["a", "b"], "success", True)[0], expected)
        for steps, uploads, status in (
                ({}, ["missing"], "success"), ({}, ["bad.name"], "success"),
                ({}, ["a", "a"], "success"), ({}, "upload", "success"),
                ([], [], "success"), ({}, [], "unknown")):
            with self.subTest(uploads=uploads, status=status), self.assertRaises(ValueError):
                subject.migrated_publication_status(steps, uploads, status, True)

    def test_jobs_without_uploads_preserve_unexpected_evidence(self):
        with mock.patch.dict(os.environ, BAZEL_CLEANUP_UPLOAD_STEPS="[]"):
            self.authorize()
        self.cleanup()
        self.assertTrue(self.artifacts.exists())
        self.assertTrue(self.directory.exists())

    def test_jobs_without_outputs_can_remove_their_empty_registered_directory(self):
        shutil.rmtree(self.artifacts)
        with mock.patch.dict(os.environ, BAZEL_CLEANUP_UPLOAD_STEPS="[]"):
            self.authorize()
        self.cleanup()
        self.assertFalse(self.directory.exists())
        self.assertTrue(self.cache.exists())

    def test_dashboard_only_authorizes_its_generated_site_and_history(self):
        with mock.patch.dict(os.environ, GITHUB_JOB="publish"):
            self.directory = subject.prepare(self.workspace, self.runner_temp)
            for name in ("site", "history"):
                (self.workspace / name).mkdir()
                (self.workspace / name / "data.json").write_text("{}")
            self.authorize()
            self.cleanup()
        self.assertTrue(self.artifacts.is_dir())
        self.assertFalse((self.workspace / "site").exists())
        self.assertFalse((self.workspace / "history").exists())

    def test_missing_authorization_preserves_outputs_even_after_permission_repair(self):
        self.cleanup()
        self.assertTrue(self.artifacts.is_dir())
        self.assertTrue(self.directory.is_dir())

    def test_changed_artifact_identity_and_broad_targets_are_rejected(self):
        self.authorize()
        self.artifacts.rename(self.workspace / "original-artifacts")
        self.artifacts.mkdir()
        with self.assertRaisesRegex(ValueError, "target changed"):
            self.cleanup()
        marker = self.directory / subject.AUTHORIZATION
        report = json.loads(marker.read_text())
        report["artifacts"] = {".": [self.workspace.stat().st_dev, self.workspace.stat().st_ino]}
        marker.write_text(json.dumps(report))
        with self.assertRaisesRegex(ValueError, "unexpected cleanup directory"):
            self.cleanup()
        self.assertTrue(self.workspace.is_dir())
        self.assertTrue((self.workspace / "original-artifacts/evidence").is_file())

    def test_redirected_artifacts_and_unregistered_job_directories_are_rejected(self):
        self.artifacts.rename(self.workspace / "original-artifacts")
        self.artifacts.symlink_to(self.workspace / "original-artifacts", target_is_directory=True)
        with self.assertRaises(OSError):
            self.authorize()
        self.directory = self.runner_temp
        with mock.patch.object(subject, "repair_workspace") as repair, self.assertRaises(ValueError):
            subject.cleanup(self.home, self.workspace, self.runner_temp, self.directory,
                            host=True, migrated=True)
        repair.assert_not_called()

    def test_permission_repair_uses_runner_identity_and_skips_links(self):
        with mock.patch.object(subject, "check_mounts"), \
                mock.patch.object(subject, "check_container_users") as containers, \
                mock.patch.object(subject.subprocess, "run") as run, \
                mock.patch.object(subject.os, "geteuid", return_value=123), \
                mock.patch.object(subject.os, "getegid", return_value=456):
            subject.repair_workspace(self.workspace, self.runner_temp, host=True)
        containers.assert_called_once_with(self.workspace)
        commands = [call.args[0] for call in run.call_args_list]
        self.assertEqual(len(commands), 3)
        for command in commands:
            self.assertEqual(command[:6], ["sudo", "-n", "find", "-P", str(self.workspace), "-xdev"])
            self.assertNotIn("-L", command)
            self.assertNotIn("rm", command)
        self.assertIn("123:456", commands[0])
        self.assertIn("u+rwx", commands[1])
        self.assertEqual(commands[2][6:10], ["-type", "f", "-links", "1"])

    def test_container_permission_repair_uses_parent_owner_not_root(self):
        with mock.patch.object(subject, "host_paths", return_value=(self.workspace, self.runner_temp)), \
                mock.patch.object(subject, "check_mounts"), \
                mock.patch.object(subject.Path, "stat", return_value=SimpleNamespace(st_uid=123, st_gid=456)), \
                mock.patch.object(subject.os, "geteuid", return_value=0), \
                mock.patch.object(subject.subprocess, "run") as run:
            subject.repair_workspace(self.workspace, self.runner_temp, host=False)
        self.assertEqual(run.call_args_list[0].args[0][:2], ["find", "-P"])
        self.assertIn("123:456", run.call_args_list[0].args[0])

    @unittest.skipUnless(os.geteuid() == 0, "Root is needed to create root-owned build leftovers")
    def test_real_root_owned_leftovers_are_repaired_without_following_links(self):
        os.chown(self.workspace.parent, 12345, 12345)
        nested = self.workspace / "root-owned"
        nested.mkdir(mode=0o700)
        evidence = nested / "evidence"
        evidence.write_text("root-owned")
        evidence.chmod(0)
        outside = self.root / "outside"
        outside.mkdir()
        retained = outside / "hardlinked"
        retained.write_text("do not chown")
        os.link(retained, nested / "hardlinked")
        (nested / "redirect").symlink_to(outside, target_is_directory=True)
        subject.repair_workspace(self.workspace, self.runner_temp, host=False)
        self.assertEqual(evidence.stat().st_uid, 12345)
        self.assertEqual(nested.stat().st_uid, 12345)
        self.assertEqual(nested.stat().st_mode & 0o700, 0o700)
        self.assertEqual(evidence.stat().st_mode & 0o777, 0)
        self.assertEqual(retained.stat().st_uid, 0)
        self.assertEqual(outside.stat().st_uid, 0)

    def test_mount_or_active_container_blocks_permission_changes(self):
        for helper in ("check_mounts", "check_container_users"):
            with self.subTest(helper=helper), \
                    mock.patch.object(subject, "check_mounts"), \
                    mock.patch.object(subject, "check_container_users"), \
                    mock.patch.object(subject, helper, side_effect=ValueError("busy")), \
                    mock.patch.object(subject.subprocess, "run") as run, \
                    self.assertRaisesRegex(ValueError, "busy"):
                subject.repair_workspace(self.workspace, self.runner_temp, host=True)
            run.assert_not_called()

    def test_cli_rejects_hosted_unsupported_and_redirected_contexts(self):
        for environment in (
                {"RUNNER_ENVIRONMENT": "github-hosted"},
                {"GITHUB_WORKFLOW_REF": "other/project/.github/workflows/bazel-make-cache.yml@ref"},
                {"GITHUB_WORKFLOW_REF": "securely1g/project/.github/workflows/unrelated.yml@ref"},
                {"GITHUB_JOB": "unsupported"}, {"GITHUB_ACTIONS": "false"},
                {"GITHUB_WORKSPACE": str(self.root)}, {"GITHUB_REPOSITORY": "securely1g/other"}):
            with self.subTest(environment=environment), \
                    mock.patch.dict(os.environ, environment), \
                    mock.patch.object(sys, "argv", ["cleanup", "--migrated-job", "--prepare"]), \
                    mock.patch.object(subject.Path, "is_file", return_value=False), \
                    contextlib.redirect_stderr(io.StringIO()), \
                    mock.patch.object(subject, "prepare") as prepare, self.assertRaises(SystemExit):
                subject.main()
            prepare.assert_not_called()

    def test_host_cli_runs_the_complete_lifecycle(self):
        with mock.patch.object(subject.Path, "is_file", return_value=False):
            with mock.patch.object(sys, "argv", ["cleanup", "--migrated-job", "--prepare"]), \
                    contextlib.redirect_stdout(io.StringIO()) as output:
                subject.main()
            directory = Path(json.loads(output.getvalue())["directory"])
            with mock.patch.dict(os.environ, SONIC_BAZEL_JOB_TEMP=str(directory),
                                 BAZEL_CLEANUP_STEPS=json.dumps(self.steps)), \
                    mock.patch.object(sys, "argv", ["cleanup", "--migrated-job", "--mark-artifacts"]), \
                    contextlib.redirect_stdout(io.StringIO()):
                subject.main()
            with mock.patch.object(sys, "argv", [
                    "cleanup", "--migrated-job", "--job-directory", str(directory)]), \
                    mock.patch.object(subject, "repair_workspace"), \
                    mock.patch.object(subject, "servers", return_value=[]), \
                    mock.patch.object(subject, "check_container_users"), \
                    contextlib.redirect_stdout(io.StringIO()):
                subject.main()
        self.assertFalse(directory.exists())
        self.assertFalse(self.artifacts.exists())
        self.assertTrue(self.cache.exists())


class MigratedWorkflowCleanupTest(unittest.TestCase):
    def test_every_stateful_migrated_job_registers_before_cache_setup(self):
        self.assertEqual(sum(map(len, subject.MIGRATED_JOBS.values())), 28)
        for filename, names in subject.MIGRATED_JOBS.items():
            value = yaml.safe_load((ROOT / ".github/workflows" / filename).read_text())
            actual = {name for name, job in value["jobs"].items()
                      if any(step.get("with", {}).get("migrated-job") == "true"
                             for step in job["steps"])}
            self.assertEqual(actual, names)
            for name in names:
                with self.subTest(workflow=filename, job=name):
                    job = value["jobs"][name]
                    steps = job["steps"]
                    register = next(i for i, step in enumerate(steps)
                                    if step.get("id") == "runner_cleanup")
                    checkout = next(i for i, step in enumerate(steps)
                                    if step.get("uses", "").startswith("actions/checkout@"))
                    self.assertEqual(register, checkout + 1)
                    self.assertEqual(steps[register]["if"], "runner.environment == 'self-hosted'")
                    for i, step in enumerate(steps):
                        if "setup-bazel@" in step.get("uses", ""):
                            self.assertLess(register, i)
                    final = steps[-1]
                    self.assertEqual(final["if"], "always() && steps.runner_cleanup.outcome == 'success'")
                    self.assertIn("--migrated-job --mark-artifacts", final["run"])
                    uploads = [step["id"] for step in steps if step.get("uses", "").startswith(
                        ("actions/upload-artifact@", "actions/upload-pages-artifact@"))]
                    self.assertEqual(json.loads(final["env"]["BAZEL_CLEANUP_UPLOAD_STEPS"]), uploads)
                    self.assertEqual(final["env"]["BAZEL_CLEANUP_JOB_STATUS"], "${{ job.status }}")
                    if "container" in job:
                        self.assertIn("--init", job["container"]["options"])
                    else:
                        repair = steps[checkout - 1]
                        self.assertEqual(repair["name"], "Restore workspace access before checkout")
                        self.assertEqual(repair["working-directory"], "${{ runner.temp }}")
                        self.assertEqual(repair["if"], "runner.environment == 'self-hosted'")
                        self.assertIn('sudo -n find -P "$expected" -xdev', repair["run"])
            for event in value.get("on", value.get(True)).values():
                if isinstance(event, dict) and "paths" in event:
                    self.assertIn(".github/actions/bazel-post-job-cleanup/**", event["paths"])
                    self.assertIn("tools/bazel/ci/cleanup_container_bazel.py", event["paths"])
                    self.assertIn("tools/bazel/tests/migrated_job_cleanup_test.py", event["paths"])

    def test_only_the_five_stateless_migrated_jobs_need_no_workspace_hook(self):
        stateless = set()
        for filename, stateful in subject.MIGRATED_JOBS.items():
            jobs = yaml.safe_load((ROOT / ".github/workflows" / filename).read_text())["jobs"]
            for name, job in jobs.items():
                if "ubuntu-x64" not in str(job.get("runs-on")) or name in stateful:
                    continue
                stateless.add((filename, name))
                for step in job["steps"]:
                    self.assertNotIn("checkout@", step.get("uses", ""))
                    self.assertNotIn("setup-bazel@", step.get("uses", ""))
                    self.assertNotIn("upload-artifact@", step.get("uses", ""))
        self.assertEqual(stateless, {
            ("bazel-oci.yml", "database_acceptance"), ("bazel-oci.yml", "gnmi_acceptance"),
            ("bazel-oci.yml", "mgmt_framework_acceptance"), ("bazel-oci.yml", "radv_acceptance"),
            ("build-dashboard.yml", "deploy"),
        })
        validator = yaml.safe_load(
            (ROOT / ".github/workflows/runtime-image-validation.yml").read_text())["jobs"]["native-tests"]
        self.assertEqual(validator["strategy"]["matrix"]["include"][1]["runner"], "ubuntu-24.04-arm")
        self.assertTrue(any("tools.bazel.tests.migrated_job_cleanup_test" in step.get("run", "")
                            for step in validator["steps"]))

    def test_precheckout_repair_rejects_redirects_and_runs_outside_checkout(self):
        job = yaml.safe_load((ROOT / ".github/workflows/bazel-make-cache.yml").read_text())["jobs"]["checks"]
        step = next(step for step in job["steps"] if step.get("name") == "Restore workspace access before checkout")
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()
            checkout = root / "work/project/project"
            checkout.mkdir(parents=True)
            commands = root / "commands"
            commands.mkdir()
            sudo = commands / "sudo"
            sudo.write_text("#!/bin/sh\nprintf '%s\\n' \"$*\" >> \"$CALLS\"\n")
            sudo.chmod(0o755)
            calls = root / "calls"
            environment = dict(os.environ, GITHUB_REPOSITORY="securely1g/project",
                               GITHUB_WORKSPACE=str(checkout),
                               RUNNER_PIPELINE_WORKSPACE=str(checkout.parent), CALLS=str(calls),
                               PATH=str(commands) + os.pathsep + os.environ["PATH"])
            result = subprocess.run(["bash", "-c", step["run"]], env=environment,
                                    cwd=root, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(len(calls.read_text().splitlines()), 3)
            original = checkout.with_name("original")
            checkout.rename(original)
            checkout.symlink_to(original, target_is_directory=True)
            result = subprocess.run(["bash", "-c", step["run"]], env=environment,
                                    cwd=root, capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("redirected workspace", result.stderr)
            self.assertEqual(len(calls.read_text().splitlines()), 3)

    @unittest.skipUnless(shutil.which("node"), "Node is needed for the action fixture")
    def test_action_preserves_migrated_mode_for_the_post_hook(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            commands = root / "commands"
            commands.mkdir()
            python = commands / "python3"
            python.write_text("#!" + sys.executable + "\n" + """
import json, os, pathlib, sys
pathlib.Path(os.environ["CALLS"]).write_text(json.dumps(sys.argv[1:]))
if "--prepare" in sys.argv:
    print(json.dumps({"directory": os.environ["JOB_TEMP"]}))
else:
    sys.exit(19)
""")
            python.chmod(0o755)
            environment = dict(os.environ, GITHUB_WORKSPACE=str(root),
                               GITHUB_ENV=str(root / "env"), GITHUB_STATE=str(root / "state"),
                               CALLS=str(root / "calls"), JOB_TEMP=str(root / "job-temp"),
                               PATH=str(commands) + os.pathsep + os.environ["PATH"])
            environment["INPUT_MIGRATED-JOB"] = "true"
            result = subprocess.run(["node", str(ACTION / "main.js")], env=environment,
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("--migrated-job", json.loads((root / "calls").read_text()))
            for line in (root / "state").read_text().splitlines():
                key, value = line.split("=", 1)
                environment["STATE_" + key] = value
            result = subprocess.run(["node", str(ACTION / "post.js")], env=environment,
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 19, result.stderr)
            self.assertIn("--migrated-job", json.loads((root / "calls").read_text()))
            environment["INPUT_MIGRATED-JOB"] = "invalid"
            result = subprocess.run(["node", str(ACTION / "main.js")], env=environment,
                                    capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("must be true or false", result.stderr)


if __name__ == "__main__":
    unittest.main()
