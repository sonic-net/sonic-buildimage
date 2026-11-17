"""Check bounded cleanup of previous container and VS jobs' Bazel caches."""

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

from tools.bazel.ci import cleanup_runner_bazel_cache as subject


class RunnerBazelCacheCleanupTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name).resolve()
        self.cache = self.root / "_github_home" / ".cache"
        self.cache.mkdir(parents=True)
        self.workspace = self.root / "runner workspace"
        self.vs_cache = self.workspace / "vs-cache" / "bazel"
        self.vs_cache.parent.mkdir(parents=True)
        self.paths = [self.cache / name for name in subject.CACHE_NAMES] + [self.vs_cache]

    def populate(self):
        for path in self.paths:
            path.mkdir()
            (path / "data").write_text("cached data")

    def test_removes_only_container_and_vs_bazel_caches(self):
        self.populate()
        retained = [
            self.cache / "bazelisk", self.root / "job-script.sh",
            self.root / "_github_home" / "credentials",
            self.vs_cache.parent / "packages", self.workspace / "checkout-marker",
        ]
        for path in retained:
            path.write_text("retain")

        def remove(command, check):
            self.assertTrue(check)
            self.assertEqual(command[:-1],
                             ["sudo", "-n", "rm", "--recursive", "--force",
                              "--one-file-system", "--"])
            path = Path(command[-1])
            self.assertIn(path, self.paths)
            shutil.rmtree(path)

        with mock.patch.object(subject, "check_container_users") as containers, \
                mock.patch.object(subject, "check_mounts") as mounts, \
                mock.patch.object(subject.subprocess, "run", side_effect=remove) as run:
            subject.cleanup(self.root, self.workspace)
        self.assertEqual(containers.call_args_list, [mock.call(path) for path in self.paths])
        self.assertEqual(mounts.call_args_list, [mock.call(path) for path in self.paths])
        self.assertEqual(run.call_count, 4)
        self.assertTrue(all(path.read_text() == "retain" for path in retained))
        self.assertTrue(all(not path.exists() for path in self.paths))

    def test_missing_caches_need_no_docker_or_sudo(self):
        for root in (self.root, self.root / "fresh"):
            root.mkdir(exist_ok=True)
            with self.subTest(root=root), \
                    mock.patch.object(subject, "check_container_users") as containers, \
                    mock.patch.object(subject.subprocess, "run") as run:
                subject.cleanup(root, self.workspace)
                containers.assert_not_called()
                run.assert_not_called()

    def test_rejects_redirected_home_cache_and_targets(self):
        outside = self.root / "retained"
        outside.mkdir()
        for path in (self.cache / "bazel", self.cache, self.cache.parent):
            with self.subTest(path=path):
                if path == self.cache.parent:
                    self.cache.rmdir()
                if path.is_dir():
                    path.rmdir()
                path.symlink_to(outside, target_is_directory=True)
                with mock.patch.object(subject.subprocess, "run") as run:
                    with self.assertRaisesRegex(ValueError, "Refusing"):
                        subject.cleanup(self.root, self.workspace)
                    run.assert_not_called()
                path.unlink()
                if path != self.cache / "bazel":
                    path.mkdir()
        self.assertTrue(outside.is_dir())

    def test_rejects_invalid_runner_temp_and_non_directory_cache(self):
        for root in (Path("."), self.root / "missing"):
            with self.subTest(root=root), self.assertRaises(ValueError):
                subject.cleanup(root, self.workspace)
        (self.cache / "bazel").write_text("not a directory")
        with self.assertRaisesRegex(ValueError, "unexpected local cache"):
            subject.cleanup(self.root, self.workspace)

    def test_container_or_mount_reference_prevents_all_deletion(self):
        self.populate()
        for guard in ("check_container_users", "check_mounts"):
            with self.subTest(guard=guard), \
                    mock.patch.object(subject, "check_container_users"), \
                    mock.patch.object(subject, "check_mounts"), \
                    mock.patch.object(subject, guard, side_effect=ValueError("in use")), \
                    mock.patch.object(subject.subprocess, "run") as run:
                with self.assertRaisesRegex(ValueError, "in use"):
                    subject.cleanup(self.root, self.workspace)
                run.assert_not_called()
                self.assertTrue(all((self.cache / name).is_dir() for name in subject.CACHE_NAMES))

    def test_removal_failure_is_reported(self):
        self.populate()
        with mock.patch.object(subject, "check_container_users"), \
                mock.patch.object(subject, "check_mounts"), \
                mock.patch.object(subject.subprocess, "run",
                                  side_effect=subprocess.CalledProcessError(1, "rm")):
            with self.assertRaises(subprocess.CalledProcessError):
                subject.cleanup(self.root, self.workspace)
        self.assertTrue((self.cache / "bazel").is_dir())

    def test_success_without_removal_is_reported(self):
        self.populate()
        with mock.patch.object(subject, "check_container_users"), \
                mock.patch.object(subject, "check_mounts"), \
                mock.patch.object(subject.subprocess, "run"):
            with self.assertRaisesRegex(ValueError, "still exists"):
                subject.cleanup(self.root, self.workspace)

    def test_vs_cache_is_removed_even_without_container_home_caches(self):
        self.vs_cache.mkdir()
        (self.vs_cache / "output").write_text("VS build output")
        packages = self.vs_cache.parent / "packages"
        packages.mkdir()
        (packages / "package.deb").write_text("retain")

        def remove(command, check):
            self.assertTrue(check)
            self.assertEqual(Path(command[-1]), self.vs_cache)
            shutil.rmtree(self.vs_cache)

        with mock.patch.object(subject, "check_container_users") as containers, \
                mock.patch.object(subject, "check_mounts") as mounts, \
                mock.patch.object(subject.subprocess, "run", side_effect=remove):
            subject.cleanup(self.root, self.workspace)
        containers.assert_called_once_with(self.vs_cache)
        mounts.assert_called_once_with(self.vs_cache)
        self.assertFalse(self.vs_cache.exists())
        self.assertEqual((packages / "package.deb").read_text(), "retain")

    def test_vs_reference_blocks_all_deletion(self):
        self.populate()

        def check(path):
            if path == self.vs_cache:
                raise ValueError("VS cache is in use")

        for guard in ("check_container_users", "check_mounts"):
            with self.subTest(guard=guard), \
                    mock.patch.object(subject, "check_container_users"), \
                    mock.patch.object(subject, "check_mounts"), \
                    mock.patch.object(subject, guard, side_effect=check), \
                    mock.patch.object(subject.subprocess, "run") as run:
                with self.assertRaisesRegex(ValueError, "VS cache is in use"):
                    subject.cleanup(self.root, self.workspace)
                run.assert_not_called()
                self.assertTrue(all(path.is_dir() for path in self.paths))

    def test_rejects_redirected_vs_cache_and_parent(self):
        outside = self.root / "retained"
        outside.mkdir()
        (outside / "sentinel").write_text("retain")
        for path in (self.vs_cache, self.vs_cache.parent):
            with self.subTest(path=path):
                if path.is_dir():
                    path.rmdir()
                path.symlink_to(outside, target_is_directory=True)
                with mock.patch.object(subject.subprocess, "run") as run:
                    with self.assertRaisesRegex(ValueError, "Refusing"):
                        subject.cleanup(self.root, self.workspace)
                    run.assert_not_called()
                path.unlink()
                if path == self.vs_cache.parent:
                    path.mkdir()
        self.assertEqual((outside / "sentinel").read_text(), "retain")

    def test_rejects_invalid_workspace(self):
        for workspace in (Path("."), self.root / "missing"):
            with self.subTest(workspace=workspace), self.assertRaisesRegex(
                    ValueError, "runner workspace"):
                subject.cleanup(self.root, workspace)

    def test_cli_accepts_workspace_with_spaces(self):
        result = subprocess.run(
            [sys.executable, "-B", "-m", subject.__name__,
             "--runner-workspace", str(self.workspace)],
            cwd=Path(__file__).resolve().parents[3],
            env=dict(os.environ, RUNNER_TEMP=str(self.root)),
            text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("No selected local build caches", result.stdout)

    def test_explicit_package_cleanup_removes_only_the_obsolete_local_directory(self):
        self.populate()
        packages = self.vs_cache.parent / "packages"
        packages.mkdir()
        (packages / "package.tgz").write_text("old local cache")
        nfs = self.root / "shared-nfs"
        nfs.mkdir()
        (nfs / "package.tgz").write_text("retain shared cache")
        expected = [*self.paths, packages]

        def remove(command, check):
            self.assertTrue(check)
            path = Path(command[-1])
            self.assertIn(path, expected)
            shutil.rmtree(path)

        with mock.patch.object(subject, "check_container_users") as containers, \
                mock.patch.object(subject, "check_mounts") as mounts, \
                mock.patch.object(subject.subprocess, "run", side_effect=remove) as run:
            subject.cleanup(self.root, self.workspace, remove_package_cache=True)
        self.assertEqual(containers.call_args_list, [mock.call(path) for path in expected])
        self.assertEqual(mounts.call_args_list, [mock.call(path) for path in expected])
        self.assertEqual(run.call_count, 5)
        self.assertTrue(all(not path.exists() for path in expected))
        self.assertEqual((nfs / "package.tgz").read_text(), "retain shared cache")

    def test_package_reference_blocks_all_cleanup(self):
        self.populate()
        packages = self.vs_cache.parent / "packages"
        packages.mkdir()

        def check(path):
            if path == packages:
                raise ValueError("package cache is in use")

        for guard in ("check_container_users", "check_mounts"):
            with self.subTest(guard=guard), \
                    mock.patch.object(subject, "check_container_users"), \
                    mock.patch.object(subject, "check_mounts"), \
                    mock.patch.object(subject, guard, side_effect=check), \
                    mock.patch.object(subject.subprocess, "run") as run:
                with self.assertRaisesRegex(ValueError, "package cache is in use"):
                    subject.cleanup(self.root, self.workspace, remove_package_cache=True)
                run.assert_not_called()
                self.assertTrue(all(path.is_dir() for path in [*self.paths, packages]))

    def test_package_symlink_is_never_followed_or_removed(self):
        outside = self.root / "shared-nfs"
        outside.mkdir()
        (outside / "sentinel").write_text("retain")
        packages = self.vs_cache.parent / "packages"
        packages.symlink_to(outside, target_is_directory=True)
        with mock.patch.object(subject.subprocess, "run") as run:
            with self.assertRaisesRegex(ValueError, "Refusing unexpected local cache"):
                subject.cleanup(self.root, self.workspace, remove_package_cache=True)
            run.assert_not_called()
        self.assertEqual((outside / "sentinel").read_text(), "retain")
