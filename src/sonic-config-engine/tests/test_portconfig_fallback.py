#!/usr/bin/env python3
"""
Unit tests for portconfig.py fallback functionality
Tests the enhanced get_hwsku_file_name() function with Generic HWSKU compatibility
"""

import os
import sys
import tempfile
import shutil
import json
import unittest
from unittest.mock import patch

# Add the parent directory to the path to import portconfig
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import portconfig

class TestPortConfigFallback(unittest.TestCase):
    """Test class for portconfig fallback functionality"""
    
    def setUp(self):
        """Set up test environment with temporary directories"""
        self.test_dir = tempfile.mkdtemp()
        self.platform_name = "x86_64-arista_7050cx3_32s"
        self.specific_hwsku = "Arista-7050CX3-32S-C32"
        
        # Create platform directory structure
        self.platform_dir = os.path.join(self.test_dir, self.platform_name)
        os.makedirs(self.platform_dir, exist_ok=True)
        
        # Patch the path constants to use our test directory
        self.original_platform_root = portconfig.PLATFORM_ROOT_PATH
        portconfig.PLATFORM_ROOT_PATH = self.test_dir
        
    def tearDown(self):
        """Clean up test environment"""
        portconfig.PLATFORM_ROOT_PATH = self.original_platform_root
        shutil.rmtree(self.test_dir, ignore_errors=True)
        
    def create_platform_level_hwsku_json(self):
        """Create platform-level hwsku.json file"""
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
        return hwsku_json_path
        
    def create_hwsku_specific_hwsku_json(self):
        """Create HWSKU-specific hwsku.json file"""
        hwsku_dir = os.path.join(self.platform_dir, self.specific_hwsku)
        os.makedirs(hwsku_dir, exist_ok=True)
        
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
        return hwsku_json_path
        
    def test_fallback_to_platform_level_when_hwsku_missing(self):
        """Test that get_hwsku_file_name falls back to platform-level when HWSKU-specific is missing"""
        # Create only platform-level file
        platform_hwsku_path = self.create_platform_level_hwsku_json()
        
        # Call function with specific HWSKU that doesn't have its own folder
        result = portconfig.get_hwsku_file_name(
            hwsku=self.specific_hwsku, 
            platform=self.platform_name
        )
        
        # Should return platform-level file as fallback
        self.assertEqual(result, platform_hwsku_path)
        
    def test_uses_hwsku_specific_when_available(self):
        """Test that get_hwsku_file_name uses HWSKU-specific file when available (preservation)"""
        # Create both platform-level and HWSKU-specific files
        self.create_platform_level_hwsku_json()
        hwsku_specific_path = self.create_hwsku_specific_hwsku_json()
        
        # Call function with specific HWSKU that has its own folder
        result = portconfig.get_hwsku_file_name(
            hwsku=self.specific_hwsku, 
            platform=self.platform_name
        )
        
        # Should return HWSKU-specific file (preservation)
        self.assertEqual(result, hwsku_specific_path)
        
    def test_returns_none_when_no_files_exist(self):
        """Test that get_hwsku_file_name returns None when no files exist"""
        # Don't create any files
        
        # Call function with specific HWSKU
        result = portconfig.get_hwsku_file_name(
            hwsku=self.specific_hwsku, 
            platform=self.platform_name
        )
        
        # Should return None when no files exist
        self.assertIsNone(result)
        
    def test_works_without_hwsku_parameter(self):
        """Test that get_hwsku_file_name works without hwsku parameter (existing behavior)"""
        # Create platform-level file
        self.create_platform_level_hwsku_json()
        
        # Call function without hwsku parameter
        result = portconfig.get_hwsku_file_name(platform=self.platform_name)
        
        # Should check other paths and potentially return None or other files
        # This tests that we didn't break existing behavior
        self.assertIsNotNone(result)  # May find files in other standard locations
        
    def test_works_without_platform_parameter(self):
        """Test that get_hwsku_file_name works without platform parameter (existing behavior)"""
        # Call function without platform parameter
        result = portconfig.get_hwsku_file_name(hwsku=self.specific_hwsku)
        
        # Should check other paths and potentially return None
        # This tests that we didn't break existing behavior
        # Result can be None or a path from other standard locations
        pass  # Just ensure no exceptions are raised
        
    def test_fallback_search_order(self):
        """Test that fallback maintains proper search order"""
        # Create platform-level file
        platform_hwsku_path = self.create_platform_level_hwsku_json()
        
        # Create a file in HWSKU_ROOT_PATH to test search order
        hwsku_root_dir = os.path.join(self.test_dir, "..", "hwsku")
        os.makedirs(hwsku_root_dir, exist_ok=True)
        hwsku_root_path = os.path.join(hwsku_root_dir, "hwsku.json")
        with open(hwsku_root_path, 'w') as f:
            json.dump({"test": "hwsku_root"}, f)
            
        # Temporarily patch HWSKU_ROOT_PATH
        original_hwsku_root = portconfig.HWSKU_ROOT_PATH
        portconfig.HWSKU_ROOT_PATH = hwsku_root_dir
        
        try:
            result = portconfig.get_hwsku_file_name(
                hwsku=self.specific_hwsku, 
                platform=self.platform_name
            )
            
            # Should return the first file found in search order
            # HWSKU_ROOT_PATH comes first, so it should be returned
            self.assertEqual(result, hwsku_root_path)
        finally:
            portconfig.HWSKU_ROOT_PATH = original_hwsku_root
            shutil.rmtree(hwsku_root_dir, ignore_errors=True)

if __name__ == '__main__':
    unittest.main()