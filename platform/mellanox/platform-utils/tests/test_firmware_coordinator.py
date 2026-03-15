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
Unit tests for FirmwareCoordinator class
"""

from mellanox_fw_manager.firmware_coordinator import FirmwareCoordinator
from mellanox_fw_manager.fw_manager import (
    create_firmware_manager, UpgradeStatus, UpgradeStatusType,
    FirmwareUpgradeError, FirmwareUpgradePartialError, FirmwareManagerError
)
import os
import tempfile
import unittest
from unittest.mock import patch, MagicMock, mock_open
import sys
import multiprocessing

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))


class TestFirmwareCoordinator(unittest.TestCase):
    """Test cases for FirmwareCoordinator class"""

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
    def test_init(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test FirmwareCoordinator initialization"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 2
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["01:00.0", "02:00.0"]
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"
        mock_create_manager.return_value = MagicMock()

        coordinator = FirmwareCoordinator(verbose=True, from_image=False, clear_semaphore=True)

        self.assertTrue(coordinator.verbose)
        self.assertFalse(coordinator.from_image)
        self.assertTrue(coordinator.clear_semaphore)
        self.assertEqual(coordinator.fw_bin_path, "/etc/mlnx")

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_init_from_image(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test FirmwareCoordinator initialization with from_image=True"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 1
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["01:00.0"]
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"
        mock_create_manager.return_value = MagicMock()

        with patch.object(FirmwareCoordinator, '_get_image_firmware_path', return_value="/test/fw"):
            coordinator = FirmwareCoordinator(verbose=False, from_image=True, clear_semaphore=False)

            self.assertFalse(coordinator.verbose)
            self.assertTrue(coordinator.from_image)
            self.assertFalse(coordinator.clear_semaphore)
            self.assertEqual(coordinator.fw_bin_path, "/test/fw")

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_get_asic_count(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test get_asic_count method"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 3
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = []
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"
        mock_create_manager.return_value = MagicMock()

        coordinator = FirmwareCoordinator()
        result = coordinator.get_asic_count()

        self.assertEqual(result, 3)

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_get_asic_pci_ids(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test get_asic_pci_ids method"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 2
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["01:00.0", "02:00.0"]
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"
        mock_create_manager.return_value = MagicMock()

        coordinator = FirmwareCoordinator()
        result = coordinator.get_asic_pci_ids()

        self.assertEqual(result, ["01:00.0", "02:00.0"])

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_reset_firmware_config_not_supported(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test reset_firmware_config when not supported"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 1
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = []
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"
        mock_create_manager.return_value = MagicMock()

        coordinator = FirmwareCoordinator()

        with self.assertRaises(FirmwareManagerError):
            coordinator.reset_firmware_config()

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_reset_firmware_config_supported(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test reset_firmware_config when supported"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 1
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = []
        mock_asic_manager.return_value.get_asic_type.return_value = "bluefield3"

        from mellanox_fw_manager.fw_manager import BluefieldFirmwareManager
        mock_manager = MagicMock(spec=BluefieldFirmwareManager)
        mock_manager.reset_firmware_config = MagicMock()
        mock_manager.asic_type = "bluefield3"
        mock_manager.asic_index = 0
        mock_create_manager.return_value = mock_manager

        coordinator = FirmwareCoordinator()
        coordinator.managers = [mock_manager]

        coordinator.reset_firmware_config()

        mock_manager.reset_firmware_config.assert_called_once()

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_check_upgrade_required_no_failures(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test check_upgrade_required when no previous failures"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 1
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = []
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_manager = MagicMock()
        mock_manager.asic_index = 0
        mock_manager.is_upgrade_required.return_value = True
        mock_create_manager.return_value = mock_manager

        coordinator = FirmwareCoordinator()
        result = coordinator.check_upgrade_required()

        self.assertTrue(result)

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_check_upgrade_required_with_failures(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test check_upgrade_required when there are previous failures"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 1
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = []
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_manager = MagicMock()
        mock_manager.asic_index = 0
        mock_manager.is_upgrade_required.return_value = True
        mock_create_manager.return_value = mock_manager

        coordinator = FirmwareCoordinator()
        result = coordinator.check_upgrade_required()

        self.assertTrue(result)

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_init_manager_creation_failure(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test FirmwareCoordinator initialization when manager creation fails (lines 72-75)"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 1
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["01:00.0"]
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_create_manager.side_effect = FirmwareManagerError("Failed to create manager")

        with self.assertRaises(FirmwareManagerError):
            FirmwareCoordinator()

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_upgrade_firmware_queue_processing_exception(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test upgrade_firmware when queue processing raises exception (lines 140-141)"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 1
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["01:00.0"]
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_manager = MagicMock()
        mock_manager.asic_index = 0
        mock_manager.is_alive.return_value = False
        mock_create_manager.return_value = mock_manager

        coordinator = FirmwareCoordinator()

        with patch('mellanox_fw_manager.firmware_coordinator.Queue') as mock_queue:
            mock_queue_instance = MagicMock()
            mock_queue.return_value = mock_queue_instance

            mock_queue_instance.empty.side_effect = [False, True]
            mock_queue_instance.get_nowait.side_effect = Exception("Queue error")

            with self.assertRaises(FirmwareUpgradeError):
                coordinator.upgrade_firmware()

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_upgrade_firmware_all_failures(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test upgrade_firmware when all ASICs fail (lines 151-152)"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 1
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["01:00.0"]
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_manager = MagicMock()
        mock_manager.asic_index = 0
        mock_manager.is_alive.return_value = False
        mock_create_manager.return_value = mock_manager

        coordinator = FirmwareCoordinator()

        with patch('mellanox_fw_manager.firmware_coordinator.Queue') as mock_queue:
            mock_queue_instance = MagicMock()
            mock_queue.return_value = mock_queue_instance

            status_data = {
                'asic_index': 0,
                'status': 'failed',
                'message': 'Upgrade failed',
                'current_version': '1.0',
                'available_version': '1.1',
                'pci_id': '01:00.0'
            }
            mock_queue_instance.empty.side_effect = [False, True]
            mock_queue_instance.get_nowait.return_value = status_data

            with self.assertRaises(FirmwareUpgradeError) as context:
                coordinator.upgrade_firmware()

            self.assertIn("All ASIC upgrades failed", str(context.exception))

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_upgrade_firmware_partial_failures(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test upgrade_firmware with partial failures (lines 154-155)"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 2
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["01:00.0", "02:00.0"]
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_manager1 = MagicMock()
        mock_manager1.asic_index = 0
        mock_manager1.is_alive.return_value = False
        mock_manager2 = MagicMock()
        mock_manager2.asic_index = 1
        mock_manager2.is_alive.return_value = False
        mock_create_manager.side_effect = [mock_manager1, mock_manager2]

        coordinator = FirmwareCoordinator()

        with patch('mellanox_fw_manager.firmware_coordinator.Queue') as mock_queue:
            mock_queue_instance = MagicMock()
            mock_queue.return_value = mock_queue_instance

            status_data = [
                {
                    'asic_index': 0,
                    'status': 'success',
                    'message': 'Upgrade completed',
                    'current_version': '1.0',
                    'available_version': '1.1',
                    'pci_id': '01:00.0'
                },
                {
                    'asic_index': 1,
                    'status': 'failed',
                    'message': 'Upgrade failed',
                    'current_version': '1.0',
                    'available_version': '1.1',
                    'pci_id': '02:00.0'
                }
            ]
            mock_queue_instance.empty.side_effect = [False, False, True]
            mock_queue_instance.get_nowait.side_effect = status_data

            with self.assertRaises(FirmwareUpgradePartialError) as context:
                coordinator.upgrade_firmware()

            self.assertIn("Some ASIC upgrades failed", str(context.exception))

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_check_upgrade_required_error_handling(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test check_upgrade_required when manager.is_upgrade_required raises exception (lines 191-194)"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 1
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["01:00.0"]
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_manager = MagicMock()
        mock_manager.asic_index = 0
        mock_manager.is_upgrade_required.side_effect = Exception("Check failed")
        mock_create_manager.return_value = mock_manager

        coordinator = FirmwareCoordinator()
        result = coordinator.check_upgrade_required()

        self.assertTrue(result)

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    @patch('mellanox_fw_manager.firmware_coordinator.run_command')
    def test_get_image_firmware_path_sonic_installer_failure(self, mock_run, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test _get_image_firmware_path when sonic-installer fails (lines 214-215)"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 0
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = []
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_run.return_value = MagicMock(returncode=1, stdout="", stderr="Command failed")

        with self.assertRaises(FirmwareUpgradeError) as context:
            FirmwareCoordinator(from_image=True)

        self.assertIn("Failed to get SONiC image list", str(context.exception))

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    @patch('mellanox_fw_manager.firmware_coordinator.run_command')
    def test_get_image_firmware_path_no_next_image(self, mock_run, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test _get_image_firmware_path when no next image found (lines 225-226)"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 0
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = []
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_run.return_value = MagicMock(
            returncode=0,
            stdout="Current: SONiC-OS-202301.01\nAvailable: SONiC-OS-202302.01"
        )

        with self.assertRaises(FirmwareUpgradeError) as context:
            FirmwareCoordinator(from_image=True)

        self.assertIn("No next SONiC image found", str(context.exception))

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    @patch('mellanox_fw_manager.firmware_coordinator.run_command')
    @patch('mellanox_fw_manager.firmware_coordinator.os.path.exists')
    def test_get_image_firmware_path_current_equals_next(self, mock_exists, mock_run, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test _get_image_firmware_path when current equals next image - should proceed normally"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 0
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = []
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_run.return_value = MagicMock(
            returncode=0,
            stdout="Current: SONiC-OS-202301.01\nNext: SONiC-OS-202301.01"
        )

        mock_exists.return_value = True

        coordinator = FirmwareCoordinator(from_image=True)

        expected_path = "/host/image-202301.01/platform/fw/asic/"
        self.assertEqual(coordinator.fw_bin_path, expected_path)

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    @patch('mellanox_fw_manager.firmware_coordinator.run_command')
    @patch('mellanox_fw_manager.firmware_coordinator.os.path.exists')
    def test_get_image_firmware_path_platform_specific(self, mock_exists, mock_run, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test _get_image_firmware_path with platform-specific path (lines 234-236)"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 0
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = []
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_run.return_value = MagicMock(
            returncode=0,
            stdout="Current: SONiC-OS-202301.01\nNext: SONiC-OS-202302.01"
        )

        mock_exists.return_value = True

        coordinator = FirmwareCoordinator(from_image=True)

        expected_path = "/host/image-202302.01/platform/fw/asic/"
        self.assertEqual(coordinator.fw_bin_path, expected_path)

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    @patch('mellanox_fw_manager.firmware_coordinator.run_command')
    @patch('mellanox_fw_manager.firmware_coordinator.os.path.exists')
    @patch('mellanox_fw_manager.firmware_coordinator.os.makedirs')
    def test_get_image_firmware_path_squashfs_mount_success(self, mock_makedirs, mock_exists, mock_run, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test _get_image_firmware_path with squashfs mounting (lines 245-254)"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 0
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = []
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_run.side_effect = [
            MagicMock(returncode=0, stdout="Current: SONiC-OS-202301.01\nNext: SONiC-OS-202302.01"),
            MagicMock(returncode=0)
        ]

        mock_exists.return_value = False

        coordinator = FirmwareCoordinator(from_image=True)

        expected_path = "/tmp/image-202302.01-fs/etc/mlnx/"
        self.assertEqual(coordinator.fw_bin_path, expected_path)

        mount_call = mock_run.call_args_list[1]
        self.assertIn('mount', mount_call[0][0])
        self.assertIn('squashfs', mount_call[0][0])

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    @patch('mellanox_fw_manager.firmware_coordinator.run_command')
    @patch('mellanox_fw_manager.firmware_coordinator.os.path.exists')
    @patch('mellanox_fw_manager.firmware_coordinator.os.makedirs')
    def test_get_image_firmware_path_mount_failure(self, mock_makedirs, mock_exists, mock_run, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test _get_image_firmware_path when mount fails (lines 250-251)"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 0
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = []
        mock_asic_manager.return_value.get_asic_type.return_value = "spectrum"

        mock_run.side_effect = [
            MagicMock(returncode=0, stdout="Current: SONiC-OS-202301.01\nNext: SONiC-OS-202302.01"),
            MagicMock(returncode=1)
        ]

        mock_exists.return_value = False

        with self.assertRaises(FirmwareUpgradeError) as context:
            FirmwareCoordinator(from_image=True)

        self.assertIn("Failed to mount", str(context.exception))

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_reset_firmware_config_reset_failure(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test reset_firmware_config when reset fails (lines 305-306)"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 1
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["08:00.0"]
        mock_asic_manager.return_value.get_asic_type.return_value = "bluefield3"

        from mellanox_fw_manager.bluefield_manager import BluefieldFirmwareManager
        mock_manager = MagicMock(spec=BluefieldFirmwareManager)
        mock_manager.asic_index = 0
        mock_manager.reset_firmware_config.return_value = False
        mock_create_manager.return_value = mock_manager

        coordinator = FirmwareCoordinator()

        with self.assertRaises(FirmwareManagerError):
            coordinator.reset_firmware_config()

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_reset_firmware_config_exception(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test reset_firmware_config when exception occurs (lines 307-309)"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 1
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["08:00.0"]
        mock_asic_manager.return_value.get_asic_type.return_value = "bluefield3"

        from mellanox_fw_manager.bluefield_manager import BluefieldFirmwareManager
        mock_manager = MagicMock(spec=BluefieldFirmwareManager)
        mock_manager.asic_index = 0
        mock_manager.reset_firmware_config.side_effect = Exception("Reset error")
        mock_create_manager.return_value = mock_manager

        coordinator = FirmwareCoordinator()

        with self.assertRaises(FirmwareManagerError):
            coordinator.reset_firmware_config()

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_reset_firmware_config_all_failures(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test reset_firmware_config when all resets fail (line 312)"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 2
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["08:00.0", "09:00.0"]
        mock_asic_manager.return_value.get_asic_type.return_value = "bluefield3"

        from mellanox_fw_manager.bluefield_manager import BluefieldFirmwareManager
        mock_manager1 = MagicMock(spec=BluefieldFirmwareManager)
        mock_manager1.asic_index = 0
        mock_manager1.reset_firmware_config.return_value = False
        mock_manager2 = MagicMock(spec=BluefieldFirmwareManager)
        mock_manager2.asic_index = 1
        mock_manager2.reset_firmware_config.return_value = False
        mock_create_manager.side_effect = [mock_manager1, mock_manager2]

        coordinator = FirmwareCoordinator()

        with self.assertRaises(FirmwareManagerError) as context:
            coordinator.reset_firmware_config()

        self.assertIn("All BlueField ASIC resets failed", str(context.exception))

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_reset_firmware_config_partial_failures(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test reset_firmware_config with partial failures (line 314)"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 2
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = ["08:00.0", "09:00.0"]
        mock_asic_manager.return_value.get_asic_type.return_value = "bluefield3"

        from mellanox_fw_manager.bluefield_manager import BluefieldFirmwareManager
        mock_manager1 = MagicMock(spec=BluefieldFirmwareManager)
        mock_manager1.asic_index = 0
        mock_manager1.reset_firmware_config.return_value = True
        mock_manager2 = MagicMock(spec=BluefieldFirmwareManager)
        mock_manager2.asic_index = 1
        mock_manager2.reset_firmware_config.return_value = False
        mock_create_manager.side_effect = [mock_manager1, mock_manager2]

        coordinator = FirmwareCoordinator()
        coordinator.reset_firmware_config()

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_upgrade_firmware_4_asics_all_success(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test upgrade_firmware with 4 ASICs all succeeding"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 4
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = [
            "0000:01:10.0", "0000:02:11.0", "0000:03:12.0", "0000:04:13.0"
        ]
        mock_asic_manager.return_value.get_asic_type.return_value = "SPC5"

        mock_managers = []
        for i in range(4):
            mock_manager = MagicMock()
            mock_manager.asic_index = i
            mock_manager.is_alive.return_value = False
            mock_managers.append(mock_manager)

        mock_create_manager.side_effect = mock_managers

        coordinator = FirmwareCoordinator()

        with patch('mellanox_fw_manager.firmware_coordinator.Queue') as mock_queue_class:
            mock_queue_instance = MagicMock()
            mock_queue_class.return_value = mock_queue_instance

            status_data = [
                {
                    'asic_index': i,
                    'status': UpgradeStatusType.SUCCESS.value,
                    'message': f'ASIC {i} upgrade completed',
                    'current_version': '1.0',
                    'available_version': '1.1',
                    'pci_id': f'0000:0{i + 1}:1{i}.0'
                }
                for i in range(4)
            ]
            mock_queue_instance.empty.side_effect = [False, False, False, False, True]
            mock_queue_instance.get_nowait.side_effect = status_data

            coordinator.upgrade_firmware()

            # Verify all 4 managers were started
            for i, manager in enumerate(mock_managers):
                manager.start.assert_called_once()
                self.assertEqual(manager.status_queue, mock_queue_instance)

    @patch('mellanox_fw_manager.firmware_coordinator._detect_platform')
    @patch('mellanox_fw_manager.firmware_coordinator.AsicManager')
    @patch('mellanox_fw_manager.fw_manager.create_firmware_manager')
    def test_upgrade_firmware_4_asics_verify_parallel_start(self, mock_create_manager, mock_asic_manager, mock_detect_platform):
        """Test that all 4 ASIC managers are started in parallel (before any join)"""
        mock_detect_platform.return_value = "test-platform"
        mock_asic_manager.return_value.get_asic_count.return_value = 4
        mock_asic_manager.return_value.get_asic_pci_ids.return_value = [
            "0000:01:10.0", "0000:02:11.0", "0000:03:12.0", "0000:04:13.0"
        ]
        mock_asic_manager.return_value.get_asic_type.return_value = "SPC5"

        start_order = []
        join_order = []

        mock_managers = []
        for i in range(4):
            mock_manager = MagicMock()
            mock_manager.asic_index = i
            mock_manager.is_alive.return_value = False

            # Track when start() and join() are called
            def make_start_tracker(idx):
                def track_start():
                    start_order.append(idx)
                return track_start

            def make_join_tracker(idx):
                def track_join(timeout=None):
                    join_order.append(idx)
                return track_join

            mock_manager.start.side_effect = make_start_tracker(i)
            mock_manager.join.side_effect = make_join_tracker(i)
            mock_managers.append(mock_manager)

        mock_create_manager.side_effect = mock_managers

        coordinator = FirmwareCoordinator()

        with patch('mellanox_fw_manager.firmware_coordinator.Queue') as mock_queue_class:
            mock_queue_instance = MagicMock()
            mock_queue_class.return_value = mock_queue_instance

            status_data = [
                {
                    'asic_index': i,
                    'status': UpgradeStatusType.SUCCESS.value,
                    'message': f'ASIC {i} upgrade completed',
                    'current_version': '1.0',
                    'available_version': '1.1',
                    'pci_id': f'0000:0{i + 1}:1{i}.0'
                }
                for i in range(4)
            ]
            mock_queue_instance.empty.side_effect = [False, False, False, False, True]
            mock_queue_instance.get_nowait.side_effect = status_data

            coordinator.upgrade_firmware()

            # Verify all managers were started BEFORE any joins
            self.assertEqual(start_order, [0, 1, 2, 3], "All processes should start before joining")
            self.assertEqual(join_order, [0, 1, 2, 3], "Processes should join in order")

            # Verify start() called on all before first join
            for manager in mock_managers:
                manager.start.assert_called_once()
                manager.join.assert_called_once()


if __name__ == '__main__':
    unittest.main()
