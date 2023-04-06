#!/usr/bin/env python

#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the sfp status which are available in the platform
#
#############################################################################

import time

try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


SFP_I2C_START = 26
I2C_EEPROM_PATH = '/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom'


class SfpOptoe(SfpOptoeBase):
    """Platform-specific SfpOptoe class"""

    RESET_PATH = "/sys/devices/platform/dx010_cpld/qsfp_reset"
    LP_PATH = "/sys/devices/platform/dx010_cpld/qsfp_lpmode"
    PRS_PATH = "/sys/devices/platform/dx010_cpld/qsfp_modprs"

    def __init__(self, sfp_index=0, sfp_name=None):
        SfpOptoeBase.__init__(self)

        self._index = sfp_index
        self._port_num = self._index + 1
        self._api_helper = APIHelper()
        self._name = sfp_name
        self._sfp_type = None

    def _detect_sfp_type(self):
        sfp_type = 'N/A'
        info = self.get_transceiver_info()
        if info:
            sfp_type = info.get("type_abbrv_name")
        # XXX: Need this hack until xcvrd is refactored
        if sfp_type in ["OSFP-8X", "QSFP-DD"]:
            sfp_type = "QSFP_DD"
        return sfp_type

    @property
    def sfp_type(self):
        if self._sfp_type is None:
            self._sfp_type = self._detect_sfp_type()
        return self._sfp_type

    def get_eeprom_path(self):
        port_to_i2c_mapping = SFP_I2C_START + self._index
        port_eeprom_path = I2C_EEPROM_PATH.format(port_to_i2c_mapping)
        return port_eeprom_path

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self._name

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if not self.get_presence():
            return False
        reset = self.get_reset_status()
        if reset:
            status = False
        else:
            status = True
        return status

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        reset_status_raw = self._api_helper.read_txt_file(
            self.RESET_PATH).rstrip()
        if not reset_status_raw:
            return False

        reg_value = int(reset_status_raw, 16)
        bin_format = bin(reg_value)[2:].zfill(32)
        return bin_format[::-1][self._index] == '0'

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        presence_status_raw = self._api_helper.read_txt_file(
            self.PRS_PATH).rstrip()
        if not presence_status_raw:
            return False

        content = presence_status_raw.rstrip()
        reg_value = int(content, 16)

        # Determind if port_num start from 1 or 0
        bit_index = self._index

        # Mask off the bit corresponding to our port
        mask = (1 << bit_index)

        # ModPrsL is active low
        if reg_value & mask == 0:
            return True

        return False

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        try:
            reg_file = open(self.LP_PATH, "r")
            content = reg_file.readline().rstrip()
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Determind if port_num start from 1 or 0
        bit_index = self._index

        # Mask off the bit corresponding to our port
        mask = (1 << bit_index)

        # LPMode is active high
        if reg_value & mask == 0:
            return False

        return True

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        # Check for invalid port_num

        try:
            reg_file = open(self.RESET_PATH, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # File content is a string containing the hex representation of the
        # register
        reg_value = int(content, 16)

        # Determind if port_num start from 1 or 0
        bit_index = self._index

        # Mask off the bit corresponding to our port
        mask = (1 << bit_index)

        # ResetL is active low
        reg_value = reg_value & ~mask

        # Convert our register value back to a hex string and write back
        reg_file.seek(0)
        reg_file.write(hex(reg_value).rstrip('L'))
        reg_file.close()

        # Sleep 1 second to allow it to settle
        time.sleep(1)

        # Flip the bit back high and write back to the register
        # to take port out of reset
        try:
            reg_file = open(self.RESET_PATH, "w")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = reg_value | mask
        reg_file.seek(0)
        reg_file.write(hex(reg_value).rstrip('L'))
        reg_file.close()

        return True

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        try:
            reg_file = open(self.LP_PATH, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Determind if port_num start from 1 or 0
        bit_index = self._index

        # Mask off the bit corresponding to our port
        mask = (1 << bit_index)
        # LPMode is active high; set or clear the bit accordingly
        reg_value = reg_value | mask if lpmode else reg_value & ~mask

        # Convert our register value back to a hex string and write back
        content = hex(reg_value).strip('L')

        reg_file.seek(0)
        reg_file.write(content)
        reg_file.close()

        return True

    def get_error_description(self):
        """
        Retrives the error descriptions of the SFP module
        Returns:
            String that represents the current error descriptions of
            vendor specific errors
            In case there are multiple errors, they should be joined by '|',
            like: "Bad EEPROM|Unsupported cable"
        """
        if self.sfp_type == "QSFP_DD":
            return super().get_error_description()

        if not self.get_presence():
            return self.SFP_STATUS_UNPLUGGED
        return self.SFP_STATUS_OK

    def get_position_in_parent(self):
        return self._index

    def get_index(self):
        """
        Retrieves current sfp index
        Returns:
            A int value, sfp index
        """
        return self._index

    def is_replaceable(self):
        return True
