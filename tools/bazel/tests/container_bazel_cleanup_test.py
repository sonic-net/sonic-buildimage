"""Check post-cache-save cleanup without touching shared caches or other jobs."""

import contextlib
import io
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

import yaml

from tools.bazel.ci import cleanup_container_bazel as subject


ROOT = Path(__file__).resolve().parents[3]
ACTION = ROOT / ".github/actions/bazel-post-job-cleanup"


class ContainerBazelCleanupTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="container cleanup ")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name).resolve()
        self.workspace = self.root / "checkout"
        self.workspace.mkdir()
        self.runner_temp = self.root / "_temp"
        self.runner_temp.mkdir()
        self.home = self.root / "home"
        self.cache = self.home / ".cache"
        self.cache.mkdir(parents=True)
        environment = mock.patch.dict(
            os.environ, GITHUB_RUN_ID="123", GITHUB_RUN_ATTEMPT="2", GITHUB_JOB="archive")
        environment.start()
        self.addCleanup(environment.stop)
        self.directory = subject.prepare(self.workspace, self.runner_temp)
        self.steps = {"public_artifacts": {"outcome": "success"},
                      "bazel_upload_public_artifacts": {"outcome": "success"},
                      "failed_validator": {"outcome": "failure"}}
        self.artifacts = self.workspace / "artifacts"
        self.artifacts.mkdir()
        (self.artifacts / "image.gz").write_bytes(b"published image")
        (self.directory / "pre-sysmgr-artifacts").mkdir()
        (self.directory / "pre-sysmgr-artifacts" / "image.gz").write_bytes(b"preserved image")
        for name in subject.LOCAL_CACHES:
            (self.cache / name).mkdir()
            (self.cache / name / "data").write_bytes(b"cache")

    def authorize(self):
        subject.mark_artifacts(self.workspace, self.runner_temp, self.directory, self.steps)

    def cleanup(self):
        with mock.patch.object(subject, "servers", return_value=[]):
            subject.cleanup(self.home, self.workspace, self.runner_temp, self.directory)

    def test_removes_only_job_caches_and_successfully_uploaded_artifacts(self):
        self.authorize()
        retained = [self.root / "nfs-cache", self.workspace / "tracked-source",
                    self.runner_temp / "pre-sysmgr-artifacts.other-job", self.cache / "other-cache"]
        for path in retained:
            path.write_bytes(b"retain")
        self.cleanup()
        self.assertFalse(self.artifacts.exists())
        self.assertFalse(self.directory.exists())
        self.assertTrue(all(not (self.cache / name).exists() for name in subject.LOCAL_CACHES))
        self.assertTrue(all(path.read_bytes() == b"retain" for path in retained))

    def test_failed_upload_or_publication_preserves_all_evidence_but_reclaims_caches(self):
        for publisher, upload in (("success", "failure"), ("failure", "skipped"),
                                  ("success", "cancelled"), ("skipped", "skipped")):
            with self.subTest(publisher=publisher, upload=upload):
                self.steps["public_artifacts"]["outcome"] = publisher
                self.steps["bazel_upload_public_artifacts"]["outcome"] = upload
                marker = self.directory / subject.AUTHORIZATION
                marker.unlink(missing_ok=True)
                self.authorize()
                with contextlib.redirect_stdout(io.StringIO()) as logs:
                    self.cleanup()
                self.assertIn("::warning::Preserving", logs.getvalue())
                self.assertTrue((self.artifacts / "image.gz").is_file())
                self.assertTrue((self.directory / "pre-sysmgr-artifacts/image.gz").is_file())
                self.assertTrue(all(not (self.cache / name).exists() for name in subject.LOCAL_CACHES))

    def test_otel_cleanup_only_removes_its_published_artifacts(self):
        with mock.patch.dict(os.environ, GITHUB_JOB="otel_archive"):
            directory = subject.prepare(self.workspace, self.runner_temp)
            output = self.artifacts / "otel"
            output.mkdir()
            (output / "runtime.tar").write_bytes(b"published")
            subject.mark_artifacts(self.workspace, self.runner_temp, directory, self.steps)
            with mock.patch.object(subject, "servers", return_value=[]):
                subject.cleanup(self.home, self.workspace, self.runner_temp, directory)
            self.assertFalse(directory.exists())
            self.assertFalse(output.exists())
            self.assertTrue((self.artifacts / "image.gz").is_file())

    def test_skipped_matrix_groups_are_allowed_alongside_successful_uploads(self):
        self.steps.update({"teamd_public": {"outcome": "skipped"},
                           "bazel_upload_teamd_public": {"outcome": "skipped"}})
        self.authorize()
        self.cleanup()
        self.assertFalse(self.directory.exists())

    def test_dash_cleanup_only_removes_its_published_job_directory(self):
        with mock.patch.dict(os.environ, GITHUB_JOB="dash_engine_archive"):
            self.directory = subject.prepare(self.workspace, self.runner_temp)
            evidence = self.directory / "dash-engine.scratch"
            evidence.mkdir()
            (evidence / "image.gz").write_bytes(b"DASH archive")
            self.steps["bazel_upload_public_artifacts"]["outcome"] = "failure"
            self.authorize()
            self.cleanup()
            self.assertTrue((evidence / "image.gz").is_file())
            self.steps["bazel_upload_public_artifacts"]["outcome"] = "success"
            (self.directory / subject.AUTHORIZATION).unlink()
            self.authorize()
            self.cleanup()
            self.assertFalse(self.directory.exists())
            self.assertTrue((self.artifacts / "image.gz").is_file())

    def test_missing_authorization_preserves_evidence(self):
        probe = self.directory / "archive-bazel/interrupted-probe"
        probe.mkdir(parents=True)
        (probe / "edited-source").write_bytes(b"probe")
        with contextlib.redirect_stdout(io.StringIO()) as logs:
            self.cleanup()
        self.assertIn("no upload-cleanup authorization", logs.getvalue())
        self.assertFalse(probe.parent.exists())
        self.assertTrue(self.directory.exists())
        self.assertTrue(self.artifacts.exists())
        self.assertFalse((self.cache / "bazel").exists())

    def test_invalid_step_status_is_an_explicit_error(self):
        for steps in ([], {}, {"bazel_upload_missing": {"outcome": "success"}},
                      {"public_artifacts": {"outcome": "success"},
                       "bazel_upload_public_artifacts": {"outcome": "invalid"}}):
            with self.subTest(steps=steps), self.assertRaises(ValueError):
                subject.publication_status(steps)

    def test_other_run_identity_blocks_all_cleanup(self):
        self.authorize()
        with mock.patch.dict(os.environ, GITHUB_RUN_ID="456"), self.assertRaises(ValueError):
            self.cleanup()
        self.assertTrue((self.cache / "bazel/data").is_file())
        self.assertTrue(self.artifacts.exists())

    def test_broad_or_unregistered_job_directory_is_rejected(self):
        for path in (self.runner_temp, self.root, self.workspace, Path(".")):
            with self.subTest(path=path), self.assertRaises(ValueError):
                subject.job_identity(self.workspace, self.runner_temp, path)

    def test_symlinked_cache_or_artifact_root_never_deletes_outside_files(self):
        outside = self.root / "outside"
        outside.mkdir()
        sentinel = outside / "retain"
        sentinel.write_bytes(b"retain")
        self.authorize()
        for path in (self.cache / "bazel", self.artifacts):
            with self.subTest(path=path):
                original = path.with_name(path.name + "-original")
                path.rename(original)
                path.symlink_to(outside, target_is_directory=True)
                with self.assertRaises(OSError):
                    self.cleanup()
                self.assertEqual(sentinel.read_bytes(), b"retain")
                path.unlink()
                original.rename(path)

    def test_nested_symlinks_and_cache_hardlinks_leave_outside_content_and_mode_unchanged(self):
        outside = self.root / "outside"
        outside.mkdir()
        sentinel = outside / "retain"
        sentinel.write_bytes(b"retain")
        sentinel.chmod(0o444)
        (self.cache / "bazel/link").symlink_to(outside, target_is_directory=True)
        os.link(sentinel, self.cache / "bazel/hardlink")
        self.authorize()
        self.cleanup()
        self.assertEqual(sentinel.read_bytes(), b"retain")
        self.assertEqual(stat.S_IMODE(sentinel.stat().st_mode), 0o444)

    def test_mounted_cache_prevents_all_cache_deletion(self):
        self.authorize()
        def check(path):
            if path.name == subject.LOCAL_CACHES[-1]:
                raise ValueError("mounted cache")
        with mock.patch.object(subject, "check_mounts", side_effect=check):
            with self.assertRaisesRegex(ValueError, "mounted cache"):
                self.cleanup()
        self.assertTrue(all((self.cache / name).exists() for name in subject.LOCAL_CACHES))

    def test_replaced_artifact_directory_is_not_authorized(self):
        self.authorize()
        self.artifacts.rename(self.workspace / "original-artifacts")
        self.artifacts.mkdir()
        (self.artifacts / "new-data").write_bytes(b"retain")
        with self.assertRaisesRegex(ValueError, "Authorized cleanup target changed"):
            self.cleanup()
        self.assertTrue((self.artifacts / "new-data").is_file())
        self.assertTrue(self.directory.exists())

    def test_shutdown_uses_explicit_output_roots_and_failure_prevents_deletion(self):
        base, root = self.cache / "bazel/base", self.cache / "bazel/state"
        with mock.patch.object(subject, "servers", side_effect=[[(base, root, self.workspace)], []]), \
                mock.patch.object(subject.subprocess, "run") as run:
            subject.stop_servers(self.workspace, self.home, self.directory, "native-bazel")
        run.assert_called_once_with(
            ["native-bazel", "--ignore_all_rc_files", f"--output_user_root={root}",
             f"--output_base={base}", "shutdown"],
            cwd=self.workspace, stdin=subprocess.DEVNULL, check=True, timeout=120)
        with mock.patch.object(subject, "servers", return_value=[(base, root, self.workspace)]), \
                mock.patch.object(subject.subprocess, "run"):
            with self.assertRaisesRegex(ValueError, "remain active"):
                subject.cleanup(self.home, self.workspace, self.runner_temp, self.directory)
        self.assertTrue((self.cache / "bazel/data").is_file())

    def test_process_discovery_ignores_other_checkouts_and_selects_job_temp_servers(self):
        base, root = self.root / "base", self.root / "state"
        for workspace, expected in ((self.workspace, [(base, root, self.workspace)]),
                                    (self.directory / "snmp", [(base, root, self.directory / "snmp")]),
                                    (self.root / "other-checkout", [])):
            arguments = (b"bazel(checkout)\0--workspace_directory=" + os.fsencode(workspace) +
                         b"\0--output_base=" + os.fsencode(base) +
                         b"\0--output_user_root=" + os.fsencode(root) + b"\0")
            with self.subTest(workspace=workspace), \
                    mock.patch.object(subject.Path, "iterdir", return_value=[Path("/proc/123")]), \
                    mock.patch.object(subject.Path, "read_bytes", return_value=arguments):
                self.assertEqual(subject.servers(self.workspace, self.home, self.directory), expected)

    def test_shutdown_preserves_the_component_server_workspace(self):
        component = self.workspace / "src/sonic-yang-models"
        component.mkdir(parents=True)
        base, root = self.cache / "bazel/base", self.cache / "bazel/state"
        with mock.patch.object(subject, "servers", side_effect=[[(base, root, component)], []]), \
                mock.patch.object(subject.subprocess, "run") as run:
            subject.stop_servers(self.workspace, self.home, self.directory, "bazel")
        self.assertEqual(run.call_args.kwargs["cwd"], component)

    def test_host_process_discovery_does_not_claim_shared_home_servers(self):
        arguments = (b"bazel(other)\0--workspace_directory=/other/checkout\0--output_base=" +
                     os.fsencode(self.cache / "bazel/other") + b"\0--output_user_root=" +
                     os.fsencode(self.cache / "bazel") + b"\0")
        with mock.patch.object(subject.Path, "iterdir", return_value=[Path("/proc/123")]), \
                mock.patch.object(subject.Path, "read_bytes", return_value=arguments):
            self.assertEqual(subject.servers(self.workspace, self.home, self.directory,
                                             private_home=False), [])

    def test_inaccessible_owned_process_blocks_cleanup_but_foreign_process_is_ignored(self):
        for uid in (os.geteuid(), os.geteuid() + 1):
            with self.subTest(uid=uid), \
                    mock.patch.object(subject.Path, "iterdir", return_value=[Path("/proc/123")]), \
                    mock.patch.object(subject.Path, "read_bytes", side_effect=PermissionError("cmdline")), \
                    mock.patch.object(subject.Path, "stat", return_value=mock.Mock(st_uid=uid)):
                if uid == os.geteuid():
                    with self.assertRaises(PermissionError):
                        subject.servers(self.workspace, self.home, self.directory, private_home=False)
                else:
                    self.assertEqual(subject.servers(self.workspace, self.home, self.directory,
                                                     private_home=False), [])

    def test_host_cleanup_removes_only_exact_bases_and_uploaded_yang_artifacts(self):
        with mock.patch.dict(os.environ, GITHUB_JOB="package"):
            directory = subject.prepare(self.workspace, self.runner_temp)
            component = self.workspace / "src/sonic-yang-models"
            artifacts = component / "artifacts"
            artifacts.mkdir(parents=True)
            (artifacts / "package.whl").write_bytes(b"published")
            bases = [self.cache / "bazel/_bazel_user" / ("a" * 32),
                     self.cache / "bazel/_bazel_user" / ("b" * 32)]
            other = self.cache / "bazel/_bazel_user" / ("c" * 32)
            for base in (*bases, other):
                base.mkdir(parents=True)
                (base / "data").write_bytes(b"cache")
            subject.mark_artifacts(self.workspace, self.runner_temp, directory, self.steps)
            responses = [subprocess.CompletedProcess([], 0, str(base) + "\n") for base in bases]
            with mock.patch.object(subject, "servers", return_value=[]), \
                    mock.patch.object(subject, "check_container_users") as guard, \
                    mock.patch.object(subject.subprocess, "run", side_effect=responses) as run:
                subject.cleanup(self.home, self.workspace, self.runner_temp, directory, host=True)
            self.assertEqual([call.kwargs["cwd"] for call in run.call_args_list],
                             [self.workspace, component])
            self.assertTrue(all(not base.exists() for base in bases))
            self.assertTrue((other / "data").is_file())
            self.assertTrue(all((self.cache / name / "data").is_file() for name in subject.LOCAL_CACHES))
            self.assertFalse(artifacts.exists())
            self.assertFalse(directory.exists())
            self.assertTrue(self.artifacts.exists())
            self.assertEqual(guard.call_count, 4)

    def test_host_cleanup_rejects_broad_output_roots_and_docker_references(self):
        with mock.patch.object(subject.subprocess, "run", return_value=
                               subprocess.CompletedProcess([], 0, str(self.cache / "bazel"))):
            with self.assertRaisesRegex(ValueError, "unexpected host output base"):
                subject.cleanup_host_bases(self.home, self.workspace, "bazel")
        base = self.cache / "bazel/_bazel_user" / ("a" * 32)
        base.mkdir(parents=True)
        with mock.patch.object(subject.subprocess, "run", return_value=
                               subprocess.CompletedProcess([], 0, str(base))), \
                mock.patch.object(subject, "check_container_users", side_effect=ValueError("active Docker")):
            with self.assertRaisesRegex(ValueError, "active Docker"):
                subject.cleanup_host_bases(self.home, self.workspace, "bazel")
        self.assertTrue(base.exists())

    def test_registration_rejects_newline_paths(self):
        runner_temp = self.root / "unsafe\npath"
        runner_temp.mkdir()
        with self.assertRaisesRegex(ValueError, "single-line"):
            subject.prepare(self.workspace, runner_temp)
        self.assertEqual(list(runner_temp.iterdir()), [])

    def test_registration_rejects_redirected_workspaces(self):
        link = self.root / "checkout-link"
        link.symlink_to(self.workspace, target_is_directory=True)
        with self.assertRaises(OSError):
            subject.prepare(link, self.runner_temp)

    def test_host_cli_requires_the_github_workspace_layout(self):
        work = self.root / "_work"
        workspace = work / "project/project"
        workspace.mkdir(parents=True)
        runner_temp = work / "_temp"
        runner_temp.mkdir()
        environment = {
            "HOME": str(self.home), "GITHUB_ACTIONS": "true", "GITHUB_JOB": "package",
            "GITHUB_REPOSITORY": "securely1g/project", "GITHUB_WORKSPACE": str(workspace),
            "RUNNER_TEMP": str(runner_temp),
        }
        with mock.patch.dict(os.environ, environment), \
                mock.patch.object(sys, "argv", ["cleanup", "--prepare"]), \
                mock.patch.object(subject.Path, "is_file", return_value=False), \
                contextlib.redirect_stdout(io.StringIO()) as output:
            subject.main()
        directory = Path(json.loads(output.getvalue())["directory"])
        self.assertEqual(directory.parent, runner_temp)
        self.assertTrue((directory / subject.IDENTITY).is_file())
        with mock.patch.dict(os.environ, {**environment, "GITHUB_WORKSPACE": str(self.workspace)}), \
                mock.patch.object(sys, "argv", ["cleanup", "--prepare"]), \
                mock.patch.object(subject.Path, "is_file", return_value=False), \
                contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as error:
            subject.main()
        self.assertEqual(error.exception.code, 1)
        self.assertTrue((self.cache / "bazel/data").is_file())

    def test_host_cli_supports_a_symlinked_work_root_through_the_full_lifecycle(self):
        work = self.root / "storage/work"
        workspace = work / "project/project"
        workspace.mkdir(parents=True)
        runner_temp = work / "_temp"
        runner_temp.mkdir()
        runner = self.root / "actions-runner"
        runner.mkdir()
        alias = runner / "_work"
        alias.symlink_to(work, target_is_directory=True)
        artifacts = workspace / "src/sonic-yang-models/artifacts"
        artifacts.mkdir(parents=True)
        (artifacts / "package.whl").write_bytes(b"published")
        environment = {
            "HOME": str(self.home), "GITHUB_ACTIONS": "true", "GITHUB_JOB": "package",
            "GITHUB_REPOSITORY": "securely1g/project",
            "GITHUB_WORKSPACE": str(alias / "project/project"),
            "RUNNER_TEMP": str(alias / "_temp"),
        }
        with mock.patch.dict(os.environ, environment), \
                mock.patch.object(subject.Path, "is_file", return_value=False):
            with mock.patch.object(sys, "argv", ["cleanup", "--prepare"]), \
                    contextlib.redirect_stdout(io.StringIO()) as output:
                subject.main()
            directory = Path(json.loads(output.getvalue())["directory"])
            self.assertEqual(directory.parent, runner_temp)
            self.assertEqual(json.loads((directory / subject.IDENTITY).read_text())["workspace"],
                             str(workspace))
            with mock.patch.dict(os.environ, SONIC_BAZEL_JOB_TEMP=str(directory),
                                 BAZEL_CLEANUP_STEPS=json.dumps(self.steps)), \
                    mock.patch.object(sys, "argv", ["cleanup", "--mark-artifacts"]), \
                    contextlib.redirect_stdout(io.StringIO()):
                subject.main()
            with mock.patch.object(sys, "argv", ["cleanup", "--job-directory", str(directory)]), \
                    mock.patch.object(subject, "servers", return_value=[]), \
                    mock.patch.object(subject, "cleanup_host_bases") as bases, \
                    mock.patch.object(subject, "check_container_users"), \
                    contextlib.redirect_stdout(io.StringIO()):
                subject.main()
            bases.assert_called_once_with(self.home, workspace, "bazel")
        self.assertFalse(directory.exists())
        self.assertFalse(artifacts.exists())
        self.assertTrue((self.cache / "bazel/data").is_file())

    def test_host_paths_reject_symlinks_below_the_work_root(self):
        work = self.root / "storage/work"
        workspace = work / "project/project"
        workspace.mkdir(parents=True)
        runner_temp = work / "_temp"
        runner_temp.mkdir()
        alias = self.root / "_work"
        alias.symlink_to(work, target_is_directory=True)
        self.assertEqual(subject.host_paths(alias / "project/project", alias / "_temp"),
                         (workspace, runner_temp))
        for path in (workspace, runner_temp):
            with self.subTest(path=path):
                original = path.with_name(path.name + "-original")
                path.rename(original)
                path.symlink_to(original, target_is_directory=True)
                with self.assertRaises(OSError):
                    subject.host_paths(alias / "project/project", alias / "_temp")
                path.unlink()
                original.rename(path)

    def test_retargeting_the_work_root_blocks_registered_job_cleanup(self):
        roots = [self.root / name for name in ("old-work", "new-work")]
        for root in roots:
            (root / "project/project").mkdir(parents=True)
            (root / "_temp").mkdir()
        alias = self.root / "_work"
        alias.symlink_to(roots[0], target_is_directory=True)
        with mock.patch.dict(os.environ, GITHUB_JOB="package"):
            workspace, runner_temp = subject.host_paths(alias / "project/project", alias / "_temp")
            directory = subject.prepare(workspace, runner_temp)
            alias.unlink()
            alias.symlink_to(roots[1], target_is_directory=True)
            workspace, runner_temp = subject.host_paths(alias / "project/project", alias / "_temp")
            with mock.patch.object(subject, "servers") as servers, \
                    self.assertRaisesRegex(ValueError, "registered runner job directory"):
                subject.cleanup(self.home, workspace, runner_temp, directory, host=True)
            servers.assert_not_called()
        self.assertTrue(directory.exists())
        self.assertTrue((self.cache / "bazel/data").is_file())

    def test_malformed_artifact_identity_is_an_explicit_error(self):
        self.authorize()
        marker = self.directory / subject.AUTHORIZATION
        report = json.loads(marker.read_text())
        report["artifacts"] = {"artifacts": [1]}
        marker.write_text(json.dumps(report))
        with self.assertRaisesRegex(ValueError, "unexpected cleanup directory"):
            self.cleanup()
        self.assertTrue(self.artifacts.exists())


