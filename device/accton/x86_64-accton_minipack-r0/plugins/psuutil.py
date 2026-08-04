#############################################################################
# Accton
#
# Module contains an implementation of SONiC PSU Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

import os.path

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)

        self.num = 4
        self.psu_path = "/sys/bus/platform/devices/minipack_psensor/"
        self.psu_voltage = "/in{0}_input"

    def get_num_psus(self):
        return self.num

    def get_psu_status(self, index):
        if index is None:
            return False

        status = 0
        node = self.psu_path + self.psu_voltage.format(index*2-1)
        try:
            with open(node, 'r') as power_status:
                status = int(power_status.read())
        except IOError:
            return False

        return status > 0

    def get_psu_presence(self, index):
        if index is None:
            return False

        status = 0
        node = self.psu_path + self.psu_voltage.format(index*2)
        try:
            with open(node, 'r') as presence_status:
                status = int(presence_status.read())
        except IOError:
            return False

        return status > 0
