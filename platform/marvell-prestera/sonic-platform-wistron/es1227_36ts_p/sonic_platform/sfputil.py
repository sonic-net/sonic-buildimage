#!/usr/bin/env python3
"""
SFP Utility for detecting presence and reading EEPROM data on SFP modules.
"""

import os

class SfpUtil:
    """
    This is the main class for SFP detection and EEPROM handling.
    """

    # Mapping between port numbers and I2C bus addresses
    PORT_TO_I2C_MAPPING = {
        32: "4-0050",  # Ethernet32 -> i2c-4 bus, address 0x50
        33: "5-0050",  # Ethernet33 -> i2c-5 bus, address 0x50
        34: "6-0050",  # Ethernet34 -> i2c-6 bus, address 0x50
        35: "7-0050",  # Ethernet35 -> i2c-7 bus, address 0x50
    }

    # Base device path format for EEPROM files
    I2C_DEVICE_PATH = "/sys/bus/i2c/devices/{}/eeprom"

    def get_presence(self, port_num):
        """
        Check if an SFP module is present in the given port.

        Args:
            port_num (int): Port number (e.g., 32 for Ethernet32)

        Returns:
            bool: True if SFP module is detected, False otherwise.
        """
        try:
            # Get the I2C device address from mapping
            i2c_address = self.PORT_TO_I2C_MAPPING.get(port_num)
            if not i2c_address:
                print(f"Port {port_num} is not a valid SFP port.")
                return False

            # Construct the EEPROM file path
            eeprom_path = self.I2C_DEVICE_PATH.format(i2c_address)
            # Check if EEPROM data can be read
            with open(eeprom_path, "rb") as eeprom_file:
                eeprom_file.read(1)  # Attempt to read 1 byte
            print(f"SFP module detected at port {port_num}.")  # Debug output
            return True
        except FileNotFoundError:
            print(f"No SFP module detected at port {port_num}.")  # Debug output
            return False
        except Exception as e:
            print(f"Unexpected error checking presence on port {port_num}: {e}")
            return False

    def read_eeprom(self, port_num):
        """
        Read and parse EEPROM data for the given port.

        Args:
            port_num (int): Port number (e.g., 32 for Ethernet32)

        Returns:
            dict or None: Dictionary of EEPROM information, or None if not available.
        """
        try:
            # Get the I2C device address
            i2c_address = self.PORT_TO_I2C_MAPPING.get(port_num)
            if not i2c_address:
                print(f"Port {port_num} is not a valid SFP port.")
                return None

            # Construct the EEPROM file path
            eeprom_path = self.I2C_DEVICE_PATH.format(i2c_address)
            # Read the raw EEPROM data
            with open(eeprom_path, "rb") as eeprom_file:
                eeprom_data = eeprom_file.read(256)  # Read the first 256 bytes

            # Parse specific fields (indices may vary by SFP vendor)
            eeprom_info = {
                "Vendor Name": eeprom_data[20:36].decode("ascii", errors="ignore").strip(),
                "Part Number": eeprom_data[40:56].decode("ascii", errors="ignore").strip(),
                "Serial Number": eeprom_data[68:84].decode("ascii", errors="ignore").strip(),
                "Date Code": eeprom_data[84:92].decode("ascii", errors="ignore").strip()
            }
            return eeprom_info
        except FileNotFoundError:
            print(f"No EEPROM data available or SFP not present at port {port_num}.")
            return None
        except Exception as e:
            print(f"Error reading EEPROM on port {port_num}: {e}")
            return None

# Main script execution (for debugging)
if __name__ == "__main__":
    sfputil = SfpUtil()

    # Check SFP presence and determine EEPROM info for all ports
    for port_num in range(32, 36):
        is_present = sfputil.get_presence(port_num)
        if is_present:
            print(f"Port {port_num}: SFP module present.")
            eeprom_info = sfputil.read_eeprom(port_num)
            if eeprom_info:
                print(f"EEPROM info for Port {port_num}: {eeprom_info}")
            else:
                print(f"Failed to read EEPROM for Port {port_num}.")
        else:
            print(f"Port {port_num}: No SFP module detected.")