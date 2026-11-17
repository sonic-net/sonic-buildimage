"""Check VS run cleanup without touching Docker or using privileged deletion."""

import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from tools.bazel.ci import cleanup_vs_workspace as cleanup


class VsWorkspaceCleanupTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="vs cleanup ")
        self.addCleanup(temporary.cleanup)
        self.workspace = Path(temporary.name).resolve() / "workspace"
        self.workspace.mkdir()
        self.checkout = self.workspace / "vs-123-2"
        self.checkout.mkdir()
        self.docker = self.enterContext(patch.object(cleanup.subprocess, "check_output", return_value=""))
        self.remove = self.enterContext(patch.object(cleanup.subprocess, "run"))

    def clean(self):
        cleanup.cleanup(self.workspace, "123", "2")

    def test_removes_only_current_run_and_preserves_cached_hardlinks_and_symlink_targets(self):
        cache = self.workspace.parent / "vs-cache"
        cache.mkdir()
        retained = cache / "package.deb"
        retained.write_bytes(b"retained package")
        retained.chmod(0o444)
        (self.checkout / "package.deb").hardlink_to(retained)
        (self.checkout / "cache-link").symlink_to(cache, target_is_directory=True)
        other = self.workspace / "vs-456-1"
        other.mkdir()
        (self.checkout / "fsroot.docker.verify-images").mkdir()

        def remove(command, check):
            self.assertTrue(check)
            self.assertEqual(command, [
                "sudo", "-n", "rm", "--recursive", "--force", "--one-file-system",
                "--", str(self.checkout)])
            shutil.rmtree(command[-1])

        self.remove.side_effect = remove
        self.clean()
        self.assertFalse(self.checkout.exists())
        self.assertTrue(other.is_dir())
        self.assertEqual(retained.read_bytes(), b"retained package")
        self.assertEqual(retained.stat().st_mode & 0o777, 0o444)
        self.docker.assert_called_once_with(
            ["docker", "ps", "--all", "--quiet", "--no-trunc"], text=True)

    def test_missing_checkout_needs_neither_docker_nor_sudo(self):
        self.checkout.rmdir()
        self.clean()
        self.docker.assert_not_called()
        self.remove.assert_not_called()

    def test_rejects_redirected_checkout_including_dangling_symlink(self):
        self.checkout.rmdir()
        for target in (self.workspace.parent, self.workspace / "missing"):
            with self.subTest(target=target):
                self.checkout.symlink_to(target, target_is_directory=True)
                try:
                    with self.assertRaisesRegex(ValueError, "redirected"):
                        self.clean()
                finally:
                    self.checkout.unlink()
        self.remove.assert_not_called()

    def test_rejects_invalid_identity_and_workspace(self):
        for run_id, attempt in (("", "2"), ("../", "2"), ("123", "0"), ("123", "2/../x")):
            with self.subTest(run_id=run_id, attempt=attempt):
                with self.assertRaisesRegex(ValueError, "numeric"):
                    cleanup.cleanup(self.workspace, run_id, attempt)
        with self.assertRaisesRegex(ValueError, "absolute"):
            cleanup.cleanup(Path("."), "123", "2")
        self.remove.assert_not_called()

    def test_refuses_file_instead_of_checkout(self):
        self.checkout.rmdir()
        self.checkout.write_text("not a checkout")
        with self.assertRaisesRegex(ValueError, "not a directory"):
            self.clean()
        self.remove.assert_not_called()

    def test_refuses_mounts_in_checkout_even_on_same_filesystem(self):
        for mount in (self.checkout, self.checkout / "target/mounted path"):
            with self.subTest(mount=mount):
                encoded = str(mount).replace(" ", r"\040")
                mountinfo = f"1 0 8:1 / {encoded} rw - ext4 /dev/test rw\n"
                with patch.object(cleanup.Path, "read_text", return_value=mountinfo):
                    with self.assertRaisesRegex(ValueError, "mount remains"):
                        self.clean()
        self.remove.assert_not_called()

    def test_refuses_container_references_to_checkout_children_or_ancestors(self):
        for source in (self.checkout, self.checkout / "fsroot.docker", self.workspace):
            with self.subTest(source=source):
                self.docker.side_effect = [
                    "container-id\n", json.dumps([{"Source": str(source)}]) + "\n"]
                with self.assertRaisesRegex(ValueError, "container still references"):
                    self.clean()
        self.remove.assert_not_called()

    def test_allows_containers_using_only_other_workspaces(self):
        self.docker.side_effect = [
            "other-container\n",
            json.dumps([{"Source": str(self.workspace / "vs-456-1")}]) + "\n"]
        self.remove.side_effect = lambda *args, **kwargs: shutil.rmtree(self.checkout)
        self.clean()
        self.assertFalse(self.checkout.exists())

    def test_docker_or_sudo_errors_are_not_ignored(self):
        self.docker.side_effect = subprocess.CalledProcessError(1, ["docker", "ps"])
        with self.assertRaises(subprocess.CalledProcessError):
            self.clean()
        self.remove.assert_not_called()
        self.docker.side_effect = None
        self.remove.side_effect = subprocess.CalledProcessError(1, ["sudo", "-n", "rm"])
        with self.assertRaises(subprocess.CalledProcessError):
            self.clean()
        self.assertTrue(self.checkout.is_dir())

    def test_reports_incomplete_deletion(self):
        with self.assertRaisesRegex(ValueError, "still exists"):
            self.clean()

    def test_refuses_checkout_replaced_during_container_checks(self):
        original = self.workspace / "retained-original"

        def replace(*args, **kwargs):
            self.checkout.rename(original)
            self.checkout.mkdir()
            return ""

        self.docker.side_effect = replace
        with self.assertRaisesRegex(ValueError, "changed during"):
            self.clean()
        self.remove.assert_not_called()


if __name__ == "__main__":
    unittest.main()