class ContainerCleanupWorkflowTest(unittest.TestCase):
    def test_original_bazel_container_jobs_keep_their_specialized_cleanup(self):
        jobs = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())["jobs"]
        action = "./.github/actions/bazel-post-job-cleanup"
        selected = {name for name, job in jobs.items()
                    if any(step.get("uses") == action and
                           step.get("with", {}).get("migrated-job") != "true"
                           for step in job["steps"])}
        self.assertEqual(selected, {"archive"})
        self.assertLessEqual(selected, subject.CONTAINER_JOBS)
        self.assertEqual({entry["architecture"] for entry in jobs["archive"]["strategy"]["matrix"]["include"]},
                         {"amd64", "arm64"})
        for name in selected:
            with self.subTest(job=name):
                job = jobs[name]
                self.assertIn("--init", job["container"]["options"])
                steps = job["steps"]
                register = next(index for index, step in enumerate(steps) if step.get("uses") == action)
                setup = next(index for index, step in enumerate(steps)
                             if "setup-bazel@" in step.get("uses", ""))
                self.assertLess(register, setup)
                self.assertEqual(steps[register]["id"], "bazel_cleanup_register")
                self.assertEqual(steps[-1]["if"],
                                 "always() && steps.bazel_cleanup_register.outcome == 'success'")
                self.assertEqual(steps[-1]["env"]["BAZEL_CLEANUP_STEPS"], "${{ toJSON(steps) }}")
                self.assertIn("--mark-artifacts", steps[-1]["run"])
                for step in steps:
                    if "upload-artifact@" in step.get("uses", ""):
                        identifier = step["id"]
                        self.assertTrue(identifier.startswith(subject.UPLOAD_PREFIX))
                        self.assertIn("steps." + identifier.removeprefix(subject.UPLOAD_PREFIX) +
                                      ".outcome == 'success'", step["if"])
        value = yaml.safe_load((ACTION / "action.yml").read_text())["runs"]
        self.assertEqual(value["using"], "node24")
        self.assertEqual(value["post-if"], "always()")
        self.assertEqual(value["post"], "post.js")
        self.assertNotIn("bazel-post-job-cleanup", str(jobs["vs_container"]))

    def test_additional_bazel_agent_workflows_register_cleanup_before_cache_setup(self):
        for workflow, name in (("bazel-eventd.yml", "native"),
                               ("bazel-supervisor-rust.yml", "native"), ("bazel-yang-models.yml", "package")):
            with self.subTest(workflow=workflow):
                value = yaml.safe_load((ROOT / ".github/workflows" / workflow).read_text())
                job = value["jobs"][name]
                steps = job["steps"]
                register = next(index for index, step in enumerate(steps)
                                if step.get("uses", "").endswith("/bazel-post-job-cleanup"))
                setup = next(index for index, step in enumerate(steps)
                             if "setup-bazel@" in step.get("uses", ""))
                self.assertLess(register, setup)
                if name == "native":
                    self.assertIn("--init", job["container"]["options"])
                else:
                    self.assertNotIn("container", job)
                self.assertEqual(steps[register]["id"], "bazel_cleanup_register")
                self.assertEqual(steps[-1]["if"],
                                 "always() && steps.bazel_cleanup_register.outcome == 'success'")
                self.assertEqual(steps[-1]["env"]["BAZEL_CLEANUP_STEPS"], "${{ toJSON(steps) }}")
                self.assertIn("--mark-artifacts", steps[-1]["run"])
                uploads = [step for step in steps if "upload-artifact@" in step.get("uses", "")]
                self.assertTrue(uploads)
                for upload in uploads:
                    self.assertTrue(upload["id"].startswith(subject.UPLOAD_PREFIX))
                    self.assertIn(upload["id"].removeprefix(subject.UPLOAD_PREFIX),
                                  {step.get("id") for step in steps})
                for event in (value.get("on", value.get(True)) or {}).values():
                    if isinstance(event, dict) and "paths" in event:
                        self.assertIn(".github/actions/bazel-post-job-cleanup/**", event["paths"])
                        self.assertIn("tools/bazel/ci/cleanup_container_bazel.py", event["paths"])

    @unittest.skipUnless(shutil.which("node"), "Node is needed to execute the action fixture")
    def test_node_action_registers_job_state_and_propagates_post_cleanup_failures(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            commands = root / "commands"
            commands.mkdir()
            fake = commands / "python3"
            fake.write_text("#!" + sys.executable + "\n" + """
import json, os, pathlib, sys
if "--prepare" in sys.argv:
    print(json.dumps({"directory": os.environ["FIXTURE_JOB_TEMP"]}))
else:
    pathlib.Path(os.environ["FIXTURE_CALL"]).write_text(json.dumps(sys.argv[1:]))
    sys.exit(int(os.environ.get("FIXTURE_EXIT", "0")))
""")
            fake.chmod(0o755)
            environment = dict(
                os.environ, PATH=str(commands) + os.pathsep + os.environ["PATH"],
                GITHUB_WORKSPACE=str(root), GITHUB_ENV=str(root / "env"),
                GITHUB_STATE=str(root / "state"), FIXTURE_JOB_TEMP=str(root / "job-temp"),
                FIXTURE_CALL=str(root / "call"))
            result = subprocess.run(["node", str(ACTION / "main.js")], env=environment,
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual((root / "env").read_text(),
                             f"SONIC_BAZEL_JOB_TEMP={root / 'job-temp'}\n")
            self.assertEqual((root / "state").read_text(),
                             f"cleanup_directory={root / 'job-temp'}\ncleanup_controller={root}\n"
                             "cleanup_migrated_job=false\n")
            environment["STATE_cleanup_directory"] = str(root / "job-temp")
            environment["STATE_cleanup_controller"] = str(root)
            for status in (0, 19):
                environment["FIXTURE_EXIT"] = str(status)
                result = subprocess.run(["node", str(ACTION / "post.js")], env=environment,
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, status, result.stderr)
                self.assertEqual(json.loads((root / "call").read_text()),
                                 ["-B", "-m", "tools.bazel.ci.cleanup_container_bazel",
                                  "--job-directory", str(root / "job-temp")])


if __name__ == "__main__":
    unittest.main()
