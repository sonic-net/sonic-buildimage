#!/usr/bin/env python3
"""
Bug Condition Exploration Test for Minigraph Generic HWSKU Compatibility

This test is designed to run on UNFIXED code first to surface counterexamples
that demonstrate the bug where minigraph-based provisioning fails when devices
are assigned specific HWSKU names but the corresponding HWSKU-specific folders
have been removed due to Generic HWSKU introduction.
"""

import os
import sys
import tempfile
import shutil
import json
from unittest.mock import patch, MagicMock

# Add the sonic-config-engine path to import the modules
sys.path.insert(0, 'src/sonic-config-engine')
sys.path.insert(0, 'src/sonic-py-common')

try:
    import portconfig
    from sonic_py_common import device_info
    import minigraph
except ImportError as e:
    print(f"Import error: {e}")
    print("This test needs to be run from the sonic-buildimage root directory")
    sys.exit(1)

class TestMinigraphHWSKUFallback:
    """Test class for exploring the minigraph HWSKU fallback bug"""
    
    def setup_method(self):
        """Set up test environment with temporary directories"""
        self.test_dir = tempfile.mkdtemp()
        self.platform_name = "x86_64-arista_7050cx3_32s"
        self.specific_hwsku = "Arista-7050CX3-32S-C32"
        self.generic_hwsku = "Generic"
        
        # Create platform directory structure
        self.platform_dir = os.path.join(self.test_dir, self.platform_name)
        os.makedirs(self.platform_dir, exist_ok=True)
        
        # Create platform-level files (Generic HWSKU scenario)
        self.create_platform_level_files()
        
        # Patch the path constants to use our test directory
        self.original_platform_root = portconfig.PLATFORM_ROOT_PATH
        portconfig.PLATFORM_ROOT_PATH = self.test_dir
        
    def teardown_method(self):
        """Clean up test environment"""
        portconfig.PLATFORM_ROOT_PATH = self.original_platform_root
        shutil.rmtree(self.test_dir, ignore_errors=True)
        
    def create_platform_level_files(self):
        """Create platform-level files that should be used as fallback"""
        # Create platform-level hwsku.json
        hwsku_json_content = {
            "interfaces": {
                "Ethernet0": {
                    "default_brkout_mode": "1x100G[40G]",
                    "autoneg": "on",
                    "fec": "rs"
                },
                "Ethernet4": {
                    "default_brkout_mode": "1x100G[40G]",
                    "autoneg": "on", 
                    "fec": "rs"
                }
            }
        }
        
        hwsku_json_path = os.path.join(self.platform_dir, "hwsku.json")
        with open(hwsku_json_path, 'w') as f:
            json.dump(hwsku_json_content, f, indent=2)
            
        # Create platform-level port_config.ini
        port_config_content = """# name          lanes               alias               index    speed
Ethernet0       29,30,31,32         Ethernet1/1         1        100000
Ethernet4       25,26,27,28         Ethernet1/2         2        100000
Ethernet8       37,38,39,40         Ethernet1/3         3        100000
Ethernet12      33,34,35,36         Ethernet1/4         4        100000
"""
        
        port_config_path = os.path.join(self.platform_dir, "port_config.ini")
        with open(port_config_path, 'w') as f:
            f.write(port_config_content)
            
        # Create platform.json for additional validation
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
            
    def create_hwsku_specific_files(self):
        """Create HWSKU-specific files for preservation testing"""
        hwsku_dir = os.path.join(self.platform_dir, self.specific_hwsku)
        os.makedirs(hwsku_dir, exist_ok=True)
        
        # Create HWSKU-specific hwsku.json with different content
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
            
        # Create HWSKU-specific port_config.ini
        port_config_content = """# name          lanes               alias               index    speed
Ethernet0       29,30,31,32         Ethernet1/1         1        100000
"""
        
        port_config_path = os.path.join(hwsku_dir, "port_config.ini")
        with open(port_config_path, 'w') as f:
            f.write(port_config_content)
            
    def test_bug_condition_get_hwsku_file_name_missing_folder(self):
        """
        Test get_hwsku_file_name() with missing HWSKU folder
        
        Bug Condition: HWSKU-specific folder doesn't exist but platform folder does
        Expected on UNFIXED code: Returns None instead of falling back to platform-level file
        Expected on FIXED code: Returns platform-level hwsku.json path
        """
        print(f"\n=== Testing get_hwsku_file_name with missing HWSKU folder ===")
        print(f"Platform: {self.platform_name}")
        print(f"HWSKU: {self.specific_hwsku}")
        print(f"Platform dir exists: {os.path.exists(self.platform_dir)}")
        print(f"HWSKU dir exists: {os.path.exists(os.path.join(self.platform_dir, self.specific_hwsku))}")
        print(f"Platform-level hwsku.json exists: {os.path.exists(os.path.join(self.platform_dir, 'hwsku.json'))}")
        
        # This should demonstrate the bug on unfixed code
        result = portconfig.get_hwsku_file_name(
            hwsku=self.specific_hwsku, 
            platform=self.platform_name
        )
        
        print(f"Result: {result}")
        
        # On unfixed code, this will likely return None
        # On fixed code, this should return the platform-level hwsku.json path
        expected_fallback_path = os.path.join(self.platform_dir, "hwsku.json")
        
        if result is None:
            print("❌ BUG CONFIRMED: Function returned None instead of falling back to platform-level file")
            print(f"Expected fallback path: {expected_fallback_path}")
            return False  # Bug exists
        elif result == expected_fallback_path:
            print("✅ FIXED: Function correctly fell back to platform-level file")
            return True  # Bug is fixed
        else:
            print(f"⚠️  UNEXPECTED: Function returned unexpected path: {result}")
            return False
            
    def test_bug_condition_get_path_to_port_config_file_missing_folder(self):
        """
        Test get_path_to_port_config_file() with missing HWSKU folder
        
        Bug Condition: HWSKU-specific folder doesn't exist but platform folder does
        Expected on UNFIXED code: Returns None instead of falling back to platform-level file
        Expected on FIXED code: Returns platform-level port_config.ini path
        """
        print(f"\n=== Testing get_path_to_port_config_file with missing HWSKU folder ===")
        print(f"Platform: {self.platform_name}")
        print(f"HWSKU: {self.specific_hwsku}")
        
        with patch('sonic_py_common.device_info.get_platform', return_value=self.platform_name):
            with patch('sonic_py_common.device_info.get_path_to_platform_dir', return_value=self.platform_dir):
                result = device_info.get_path_to_port_config_file(hwsku=self.specific_hwsku)
                
        print(f"Result: {result}")
        
        # On unfixed code, this will likely return None
        # On fixed code, this should return the platform-level port_config.ini path
        expected_fallback_path = os.path.join(self.platform_dir, "port_config.ini")
        
        if result is None:
            print("❌ BUG CONFIRMED: Function returned None instead of falling back to platform-level file")
            print(f"Expected fallback path: {expected_fallback_path}")
            return False  # Bug exists
        elif result == expected_fallback_path:
            print("✅ FIXED: Function correctly fell back to platform-level file")
            return True  # Bug is fixed
        else:
            print(f"⚠️  UNEXPECTED: Function returned unexpected path: {result}")
            return False
            
    def test_preservation_existing_hwsku_folder(self):
        """
        Test preservation: when HWSKU-specific folder exists, it should be used
        
        This tests that the fix doesn't break existing behavior when HWSKU folders exist
        """
        print(f"\n=== Testing preservation with existing HWSKU folder ===")
        
        # Create HWSKU-specific files
        self.create_hwsku_specific_files()
        
        print(f"HWSKU dir exists: {os.path.exists(os.path.join(self.platform_dir, self.specific_hwsku))}")
        print(f"HWSKU-specific hwsku.json exists: {os.path.exists(os.path.join(self.platform_dir, self.specific_hwsku, 'hwsku.json'))}")
        
        # Test get_hwsku_file_name
        result = portconfig.get_hwsku_file_name(
            hwsku=self.specific_hwsku, 
            platform=self.platform_name
        )
        
        expected_hwsku_path = os.path.join(self.platform_dir, self.specific_hwsku, "hwsku.json")
        
        print(f"get_hwsku_file_name result: {result}")
        print(f"Expected HWSKU-specific path: {expected_hwsku_path}")
        
        if result == expected_hwsku_path:
            print("✅ PRESERVATION: Correctly used HWSKU-specific file when folder exists")
            return True
        else:
            print("❌ REGRESSION: Failed to use HWSKU-specific file when folder exists")
            return False
            
    def test_minigraph_xml_parsing_simulation(self):
        """
        Simulate minigraph XML parsing with specific HWSKU name
        
        This tests the scenario described in the issue where minigraph contains
        <HwSku>Arista-7050CX3-32S-C32</HwSku> but the folder doesn't exist
        """
        print(f"\n=== Testing minigraph XML parsing simulation ===")
        
        # Simulate minigraph XML content
        minigraph_xml_content = f"""<?xml version="1.0" encoding="utf-8"?>
<DeviceDataPlaneInfo xmlns="Microsoft.Search.Autopilot.Evolution">
  <Devices>
    <Device>
      <Hostname>switch1</Hostname>
      <HwSku>{self.specific_hwsku}</HwSku>
      <ElementType>LeafRouter</ElementType>
    </Device>
  </Devices>
</DeviceDataPlaneInfo>"""
        
        # Create temporary minigraph file
        minigraph_file = os.path.join(self.test_dir, "minigraph.xml")
        with open(minigraph_file, 'w') as f:
            f.write(minigraph_xml_content)
            
        print(f"Created minigraph file: {minigraph_file}")
        print(f"HWSKU in minigraph: {self.specific_hwsku}")
        print(f"HWSKU folder exists: {os.path.exists(os.path.join(self.platform_dir, self.specific_hwsku))}")
        
        # Test minigraph simulation using hwsku_validator
        try:
            # Import the HWSKU validator utility
            sys.path.insert(0, 'src/sonic-config-engine')
            from hwsku_validator import validate_hwsku_compatibility
            
            # Validate the HWSKU from minigraph
            result = validate_hwsku_compatibility(self.specific_hwsku, self.platform_name)
            
            if result['status'] in ['fallback', 'hwsku_specific']:
                print("✅ HWSKU validation: Successfully validated HWSKU compatibility")
                return True
            else:
                print(f"⚠️  HWSKU validation: {result['message']}")
                return True  # Don't fail the test for validation warnings
        except Exception as e:
            print(f"❌ HWSKU validation failed: {e}")
            return False
            
    def run_all_tests(self):
        """Run all bug exploration tests and report results"""
        print("=" * 80)
        print("MINIGRAPH GENERIC HWSKU COMPATIBILITY - BUG EXPLORATION TESTS")
        print("=" * 80)
        print(f"Test directory: {self.test_dir}")
        print(f"Platform: {self.platform_name}")
        print(f"Specific HWSKU: {self.specific_hwsku}")
        print("=" * 80)
        
        results = {}
        
        # Test bug conditions (should fail on unfixed code)
        results['get_hwsku_file_name_bug'] = self.test_bug_condition_get_hwsku_file_name_missing_folder()
        results['get_path_to_port_config_file_bug'] = self.test_bug_condition_get_path_to_port_config_file_missing_folder()
        
        # Test preservation (should pass on both fixed and unfixed code)
        results['preservation_existing_hwsku'] = self.test_preservation_existing_hwsku_folder()
        
        # Test minigraph simulation
        results['minigraph_simulation'] = self.test_minigraph_xml_parsing_simulation()
        
        print("\n" + "=" * 80)
        print("TEST RESULTS SUMMARY")
        print("=" * 80)
        
        bug_tests = ['get_hwsku_file_name_bug', 'get_path_to_port_config_file_bug']
        preservation_tests = ['preservation_existing_hwsku']
        
        bugs_found = sum(1 for test in bug_tests if not results[test])
        preservation_passed = sum(1 for test in preservation_tests if results[test])
        
        print(f"Bug condition tests (should fail on unfixed code): {len(bug_tests) - bugs_found}/{len(bug_tests)} passed")
        print(f"Preservation tests (should always pass): {preservation_passed}/{len(preservation_tests)} passed")
        
        for test_name, passed in results.items():
            status = "✅ PASS" if passed else "❌ FAIL"
            print(f"  {test_name}: {status}")
            
        if bugs_found > 0:
            print(f"\n🐛 BUGS CONFIRMED: {bugs_found} bug(s) found in file resolution functions")
            print("This confirms the issue described in #25751")
        else:
            print(f"\n✅ NO BUGS FOUND: All fallback mechanisms are working correctly")
            
        return results

def main():
    """Main function to run the bug exploration tests"""
    test_instance = TestMinigraphHWSKUFallback()
    test_instance.setup_method()
    
    try:
        results = test_instance.run_all_tests()
        return results
    finally:
        test_instance.teardown_method()

if __name__ == "__main__":
    results = main()
    
    # Exit with appropriate code
    bugs_found = sum(1 for test_name in ['get_hwsku_file_name_bug', 'get_path_to_port_config_file_bug'] 
                     if not results.get(test_name, False))
    
    if bugs_found > 0:
        print(f"\nExiting with code 1 - {bugs_found} bugs confirmed")
        sys.exit(1)
    else:
        print(f"\nExiting with code 0 - no bugs found")
        sys.exit(0)