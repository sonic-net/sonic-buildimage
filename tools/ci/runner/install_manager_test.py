"""Check manager installation on temporary files without systemd or privileges."""

import contextlib
import io
import json
import os
from pathlib import Path
from types import SimpleNamespace
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch

import install_manager as subject


class InstallerTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="runner-manager-install-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.source = self.root / "reviewed-source"
        self.source.mkdir()
        for name in subject.HELPERS:
            (self.source / name).write_text("# Reviewed helper: " + name + "\n")
        self.tools = self.root / "opt/sonic-runner-tools/runner"
        self.units = self.root / "etc/systemd/system"
        self.units.mkdir(parents=True)
        self.config_path = self.root / "etc/sonic-vs-runner-manager.json"
        self.config = subject.configuration(SimpleNamespace(
            operator="builder", github_account="securely1g",
            repo="securely1g/sonic-buildimage", pr=9))
        self.operator = SimpleNamespace(pw_uid=1000, pw_name="builder")
        self.tool_owner = 0
        self.states = {subject.SERVICE: "not-found", subject.TIMER: "not-found"}
        self.commands = []
        self.github = Mock()
        self.github_factory = Mock(return_value=self.github)

        # File creation, chmod and atomic replacement remain real. Simulate root
        # ownership only for the installed helper directory and fchown calls.
        original_stat = Path.stat

        def directory_stat(path, *args, **kwargs):
            value = original_stat(path, *args, **kwargs)
            if path == self.tools:
                fields = list(value)
                fields[4] = self.tool_owner
                return os.stat_result(fields)
            return value

        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        for name, value in (("TOOL_ROOT", self.tools), ("UNIT_ROOT", self.units),
                            ("CONFIG", self.config_path),
                            ("__file__", str(self.source / "install_manager.py"))):
            stack.enter_context(patch.object(subject, name, value))
        stack.enter_context(patch.object(subject.Path, "stat", directory_stat))
        self.euid = stack.enter_context(patch.object(subject.os, "geteuid", return_value=0))
        self.fchown = stack.enter_context(patch.object(subject.os, "fchown"))
        self.accounts = stack.enter_context(patch.object(subject.pwd, "getpwnam", return_value=self.operator))
        self.inspect = stack.enter_context(patch.object(subject.subprocess, "check_output", side_effect=self.inspect_job))
        self.run = stack.enter_context(patch.object(subject.subprocess, "run", side_effect=self.systemctl))
        stack.enter_context(patch.dict(sys.modules, {"manager": SimpleNamespace(OperatorGitHub=self.github_factory)}))
        stack.enter_context(contextlib.redirect_stdout(io.StringIO()))

    def inspect_job(self, command, **kwargs):
        self.assertEqual(command, ["systemctl", "show", "sonic-vs-runner.service",
                                   "--property=LoadState", "--value"])
        self.commands.append(command)
        return "loaded\n"

    def systemctl(self, command, **kwargs):
        self.commands.append(command)
        self.assertEqual(command[0], "systemctl")
        # No test can accidentally stop/restart a running build service.
        self.assertNotIn("sonic-vs-runner.service", command)
        if command[1] == "show":
            self.assertEqual(command[3:], ["--property=LoadState", "--value"])
            state = self.states[command[2]]
            return subprocess.CompletedProcess(command, 4 if state == "not-found" else 0,
                                               stdout=state + "\n", stderr="")
        self.assertTrue(kwargs.get("check"), command)
        return subprocess.CompletedProcess(command, 0)

    def managed_files(self):
        return [*(self.tools / name for name in subject.HELPERS), self.config_path,
                self.units / subject.SERVICE, self.units / subject.TIMER]

    def snapshot(self):
        return {str(path.relative_to(self.root)): (path.read_bytes(), path.stat().st_mode & 0o777)
                for path in self.managed_files() if path.exists()}

    def changes(self):
        return [command for command in self.commands if command[1] != "show"]

    def assert_installed(self, config):
        self.assertEqual(json.loads(self.config_path.read_text()), config)
        for name in subject.HELPERS:
            self.assertEqual((self.tools / name).read_bytes(), (self.source / name).read_bytes())
            self.assertEqual((self.tools / name).stat().st_mode & 0o777, 0o755)
        self.assertEqual(self.config_path.stat().st_mode & 0o777, 0o600)
        for unit in (subject.SERVICE, subject.TIMER):
            path = self.units / unit
            self.assertEqual(path.stat().st_mode & 0o777, 0o644)
            self.assertTrue(path.read_text().startswith(subject.HEADER))
        self.assertFalse(list(self.tools.glob(".*")))
        self.assertFalse(list(self.units.glob(".*")))
        self.assertFalse(list(self.config_path.parent.glob(".sonic-vs-runner-manager.json.*")))

    def test_first_install_handles_missing_manager_units_and_enables_timer(self):
        subject.install(self.config)
        self.assert_installed(self.config)
        self.github_factory.assert_called_once_with(self.config)
        self.github.verify_identity.assert_called_once_with()
        self.assertEqual(self.changes(), [
            ["systemctl", "daemon-reload"],
            ["systemctl", "enable", "--now", subject.TIMER],
        ])
        self.assertEqual(self.fchown.call_count, len(self.managed_files()))
        self.assertTrue(all(call.args[1:] == (0, 0) for call in self.fchown.call_args_list))

    def test_repeated_install_preserves_content_and_only_stops_manager_units(self):
        subject.install(self.config)
        original = self.snapshot()
        self.states = {subject.SERVICE: "loaded", subject.TIMER: "loaded"}
        self.commands.clear()
        subject.install(self.config)
        self.assertEqual(self.snapshot(), original)
        self.assertEqual(self.changes(), [
            ["systemctl", "stop", subject.TIMER],
            ["systemctl", "stop", subject.SERVICE],
            ["systemctl", "daemon-reload"],
            ["systemctl", "enable", "--now", subject.TIMER],
        ])

    def test_update_publishes_reviewed_helpers_and_master_route(self):
        subject.install(self.config)
        self.states = {subject.SERVICE: "loaded", subject.TIMER: "loaded"}
        (self.source / "manager.py").write_text("# Updated reviewed manager\n")
        updated = {**self.config, "pr": None}
        subject.install(updated)
        self.assert_installed(updated)
        self.assertEqual(self.github_factory.call_args.args, (updated,))

    def test_failed_operator_authentication_changes_no_files_or_units(self):
        subject.install(self.config)
        original = self.snapshot()
        self.commands.clear()
        self.github.verify_identity.side_effect = RuntimeError("wrong GitHub identity")
        with self.assertRaisesRegex(RuntimeError, "wrong GitHub identity"):
            subject.install({**self.config, "github_account": "another-account"})
        self.assertEqual(self.snapshot(), original)
        self.assertEqual(self.changes(), [])
        self.assertEqual(len(self.commands), 1)  # Read the prepared job service only.

    def test_failed_first_authentication_does_not_create_helper_directory(self):
        self.github.verify_identity.side_effect = RuntimeError("login is unavailable")
        with self.assertRaisesRegex(RuntimeError, "login is unavailable"):
            subject.install(self.config)
        self.assertFalse(self.tools.exists())
        self.assertEqual(self.snapshot(), {})
        self.assertEqual(self.changes(), [])

    def test_no_enable_installs_without_starting_timer(self):
        subject.install(self.config, enable=False)
        self.assert_installed(self.config)
        self.assertEqual(self.changes(), [
            ["systemctl", "daemon-reload"],
            ["systemctl", "disable", subject.TIMER],
        ])
        self.states = {subject.SERVICE: "loaded", subject.TIMER: "loaded"}
        self.commands.clear()
        subject.install(self.config, enable=False)
        self.assertEqual(self.changes(), [
            ["systemctl", "stop", subject.TIMER],
            ["systemctl", "stop", subject.SERVICE],
            ["systemctl", "daemon-reload"],
            ["systemctl", "disable", subject.TIMER],
        ])

    def test_unprepared_host_and_privileged_operator_are_rejected_before_authentication(self):
        self.inspect.side_effect = None
        self.inspect.return_value = "not-found\n"
        with self.assertRaisesRegex(RuntimeError, "bootstrap.sh"):
            subject.install(self.config)
        self.github_factory.assert_not_called()
        for account in (SimpleNamespace(pw_uid=0, pw_name="root"),
                        SimpleNamespace(pw_uid=1001, pw_name="sonic-runner")):
            with self.subTest(account=account.pw_name):
                self.accounts.return_value = account
                with self.assertRaisesRegex(RuntimeError, "normal GitHub-authenticated operator"):
                    subject.install(self.config)
        self.assertEqual(self.snapshot(), {})
        self.assertEqual(self.changes(), [])

    def test_non_root_install_stops_before_host_access(self):
        self.euid.return_value = 1000
        with self.assertRaisesRegex(RuntimeError, "Install with sudo"):
            subject.install(self.config)
        self.accounts.assert_not_called()
        self.inspect.assert_not_called()
        self.assertEqual(self.snapshot(), {})

    def test_insecure_helper_directory_is_not_populated(self):
        self.tools.mkdir(parents=True)
        for owner, mode in ((1000, 0o755), (0, 0o775), (0, 0o777)):
            with self.subTest(owner=owner, mode=mode):
                self.tool_owner = owner
                self.tools.chmod(mode)
                with self.assertRaisesRegex(RuntimeError, "root-owned and not group/world writable"):
                    subject.install(self.config)
                self.assertEqual(self.snapshot(), {})
                self.assertEqual(self.changes(), [])

    def test_generated_units_rearm_after_boot_and_keep_job_service_independent(self):
        subject.install(self.config)
        service = (self.units / subject.SERVICE).read_text()
        timer = (self.units / subject.TIMER).read_text()
        self.assertIn("Type=oneshot\nUser=root\n", service)
        self.assertIn("--config /etc/sonic-vs-runner-manager.json", service)
        self.assertIn("RequiresMountsFor=/data/sonic-runner\n", service)
        self.assertIn("StateDirectory=sonic-vs-runner-manager\n", service)
        self.assertIn("StateDirectoryMode=0700\n", service)
        self.assertNotIn("sonic-vs-runner.service", service)
        self.assertIn("OnBootSec=30s\n", timer)
        self.assertIn("OnUnitInactiveSec=60s\n", timer)
        self.assertIn("Unit=" + subject.SERVICE + "\n", timer)
        self.assertIn("WantedBy=timers.target\n", timer)
        config = json.loads(self.config_path.read_text())
        self.assertEqual(config["_generated"], {
            "notice": "AUTO-GENERATED. DO NOT EDIT MANUALLY.",
            "generator": "tools/ci/runner/install_manager.py",
        })
        self.assertEqual(set(config), {"_generated", "operator", "github_account", "repo", "pr"})

    def test_atomic_publication_replaces_symlink_without_changing_its_target(self):
        target = self.root / "unrelated-file"
        target.write_bytes(b"preserve these bytes")
        self.config_path.symlink_to(target)
        subject.write_owned(self.config_path, b"generated configuration\n", 0o600)
        self.assertFalse(self.config_path.is_symlink())
        self.assertEqual(self.config_path.read_bytes(), b"generated configuration\n")
        self.assertEqual(self.config_path.stat().st_mode & 0o777, 0o600)
        self.assertEqual(target.read_bytes(), b"preserve these bytes")

    def test_failed_atomic_publication_preserves_existing_config_and_removes_temporary_file(self):
        self.config_path.write_bytes(b"previous configuration\n")
        with patch.object(subject.os, "replace", side_effect=OSError("publication failed")):
            with self.assertRaisesRegex(OSError, "publication failed"):
                subject.write_owned(self.config_path, b"replacement configuration\n", 0o600)
        self.assertEqual(self.config_path.read_bytes(), b"previous configuration\n")
        self.assertFalse(list(self.config_path.parent.glob(".sonic-vs-runner-manager.json.*")))

    def test_preview_does_not_access_accounts_systemd_authentication_or_files(self):
        with patch.object(sys, "argv", ["install_manager.py", "--operator", "builder", "--master", "--dry-run"]), \
                patch.object(subject, "install", side_effect=AssertionError("preview attempted installation")), \
                contextlib.redirect_stdout(io.StringIO()) as output:
            subject.main()
        self.euid.assert_not_called()
        self.accounts.assert_not_called()
        self.inspect.assert_not_called()
        self.run.assert_not_called()
        self.github_factory.assert_not_called()
        self.fchown.assert_not_called()
        self.assertFalse(self.tools.exists())
        self.assertEqual(self.snapshot(), {})
        self.assertIn('"pr": null', output.getvalue())
        self.assertIn("OnBootSec=30s", output.getvalue())
        self.assertIn("AUTO-GENERATED. DO NOT EDIT MANUALLY.", output.getvalue())

    def test_invalid_routes_and_account_names_are_rejected(self):
        args = dict(operator="builder", github_account="securely1g",
                    repo="securely1g/sonic-buildimage", pr=None)
        for override in ({"operator": "builder;false"}, {"operator": "-root"},
                         {"github_account": "user name"}, {"repo": "owner/repo/extra"},
                         {"repo": "../repo"}, {"pr": 0}):
            with self.subTest(override=override), self.assertRaises(ValueError):
                subject.configuration(SimpleNamespace(**{**args, **override}))
        self.assertEqual(self.commands, [])


if __name__ == "__main__":
    unittest.main()
