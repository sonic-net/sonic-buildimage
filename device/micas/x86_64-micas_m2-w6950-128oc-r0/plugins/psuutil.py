#
# psuutil.py
# Platform-specific PSU status interface for SONiC
#

import os.path

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)

    def get_num_psus(self):
        return 4

    def get_psu_status(self, index):
        if index != 1 and index != 2 and index != 4:
            return False

        psu_path = "/sys/rg_plat/psu/psu%d/output" % index

        try:
            data = open(psu_path, "rb")
        except IOError:
            return False

        result = int(data.read(), 10)
        data.close()

        if result == 1:
            return True

        return False

    def get_psu_presence(self, index):
        if index != 1 and index != 2 and index != 4:
            return False

        psu_path = "/sys/rg_plat/psu/psu%d/present" % index

        try:
            data = open(psu_path, "rb")
        except IOError:
            return False

        result = int(data.read(), 10)
        data.close()

        if result == 1:
            return True

        return False
