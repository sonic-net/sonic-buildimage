#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

import re

try:
    from sonic_platform_base.component_base import ComponentBase
    from sonic_platform.helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

BMC_EXIST = APIHelper().get_bmc_status()

BIOS_VERSION_CMD = ['dmidecode', '-s', 'bios-version']
ONIE_VERSION_CMD = ['cat', '/host/machine.conf']
BMC_VERSION_CMD = ['ipmitool', 'mc', 'info']
SSD_VERSION_CMD = ['smartctl', '-i', '/dev/sda']

COMPONENT_LIST = [
    ("BIOS",       "Basic Input/Output System"),
    ("ONIE",       "Open Network Install Environment"),
    ("CPLD COMe",  "COMe board CPLD"),
    ("CPLD SW1",   "CPLD1 for board functions and watchdog"),
    ("CPLD SW2",   "CPLD2 for port led control"),
    ("SSD",        "Solid State Drive firmware"),
]

if BMC_EXIST:
    COMPONENT_LIST.insert(2,("BMC", "Baseboard Management Controller"))

class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.get_name()
        self._api_helper = APIHelper()

    def __get_come_cpld_ver(self):
        status, data = self._api_helper.cpld_lpc_read(0xA1E0, 2)
        if status:
           ver = int(data.strip(), 16)
           version = "{}.{}".format(ver >> 4 & 0xF, ver & 0xF)
           return version
        else:
           return "N/A"

    def __get_cpld1_ver(self):
        status, data = self._api_helper.cpld_lpc_read(0xA200, 1)
        if status:
           ver = int(data.strip(), 16)
           version = "{}.{}".format(ver >> 4 & 0xF, ver & 0xF)
           return version
        else:
           return "N/A"

    def __get_cpld2_ver(self):
        status, data = self._api_helper.cpld_lpc_read(0xA100, 2)
        if status:
           ver = int(data.strip(), 16)
           version = "{}.{}".format(ver >> 4 & 0xF, ver & 0xF)
           return version
        else:
           return "N/A"

    def __get_bios_ver(self):
        status, data = self._api_helper.run_command(BIOS_VERSION_CMD)
        if status:
           version = data.strip()
           return version
        else:
           return "N/A"

    def __get_bmc_ver(self):
        version = "N/A"
        status, data =  self._api_helper.run_command(BMC_VERSION_CMD)
        if status:
            ret = re.search(r"Firmware Revision\s*:\s*(.+)", data)
            if ret:
                version = ret.group(1)
        return version

    def __get_onie_ver(self):
        version = "N/A"
        status, data =  self._api_helper.run_command(ONIE_VERSION_CMD)
        if status:
           ret = re.search(r"(?<=onie_version=).+", data)
           if ret:
              version = ret.group(0)
        return version

    def __get_cpld_ver(self):
        version = "N/A"
        cpld_ver_info = {
            'CPLD COMe': self.__get_come_cpld_ver(),
            'CPLD SW1': self.__get_cpld1_ver(),
            'CPLD SW2': self.__get_cpld2_ver()
        }
        if self.name in cpld_ver_info.keys():
            version = cpld_ver_info[self.name]
            return version

    def __get_ssd_ver(self):
        version = "N/A"
        status, data = self._api_helper.run_command(SSD_VERSION_CMD)
        if status:
            ret = re.search(r"Firmware Version:\s*(.+)", data)
            if ret:
                version = ret.group(1)
        return version

    def get_name(self):
        """
        Retrieves the name of the component
         Returns:
            A string containing the name of the component
        """
        return COMPONENT_LIST[self.index][0]

    def get_description(self):
        """
        Retrieves the description of the component
            Returns:
            A string containing the description of the component
        """
        return COMPONENT_LIST[self.index][1]

    def get_firmware_version(self):
        """
        Retrieves the firmware version of module
        Returns:
            string: The firmware versions of the module
        """
        fw_version = None

        if "BIOS" in self.name:
            fw_version = self.__get_bios_ver()
        elif "ONIE" in self.name:
            fw_version = self.__get_onie_ver()
        elif "BMC" in self.name:
            fw_version = self.__get_bmc_ver()
        elif "CPLD" in self.name:
            fw_version = self.__get_cpld_ver()
        elif "SSD" in self.name:
            fw_version = self.__get_ssd_ver()

        return fw_version

    def install_firmware(self, image_path):
        """
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """
        return False

    def update_firmware(self, image_path):
        return False

    def get_available_firmware_version(self, image_path):
        return 'N/A'

    def get_firmware_update_notification(self, image_path):
        return "None"
