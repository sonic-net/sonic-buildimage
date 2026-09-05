#############################################################################
# psuutil.py
# Platform-specific PSU status interface for SONiC
#############################################################################

import os.path
import sonic_platform

try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FAN_MAX_RPM = 9600
PSU_NAME_LIST = ["PSU-0", "PSU-1"]


class Psu(PsuBase):
    """Platform-specific Psu class"""

    SYSFS_PSU_DIR = ["/sys/bus/i2c/devices/10-0050",
                     "/sys/bus/i2c/devices/11-0053"]

    def __init__(self):
        self.index = psu_index
        PsuBase.__init__(self)

    def get_fan(self):
        """
        Retrieves object representing the fan module contained in this PSU
        Returns:
            An object dervied from FanBase representing the fan module
            contained in this PSU
        """
        # Hardware not supported
        return False

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU
        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        return self.get_status()

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED
        Args:
            color: A string representing the color with which to set the PSU status LED
                   Note: Only support green and off
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        # Hardware not supported
        return False

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return PSU_NAME_LIST[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        attr_file = 'psu_present'
        attr_path = self.SYSFS_PSU_DIR[self.index-1] + '/' + attr_file
        status = 0
        try:
            with open(attr_path, 'r') as psu_prs:
                status = int(psu_prs.read())
        except IOError:
            return False

        return status == 1

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        attr_file = 'psu_power_good'
        attr_path = self.SYSFS_PSU_DIR[self.index-1] + '/' + attr_file
        status = 0
        try:
            with open(attr_path, 'r') as power_status:
                status = int(power_status.read())
        except IOError:
            return False

        return status == 1
