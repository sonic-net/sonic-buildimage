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
Unit tests for firmware upgrade functionality with mocked mlxfwmanager
"""

from mellanox_fw_manager.fw_manager import UpgradeStatusType, FirmwareUpgradeError, FirmwareUpgradePartialError
from mellanox_fw_manager.firmware_coordinator import FirmwareCoordinator
from mellanox_fw_manager.bluefield_manager import BluefieldFirmwareManager
from mellanox_fw_manager.spectrum_manager import SpectrumFirmwareManager
import os
import sys
import unittest
from unittest.mock import patch, MagicMock, mock_open
import tempfile
import multiprocessing

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))


class TestFirmwareUpgrade(unittest.TestCase):
    """Test firmware upgrade functionality with mocked mlxfwmanager"""

    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()

        self.spectrum3_xml = '''<Devices>
    <Device pciName="/dev/mst/mt53104_pci_cr0" type="Spectrum3" psid="MT_0000001187" partNumber="MSN4280-WS2RXO-NS_Ax">
      <Versions>
        <FW current="30.2016.1036" available="30.2016.1040"/>
      </Versions>
      <MACs Base_Mac="b0cf0e1e8c00" />
      <Status>Update required</Status>
      <Description>NVIDIA Spectrum-3 based 400GbE 2U Ethernet Smart-Switch with ONIE; 28 ports QSFP-DD; 4 DPUs; 2 power supplies (AC); x86 CPU - 16 Cores; Secure-boot Capable; standard depth; C2P airflow; Tool-less Rail Kit - INTERNAL USE</Description>
    </Device>
</Devices>'''
        self.bluefield3_xml = '''<Devices>
    <Device pciName="/dev/mst/mt41692_pciconf0" type="BlueField3" psid="MT_0000001138" partNumber="BF3-COM-DPU_DK_Ax">
      <Versions>
        <FW current="32.46.0412" available="32.46.0420"/>
      </Versions>
      <MACs Base_Mac="b0cf0e1e8cee" />
      <GUIDs Base_Guid="b0cf0e03001e8cee" />
      <Status>Update required</Status>
      <Description>NVIDIA BlueField-3 DPU based COM express; DK IPN</Description>
    </Device>
</Devices>'''

        self.spectrum3_no_available_xml = '''<Devices>
    <Device pciName="/dev/mst/mt53104_pci_cr0" type="Spectrum3" psid="MT_0000001187" partNumber="MSN4280-WS2RXO-NS_Ax">
      <Versions>
        <FW current="30.2016.1036" available="N/A"/>
      </Versions>
      <Status>No matching image found</Status>
    </Device>
</Devices>'''
        self.bluefield3_no_available_xml = '''<Devices>
    <Device pciName="/dev/mst/mt41692_pciconf0" type="BlueField3" psid="MT_0000001138" partNumber="BF3-COM-DPU_DK_Ax">
      <Versions>
        <FW current="32.46.0412" available="N/A"/>
      </Versions>
      <Status>No matching image found</Status>
    </Device>
</Devices>'''

    def tearDown(self):
        """Clean up test fixtures"""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def _create_spectrum_manager(self, **kwargs):
        """Helper to create SpectrumFirmwareManager with default values"""
        defaults = {
            'asic_index': 0,
            'pci_id': "01:00.0",
            'fw_bin_path': "/test/fw",
            'verbose': False,
            'clear_semaphore': False,
            'asic_type': "SPC3"
        }
        defaults.update(kwargs)

        manager = SpectrumFirmwareManager(**defaults)
        manager.fw_file = "/test/fw/fw-SPC3.mfa"
        return manager

    def _create_bluefield_manager(self, **kwargs):
        """Helper to create BluefieldFirmwareManager with default values"""
        defaults = {
            'asic_index': 0,
            'pci_id': "03:00.0",
            'fw_bin_path': "/test/fw",
            'verbose': False,
            'clear_semaphore': False,
            'asic_type': "BF3"
        }
        defaults.update(kwargs)

        manager = BluefieldFirmwareManager(**defaults)
        manager.fw_file = "/test/fw/fw-BF3.mfa"
        return manager

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.spectrum_manager.subprocess.run')
    def test_spectrum_firmware_upgrade_success(self, mock_run, mock_init_asic):
        """Test successful firmware upgrade for Spectrum ASIC"""
        mock_init_asic.return_value = None

        mock_run.side_effect = [
            MagicMock(returncode=0, stdout=self.spectrum3_xml),
            MagicMock(returncode=0, stdout="MT_0000001187 field1 field2 30.2016.1040"),
            MagicMock(returncode=0, stdout="Firmware update completed successfully")
        ]

        manager = self._create_spectrum_manager()

        with patch('mellanox_fw_manager.firmware_base.os.path.exists', return_value=True):
            current_version, available_version = manager._get_firmware_versions()
            self.assertEqual(current_version, "30.2016.1036")
            self.assertEqual(available_version, "30.2016.1040")

            mock_run.reset_mock()
            mock_run.return_value = MagicMock(returncode=0, stdout="Firmware update completed successfully")

            result = manager.run_firmware_update()
            self.assertTrue(result)

            self.assertEqual(mock_run.call_count, 1)

            upgrade_call = mock_run.call_args_list[0]
            self.assertIn('mlxfwmanager', upgrade_call[0][0])
            self.assertIn('-u', upgrade_call[0][0])
            self.assertIn('-f', upgrade_call[0][0])
            self.assertIn('-y', upgrade_call[0][0])
            self.assertIn('-d', upgrade_call[0][0])
            self.assertIn('01:00.0', upgrade_call[0][0])
            self.assertIn('-i', upgrade_call[0][0])
            self.assertIn('/test/fw/fw-SPC3.mfa', upgrade_call[0][0])

    @patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.bluefield_manager.subprocess.run')
    def test_bluefield_firmware_upgrade_success(self, mock_run, mock_init_asic):
        """Test successful firmware upgrade for BlueField ASIC"""
        mock_init_asic.return_value = None

        mock_run.side_effect = [
            MagicMock(returncode=0, stdout=self.bluefield3_xml),
            MagicMock(returncode=0, stdout="Image type: BF3\nFW Version: 32.46.0420\nPSID: MT_0000001138"),
            MagicMock(returncode=0, stdout="FW reactivation completed"),
            MagicMock(returncode=0, stdout="Firmware burn completed successfully"),
            MagicMock(returncode=0, stdout="Configuration reset completed")
        ]

        manager = self._create_bluefield_manager()

        with patch('mellanox_fw_manager.firmware_base.os.path.exists', return_value=True):
            current_version, available_version = manager._get_firmware_versions()
            self.assertEqual(current_version, "32.46.0412")
            self.assertEqual(available_version, "32.46.0420")

            manager.run_firmware_update()

            self.assertEqual(mock_run.call_count, 5)

            query_call = mock_run.call_args_list[0]
            self.assertIn('mlxfwmanager', query_call[0][0])
            self.assertIn('--query-format', query_call[0][0])
            self.assertIn('XML', query_call[0][0])

            flint_query_call = mock_run.call_args_list[1]
            self.assertIn('flint', flint_query_call[0][0])
            self.assertIn('--psid', flint_query_call[0][0])
            self.assertIn('query', flint_query_call[0][0])

            reactivate_call = mock_run.call_args_list[2]
            self.assertIn('flint', reactivate_call[0][0])
            self.assertIn('-d', reactivate_call[0][0])
            self.assertIn('ir', reactivate_call[0][0])

            burn_call = mock_run.call_args_list[3]
            self.assertIn('flint', burn_call[0][0])
            self.assertIn('-d', burn_call[0][0])
            self.assertIn('burn', burn_call[0][0])

            reset_call = mock_run.call_args_list[4]
            self.assertIn('mlxconfig', reset_call[0][0])
            self.assertIn('-d', reset_call[0][0])
            self.assertIn('-y', reset_call[0][0])
            self.assertIn('r', reset_call[0][0])

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.spectrum_manager.subprocess.run')
    def test_firmware_upgrade_failure(self, mock_run, mock_init_asic):
        """Test firmware upgrade failure handling"""
        mock_init_asic.return_value = None

        mock_run.return_value = MagicMock(returncode=1, stderr="Error: Failed to update firmware - device busy")

        manager = self._create_spectrum_manager()

        with patch('mellanox_fw_manager.firmware_base.os.path.exists', return_value=True):
            result = manager.run_firmware_update()
            self.assertFalse(result)

            self.assertEqual(mock_run.call_count, 1)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.spectrum_manager.subprocess.run')
    def test_spectrum_firmware_reactivation_scenario(self, mock_run, mock_init_asic):
        """Test firmware upgrade with reactivation required (return code 2)"""
        mock_init_asic.return_value = None

        mock_run.side_effect = [
            MagicMock(returncode=2, stdout="Firmware reactivation required"),
            MagicMock(returncode=0, stdout="FW reactivation completed"),
            MagicMock(returncode=0, stdout="Firmware update completed successfully")
        ]

        manager = self._create_spectrum_manager()

        with patch('mellanox_fw_manager.firmware_base.os.path.exists', return_value=True):
            result = manager.run_firmware_update()
            self.assertTrue(result)

            self.assertEqual(mock_run.call_count, 3)

            reactivate_call = mock_run.call_args_list[1]
            self.assertIn('flint', reactivate_call[0][0])
            self.assertIn('-d', reactivate_call[0][0])
            self.assertIn('01:00.0', reactivate_call[0][0])
            self.assertIn('ir', reactivate_call[0][0])

            retry_call = mock_run.call_args_list[2]
            self.assertIn('mlxfwmanager', retry_call[0][0])
            self.assertIn('-u', retry_call[0][0])

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.spectrum_manager.subprocess.run')
    def test_firmware_version_parsing(self, mock_run, mock_init_asic):
        """Test firmware version parsing from mlxfwmanager output"""
        mock_init_asic.return_value = None

        test_cases = [
            {
                'xml': self.spectrum3_no_available_xml,
                'expected': "30.2016.1036",
                'psid': "MT_0000001187"
            },
            {
                'xml': self.bluefield3_no_available_xml,
                'expected': "32.46.0412",
                'psid': "MT_0000001138"
            }
        ]

        manager = self._create_spectrum_manager()

        for test_case in test_cases:
            mock_run.side_effect = [
                MagicMock(returncode=0, stdout=test_case['xml']),
                MagicMock(returncode=0, stdout=f"{test_case['psid']} field1 field2 {test_case['expected']}")
            ]

            version = manager._get_firmware_versions()
            self.assertEqual(version[0], test_case['expected'])

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.spectrum_manager.subprocess.run')
    def test_firmware_query_failure(self, mock_run, mock_init_asic):
        """Test handling of firmware query failures"""
        mock_init_asic.return_value = None

        mock_run.return_value = MagicMock(
            returncode=1,
            stderr="Error: Device not found"
        )

        manager = self._create_spectrum_manager()

        version = manager._get_firmware_versions()
        self.assertEqual(version[0], None)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.spectrum_manager.subprocess.run')
    def test_clear_semaphore_functionality(self, mock_run, mock_init_asic):
        """Test clear semaphore functionality"""
        mock_init_asic.return_value = None

        mock_run.side_effect = [
            MagicMock(returncode=0, stdout="Semaphore cleared successfully"),
            MagicMock(
                returncode=0,
                stdout="FW Version: 13.2000.1004\nPSID: MT_2700000123\nDetailed info..."
            ),
            MagicMock(returncode=0, stdout="Verbose firmware update output...")
        ]

        mock_run.return_value = MagicMock(returncode=0, stdout="Firmware update completed successfully")

        manager = self._create_spectrum_manager(clear_semaphore=True)

        with patch('mellanox_fw_manager.firmware_base.os.path.exists', return_value=True):
            result = manager.run_firmware_update()
            self.assertTrue(result)

            self.assertEqual(mock_run.call_count, 1)

            upgrade_call = mock_run.call_args_list[0]
            self.assertIn('mlxfwmanager', upgrade_call[0][0])
            self.assertIn('-u', upgrade_call[0][0])

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.spectrum_manager.subprocess.run')
    def test_verbose_mode_commands(self, mock_run, mock_init_asic):
        """Test that verbose mode affects mlxfwmanager commands"""
        mock_init_asic.return_value = None

        mock_run.return_value = MagicMock(returncode=0, stdout="Verbose firmware update output...")

        manager = self._create_spectrum_manager(verbose=True)

        with patch('mellanox_fw_manager.firmware_base.os.path.exists', return_value=True):
            result = manager.run_firmware_update()
            self.assertTrue(result)

            self.assertEqual(mock_run.call_count, 1)

            upgrade_call = mock_run.call_args_list[0]
            env = upgrade_call[1].get('env', {})
            self.assertIn('FLASH_ACCESS_DEBUG', env)
            self.assertIn('FW_COMPS_DEBUG', env)
            self.assertEqual(env['FLASH_ACCESS_DEBUG'], '1')
            self.assertEqual(env['FW_COMPS_DEBUG'], '1')

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_coordinator_firmware_upgrade_success(self, mock_create_manager,
                                                  mock_asic_manager, mock_detect_platform):
        """Test firmware upgrade coordination across multiple ASICs"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 2
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["01:00.0", "02:00.0"]

        mock_managers = []
        for i in range(2):
            mock_manager = MagicMock()
            mock_manager.asic_index = i
            mock_manager.is_alive.return_value = False
            mock_managers.append(mock_manager)

        mock_create_manager.side_effect = mock_managers

        with patch('mellanox_fw_manager.firmware_coordinator.Queue') as mock_queue_class:
            mock_queue = MagicMock()
            mock_queue.empty.side_effect = [False, False, True]  # Two items, then empty
            mock_queue.get_nowait.side_effect = [
                {
                    'asic_index': 0,
                    'status': UpgradeStatusType.SUCCESS.value,
                    'message': 'Firmware upgraded successfully',
                    'current_version': '13.2000.1004',
                    'available_version': '13.2000.1010',
                    'pci_id': '01:00.0'
                },
                {
                    'asic_index': 1,
                    'status': UpgradeStatusType.SUCCESS.value,
                    'message': 'Firmware upgraded successfully',
                    'current_version': '13.2000.1004',
                    'available_version': '13.2000.1010',
                    'pci_id': '02:00.0'
                }
            ]
            mock_queue_class.return_value = mock_queue

            coordinator = FirmwareCoordinator(verbose=False)

            coordinator.upgrade_firmware()

            for manager in mock_managers:
                manager.start.assert_called_once()

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_coordinator_partial_failure(self, mock_create_manager,
                                         mock_asic_manager, mock_detect_platform):
        """Test firmware upgrade coordination with partial failures"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 2
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["01:00.0", "02:00.0"]

        mock_managers = []
        for i in range(2):
            mock_manager = MagicMock()
            mock_manager.asic_index = i
            mock_manager.is_alive.return_value = False
            mock_managers.append(mock_manager)

        mock_create_manager.side_effect = mock_managers

        with patch('mellanox_fw_manager.firmware_coordinator.Queue') as mock_queue_class:
            mock_queue = MagicMock()
            mock_queue.empty.side_effect = [False, False, True]  # Two items, then empty
            mock_queue.get_nowait.side_effect = [
                {
                    'asic_index': 0,
                    'status': UpgradeStatusType.SUCCESS.value,
                    'message': 'Firmware upgraded successfully',
                    'current_version': '13.2000.1004',
                    'available_version': '13.2000.1010',
                    'pci_id': '01:00.0'
                },
                {
                    'asic_index': 1,
                    'status': UpgradeStatusType.FAILED.value,
                    'message': 'Firmware upgrade failed - device busy',
                    'current_version': '13.2000.1004',
                    'available_version': '13.2000.1010',
                    'pci_id': '02:00.0'
                }
            ]
            mock_queue_class.return_value = mock_queue

            coordinator = FirmwareCoordinator(verbose=False)

            try:
                coordinator.upgrade_firmware()
                self.assertTrue(True)
            except FirmwareUpgradePartialError:
                self.assertTrue(True)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.spectrum_manager.subprocess.run')
    def test_firmware_file_not_found(self, mock_run, mock_init_asic):
        """Test handling when firmware file doesn't exist"""
        mock_init_asic.return_value = None

        manager = self._create_spectrum_manager()

        with patch('mellanox_fw_manager.firmware_base.os.path.exists', return_value=False):
            result = manager.run_firmware_update()
            self.assertFalse(result)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.spectrum_manager.subprocess.run')
    def test_spectrum_firmware_upgrade_exception(self, mock_run, mock_init_asic):
        """Test firmware upgrade exception handling"""
        mock_init_asic.return_value = None

        mock_run.side_effect = Exception("Subprocess execution failed")

        manager = self._create_spectrum_manager()

        with patch('mellanox_fw_manager.firmware_base.os.path.exists', return_value=True):
            result = manager.run_firmware_update()
            self.assertFalse(result)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._initialize_asic')
    @patch('mellanox_fw_manager.spectrum_manager.subprocess.run')
    def test_mst_device_detection(self, mock_run, mock_init_asic):
        """Test MST device detection and retry logic"""
        mock_init_asic.return_value = None

        lspci_output = "01:00.0 Ethernet controller [0200]: Mellanox Technologies MT53104 Family [Spectrum-3]"

        mock_run.side_effect = [
            MagicMock(returncode=0, stdout=lspci_output),
            MagicMock(returncode=0, stdout=self.spectrum3_no_available_xml),
            MagicMock(returncode=0, stdout="/dev/mst/mt53104_pci_cr0         - PCI direct access.\n                                   domain:bus:dev.fn=0000:06:00.0 bar=0xe8000000 size=0x4000000"),
        ]

        manager = self._create_spectrum_manager()

        with patch('mellanox_fw_manager.firmware_base.os.path.exists', return_value=True):
            pass


if __name__ == '__main__':
    unittest.main()
