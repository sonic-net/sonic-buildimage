"""Keep root-container Bazel ownership repair inside its startup directories."""

import contextlib
import io
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

from tools.bazel.ci import repair_bazel_startup as repair


class BazelStartupTest(unittest.TestCase):
    def test_corrupt_install_cleanup_preserves_other_installs_and_shared_caches(self):
        with tempfile.TemporaryDirectory() as temporary:
            home = Path(temporary)
            state = home / ".cache/bazel/_bazel_root"
            installation = state / "install" / ("a" * 32)
            nested = installation / "nested"
            nested.mkdir(parents=True)
            damaged = installation / "A-server.jar"
            damaged.write_text("corrupt")
            retained = home / ".cache/bazel-repo/object"
            retained.parent.mkdir()
            retained.write_text("shared")
            retained.chmod(0o444)
            before = retained.stat()
            os.link(retained, nested / "hardlink")
            (nested / "symlink").symlink_to(retained.parent, target_is_directory=True)
            sibling = state / "install" / ("b" * 32)
            sibling.mkdir()
            (sibling / "healthy").write_text("keep")
            output = state / "output-base"
            output.mkdir()
            (output / "state").write_text("keep")
            nested.chmod(0o555)
            with contextlib.redirect_stdout(io.StringIO()):
                repair.remove_corrupt_installation(state, installation, damaged)
            self.assertFalse(installation.exists())
            self.assertEqual((sibling / "healthy").read_text(), "keep")
            self.assertEqual((output / "state").read_text(), "keep")
            after = retained.stat()
            self.assertEqual((after.st_mode, after.st_uid, after.st_gid, after.st_mtime_ns),
                             (before.st_mode, before.st_uid, before.st_gid, before.st_mtime_ns))
            self.assertEqual(retained.read_text(), "shared")

    def test_corrupt_install_cleanup_rejects_unexpected_paths_and_symlink_roots(self):
        for scenario in ("outside", "bad-hash", "outside-file", "parent-file", "symlink"):
            with self.subTest(scenario=scenario), tempfile.TemporaryDirectory() as temporary:
                home = Path(temporary)
                state = home / ".cache/bazel/_bazel_root"
                installation = state / "install" / ("a" * 32)
                installation.mkdir(parents=True)
                damaged = installation / "A-server.jar"
                damaged.write_text("keep")
                selected = installation
                selected_file = damaged
                if scenario == "outside":
                    selected = home / ("a" * 32)
                elif scenario == "bad-hash":
                    selected = installation.parent
                elif scenario == "outside-file":
                    selected_file = home / "outside-file"
                elif scenario == "parent-file":
                    selected_file = installation / "../outside-file"
                elif scenario == "symlink":
                    selected = installation.parent / ("b" * 32)
                    selected.symlink_to(installation, target_is_directory=True)
                    selected_file = selected / "A-server.jar"
                with self.assertRaises((OSError, ValueError)):
                    repair.remove_corrupt_installation(state, selected, selected_file)
                self.assertEqual(damaged.read_text(), "keep")

    def test_corrupt_install_cleanup_rejects_install_and_nested_mounts(self):
        for component in ("install", "hash", "nested"):
            with self.subTest(component=component), tempfile.TemporaryDirectory() as temporary:
                state = Path(temporary) / ".cache/bazel/_bazel_root"
                installation = state / "install" / ("a" * 32)
                nested = installation / "nested"
                nested.mkdir(parents=True)
                damaged = nested / "A-server.jar"
                damaged.write_text("keep")
                mounted = {"install": installation.parent, "hash": installation,
                           "nested": nested}[component]
                original = repair.mount_id

                def selected_mount(descriptor):
                    path = Path(f"/proc/self/fd/{descriptor}").resolve()
                    return original(descriptor) + (path == mounted)

                with mock.patch.object(repair, "mount_id", side_effect=selected_mount), \
                        contextlib.redirect_stdout(io.StringIO()):
                    with self.assertRaisesRegex(ValueError, "mount"):
                        repair.remove_corrupt_installation(state, installation, damaged)
                self.assertEqual(damaged.read_text(), "keep")

    def test_startup_recovers_only_reported_install_and_retries_once(self):
        with tempfile.TemporaryDirectory() as temporary:
            state = Path(temporary) / ".cache/bazel/_bazel_root"
            installation = state / "install" / ("a" * 32)
            installation.mkdir(parents=True)
            damaged = installation / "A-server.jar"
            damaged.write_text("corrupt")
            error = (f"FATAL: corrupt installation: file '{damaged}' is missing or modified.  "
                     f"Please remove '{installation}' and try again.\n")
            results = [subprocess.CompletedProcess([], 36, "", error),
                       subprocess.CompletedProcess([], 0, "output-base\n", "")]
            with mock.patch.object(repair.subprocess, "run", side_effect=results) as run, \
                    contextlib.redirect_stdout(io.StringIO()) as output, \
                    contextlib.redirect_stderr(io.StringIO()) as errors:
                repair.verify_startup(state)
            self.assertEqual(run.call_count, 2)
            self.assertFalse(installation.exists())
            self.assertIn("Retrying Bazel startup once", output.getvalue())
            self.assertIn(error, errors.getvalue())
            for call in run.call_args_list:
                self.assertEqual(call.args[0], [
                    "bazel", "--batch", "--nosystem_rc", "--nohome_rc", "--noworkspace_rc",
                    "info", "output_base",
                ])

    def test_other_startup_results_do_not_trigger_cleanup_or_retry(self):
        for code, error in ((0, ""), (1, "failure"), (36, "permission denied")):
            with self.subTest(code=code), \
                    mock.patch.object(repair.subprocess, "run",
                                      return_value=subprocess.CompletedProcess([], code, "", error)) as run, \
                    mock.patch.object(repair, "remove_corrupt_installation") as remove, \
                    contextlib.redirect_stderr(io.StringIO()):
                if code:
                    with self.assertRaises(subprocess.CalledProcessError) as result:
                        repair.verify_startup(Path("/github/home/.cache/bazel/_bazel_root"))
                    self.assertEqual(result.exception.returncode, code)
                else:
                    repair.verify_startup(Path("/github/home/.cache/bazel/_bazel_root"))
                remove.assert_not_called()
                self.assertEqual(run.call_count, 1)

    def test_failed_recovery_stops_after_one_retry(self):
        state = Path("/github/home/.cache/bazel/_bazel_root")
        installation = state / "install" / ("a" * 32)
        error = (f"FATAL: corrupt installation: file '{installation}/A-server.jar' "
                 f"is missing or modified.  Please remove '{installation}' and try again.\n")
        with mock.patch.object(repair.subprocess, "run",
                               return_value=subprocess.CompletedProcess([], 36, "", error)) as run, \
                mock.patch.object(repair, "remove_corrupt_installation") as remove, \
                contextlib.redirect_stdout(io.StringIO()), \
                contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(subprocess.CalledProcessError) as result:
                repair.verify_startup(state)
            self.assertEqual(result.exception.returncode, 36)
            self.assertEqual(run.call_count, 2)
            remove.assert_called_once()

    def test_cli_preserves_bazel_startup_failure_exit_code(self):
        state = Path("/github/home/.cache/bazel/_bazel_root")
        with mock.patch.dict(os.environ, GITHUB_ACTIONS="true", HOME="/github/home"), \
                mock.patch.object(repair.os, "geteuid", return_value=0), \
                mock.patch.object(Path, "is_file", return_value=True), \
                mock.patch.object(repair, "repair_startup", return_value=state), \
                mock.patch.object(repair, "verify_startup",
                                  side_effect=subprocess.CalledProcessError(36, ["bazel"])), \
                mock.patch.object(sys, "argv", ["repair_bazel_startup.py"]), \
                contextlib.redirect_stdout(io.StringIO()), \
                contextlib.redirect_stderr(io.StringIO()) as errors:
            with self.assertRaises(SystemExit) as result:
                repair.main()
            self.assertEqual(result.exception.code, 36)
            self.assertIn("Bazel startup verification failed.", errors.getvalue())

    def test_creates_default_startup_directory(self):
        with tempfile.TemporaryDirectory() as temporary:
            home = Path(temporary)
            state = repair.repair_startup(home)
            self.assertEqual(state, home / ".cache/bazel/_bazel_root")
            self.assertEqual(state.stat().st_uid, os.geteuid())
            self.assertEqual(state.stat().st_gid, os.getegid())

    def test_only_directories_are_repaired_and_shared_files_are_preserved(self):
        with tempfile.TemporaryDirectory() as temporary:
            home = Path(temporary) / "home"
            state = home / ".cache/bazel/_bazel_root"
            nested = state / "install/hash"
            nested.mkdir(parents=True)
            outside = Path(temporary) / "shared"
            outside.mkdir()
            retained = outside / "cache-object"
            retained.write_text("keep")
            retained.chmod(0o444)
            os.link(retained, nested / "hardlink")
            (state / "symlink").symlink_to(outside, target_is_directory=True)
            regular = nested / "regular-file"
            regular.write_text("keep")
            before = retained.stat()
            changed = []

            def record(descriptor, uid, gid):
                self.assertEqual((uid, gid), (os.geteuid(), os.getegid()))
                changed.append(Path(f"/proc/self/fd/{descriptor}").resolve())

            with mock.patch.object(repair.os, "fchown", side_effect=record):
                repair.repair_startup(home)
            self.assertEqual(set(changed), {state, state / "install", nested})
            self.assertEqual(retained.stat(), before)
            self.assertEqual(regular.read_text(), "keep")
            self.assertTrue((state / "symlink").is_symlink())

    def test_symlinks_in_startup_path_are_rejected_without_changes(self):
        for component in ("home", ".cache", "bazel", "_bazel_root"):
            with self.subTest(component=component), tempfile.TemporaryDirectory() as temporary:
                home = Path(temporary) / "home"
                candidates = [home, home / ".cache", home / ".cache/bazel",
                              home / ".cache/bazel/_bazel_root"]
                redirected = next(path for path in candidates if path.name == component)
                redirected.parent.mkdir(parents=True, exist_ok=True)
                outside = Path(temporary) / "outside"
                outside.mkdir()
                redirected.symlink_to(outside, target_is_directory=True)
                with mock.patch.object(repair.os, "fchown") as chown:
                    with self.assertRaises(OSError):
                        repair.repair_startup(home)
                    chown.assert_not_called()
                self.assertEqual(list(outside.iterdir()), [])

    def test_nested_mounts_are_skipped_and_redirected_startup_mounts_rejected(self):
        for redirected in (False, True):
            with self.subTest(redirected=redirected), tempfile.TemporaryDirectory() as temporary:
                home = Path(temporary)
                state = home / ".cache/bazel/_bazel_root"
                mounted = state if redirected else state / "mounted"
                (mounted / "child").mkdir(parents=True)
                original_mount_id = repair.mount_id
                changed = []

                def selected_mount(descriptor):
                    path = Path(f"/proc/self/fd/{descriptor}").resolve()
                    return original_mount_id(descriptor) + (1 if path == mounted else 0)

                with mock.patch.object(repair, "mount_id", side_effect=selected_mount), \
                        mock.patch.object(repair.os, "fchown", side_effect=lambda fd, uid, gid:
                                          changed.append(Path(f"/proc/self/fd/{fd}").resolve())):
                    if redirected:
                        with self.assertRaisesRegex(ValueError, "redirected cache mount"):
                            repair.repair_startup(home)
                        self.assertEqual(changed, [])
                    else:
                        repair.repair_startup(home)
                        self.assertEqual(changed, [state])

    def test_permission_errors_are_not_hidden(self):
        with tempfile.TemporaryDirectory() as temporary, \
                mock.patch.object(repair.os, "fchown", side_effect=PermissionError("denied")):
            with self.assertRaisesRegex(PermissionError, "denied"):
                repair.repair_startup(Path(temporary))

    def test_cli_rejects_unexpected_execution_context_before_repair(self):
        for actions, home, uid, container in [
            ("false", "/github/home", 0, True),
            ("true", "/other/home", 0, True),
            ("true", "/github/home", 1000, True),
            ("true", "/github/home", 0, False),
        ]:
            with self.subTest(actions=actions, home=home, uid=uid, container=container), \
                    mock.patch.dict(os.environ, GITHUB_ACTIONS=actions, HOME=home), \
                    mock.patch.object(repair.os, "geteuid", return_value=uid), \
                    mock.patch.object(Path, "is_file", return_value=container), \
                    mock.patch.object(repair, "repair_startup") as restore, \
                    mock.patch.object(sys, "argv", ["repair_bazel_startup.py"]), \
                    contextlib.redirect_stderr(io.StringIO()) as errors:
                with self.assertRaises(SystemExit) as result:
                    repair.main()
                self.assertEqual(result.exception.code, 1)
                self.assertIn("Cannot repair Bazel startup ownership:", errors.getvalue())
                restore.assert_not_called()


if __name__ == "__main__":
    unittest.main()
