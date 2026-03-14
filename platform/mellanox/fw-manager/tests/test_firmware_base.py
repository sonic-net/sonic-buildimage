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
Comprehensive unit tests for firmware_base.py
"""

from mellanox_fw_manager.firmware_base import (
    FirmwareManagerBase, FirmwareManagerError, FirmwareUpgradeError, FirmwareUpgradePartialError,
    UpgradeStatus, UpgradeStatusType
)
import os
import sys
import unittest
import tempfile
import xml.etree.ElementTree as ET
from unittest.mock import patch, MagicMock, mock_open
from multiprocessing import Queue

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))


class ConcreteFirmwareManager(FirmwareManagerBase):
    """Concrete implementation of FirmwareManagerBase for testing"""

    @classmethod
    def get_asic_type_map(cls):
        return {'test_pci': 'test'}

    def get_firmware_filename(self):
        firmware_map = {'test': 'test-fw.mfa'}
        return firmware_map.get(self.asic_type)

    def _get_mst_device_type(self):
        return "TestDevice"

    def run_firmware_update(self):
        return True

    def _get_available_firmware_version(self, psid):
        return "2.0.0"


class TestUpgradeStatus(unittest.TestCase):
    """Test cases for UpgradeStatus class"""

    def test_upgrade_status_init(self):
        """Test UpgradeStatus initialization"""
        status = UpgradeStatus(
            asic_index=0,
            status=UpgradeStatusType.SUCCESS,
            message="Test message",
            current_version="1.0.0",
            available_version="2.0.0",
            pci_id="01:00.0"
        )

        self.assertEqual(status.asic_index, 0)
        self.assertEqual(status.status, UpgradeStatusType.SUCCESS)
        self.assertEqual(status.message, "Test message")
        self.assertEqual(status.current_version, "1.0.0")
        self.assertEqual(status.available_version, "2.0.0")
        self.assertEqual(status.pci_id, "01:00.0")
        self.assertIsNone(status.timestamp)

    def test_upgrade_status_to_dict(self):
        """Test UpgradeStatus to_dict method (line 65-73)"""
        status = UpgradeStatus(
            asic_index=1,
            status=UpgradeStatusType.FAILED,
            message="Upgrade failed",
            current_version="1.0.0",
            available_version="2.0.0",
            pci_id="02:00.0"
        )
        status.timestamp = "2023-01-01T00:00:00"

        result = status.to_dict()

        expected = {
            'asic_index': 1,
            'status': 'failed',
            'message': 'Upgrade failed',
            'current_version': '1.0.0',
            'available_version': '2.0.0',
            'pci_id': '02:00.0',
            'timestamp': '2023-01-01T00:00:00'
        }

        self.assertEqual(result, expected)

    def test_upgrade_status_from_dict(self):
        """Test UpgradeStatus from_dict method (lines 78-87)"""
        data = {
            'asic_index': 2,
            'status': 'error',
            'message': 'Error occurred',
            'current_version': '1.5.0',
            'available_version': '2.0.0',
            'pci_id': '03:00.0',
            'timestamp': '2023-01-01T12:00:00'
        }

        status = UpgradeStatus.from_dict(data)

        self.assertEqual(status.asic_index, 2)
        self.assertEqual(status.status, UpgradeStatusType.ERROR)
        self.assertEqual(status.message, "Error occurred")
        self.assertEqual(status.current_version, "1.5.0")
        self.assertEqual(status.available_version, "2.0.0")
        self.assertEqual(status.pci_id, "03:00.0")
        self.assertEqual(status.timestamp, "2023-01-01T12:00:00")

    def test_upgrade_status_from_dict_missing_optional(self):
        """Test UpgradeStatus from_dict with missing optional fields"""
        data = {
            'asic_index': 3,
            'status': 'success',
            'message': 'Success',
            'current_version': '2.0.0',
            'available_version': '2.0.0'
        }

        status = UpgradeStatus.from_dict(data)

        self.assertEqual(status.asic_index, 3)
        self.assertEqual(status.status, UpgradeStatusType.SUCCESS)
        self.assertIsNone(status.pci_id)
        self.assertIsNone(status.timestamp)


class TestFirmwareManagerBase(unittest.TestCase):
    """Test cases for FirmwareManagerBase class"""

    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()

    def tearDown(self):
        """Clean up test fixtures"""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_firmware_manager_base_init_success(self):
        """Test successful FirmwareManagerBase initialization (lines 108-127)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic') as mock_init:
            mock_init.return_value = None

            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                fw_bin_path="/test/fw",
                verbose=True,
                clear_semaphore=True,
                asic_type="test"
            )

            self.assertEqual(manager.asic_index, 0)
            self.assertEqual(manager.pci_id, "01:00.0")
            self.assertEqual(manager.fw_bin_path, "/test/fw")
            self.assertTrue(manager.verbose)
            self.assertTrue(manager.should_clear_semaphore)
            self.assertEqual(manager.asic_type, "test")
            self.assertIsNone(manager.fw_file)
            self.assertIsNone(manager.current_version)
            self.assertIsNone(manager.available_version)
            self.assertIsNotNone(manager.logger)
            mock_init.assert_called_once()

    def test_initialize_asic_no_asic_type(self):
        """Test _initialize_asic when no ASIC type provided (lines 133-134)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type=None
            )

        with self.assertRaises(FirmwareManagerError) as context:
            FirmwareManagerBase._initialize_asic(manager)

        self.assertIn("ASIC type not provided", str(context.exception))

    def test_initialize_asic_firmware_file_not_found(self):
        """Test _initialize_asic when firmware file not found (lines 138-139)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        with patch.object(manager, '_get_firmware_file_path', return_value="/nonexistent/file.mfa"):
            with patch('mellanox_fw_manager.firmware_base.os.path.exists', return_value=False):
                with self.assertRaises(FirmwareManagerError) as context:
                    manager._initialize_asic()

                self.assertIn("Firmware file not found", str(context.exception))

    def test_initialize_asic_no_firmware_versions(self):
        """Test _initialize_asic when firmware versions not found (lines 148-149)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        with patch.object(manager, '_get_firmware_file_path', return_value="/test/fw.mfa"):
            with patch('mellanox_fw_manager.firmware_base.os.path.exists', return_value=True):
                with patch.object(manager, '_get_firmware_versions', return_value=(None, None)):
                    with self.assertRaises(FirmwareManagerError) as context:
                        manager._initialize_asic()

                    self.assertIn("Could not retrieve firmware versions", str(context.exception))

    def test_initialize_asic_success_with_logging(self):
        """Test successful _initialize_asic with logging (lines 151-152)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        with patch.object(manager, '_get_firmware_file_path', return_value="/test/fw.mfa"):
            with patch('mellanox_fw_manager.firmware_base.os.path.exists', return_value=True):
                with patch.object(manager, '_get_firmware_versions', return_value=("1.0.0", "2.0.0")):
                    manager._initialize_asic()

                    self.assertEqual(manager.current_version, "1.0.0")
                    self.assertEqual(manager.available_version, "2.0.0")

    def test_initialize_asic_exception_handling(self):
        """Test _initialize_asic exception handling (lines 154-156)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        with patch.object(manager, '_get_firmware_file_path', side_effect=Exception("Test error")):
            with self.assertRaises(Exception):
                manager._initialize_asic()

    def test_run_method_up_to_date(self):
        """Test run method when firmware is up to date (lines 162-164)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        with patch.object(manager, 'is_upgrade_required', return_value=False):
            with patch.object(manager, '_report_status') as mock_report:
                manager.run()

                mock_report.assert_called_once_with(
                    UpgradeStatusType.SUCCESS,
                    f"ASIC {manager.asic_index} firmware is up to date"
                )

    def test_run_method_with_semaphore_clear_success(self):
        """Test run method with semaphore clearing (lines 167-170)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test",
                clear_semaphore=True
            )

        with patch.object(manager, 'is_upgrade_required', return_value=True):
            with patch.object(manager, 'clear_semaphore', return_value=True):
                with patch.object(manager, 'run_firmware_update', return_value=True):
                    with patch.object(manager, '_report_status') as mock_report:
                        manager.run()

                        mock_report.assert_called_once_with(
                            UpgradeStatusType.SUCCESS,
                            f"ASIC {manager.asic_index} upgrade completed"
                        )

    def test_run_method_with_semaphore_clear_failure(self):
        """Test run method when semaphore clearing fails but continues (lines 169-170)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test",
                clear_semaphore=True
            )

        with patch.object(manager, 'is_upgrade_required', return_value=True):
            with patch.object(manager, 'clear_semaphore', return_value=False):
                with patch.object(manager, 'run_firmware_update', return_value=True):
                    with patch.object(manager, '_report_status') as mock_report:
                        manager.run()

                        mock_report.assert_called_once_with(
                            UpgradeStatusType.SUCCESS,
                            f"ASIC {manager.asic_index} upgrade completed"
                        )

    def test_run_method_upgrade_success(self):
        """Test run method with successful upgrade (lines 173-174)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        with patch.object(manager, 'is_upgrade_required', return_value=True):
            with patch.object(manager, 'run_firmware_update', return_value=True):
                with patch.object(manager, '_report_status') as mock_report:
                    manager.run()

                    mock_report.assert_called_once_with(
                        UpgradeStatusType.SUCCESS,
                        f"ASIC {manager.asic_index} upgrade completed"
                    )

    def test_run_method_upgrade_failure(self):
        """Test run method with failed upgrade (lines 175-176)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        with patch.object(manager, 'is_upgrade_required', return_value=True):
            with patch.object(manager, 'run_firmware_update', return_value=False):
                with patch.object(manager, '_report_status') as mock_report:
                    manager.run()

                    mock_report.assert_called_once_with(
                        UpgradeStatusType.FAILED,
                        f"ASIC {manager.asic_index} upgrade failed"
                    )

    def test_run_method_exception_handling(self):
        """Test run method exception handling (lines 178-179)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        with patch.object(manager, 'is_upgrade_required', side_effect=Exception("Test error")):
            with patch.object(manager, '_report_status') as mock_report:
                manager.run()

                mock_report.assert_called_once_with(
                    UpgradeStatusType.ERROR,
                    "Unexpected error: Test error"
                )

    def test_is_upgrade_required_true(self):
        """Test is_upgrade_required when upgrade is needed (line 189)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        manager.current_version = "1.0.0"
        manager.available_version = "2.0.0"

        result = manager.is_upgrade_required()
        self.assertTrue(result)

    def test_is_upgrade_required_false(self):
        """Test is_upgrade_required when no upgrade needed (line 189)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        manager.current_version = "2.0.0"
        manager.available_version = "2.0.0"

        result = manager.is_upgrade_required()
        self.assertFalse(result)

    def test_report_status_with_queue(self):
        """Test _report_status with status queue (lines 193-202)"""
        mock_queue = MagicMock()

        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test",
                status_queue=mock_queue
            )

        manager.current_version = "1.0.0"
        manager.available_version = "2.0.0"

        manager._report_status(UpgradeStatusType.SUCCESS, "Test message")

        mock_queue.put.assert_called_once()
        call_args = mock_queue.put.call_args[0][0]

        self.assertEqual(call_args['asic_index'], 0)
        self.assertEqual(call_args['status'], 'success')
        self.assertEqual(call_args['message'], 'Test message')
        self.assertEqual(call_args['current_version'], '1.0.0')
        self.assertEqual(call_args['available_version'], '2.0.0')
        self.assertEqual(call_args['pci_id'], '01:00.0')

    def test_report_status_without_queue(self):
        """Test _report_status without status queue"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        manager._report_status(UpgradeStatusType.SUCCESS, "Test message")

    def test_get_asic_type(self):
        """Test get_asic_type method (line 206)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test_type"
            )

        result = manager.get_asic_type()
        self.assertEqual(result, "test_type")

    def test_get_firmware_file_path_with_fw_bin_path(self):
        """Test _get_firmware_file_path with custom fw_bin_path"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        result = manager._get_firmware_file_path("test", "/custom/path")
        expected = "/custom/path/test-fw.mfa"
        self.assertEqual(result, expected)

    def test_get_firmware_file_path_default_path(self):
        """Test _get_firmware_file_path with default path"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        result = manager._get_firmware_file_path("test", None)
        expected = "/etc/mlnx/test-fw.mfa"
        self.assertEqual(result, expected)

    def test_get_firmware_file_path_unknown_type(self):
        """Test _get_firmware_file_path with unknown ASIC type"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="unknown"
            )

        result = manager._get_firmware_file_path("unknown", "/test/path")
        self.assertIsNone(result)

    @patch('mellanox_fw_manager.firmware_base.time.sleep')
    @patch('mellanox_fw_manager.firmware_base.subprocess.run')
    def test_get_firmware_versions_success(self, mock_run, mock_sleep):
        """Test _get_firmware_versions success path"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        xml_output = '''<?xml version="1.0"?>
        <Devices>
            <Device psid="MT_0000001187">
                <Versions>
                    <FW current="1.0.0"/>
                </Versions>
            </Device>
        </Devices>'''
        mock_run.return_value = MagicMock(returncode=0, stdout=xml_output)

        current, available = manager._get_firmware_versions()

        self.assertEqual(current, "1.0.0")
        self.assertEqual(available, "2.0.0")
        mock_sleep.assert_not_called()

    @patch('mellanox_fw_manager.firmware_base.time.sleep')
    @patch('mellanox_fw_manager.firmware_base.subprocess.run')
    def test_get_firmware_versions_command_failure(self, mock_run, mock_sleep):
        """Test _get_firmware_versions when command fails with retry logic"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        mock_run.return_value = MagicMock(returncode=1)

        with patch.object(manager.logger, 'info') as mock_info:
            with patch.object(manager.logger, 'error') as mock_error:
                current, available = manager._get_firmware_versions()

                self.assertIsNone(current)
                self.assertIsNone(available)
                self.assertEqual(mock_run.call_count, 10)
                self.assertEqual(mock_sleep.call_count, 9)
                # 10 "Executing" logs from run_command + 9 "Unable to query" logs = 19 total
                self.assertEqual(mock_info.call_count, 19)
                mock_error.assert_called_once_with("Failed to get firmware versions after 10 attempts: Query returned non-zero exit code")

    @patch('mellanox_fw_manager.firmware_base.time.sleep')
    @patch('mellanox_fw_manager.firmware_base.subprocess.run')
    def test_get_firmware_versions_xml_parsing_error(self, mock_run, mock_sleep):
        """Test _get_firmware_versions with XML parsing error and retry logic"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        mock_run.return_value = MagicMock(returncode=0, stdout="invalid xml")

        with patch.object(manager.logger, 'info') as mock_info:
            with patch.object(manager.logger, 'error') as mock_error:
                current, available = manager._get_firmware_versions()

                self.assertIsNone(current)
                self.assertIsNone(available)
                self.assertEqual(mock_run.call_count, 10)
                self.assertEqual(mock_sleep.call_count, 9)
                # 10 "Executing" logs from run_command + 9 "Unable to..." logs = 19 total
                self.assertEqual(mock_info.call_count, 19)
                mock_error.assert_called_once()

    @patch('mellanox_fw_manager.firmware_base.time.sleep')
    @patch('mellanox_fw_manager.firmware_base.subprocess.run')
    def test_get_firmware_versions_success_on_retry(self, mock_run, mock_sleep):
        """Test _get_firmware_versions succeeds on third attempt"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        xml_output = '''<?xml version="1.0"?>
        <Devices>
            <Device psid="MT_0000001187">
                <Versions>
                    <FW current="1.0.0"/>
                </Versions>
            </Device>
        </Devices>'''

        mock_run.side_effect = [
            MagicMock(returncode=1),
            MagicMock(returncode=1),
            MagicMock(returncode=0, stdout=xml_output)
        ]

        with patch.object(manager.logger, 'info') as mock_info:
            current, available = manager._get_firmware_versions()

            self.assertEqual(current, "1.0.0")
            self.assertEqual(available, "2.0.0")
            self.assertEqual(mock_run.call_count, 3)
            self.assertEqual(mock_sleep.call_count, 2)
            # 3 "Executing" logs from run_command + 2 "Unable to query" logs = 5 total
            self.assertEqual(mock_info.call_count, 5)

    @patch('mellanox_fw_manager.firmware_base.time.sleep')
    @patch('mellanox_fw_manager.firmware_base.subprocess.run')
    def test_get_firmware_versions_missing_version_field(self, mock_run, mock_sleep):
        """Test _get_firmware_versions when version field is missing"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        xml_output = '''<?xml version="1.0"?>
        <Devices>
            <Device psid="MT_0000001187">
                <Versions>
                    <FW/>
                </Versions>
            </Device>
        </Devices>'''

        mock_run.return_value = MagicMock(returncode=0, stdout=xml_output)

        with patch.object(manager.logger, 'info') as mock_info:
            with patch.object(manager.logger, 'error') as mock_error:
                current, available = manager._get_firmware_versions()

                self.assertIsNone(current)
                self.assertIsNone(available)
                self.assertEqual(mock_run.call_count, 10)
                self.assertEqual(mock_sleep.call_count, 9)
                # 10 "Executing" logs from run_command + 9 "Unable to parse" logs = 19 total
                self.assertEqual(mock_info.call_count, 19)
                mock_error.assert_called_once_with("Failed to get firmware versions after 10 attempts: Version or PSID not found in response")

    @patch('mellanox_fw_manager.firmware_base.time.sleep')
    @patch('mellanox_fw_manager.firmware_base.subprocess.run')
    def test_get_firmware_versions_missing_psid_field(self, mock_run, mock_sleep):
        """Test _get_firmware_versions when psid field is missing"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        xml_output = '''<?xml version="1.0"?>
        <Devices>
            <Device>
                <Versions>
                    <FW current="1.0.0"/>
                </Versions>
            </Device>
        </Devices>'''

        mock_run.return_value = MagicMock(returncode=0, stdout=xml_output)

        with patch.object(manager.logger, 'info') as mock_info:
            with patch.object(manager.logger, 'error') as mock_error:
                current, available = manager._get_firmware_versions()

                self.assertIsNone(current)
                self.assertIsNone(available)
                self.assertEqual(mock_run.call_count, 10)
                self.assertEqual(mock_sleep.call_count, 9)
                # 10 "Executing" logs from run_command + 9 "Unable to parse" logs = 19 total
                self.assertEqual(mock_info.call_count, 19)
                mock_error.assert_called_once_with("Failed to get firmware versions after 10 attempts: Version or PSID not found in response")

    def test_get_env_verbose_mode(self):
        """Test _get_env with verbose mode"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test",
                verbose=True
            )

        env = manager._get_env()

        self.assertIn('FLASH_ACCESS_DEBUG', env)
        self.assertIn('FW_COMPS_DEBUG', env)
        self.assertEqual(env['FLASH_ACCESS_DEBUG'], '1')
        self.assertEqual(env['FW_COMPS_DEBUG'], '1')

    def test_get_env_normal_mode(self):
        """Test _get_env without verbose mode"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test",
                verbose=False
            )

        env = manager._get_env()

        self.assertNotIn('FLASH_ACCESS_DEBUG', env)
        self.assertNotIn('FW_COMPS_DEBUG', env)

    @patch('mellanox_fw_manager.firmware_base.subprocess.run')
    def test_clear_semaphore_success(self, mock_run):
        """Test clear_semaphore success (lines 356-373)"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        mock_run.return_value = MagicMock(returncode=0)

        result = manager.clear_semaphore()

        self.assertTrue(result)
        mock_run.assert_called_once_with(
            ['/usr/bin/flint', '-d', '01:00.0', '--clear_semaphore'],
            capture_output=True,
            text=True,
            check=True
        )

    @patch('mellanox_fw_manager.firmware_base.subprocess.run')
    def test_clear_semaphore_subprocess_error(self, mock_run):
        """Test clear_semaphore with subprocess error"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        from subprocess import CalledProcessError
        mock_run.side_effect = CalledProcessError(1, 'flint', stderr="Command failed")

        result = manager.clear_semaphore()

        self.assertFalse(result)

    @patch('mellanox_fw_manager.firmware_base.subprocess.run')
    def test_clear_semaphore_general_exception(self, mock_run):
        """Test clear_semaphore with general exception"""
        with patch.object(ConcreteFirmwareManager, '_initialize_asic'):
            manager = ConcreteFirmwareManager(
                asic_index=0,
                pci_id="01:00.0",
                asic_type="test"
            )

        mock_run.side_effect = Exception("General error")

        result = manager.clear_semaphore()

        self.assertFalse(result)


if __name__ == '__main__':
    unittest.main()
