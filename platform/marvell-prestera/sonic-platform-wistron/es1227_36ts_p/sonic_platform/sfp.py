#
# sfp.py
#
# Platform-specific SFP transceiver interface for a 4-port device.
# This version correctly maps SONiC logical ports (e.g., 32-35) to
# physical CPLD port numbers (1-4).
#
import os
import sys

try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SFP_TYPE = "SFP"
SYSLOG_IDENTIFIER = "xcvrd"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)


class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""

    # Path to the CPLD sysfs directory for SFP control
    CPLD_DEV_PATH = "/sys/bus/i2c/devices/0-0033"

    # Path for SFP EEPROM access
    EEPROM_PATH = "/sys/bus/i2c/devices/{0}-0050/eeprom"

    # CRITICAL FIX: Map SONiC logical port numbers to physical CPLD port numbers.
    # The platform framework uses logical port numbers (e.g., 32 for Ethernet32),
    # but the CPLD driver uses physical numbers (p1, p2, ...).
    LOGICAL_TO_PHYSICAL_PORT_MAP = {
        33: 1,
        34: 2,
        35: 3,
        36: 4,
    }

    # CRITICAL FIX: The keys for this mapping must be the LOGICAL port numbers,
    # as this is what the framework provides during initialization.
    PORT_EEPROM_I2C_MAPPING = {
        33: 4,
        34: 5,
        35: 6,
        36: 7,
    }

    def __init__(self, index, sfp_type):
        SfpOptoeBase.__init__(self)
        self.logical_port = index  # This is the SONiC logical port number (e.g., 32)
        self.sfp_type = sfp_type

        # Determine the physical port number from the logical port number
        self.physical_port = self.LOGICAL_TO_PHYSICAL_PORT_MAP.get(self.logical_port)

        # The eeprom path is determined by the logical port number
        i2c_bus = self.PORT_EEPROM_I2C_MAPPING.get(self.logical_port)
        if i2c_bus is not None:
            self.eeprom_path = self.EEPROM_PATH.format(i2c_bus)
        else:
            self.eeprom_path = None

    def _decode_eeprom_data(self, raw_data):
        """
        Decode and clean the raw EEPROM data. Returns the decoded content.
        """
        decoded_data = []
        for byte in raw_data:
            # Only keep printable ASCII characters (32-126), otherwise replace with '.'
            if 32 <= byte <= 126:
                decoded_data.append(chr(byte))
            else:
                decoded_data.append('.')
        return ''.join(decoded_data)

    def _read_eeprom(self):
        """
        Helper function to read EEPROM data and decode mixed-format values.
        """
        if not self.eeprom_path:  # Ensure path is valid
            return None

        try:
            with open(self.eeprom_path, 'rb') as eeprom_file:
                raw_data = eeprom_file.read(256)  # Read maximum data size
                return self._decode_eeprom_data(raw_data)
        except IOError:
            sonic_logger.log_warning(f"Failed to read EEPROM from {self.eeprom_path}")
            return None


    def _read_cpld_sysfs(self, filename_suffix):
        """Helper function to read a value from a CPLD sysfs file."""
        # If this port is not a CPLD-managed SFP, do nothing.
        if self.physical_port is None:
            return None

        file_path = f"{self.CPLD_DEV_PATH}/sfp_p{self.physical_port}_{filename_suffix}"
        try:
            with open(file_path, 'r') as f:
                return f.read().strip()
        except (IOError, ValueError):
            # Log a warning only if it fails for an expected physical port
            # sonic_logger.log_warning(f"Failed to read {file_path}")
            return None

    def _write_cpld_sysfs(self, filename_suffix, value):
        """Helper function to write a value to a CPLD sysfs file."""
        if self.physical_port is None:
            return False

        file_path = f"{self.CPLD_DEV_PATH}/sfp_p{self.physical_port}_{filename_suffix}"
        try:
            with open(file_path, 'w') as f:
                f.write(value)
            return True
        except IOError:
            sonic_logger.log_warning(f"Failed to write to {file_path}")
            return False

    def get_presence(self):
        """
        Retrieves the presence of the SFP module from the CPLD driver.
        """
        # Only check presence for valid SFP ports
        if self.physical_port is None:
            return False

        value = self._read_cpld_sysfs("present")
        if value is not None:
            # Presence is typically active-low (0 means present).
            return int(value) == 0
        return False

    def get_rx_los(self):
        """
        Retrieves the RX LOS status from CPLD driver.
        """
        if self.physical_port is None:
            return [False]

        value = self._read_cpld_sysfs("loss")
        if value is not None:
            return [int(value) == 1]
        return [False]

    def get_tx_fault(self):
        """
        Retrieves the TX fault status from CPLD driver.
        """
        if self.physical_port is None:
            return [False]

        value = self._read_cpld_sysfs("fault")
        if value is not None:
            return [int(value) == 1]
        return [False]

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status from CPLD driver.
        """
        if self.physical_port is None:
            return [False]

        value = self._read_cpld_sysfs("tx_disable")
        if value is not None:
            return [int(value) == 1]
        return [False]

    def tx_disable(self, disable):
        """
        Disable SFP TX via CPLD driver.
        """
        return self._write_cpld_sysfs("tx_disable", '1' if disable else '0')

    def get_eeprom_path(self):
        """
        Returns the path to the EEPROM file for the SFP.
        """
        return self.eeprom_path

    def get_eeprom_content(self):
        """
        Returns the cleaned ASCII content from the EEPROM.
        """
        content = self._read_eeprom()
        if content is None:
            return None
        # Log the decoded content for debugging purposes
        sonic_logger.log_info(f"Decoded EEPROM content for port {self.logical_port}: {content}")
        return content

    # --- Boilerplate/unsupported methods ---
    def get_name(self):
        # The name should reflect the logical port number given by the framework
        return f"Ethernet{self.logical_port}"

    def get_position_in_parent(self):
        return self.logical_port

    def is_replaceable(self):
        # Only our mapped ports are replaceable SFPs
        return self.physical_port is not None

    def get_status(self):
        return self.get_presence()

    def reset(self):
        return False  # Not supported

    def set_lpmode(self, lpmode):
        return False  # Not supported

    def get_lpmode(self):
        return False  # Not supported