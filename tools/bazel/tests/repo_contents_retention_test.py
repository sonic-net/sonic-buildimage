"""Exercise host retention of Bazel's prepared-repository cache."""

from contextlib import ExitStack, redirect_stdout
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock

import yaml

from tools.bazel.ci import retain_repo_contents_cache as subject
from tools.bazel.ci import cleanup_runner_bazel_cache


ROOT = Path(__file__).resolve().parents[3]


class RepoContentsRetentionTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="repo-contents-retention-")
        self.addCleanup(temporary.cleanup)
        self.workspace = Path(temporary.name).resolve()
        self.cache = self.workspace / "vs-cache" / "repo-contents"
        self.cache.mkdir(parents=True)
        self.now = time.time()
        self.guards = ExitStack()
        self.addCleanup(self.guards.close)
        self.containers = self.guards.enter_context(
            mock.patch.object(subject, "check_container_users"))
        self.mounts = self.guards.enter_context(mock.patch.object(subject, "check_mounts"))

        def remove(command, check):
            self.assertTrue(check)
            self.assertEqual(command[:-1],
                             ["sudo", "-n", "rm", "--recursive", "--force",
                              "--one-file-system", "--"])
            path = Path(command[-1])
            self.assertTrue(path.is_relative_to(self.cache))
            self.assertEqual(path.name, "_trash")
            shutil.rmtree(path)

        self.original_run = subprocess.run
        self.remover = self.guards.enter_context(
            mock.patch.object(subject.subprocess, "run", wraps=self.original_run))
        self.remover.side_effect = lambda command, **kwargs: (
            remove(command, **kwargs) if command[0] == "sudo"
            else self.original_run(command, **kwargs))

    def entry(self, scope="review", number=1, age_days=1, group="a" * 64):
        parent = self.cache / scope / group
        parent.mkdir(parents=True, exist_ok=True)
        (parent / "counter").write_text(str(number))
        (parent / "lock").touch()
        contents = parent / str(number)
        contents.mkdir()
        (contents / "data").write_bytes(b"x" * 8192)
        marker = parent / f"{number}.recorded_inputs"
        marker.write_text("BAZEL_REPO_MARKER:fixture")
        os.utime(marker, (self.now - age_days * 86400,) * 2)
        return contents, marker

    def retain(self, **kwargs):
        options = dict(max_bytes=subject.GIB, max_age_days=14, min_free_bytes=1)
        options.update(kwargs)
        with redirect_stdout(io.StringIO()):
            return subject.retain(self.workspace, **options)

    def test_expiry_uses_bazel_marker_not_directory_mtime_and_preserves_other_caches(self):
        expired, old_marker = self.entry(age_days=15)
        warm, warm_marker = self.entry(number=2)
        os.utime(expired, (self.now,) * 2)
        os.utime(warm, (self.now - 30 * 86400,) * 2)
        outside = self.workspace / "outside"
        outside.mkdir()
        (outside / "sentinel").write_text("retain")
        (expired / "symlink").symlink_to(outside, target_is_directory=True)
        packages = self.workspace / "vs-cache" / "packages"
        packages.mkdir()
        (packages / "data").write_text("retain")
        report = self.retain()
        self.assertFalse(expired.exists())
        self.assertFalse(old_marker.exists())
        self.assertTrue(warm.exists())
        self.assertTrue(warm_marker.exists())
        self.assertEqual((outside / "sentinel").read_text(), "retain")
        self.assertEqual((packages / "data").read_text(), "retain")
        self.assertTrue((warm.parent / "counter").exists())
        self.assertTrue((warm.parent.parent / "gc_lock").exists())
        self.assertEqual(report["expired_entries"], 1)
        self.assertEqual(report["evicted_entries"], 1)

    def test_combined_budget_evicts_oldest_across_scopes(self):
        old, old_marker = self.entry(scope="master", age_days=3)
        middle, _ = self.entry(scope="review", age_days=2)
        newest, _ = self.entry(scope="dispatch", age_days=1)
        before = sum(subject.allocated_sizes(
            [self.cache / scope for scope in subject.SCOPES]).values())
        reclaimable = sum(subject.allocated_sizes([old, old_marker]).values())
        report = self.retain(max_bytes=before - reclaimable + 4096)
        self.assertFalse(old.exists())
        self.assertTrue(middle.exists())
        self.assertTrue(newest.exists())
        self.assertEqual(report["evicted_entries"], 1)
        self.assertLessEqual(report["after_bytes"], report["max_bytes"])

    def test_free_space_pressure_remeasures_actual_reclamation(self):
        old, _ = self.entry(age_days=3)
        newer, _ = self.entry(number=2, age_days=2)
        # Model blocks hardlinked outside the cache: the first eviction does
        # not free the estimated bytes, so the newer entry must also go.
        usage = shutil.disk_usage(self.workspace)
        free = lambda value: usage._replace(free=value)
        with mock.patch.object(subject.shutil, "disk_usage",
                               side_effect=[free(100), free(100), free(100),
                                            free(30000), free(30000), free(30000)]):
            report = self.retain(min_free_bytes=10000)
        self.assertFalse(old.exists())
        self.assertFalse(newer.exists())
        self.assertEqual(report["evicted_entries"], 2)
        self.assertGreaterEqual(report["after_free_bytes"], 10000)

    def test_insufficient_headroom_fails_even_when_cache_is_empty(self):
        usage = shutil.disk_usage(self.workspace)._replace(free=100)
        with mock.patch.object(subject.shutil, "disk_usage", return_value=usage):
            with self.assertRaisesRegex(ValueError, "cannot satisfy"):
                self.retain(min_free_bytes=101)

    def test_dry_run_previews_expiry_without_deleting(self):
        old, marker = self.entry(age_days=15)
        report = self.retain(dry_run=True)
        self.assertEqual(report["evicted_entries"], 1)
        self.assertTrue(old.exists())
        self.assertTrue(marker.exists())
        self.assertFalse((old.parent.parent / "_trash").exists())
        self.assertFalse(any(call.args[0][0] == "sudo"
                             for call in self.remover.call_args_list))

    def test_recovers_interrupted_trash_orphan_directory_and_orphan_marker(self):
        orphan, marker = self.entry()
        marker.unlink()
        missing, lone_marker = self.entry(number=2)
        shutil.rmtree(missing)
        trash = orphan.parent.parent / "_trash" / "interrupted"
        trash.mkdir(parents=True)
        (trash / "data").write_text("interrupted")
        report = self.retain()
        self.assertFalse(orphan.exists())
        self.assertFalse(lone_marker.exists())
        self.assertFalse(trash.exists())
        self.assertEqual(report["expired_entries"], 2)

    def test_unknown_layout_and_redirected_paths_prevent_all_deletion(self):
        old, marker = self.entry(age_days=30)
        outside = self.workspace / "outside"
        outside.mkdir()
        scope = old.parent.parent
        for path in (self.cache / "unknown", scope / "not-a-hash",
                     old.parent / "unexpected"):
            path.mkdir()
            with self.subTest(path=path), self.assertRaisesRegex(ValueError, "Unknown"):
                self.retain()
            path.rmdir()
        for path in (self.cache / "master", scope / "_trash",
                     old.parent / "3.recorded_inputs"):
            path.symlink_to(outside, target_is_directory=True)
            with self.subTest(path=path), self.assertRaisesRegex(ValueError, "unexpected"):
                self.retain()
            path.unlink()
        self.assertTrue(old.exists())
        self.assertTrue(marker.exists())
        self.assertFalse(any(call.args[0][0] == "sudo"
                             for call in self.remover.call_args_list))

    def test_root_symlink_is_refused(self):
        self.cache.rmdir()
        outside = self.workspace / "outside"
        outside.mkdir()
        self.cache.symlink_to(outside, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, "redirected"):
            self.retain()

    def test_mount_or_container_reference_prevents_eviction(self):
        old, marker = self.entry(age_days=30)
        for guard in (self.mounts, self.containers):
            guard.side_effect = ValueError("cache in use")
            with self.subTest(guard=guard), self.assertRaisesRegex(ValueError, "in use"):
                self.retain()
            guard.side_effect = None
        self.assertTrue(old.exists())
        self.assertTrue(marker.exists())

    def test_bazel_compatible_shared_lock_blocks_all_scopes(self):
        old, marker = self.entry(scope="master", age_days=30)
        self.entry(scope="review")
        lock = self.cache / "review" / "gc_lock"
        code = (
            "import fcntl,sys; "
            "f=open(sys.argv[1],'a+'); fcntl.lockf(f,fcntl.LOCK_SH); "
            "print('locked',flush=True); sys.stdin.readline()")
        process = subprocess.Popen(
            [sys.executable, "-c", code, str(lock)],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
        try:
            self.assertEqual(process.stdout.readline().strip(), "locked")
            with self.assertRaisesRegex(ValueError, "in use"):
                self.retain()
            self.assertTrue(old.exists())
            self.assertTrue(marker.exists())
        finally:
            process.communicate("\n", timeout=10)

    def test_removal_failure_is_not_silently_ignored(self):
        self.entry(age_days=30)

        def fail(command, **kwargs):
            if command[0] == "sudo":
                raise subprocess.CalledProcessError(1, "sudo")
            return self.original_run(command, **kwargs)

        self.remover.side_effect = fail
        with self.assertRaises(subprocess.CalledProcessError):
            self.retain()

    def test_invalid_limits_fail_before_touching_cache(self):
        for limit in ("max_bytes", "max_age_days", "min_free_bytes"):
            with self.subTest(limit=limit), self.assertRaisesRegex(ValueError, "positive"):
                self.retain(**{limit: 0})
        self.containers.assert_not_called()

    def test_headroom_only_never_scans_locks_or_modifies_cache(self):
        old, marker = self.entry(age_days=30)
        usage = shutil.disk_usage(self.workspace)._replace(free=100)
        with mock.patch.object(subject.shutil, "disk_usage", return_value=usage), \
                mock.patch.object(subject, "inventory") as inventory, \
                mock.patch.object(subject, "acquire_lock") as lock, \
                redirect_stdout(io.StringIO()):
            self.assertEqual(subject.check_free_space(self.workspace, 100), 100)
            with self.assertRaisesRegex(ValueError, "Insufficient"):
                subject.check_free_space(self.workspace, 101)
        inventory.assert_not_called()
        lock.assert_not_called()
        self.assertTrue(old.exists())
        self.assertTrue(marker.exists())
        self.assertFalse((old.parent.parent / "gc_lock").exists())

    def test_root_supervisor_only_assigns_ownership_of_new_lock(self):
        contents, _ = self.entry()
        scope = contents.parent.parent
        owner = scope.stat()
        with mock.patch.object(subject.os, "geteuid", return_value=0), \
                mock.patch.object(subject.os, "fchown") as chown:
            with ExitStack() as stack:
                subject.acquire_lock(stack, scope)
            self.assertEqual(chown.call_args.args[1:], (owner.st_uid, owner.st_gid))
            chown.reset_mock()
            with ExitStack() as stack:
                subject.acquire_lock(stack, scope)
            chown.assert_not_called()

    def test_workflow_reuses_cleanup_and_keeps_headroom_check_before_build(self):
        job = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())[
            "jobs"]["vs_container"]
        steps = job["steps"]
        retention = next(step for step in steps
                         if step["name"] == "Reclaim local build caches")
        preflight = next(step for step in steps
                         if step["name"] == "Check host-agent build prerequisites")
        prepare = next(step for step in steps
                       if step["name"] == "Prepare runner-local Bazel repository contents cache")
        self.assertLess(steps.index(retention), steps.index(preflight))
        self.assertLess(steps.index(retention), steps.index(prepare))
        # Execute the actual workflow shell, replacing only the interpreter.
        commands = self.workspace / "commands"
        commands.mkdir()
        python = commands / "python3"
        python.write_text(
            "#!" + sys.executable + "\n"
            "import json,os,sys\n"
            "print(json.dumps(sys.argv[1:]))\n"
            "sys.exit(int(os.environ.get('FIXTURE_EXIT', '0')))\n")
        python.chmod(0o755)
        environment = dict(os.environ, PATH=str(commands) + os.pathsep + os.environ["PATH"],
                           VS_RUNNER_WORKSPACE=str(self.workspace / "workspace with spaces"),
                           CACHE_MAX_GIB="70", CACHE_MAX_AGE_DAYS="14", CACHE_MIN_FREE_GIB="220")
        result = self.original_run(["bash", "-e", "-c", retention["run"]],
                                   env=environment, text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout), [
            "-B", "-m", cleanup_runner_bazel_cache.__name__, "--runner-workspace",
            environment["VS_RUNNER_WORKSPACE"], "--remove-package-cache",
            "--retain-repo-contents", "--repo-contents-max-gib", "70",
            "--repo-contents-max-age-days", "14", "--repo-contents-min-free-gib", "220"])
        result = self.original_run(["bash", "-e", "-c", preflight["run"]],
                                   env=environment, text=True, capture_output=True)
        calls = [json.loads(line) for line in result.stdout.splitlines()]
        self.assertEqual(calls[0], [
            "-B", "-m", subject.__name__, "--runner-workspace",
            environment["VS_RUNNER_WORKSPACE"], "--min-free-gib", "220",
            "--check-free-space-only"])
        self.assertEqual(calls[1][:2], ["-B", "tools/ci/runner/preflight.py"])
        environment["FIXTURE_EXIT"] = "1"
        result = self.original_run(["bash", "-e", "-c", retention["run"]],
                                   env=environment, capture_output=True)
        self.assertEqual(result.returncode, 1)
        result = self.original_run(["bash", "-e", "-c", preflight["run"]],
                                   env=environment, text=True, capture_output=True)
        self.assertEqual(result.returncode, 1)
        self.assertEqual(len(result.stdout.splitlines()), 1)


if __name__ == "__main__":
    unittest.main()
