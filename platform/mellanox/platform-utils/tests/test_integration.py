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
Integration tests for the complete firmware manager system
"""

from mellanox_fw_manager.asic_manager import AsicManager
from mellanox_fw_manager.fw_manager import create_firmware_manager
from mellanox_fw_manager.firmware_coordinator import FirmwareCoordinator
import os
import tempfile
import unittest
from unittest.mock import patch, MagicMock, mock_open
import sys
import multiprocessing

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))


class TestIntegration(unittest.TestCase):
    """Integration tests for the complete system"""

    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()

    def tearDown(self):
        """Clean up test fixtures"""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_error_handling_integration(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test error handling integration across components"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 2
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["01:00.0", "02:00.0"]
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_manager1 = MagicMock()
        mock_manager1.asic_index = 0
        mock_manager1.is_upgrade_required.return_value = True

        mock_manager2 = MagicMock()
        mock_manager2.asic_index = 1
        mock_manager2.is_upgrade_required.return_value = False

        mock_create_manager.side_effect = [mock_manager1, mock_manager2]

        coordinator = FirmwareCoordinator(verbose=True)

        coordinator.managers = [mock_manager1, mock_manager2]

        result = coordinator.check_upgrade_required()
        self.assertTrue(result)

        mock_manager1.is_upgrade_required.assert_called_once()
    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_asic_detection_integration(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test ASIC detection integration"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 1
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["01:00.0"]
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_manager = MagicMock()
        mock_manager.asic_index = 0
        mock_manager.is_upgrade_required.return_value = False
        mock_create_manager.return_value = mock_manager

        coordinator = FirmwareCoordinator()

        asic_count = coordinator.get_asic_count()
        self.assertEqual(asic_count, 1)

        pci_ids = coordinator.get_asic_pci_ids()
        self.assertEqual(pci_ids, ["01:00.0"])

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._initialize_asic')
    def test_firmware_manager_creation_integration(self, mock_init_asic):
        """Test firmware manager creation integration for Spectrum"""
        mock_init_asic.return_value = None

        manager = create_firmware_manager(0, "01:00.0", asic_type="SPC3", fw_bin_path="/test/fw")

        from mellanox_fw_manager.spectrum_manager import SpectrumFirmwareManager
        self.assertIsInstance(manager, SpectrumFirmwareManager)
        self.assertEqual(manager.asic_index, 0)
        self.assertEqual(manager.pci_id, "01:00.0")
        self.assertEqual(manager.fw_bin_path, "/test/fw")
        self.assertEqual(manager.asic_type, "SPC3")

    @patch('mellanox_fw_manager.bluefield_manager.BluefieldFirmwareManager._initialize_asic')
    def test_firmware_manager_creation_bluefield(self, mock_init_asic):
        """Test firmware manager creation for BlueField"""
        mock_init_asic.return_value = None

        manager = create_firmware_manager(0, "08:00.0", asic_type="BF3", fw_bin_path="/test/fw")

        from mellanox_fw_manager.bluefield_manager import BluefieldFirmwareManager
        self.assertIsInstance(manager, BluefieldFirmwareManager)
        self.assertEqual(manager.asic_index, 0)
        self.assertEqual(manager.pci_id, "08:00.0")
        self.assertEqual(manager.fw_bin_path, "/test/fw")
        self.assertEqual(manager.asic_type, "BF3")

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_coordinator_lifecycle(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test complete coordinator lifecycle"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 1
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["01:00.0"]
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_manager = MagicMock()
        mock_manager.asic_index = 0
        mock_manager.is_upgrade_required.return_value = False
        mock_create_manager.return_value = mock_manager

        coordinator = FirmwareCoordinator(verbose=True, from_image=False, clear_semaphore=True)

        self.assertEqual(coordinator.get_asic_count(), 1)
        self.assertEqual(coordinator.get_asic_pci_ids(), ["01:00.0"])
        self.assertFalse(coordinator.check_upgrade_required())


if __name__ == '__main__':
    unittest.main()
