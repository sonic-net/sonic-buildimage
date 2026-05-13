"""
ssd_util.py
Platform-specific nvme supported SSD interface for SONiC
"""

from subprocess import Popen, PIPE, CalledProcessError
from re import findall
from os.path import exists
from sonic_platform_base.sonic_ssd.ssd_base import SsdBase

NOT_AVAILABLE = "N/A"


class SsdUtil(SsdBase):

    def __init__(self, disk_dev):
        """
        Constructor
        Args:
            disk_dev: Linux device name to get parameters for
        """

        disk_dev = "/dev/nvme0"
        super().__init__(disk_dev)
        if not isinstance(disk_dev, str):
            raise TypeError("disk dev type wrong {}".format(type(disk_dev)))

        if not exists(disk_dev):
            raise RuntimeError("disk dev {} not found".format(disk_dev))

        self.model = NOT_AVAILABLE
        self.serial = NOT_AVAILABLE
        self.firmware = NOT_AVAILABLE
        self.temperature = NOT_AVAILABLE
        self.health = NOT_AVAILABLE

        inno_disk = ["smartctl", "-a", disk_dev]
        self.ssd_info = self._execute_shell(inno_disk)

        self.model = self._parse_re(r'Model Number:\s*(.+?)\n', self.ssd_info)
        self.serial = self._parse_re(r'Serial Number:\s*(.+?)\n', self.ssd_info)
        self.firmware = self._parse_re(r'Firmware Version:\s*(.+?)\n', self.ssd_info)
        temp_temperature = self._parse_re(r'Temperature:\s*(.+?)\n', self.ssd_info)
        self.temperature = self._parse_re(r"^([\S]+)", temp_temperature)
        temp_health = self._parse_re(r'Percentage Used:\s*(.+?)\n', self.ssd_info)
        self.health = (100 - int(float(temp_health.replace("%", ""))))

    @staticmethod
    def _execute_shell(cmd):
        try:
            output = ""
            process = Popen(cmd, universal_newlines=True, stdout=PIPE)
            output, _ = process.communicate()
        except CalledProcessError as err:
            print("Exception when calling smartctl -> %s\n" %(err))
            pass
        return output

    @staticmethod
    def _parse_re(pattern, buffer):
        res_list = findall(pattern, buffer)
        return res_list[0] if res_list else NOT_AVAILABLE

    def get_health(self):
        """
        Retrieves current disk health in percentages
        Returns:
            A float number of current ssd health
            e.g. 83.5
        """
        return self.health

    def get_temperature(self):
        """
        Retrieves current disk temperature in Celsius
        Returns:
            A float number of current temperature in Celsius
            e.g. 40.1
        """
        return self.temperature

    def get_model(self):
        """
        Retrieves model for the given disk device
        Returns:
            A string holding disk model as provided by the manufacturer
        """
        return self.model

    def get_firmware(self):
        """
        Retrieves firmware version for the given disk device
        Returns:
            A string holding disk firmware version as provided by the manufacturer
        """
        return self.firmware

    def get_serial(self):
        """
        Retrieves serial number for the given disk device
        Returns:
            A string holding disk serial number as provided by the manufacturer
        """
        return self.serial

    def get_vendor_output(self):
        """
        Retrieves vendor specific data for the given disk device
        Returns:
            A string holding some vendor specific disk information
        """
        return self.ssd_info
