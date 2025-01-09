#!/usr/bin/env python

#############################################################################
# Celestica
#
# Sfp contains an implementation of SONiC Platform Base API and
# provides the sfp device status which are available in the platform
#
#############################################################################
import time

try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SFP_TYPE = "SFP"
QSFP_TYPE = "QSFP"

QSFP_PORT_START = 1
QSFP_PORT_END = 32
SFP_PORT_START = 33
SFP_PORT_END = 33

PORT_INFO_PATH = '/sys/class/seastone2_fpga'
SFP_I2C_START = 2


class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""

    # Path to QSFP sysfs
    PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
    PMON_HWSKU_PATH = "/usr/share/sonic/hwsku"

    def __init__(self, sfp_index):
        SfpOptoeBase.__init__(self)
        # Init index
        self.index = sfp_index + 1
        self.sfp_type, self.port_name = self.__get_sfp_info()
        self._api_helper = APIHelper()
        self.platform = self._api_helper.platform
        self.hwsku = self._api_helper.hwsku

        # Init eeprom path
        self.eeprom_path = '/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom'.format(SFP_I2C_START+sfp_index)

    def __get_sfp_info(self):
        port_name = "Unknown"
        sfp_type = "Unknown"

        if self.index >= QSFP_PORT_START and self.index <= QSFP_PORT_END:
            sfp_type = QSFP_TYPE
            port_name = sfp_type + str(self.index)
        elif self.index >= SFP_PORT_START and self.index <= SFP_PORT_END:
            sfp_type = SFP_TYPE
            port_name = sfp_type + str(self.index - SFP_PORT_START + 1)

        return sfp_type, port_name

    def __get_path_to_port_config_file(self):
        platform_path = "/".join([self.PLATFORM_ROOT_PATH, self.platform])
        hwsku_path = "/".join([platform_path, self.hwsku]
                              ) if self._api_helper.is_host() else self.PMON_HWSKU_PATH
        return "/".join([hwsku_path, "port_config.ini"])

    def get_eeprom_path(self):
        return self.eeprom_path

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        reg_status = self._api_helper.read_one_line_file(
            "/".join([PORT_INFO_PATH, self.port_name, "qsfp_reset"]))

        return (int(reg_status) == 0)


    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        if self.sfp_type != QSFP_TYPE:
            return False

        try:
            reg_file = open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_reset"]), "w")
        except IOError as e:
            #print("Error: unable to open file: %s" % str(e))
            return False

        # Convert our register value back to a hex string and write back
        reg_file.seek(0)
        reg_file.write(hex(0))
        reg_file.close()

        # Sleep 1 second to allow it to settle
        time.sleep(1)

        # Flip the bit back high and write back to the register to take port out of reset
        try:
            reg_file = open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_reset"]), "w")
        except IOError as e:
            #print("Error: unable to open file: %s" % str(e))
            return False

        reg_file.seek(0)
        reg_file.write(hex(1))
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
            reg_file = open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_lpmode"]), "w")
        except IOError as e:
            return False

        if lpmode:
            value=1
        else:
            value=0
        reg_file.write(hex(value))
        reg_file.close()

        return True

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """

        reg_status = self._api_helper.read_one_line_file(
            "/".join([PORT_INFO_PATH, self.port_name, "qsfp_lpmode"]))

        return (int(reg_status) == 1)

    def get_position_in_parent(self):
        return self.index

    def is_replaceable(self):
        return True

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        sfputil_helper = SfpUtilHelper()
        sfputil_helper.read_porttab_mappings(
            self.__get_path_to_port_config_file())
        name = sfputil_helper.physical_to_logical[self.index] or "Unknown"
        return name

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        # Get path for access port presence status
        sysfs_filename = "sfp_modabs" if self.sfp_type == SFP_TYPE else "qsfp_modprs"
        reg_path = "/".join([PORT_INFO_PATH, self.port_name, sysfs_filename])

        content = self._api_helper.read_one_line_file(reg_path)
        reg_value = int(content)

        return (reg_value == 0)

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence() and not self.get_reset_status()
