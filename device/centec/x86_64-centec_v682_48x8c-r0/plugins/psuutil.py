#!/usr/bin/env python

#############################################################################
# Centec
#
# Module contains an implementation of SONiC PSU Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

from subprocess import Popen, PIPE, STDOUT

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

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

        cmd = ['i2cget', '-y', '0', '0x36', '0x1e']
        status = int(Popen(cmd, stdout=PIPE, stderr=STDOUT).stdout.readline(), 16)
        powergood = ((status & (1 << (3 * (index - 1) + 2))) != 0)
        return powergood

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by 1-based index <index>

        :param index: An integer, 1-based index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        if index is None:
            return False

        cmd = ['i2cget', '-y', '0', '0x36', '0x1e']
        status = int(Popen(cmd, stdout=PIPE, stderr=STDOUT).stdout.readline(), 16)
        presence = ((status & (1 << (3 * (index - 1) + 1))) == 0)
        return presence
