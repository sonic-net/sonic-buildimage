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
Comprehensive unit tests for main.py CLI module

This file tests both the CLI command functionality and module-level helper functions.
"""

from click.testing import CliRunner
from mellanox_fw_manager.fw_manager import FirmwareManagerError, FirmwareUpgradeError, FirmwareUpgradePartialError
from mellanox_fw_manager.main import (
    setup_logging, _exit_if_qemu, _lock_state_change,
    handle_reset, handle_dry_run, handle_upgrade, main,
    EXIT_SUCCESS, EXIT_FAILURE, FW_UPGRADE_IS_REQUIRED
)
import os
import sys
import unittest
from unittest.mock import patch, MagicMock, mock_open
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))


class TestMainModule(unittest.TestCase):
    """Test cases for main module helper functions"""

    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()

    def tearDown(self):
        """Clean up test fixtures"""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    @patch('mellanox_fw_manager.main.logging.getLogger')
    @patch('mellanox_fw_manager.main.logging.handlers.SysLogHandler')
    @patch('mellanox_fw_manager.main.logging.StreamHandler')
    def test_setup_logging_verbose(self, mock_stream_handler, mock_syslog_handler, mock_get_logger):
        """Test setup_logging with verbose=True"""
        mock_logger = MagicMock()
        mock_get_logger.return_value = mock_logger

        setup_logging(verbose=True)

        self.assertEqual(mock_logger.addHandler.call_count, 2)
        mock_syslog_handler.assert_called_once()
        mock_stream_handler.assert_called_once()

    @patch('mellanox_fw_manager.main.logging.getLogger')
    @patch('mellanox_fw_manager.main.logging.handlers.SysLogHandler')
    def test_setup_logging_not_verbose(self, mock_syslog_handler, mock_get_logger):
        """Test setup_logging with verbose=False"""
        mock_logger = MagicMock()
        mock_get_logger.return_value = mock_logger

        setup_logging(verbose=False)

        mock_logger.addHandler.assert_called_once()
        mock_syslog_handler.assert_called_once()

    @patch('mellanox_fw_manager.main.subprocess.run')
    @patch('mellanox_fw_manager.main.logger')
    def test_exit_if_qemu_not_qemu(self, mock_logger, mock_run):
        """Test _exit_if_qemu when not running on QEMU"""
        mock_run.return_value.stdout = "Some other device"
        mock_run.return_value.returncode = 0

        try:
            _exit_if_qemu()
        except SystemExit:
            self.fail("_exit_if_qemu() raised SystemExit unexpectedly")

    @patch('mellanox_fw_manager.main.subprocess.run')
    @patch('mellanox_fw_manager.main.logger')
    def test_exit_if_qemu_is_qemu(self, mock_logger, mock_run):
        """Test _exit_if_qemu when running on QEMU"""
        mock_run.return_value.stdout = "SimX device detected"
        mock_run.return_value.returncode = 0

        with self.assertRaises(SystemExit) as cm:
            _exit_if_qemu()

        self.assertEqual(cm.exception.code, 0)

    @patch('mellanox_fw_manager.main.subprocess.run')
    @patch('mellanox_fw_manager.main.logger')
    def test_exit_if_qemu_command_fails(self, mock_logger, mock_run):
        """Test _exit_if_qemu when lspci command fails"""
        mock_run.return_value.returncode = 1

        try:
            _exit_if_qemu()
        except SystemExit:
            self.fail("_exit_if_qemu() raised SystemExit when command failed")

    @patch('mellanox_fw_manager.main.subprocess.run')
    @patch('mellanox_fw_manager.main.logger')
    def test_exit_if_qemu_exception_handling(self, mock_logger, mock_run):
        """Test _exit_if_qemu exception handling (line 69)"""
        mock_run.side_effect = Exception("Command failed")

        try:
            _exit_if_qemu()
            mock_logger.warning.assert_called_once()
        except SystemExit:
            self.fail("_exit_if_qemu() raised SystemExit when exception occurred")

    @patch('mellanox_fw_manager.main.logger')
    def test_lock_state_change_context_manager(self, mock_logger):
        """Test _lock_state_change as context manager"""
        with patch('mellanox_fw_manager.main.fcntl.flock') as mock_flock:
            with patch('builtins.open', mock_open()) as mock_file:
                with _lock_state_change():
                    pass

                self.assertEqual(mock_flock.call_count, 2)

    @patch('mellanox_fw_manager.main.logger')
    def test_lock_state_change_exception_handling(self, mock_logger):
        """Test _lock_state_change exception handling"""
        with patch('mellanox_fw_manager.main.fcntl.flock', side_effect=OSError("Permission denied")):
            with patch('builtins.open', mock_open()):
                with self.assertRaises(FirmwareManagerError):
                    with _lock_state_change():
                        pass

    def test_main_function_exists(self, ):
        """Test that main function exists and is callable"""
        self.assertTrue(callable(main))


class TestHandlerFunctions(unittest.TestCase):
    """Test cases for individual handler functions"""

    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()

    def tearDown(self):
        """Clean up test fixtures"""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    @patch('mellanox_fw_manager.main.FirmwareCoordinator')
    def test_handle_reset_success(self, mock_coordinator_class):
        """Test handle_reset successful operation"""
        mock_coordinator = MagicMock()
        mock_coordinator.reset_firmware_config.return_value = None
        mock_coordinator_class.return_value = mock_coordinator

        with patch('builtins.print') as mock_print:
            result = handle_reset(verbose=False)

        self.assertEqual(result, EXIT_SUCCESS)
        mock_coordinator.reset_firmware_config.assert_called_once()
        mock_print.assert_called_with("Firmware configuration reset completed successfully.")

    @patch('mellanox_fw_manager.main.FirmwareCoordinator')
    def test_handle_reset_failure(self, mock_coordinator_class):
        """Test handle_reset failure"""
        mock_coordinator = MagicMock()
        mock_coordinator.reset_firmware_config.side_effect = Exception("Reset failed")
        mock_coordinator_class.return_value = mock_coordinator

        with patch('builtins.print') as mock_print:
            result = handle_reset(verbose=False)

        self.assertEqual(result, EXIT_FAILURE)

    @patch('mellanox_fw_manager.main.FirmwareCoordinator')
    @patch('mellanox_fw_manager.main.logger')
    def test_handle_dry_run_upgrade_required(self, mock_logger, mock_coordinator_class):
        """Test handle_dry_run when upgrade is required"""
        mock_coordinator = MagicMock()
        mock_coordinator.check_upgrade_required.return_value = True
        mock_coordinator_class.return_value = mock_coordinator

        with patch('builtins.print') as mock_print:
            result = handle_dry_run(verbose=False, upgrade=False)

        self.assertEqual(result, FW_UPGRADE_IS_REQUIRED)
        mock_print.assert_called_with("Firmware upgrade is required.")

    @patch('mellanox_fw_manager.main.FirmwareCoordinator')
    @patch('mellanox_fw_manager.main.logger')
    def test_handle_dry_run_up_to_date(self, mock_logger, mock_coordinator_class):
        """Test handle_dry_run when firmware is up to date"""
        mock_coordinator = MagicMock()
        mock_coordinator.check_upgrade_required.return_value = False
        mock_coordinator_class.return_value = mock_coordinator

        with patch('builtins.print') as mock_print:
            result = handle_dry_run(verbose=False, upgrade=False)

        self.assertEqual(result, EXIT_SUCCESS)
        mock_print.assert_called_with("Firmware is up to date.")

    @patch('mellanox_fw_manager.main.FirmwareCoordinator')
    @patch('mellanox_fw_manager.main.logger')
    def test_handle_dry_run_exception(self, mock_logger, mock_coordinator_class):
        """Test handle_dry_run exception handling"""
        mock_coordinator_class.side_effect = Exception("Check failed")

        with patch('builtins.print') as mock_print:
            result = handle_dry_run(verbose=False, upgrade=False)

        self.assertEqual(result, EXIT_FAILURE)

    @patch('mellanox_fw_manager.main.FirmwareCoordinator')
    @patch('mellanox_fw_manager.main.logger')
    def test_handle_upgrade_success(self, mock_logger, mock_coordinator_class):
        """Test handle_upgrade successful operation"""
        mock_coordinator = MagicMock()
        mock_coordinator.check_upgrade_required.return_value = True
        mock_coordinator.upgrade_firmware.return_value = None
        mock_coordinator_class.return_value = mock_coordinator

        with patch('builtins.print') as mock_print:
            result = handle_upgrade(verbose=False, upgrade=False, clear_semaphore=False)

        self.assertEqual(result, EXIT_SUCCESS)
        mock_coordinator.upgrade_firmware.assert_called_once()

    @patch('mellanox_fw_manager.main.FirmwareCoordinator')
    @patch('mellanox_fw_manager.main.logger')
    def test_handle_upgrade_up_to_date(self, mock_logger, mock_coordinator_class):
        """Test handle_upgrade when firmware is already up to date"""
        mock_coordinator = MagicMock()
        mock_coordinator.check_upgrade_required.return_value = False
        mock_coordinator_class.return_value = mock_coordinator

        with patch('builtins.print') as mock_print:
            result = handle_upgrade(verbose=False, upgrade=False, clear_semaphore=False)

        self.assertEqual(result, EXIT_SUCCESS)
        mock_coordinator.upgrade_firmware.assert_not_called()
        mock_print.assert_called_with("Firmware is up to date.")

    @patch('mellanox_fw_manager.main.FirmwareCoordinator')
    @patch('mellanox_fw_manager.main.logger')
    def test_handle_upgrade_failure(self, mock_logger, mock_coordinator_class):
        """Test handle_upgrade with FirmwareUpgradeError"""
        mock_coordinator = MagicMock()
        mock_coordinator.check_upgrade_required.return_value = True
        mock_coordinator.upgrade_firmware.side_effect = FirmwareUpgradeError("Upgrade failed")
        mock_coordinator_class.return_value = mock_coordinator

        with patch('builtins.print') as mock_print:
            result = handle_upgrade(verbose=False, upgrade=False, clear_semaphore=False)

        self.assertEqual(result, EXIT_FAILURE)

    @patch('mellanox_fw_manager.main.FirmwareCoordinator')
    @patch('mellanox_fw_manager.main.logger')
    def test_handle_upgrade_partial_failure(self, mock_logger, mock_coordinator_class):
        """Test handle_upgrade with FirmwareUpgradePartialError"""
        mock_coordinator = MagicMock()
        mock_coordinator.check_upgrade_required.return_value = True
        mock_coordinator.upgrade_firmware.side_effect = FirmwareUpgradePartialError("Partial failure")
        mock_coordinator_class.return_value = mock_coordinator

        with patch('builtins.print') as mock_print:
            result = handle_upgrade(verbose=False, upgrade=False, clear_semaphore=False)

        self.assertEqual(result, EXIT_SUCCESS)

    @patch('mellanox_fw_manager.main.FirmwareCoordinator')
    @patch('mellanox_fw_manager.main.logger')
    def test_handle_upgrade_generic_exception(self, mock_logger, mock_coordinator_class):
        """Test handle_upgrade with generic exception"""
        mock_coordinator = MagicMock()
        mock_coordinator.check_upgrade_required.return_value = True
        mock_coordinator.upgrade_firmware.side_effect = Exception("Unexpected error")
        mock_coordinator_class.return_value = mock_coordinator

        with patch('builtins.print') as mock_print:
            result = handle_upgrade(verbose=False, upgrade=False, clear_semaphore=False)

        self.assertEqual(result, EXIT_FAILURE)


class TestMainCLI(unittest.TestCase):
    """Test main CLI function directly with proper mocking"""

    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()

    def tearDown(self):
        """Clean up test fixtures"""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    @patch('mellanox_fw_manager.main.logger')
    @patch('mellanox_fw_manager.main.setup_logging')
    @patch('mellanox_fw_manager.main._exit_if_qemu')
    @patch('mellanox_fw_manager.main._lock_state_change')
    @patch('mellanox_fw_manager.main.handle_upgrade')
    def test_verbose_mode_flag(self, mock_handle_upgrade, mock_lock, mock_exit_qemu, mock_setup_logging, mock_logger):
        """Test that verbose flag is passed correctly"""
        mock_lock.return_value.__enter__ = MagicMock()
        mock_lock.return_value.__exit__ = MagicMock()
        mock_handle_upgrade.return_value = EXIT_SUCCESS

        runner = CliRunner()

        result = runner.invoke(main, ['--verbose'])

        mock_setup_logging.assert_called_once_with(True, False)

    @patch('mellanox_fw_manager.main.logger')
    @patch('mellanox_fw_manager.main.setup_logging')
    @patch('mellanox_fw_manager.main._exit_if_qemu')
    @patch('mellanox_fw_manager.main._lock_state_change')
    @patch('mellanox_fw_manager.main.handle_upgrade')
    def test_nosyslog_flag(self, mock_handle_upgrade, mock_lock, mock_exit_qemu, mock_setup_logging, mock_logger):
        """Test that nosyslog flag is passed correctly"""
        mock_lock.return_value.__enter__ = MagicMock()
        mock_lock.return_value.__exit__ = MagicMock()
        mock_handle_upgrade.return_value = EXIT_SUCCESS

        runner = CliRunner()

        result = runner.invoke(main, ['--nosyslog'])

        mock_setup_logging.assert_called_once_with(False, True)

    @patch('mellanox_fw_manager.main.logger')
    @patch('mellanox_fw_manager.main.setup_logging')
    @patch('mellanox_fw_manager.main._exit_if_qemu')
    @patch('mellanox_fw_manager.main._lock_state_change')
    @patch('mellanox_fw_manager.main.handle_reset')
    def test_reset_operation_cli(self, mock_handle_reset, mock_lock, mock_exit_qemu, mock_setup_logging, mock_logger):
        """Test main CLI with --reset"""
        mock_lock.return_value.__enter__ = MagicMock()
        mock_lock.return_value.__exit__ = MagicMock()
        mock_handle_reset.return_value = EXIT_SUCCESS

        runner = CliRunner()
        result = runner.invoke(main, ['--reset'])

        mock_handle_reset.assert_called_once_with(False)

    @patch('mellanox_fw_manager.main.logger')
    @patch('mellanox_fw_manager.main.setup_logging')
    @patch('mellanox_fw_manager.main._exit_if_qemu')
    @patch('mellanox_fw_manager.main._lock_state_change')
    @patch('mellanox_fw_manager.main.handle_dry_run')
    def test_dry_run_operation_cli(self, mock_handle_dry_run, mock_lock, mock_exit_qemu, mock_setup_logging, mock_logger):
        """Test main CLI with --dry-run"""
        mock_lock.return_value.__enter__ = MagicMock()
        mock_lock.return_value.__exit__ = MagicMock()
        mock_handle_dry_run.return_value = EXIT_SUCCESS

        runner = CliRunner()
        result = runner.invoke(main, ['--dry-run'])

        mock_handle_dry_run.assert_called_once_with(False, False)

    @patch('mellanox_fw_manager.main.logger')
    @patch('mellanox_fw_manager.main.setup_logging')
    @patch('mellanox_fw_manager.main._exit_if_qemu')
    @patch('mellanox_fw_manager.main._lock_state_change')
    @patch('mellanox_fw_manager.main.handle_upgrade')
    def test_upgrade_with_flags_cli(self, mock_handle_upgrade, mock_lock, mock_exit_qemu, mock_setup_logging, mock_logger):
        """Test main CLI with --upgrade and --clear-semaphore"""
        mock_lock.return_value.__enter__ = MagicMock()
        mock_lock.return_value.__exit__ = MagicMock()
        mock_handle_upgrade.return_value = EXIT_SUCCESS

        runner = CliRunner()

        result = runner.invoke(main, ['--upgrade', '--clear-semaphore'])

        mock_handle_upgrade.assert_called_once_with(False, True, True)


def mock_open():
    """Helper function to create mock file object"""
    mock_file = MagicMock()
    mock_file.__enter__ = MagicMock(return_value=mock_file)
    mock_file.__exit__ = MagicMock(return_value=None)
    return mock_file


if __name__ == '__main__':
    unittest.main()
