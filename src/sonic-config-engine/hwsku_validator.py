#!/usr/bin/env python3
"""
HWSKU Validation Utility for Generic HWSKU Compatibility

This module provides HWSKU validation functionality to support Generic HWSKU
compatibility without modifying the protected minigraph.py file.
"""

import os
import sys


def validate_hwsku_compatibility(hwsku, platform=None):
    """
    Validate HWSKU and log fallback behavior for Generic HWSKU compatibility
    
    Args:
        hwsku (str): The HWSKU name from minigraph
        platform (str): The platform name (optional)
    
    Returns:
        dict: Validation results with status and messages
    """
    if not hwsku or hwsku == "Generic":
        return {
            'status': 'skip',
            'message': 'No validation needed for Generic HWSKU'
        }
    
    try:
        from sonic_py_common import device_info
        from portconfig import get_hwsku_file_name
        
        if not platform:
            platform = device_info.get_platform()
            
        if not platform:
            return {
                'status': 'warning',
                'message': f'Could not determine platform for HWSKU validation: {hwsku}'
            }
        
        # Check if HWSKU-specific folder exists by trying to get hwsku file
        hwsku_file = get_hwsku_file_name(hwsku=hwsku, platform=platform)
        
        if hwsku_file:
            platform_root = '/usr/share/sonic/device'
            hwsku_specific_path = os.path.join(platform_root, platform, hwsku, 'hwsku.json')
            platform_level_path = os.path.join(platform_root, platform, 'hwsku.json')
            
            if hwsku_file == platform_level_path and os.path.exists(platform_level_path):
                message = f"INFO: HWSKU '{hwsku}' from minigraph does not have specific folder, using platform-level files for Generic HWSKU compatibility"
                print(message, file=sys.stderr)
                return {
                    'status': 'fallback',
                    'message': message,
                    'hwsku_file': hwsku_file
                }
            elif hwsku_file == hwsku_specific_path:
                message = f"INFO: Using HWSKU-specific configuration for '{hwsku}'"
                print(message, file=sys.stderr)
                return {
                    'status': 'hwsku_specific',
                    'message': message,
                    'hwsku_file': hwsku_file
                }
        else:
            message = f"WARNING: HWSKU '{hwsku}' from minigraph has no corresponding configuration files, provisioning may fail"
            print(message, file=sys.stderr)
            return {
                'status': 'error',
                'message': message
            }
            
    except Exception as e:
        # Don't fail if validation fails
        message = f"WARNING: Could not validate HWSKU '{hwsku}': {e}"
        print(message, file=sys.stderr)
        return {
            'status': 'error',
            'message': message,
            'exception': str(e)
        }
    
    return {
        'status': 'unknown',
        'message': f'Unexpected validation result for HWSKU: {hwsku}'
    }


def main():
    """
    Command-line interface for HWSKU validation
    """
    if len(sys.argv) < 2:
        print("Usage: python hwsku_validator.py <hwsku> [platform]", file=sys.stderr)
        sys.exit(1)
    
    hwsku = sys.argv[1]
    platform = sys.argv[2] if len(sys.argv) > 2 else None
    
    result = validate_hwsku_compatibility(hwsku, platform)
    
    print(f"HWSKU Validation Result:")
    print(f"  Status: {result['status']}")
    print(f"  Message: {result['message']}")
    
    if result['status'] == 'error':
        sys.exit(1)
    else:
        sys.exit(0)


if __name__ == "__main__":
    main()