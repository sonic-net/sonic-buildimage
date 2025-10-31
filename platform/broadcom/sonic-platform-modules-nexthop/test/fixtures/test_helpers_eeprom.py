#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Shared test helpers for EEPROM testing.

This module contains common utilities used by both unit and integration tests
for EEPROM functionality. It helps avoid code duplication between test files.
"""

import os
import tempfile
from typing import Dict


# Constants
EEPROM_SIZE = 8192


class EepromTestHelpers:
    """Shared helper methods for EEPROM testing."""

    @staticmethod
    def create_fake_eeprom(path_to_file: str, size: int = EEPROM_SIZE) -> None:
        """
        Create a fake EEPROM file for testing.

        Args:
            path_to_file: Path where the EEPROM file should be created
            size: Size of the EEPROM file in bytes (default: 8192)
        """
        # Ensure the directory exists
        os.makedirs(os.path.dirname(path_to_file), exist_ok=True)
        with open(path_to_file, "w+b") as f:
            f.write(bytearray([0xFF] * size))

    @staticmethod
    def create_fake_i2c_device(device_name: str, file_to_content: Dict[str, str], root: str) -> None:
        """
        Create a fake I2C device directory structure for testing.
        
        Args:
            device_name: Name of the I2C device (e.g., "11-0050")
            file_to_content: Dictionary mapping file names to their content
            root: Root directory where the device structure should be created
        """
        device_dir = os.path.join(root, "sys/bus/i2c/devices", device_name)
        os.makedirs(device_dir, exist_ok=True)
        for file_name, content in file_to_content.items():
            with open(os.path.join(device_dir, file_name), "x") as f:
                f.write(f"{content}\n")

    @staticmethod
    def setup_test_i2c_environment(root: str) -> None:
        """
        Set up a complete test I2C environment with multiple devices.
        
        This creates a realistic I2C device tree for testing EEPROM discovery.
        
        Args:
            root: Root directory where the I2C structure should be created
        """
        # Create various I2C devices for comprehensive testing
        EepromTestHelpers.create_fake_i2c_device("i2c-0", {"name": "i2c-pci-3"}, root)
        EepromTestHelpers.create_fake_i2c_device("11-0050", {"name": "24c64", "eeprom": ""}, root)
        EepromTestHelpers.create_fake_i2c_device("8-0050", {"name": "24c64"}, root)  # No eeprom file
        EepromTestHelpers.create_fake_i2c_device("9-0050", {"name": "24c64", "eeprom": "32423"}, root)

    @staticmethod
    def get_expected_eeprom_paths(root: str) -> list:
        """
        Get the expected EEPROM paths for the standard test I2C environment.
        
        Args:
            root: Root directory of the test I2C structure
            
        Returns:
            List of expected EEPROM device paths
        """
        return [
            os.path.join(root, "sys/bus/i2c/devices/11-0050/eeprom"),
            os.path.join(root, "sys/bus/i2c/devices/9-0050/eeprom"),
        ]

    @staticmethod
    def create_test_eeprom_with_data(eeprom_path: str) -> None:
        """
        Create a test EEPROM file and return the path.
        
        Args:
            eeprom_path: Path where the EEPROM should be created
        """
        # Ensure the directory exists
        os.makedirs(os.path.dirname(eeprom_path), exist_ok=True)
        EepromTestHelpers.create_fake_eeprom(eeprom_path)

    @staticmethod
    def get_standard_eeprom_program_data() -> dict:
        """
        Get standard EEPROM programming data for consistent testing.
        
        Returns:
            Dictionary with standard EEPROM programming parameters
        """
        return {
            "product_name": "NH-4010",
            "part_num": "ABC",
            "serial_num": "XYZ",
            "mac": "00:E1:4C:68:00:C4",
            "device_version": "0",
            "label_revision": "P0",
            "platform_name": "x86_64-nexthop_4010-r0",
            "manufacturer_name": "Nexthop",
            "vendor_name": "Nexthop",
            "service_tag": "www.nexthop.ai",
            "custom_serial_number": "123",
            "regulatory_model_number": "NH99-99",
        }

    @staticmethod
    def get_expected_tlv_output() -> str:
        """
        Get the expected TLV output for the standard EEPROM programming data.
        
        Returns:
            Expected TLV output string for verification
        """
        return """\
TlvInfo Header:
   Id String:    TlvInfo
   Version:      1
   Total Length: 122
TLV Name                  Code Len Value
------------------------- ---- --- -----
Product Name              0x21   7 NH-4010
Part Number               0x22   3 ABC
Serial Number             0x23   3 XYZ
Base MAC Address          0x24   6 00:E1:4C:68:00:C4
Device Version            0x26   1 0
Label Revision            0x27   2 P0
Platform Name             0x28  22 x86_64-nexthop_4010-r0
Manufacturer              0x2B   7 Nexthop
Vendor Name               0x2D   7 Nexthop
Service Tag               0x2F  14 www.nexthop.ai
Custom Serial Number      0xFD   8 123
Regulatory Model Number   0xFD  12 NH99-99
CRC-32                    0xFE   4 0x368C1825
"""


class EepromTestMixin:
    """
    Mixin class that provides EEPROM test helper methods to test classes.
    
    Test classes can inherit from this mixin to get access to all the helper methods
    as instance methods, which is convenient for test organization.
    """

    def create_fake_eeprom(self, path_to_file: str, size: int = EEPROM_SIZE) -> None:
        """Create a fake EEPROM file for testing."""
        return EepromTestHelpers.create_fake_eeprom(path_to_file, size)

    def create_fake_i2c_device(self, device_name: str, file_to_content: Dict[str, str], root: str) -> None:
        """Create a fake I2C device directory structure for testing."""
        return EepromTestHelpers.create_fake_i2c_device(device_name, file_to_content, root)

    def setup_test_i2c_environment(self, root: str) -> None:
        """Set up a complete test I2C environment with multiple devices."""
        return EepromTestHelpers.setup_test_i2c_environment(root)

    def get_expected_eeprom_paths(self, root: str) -> list:
        """Get the expected EEPROM paths for the standard test I2C environment."""
        return EepromTestHelpers.get_expected_eeprom_paths(root)

    def create_test_eeprom_with_data(self, eeprom_path: str) -> None:
        """Create a test EEPROM file and return the path."""
        return EepromTestHelpers.create_test_eeprom_with_data(eeprom_path)

    def get_standard_eeprom_program_data(self) -> dict:
        """Get standard EEPROM programming data for consistent testing."""
        return EepromTestHelpers.get_standard_eeprom_program_data()

    def get_expected_tlv_output(self) -> str:
        """Get the expected TLV output for the standard EEPROM programming data."""
        return EepromTestHelpers.get_expected_tlv_output()
