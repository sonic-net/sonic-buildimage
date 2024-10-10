#
# ssd_util.py
#
# Generic implementation of the SSD health API
# SSD models supported:
#  - InnoDisk
#  - StorFly
#  - Virtium

try:
    import subprocess
    from sonic_platform_base.sonic_ssd.ssd_base import SsdBase
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

HEALTH_CMD = "cat /sys/kernel/debug/mmc0/mmc0:0001/ext_csd | cut -c 537-538"
SERIAL_CMD = "cat /sys/bus/mmc/devices/mmc0\\:0001/serial"
FIRMWARE_CMD = "cat /sys/kernel/debug/mmc0/mmc0:0001/ext_csd | cut -c 509-522"
NOT_AVAILABLE = "N/A"

class SsdUtil(SsdBase):
    """
    Generic implementation of the SSD health API
    """
    def __init__(self, diskdev):
        self.model = "KLMCG4JETD-B041"
        self.temperature = NOT_AVAILABLE
        self.vendor_ssd_info = "====No vendor information===="
        self.health_list = [100,90,80,70,60,50,40,30,20,10,0]
        try:
            life_time = self._execute_shell(HEALTH_CMD)
            if int(life_time) in range(1,12):
                self.health = self.health_list[int(life_time) - 1]
            else:
                self.health = NOT_AVAILABLE
        except Exception as e:
            self.health = NOT_AVAILABLE

        try:
            self.firmware = self._execute_shell(FIRMWARE_CMD)
        except Exception as e:
            self.firmware = NOT_AVAILABLE

        try:
            serial = self._execute_shell(SERIAL_CMD)
            self.serial = serial.replace("0x",'')
        except Exception as e:
            self.serial = NOT_AVAILABLE

    def _execute_shell(self, cmd):
        status, output = subprocess.getstatusoutput(cmd)
        if status:
            return None

        return output

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

