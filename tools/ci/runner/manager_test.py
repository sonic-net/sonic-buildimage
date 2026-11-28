"""Runner-manager lifecycle checks with host and GitHub operations replaced."""

import contextlib
import io
import json
import os
from pathlib import Path
import re
from types import SimpleNamespace
import tempfile
import unittest
from unittest.mock import Mock, patch

import manager


CONFIG = {
    "operator": "lgh",
    "github_account": "securely1g",
    "repo": "securely1g/sonic-buildimage",
    "pr": None,
}


class HostIdleTests(unittest.TestCase):
    def check_host(self, *, service="inactive", process=1, active_process=None, containers="", docker_status=0):
        calls = []

        def run(command, **kwargs):
            calls.append(command)
            name = Path(command[0]).name
            if name == "systemctl":
                self.assertEqual(command[1], "show")
                return SimpleNamespace(returncode=0,
                                       stdout="LoadState=loaded\nActiveState=" + service + "\nSubState=dead\n", stderr="")
            if name == "pgrep":
                status = 0 if active_process and re.search(command[-1], active_process) else process
                return SimpleNamespace(returncode=status, stdout="123\n" if status == 0 else "", stderr="")
            if name == "docker":
                self.assertEqual(command[1], "ps")
                return SimpleNamespace(returncode=docker_status, stdout=containers, stderr="inspection unavailable")
            self.fail("Unexpected host mutation or command: " + repr(command))

        with patch.object(manager.subprocess, "run", side_effect=run), contextlib.redirect_stdout(io.StringIO()):
            result = manager.host_idle()
        return result, calls

    def test_active_or_stopping_runner_service_defers_without_touching_jobs(self):
        for state in ("active", "activating", "deactivating", "reloading"):
            with self.subTest(state=state):
                idle, calls = self.check_host(service=state)
                self.assertFalse(idle)
                self.assertEqual(len(calls), 1)

    def test_listener_or_worker_process_prevents_registration(self):
        for name in ("Runner.Listener", "Runner.Worker"):
            with self.subTest(process=name):
                idle, calls = self.check_host(active_process=name)
                self.assertFalse(idle)
                self.assertFalse(any(Path(command[0]).name == "docker" for command in calls))

    def test_remaining_container_prevents_registration_after_runner_exits(self):
        idle, calls = self.check_host(containers="builder-still-running\n")
        self.assertFalse(idle)
        self.assertTrue(any(Path(command[0]).name == "docker" for command in calls))

    def test_idle_requires_service_process_and_container_checks(self):
        idle, calls = self.check_host()
        self.assertTrue(idle)
        self.assertEqual([Path(command[0]).name for command in calls], ["systemctl", "pgrep", "docker"])

    def test_unknown_process_or_container_state_fails_closed(self):
        for options in ({"process": 2}, {"docker_status": 1}, {"service": "unknown"}):
            with self.subTest(options=options), self.assertRaises(RuntimeError):
                self.check_host(**options)


class ManagerLifecycleTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.home = Path(temporary.name)
        (self.home / "attempts").mkdir()
        (self.home / "cache").mkdir()
        self.cache = self.home / "cache/build-cache"
        self.cache.write_bytes(b"retained compiled inputs")
        self.remote = {}
        self.calls = []
        self.counter = 0
        self.held = []
        self.output = io.StringIO()
        self.stack = contextlib.ExitStack()
        self.addCleanup(self.stack.close)
        self.stack.enter_context(contextlib.redirect_stdout(self.output))
        self.stack.enter_context(patch.object(manager.rearm, "RUNNER_HOME", self.home))
        self.stack.enter_context(patch.object(manager, "REARM_LOCK", self.home / "rearm.lock"))
        self.stack.enter_context(patch.object(manager, "MANAGER_LOCK", self.home / "manager.lock"))
        self.state = self.home / "manager-state"
        self.stack.enter_context(patch.object(manager, "STATE_DIRECTORY", self.state))
        self.stack.enter_context(patch.object(manager, "PENDING", self.state / "pending.json"))
        actual_lstat = Path.lstat

        def root_owned_state(path):
            info = actual_lstat(path)
            if path == self.state:
                fields = list(info)
                fields[4] = 0
                return os.stat_result(fields)
            return info

        self.stack.enter_context(patch.object(Path, "lstat", autospec=True, side_effect=root_owned_state))
        actual_fstat = os.fstat

        def root_owned_open_file(descriptor):
            fields = list(actual_fstat(descriptor))
            fields[4] = 0
            return os.stat_result(fields)

        self.stack.enter_context(patch.object(manager.os, "fstat", side_effect=root_owned_open_file))
        self.stack.enter_context(patch.object(manager, "locked", side_effect=self.locked))
        self.idle = self.stack.enter_context(patch.object(manager, "host_idle", return_value=True))
        self.preflight = self.stack.enter_context(patch.object(manager, "preflight"))
        self.service = self.stack.enter_context(patch.object(manager, "root_command", return_value=""))
        self.local_remove = self.stack.enter_context(patch.object(manager.rearm, "as_runner", side_effect=self.remove_local))
        self.register = self.stack.enter_context(patch.object(manager.rearm, "register_from_operator", side_effect=self.register_next))
        self.ready = self.stack.enter_context(patch.object(manager.rearm, "wait_until_ready"))
        self.api = Mock()
        self.api.request.side_effect = self.request

    @contextlib.contextmanager
    def locked(self, path):
        self.held.append(path)
        try:
            yield
        finally:
            self.held.remove(path)

    def make_registration(self):
        self.counter += 1
        stamp = "20261010T000000Z-" + f"{self.counter:08x}"
        attempt = self.home / "attempts" / stamp
        attempt.mkdir()
        (attempt / "_diag").mkdir()
        (attempt / "_diag/runner.log").write_text("keep diagnostic evidence\n")
        (attempt / "_work").mkdir()
        (attempt / "_work/build.log").write_text("keep previous checkout\n")
        registration = {"id": 100 + self.counter, "name": "sonic-vs-master-" + stamp, "attempt": str(attempt)}
        data = {"agentId": registration["id"], "agentName": registration["name"], "ephemeral": True,
                "workFolder": "_work", "gitHubUrl": "https://github.com/" + CONFIG["repo"]}
        (attempt / ".runner").write_text(json.dumps(data), encoding="utf-8-sig")
        (attempt / ".credentials").write_text("job-scoped runner credentials")
        current = self.home / "current"
        current.unlink(missing_ok=True)
        current.symlink_to(attempt)
        self.remote[registration["id"]] = {"id": registration["id"], "name": registration["name"],
                                             "status": "offline", "busy": False,
                                             "labels": [{"name": "sonic-vs-source-master"}]}
        return registration

    def request(self, endpoint, deadline=None, method="GET"):
        self.calls.append((method, endpoint))
        if endpoint.endswith("/registration-token"):
            self.assertEqual(method, "POST")
            return {"token": "SHORT_LIVED_REGISTRATION_SECRET"}
        self.assertEqual(method, "GET", "The manager must never delete a remote runner")
        if "/actions/runners?" in endpoint:
            return {"total_count": len(self.remote), "runners": list(self.remote.values())}
        if "/actions/runners/" in endpoint:
            identifier = int(endpoint.rsplit("/", 1)[1])
            if identifier not in self.remote:
                raise manager.GitHubNotFound("HTTP 404")
            return self.remote[identifier]
        self.fail("Unexpected GitHub operation: " + endpoint)

    def remove_local(self, command, *, cwd=None, **kwargs):
        self.assertEqual(command, [str(cwd / "config.sh"), "remove", "--local"])
        (cwd / ".runner").unlink()
        (cwd / ".credentials").unlink()

    def register_next(self, command, token, *, env=None):
        self.assertNotIn(manager.REARM_LOCK, self.held, "Child registration owns the shared rearm lock")
        self.assertIn("--register", command)
        self.assertIn("--master", command)
        self.assertNotIn(token, repr(command))
        self.assertEqual(token, "SHORT_LIVED_REGISTRATION_SECRET")
        self.assertFalse({"GH_TOKEN", "GITHUB_TOKEN", "SSH_AUTH_SOCK", "GH_CONFIG_DIR"} & env.keys())
        return self.make_registration()

    def snapshot(self):
        return {str(path.relative_to(self.home)): path.read_bytes()
                for path in self.home.rglob("*") if path.is_file() and not path.is_symlink()}

    def test_completed_job_gets_a_fresh_runner_and_active_job_stays_untouched(self):
        self.assertEqual(manager.reconcile(CONFIG, self.api), "registered")
        first = manager.current_registration(CONFIG)
        before = self.snapshot()
        self.idle.return_value = False
        self.assertEqual(manager.reconcile(CONFIG, self.api), "deferred")
        self.assertEqual(self.snapshot(), before)
        self.assertEqual(self.register.call_count, 1)
        self.service.assert_not_called()

        # Ephemeral completion removes its registration; a later timer tick arms
        # a new attempt while preserving the old checkout, evidence and caches.
        del self.remote[first["id"]]
        Path(first["attempt"], ".runner").unlink()
        self.idle.return_value = True
        self.assertEqual(manager.reconcile(CONFIG, self.api), "registered")
        second = manager.current_registration(CONFIG)
        self.assertNotEqual(first["id"], second["id"])
        self.assertNotEqual(first["attempt"], second["attempt"])
        self.assertEqual(self.register.call_count, 2)
        self.assertEqual(Path(first["attempt"], "_work/build.log").read_text(), "keep previous checkout\n")
        self.assertEqual(self.cache.read_bytes(), b"retained compiled inputs")

    def test_retained_registration_restarts_after_reboot_without_a_new_token(self):
        registration = self.make_registration()
        before = self.snapshot()
        self.assertEqual(manager.reconcile(CONFIG, self.api), "started")
        self.assertEqual(self.snapshot(), before)
        self.register.assert_not_called()
        self.local_remove.assert_not_called()
        self.assertFalse(any(method == "POST" for method, _ in self.calls))
        self.assertEqual([call.args[0][1] for call in self.service.call_args_list], ["reset-failed", "start"])
        self.ready.assert_called_once_with(CONFIG["repo"], registration, api=self.api.request)

    def test_crash_after_registration_recovers_and_allows_the_following_job(self):
        actual_unlink = Path.unlink

        def crash_before_clearing_marker(path, *args, **kwargs):
            if path == manager.PENDING:
                raise OSError("simulated interruption after registration")
            return actual_unlink(path, *args, **kwargs)

        with patch.object(Path, "unlink", autospec=True, side_effect=crash_before_clearing_marker):
            with self.assertRaisesRegex(OSError, "simulated interruption"):
                manager.reconcile(CONFIG, self.api)
        first = manager.current_registration(CONFIG)
        self.assertTrue(manager.PENDING.exists())
        self.assertEqual(self.register.call_count, 1)

        # The next tick proves this exact local registration still exists on
        # GitHub, restarts it, and resolves the interrupted registration marker.
        self.assertEqual(manager.reconcile(CONFIG, self.api), "started")
        self.assertFalse(manager.PENDING.exists())
        self.assertEqual(self.register.call_count, 1)

        del self.remote[first["id"]]
        Path(first["attempt"], ".runner").unlink()
        self.assertEqual(manager.reconcile(CONFIG, self.api), "registered")
        self.assertEqual(self.register.call_count, 2)
        self.assertNotEqual(manager.current_registration(CONFIG)["id"], first["id"])
        self.assertFalse(manager.PENDING.exists())
        self.assertEqual(Path(first["attempt"], "_work/build.log").read_text(), "keep previous checkout\n")
        self.assertEqual(self.cache.read_bytes(), b"retained compiled inputs")

    def test_pending_marker_for_different_configuration_is_preserved(self):
        self.make_registration()
        for change in ({"repo": "another/repository"}, {"pr": 9},
                       {"operator": "another-operator"}, {"github_account": "another-account"}):
            with self.subTest(change=change):
                manager.mark_pending({**CONFIG, **change})
                try:
                    before = self.snapshot()
                    with self.assertRaises(RuntimeError):
                        manager.reconcile(CONFIG, self.api)
                    self.assertEqual(self.snapshot(), before)
                finally:
                    manager.PENDING.unlink(missing_ok=True)
        self.service.assert_not_called()
        self.local_remove.assert_not_called()
        self.register.assert_not_called()

    def test_confirmed_remote_404_clears_only_local_registration_and_preserves_attempt(self):
        previous = self.make_registration()
        del self.remote[previous["id"]]
        self.assertEqual(manager.reconcile(CONFIG, self.api), "registered")
        self.local_remove.assert_called_once()
        self.assertFalse(Path(previous["attempt"], ".runner").exists())
        self.assertTrue(Path(previous["attempt"], "_diag/runner.log").is_file())
        self.assertTrue(Path(previous["attempt"], "_work/build.log").is_file())
        self.assertEqual(self.cache.read_bytes(), b"retained compiled inputs")
        self.assertFalse(any(method == "DELETE" for method, _ in self.calls))

    def test_remote_busy_runner_is_preserved_even_if_host_looks_idle(self):
        registration = self.make_registration()
        self.remote[registration["id"]]["busy"] = True
        before = self.snapshot()
        self.assertEqual(manager.reconcile(CONFIG, self.api), "deferred")
        self.assertEqual(self.snapshot(), before)
        self.service.assert_not_called()
        self.preflight.assert_not_called()
        self.register.assert_not_called()

    def test_mismatched_remote_identity_or_routing_fails_without_mutation(self):
        registration = self.make_registration()
        original = self.remote[registration["id"]].copy()
        before = self.snapshot()
        for change in ({"id": 999}, {"name": "somebody-elses-runner"}, {"labels": []}, {"busy": "false"}):
            with self.subTest(change=change):
                self.remote[registration["id"]] = {**original, **change}
                with self.assertRaises(RuntimeError):
                    manager.reconcile(CONFIG, self.api)
                self.assertEqual(self.snapshot(), before)
        self.service.assert_not_called()
        self.local_remove.assert_not_called()
        self.register.assert_not_called()

    def test_authentication_or_preflight_failure_preserves_old_registration_and_cache(self):
        previous = self.make_registration()
        del self.remote[previous["id"]]
        before = self.snapshot()
        for operation in (self.api.verify_identity, self.preflight):
            with self.subTest(operation=operation):
                operation.side_effect = RuntimeError("admission failed")
                with self.assertRaisesRegex(RuntimeError, "admission failed"):
                    manager.reconcile(CONFIG, self.api)
                operation.side_effect = None
                self.assertEqual(self.snapshot(), before)
        self.local_remove.assert_not_called()
        self.register.assert_not_called()
        self.service.assert_not_called()

    def test_token_failure_does_not_clear_stale_local_registration(self):
        previous = self.make_registration()
        del self.remote[previous["id"]]
        before = self.snapshot()

        def fail_token(endpoint, deadline=None, method="GET"):
            if method == "POST":
                raise manager.rearm.GitHubApiError("HTTP 403")
            return self.request(endpoint, deadline, method)

        self.api.request.side_effect = fail_token
        with self.assertRaisesRegex(RuntimeError, "HTTP 403"):
            manager.reconcile(CONFIG, self.api)
        self.assertEqual(self.snapshot(), before)
        self.local_remove.assert_not_called()
        self.register.assert_not_called()

    def test_404_is_insufficient_without_a_successful_repository_inventory(self):
        previous = self.make_registration()
        del self.remote[previous["id"]]
        before = self.snapshot()
        self.api.request.side_effect = manager.GitHubNotFound("access unavailable")
        with self.assertRaises(manager.GitHubNotFound):
            manager.reconcile(CONFIG, self.api)
        self.assertEqual(self.snapshot(), before)
        self.local_remove.assert_not_called()
        self.register.assert_not_called()

    def test_job_starting_during_admission_prevents_cleanup_and_registration(self):
        previous = self.make_registration()
        del self.remote[previous["id"]]
        before = self.snapshot()
        self.idle.side_effect = [True, False]
        self.assertEqual(manager.reconcile(CONFIG, self.api), "deferred")
        self.assertEqual(self.snapshot(), before)
        self.local_remove.assert_not_called()
        self.register.assert_not_called()

    def test_ambiguous_remote_registration_blocks_duplicates(self):
        self.remote[42] = {"id": 42, "name": "sonic-vs-master-unpublished"}
        before = self.snapshot()
        with self.assertRaisesRegex(RuntimeError, "conflict"):
            manager.reconcile(CONFIG, self.api)
        self.assertEqual(self.snapshot(), before)
        self.register.assert_not_called()

    def test_registration_failure_leaves_a_token_free_marker_that_blocks_retry(self):
        self.register.side_effect = RuntimeError("registration result unavailable")
        with self.assertRaisesRegex(RuntimeError, "result unavailable"):
            manager.reconcile(CONFIG, self.api)
        self.assertTrue(manager.PENDING.is_file())
        before = self.snapshot()
        with self.assertRaisesRegex(RuntimeError, "outcome is uncertain"):
            manager.reconcile(CONFIG, self.api)
        self.assertEqual(self.register.call_count, 1)
        self.assertEqual(self.snapshot(), before)
        self.assertNotIn("SHORT_LIVED_REGISTRATION_SECRET", repr(before))
        self.assertNotIn("SHORT_LIVED_REGISTRATION_SECRET", self.output.getvalue())

    def test_unfinished_attempt_outside_current_blocks_automatic_recovery(self):
        self.make_registration()
        self.make_registration()
        before = self.snapshot()
        with self.assertRaisesRegex(RuntimeError, "outside current"):
            manager.reconcile(CONFIG, self.api)
        self.assertEqual(self.snapshot(), before)
        self.register.assert_not_called()
        self.local_remove.assert_not_called()

    def test_current_identity_must_match_repository_mode_and_routing(self):
        registration = self.make_registration()
        path = Path(registration["attempt"], ".runner")
        original = json.loads(path.read_text(encoding="utf-8-sig"))
        for change in ({"ephemeral": False}, {"gitHubUrl": "https://github.com/another/repo"},
                       {"agentName": "another-runner"}, {"agentId": True}, {"workFolder": "elsewhere"}):
            with self.subTest(change=change):
                path.write_text(json.dumps({**original, **change}))
                with self.assertRaisesRegex(RuntimeError, "differs from manager config"):
                    manager.reconcile(CONFIG, self.api)
        self.api.request.assert_not_called()
        self.register.assert_not_called()
        self.local_remove.assert_not_called()

    def test_current_cannot_redirect_manager_outside_attempts(self):
        outside = self.home / "outside"
        outside.mkdir()
        (outside / ".runner").write_text("preserve this unrelated file")
        (self.home / "current").symlink_to(outside)
        before = self.snapshot()
        with self.assertRaisesRegex(RuntimeError, "outside the attempts"):
            manager.reconcile(CONFIG, self.api)
        self.assertEqual(self.snapshot(), before)
        self.register.assert_not_called()

    def test_manager_and_manual_rearm_lock_contention_defer_without_mutation(self):
        for busy_path in (manager.MANAGER_LOCK, manager.REARM_LOCK):
            @contextlib.contextmanager
            def competing_lock(path):
                if path == busy_path:
                    raise manager.LockBusy("another operation")
                yield

            with self.subTest(path=busy_path), \
                    patch.object(manager, "locked", side_effect=competing_lock), \
                    patch.object(manager, "OperatorGitHub", return_value=self.api), \
                    patch.object(manager.os, "geteuid", return_value=0):
                before = self.snapshot()
                self.assertEqual(manager.run_once(CONFIG), "deferred")
                self.assertEqual(self.snapshot(), before)
        self.api.verify_identity.assert_not_called()
        self.register.assert_not_called()
        self.service.assert_not_called()


