#!/usr/bin/env python3
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
Unit tests for AsicManager class
"""

import os
import tempfile
import unittest
from unittest.mock import patch, mock_open
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from mellanox_fw_manager.asic_manager import AsicManager

class TestAsicManager(unittest.TestCase):
    """Test cases for AsicManager class"""

    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()
        self.platform = "test-platform"
        self.asic_manager = AsicManager(self.platform)

    def tearDown(self):
        """Clean up test fixtures"""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_init_sets_platform(self):
        """Test that __init__ sets the platform correctly"""
        self.assertEqual(self.asic_manager.platform, self.platform)

    def test_init_sets_asic_conf_file(self):
        """Test that __init__ sets the asic.conf file path correctly"""
        expected_path = f"/usr/share/sonic/device/{self.platform}/asic.conf"
        self.assertEqual(self.asic_manager.asic_conf_file, expected_path)

    @patch('mellanox_fw_manager.asic_manager.os.path.exists')
    def test_load_asic_data_file_not_exists(self, mock_exists):
        """Test loading ASIC data when asic.conf file doesn't exist"""
        mock_exists.return_value = False

        asic_manager = AsicManager(self.platform)

        self.assertEqual(asic_manager._asic_count, 1)
        self.assertEqual(asic_manager._pci_ids, [])

    def test_load_asic_data_with_file(self):
        """Test loading ASIC data from asic.conf file"""
        import tempfile
        import os

        with tempfile.NamedTemporaryFile(mode='w', delete=False) as f:
            f.write("NUM_ASIC=3\n")
            f.write("DEV_ID_ASIC_0=01:00.0\n")
            f.write("DEV_ID_ASIC_1=02:00.0\n")
            f.write("DEV_ID_ASIC_2=03:00.0\n")
            temp_file = f.name

        try:
            asic_manager = AsicManager(self.platform)
            asic_manager.asic_conf_file = temp_file
            asic_manager._load_asic_data()

            self.assertEqual(asic_manager._asic_count, 3)
            self.assertEqual(asic_manager._pci_ids, ["01:00.0", "02:00.0", "03:00.0"])
        finally:
            os.unlink(temp_file)

    @patch('mellanox_fw_manager.asic_manager.os.path.exists')
    @patch('builtins.open', new_callable=mock_open)
    def test_load_asic_data_invalid_file(self, mock_file, mock_exists):
        """Test loading ASIC data with invalid file content"""
        mock_exists.return_value = True
        mock_file.return_value.__enter__.return_value = mock_file.return_value
        mock_file.return_value.__iter__.return_value = iter(["invalid content"])

        asic_manager = AsicManager(self.platform)

        self.assertEqual(asic_manager._asic_count, 1)
        self.assertEqual(asic_manager._pci_ids, ["unknown"])

    def test_get_asic_count(self):
        """Test getting ASIC count"""
        self.asic_manager._asic_count = 4
        result = self.asic_manager.get_asic_count()
        self.assertEqual(result, 4)

    def test_get_asic_pci_ids(self):
        """Test getting ASIC PCI IDs"""
        test_pci_ids = ["01:00.0", "02:00.0", "03:00.0"]
        self.asic_manager._pci_ids = test_pci_ids
        result = self.asic_manager.get_asic_pci_ids()
        self.assertEqual(result, test_pci_ids)

    @patch('mellanox_fw_manager.asic_manager.os.path.exists')
    def test_load_asic_data_partial_data(self, mock_exists):
        """Test loading ASIC data with partial data (NUM_ASIC but no DEV_ID)"""
        mock_exists.return_value = True
        content = "NUM_ASIC=2\n# No DEV_ID lines"
        with patch('builtins.open', mock_open(read_data=content)):
            from mellanox_fw_manager.asic_manager import AsicManager
            manager = AsicManager("test-platform")

            self.assertEqual(manager._asic_count, 2)
            self.assertEqual(manager._pci_ids, ["unknown", "unknown"])

    @patch('mellanox_fw_manager.asic_manager.os.path.exists')
    def test_load_asic_data_missing_num_asic(self, mock_exists):
        """Test loading ASIC data without NUM_ASIC line"""
        mock_exists.return_value = True
        content = "DEV_ID_ASIC_0=01:00.0\nDEV_ID_ASIC_1=02:00.0\n"
        with patch('builtins.open', mock_open(read_data=content)):
            from mellanox_fw_manager.asic_manager import AsicManager
            manager = AsicManager("test-platform")

            self.assertEqual(manager._asic_count, 1)
            self.assertEqual(manager._pci_ids, ["01:00.0"])

    @patch('mellanox_fw_manager.asic_manager.os.path.exists')
    def test_load_asic_data_malformed_num_asic(self, mock_exists):
        """Test loading ASIC data with malformed NUM_ASIC line"""
        mock_exists.return_value = True
        content = "NUM_ASIC=invalid\nDEV_ID_ASIC_0=01:00.0\n"
        with patch('builtins.open', mock_open(read_data=content)):
            from mellanox_fw_manager.asic_manager import AsicManager
            manager = AsicManager("test-platform")

            self.assertEqual(manager._asic_count, 1)
            self.assertEqual(manager._pci_ids, [])

    @patch('mellanox_fw_manager.asic_manager.os.path.exists')
    def test_load_asic_data_malformed_dev_id(self, mock_exists):
        """Test loading ASIC data with malformed DEV_ID lines"""
        mock_exists.return_value = True
        content = "NUM_ASIC=2\nDEV_ID_ASIC_0=01:00.0\nDEV_ID_ASIC_1=invalid_format\n"
        with patch('builtins.open', mock_open(read_data=content)):
            from mellanox_fw_manager.asic_manager import AsicManager
            manager = AsicManager("test-platform")

            self.assertEqual(manager._asic_count, 2)
            self.assertEqual(manager._pci_ids, ["01:00.0", "invalid_format"])

    @patch('mellanox_fw_manager.asic_manager.os.path.exists')
    def test_load_asic_data_negative_asic_count(self, mock_exists):
        """Test loading ASIC data with negative ASIC count"""
        mock_exists.return_value = True
        content = "NUM_ASIC=-1\n"
        with patch('builtins.open', mock_open(read_data=content)):
            from mellanox_fw_manager.asic_manager import AsicManager
            manager = AsicManager("test-platform")

            self.assertEqual(manager._asic_count, -1)
            self.assertEqual(manager._pci_ids, [])

    @patch('mellanox_fw_manager.asic_manager.os.path.exists')
    def test_load_asic_data_large_asic_count(self, mock_exists):
        """Test loading ASIC data with very large ASIC count"""
        mock_exists.return_value = True
        content = "NUM_ASIC=1000\n"
        with patch('builtins.open', mock_open(read_data=content)):
            from mellanox_fw_manager.asic_manager import AsicManager
            manager = AsicManager("test-platform")

            self.assertEqual(manager._asic_count, 1000)
            self.assertEqual(len(manager._pci_ids), 1000)
            self.assertTrue(all(pci_id == "unknown" for pci_id in manager._pci_ids))

    @patch('mellanox_fw_manager.asic_manager.os.path.exists')
    def test_load_asic_data_file_read_error(self, mock_exists):
        """Test loading ASIC data when file read fails"""
        mock_exists.return_value = True
        with patch('builtins.open', side_effect=IOError("Permission denied")):
            from mellanox_fw_manager.asic_manager import AsicManager
            manager = AsicManager("test-platform")

            self.assertEqual(manager._asic_count, 1)
            self.assertEqual(manager._pci_ids, [])

    @patch('mellanox_fw_manager.asic_manager.os.path.exists')
    def test_load_asic_data_with_comments_and_whitespace(self, mock_exists):
        """Test loading ASIC data with comments and whitespace"""
        mock_exists.return_value = True
        content = """# This is a comment
NUM_ASIC=2
# Another comment
DEV_ID_ASIC_0=01:00.0
DEV_ID_ASIC_1=02:00.0
# End comment
"""
        with patch('builtins.open', mock_open(read_data=content)):
            from mellanox_fw_manager.asic_manager import AsicManager
            manager = AsicManager("test-platform")

            self.assertEqual(manager._asic_count, 2)
            self.assertEqual(manager._pci_ids, ["01:00.0", "02:00.0"])

if __name__ == '__main__':
    unittest.main()
