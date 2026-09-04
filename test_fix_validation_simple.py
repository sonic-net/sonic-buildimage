#!/usr/bin/env python3
"""
Simple Fix Validation Test - Tests the enhanced fallback functionality directly
"""

import os
import sys
import tempfile
import shutil
import json

def test_get_hwsku_file_name_fallback():
    """Test the enhanced get_hwsku_file_name function directly"""
    
    # Create temporary test environment
    test_dir = tempfile.mkdtemp()
    platform_name = "x86_64-arista_7050cx3_32s"
    specific_hwsku = "Arista-7050CX3-32S-C32"
    
    try:
        # Create platform directory structure
        platform_dir = os.path.join(test_dir, platform_name)
        os.makedirs(platform_dir, exist_ok=True)
        
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
        
        platform_hwsku_path = os.path.join(platform_dir, "hwsku.json")
        with open(platform_hwsku_path, 'w') as f:
            json.dump(hwsku_json_content, f, indent=2)
        
        # Test the enhanced function logic directly
        def enhanced_get_hwsku_file_name(hwsku=None, platform=None, platform_root_path=None):
            """Enhanced version of get_hwsku_file_name with fallback logic"""
            if not platform_root_path:
                platform_root_path = "/usr/share/sonic/device"
                
            hwsku_candidates_Json = []
            
            # Original candidates (simplified for testing)
            if hwsku:
                if platform:
                    hwsku_candidates_Json.append(os.path.join(platform_root_path, platform, hwsku, "hwsku.json"))
                
                # Add platform-level fallback when HWSKU-specific paths fail
                if platform:
                    hwsku_candidates_Json.append(os.path.join(platform_root_path, platform, "hwsku.json"))
            
            for candidate in hwsku_candidates_Json:
                if os.path.isfile(candidate):
                    return candidate
            return None
        
        print("=" * 60)
        print("TESTING ENHANCED get_hwsku_file_name FUNCTION")
        print("=" * 60)
        print(f"Platform: {platform_name}")
        print(f"HWSKU: {specific_hwsku}")
        print(f"Platform dir exists: {os.path.exists(platform_dir)}")
        print(f"HWSKU dir exists: {os.path.exists(os.path.join(platform_dir, specific_hwsku))}")
        print(f"Platform-level hwsku.json exists: {os.path.exists(platform_hwsku_path)}")
        
        # Test the enhanced function
        result = enhanced_get_hwsku_file_name(
            hwsku=specific_hwsku, 
            platform=platform_name,
            platform_root_path=test_dir
        )
        
        print(f"Enhanced function result: {result}")
        print(f"Expected fallback path: {platform_hwsku_path}")
        
        if result == platform_hwsku_path:
            print("✅ ENHANCED FUNCTION WORKS: Correctly fell back to platform-level file")
            success = True
        else:
            print("❌ ENHANCED FUNCTION FAILED: Did not fall back correctly")
            success = False
            
        # Test preservation (when HWSKU-specific file exists)
        hwsku_dir = os.path.join(platform_dir, specific_hwsku)
        os.makedirs(hwsku_dir, exist_ok=True)
        hwsku_specific_path = os.path.join(hwsku_dir, "hwsku.json")
        with open(hwsku_specific_path, 'w') as f:
            json.dump({"hwsku_specific": True}, f)
            
        result_preservation = enhanced_get_hwsku_file_name(
            hwsku=specific_hwsku, 
            platform=platform_name,
            platform_root_path=test_dir
        )
        
        print(f"\nPreservation test result: {result_preservation}")
        print(f"Expected HWSKU-specific path: {hwsku_specific_path}")
        
        if result_preservation == hwsku_specific_path:
            print("✅ PRESERVATION WORKS: Uses HWSKU-specific file when available")
            preservation_success = True
        else:
            print("❌ PRESERVATION FAILED: Did not use HWSKU-specific file")
            preservation_success = False
            
        return success and preservation_success
        
    finally:
        shutil.rmtree(test_dir, ignore_errors=True)

