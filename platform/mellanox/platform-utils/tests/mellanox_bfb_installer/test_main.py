#!/usr/bin/env python3
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
Unit tests for mellanox_bfb_installer.main module.
"""

from contextlib import contextmanager
import logging
import os
import re
import time
import subprocess
import sys
import tempfile
import textwrap
import unittest
from unittest import mock

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))


class TestLoggingConfig(unittest.TestCase):
    """Tests for logging configuration."""

    def test_setup_logging_handlers(self):
        """Test setup_log_handlers() sets correct handlers."""
        from mellanox_bfb_installer.main import logger, setup_log_handlers

        @contextmanager
        def restore_log_handlers():
            original_handlers = logger.handlers
            yield
            logger.handlers = original_handlers

        with restore_log_handlers():
            setup_log_handlers()

            self.assertEqual(
                len(logger.handlers),
                2,
                "Expected two handlers (stdout and syslog) when syslog_enabled=True",
            )
            handler_types = [type(h) for h in logger.handlers]
            self.assertIn(
                logging.StreamHandler,
                handler_types,
                "Expected a StreamHandler (stdout)",
            )
            self.assertIn(
                logging.handlers.SysLogHandler,
                handler_types,
                "Expected a SysLogHandler when syslog_enabled=True",
            )

    def test_set_logging_level(self):
        """Test set_logging_level() sets correct levels on the root logger."""
        from mellanox_bfb_installer.main import setup_log_handlers, set_logging_level

        setup_log_handlers()

        from mellanox_bfb_installer.main import logger

        @contextmanager
        def set_logging_level_context(verbose: bool):
            original_level = logger.level
            yield
            logger.level = original_level

        with set_logging_level_context(verbose=False):
            set_logging_level(verbose=False)
            self.assertEqual(logger.level, logging.INFO)
            set_logging_level(verbose=True)
            self.assertEqual(logger.level, logging.DEBUG)


class TestLockFileOrExit(unittest.TestCase):
    """Tests for _lock_file_or_exit context manager."""

    def test_lock_file_or_exit_acquire_and_release_success(self):
        """Acquiring the lock with a temp file succeeds; exiting context releases it."""
        from mellanox_bfb_installer.main import _lock_file_or_exit

        with tempfile.NamedTemporaryFile(delete=False, prefix="bfb_installer_lock_") as f:
            lock_path = f.name
        try:
            with _lock_file_or_exit(lock_path) as lock_file:
                self.assertIsNotNone(lock_file)
                self.assertFalse(lock_file.closed)
            # Re-acquire after exit (same process): should succeed
            with _lock_file_or_exit(lock_path) as lock_file2:
                self.assertIsNotNone(lock_file2)
        finally:
            try:
                os.unlink(lock_path)
            except FileNotFoundError:
                pass

    def test_lock_file_or_exit_already_locked_exits(self):
        """When lock is held, _lock_file_or_exit causes process to exit with code 1."""
        from mellanox_bfb_installer.main import _lock_file_or_exit

        with tempfile.NamedTemporaryFile(delete=False, prefix="bfb_installer_lock_") as f:
            lock_path = f.name
        try:
            with _lock_file_or_exit(lock_path) as lock_file:
                self.assertIsNotNone(lock_file)
                self.assertFalse(lock_file.closed)
                child_script = textwrap.dedent(
                    f"""
                    import sys
                    sys.path.insert(0, {repr(os.path.join(os.path.dirname(__file__), '..'))})
                    from mellanox_bfb_installer.main import _lock_file_or_exit
                    with _lock_file_or_exit({repr(lock_path)}):
                        pass
                    """
                ).strip()
                child = subprocess.run(
                    [sys.executable, "-c", child_script],
                    capture_output=True,
                    timeout=2,
                )
                self.assertEqual(child.returncode, 1, "Process should exit 1 when lock is held")

        finally:
            try:
                os.unlink(lock_path)
            except FileNotFoundError:
                pass


class TestUsage(unittest.TestCase):
    """Tests for usage / help output."""

    def test_usage_syntax_contains_script_name(self):
        from mellanox_bfb_installer.main import SCRIPT_NAME, USAGE_SYNTAX

        self.assertIn(SCRIPT_NAME, USAGE_SYNTAX)
        self.assertIn("-b|--bfb", USAGE_SYNTAX)
        self.assertIn("--help", USAGE_SYNTAX)

    def test_usage_arguments_contains_all_options(self):
        """USAGE_ARGUMENTS includes the main visible options (--rshim is hidden)."""
        from mellanox_bfb_installer.main import USAGE_ARGUMENTS

        self.assertIn("-b|--bfb", USAGE_ARGUMENTS)
        self.assertIn("-d|--dpu", USAGE_ARGUMENTS)
        self.assertIn("-s|--skip-extract", USAGE_ARGUMENTS)
        self.assertIn("-v|--verbose", USAGE_ARGUMENTS)
        self.assertIn("-c|--config", USAGE_ARGUMENTS)
        self.assertIn("--debug-shell", USAGE_ARGUMENTS)
        self.assertIn("-h|--help", USAGE_ARGUMENTS)
        self.assertNotIn("--rshim", USAGE_ARGUMENTS)


class TestDpuNameToId(unittest.TestCase):
    """Tests for _dpu_name_to_id."""

    def test_valid_names(self):
        from mellanox_bfb_installer.main import _dpu_name_to_id

        self.assertEqual(_dpu_name_to_id("dpu0"), 0)
        self.assertEqual(_dpu_name_to_id("dpu1"), 1)
        self.assertEqual(_dpu_name_to_id("dpu12"), 12)

    def test_invalid_name_raises(self):
        from mellanox_bfb_installer.main import _dpu_name_to_id

        with self.assertRaises(ValueError):
            _dpu_name_to_id("rshim0")
        with self.assertRaises(ValueError):
            _dpu_name_to_id("DPU0")
        with self.assertRaises(ValueError):
            _dpu_name_to_id("dpu")


class TestGenerateAdditionalConfigLines(unittest.TestCase):
    """Tests for _generate_additional_config_lines."""

    def _make_target(self, idx: int):
        from mellanox_bfb_installer.device_selection import TargetInfo

        return TargetInfo(
            dpu=f"dpu{idx}",
            rshim=f"rshim{idx}",
            dpu_pci_bus_id=f"0000:0{idx}:00.0",
            rshim_pci_bus_id=f"0000:0{idx}:00.1",
        )

    def test_npu_time_and_per_rshim_dpu_ids_are_present(self):
        """Generate one shared time and one RShim-prefixed DPU ID per target."""
        from mellanox_bfb_installer.main import _generate_additional_config_lines

        t_before = int(time.time())
        result = _generate_additional_config_lines(
            [self._make_target(0), self._make_target(3)]
        )
        t_after = int(time.time())

        match = re.fullmatch(
            r"NPU_TIME=(\d+)\nRSHIM0_DPU_ID=0\nRSHIM3_DPU_ID=3\n",
            result,
        )
        self.assertIsNotNone(match, f"Unexpected additional config: {result!r}")
        npu_time = int(match.group(1))
        self.assertGreaterEqual(npu_time, t_before)
        self.assertLessEqual(npu_time, t_after)

    def test_debug_shell_flag_omits_line_when_false(self):
        """When debug_shell=False (default), DEBUG_SHELL is not present in the output."""
        from mellanox_bfb_installer.main import _generate_additional_config_lines

        result = _generate_additional_config_lines([self._make_target(0)])
        self.assertNotIn("DEBUG_SHELL", result)

    def test_debug_shell_flag_adds_line_when_true(self):
        """When debug_shell=True, append the shared DEBUG_SHELL setting."""
        from mellanox_bfb_installer.main import _generate_additional_config_lines

        result = _generate_additional_config_lines(
            [self._make_target(2)], debug_shell=True
        )
        self.assertIn("RSHIM2_DPU_ID=2\n", result)
        self.assertIn("DEBUG_SHELL=true\n", result)

    def test_flag_without_url_raises(self):
        """Reject dump collection without its upload URL."""
        from mellanox_bfb_installer.main import _generate_additional_config_lines

        with self.assertRaises(ValueError):
            _generate_additional_config_lines(
                [self._make_target(0)],
                collect_dump_on_failure=True,
            )

    def test_url_without_flag_raises(self):
        """Reject an upload URL when dump collection is disabled."""
        from mellanox_bfb_installer.main import _generate_additional_config_lines

        with self.assertRaises(ValueError):
            _generate_additional_config_lines(
                [self._make_target(0)],
                dump_upload_base_url="http://192.168.100.254:8090",
            )

    def test_all_flags_are_added_to_shared_config(self):
        """Generate per-RShim IDs and all shared recovery settings."""
        from mellanox_bfb_installer.main import _generate_additional_config_lines

        result = _generate_additional_config_lines(
            [self._make_target(1), self._make_target(5)],
            debug_shell=True,
            collect_dump_on_failure=True,
            dump_upload_base_url="http://192.168.100.254:8090",
        )

        self.assertRegex(result, r"^NPU_TIME=\d+\n")
        self.assertIn("RSHIM1_DPU_ID=1\n", result)
        self.assertIn("RSHIM5_DPU_ID=5\n", result)
        self.assertIn("DEBUG_SHELL=true\n", result)
        self.assertIn("COLLECT_DUMP_ON_FAILURE=true\n", result)
        self.assertIn(
            "DUMP_UPLOAD_BASE_URL=http://192.168.100.254:8090\n",
            result,
        )


class TestBuildSharedConfig(unittest.TestCase):
    """Tests for the shared DOCA configuration."""

    def test_appends_additional_lines_to_user_config(self):
        from mellanox_bfb_installer import main

        with tempfile.TemporaryDirectory(prefix="shared_config_test_") as work_dir:
            config_path = os.path.join(work_dir, "bf.cfg")
            with open(config_path, "w") as config_file:
                config_file.write("USER_VALUE=1\n")

            generated_path = main._build_shared_config(
                config_path, "NPU_TIME=123\n", work_dir
            )

            self.assertNotEqual(generated_path, config_path)
            self.assertTrue(generated_path.endswith(".cfg"))
            with open(generated_path) as generated_config:
                self.assertEqual(
                    generated_config.read(), "USER_VALUE=1\n\nNPU_TIME=123\n\n"
                )

    def test_creates_config_when_user_config_is_absent(self):
        from mellanox_bfb_installer import main

        with tempfile.TemporaryDirectory(prefix="shared_config_test_") as work_dir:
            generated_path = main._build_shared_config(None, "NPU_TIME=123\n", work_dir)

            self.assertIn("empty-config.", generated_path)
            self.assertTrue(generated_path.endswith(".cfg"))
            with open(generated_path) as generated_config:
                self.assertEqual(generated_config.read(), "\nNPU_TIME=123\n\n")


class TestDumpReceiverSubprocess(unittest.TestCase):
    """Tests for the _dump_receiver_subprocess context manager.

    These are pure unit tests: subprocess.Popen and time.sleep are mocked, so
    no actual child process is spawned and no real socket is bound.
    """

    def test_popen_is_invoked_with_expected_argv(self):
        """The child process is launched with -m mellanox_bfb_installer.dump_receiver
        and the right CLI args."""
        from mellanox_bfb_installer import main

        mock_proc = mock.MagicMock()
        # First poll() = None (alive after 0.5s sleep), second poll() = 0 (on teardown still alive)
        mock_proc.poll.side_effect = [None, None]
        mock_proc.wait.return_value = 0
        mock_proc.pid = 4242

        with (
            mock.patch.object(main.subprocess, "Popen", return_value=mock_proc) as mock_popen,
            mock.patch.object(main.time, "sleep"),
        ):
            with main._dump_receiver_subprocess(
                output_dir="/var/log/dpu",
                bind_ip="10.0.0.5",
                port=9001,
                verbose=True,
            ) as proc:
                self.assertIs(proc, mock_proc)

        mock_popen.assert_called_once()
        argv = mock_popen.call_args[0][0]
        # Python interpreter, then -m, then module name.
        self.assertEqual(argv[0], sys.executable)
        self.assertIn("-m", argv)
        self.assertIn("mellanox_bfb_installer.dump_receiver", argv)
        self.assertIn("--bind-ip", argv)
        self.assertIn("10.0.0.5", argv)
        self.assertIn("--port", argv)
        self.assertIn("9001", argv)
        self.assertIn("--output-dir", argv)
        self.assertIn("/var/log/dpu", argv)
        self.assertIn("--verbose", argv)

    def test_normal_lifecycle_terminates_proc_on_exit(self):
        """Happy path: yields the proc, then SIGTERMs it on context exit."""
        from mellanox_bfb_installer import main

        mock_proc = mock.MagicMock()
        # poll() after 0.5s sleep: still alive (None). On teardown: also alive (None) -> terminate.
        mock_proc.poll.side_effect = [None, None]
        mock_proc.wait.return_value = 0

        with (
            mock.patch.object(main.subprocess, "Popen", return_value=mock_proc),
            mock.patch.object(main.time, "sleep"),
        ):
            with main._dump_receiver_subprocess("/tmp/out") as proc:
                self.assertIs(proc, mock_proc)

        mock_proc.terminate.assert_called_once()
        mock_proc.wait.assert_called()
        mock_proc.kill.assert_not_called()

    def test_popen_oserror_yields_none_and_proceeds(self):
        """OSError on Popen (e.g. exec failure) must not raise; context yields None."""
        from mellanox_bfb_installer import main

        with (
            mock.patch.object(main.subprocess, "Popen", side_effect=OSError("no exec")),
            mock.patch.object(main.time, "sleep"),
        ):
            with main._dump_receiver_subprocess("/tmp/out") as proc:
                self.assertIsNone(proc)
        # Nothing to assert on teardown: no proc was ever started.

    def test_proc_exits_immediately_yields_none_no_teardown(self):
        """If the child exits before the 0.5s readiness sleep, we yield None and
        don't try to SIGTERM/SIGKILL it later."""
        from mellanox_bfb_installer import main

        mock_proc = mock.MagicMock()
        # poll() = 1 right after Popen (process exited immediately).
        mock_proc.poll.return_value = 1
        mock_proc.returncode = 1

        with (
            mock.patch.object(main.subprocess, "Popen", return_value=mock_proc),
            mock.patch.object(main.time, "sleep"),
        ):
            with main._dump_receiver_subprocess("/tmp/out") as proc:
                self.assertIsNone(proc)

        mock_proc.terminate.assert_not_called()
        mock_proc.kill.assert_not_called()

    def test_terminate_timeout_falls_back_to_kill(self):
        """If proc.wait() after terminate() times out, we SIGKILL the child."""
        from mellanox_bfb_installer import main

        mock_proc = mock.MagicMock()
        mock_proc.poll.side_effect = [None, None]
        # First wait (after terminate) times out, second wait (after kill) succeeds.
        mock_proc.wait.side_effect = [
            subprocess.TimeoutExpired(cmd="dump_receiver", timeout=5.0),
            0,
        ]
        mock_proc.pid = 1234

        with (
            mock.patch.object(main.subprocess, "Popen", return_value=mock_proc),
            mock.patch.object(main.time, "sleep"),
        ):
            with main._dump_receiver_subprocess("/tmp/out"):
                pass

        mock_proc.terminate.assert_called_once()
        mock_proc.kill.assert_called_once()


