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
        self.assertIn("-h|--help", USAGE_ARGUMENTS)
        self.assertNotIn("--rshim", USAGE_ARGUMENTS)


class TestGenerateAdditionalConfigLines(unittest.TestCase):
    """Tests for _generate_additional_config_lines."""

    def test_npu_time_is_present_and_correct(self):
        """Returned string contains NPU_TIME=<integer> and is correct."""
        import re
        import time

        from mellanox_bfb_installer.main import _generate_additional_config_lines

        t_before = int(time.time())
        result = _generate_additional_config_lines()
        t_after = int(time.time())

        match = re.fullmatch(r"NPU_TIME=(\d+)\n", result)
        self.assertIsNotNone(match, f"NPU_TIME=<integer>\\n not found in {result!r}")
        npu_time = int(match.group(1))

        self.assertGreaterEqual(npu_time, t_before)
        self.assertLessEqual(npu_time, t_after)


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

    def test_single_target_creates_temp_file_with_original_plus_additional_lines(self):
        """Single target: creates new file with original contents + temp_config_lines."""
        from mellanox_bfb_installer import device_selection, main

        with tempfile.TemporaryDirectory(prefix="add_config_test_") as tempdir:
            config_path = os.path.join(tempdir, "config.json")
            with open(config_path, "w") as f:
                f.write("original_line\n")
            target = self._make_target(0, config_path)
            targets = [target]
            additional = "line 1\nline 2\n"
            main._add_additional_config_lines(targets, additional, tempdir)
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
            self.assertIn("line 1\n", content)
            self.assertIn("line 2\n", content)

    def test_two_targets_same_config_create_one_file_both_updated(self):
        """Two targets with same config: one temp file created, both targets point to it."""
        from mellanox_bfb_installer import device_selection, main

        with tempfile.TemporaryDirectory(prefix="add_config_test_") as tempdir:
            config_path = os.path.join(tempdir, "shared.json")
            with open(config_path, "w") as f:
                f.write("shared_content\n")
            targets = [
                self._make_target(0, config_path),
                self._make_target(1, config_path),
            ]
            additional = "extra\n"
            main._add_additional_config_lines(targets, additional, tempdir)
            self.assertEqual(len(targets), 2)
            self.assertEqual(targets[0].dpu, "dpu0")
            self.assertEqual(targets[0].rshim, "rshim0")
            self.assertEqual(targets[0].dpu_pci_bus_id, "0000:00:00.0")
            self.assertEqual(targets[0].rshim_pci_bus_id, "0000:00:00.1")
            self.assertEqual(targets[1].dpu, "dpu1")
            self.assertEqual(targets[1].rshim, "rshim1")
            self.assertEqual(targets[1].dpu_pci_bus_id, "0000:01:00.0")
            self.assertEqual(targets[1].rshim_pci_bus_id, "0000:01:00.1")

            self.assertTrue(targets[0].config_path.startswith(os.path.join(tempdir, "shared.json.")))
            with open(targets[0].config_path, "r") as f:
                content = f.read()
            self.assertEqual("shared_content\n\nextra\n\n", content)
            self.assertEqual(targets[1].config_path, targets[0].config_path)

    def test_two_targets_different_configs_create_two_files(self):
        """Two targets with different configs: two temp files, each target gets correct path."""
        from mellanox_bfb_installer import device_selection, main

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
            additional = "extra\n"
            main._add_additional_config_lines(targets, additional, tempdir)
            self.assertNotEqual(targets[0].config_path, targets[1].config_path)
            self.assertIn("cfg1.json.", targets[0].config_path)
            self.assertIn("cfg2.json.", targets[1].config_path)
            with open(targets[0].config_path, "r") as f:
                self.assertEqual("config1_content\n\nextra\n\n", f.read())
            with open(targets[1].config_path, "r") as f:
                self.assertEqual("config2_content\n\nextra\n\n", f.read())

    def test_target_with_config_none_creates_empty_config_file(self):
        """Two targets with config_path None: one file created with only additional lines."""
        from mellanox_bfb_installer import device_selection, main

        with tempfile.TemporaryDirectory(prefix="add_config_test_") as tempdir:
            targets = [
                self._make_target(0, None),
                self._make_target(1, None),
            ]
            additional = "extra\n"
            main._add_additional_config_lines(targets, additional, tempdir)
            self.assertEqual(len(targets), 2)
            new_path = targets[0].config_path
            self.assertIsNotNone(new_path)
            self.assertIn("empty-config.", new_path)
            self.assertEqual(targets[1].config_path, new_path, "Both targets share same output file")
            with open(new_path, "r") as f:
                content = f.read()
            self.assertEqual("\nextra\n\n", content)


if __name__ == "__main__":
    unittest.main()
