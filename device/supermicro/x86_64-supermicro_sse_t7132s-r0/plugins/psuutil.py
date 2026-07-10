import os.path
import subprocess
import sys
import re

IPMI_PSU1_DATA = "docker exec -it pmon ipmitool sdr list | grep PS1 | awk -F \"|\" '{print $2}'"
IPMI_PSU2_DATA = "docker exec -it pmon ipmitool sdr list | grep PS2 | awk -F \"|\" '{print $2}'"

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)

    def run_command(self, command):
        proc = subprocess.Popen(command, shell=True, universal_newlines=True, stdout=subprocess.PIPE)
        (out, err) = proc.communicate()

        if proc.returncode != 0:
            sys.exit(proc.returncode)

        return out

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
            res_string = self.run_command(IPMI_PSU1_DATA)
        else:
            res_string = self.run_command(IPMI_PSU2_DATA)

        try:
            ret_value = int(res_string, 0)
        except ValueError as e:
            return False

        if ret_value == 0x1:
            return True
        else: 
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
            res_string = self.run_command(IPMI_PSU1_DATA)
        else:
            res_string = self.run_command(IPMI_PSU2_DATA)

        try:
            ret_value = int(res_string, 0)
        except ValueError as e:
            return False

        if ret_value == 0x1 or ret_value == 0xb:
            return True
        else: 
            return False
