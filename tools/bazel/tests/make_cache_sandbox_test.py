"""Check scoped CI policy ownership and rollback without touching host AppArmor."""

from contextlib import ExitStack
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import make_cache_sandbox as sandbox


class SandboxPolicyTest(unittest.TestCase):
    JOB = "12345-2-arm64"

    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.directory = self.root / "apparmor.d"
        self.directory.mkdir()
        self.restriction = self.root / "restriction"
        self.restriction.write_text("1\n")
        self.profiles = self.root / "profiles"
        self.profiles.write_text("")
        self.receipt = self.root / "receipt.json"
        self.profile = self.directory / sandbox.profile_name(self.JOB)
        self.options = dict(directory=self.directory, profiles=self.profiles, restriction=self.restriction)

    def test_only_the_exact_sandbox_executable_gets_userns_permission(self):
        contents = sandbox.profile_contents(self.JOB)
        self.assertIn(sandbox.SANDBOX + " flags=(unconfined)", contents)
        self.assertEqual(contents.count("userns,"), 1)
        self.assertNotIn("*", contents)
        self.assertNotIn("sysctl", contents)
        for value in ("../administrator", "1-2-amd64/**", "1-2-amd64\nprofile other {}"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                sandbox.profile_contents(value)

    def test_existing_administrator_file_is_never_replaced(self):
        self.profile.write_text("administrator policy\n")
        with patch.object(sandbox.subprocess, "run") as run, self.assertRaises(ValueError):
            sandbox.install(self.JOB, self.receipt, **self.options)
        run.assert_not_called()
        self.assertEqual(self.profile.read_text(), "administrator policy\n")
        self.assertEqual(self.restriction.read_text(), "1\n")

    def test_existing_loaded_policy_is_never_replaced(self):
        self.profiles.write_text(sandbox.profile_name(self.JOB) + " (enforce)\n")
        with patch.object(sandbox.subprocess, "run") as run, self.assertRaises(ValueError):
            sandbox.install(self.JOB, self.receipt, **self.options)
        run.assert_not_called()
        self.assertFalse(self.profile.exists())

    def test_unrestricted_host_needs_no_policy_change(self):
        self.restriction.write_text("0\n")
        with patch.object(sandbox.subprocess, "run") as run:
            sandbox.install(self.JOB, self.receipt, **self.options)
        run.assert_not_called()
        self.assertFalse(json.loads(self.receipt.read_text())["installed"])
        self.assertEqual(self.restriction.read_text(), "0\n")

    def test_receipt_failure_after_loading_rolls_back_policy(self):
        real_save = sandbox.save_receipt
        def save(path, receipt):
            if receipt.get("installed"):
                raise OSError("receipt write failed")
            real_save(path, receipt)
        with ExitStack() as stack:
            stack.enter_context(patch.object(sandbox, "check_policy_directory"))
            stack.enter_context(patch.object(sandbox, "check_managed_file"))
            stack.enter_context(patch.object(sandbox.os, "chown"))
            stack.enter_context(patch.object(sandbox.shutil, "which", return_value="/usr/sbin/apparmor_parser"))
            stack.enter_context(patch.object(sandbox, "save_receipt", side_effect=save))
            run = stack.enter_context(patch.object(sandbox.subprocess, "run"))
            with self.assertRaisesRegex(OSError, "receipt write failed"):
                sandbox.install(self.JOB, self.receipt, **self.options)
        self.assertEqual([call.args[0][1] for call in run.call_args_list], ["-Q", "-r", "-R"])
        self.assertFalse(self.profile.exists())
        self.assertEqual(list(self.directory.iterdir()), [])
        self.assertTrue(json.loads(self.receipt.read_text())["removed"])
        self.assertEqual(self.restriction.read_text(), "1\n")

    def test_cleanup_refuses_changed_or_symlinked_policy(self):
        sandbox.save_receipt(self.receipt, {"profile": sandbox.profile_name(self.JOB), "installed": True})
        self.profile.write_text("administrator replacement\n")
        with patch.object(sandbox.subprocess, "run") as run, self.assertRaises(ValueError):
            sandbox.remove(self.JOB, self.receipt, **self.options)
        run.assert_not_called()
        self.assertTrue(self.profile.exists())
        self.profile.unlink()
        self.profile.symlink_to(self.restriction)
        with patch.object(sandbox.subprocess, "run") as run, self.assertRaises(ValueError):
            sandbox.remove(self.JOB, self.receipt, **self.options)
        run.assert_not_called()
        self.assertEqual(self.restriction.read_text(), "1\n")

    def test_cleanup_removes_only_its_managed_profile(self):
        self.profile.write_text(sandbox.profile_contents(self.JOB))
        other = self.directory / "administrator-profile"
        other.write_text("preserve this policy\n")
        sandbox.save_receipt(self.receipt, {"profile": sandbox.profile_name(self.JOB),
                                           "installed": True, "restriction_before": "1"})
        with patch.object(sandbox, "check_managed_file") as check, \
                patch.object(sandbox.shutil, "which", return_value="/usr/sbin/apparmor_parser"), \
                patch.object(sandbox.subprocess, "run") as run:
            sandbox.remove(self.JOB, self.receipt, **self.options)
        check.assert_called_once_with(self.profile, sandbox.profile_contents(self.JOB))
        run.assert_called_once_with(["/usr/sbin/apparmor_parser", "-R", str(self.profile)], check=True)
        self.assertFalse(self.profile.exists())
        self.assertEqual(other.read_text(), "preserve this policy\n")
        self.assertEqual(self.restriction.read_text(), "1\n")
        self.assertTrue(json.loads(self.receipt.read_text())["removed"])


if __name__ == "__main__":
    unittest.main()
