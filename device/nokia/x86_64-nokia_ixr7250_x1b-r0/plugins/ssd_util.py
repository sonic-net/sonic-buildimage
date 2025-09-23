#
# ssd_util.py
#

try:
    import re
    import subprocess
    from sonic_platform_base.sonic_storage.storage_base import StorageBase
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

NOT_AVAILABLE = "N/A"
SMARTCTL = "smartctl {} -a"
GENERIC_HEALTH_ID = 169
PHISON_HEALTH_ID = 231

class SsdUtil(StorageBase):
    """
    Generic implementation of the SSD health API
    """
    model = NOT_AVAILABLE
    serial = NOT_AVAILABLE
    firmware = NOT_AVAILABLE
    temperature = NOT_AVAILABLE
    health = NOT_AVAILABLE
    ssd_info = NOT_AVAILABLE
    vendor_ssd_info = NOT_AVAILABLE

    def __init__(self, diskdev):

        self.vendor_ssd_utility = {
            "Generic"  : { "utility" : SMARTCTL, "parser" : self.parse_generic_ssd_info }
        }

        self.dev = diskdev
        self.fetch_parse_info(diskdev)

    def fetch_parse_info(self, diskdev):
        self.fetch_generic_ssd_info(diskdev)
        self.parse_generic_ssd_info()

    def _execute_shell(self, cmd):
        process = subprocess.Popen(cmd.split(), universal_newlines=True, stdout=subprocess.PIPE)
        output, error = process.communicate()
        return output

    def _parse_re(self, pattern, buffer):
        res_list = re.findall(pattern, buffer)
        return res_list[0] if res_list else NOT_AVAILABLE

    def fetch_generic_ssd_info(self, diskdev):
        self.ssd_info = self._execute_shell(self.vendor_ssd_utility["Generic"]["utility"].format(diskdev))

    def parse_generic_ssd_info(self):
        self.model = self._parse_re('Device Model:\s*(.+?)\n', self.ssd_info)
        if self.model.startswith('VTSM'):
            health_id = PHISON_HEALTH_ID
        else:
            health_id = GENERIC_HEALTH_ID

        health_raw = self.parse_id_number(health_id, self.ssd_info)
        if health_raw == NOT_AVAILABLE:
            self.health = NOT_AVAILABLE
        else: self.health = health_raw.split()[-1]

        temp_raw = self._parse_re('Temperature_Celsius\s*(.+?)\n', self.ssd_info)
        if temp_raw == NOT_AVAILABLE:
            self.temperature = NOT_AVAILABLE
        else:
            self.temperature = temp_raw.split()[7].split()[0]

        self.serial = self._parse_re('Serial Number:\s*(.+?)\n', self.ssd_info)
        self.firmware = self._parse_re('Firmware Version:\s*(.+?)\n', self.ssd_info)


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
        return self.vendor_ssd_info

    def parse_id_number(self, id, buffer):
        if buffer:
            buffer_lines = buffer.split('\n')
            for line in buffer_lines:
                if line.strip().startswith(str(id)):
                    return line[len(str(id)):]

        return NOT_AVAILABLE
