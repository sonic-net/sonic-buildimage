#!/usr/bin/env python3
"""
Property-based tests for minigraph HWSKU fallback functionality
Uses hypothesis to generate random test cases and verify fallback behavior
"""

import os
import sys
import tempfile
import shutil
import json
import unittest
from unittest.mock import patch
import string
import random

# Add paths to import the modules
sys.path.insert(0, 'src/sonic-config-engine')
sys.path.insert(0, 'src/sonic-py-common')

try:
    import portconfig
    from sonic_py_common import device_info
except ImportError as e:
    print(f"Import error: {e}")
    print("This test needs to be run from the sonic-buildimage root directory")
    sys.exit(1)

# Try to import hypothesis for property-based testing
try:
    from hypothesis import given, strategies as st, assume, settings
    HYPOTHESIS_AVAILABLE = True
except ImportError:
    HYPOTHESIS_AVAILABLE = False
    print("Warning: hypothesis not available, property-based tests will be skipped")

class TestMinigraphHWSKUProperties(unittest.TestCase):
    """Property-based tests for minigraph HWSKU fallback functionality"""
    
    def setUp(self):
        """Set up test environment"""
        self.test_dir = tempfile.mkdtemp()
        self.original_platform_root = portconfig.PLATFORM_ROOT_PATH
        portconfig.PLATFORM_ROOT_PATH = self.test_dir
        
    def tearDown(self):
        """Clean up test environment"""
        portconfig.PLATFORM_ROOT_PATH = self.original_platform_root
        shutil.rmtree(self.test_dir, ignore_errors=True)
        
    def create_test_files(self, platform, hwsku, create_hwsku_specific=True, create_platform_level=True):
        """Helper to create test files for given platform and hwsku"""
        platform_dir = os.path.join(self.test_dir, platform)
        os.makedirs(platform_dir, exist_ok=True)
        
        hwsku_file_path = None
        platform_file_path = None
        
        if create_platform_level:
            # Create platform-level hwsku.json
            platform_file_path = os.path.join(platform_dir, "hwsku.json")
            with open(platform_file_path, 'w') as f:
                json.dump({"platform_level": True, "platform": platform}, f)
                
        if create_hwsku_specific:
            # Create HWSKU-specific hwsku.json
            hwsku_dir = os.path.join(platform_dir, hwsku)
            os.makedirs(hwsku_dir, exist_ok=True)
            hwsku_file_path = os.path.join(hwsku_dir, "hwsku.json")
            with open(hwsku_file_path, 'w') as f:
                json.dump({"hwsku_specific": True, "hwsku": hwsku, "platform": platform}, f)
                
        return hwsku_file_path, platform_file_path

    @unittest.skipUnless(HYPOTHESIS_AVAILABLE, "hypothesis not available")
    @given(
        platform=st.text(alphabet=string.ascii_letters + string.digits + '-_', min_size=5, max_size=30),
        hwsku=st.text(alphabet=string.ascii_letters + string.digits + '-_', min_size=5, max_size=30)
    )
    @settings(max_examples=50, deadline=5000)
    def test_property_fallback_behavior(self, platform, hwsku):
        """Property: When HWSKU-specific file doesn't exist but platform-level does, fallback should work"""
        # Assume valid platform and hwsku names (no empty strings, no special chars that break paths)
        assume(platform and hwsku)
        assume('/' not in platform and '/' not in hwsku)
        assume('\\' not in platform and '\\' not in hwsku)
        assume(platform != hwsku)  # Avoid confusion
        
        # Create only platform-level file (no HWSKU-specific)
        _, platform_file_path = self.create_test_files(
            platform, hwsku, 
            create_hwsku_specific=False, 
            create_platform_level=True
        )
        
        # Test the fallback behavior
        result = portconfig.get_hwsku_file_name(hwsku=hwsku, platform=platform)
        
        # Property: Should return platform-level file when HWSKU-specific doesn't exist
        self.assertEqual(result, platform_file_path, 
                        f"Failed fallback for platform={platform}, hwsku={hwsku}")

    @unittest.skipUnless(HYPOTHESIS_AVAILABLE, "hypothesis not available")
    @given(
        platform=st.text(alphabet=string.ascii_letters + string.digits + '-_', min_size=5, max_size=30),
        hwsku=st.text(alphabet=string.ascii_letters + string.digits + '-_', min_size=5, max_size=30)
    )
    @settings(max_examples=50, deadline=5000)
    def test_property_preservation_behavior(self, platform, hwsku):
        """Property: When HWSKU-specific file exists, it should be used (preservation)"""
        # Assume valid platform and hwsku names
        assume(platform and hwsku)
        assume('/' not in platform and '/' not in hwsku)
        assume('\\' not in platform and '\\' not in hwsku)
        assume(platform != hwsku)
        
        # Create both HWSKU-specific and platform-level files
        hwsku_file_path, _ = self.create_test_files(
            platform, hwsku, 
            create_hwsku_specific=True, 
            create_platform_level=True
        )
        
        # Test the preservation behavior
        result = portconfig.get_hwsku_file_name(hwsku=hwsku, platform=platform)
        
        # Property: Should return HWSKU-specific file when it exists (preservation)
        self.assertEqual(result, hwsku_file_path, 
                        f"Failed preservation for platform={platform}, hwsku={hwsku}")

    @unittest.skipUnless(HYPOTHESIS_AVAILABLE, "hypothesis not available")
    @given(
        platform=st.text(alphabet=string.ascii_letters + string.digits + '-_', min_size=5, max_size=30),
        hwsku=st.text(alphabet=string.ascii_letters + string.digits + '-_', min_size=5, max_size=30)
    )
    @settings(max_examples=50, deadline=5000)
    def test_property_no_files_behavior(self, platform, hwsku):
        """Property: When no files exist, should return None"""
        # Assume valid platform and hwsku names
        assume(platform and hwsku)
        assume('/' not in platform and '/' not in hwsku)
        assume('\\' not in platform and '\\' not in hwsku)
        
        # Create platform directory but no files
        platform_dir = os.path.join(self.test_dir, platform)
        os.makedirs(platform_dir, exist_ok=True)
        
        # Test behavior when no files exist
        result = portconfig.get_hwsku_file_name(hwsku=hwsku, platform=platform)
        
        # Property: Should return None when no relevant files exist
        self.assertIsNone(result, 
                         f"Should return None when no files exist for platform={platform}, hwsku={hwsku}")

    @unittest.skipUnless(HYPOTHESIS_AVAILABLE, "hypothesis not available")
    @given(
        platforms=st.lists(
            st.text(alphabet=string.ascii_letters + string.digits + '-_', min_size=5, max_size=20),
            min_size=2, max_size=5, unique=True
        ),
        hwskus=st.lists(
            st.text(alphabet=string.ascii_letters + string.digits + '-_', min_size=5, max_size=20),
            min_size=2, max_size=5, unique=True
        )
    )
    @settings(max_examples=20, deadline=10000)
    def test_property_consistency_across_platforms(self, platforms, hwskus):
        """Property: Fallback behavior should be consistent across different platforms"""
        # Assume valid names
        assume(all(p and '/' not in p and '\\' not in p for p in platforms))
        assume(all(h and '/' not in h and '\\' not in h for h in hwskus))
        
        results = {}
        
        for platform in platforms:
            for hwsku in hwskus:
                # Create only platform-level files for consistent testing
                _, platform_file_path = self.create_test_files(
                    platform, hwsku,
                    create_hwsku_specific=False,
                    create_platform_level=True
                )
                
                result = portconfig.get_hwsku_file_name(hwsku=hwsku, platform=platform)
                results[(platform, hwsku)] = (result, platform_file_path)
        
        # Property: All results should follow the same pattern (fallback to platform-level)
        for (platform, hwsku), (result, expected_path) in results.items():
            self.assertEqual(result, expected_path,
                           f"Inconsistent behavior for platform={platform}, hwsku={hwsku}")

    @unittest.skipUnless(HYPOTHESIS_AVAILABLE, "hypothesis not available")
    @patch('sonic_py_common.device_info.get_platform')
    @patch('sonic_py_common.device_info.get_path_to_platform_dir')
    @given(
        platform=st.text(alphabet=string.ascii_letters + string.digits + '-_', min_size=5, max_size=30),
        hwsku=st.text(alphabet=string.ascii_letters + string.digits + '-_', min_size=5, max_size=30)
    )
    @settings(max_examples=30, deadline=5000)
    def test_property_device_info_fallback(self, mock_get_platform_dir, mock_get_platform, platform, hwsku):
        """Property: device_info.get_path_to_port_config_file should also follow fallback behavior"""
        # Assume valid names
        assume(platform and hwsku)
        assume('/' not in platform and '/' not in hwsku)
        assume('\\' not in platform and '\\' not in hwsku)
        
        # Setup mocks
        platform_dir = os.path.join(self.test_dir, platform)
        os.makedirs(platform_dir, exist_ok=True)
        mock_get_platform.return_value = platform
        mock_get_platform_dir.return_value = platform_dir
        
        # Create only platform-level port_config.ini
        platform_port_config = os.path.join(platform_dir, "port_config.ini")
        with open(platform_port_config, 'w') as f:
            f.write("# Platform-level port config\n")
        
        # Test device_info fallback
        result = device_info.get_path_to_port_config_file(hwsku=hwsku)
        
        # Property: Should return platform-level file when HWSKU-specific doesn't exist
        self.assertEqual(result, platform_port_config,
                        f"device_info fallback failed for platform={platform}, hwsku={hwsku}")

    def test_manual_edge_cases(self):
        """Test specific edge cases that might not be covered by property-based tests"""
        # Test with empty strings (should be handled gracefully)
        result = portconfig.get_hwsku_file_name(hwsku="", platform="test-platform")
        # Should not crash, result can be None or a path
        
        # Test with None values
        result = portconfig.get_hwsku_file_name(hwsku=None, platform="test-platform")
        # Should not crash
        
        result = portconfig.get_hwsku_file_name(hwsku="test-hwsku", platform=None)
        # Should not crash
        
        # Test with very long names
        long_name = "a" * 255
        platform_dir = os.path.join(self.test_dir, "test-platform")
        os.makedirs(platform_dir, exist_ok=True)
        
        result = portconfig.get_hwsku_file_name(hwsku=long_name, platform="test-platform")
        # Should handle long names gracefully
        
    def test_real_world_scenarios(self):
        """Test with realistic platform and HWSKU names from actual SONiC deployments"""
        real_scenarios = [
            ("x86_64-arista_7050cx3_32s", "Arista-7050CX3-32S-C32"),
            ("x86_64-dell_s6000_s1220", "Dell-S6000-S1220"),
            ("x86_64-mlnx_msn2700", "Mellanox-SN2700"),
            ("arm64-nokia_ixr7250e_36x400g", "Nokia-IXR7250E-36x400G"),
            ("x86_64-broadcom_td3", "Generic"),
        ]
        
        for platform, hwsku in real_scenarios:
            # Test fallback scenario
            _, platform_file_path = self.create_test_files(
                platform, hwsku,
                create_hwsku_specific=False,
                create_platform_level=True
            )
            
            result = portconfig.get_hwsku_file_name(hwsku=hwsku, platform=platform)
            if hwsku != "Generic":
                # Should fallback to platform-level for specific HWSKUs
                self.assertEqual(result, platform_file_path,
                               f"Real-world fallback failed for {platform}/{hwsku}")
            
            # Test preservation scenario
            hwsku_file_path, _ = self.create_test_files(
                platform, hwsku,
                create_hwsku_specific=True,
                create_platform_level=True
            )
            
            result = portconfig.get_hwsku_file_name(hwsku=hwsku, platform=platform)
            # Should use HWSKU-specific when available
            self.assertEqual(result, hwsku_file_path,
                           f"Real-world preservation failed for {platform}/{hwsku}")

if __name__ == '__main__':
    unittest.main()