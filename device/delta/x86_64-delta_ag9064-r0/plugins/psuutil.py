#
# Module contains an implementation of SONiC PSU Base API and
# provides the PSUs status which are available in the platform
#

import subprocess

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)
        self.psu_presence = "/sys/devices/platform/delta-ag9064-cpld.0/psu{}_scan"
        self.psu_status = "/sys/devices/platform/delta-ag9064-swpld1.0/psu{}_pwr_ok"

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

        status = 0
        self.psu_status = self.psu_status.format(index)
        try:
            p = open(self.psu_status, 'r')
            content = p.readline().rstrip()
            reg_value = int(content)
            if reg_value != 0:
                return False
            status = 1
            p.close()
        except IOError:
            return False
        return status == 1

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by 1-based index <index>

        :param index: An integer, 1-based index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        if index is None:
            return False
        status = 0
        self.psu_presence = self.psu_presence.format(index)
        try:
            p = open(self.psu_presence, 'r')
            content = p.readline().rstrip()
            reg_value = int(content, 16)
            if reg_value != 0:
                return False
            status = 1
            p.close()
        except IOError:
            return False
        return status == 1
