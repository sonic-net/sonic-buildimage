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
            config_path=None,
        )

    def test_npu_time_and_dpu_id_present_and_correct(self):
        """Returned string contains NPU_TIME=<integer> and DPU_ID=<dpu index> lines."""
        from mellanox_bfb_installer.main import _generate_additional_config_lines

        target = self._make_target(3)

        t_before = int(time.time())
        result = _generate_additional_config_lines(target)
        t_after = int(time.time())

        match = re.fullmatch(r"NPU_TIME=(\d+)\nDPU_ID=(\d+)\n", result)
        self.assertIsNotNone(
            match, f"Expected 'NPU_TIME=<int>\\nDPU_ID=<int>\\n' but got {result!r}"
        )
        npu_time = int(match.group(1))
        dpu_id = int(match.group(2))

        self.assertGreaterEqual(npu_time, t_before)
        self.assertLessEqual(npu_time, t_after)
        self.assertEqual(dpu_id, 3)

    def test_debug_shell_flag_omits_line_when_false(self):
        """When debug_shell=False (default), DEBUG_SHELL is not present in the output."""
        from mellanox_bfb_installer.main import _generate_additional_config_lines

        target = self._make_target(0)
        result = _generate_additional_config_lines(target)
        self.assertNotIn("DEBUG_SHELL", result)

    def test_debug_shell_flag_adds_line_when_true(self):
        """When debug_shell=True, DEBUG_SHELL=true line is appended after DPU_ID."""
        from mellanox_bfb_installer.main import _generate_additional_config_lines

        target = self._make_target(2)
        result = _generate_additional_config_lines(target, debug_shell=True)
        match = re.fullmatch(
            r"NPU_TIME=(\d+)\nDPU_ID=(\d+)\nDEBUG_SHELL=true\n", result
        )
        self.assertIsNotNone(
            match,
            f"Expected NPU_TIME/DPU_ID/DEBUG_SHELL block but got {result!r}",
        )
        self.assertEqual(int(match.group(2)), 2)


