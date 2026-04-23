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
Comprehensive unit tests for SpectrumFirmwareManager
"""

from mellanox_fw_manager.firmware_base import FW_ALREADY_UPDATED_FAILURE
from mellanox_fw_manager.spectrum_manager import SpectrumFirmwareManager
import os
import sys
import unittest
from unittest.mock import patch, MagicMock
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))


class TestSpectrumFirmwareManager(unittest.TestCase):
    """Test cases for SpectrumFirmwareManager methods"""

    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()

    def tearDown(self):
        """Clean up test fixtures"""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def _create_manager(self, **kwargs):
        """Helper to create SpectrumFirmwareManager with mocked initialization"""
        defaults = {
            'asic_index': 0,
            'pci_id': "01:00.0",
            'fw_bin_path': self.temp_dir,
            'verbose': False,
            'clear_semaphore': False,
            'asic_type': 'SPC3',
            'status_queue': None
        }
        defaults.update(kwargs)

        with patch.object(SpectrumFirmwareManager, '_initialize_asic'):
            manager = SpectrumFirmwareManager(**defaults)
            manager.fw_file = f"{self.temp_dir}/fw-SPC3.mfa"
            manager.current_version = "30.2016.1036"
            manager.available_version = "30.2016.1040"
            return manager

    def test_get_mst_device_type(self):
        """Test _get_mst_device_type returns correct device type"""
        manager = self._create_manager()
        result = manager._get_mst_device_type()
        self.assertEqual(result, "Spectrum")

    def test_get_firmware_filename(self):
        """Test get_firmware_filename returns correct Spectrum firmware filename"""
        # Test SPC3
        manager = self._create_manager(asic_type='SPC3')
        self.assertEqual(manager.get_firmware_filename(), 'fw-SPC3.mfa')

        # Test SPC
        manager = self._create_manager(asic_type='SPC')
        self.assertEqual(manager.get_firmware_filename(), 'fw-SPC.mfa')

        # Test SPC5
        manager = self._create_manager(asic_type='SPC5')
        self.assertEqual(manager.get_firmware_filename(), 'fw-SPC5.mfa')

    def test_unsupported_asic_type(self):
        """Test that unsupported ASIC type raises error during initialization"""
        from mellanox_fw_manager.firmware_base import FirmwareManagerError
        with self.assertRaises(FirmwareManagerError) as context:
            SpectrumFirmwareManager(
                asic_index=0, pci_id="01:00.0", fw_bin_path=self.temp_dir,
                verbose=False, clear_semaphore=False, asic_type='unknown'
            )
        self.assertIn("Unsupported ASIC type", str(context.exception))

    def test_get_asic_type_map(self):
        """Test get_asic_type_map returns correct mapping"""
        result = SpectrumFirmwareManager.get_asic_type_map()

        expected = {
            '15b3:cb84': 'SPC',
            '15b3:cf6c': 'SPC2',
            '15b3:cf70': 'SPC3',
            '15b3:cf80': 'SPC4',
            '15b3:cf82': 'SPC5',
        }

        self.assertEqual(result, expected)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_get_available_firmware_version_success(self, mock_run_cmd):
        """Test _get_available_firmware_version when successful"""
        manager = self._create_manager()
        psid = "MT_0000001187"

        mock_output = f"""
Device Info:
{psid} field1 field2 30.2016.1040 field4
MT_0000001188 field1 field2 30.2016.1050 field4
"""
        mock_run_cmd.return_value = MagicMock(returncode=0, stdout=mock_output)

        result = manager._get_available_firmware_version(psid)

        self.assertEqual(result, "30.2016.1040")
        mock_run_cmd.assert_called_once()
        call_args = mock_run_cmd.call_args[0][0]
        self.assertIn('mlxfwmanager', call_args)
        self.assertIn('--list-content', call_args)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_get_available_firmware_version_command_failure(self, mock_run_cmd):
        """Test _get_available_firmware_version when mlxfwmanager command fails"""
        manager = self._create_manager()
        psid = "MT_0000001187"

        mock_run_cmd.return_value = MagicMock(
            returncode=1,
            stdout="",
            stderr="Error: Device not found"
        )

        result = manager._get_available_firmware_version(psid)

        self.assertIsNone(result)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_get_available_firmware_version_psid_not_found(self, mock_run_cmd):
        """Test _get_available_firmware_version when PSID not found in output"""
        manager = self._create_manager()
        psid = "MT_0000001187"

        mock_output = """
Device Info:
MT_0000001188 field1 field2 30.2016.1050 field4
MT_0000001189 field1 field2 30.2016.1060 field4
"""
        mock_run_cmd.return_value = MagicMock(returncode=0, stdout=mock_output)

        result = manager._get_available_firmware_version(psid)

        self.assertIsNone(result)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_get_available_firmware_version_insufficient_fields(self, mock_run_cmd):
        """Test _get_available_firmware_version when line has insufficient fields"""
        manager = self._create_manager()
        psid = "MT_0000001187"

        mock_output = f"""
