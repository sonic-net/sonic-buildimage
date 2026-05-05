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
Unit tests for BluefieldFirmwareManager class
"""

from mellanox_fw_manager.bluefield_manager import BluefieldFirmwareManager
import os
import sys
import unittest
from unittest.mock import patch, MagicMock, mock_open
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))


class TestBluefieldFirmwareManager(unittest.TestCase):
    """Comprehensive test cases for BluefieldFirmwareManager class"""

    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()

    def tearDown(self):
        """Clean up test fixtures"""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def _create_bluefield_manager(self, **kwargs):
        """Helper to create BluefieldFirmwareManager with default values"""
        defaults = {
            'asic_index': 0, 'pci_id': "08:00.0", 'fw_bin_path': "/test/fw",
            'verbose': False, 'clear_semaphore': False, 'asic_type': "BF3"
        }
        defaults.update(kwargs)
        manager = BluefieldFirmwareManager(**defaults)
        manager.fw_file = "/test/fw/fw-BF3.mfa"
        return manager

    def test_get_mst_device_type(self):
        """Test _get_mst_device_type method"""
        with patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic'):
            manager = self._create_bluefield_manager()
            device_type = manager._get_mst_device_type()
            self.assertEqual(device_type, "BlueField3")

    def test_get_firmware_filename(self):
        """Test get_firmware_filename method"""
        with patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic'):
            # Test valid ASIC type
            manager = self._create_bluefield_manager(asic_type='BF3')
            self.assertEqual(manager.get_firmware_filename(), 'fw-BF3.mfa')

    def test_unsupported_asic_type(self):
        """Test that unsupported ASIC type raises error during initialization"""
        from mellanox_fw_manager.firmware_base import FirmwareManagerError
        with self.assertRaises(FirmwareManagerError) as context:
            BluefieldFirmwareManager(
                asic_index=0, pci_id="08:00.0", fw_bin_path="/test/fw",
                verbose=False, clear_semaphore=False, asic_type='unknown'
            )
        self.assertIn("Unsupported ASIC type", str(context.exception))

    def test_get_asic_type_map(self):
        """Test get_asic_type_map method"""
        result = BluefieldFirmwareManager.get_asic_type_map()
        expected = {'15b3:a2dc': 'BF3'}
        self.assertEqual(result, expected)

    @patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.bluefield_manager.subprocess.run')
    def test_get_available_firmware_version_success(self, mock_run, mock_init_asic):
        """Test _get_available_firmware_version when flint succeeds"""
        mock_init_asic.return_value = None

        mock_run.return_value = MagicMock(
            returncode=0,
            stdout="Image type: BF3\nFW Version: 32.46.0500\nFW Release Date: 10.1.2023"
        )

        manager = self._create_bluefield_manager()
        version = manager._get_available_firmware_version("MT_0000001138")

        self.assertEqual(version, "32.46.0500")
        mock_run.assert_called_once_with(
            ['flint', '-i', '/test/fw/fw-BF3.mfa', '--psid', 'MT_0000001138', 'query'],
            capture_output=True, text=True
        )

    @patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.bluefield_manager.subprocess.run')
    def test_get_available_firmware_version_command_failure(self, mock_run, mock_init_asic):
        """Test _get_available_firmware_version when flint command fails (line 27)"""
        mock_init_asic.return_value = None

        mock_run.return_value = MagicMock(returncode=1, stdout="", stderr="Error")

        manager = self._create_bluefield_manager()
        version = manager._get_available_firmware_version("MT_0000001138")

        self.assertIsNone(version)

    @patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.bluefield_manager.subprocess.run')
    def test_get_available_firmware_version_no_version_found(self, mock_run, mock_init_asic):
        """Test _get_available_firmware_version when no version found in output (line 35)"""
        mock_init_asic.return_value = None

        mock_run.return_value = MagicMock(
            returncode=0,
            stdout="Image type: BF3\nSome other info\nNo version here"
        )

        manager = self._create_bluefield_manager()
        version = manager._get_available_firmware_version("MT_0000001138")

        self.assertIsNone(version)

    @patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.bluefield_manager.subprocess.run')
    def test_get_available_firmware_version_exception(self, mock_run, mock_init_asic):
        """Test _get_available_firmware_version when subprocess raises exception (lines 36-37)"""
        mock_init_asic.return_value = None

        mock_run.side_effect = Exception("Subprocess error")

        manager = self._create_bluefield_manager()
        version = manager._get_available_firmware_version("MT_0000001138")

        self.assertIsNone(version)

    @patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.bluefield_manager.subprocess.run')
    def test_run_firmware_update_success(self, mock_run, mock_init_asic):
        """Test run_firmware_update when all steps succeed"""
        mock_init_asic.return_value = None

        mock_run.side_effect = [
            MagicMock(returncode=0, stderr=""),
            MagicMock(returncode=0, stderr=""),
            MagicMock(returncode=0, stderr="")
        ]

        manager = self._create_bluefield_manager(verbose=True)
        result = manager.run_firmware_update()

        self.assertTrue(result)
        self.assertEqual(mock_run.call_count, 3)

        calls = mock_run.call_args_list
        self.assertIn('flint', calls[0][0][0])
        self.assertIn('ir', calls[0][0][0])
        self.assertIn('flint', calls[1][0][0])
        self.assertIn('burn', calls[1][0][0])
        self.assertIn('mlxconfig', calls[2][0][0])

    @patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.bluefield_manager.subprocess.run')
    def test_run_firmware_update_reactivation_failure(self, mock_run, mock_init_asic):
        """Test run_firmware_update when reactivation fails but continues (line 55)"""
        mock_init_asic.return_value = None

        mock_run.side_effect = [
            MagicMock(returncode=1, stderr="Reactivation failed"),
            MagicMock(returncode=0, stderr=""),
            MagicMock(returncode=0, stderr="")
        ]

        manager = self._create_bluefield_manager()
        result = manager.run_firmware_update()

        self.assertTrue(result)
        self.assertEqual(mock_run.call_count, 3)

    @patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.bluefield_manager.subprocess.run')
    def test_run_firmware_update_burn_failure(self, mock_run, mock_init_asic):
        """Test run_firmware_update when burn command fails (line 62)"""
        mock_init_asic.return_value = None

        mock_run.side_effect = [
            MagicMock(returncode=0, stderr=""),
            MagicMock(returncode=1, stderr="Burn failed"),
        ]

        manager = self._create_bluefield_manager()
        result = manager.run_firmware_update()

        self.assertFalse(result)
        self.assertEqual(mock_run.call_count, 2)

    @patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.bluefield_manager.subprocess.run')
    def test_run_firmware_update_reset_failure(self, mock_run, mock_init_asic):
        """Test run_firmware_update when reset command fails"""
        mock_init_asic.return_value = None

        mock_run.side_effect = [
            MagicMock(returncode=0, stderr=""),
            MagicMock(returncode=0, stderr=""),
            MagicMock(returncode=1, stderr="Reset failed")
        ]

        manager = self._create_bluefield_manager()
        result = manager.run_firmware_update()

        self.assertFalse(result)
        self.assertEqual(mock_run.call_count, 3)

    @patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.bluefield_manager.subprocess.run')
    def test_run_firmware_update_exception(self, mock_run, mock_init_asic):
        """Test run_firmware_update when exception occurs (lines 70-71)"""
        mock_init_asic.return_value = None

        mock_run.side_effect = Exception("Subprocess error")

        manager = self._create_bluefield_manager()
        result = manager.run_firmware_update()

        self.assertFalse(result)

    @patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.bluefield_manager.subprocess.run')
    def test_reset_firmware_config_success(self, mock_run, mock_init_asic):
        """Test reset_firmware_config when command succeeds (lines 75-88)"""
        mock_init_asic.return_value = None

        mock_run.return_value = MagicMock(returncode=0, stdout="Reset successful")

        manager = self._create_bluefield_manager()
        result = manager.reset_firmware_config()

        self.assertTrue(result)
        mock_run.assert_called_once_with(
            ['mlxconfig', '-d', '08:00.0', '-y', 'r'],
            capture_output=True, text=True
        )

    @patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic')
    def test_reset_firmware_config_no_mst_device(self, mock_init_asic):
        """Test reset_firmware_config when no MST device available (lines 77-79)"""
        mock_init_asic.return_value = None

        manager = self._create_bluefield_manager()

        result = manager.reset_firmware_config()

        self.assertFalse(result)

    @patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.bluefield_manager.subprocess.run')
    def test_reset_firmware_config_command_failure(self, mock_run, mock_init_asic):
        """Test reset_firmware_config when mlxconfig command fails"""
        mock_init_asic.return_value = None

        mock_run.return_value = MagicMock(returncode=1, stdout="", stderr="Reset failed")

        manager = self._create_bluefield_manager()
        result = manager.reset_firmware_config()

        self.assertFalse(result)

    @patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.bluefield_manager.subprocess.run')
    def test_reset_firmware_config_exception(self, mock_run, mock_init_asic):
        """Test reset_firmware_config when exception occurs (lines 86-88)"""
        mock_init_asic.return_value = None

        mock_run.side_effect = Exception("Subprocess error")

        manager = self._create_bluefield_manager()
        result = manager.reset_firmware_config()

        self.assertFalse(result)


if __name__ == '__main__':
    unittest.main()
