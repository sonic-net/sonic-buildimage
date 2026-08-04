import os.path

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)

        self.psu_path = "/sys/devices/platform/e1031.smc/"
        self.psu_presence = "psu{}_prs"
        self.psu_oper_status = "psu{}_status"

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device

        :return: An integer, the number of PSUs available on the device
        """
        return 2

    def get_psu_status(self, index):
        """
        Retrieves the oprational status of power supply unit (PSU) defined by 1-based index <index>

        :param index: An integer, 1-based index of the PSU of which to query status
        :return: Boolean, True if PSU is operating properly, False if PSU is faulty
        """
        psu_location = ["R", "L"]
        status = 0
        try:
            with open(self.psu_path + self.psu_oper_status.format(psu_location[index - 1]), 'r') as power_status:
                status = int(power_status.read())
        except IOError:
            return False

        return status == 1

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined by 1-based index <index>

        :param index: An integer, 1-based index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        psu_location = ["R", "L"]
        status = 0
        try:
            with open(self.psu_path + self.psu_presence.format(psu_location[index - 1]), 'r') as psu_prs:
                status = int(psu_prs.read())
        except IOError:
            return False

        return status == 1
