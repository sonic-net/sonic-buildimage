#!/usr/bin/env python3
"""
Unit tests for device_info.py fallback functionality
Tests the enhanced get_path_to_port_config_file() function with Generic HWSKU compatibility
"""

import os
import sys
import tempfile
import shutil
import json
import unittest
from unittest.mock import patch, MagicMock

# Add the parent directory to the path to import device_info
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from sonic_py_common import device_info

class TestDeviceInfoFallback(unittest.TestCase):
    """Test class for device_info fallback functionality"""
    
    def setUp(self):
        """Set up test environment with temporary directories"""
        self.test_dir = tempfile.mkdtemp()
        self.platform_name = "x86_64-arista_7050cx3_32s"
        self.specific_hwsku = "Arista-7050CX3-32S-C32"
        
        # Create platform directory structure
        self.platform_dir = os.path.join(self.test_dir, self.platform_name)
        os.makedirs(self.platform_dir, exist_ok=True)
        
    def tearDown(self):
        """Clean up test environment"""
        shutil.rmtree(self.test_dir, ignore_errors=True)
        
    def create_platform_level_files(self):
        """Create platform-level port configuration files"""
        # Create platform-level port_config.ini
        port_config_content = """# name          lanes               alias               index    speed
Ethernet0       29,30,31,32         Ethernet1/1         1        100000
Ethernet4       25,26,27,28         Ethernet1/2         2        100000
"""
        
        port_config_path = os.path.join(self.platform_dir, "port_config.ini")
        with open(port_config_path, 'w') as f:
            f.write(port_config_content)
            
        # Create platform-level hwsku.json
        hwsku_json_content = {
            "interfaces": {
                "Ethernet0": {
                    "default_brkout_mode": "1x100G[40G]",
                    "autoneg": "on",
                    "fec": "rs"
                }
            }
        }
        
        hwsku_json_path = os.path.join(self.platform_dir, "hwsku.json")
        with open(hwsku_json_path, 'w') as f:
            json.dump(hwsku_json_content, f, indent=2)
            
        # Create platform.json for validation
        platform_json_content = {
            "interfaces": {
                "Ethernet0": {
                    "index": "1,1,1,1",
                    "lanes": "29,30,31,32",
                    "breakout_modes": {
                        "1x100G[40G]": ["Ethernet0"]
                    }
                }
            }
        }
        
        platform_json_path = os.path.join(self.platform_dir, "platform.json")
        with open(platform_json_path, 'w') as f:
            json.dump(platform_json_content, f, indent=2)
            
        return port_config_path, hwsku_json_path, platform_json_path
        
    def create_hwsku_specific_files(self):
        """Create HWSKU-specific port configuration files"""
        hwsku_dir = os.path.join(self.platform_dir, self.specific_hwsku)
        os.makedirs(hwsku_dir, exist_ok=True)
        
        # Create HWSKU-specific port_config.ini
        port_config_content = """# name          lanes               alias               index    speed
Ethernet0       29,30,31,32         Ethernet1/1         1        100000
"""
        
        port_config_path = os.path.join(hwsku_dir, "port_config.ini")
        with open(port_config_path, 'w') as f:
            f.write(port_config_content)
            
        # Create HWSKU-specific hwsku.json
        hwsku_json_content = {
            "interfaces": {
                "Ethernet0": {
                    "default_brkout_mode": "1x100G",
                    "autoneg": "off",
                    "fec": "none"
                }
            }
        }
        
        hwsku_json_path = os.path.join(hwsku_dir, "hwsku.json")
        with open(hwsku_json_path, 'w') as f:
            json.dump(hwsku_json_content, f, indent=2)
            
        return port_config_path, hwsku_json_path
        
    @patch('sonic_py_common.device_info.get_platform')
    @patch('sonic_py_common.device_info.get_path_to_platform_dir')
    def test_fallback_to_platform_level_when_hwsku_missing(self, mock_get_platform_dir, mock_get_platform):
        """Test that get_path_to_port_config_file falls back to platform-level when HWSKU-specific is missing"""
        # Setup mocks
        mock_get_platform.return_value = self.platform_name
        mock_get_platform_dir.return_value = self.platform_dir
        
        # Create only platform-level files
        platform_port_config_path, _, _ = self.create_platform_level_files()
        
        # Call function with specific HWSKU that doesn't have its own folder
        result = device_info.get_path_to_port_config_file(hwsku=self.specific_hwsku)
        
        # Should return platform-level file as fallback
        self.assertEqual(result, platform_port_config_path)
        
    @patch('sonic_py_common.device_info.get_platform')
    @patch('sonic_py_common.device_info.get_path_to_platform_dir')
    def test_uses_hwsku_specific_when_available(self, mock_get_platform_dir, mock_get_platform):
        """Test that get_path_to_port_config_file uses HWSKU-specific file when available (preservation)"""
        # Setup mocks
        mock_get_platform.return_value = self.platform_name
        mock_get_platform_dir.return_value = self.platform_dir
        
        # Create both platform-level and HWSKU-specific files
        self.create_platform_level_files()
        hwsku_port_config_path, _ = self.create_hwsku_specific_files()
        
        # Call function with specific HWSKU that has its own folder
        result = device_info.get_path_to_port_config_file(hwsku=self.specific_hwsku)
        
        # Should return HWSKU-specific file (preservation)
        self.assertEqual(result, hwsku_port_config_path)
        
    @patch('sonic_py_common.device_info.get_platform')
    @patch('sonic_py_common.device_info.get_path_to_platform_dir')
    def test_returns_none_when_no_files_exist(self, mock_get_platform_dir, mock_get_platform):
        """Test that get_path_to_port_config_file returns None when no files exist"""
        # Setup mocks
        mock_get_platform.return_value = self.platform_name
        mock_get_platform_dir.return_value = self.platform_dir
        
        # Don't create any files
        
        # Call function with specific HWSKU
        result = device_info.get_path_to_port_config_file(hwsku=self.specific_hwsku)
        
        # Should return None when no files exist
        self.assertIsNone(result)
        
    @patch('sonic_py_common.device_info.get_platform')
    def test_returns_none_when_no_platform(self, mock_get_platform):
        """Test that get_path_to_port_config_file returns None when platform is None"""
        # Setup mock to return None platform
        mock_get_platform.return_value = None
        
        # Call function
        result = device_info.get_path_to_port_config_file(hwsku=self.specific_hwsku)
        
        # Should return None when platform is None
        self.assertIsNone(result)
        
    @patch('sonic_py_common.device_info.get_platform')
    @patch('sonic_py_common.device_info.get_path_to_platform_dir')
    def test_platform_json_fallback_with_hwsku_json(self, mock_get_platform_dir, mock_get_platform):
        """Test that platform.json is used as fallback when platform-level hwsku.json exists"""
        # Setup mocks
        mock_get_platform.return_value = self.platform_name
        mock_get_platform_dir.return_value = self.platform_dir
        
        # Create platform-level files including platform.json
        _, _, platform_json_path = self.create_platform_level_files()
        
        # Call function with specific HWSKU that doesn't have its own folder
        result = device_info.get_path_to_port_config_file(hwsku=self.specific_hwsku)
        
        # Should return platform.json when platform-level hwsku.json exists and platform.json has interfaces
        self.assertEqual(result, platform_json_path)
        
    @patch('sonic_py_common.device_info.get_platform')
    @patch('sonic_py_common.device_info.get_paths_to_platform_and_hwsku_dirs')
    def test_works_without_hwsku_parameter(self, mock_get_paths, mock_get_platform):
        """Test that get_path_to_port_config_file works without hwsku parameter (existing behavior)"""
        # Setup mocks
        mock_get_platform.return_value = self.platform_name
        hwsku_dir = os.path.join(self.platform_dir, "Generic")
        os.makedirs(hwsku_dir, exist_ok=True)
        mock_get_paths.return_value = (self.platform_dir, hwsku_dir)
        
        # Create some files in the hwsku directory
        port_config_path = os.path.join(hwsku_dir, "port_config.ini")
        with open(port_config_path, 'w') as f:
            f.write("# test port config\n")
        
        # Call function without hwsku parameter
        result = device_info.get_path_to_port_config_file()
        
        # Should work without hwsku parameter (existing behavior)
        self.assertEqual(result, port_config_path)
        
    @patch('sonic_py_common.device_info.get_platform')
    @patch('sonic_py_common.device_info.get_path_to_platform_dir')
    def test_asic_specific_port_config(self, mock_get_platform_dir, mock_get_platform):
        """Test that ASIC-specific port config is handled correctly"""
        # Setup mocks
        mock_get_platform.return_value = self.platform_name
        mock_get_platform_dir.return_value = self.platform_dir
        
        # Create HWSKU-specific directory with ASIC-specific port config
        hwsku_dir = os.path.join(self.platform_dir, self.specific_hwsku)
        asic_dir = os.path.join(hwsku_dir, "asic0")
        os.makedirs(asic_dir, exist_ok=True)
        
        asic_port_config_path = os.path.join(asic_dir, "port_config.ini")
        with open(asic_port_config_path, 'w') as f:
            f.write("# ASIC-specific port config\n")
        
        # Call function with ASIC parameter
        result = device_info.get_path_to_port_config_file(hwsku=self.specific_hwsku, asic="asic0")
        
        # Should return ASIC-specific file
        self.assertEqual(result, asic_port_config_path)

if __name__ == '__main__':
    unittest.main()