class TestAddAdditionalConfigLines(unittest.TestCase):
    """Tests for _add_additional_config_lines."""

    def _make_target(self, idx: int, config_path):
        """Create a TargetInfo for testing."""
        from mellanox_bfb_installer.device_selection import TargetInfo

        return TargetInfo(
            dpu=f"dpu{idx}",
            rshim=f"rshim{idx}",
            dpu_pci_bus_id=f"0000:0{idx}:00.0",
            rshim_pci_bus_id=f"0000:0{idx}:00.1",
            config_path=config_path,
        )

    def _assert_has_npu_time_and_dpu_id(self, content: str, expected_dpu_id: int):
        match = re.search(r"NPU_TIME=(\d+)\nDPU_ID=(\d+)\n", content)
        self.assertIsNotNone(match, f"NPU_TIME/DPU_ID lines not found in {content!r}")
        self.assertEqual(int(match.group(2)), expected_dpu_id)

    def test_single_target_creates_temp_file_with_original_plus_additional_lines(self):
        """Single target: creates new file with original contents + per-DPU additional lines."""
        from mellanox_bfb_installer import main

        with tempfile.TemporaryDirectory(prefix="add_config_test_") as tempdir:
            config_path = os.path.join(tempdir, "config.json")
            with open(config_path, "w") as f:
                f.write("original_line\n")
            target = self._make_target(0, config_path)
            targets = [target]
            main._add_additional_config_lines(targets, tempdir)
            self.assertEqual(len(targets), 1)
            self.assertEqual(targets[0].dpu, "dpu0")
            self.assertEqual(targets[0].rshim, "rshim0")
            self.assertEqual(targets[0].dpu_pci_bus_id, "0000:00:00.0")
            self.assertEqual(targets[0].rshim_pci_bus_id, "0000:00:00.1")
            new_path = targets[0].config_path
            self.assertNotEqual(new_path, config_path)
            self.assertTrue(new_path.startswith(os.path.join(tempdir, "config.json.")))
            with open(new_path, "r") as f:
                content = f.read()
            self.assertIn("original_line\n", content)
            self._assert_has_npu_time_and_dpu_id(content, expected_dpu_id=0)

    def test_two_targets_same_config_create_distinct_files_with_per_dpu_ids(self):
        """Two targets sharing a config get distinct temp files, each with its own DPU_ID."""
        from mellanox_bfb_installer import main

        with tempfile.TemporaryDirectory(prefix="add_config_test_") as tempdir:
            config_path = os.path.join(tempdir, "shared.json")
            with open(config_path, "w") as f:
                f.write("shared_content\n")
            targets = [
                self._make_target(0, config_path),
                self._make_target(1, config_path),
            ]
            main._add_additional_config_lines(targets, tempdir)
            self.assertEqual(len(targets), 2)
            self.assertEqual(targets[0].dpu, "dpu0")
            self.assertEqual(targets[1].dpu, "dpu1")

            self.assertNotEqual(
                targets[0].config_path,
                targets[1].config_path,
                "Each target must get its own temp file since DPU_ID differs",
            )
            for tgt, expected_dpu_id in [(targets[0], 0), (targets[1], 1)]:
                self.assertTrue(tgt.config_path.startswith(os.path.join(tempdir, "shared.json.")))
                with open(tgt.config_path, "r") as f:
                    content = f.read()
                self.assertIn("shared_content\n", content)
                self._assert_has_npu_time_and_dpu_id(content, expected_dpu_id=expected_dpu_id)

    def test_two_targets_different_configs_create_two_files(self):
        """Two targets with different configs: two temp files, each target gets correct path."""
        from mellanox_bfb_installer import main

        with tempfile.TemporaryDirectory(prefix="add_config_test_") as tempdir:
            config1 = os.path.join(tempdir, "cfg1.json")
            config2 = os.path.join(tempdir, "cfg2.json")
            with open(config1, "w") as f:
                f.write("config1_content\n")
            with open(config2, "w") as f:
                f.write("config2_content\n")
            targets = [
                self._make_target(1, config1),
                self._make_target(2, config2),
            ]
            main._add_additional_config_lines(targets, tempdir)
            self.assertNotEqual(targets[0].config_path, targets[1].config_path)
            self.assertIn("cfg1.json.", targets[0].config_path)
            self.assertIn("cfg2.json.", targets[1].config_path)
            with open(targets[0].config_path, "r") as f:
                content1 = f.read()
            with open(targets[1].config_path, "r") as f:
                content2 = f.read()
            self.assertIn("config1_content\n", content1)
            self.assertIn("config2_content\n", content2)
            self._assert_has_npu_time_and_dpu_id(content1, expected_dpu_id=1)
            self._assert_has_npu_time_and_dpu_id(content2, expected_dpu_id=2)

    def test_target_with_config_none_creates_empty_config_file(self):
        """Two targets with config_path None: each gets its own temp file with only additional lines."""
        from mellanox_bfb_installer import main

        with tempfile.TemporaryDirectory(prefix="add_config_test_") as tempdir:
            targets = [
                self._make_target(0, None),
                self._make_target(1, None),
            ]
            main._add_additional_config_lines(targets, tempdir)
            self.assertEqual(len(targets), 2)
            self.assertNotEqual(
                targets[0].config_path,
                targets[1].config_path,
                "Each target must get its own temp file since DPU_ID differs",
            )
            for tgt, expected_dpu_id in [(targets[0], 0), (targets[1], 1)]:
                self.assertIsNotNone(tgt.config_path)
                self.assertIn("empty-config.", tgt.config_path)
                with open(tgt.config_path, "r") as f:
                    content = f.read()
                self._assert_has_npu_time_and_dpu_id(content, expected_dpu_id=expected_dpu_id)
                self.assertNotIn("DEBUG_SHELL", content)

    def test_debug_shell_true_propagates_to_temp_config(self):
        """debug_shell=True writes 'DEBUG_SHELL=true' into each target's temp config."""
        from mellanox_bfb_installer import main

        with tempfile.TemporaryDirectory(prefix="add_config_test_") as tempdir:
            targets = [
                self._make_target(0, None),
                self._make_target(1, None),
            ]
            main._add_additional_config_lines(targets, tempdir, debug_shell=True)
            for tgt, expected_dpu_id in [(targets[0], 0), (targets[1], 1)]:
                with open(tgt.config_path, "r") as f:
                    content = f.read()
                self._assert_has_npu_time_and_dpu_id(content, expected_dpu_id=expected_dpu_id)
                self.assertIn("DEBUG_SHELL=true\n", content)


