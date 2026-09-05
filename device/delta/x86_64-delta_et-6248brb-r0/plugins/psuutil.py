import os.path

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)

        self.psu_path = "/sys/devices/platform/delta-et6248brb-gpio.0/PSU/psu{}_pg"
        self.psu_oper_status = "in1_input"
        self.psu_presence = "/sys/devices/platform/delta-et6248brb-gpio.0/PSU/psu{}_pres"

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device

        :return: An integer, the number of PSUs available on the device
        """
        return 2

    def get_psu_status(self, index):
        if index is None:
            return False

        try:
            reg_file = open(self.psu_path.format(index))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        if int(reg_file.readline()) == 1:
            return True

        return False

    def get_psu_presence(self, index):
        if index is None:
            return False

        try:
            reg_file = open(self.psu_presence.format(index))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        if int(reg_file.readline()) == 0:
            return True

        return False
