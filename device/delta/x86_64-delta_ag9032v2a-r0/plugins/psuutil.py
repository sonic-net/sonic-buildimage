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
        try:
            cmd = ["ipmitool", "raw", "0x38", "0x2", "3", "0x6a", "0x3", "1"]
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True)
            content = p.stdout.readline().rstrip()
            reg_value = int(content, 16)
            if index == 1:
                mask = (1 << 6)
            else:
                mask = (1 << 2)
            if reg_value & mask == 0:
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
        try:
            cmd = ["ipmitool", "raw", "0x38", "0x2", "3", "0x6a", "0x3", "1"]
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True)
            content = p.stdout.readline().rstrip()
            reg_value = int(content, 16)
            if index == 1:
                mask = (1 << 7)
                if reg_value & mask == 0x80:
                    return False
            else:
                mask = (1 << 3)
                if reg_value & mask == 0x08:
                    return False
            status = 1
            p.close()
        except IOError:
            return False
        return status == 1
