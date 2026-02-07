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
Unit tests for platform detection and ASIC detection functions
"""

from mellanox_fw_manager.platform_utils import (
    _detect_platform,
    _detect_platform_from_asic_conf, _is_multi_asic,
    run_command
)
import os
import sys
import unittest
from unittest.mock import patch, MagicMock, mock_open
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))


class TestPlatformDetection(unittest.TestCase):
    """Test cases for platform detection functions"""

    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()

    def tearDown(self):
        """Clean up test fixtures"""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    @patch('mellanox_fw_manager.platform_utils.os.path.exists')
    @patch('mellanox_fw_manager.platform_utils.logging')
    def test_detect_platform_success(self, mock_logging, mock_exists):
        """Test _detect_platform when platform file exists and contains onie_platform"""
        mock_exists.return_value = True

        mock_file_content = """# SONiC platform configuration
onie_platform=x86_64-nvidia_sn4280-r0
onie_machine=x86_64-nvidia_sn4280-r0
"""

        with patch('builtins.open', mock_open(read_data=mock_file_content)):
            result = _detect_platform()

            self.assertEqual(result, "x86_64-nvidia_sn4280-r0")
            mock_logging.info.assert_called_with("Detected platform: x86_64-nvidia_sn4280-r0")

    @patch('mellanox_fw_manager.platform_utils.os.path.exists')
    @patch('mellanox_fw_manager.platform_utils.logging')
    def test_detect_platform_file_not_exists(self, mock_logging, mock_exists):
        """Test _detect_platform when platform file doesn't exist"""
        mock_exists.return_value = False

        result = _detect_platform()

        self.assertIsNone(result)
        mock_logging.error.assert_called_with("Platform configuration file not found: /host/machine.conf")

    @patch('mellanox_fw_manager.platform_utils.os.path.exists')
    @patch('mellanox_fw_manager.platform_utils.logging')
    def test_detect_platform_no_onie_platform(self, mock_logging, mock_exists):
        """Test _detect_platform when file exists but no onie_platform variable"""
        mock_exists.return_value = True

        mock_file_content = """# SONiC platform configuration
onie_machine=x86_64-nvidia_sn4280-r0
other_variable=value
"""

        with patch('builtins.open', mock_open(read_data=mock_file_content)):
            result = _detect_platform()

            self.assertIsNone(result)
            mock_logging.error.assert_called_with("Could not find onie_platform variable in /host/machine.conf")

    @patch('mellanox_fw_manager.platform_utils.os.path.exists')
    @patch('mellanox_fw_manager.platform_utils.logging')
    def test_detect_platform_file_read_error(self, mock_logging, mock_exists):
        """Test _detect_platform when file read fails"""
        mock_exists.return_value = True

        with patch('builtins.open', side_effect=IOError("Permission denied")):
            result = _detect_platform()

            self.assertIsNone(result)
            mock_logging.error.assert_called_with("Platform detection failed: Permission denied")


class TestRunCommand(unittest.TestCase):
    """Test cases for run_command utility function"""

    @patch('mellanox_fw_manager.platform_utils.subprocess.run')
    @patch('mellanox_fw_manager.platform_utils.logging.getLogger')
    def test_run_command_with_no_logger(self, mock_get_logger, mock_run):
        """Test run_command when no logger is provided (uses root logger)"""
        mock_logger = MagicMock()
        mock_get_logger.return_value = mock_logger
        mock_run.return_value = MagicMock(returncode=0, stdout="output")

        cmd = ['echo', 'test']
        result = run_command(cmd)

        mock_get_logger.assert_called_once_with()
        mock_logger.info.assert_called_once_with("Executing: echo test")
        mock_run.assert_called_once_with(cmd)
        self.assertEqual(result.returncode, 0)

    @patch('mellanox_fw_manager.platform_utils.subprocess.run')
    def test_run_command_with_logger(self, mock_run):
        """Test run_command when logger is provided"""
        mock_logger = MagicMock()
        mock_run.return_value = MagicMock(returncode=0, stdout="output")

        cmd = ['echo', 'test']
        result = run_command(cmd, logger=mock_logger)

        mock_logger.info.assert_called_once_with("Executing: echo test")
        mock_run.assert_called_once_with(cmd)
        self.assertEqual(result.returncode, 0)

    @patch('mellanox_fw_manager.platform_utils.subprocess.run')
    def test_run_command_with_kwargs(self, mock_run):
        """Test run_command passes through kwargs to subprocess.run"""
        mock_logger = MagicMock()
        mock_run.return_value = MagicMock(returncode=0, stdout="output", stderr="")

        cmd = ['ls', '-la']
        result = run_command(cmd, logger=mock_logger, capture_output=True, text=True, check=True)

        mock_logger.info.assert_called_once_with("Executing: ls -la")
        mock_run.assert_called_once_with(cmd, capture_output=True, text=True, check=True)