class ConfigurationTests(unittest.TestCase):
    def test_generated_metadata_and_explicit_master_or_pr_are_accepted(self):
        for pr in (None, 9):
            value = {**CONFIG, "pr": pr, "_generated": {"notice": "AUTO-GENERATED. DO NOT EDIT MANUALLY."}}
            self.assertEqual(manager.validate_config(value)["pr"], pr)

    def test_config_rejects_credentials_unsafe_accounts_and_invalid_routing(self):
        for change in ({"token": "long-term-secret"}, {"operator": "root"}, {"operator": "sonic-runner"},
                       {"operator": "name;command"}, {"pr": 0}, {"pr": True}, {"pr": "9"},
                       {"repo": "../another"}, {"repo": "owner/repo/extra"}):
            with self.subTest(change=change), self.assertRaises(RuntimeError):
                manager.validate_config({**CONFIG, **change})

    def test_config_file_requires_root_ownership_and_safe_mode(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "manager.json"
            path.write_text(json.dumps(CONFIG))
            for uid, mode, accepted in ((0, 0o100600, True), (1001, 0o100600, False),
                                        (0, 0o100660, False), (0, 0o100606, False)):
                with self.subTest(uid=uid, mode=mode), \
                        patch.object(manager.os, "fstat", return_value=SimpleNamespace(st_uid=uid, st_mode=mode)):
                    if accepted:
                        self.assertEqual(manager.load_config(path), CONFIG)
                    else:
                        with self.assertRaisesRegex(RuntimeError, "root-owned"):
                            manager.load_config(path)
            link = Path(directory) / "symlink.json"
            link.symlink_to(path)
            with self.assertRaises(OSError):
                manager.load_config(link)


class OperatorCredentialTests(unittest.TestCase):
    def setUp(self):
        self.account = SimpleNamespace(pw_name="lgh", pw_uid=1001, pw_gid=1001, pw_dir="/home/lgh")

    def api(self):
        with patch.object(manager.pwd, "getpwnam", return_value=self.account):
            return manager.OperatorGitHub(CONFIG)

    def test_api_uses_operator_account_and_ignores_ambient_secrets(self):
        ambient = {"GH_TOKEN": "LONG_TERM_SECRET", "GITHUB_TOKEN": "OTHER_SECRET", "SSH_AUTH_SOCK": "/agent.sock"}
        with patch.dict(os.environ, ambient):
            api = self.api()
            with patch.object(manager.os, "getgrouplist", return_value=[1001]), \
                    patch.object(manager.subprocess, "run", return_value=SimpleNamespace(returncode=0, stdout='{"login":"securely1g"}')) as run:
                api.verify_identity()
        self.assertEqual(run.call_args.kwargs["user"], 1001)
        self.assertEqual(run.call_args.kwargs["group"], 1001)
        self.assertEqual(run.call_args.kwargs["cwd"], "/home/lgh")
        self.assertEqual(run.call_args.kwargs["env"]["HOME"], "/home/lgh")
        self.assertFalse(ambient.keys() & run.call_args.kwargs["env"].keys())
        self.assertNotIn("LONG_TERM_SECRET", repr(run.call_args))

    def test_api_error_and_invalid_json_do_not_echo_sensitive_response_bodies(self):
        secret = "SHORT_LIVED_REGISTRATION_SECRET"
        results = [SimpleNamespace(returncode=1, stdout=secret, stderr=secret + " (HTTP 403)"),
                   SimpleNamespace(returncode=0, stdout="invalid json " + secret)]
        for result in results:
            with self.subTest(result=result), patch.object(manager.os, "getgrouplist", return_value=[1001]), \
                    patch.object(manager.subprocess, "run", return_value=result), \
                    contextlib.redirect_stdout(io.StringIO()) as output:
                with self.assertRaises(manager.rearm.GitHubApiError) as error:
                    self.api().request("repos/securely1g/sonic-buildimage/actions/runners/registration-token", method="POST")
                self.assertNotIn(secret, str(error.exception))
                self.assertNotIn(secret, output.getvalue())

    def test_wrong_github_account_fails_before_registration(self):
        api = self.api()
        with patch.object(api, "request", return_value={"login": "different-account"}) as request:
            with self.assertRaisesRegex(RuntimeError, "expected account"):
                api.verify_identity()
        request.assert_called_once_with("user")

    def test_registration_secret_is_only_sent_on_stdin_in_a_clean_child_environment(self):
        secret = "SHORT_LIVED_REGISTRATION_SECRET"
        registration = {"id": 42, "name": "sonic-vs-master-test", "attempt": "/data/sonic-runner/attempts/test"}
        response = SimpleNamespace(returncode=0, stdout=manager.rearm.REGISTRATION_PREFIX + json.dumps(registration))
        command = ["/usr/bin/python3", manager.REARM_PROGRAM, "--register", "--repo", CONFIG["repo"], "--master"]
        with patch.object(manager.rearm.subprocess, "run", return_value=response) as run, \
                contextlib.redirect_stdout(io.StringIO()) as output:
            self.assertEqual(manager.rearm.register_from_operator(command, secret, env=manager.ROOT_ENV), registration)
        self.assertEqual(run.call_args.kwargs["input"], secret + "\n")
        self.assertNotIn(secret, repr(run.call_args.args))
        self.assertNotIn(secret, repr(run.call_args.kwargs["env"]))
        self.assertNotIn(secret, output.getvalue())
        with patch.dict(os.environ, {"GH_TOKEN": "LONG_TERM_SECRET", "GH_CONFIG_DIR": "/home/lgh/.config/gh"}):
            self.assertFalse({"GH_TOKEN", "GITHUB_TOKEN", "GH_CONFIG_DIR", "SSH_AUTH_SOCK"} & manager.rearm.runner_environment().keys())


if __name__ == "__main__":
    unittest.main()
