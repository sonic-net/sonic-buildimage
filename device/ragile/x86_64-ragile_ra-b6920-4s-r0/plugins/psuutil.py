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
        return 4

    def get_psu_status(self, index):
        if index < 1 or index > 4:
            return False

        path_tmp = "/sys/devices/pci0000:00/0000:00:1f.0/psu_status_"
        psu_path = "%s%d"%(path_tmp, index)

        try:
            data = open(psu_path, "rb")
        except IOError:
            return False

        result = int(data.read(2), 16)
        data.close()

        if (result & 0x2):
            return True

        return False

    def get_psu_presence(self, index):
        if index < 1 or index > 4:
            return False

        path_tmp = "/sys/devices/pci0000:00/0000:00:1f.0/psu_status_"
        psu_path = "%s%d"%(path_tmp, index)

        try:
            data = open(psu_path, "rb")
        except IOError:
            return False

        result = int(data.read(2), 16)
        data.close()

        if (result & 0x1) == 0:
            return True

        return False