class TestGenerateAdditionalConfigLinesDumpReceiver(unittest.TestCase):
    """Tests for _generate_additional_config_lines with the dump-receiver knobs."""

    def _make_target(self, idx: int):
        from mellanox_bfb_installer.device_selection import TargetInfo

        return TargetInfo(
            dpu=f"dpu{idx}",
            rshim=f"rshim{idx}",
            dpu_pci_bus_id=f"0000:0{idx}:00.0",
            rshim_pci_bus_id=f"0000:0{idx}:00.1",
            config_path=None,
        )

    def test_flag_without_url_raises(self):
        """collect_dump_on_failure=True with dump_upload_base_url=None is inconsistent and must raise.

        The DPU-side installer needs both pieces of info to be useful; a half-configured bf.cfg
        is rejected up front rather than silently emitted.
        """
        from mellanox_bfb_installer.main import _generate_additional_config_lines

        with self.assertRaises(ValueError):
            _generate_additional_config_lines(
                self._make_target(0),
                collect_dump_on_failure=True,
                dump_upload_base_url=None,
            )

    def test_url_without_flag_raises(self):
        """dump_upload_base_url given but collect_dump_on_failure=False is inconsistent and must raise."""
        from mellanox_bfb_installer.main import _generate_additional_config_lines

        with self.assertRaises(ValueError):
            _generate_additional_config_lines(
                self._make_target(0),
                collect_dump_on_failure=False,
                dump_upload_base_url="http://192.168.100.254:8090",
            )

    def test_flag_and_url_together_add_both_lines_in_order(self):
        """collect_dump_on_failure=True + URL: both lines appear, in order, after DPU_ID."""
        from mellanox_bfb_installer.main import _generate_additional_config_lines

        result = _generate_additional_config_lines(
            self._make_target(2),
            collect_dump_on_failure=True,
            dump_upload_base_url="http://192.168.100.254:8090",
        )
        match = re.fullmatch(
            r"NPU_TIME=(\d+)\nDPU_ID=(\d+)\n"
            r"COLLECT_DUMP_ON_FAILURE=true\n"
            r"DUMP_UPLOAD_BASE_URL=http://192\.168\.100\.254:8090\n",
            result,
        )
        self.assertIsNotNone(
            match, f"Expected NPU_TIME/DPU_ID/COLLECT_DUMP/URL block but got {result!r}"
        )
        self.assertEqual(int(match.group(2)), 2)

    def test_all_flags_combined_block(self):
        """All flags together: NPU_TIME, DPU_ID, DEBUG_SHELL, COLLECT_DUMP, URL in that exact order."""
        from mellanox_bfb_installer.main import _generate_additional_config_lines

        result = _generate_additional_config_lines(
            self._make_target(5),
            debug_shell=True,
            collect_dump_on_failure=True,
            dump_upload_base_url="http://10.0.0.1:8090",
        )
        match = re.fullmatch(
            r"NPU_TIME=(\d+)\nDPU_ID=5\nDEBUG_SHELL=true\n"
            r"COLLECT_DUMP_ON_FAILURE=true\n"
            r"DUMP_UPLOAD_BASE_URL=http://10\.0\.0\.1:8090\n",
            result,
        )
        self.assertIsNotNone(
            match,
            f"Expected NPU_TIME/DPU_ID/DEBUG_SHELL/COLLECT_DUMP/URL block but got {result!r}",
        )


