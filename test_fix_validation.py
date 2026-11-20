#!/usr/bin/env python3
"""
Fix Validation Test - Simplified version without swsscommon dependency
Tests the enhanced fallback functionality to validate the fixes work correctly
"""

import os
import sys
import tempfile
import shutil
import json

# Add the sonic-config-engine path to import the modules
sys.path.insert(0, 'src/sonic-config-engine')

# Mock the swsscommon import to avoid dependency issues
class MockSwssCommon:
    pass

sys.modules['swsscommon'] = MockSwssCommon()
sys.modules['swsscommon.swsscommon'] = MockSwssCommon()

# Now import portconfig
import portconfig

class TestFixValidation:
    """Test class for validating the minigraph HWSKU fallback fixes"""
    
    def setup_method(self):
        """Set up test environment with temporary directories"""
        self.test_dir = tempfile.mkdtemp()
        self.platform_name = "x86_64-arista_7050cx3_32s"
        self.specific_hwsku = "Arista-7050CX3-32S-C32"
        
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
            
    def test_fix_validation_get_hwsku_file_name_fallback(self):
        """
        FIXED CODE TEST: Validate that get_hwsku_file_name now falls back to platform-level file
        
        This test should PASS on fixed code (was failing on unfixed code)
        """
        print(f"\n=== VALIDATION: Testing get_hwsku_file_name fallback fix ===")
        print(f"Platform: {self.platform_name}")
        print(f"HWSKU: {self.specific_hwsku}")
        print(f"Platform dir exists: {os.path.exists(self.platform_dir)}")
        print(f"HWSKU dir exists: {os.path.exists(os.path.join(self.platform_dir, self.specific_hwsku))}")
        print(f"Platform-level hwsku.json exists: {os.path.exists(os.path.join(self.platform_dir, 'hwsku.json'))}")
        
        # This should now work with the fix (was returning None before)
        result = portconfig.get_hwsku_file_name(
            hwsku=self.specific_hwsku, 
            platform=self.platform_name
        )
        
        print(f"Result: {result}")
        
        expected_fallback_path = os.path.join(self.platform_dir, "hwsku.json")
        
        if result == expected_fallback_path:
            print("✅ FIX VALIDATED: Function correctly fell back to platform-level file")
            return True
        elif result is None:
            print("❌ FIX FAILED: Function still returns None (bug not fixed)")
            return False
        else:
            print(f"⚠️  UNEXPECTED: Function returned unexpected path: {result}")
            return False
            
    def test_fix_validation_preservation_existing_hwsku(self):
        """
        PRESERVATION TEST: Validate that existing HWSKU behavior is preserved
        
        This test should PASS on both fixed and unfixed code
        """
        print(f"\n=== VALIDATION: Testing preservation of existing HWSKU behavior ===")
        
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
            print("✅ PRESERVATION VALIDATED: Correctly used HWSKU-specific file when folder exists")
            return True
        else:
            print("❌ REGRESSION: Failed to use HWSKU-specific file when folder exists")
            return False
            
    def test_fix_validation_search_order(self):
        """
        SEARCH ORDER TEST: Validate that the search order is maintained correctly
        """
        print(f"\n=== VALIDATION: Testing search order preservation ===")
        
        # Create platform-level file
        platform_hwsku_path = os.path.join(self.platform_dir, "hwsku.json")
        
        # Test that platform-level fallback works when no HWSKU-specific file exists
        result = portconfig.get_hwsku_file_name(
            hwsku=self.specific_hwsku, 
            platform=self.platform_name
        )
        
        print(f"Search order test result: {result}")
        print(f"Expected platform fallback: {platform_hwsku_path}")
        
        if result == platform_hwsku_path:
            print("✅ SEARCH ORDER VALIDATED: Platform-level fallback works correctly")
            return True
        else:
            print("❌ SEARCH ORDER ISSUE: Platform-level fallback not working")
            return False
            
    def run_all_validation_tests(self):
        """Run all fix validation tests and report results"""
        print("=" * 80)
        print("MINIGRAPH GENERIC HWSKU COMPATIBILITY - FIX VALIDATION TESTS")
        print("=" * 80)
        print(f"Test directory: {self.test_dir}")
        print(f"Platform: {self.platform_name}")
        print(f"Specific HWSKU: {self.specific_hwsku}")
        print("=" * 80)
        
        results = {}
        
        # Test the fixes
        results['fallback_fix'] = self.test_fix_validation_get_hwsku_file_name_fallback()
        results['preservation'] = self.test_fix_validation_preservation_existing_hwsku()
        results['search_order'] = self.test_fix_validation_search_order()
        
        print("\n" + "=" * 80)
        print("FIX VALIDATION RESULTS SUMMARY")
        print("=" * 80)
        
        fixes_working = sum(1 for test in ['fallback_fix', 'search_order'] if results[test])
        preservation_working = results['preservation']
        
        print(f"Fix validation tests: {fixes_working}/2 passed")
        print(f"Preservation test: {'PASS' if preservation_working else 'FAIL'}")
        
        for test_name, passed in results.items():
            status = "✅ PASS" if passed else "❌ FAIL"
            print(f"  {test_name}: {status}")
            
        if fixes_working == 2 and preservation_working:
            print(f"\n🎉 ALL FIXES VALIDATED: The minigraph Generic HWSKU compatibility fix is working correctly!")
            print("✅ Fallback mechanism works when HWSKU-specific folders are missing")
            print("✅ Preservation works when HWSKU-specific folders exist")
            print("✅ Search order is maintained correctly")
        else:
            print(f"\n❌ FIX VALIDATION FAILED: Some issues remain")
            if fixes_working < 2:
                print(f"   - {2 - fixes_working} fix validation test(s) failed")
            if not preservation_working:
                print(f"   - Preservation test failed (regression detected)")
            
        return results

def main():
    """Main function to run the fix validation tests"""
    test_instance = TestFixValidation()
    test_instance.setup_method()
    
    try:
        results = test_instance.run_all_validation_tests()
        return results
    finally:
        test_instance.teardown_method()

if __name__ == "__main__":
    results = main()
    
    # Exit with appropriate code
    fixes_working = sum(1 for test_name in ['fallback_fix', 'search_order'] 
                       if results.get(test_name, False))
    preservation_working = results.get('preservation', False)
    
    if fixes_working == 2 and preservation_working:
        print(f"\nExiting with code 0 - all fixes validated successfully")
        sys.exit(0)
    else:
        print(f"\nExiting with code 1 - fix validation failed")
        sys.exit(1)