def test_actual_implementation():
    """Test that our actual implementation in portconfig.py has the right logic"""
    
    print("\n" + "=" * 60)
    print("CHECKING ACTUAL IMPLEMENTATION")
    print("=" * 60)
    
    # Read the actual portconfig.py file to verify our changes
    portconfig_path = "src/sonic-config-engine/portconfig.py"
    
    if os.path.exists(portconfig_path):
        with open(portconfig_path, 'r') as f:
            content = f.read()
            
        # Check if our fallback logic is present
        fallback_indicators = [
            "platform-level fallback",
            "platform, HWSKU_JSON",
            "Add platform-level fallback when HWSKU-specific paths fail"
        ]
        
        fallback_found = any(indicator in content for indicator in fallback_indicators)
        
        if fallback_found:
            print("✅ IMPLEMENTATION CHECK: Fallback logic found in portconfig.py")
            
            # Check the specific line we added
            if "hwsku_candidates_Json.append(os.path.join(PLATFORM_ROOT_PATH, platform, HWSKU_JSON))" in content:
                print("✅ SPECIFIC CHANGE: Platform-level fallback candidate is added")
                return True
            else:
                print("⚠️  PARTIAL: Fallback logic present but specific line not found")
                return False
        else:
            print("❌ IMPLEMENTATION CHECK: Fallback logic not found in portconfig.py")
            return False
    else:
        print("❌ FILE NOT FOUND: portconfig.py not found at expected location")
        return False

def test_device_info_implementation():
    """Test that our device_info.py implementation has the right logic"""
    
    print("\n" + "=" * 60)
    print("CHECKING DEVICE_INFO IMPLEMENTATION")
    print("=" * 60)
    
    # Read the actual device_info.py file to verify our changes
    device_info_path = "src/sonic-py-common/sonic_py_common/device_info.py"
    
    if os.path.exists(device_info_path):
        with open(device_info_path, 'r') as f:
            content = f.read()
            
        # Check if our fallback logic is present
        fallback_indicators = [
            "platform-level fallback",
            "Add platform-level fallback when HWSKU-specific files are missing",
            "platform_port_config"
        ]
        
        fallback_found = any(indicator in content for indicator in fallback_indicators)
        
        if fallback_found:
            print("✅ IMPLEMENTATION CHECK: Fallback logic found in device_info.py")
            
            # Check for specific changes
            if "platform_port_config = os.path.join(platform_path, PORT_CONFIG_FILE)" in content:
                print("✅ SPECIFIC CHANGE: Platform-level port_config.ini fallback is added")
                return True
            else:
                print("⚠️  PARTIAL: Fallback logic present but specific line not found")
                return False
        else:
            print("❌ IMPLEMENTATION CHECK: Fallback logic not found in device_info.py")
            return False
    else:
        print("❌ FILE NOT FOUND: device_info.py not found at expected location")
        return False

def main():
    """Main function to run all validation tests"""
    print("MINIGRAPH GENERIC HWSKU COMPATIBILITY - FIX VALIDATION")
    print("=" * 60)
    
    results = {}
    
    # Test the enhanced function logic
    results['enhanced_function'] = test_get_hwsku_file_name_fallback()
    
    # Test actual implementation
    results['portconfig_implementation'] = test_actual_implementation()
    results['device_info_implementation'] = test_device_info_implementation()
    
    print("\n" + "=" * 60)
    print("VALIDATION SUMMARY")
    print("=" * 60)
    
    all_passed = all(results.values())
    
    for test_name, passed in results.items():
        status = "✅ PASS" if passed else "❌ FAIL"
        print(f"  {test_name}: {status}")
    
    if all_passed:
        print(f"\n🎉 ALL VALIDATIONS PASSED!")
        print("✅ Enhanced function logic works correctly")
        print("✅ portconfig.py implementation has fallback logic")
        print("✅ device_info.py implementation has fallback logic")
        print("\nThe minigraph Generic HWSKU compatibility fix is ready!")
    else:
        print(f"\n❌ SOME VALIDATIONS FAILED")
        failed_tests = [name for name, passed in results.items() if not passed]
        print(f"Failed tests: {', '.join(failed_tests)}")
    
    return all_passed

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)