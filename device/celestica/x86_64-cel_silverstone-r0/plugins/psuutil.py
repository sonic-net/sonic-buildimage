import os.path
import subprocess
import sys
import re

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        self.ipmi_raw = "docker exec -ti pmon ipmitool raw 0x4 0x2d"
        self.psu1_id = "0x2f"
        self.psu2_id = "0x39"
        PsuBase.__init__(self)

    def run_command(self, command):
        proc = subprocess.Popen(command, shell=True, universal_newlines=True, stdout=subprocess.PIPE)
        (out, err) = proc.communicate()

        if proc.returncode != 0:
            sys.exit(proc.returncode)

        return out

    def find_value(self, in_string):
        result = re.search("^.+ ([0-9a-f]{2}) .+$", in_string)
        if result:
            return result.group(1)
        else:
            return result

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

        psu_id = self.psu1_id if index == 1 else self.psu2_id
        res_string = self.run_command(self.ipmi_raw + ' ' + psu_id)
        status_byte = self.find_value(res_string)

        if status_byte is None:
            return False

        failure_detected = (int(status_byte, 16) >> 1) & 1
        input_lost = (int(status_byte, 16) >> 3) & 1
        if failure_detected or input_lost:
            return False
        else:
            return True

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by 1-based index <index>
        :param index: An integer, 1-based index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        if index is None:
            return False

        psu_id = self.psu1_id if index == 1 else self.psu2_id
        res_string = self.run_command(self.ipmi_raw + ' ' + psu_id)
        status_byte = self.find_value(res_string)

        if status_byte is None:
            return False

        presence = (int(status_byte, 16) >> 0) & 1
        if presence:
            return True
        else:
            return False
