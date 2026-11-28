#!/usr/bin/env python3
"""Exercise the supplied BMv2 CLI harness without launching any subprocess."""

import contextlib
import importlib.machinery
import importlib.util
import io
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock


if len(sys.argv) < 2:
    raise SystemExit("usage: test_bmv2_cli_reservations.py run_one_test.py.in")
TEMPLATE = Path(sys.argv.pop(1)).resolve(strict=True)


class ReservationTests(unittest.TestCase):
    def setUp(self):
        loader = importlib.machinery.SourceFileLoader("bmv2_cli_harness", str(TEMPLATE))
        spec = importlib.util.spec_from_loader(loader.name, loader)
        self.module = importlib.util.module_from_spec(spec)
        loader.exec_module(self.module)
        temporary = tempfile.TemporaryDirectory(prefix="bmv2-cli-reservations-")
        self.addCleanup(temporary.cleanup)
        previous = os.getcwd()
        os.chdir(temporary.name)
        self.addCleanup(os.chdir, previous)
        self.stack = contextlib.ExitStack()
        self.addCleanup(self.stack.close)
        self.stack.enter_context(mock.patch.object(self.module.time, "sleep"))
        self.random = self.stack.enter_context(
            mock.patch.object(self.module.random, "randint", return_value=380))
        self.lock = Path("lock_380")
        self.events = []
        self.warmup = mock.Mock()
        self.warmup.wait.return_value = 0
        self.switch = mock.Mock()
        self.switch.kill.side_effect = lambda: self.event("kill")
        self.switch.wait.side_effect = lambda: self.event("wait")
        self.cli = mock.Mock(returncode=0)
        self.cli.communicate.return_value = (b"RuntimeCmd: ok\nRuntimeCmd: ", None)
        self.popen = self.stack.enter_context(
            mock.patch.object(self.module.subprocess, "Popen", side_effect=self.spawn))
        self.remove = self.stack.enter_context(
            mock.patch.object(self.module.os, "remove", side_effect=self.unlink))
        self.stack.enter_context(mock.patch.object(
            self.module.sys, "argv", ["run_one_test.py", ".", "case", "ecmp.json"]))
        Path("case.in").write_text("table_num_entries ecmp_group\n")
        Path("case.out").write_text("????\nok\n????\n")
        self.output = self.stack.enter_context(contextlib.redirect_stdout(io.StringIO()))

    def event(self, name):
        self.assertTrue(self.lock.is_dir(), "reservation released before shutdown")
        self.events.append(name)
        return 0

    def unlink(self, path):
        self.assertEqual(path, "/tmp/bmv2-380-notifications.ipc")
        self.event("unlink")

    def spawn(self, args, **kwargs):
        self.assertTrue(self.lock.is_dir(), "switch launched without reservation")
        if args[-1] == "-h":
            return self.warmup
        if args[0].endswith("/simple_switch"):
            self.assertEqual(args[-4:], ["--thrift-port", "9470", "--device-id", "380"])
            return self.switch
        self.assertTrue(args[0].endswith("/runtime_CLI.py"))
        return self.cli

    def main_exit(self, expected):
        with self.assertRaises(SystemExit) as raised:
            self.module.main()
        self.assertEqual(raised.exception.code, expected)

    def cleaned(self):
        self.assertEqual(self.events, ["kill", "wait", "unlink"])
        self.assertFalse(self.lock.exists())

    def test_live_reservation_forces_another_candidate(self):
        first = self.module.ConcurrentInteger(os.getcwd(), 1000)
        second = self.module.ConcurrentInteger(os.getcwd(), 1000)
        self.assertEqual(first.generate(), 380)
        self.random.side_effect = [380, 381]
        self.assertEqual(second.generate(), 381)
        self.assertTrue(self.lock.is_dir())
        self.assertTrue(Path("lock_381").is_dir())
        first.release(380)
        self.random.side_effect = None
        self.assertEqual(first.generate(), 380)
        first.release(380)
        second.release(381)

    def test_exhausted_candidates_do_not_steal_reservation(self):
        allocator = self.module.ConcurrentInteger(os.getcwd(), 1000)
        self.assertEqual(allocator.generate(), 380)
        self.assertIsNone(allocator.generate())
        self.assertTrue(self.lock.is_dir())
        allocator.release(380)

    def test_success_waits_before_endpoint_removal_and_release(self):
        self.main_exit(0)
        self.cleaned()

    def test_cli_nonzero_keeps_exit_three_and_cleans(self):
        self.cli.returncode = 1
        self.main_exit(3)
        self.cleaned()

    def test_comparison_failure_keeps_exit_four_and_diagnostics(self):
        self.cli.communicate.return_value = (b"RuntimeCmd: wrong\nRuntimeCmd: ", None)
        self.main_exit(4)
        self.assertIn("Expected\nok", self.output.getvalue())
        self.assertIn("But got\nwrong", self.output.getvalue())
        self.cleaned()

    def test_empty_output_failure_cleans(self):
        self.cli.communicate.return_value = (b"", None)
        with self.assertRaises(AssertionError):
            self.module.main()
        self.cleaned()

    def test_unexpected_cli_exception_cleans_and_propagates(self):
        error = RuntimeError("injected CLI failure")
        self.cli.communicate.side_effect = error
        with self.assertRaises(RuntimeError) as raised:
            self.module.main()
        self.assertIs(raised.exception, error)
        self.cleaned()

    def test_missing_expected_output_cleans(self):
        Path("case.out").unlink()
        with self.assertRaises(FileNotFoundError):
            self.module.main()
        self.cleaned()

    def test_invalid_output_encoding_cleans(self):
        self.cli.communicate.return_value = (b"\xff", None)
        with self.assertRaises(UnicodeDecodeError):
            self.module.main()
        self.cleaned()

    def test_switch_launch_failure_releases_without_process_cleanup(self):
        self.popen.side_effect = [self.warmup, OSError("injected launch failure")]
        with self.assertRaises(OSError):
            self.module.main()
        self.assertFalse(self.lock.exists())
        self.assertEqual(self.events, [])
        self.remove.assert_not_called()

    def test_warmup_launch_failure_releases(self):
        self.popen.side_effect = OSError("injected warmup failure")
        with self.assertRaises(OSError):
            self.module.main()
        self.assertFalse(self.lock.exists())
        self.remove.assert_not_called()

    def test_uncertain_kill_retains_reservation(self):
        self.switch.kill.side_effect = PermissionError("injected kill failure")
        with self.assertRaises(PermissionError):
            self.module.main()
        self.assertTrue(self.lock.is_dir())
        self.switch.wait.assert_not_called()
        self.remove.assert_not_called()

    def test_uncertain_wait_retains_reservation(self):
        self.switch.wait.side_effect = TimeoutError("injected wait failure")
        with self.assertRaises(TimeoutError):
            self.module.main()
        self.assertTrue(self.lock.is_dir())
        self.remove.assert_not_called()

    def test_already_exited_switch_is_reaped_before_release(self):
        self.switch.kill.side_effect = ProcessLookupError()
        self.main_exit(0)
        self.assertEqual(self.events, ["wait", "unlink"])
        self.assertFalse(self.lock.exists())

    def test_missing_ipc_file_still_releases_after_wait(self):
        self.remove.side_effect = FileNotFoundError()
        self.main_exit(0)
        self.assertEqual(self.events, ["kill", "wait"])
        self.assertFalse(self.lock.exists())

    def test_ipc_cleanup_failure_does_not_reuse_endpoint(self):
        self.remove.side_effect = PermissionError("injected unlink failure")
        with self.assertRaises(PermissionError):
            self.module.main()
        self.assertEqual(self.events, ["kill", "wait"])
        self.assertTrue(self.lock.is_dir())

    def test_no_available_reservation_does_not_launch(self):
        self.lock.mkdir()
        self.main_exit(2)
        self.popen.assert_not_called()
        self.assertTrue(self.lock.is_dir())


if __name__ == "__main__":
    unittest.main()