class TestInstallOnDpusDumpLifecycle(unittest.TestCase):
    """Tests that _install_on_dpus sets up and tears down the tmfifo bridge + receiver
    when collect_dump_on_failure is True, and skips that wiring otherwise.

    Heavy mocking: device_selection.get_targets, doca_install_core,
    tmfifo_bridge.TmfifoBridge, and _dump_receiver_subprocess are all stubbed so
    no real network/system state is touched.
    """

    def _make_targets(self, count: int):
        from mellanox_bfb_installer.device_selection import TargetInfo

        return [
            TargetInfo(
                dpu=f"dpu{i}",
                rshim=f"rshim{i}",
                dpu_pci_bus_id=f"0000:0{i}:00.0",
                rshim_pci_bus_id=f"0000:0{i}:00.1",
            )
            for i in range(count)
        ]

    @contextmanager
    def _common_mocks(self, targets, install_failures=0, setup_side_effect=None):
        """Patch the boundary modules that _install_on_dpus depends on.

        Yields a dict of MagicMocks for assertions.
        """
        from mellanox_bfb_installer import main

        bridge_inst = mock.MagicMock()
        if setup_side_effect is not None:
            bridge_inst.setup.side_effect = setup_side_effect
        bridge_cls = mock.MagicMock(return_value=bridge_inst)

        @contextmanager
        def fake_dump_receiver(*args, **kwargs):
            yield mock.MagicMock()

        with (
            mock.patch.object(main.device_selection, "get_targets", return_value=list(targets)),
            mock.patch.object(
                main, "_build_shared_config", return_value="/tmp/combined.cfg"
            ) as mock_build_config,
            mock.patch.object(main.tmfifo_bridge, "TmfifoBridge", bridge_cls),
            mock.patch.object(
                main, "_dump_receiver_subprocess", side_effect=fake_dump_receiver
            ) as mock_recv,
            mock.patch.object(
                main.doca_install_core,
                "install_bfb_on_targets",
                return_value=install_failures,
            ) as mock_install,
        ):
            yield {
                "bridge_cls": bridge_cls,
                "bridge_inst": bridge_inst,
                "build_config": mock_build_config,
                "receiver": mock_recv,
                "install": mock_install,
            }

    def test_exits_when_doca_installation_fails(self):
        from mellanox_bfb_installer import main

        targets = self._make_targets(1)
        with self._common_mocks(targets, install_failures=1):
            with self.assertRaises(SystemExit) as error:
                main._install_on_dpus(
                    bfb_path="/tmp/image.bfb",
                    work_dir="/tmp",
                    rshims=None,
                    dpus="all",
                    verbose=False,
                    config=None,
                )

        self.assertEqual(error.exception.code, 1)

    def test_bridge_and_receiver_started_and_torn_down_on_success(self):
        from mellanox_bfb_installer import main

        targets = self._make_targets(1)
        with self._common_mocks(targets) as m:
            main._install_on_dpus(
                bfb_path="/tmp/x.bfb",
                work_dir="/tmp/work",
                rshims=None,
                dpus="all",
                verbose=False,
                config=None,
                debug_shell=False,
                collect_dump_on_failure=True,
                dump_output_dir="/tmp/dumps",
            )
            m["bridge_inst"].setup.assert_called_once()
            m["bridge_inst"].teardown.assert_called_once()
            m["receiver"].assert_called_once()
            m["install"].assert_called_once()
            additional_lines = m["build_config"].call_args[0][1]
            self.assertIn("RSHIM0_DPU_ID=0\n", additional_lines)
            self.assertIn("COLLECT_DUMP_ON_FAILURE=true\n", additional_lines)
            self.assertIn("DUMP_UPLOAD_BASE_URL=http://", additional_lines)
            self.assertEqual(m["build_config"].call_args[0][2], "/tmp/work")
            m["install"].assert_called_once_with(
                targets=targets,
                bfb_path="/tmp/x.bfb",
                config_path="/tmp/combined.cfg",
                verbose=False,
            )

    def test_bridge_teardown_runs_even_when_install_fails(self):
        """A DOCA failure exits nonzero after the bridge is torn down."""
        from mellanox_bfb_installer import main

        targets = self._make_targets(2)
        with self._common_mocks(targets, install_failures=1) as m:
            with self.assertRaises(SystemExit) as cm:
                main._install_on_dpus(
                    bfb_path="/tmp/x.bfb",
                    work_dir="/tmp/work",
                    rshims=None,
                    dpus="all",
                    verbose=False,
                    config=None,
                    debug_shell=False,
                    collect_dump_on_failure=True,
                    dump_output_dir="/tmp/dumps",
                )
            self.assertEqual(cm.exception.code, 1)
            m["bridge_inst"].setup.assert_called_once()
            m["bridge_inst"].teardown.assert_called_once()

    def test_collect_dump_disabled_skips_bridge_and_receiver(self):
        """collect_dump_on_failure=False -> no bridge, no receiver, but install still runs."""
        from mellanox_bfb_installer import main

        targets = self._make_targets(1)
        with self._common_mocks(targets) as m:
            main._install_on_dpus(
                bfb_path="/tmp/x.bfb",
                work_dir="/tmp/work",
                rshims=None,
                dpus="all",
                verbose=False,
                config=None,
                debug_shell=False,
                collect_dump_on_failure=False,
                dump_output_dir="/tmp/dumps",
            )
            m["bridge_cls"].assert_not_called()
            m["bridge_inst"].setup.assert_not_called()
            m["bridge_inst"].teardown.assert_not_called()
            m["receiver"].assert_not_called()
            m["install"].assert_called_once()
            additional_lines = m["build_config"].call_args[0][1]
            self.assertNotIn("COLLECT_DUMP_ON_FAILURE", additional_lines)
            self.assertNotIn("DUMP_UPLOAD_BASE_URL", additional_lines)

    def test_bridge_setup_failure_degrades_gracefully(self):
        """If bridge.setup() raises CalledProcessError, install proceeds without it
        (and bridge.teardown is NOT called, since setup never succeeded)."""
        from mellanox_bfb_installer import main

        targets = self._make_targets(1)
        setup_err = subprocess.CalledProcessError(returncode=1, cmd=["ip"])
        with self._common_mocks(targets, setup_side_effect=setup_err) as m:
            main._install_on_dpus(
                bfb_path="/tmp/x.bfb",
                work_dir="/tmp/work",
                rshims=None,
                dpus="all",
                verbose=False,
                config=None,
                debug_shell=False,
                collect_dump_on_failure=True,
                dump_output_dir="/tmp/dumps",
            )
            m["bridge_inst"].setup.assert_called_once()
            m["bridge_inst"].teardown.assert_not_called()
            m["receiver"].assert_not_called()
            m["install"].assert_called_once()

    def test_invalid_config_exits_before_target_selection(self):
        """Reject a missing user config before touching DPU state."""
        from mellanox_bfb_installer import main

        missing_config = "/tmp/missing-bfb-installer.cfg"
        with (
            mock.patch.object(main.os.path, "isfile", return_value=False),
            mock.patch.object(main.device_selection, "get_targets") as get_targets,
            mock.patch.object(main, "print_usage") as print_usage,
            mock.patch.object(main, "logger") as logger,
        ):
            with self.assertRaises(SystemExit) as error:
                main._install_on_dpus(
                    bfb_path="/tmp/image.bfb",
                    work_dir="/tmp/work",
                    rshims=None,
                    dpus="dpu0",
                    verbose=False,
                    config=missing_config,
                )

        self.assertEqual(error.exception.code, 1)
        logger.error.assert_called_once()
        assert missing_config in logger.error.call_args.args
        assert "comma-separated per-DPU configs are unsupported" in logger.error.call_args.args[0]
        print_usage.assert_called_once_with()
        get_targets.assert_not_called()


if __name__ == "__main__":
    unittest.main()
