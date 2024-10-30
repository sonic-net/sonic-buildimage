#
# ssd_health
#

from sonic_platform_base.sonic_storage.storage_base import StorageBase
from subprocess import Popen, PIPE
from re import findall
from os.path import exists

NOT_AVAILABLE = "N/A"

class SsdUtil(StorageBase):

    def __init__(self, diskdev):
        """
        Constructor
        Args:
            diskdev: Linux device name to get parameters for
        """
        if not isinstance(diskdev, str):
            raise TypeError("disk dev type wrong {}".format(type(diskdev)))

        if not exists(diskdev):
            raise RuntimeError("disk dev {} not found".format(diskdev))

        self.model = NOT_AVAILABLE
        self.serial = NOT_AVAILABLE
        self.firmware = NOT_AVAILABLE
        self.temperature = NOT_AVAILABLE
        self.health = NOT_AVAILABLE

        INNODISK = ["iSmart", "-d", diskdev]
        self.ssd_info = self._execute_shell(INNODISK)

        self.model = self._parse_re(r'Model Name:\s*(.+?)\n', self.ssd_info)
        self.serial = self._parse_re(r'Serial Number:\s*(.+?)\n', self.ssd_info)
        self.firmware = self._parse_re(r'FW Version:\s*(.+?)\n', self.ssd_info)
        self.temperature = self._parse_re(r'Temperature\s*\[\s*(.+?)\]', self.ssd_info)
        self.health = self._parse_re(r'Health:\s*(.+?)', self.ssd_info)

    def _execute_shell(self, cmd):
        process = Popen(cmd, universal_newlines=True, stdout=PIPE)
        output, _ = process.communicate()
        return output

    def _parse_re(self, pattern, buffer):
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
