#
# psuutil.py
# Platform-specific PSU status interface for SONiC
#

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)

    def get_num_psus(self):
        return 2

    def get_psu_status(self, index):
        if index != 1 and index != 2:
            return False

        psu_path = "/sys/bus/i2c/devices/6-000d/psu_status"

        try:
            data = open(psu_path, "rb")
        except IOError:
            return False

        result = int(data.read(2), 16)
        data.close()

        if index == 1 and (result & 0x2):
            return True

        if index == 2 and (result & 0x20):
            return True

        return False

    def get_psu_presence(self, index):
        if index != 1 and index != 2:
            return False

        psu_path = "/sys/bus/i2c/devices/6-000d/psu_status"

        try:
            data = open(psu_path, "rb")
        except IOError:
            return False

        result = int(data.read(2), 16)
        data.close()

        if index == 1 and (result & 0x1) == 0:
            return True

        if index == 2 and (result & 0x10) == 0:
            return True

        return False