class TestAddAdditionalConfigLinesDumpReceiver(unittest.TestCase):
    """Tests that _add_additional_config_lines threads the new args into per-target configs."""

    def _make_target(self, idx: int, config_path):
        from mellanox_bfb_installer.device_selection import TargetInfo

        return TargetInfo(
            dpu=f"dpu{idx}",
            rshim=f"rshim{idx}",
            dpu_pci_bus_id=f"0000:0{idx}:00.0",
            rshim_pci_bus_id=f"0000:0{idx}:00.1",
            config_path=config_path,
        )

    def test_collect_dump_propagates_to_each_target_temp_config(self):
        """collect_dump_on_failure=True+URL: both lines appear in every target's temp config,
        each with its own DPU_ID."""
        from mellanox_bfb_installer import main

        with tempfile.TemporaryDirectory(prefix="add_config_dr_") as tempdir:
            targets = [
                self._make_target(0, None),
                self._make_target(1, None),
            ]
            main._add_additional_config_lines(
                targets,
                tempdir,
                collect_dump_on_failure=True,
                dump_upload_base_url="http://192.168.100.254:8090",
            )
            for tgt, expected_dpu_id in [(targets[0], 0), (targets[1], 1)]:
                with open(tgt.config_path, "r") as f:
                    content = f.read()
                self.assertIn(f"DPU_ID={expected_dpu_id}\n", content)
                self.assertIn("COLLECT_DUMP_ON_FAILURE=true\n", content)
                self.assertIn(
                    "DUMP_UPLOAD_BASE_URL=http://192.168.100.254:8090\n", content
                )

    def test_collect_dump_default_does_not_add_lines(self):
        """Default kwargs (no collect_dump_on_failure) must not emit the new lines."""
        from mellanox_bfb_installer import main

        with tempfile.TemporaryDirectory(prefix="add_config_dr_default_") as tempdir:
            targets = [self._make_target(0, None)]
            main._add_additional_config_lines(targets, tempdir)
            with open(targets[0].config_path, "r") as f:
                content = f.read()
            self.assertNotIn("COLLECT_DUMP_ON_FAILURE", content)
            self.assertNotIn("DUMP_UPLOAD_BASE_URL", content)


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

    Heavy mocking: device_selection.get_targets, install_executor.run_parallel,
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
                config_path=None,
            )
            for i in range(count)
        ]

    @contextmanager
    def _common_mocks(self, targets, run_parallel_failed=0, setup_side_effect=None):
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
                main, "_add_additional_config_lines", return_value=None
            ) as mock_add_cfg,
            mock.patch.object(main.tmfifo_bridge, "TmfifoBridge", bridge_cls),
            mock.patch.object(
                main, "_dump_receiver_subprocess", side_effect=fake_dump_receiver
            ) as mock_recv,
            mock.patch.object(
                main.install_executor, "run_parallel", return_value=run_parallel_failed
            ) as mock_run_parallel,
        ):
            yield {
                "bridge_cls": bridge_cls,
                "bridge_inst": bridge_inst,
                "add_cfg": mock_add_cfg,
                "receiver": mock_recv,
                "run_parallel": mock_run_parallel,
            }

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
                configs=None,
                temp_work_dir="/tmp/work",
                debug_shell=False,
                collect_dump_on_failure=True,
                dump_output_dir="/tmp/dumps",
            )
            m["bridge_inst"].setup.assert_called_once()
            m["bridge_inst"].teardown.assert_called_once()
            m["receiver"].assert_called_once()
            m["run_parallel"].assert_called_once()
            # Per-target bf.cfg got the new args propagated.
            add_kwargs = m["add_cfg"].call_args[1]
            self.assertTrue(add_kwargs["collect_dump_on_failure"])
            self.assertTrue(add_kwargs["dump_upload_base_url"].startswith("http://"))

    def test_bridge_teardown_runs_even_when_install_fails(self):
        """run_parallel returns non-zero -> sys.exit(1), but bridge.teardown MUST still run."""
        from mellanox_bfb_installer import main

        targets = self._make_targets(2)
        with self._common_mocks(targets, run_parallel_failed=1) as m:
            with self.assertRaises(SystemExit) as cm:
                main._install_on_dpus(
                    bfb_path="/tmp/x.bfb",
                    work_dir="/tmp/work",
                    rshims=None,
                    dpus="all",
                    verbose=False,
                    configs=None,
                    temp_work_dir="/tmp/work",
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
                configs=None,
                temp_work_dir="/tmp/work",
                debug_shell=False,
                collect_dump_on_failure=False,
                dump_output_dir="/tmp/dumps",
            )
            m["bridge_cls"].assert_not_called()
            m["bridge_inst"].setup.assert_not_called()
            m["bridge_inst"].teardown.assert_not_called()
            m["receiver"].assert_not_called()
            m["run_parallel"].assert_called_once()
            # And the new bf.cfg fields are NOT propagated.
            add_kwargs = m["add_cfg"].call_args[1]
            self.assertFalse(add_kwargs["collect_dump_on_failure"])
            self.assertIsNone(add_kwargs["dump_upload_base_url"])

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
                configs=None,
                temp_work_dir="/tmp/work",
                debug_shell=False,
                collect_dump_on_failure=True,
                dump_output_dir="/tmp/dumps",
            )
            m["bridge_inst"].setup.assert_called_once()
            m["bridge_inst"].teardown.assert_not_called()
            m["receiver"].assert_not_called()
            m["run_parallel"].assert_called_once()


if __name__ == "__main__":
    unittest.main()
