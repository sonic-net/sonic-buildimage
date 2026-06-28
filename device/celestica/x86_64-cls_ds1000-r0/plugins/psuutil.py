#
# psuutil.py
# Platform-specific PSU status interface for SONiC
#

import os

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device
        :return: An integer, the number of PSUs available on the device
        """
        return 2

    def get_psu_status(self, index):
        """
        Retrieves the oprational status of power supply unit (PSU) defined
                by 1-based index <index>
        :param index: An integer, 1-based index of the PSU of which to query status
        :return: Boolean, True if PSU is operating properly, False if PSU is faulty
        """
        if index is None:
            return False

        if index == 1:
            status_path = "/sys/devices/pci0000:00/0000:00:12.0/i2c-0/i2c-4/4-0058/psu_power_good"
        elif index == 2:
            status_path = "/sys/devices/pci0000:00/0000:00:12.0/i2c-0/i2c-4/4-0059/psu_power_good"
        else:
            return False

        try:
            with open(status_path, 'r') as fd:
                status = fd.read().rstrip('\r\n')
            if status == '0':
                return True
            else:
                return False
        except IOError:
            return False

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by 1-based index <index>
        :param index: An integer, 1-based index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        if index is None:
            return False

        if index == 1:
            status_path = "/sys/devices/pci0000:00/0000:00:12.0/i2c-0/i2c-4/4-0058/psu_present"
        elif index == 2:
            status_path = "/sys/devices/pci0000:00/0000:00:12.0/i2c-0/i2c-4/4-0059/psu_present"
        else:
            return False

        try:
            with open(status_path, 'r') as fd:
                status = fd.read().rstrip('\r\n')
            if status == '1':
                return True
            else:
                return False
        except IOError:
            return False