Device Info:
{psid} field1 field2
"""
        mock_run_cmd.return_value = MagicMock(returncode=0, stdout=mock_output)

        result = manager._get_available_firmware_version(psid)

        self.assertIsNone(result)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_get_available_firmware_version_exception(self, mock_run_cmd):
        """Test _get_available_firmware_version when exception occurs"""
        manager = self._create_manager()
        psid = "MT_0000001187"

        mock_run_cmd.side_effect = Exception("Subprocess execution failed")

        result = manager._get_available_firmware_version(psid)

        self.assertIsNone(result)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_run_firmware_update_success(self, mock_run_cmd):
        """Test run_firmware_update when successful"""
        manager = self._create_manager()

        mock_run_cmd.return_value = MagicMock(
            returncode=0,
            stdout="Firmware update completed successfully",
            stderr=""
        )

        result = manager.run_firmware_update()

        self.assertTrue(result)
        mock_run_cmd.assert_called_once()
        call_args = mock_run_cmd.call_args[0][0]
        self.assertIn('mlxfwmanager', call_args)
        self.assertIn('-u', call_args)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_run_firmware_update_failure(self, mock_run_cmd):
        """Test run_firmware_update when firmware update fails"""
        manager = self._create_manager()

        mock_run_cmd.return_value = MagicMock(
            returncode=1,
            stdout="",
            stderr="Error: Failed to update firmware - device busy"
        )

        result = manager.run_firmware_update()

        self.assertFalse(result)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_run_firmware_update_reactivation_required(self, mock_run_cmd):
        """Test run_firmware_update when reactivation is required"""
        manager = self._create_manager()

        mock_run_cmd.side_effect = [
            MagicMock(returncode=FW_ALREADY_UPDATED_FAILURE, stdout="", stderr=""),
            MagicMock(returncode=0, stdout="Reactivation successful", stderr=""),
            MagicMock(returncode=0, stdout="Firmware update completed", stderr="")
        ]

        result = manager.run_firmware_update()

        self.assertTrue(result)
        self.assertEqual(mock_run_cmd.call_count, 3)

        reactivate_call = mock_run_cmd.call_args_list[1][0][0]
        self.assertIn('flint', reactivate_call)
        self.assertIn('ir', reactivate_call)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_run_firmware_update_reactivation_fails_but_continues(self, mock_run_cmd):
        """Test run_firmware_update when reactivation fails but continues with retry"""
        manager = self._create_manager()

        mock_run_cmd.side_effect = [
            MagicMock(returncode=FW_ALREADY_UPDATED_FAILURE, stdout="", stderr=""),
            MagicMock(returncode=1, stdout="", stderr="Reactivation failed"),
            MagicMock(returncode=0, stdout="Firmware update completed", stderr="")
        ]

        result = manager.run_firmware_update()

        self.assertTrue(result)
        self.assertEqual(mock_run_cmd.call_count, 3)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_run_firmware_update_reactivation_required_retry_fails(self, mock_run_cmd):
        """Test run_firmware_update when reactivation required but retry fails"""
        manager = self._create_manager()

        mock_run_cmd.side_effect = [
            MagicMock(returncode=FW_ALREADY_UPDATED_FAILURE, stdout="", stderr=""),
            MagicMock(returncode=0, stdout="Reactivation successful", stderr=""),
            MagicMock(returncode=1, stdout="", stderr="Retry failed")
        ]

        result = manager.run_firmware_update()

        self.assertFalse(result)
        self.assertEqual(mock_run_cmd.call_count, 3)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_run_firmware_update_exception(self, mock_run_cmd):
        """Test run_firmware_update when exception occurs"""
        manager = self._create_manager()

        mock_run_cmd.side_effect = Exception("Subprocess execution failed")

        result = manager.run_firmware_update()

        self.assertFalse(result)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_run_firmware_update_with_verbose_mode(self, mock_run_cmd):
        """Test run_firmware_update passes correct environment in verbose mode"""
        manager = self._create_manager(verbose=True)

        mock_run_cmd.return_value = MagicMock(
            returncode=0,
            stdout="Firmware update completed",
            stderr=""
        )

        result = manager.run_firmware_update()

        self.assertTrue(result)

        call_kwargs = mock_run_cmd.call_args[1]
        self.assertIn('env', call_kwargs)
        env = call_kwargs['env']
        self.assertIn('FLASH_ACCESS_DEBUG', env)
        self.assertIn('FW_COMPS_DEBUG', env)


if __name__ == '__main__':
    unittest.main()