class TestDetectPlatformFromAsicConf(unittest.TestCase):
    """Test cases for _detect_platform_from_asic_conf function"""

    @patch('mellanox_fw_manager.platform_utils.os.path.exists')
    @patch('mellanox_fw_manager.platform_utils.logging')
    def test_detect_platform_from_asic_conf_success(self, mock_logging, mock_exists):
        """Test _detect_platform_from_asic_conf when file exists"""
        mock_exists.return_value = True
        platform = "x86_64-nvidia_sn4280-r0"

        result = _detect_platform_from_asic_conf(platform)

        expected_path = f"/usr/share/sonic/device/{platform}/asic.conf"
        self.assertEqual(result, expected_path)
        mock_exists.assert_called_once_with(expected_path)

    @patch('mellanox_fw_manager.platform_utils.os.path.exists')
    @patch('mellanox_fw_manager.platform_utils.logging')
    def test_detect_platform_from_asic_conf_file_not_exists(self, mock_logging, mock_exists):
        """Test _detect_platform_from_asic_conf when file doesn't exist"""
        mock_exists.return_value = False
        platform = "x86_64-unknown-platform"

        result = _detect_platform_from_asic_conf(platform)

        self.assertIsNone(result)
        expected_path = f"/usr/share/sonic/device/{platform}/asic.conf"
        mock_logging.error.assert_called_once_with(f"ASIC configuration file not found: {expected_path}")

    @patch('mellanox_fw_manager.platform_utils.logging')
    @patch('mellanox_fw_manager.platform_utils.os.path.exists')
    def test_detect_platform_from_asic_conf_exception(self, mock_exists, mock_logging):
        """Test _detect_platform_from_asic_conf when an exception occurs"""
        mock_exists.side_effect = Exception("Unexpected error")
        platform = "x86_64-test-platform"

        result = _detect_platform_from_asic_conf(platform)

        self.assertIsNone(result)
        mock_logging.error.assert_called_once_with("Failed to detect platform from asic.conf: Unexpected error")


class TestIsMultiAsic(unittest.TestCase):
    """Test cases for _is_multi_asic function"""

    def test_is_multi_asic_single_asic(self):
        """Test _is_multi_asic with single ASIC configuration"""
        mock_file_content = """# ASIC configuration
NUM_ASIC=1
DEV_ID_ASIC_0=0x1234
"""
        with patch('builtins.open', mock_open(read_data=mock_file_content)):
            result = _is_multi_asic("/path/to/asic.conf")
            self.assertFalse(result)

    def test_is_multi_asic_multi_asic(self):
        """Test _is_multi_asic with multi-ASIC configuration"""
        mock_file_content = """# ASIC configuration
NUM_ASIC=2
DEV_ID_ASIC_0=0x1234
DEV_ID_ASIC_1=0x5678
"""
        with patch('builtins.open', mock_open(read_data=mock_file_content)):
            result = _is_multi_asic("/path/to/asic.conf")
            self.assertTrue(result)

    def test_is_multi_asic_no_num_asic_line(self):
        """Test _is_multi_asic when NUM_ASIC line is missing"""
        mock_file_content = """# ASIC configuration
DEV_ID_ASIC_0=0x1234
OTHER_CONFIG=value
"""
        with patch('builtins.open', mock_open(read_data=mock_file_content)):
            result = _is_multi_asic("/path/to/asic.conf")
            self.assertFalse(result)

    @patch('mellanox_fw_manager.platform_utils.logging')
    def test_is_multi_asic_file_read_error(self, mock_logging):
        """Test _is_multi_asic when file read fails"""
        with patch('builtins.open', side_effect=IOError("Permission denied")):
            result = _is_multi_asic("/path/to/asic.conf")

            self.assertFalse(result)
            mock_logging.error.assert_called_once_with("Multi-ASIC detection failed: Permission denied")

    @patch('mellanox_fw_manager.platform_utils.logging')
    def test_is_multi_asic_invalid_num_asic_value(self, mock_logging):
        """Test _is_multi_asic with invalid NUM_ASIC value"""
        mock_file_content = """# ASIC configuration
NUM_ASIC=invalid
DEV_ID_ASIC_0=0x1234
"""
        with patch('builtins.open', mock_open(read_data=mock_file_content)):
            result = _is_multi_asic("/path/to/asic.conf")

            self.assertFalse(result)
            self.assertTrue(mock_logging.error.called)


if __name__ == '__main__':
    unittest.main()
