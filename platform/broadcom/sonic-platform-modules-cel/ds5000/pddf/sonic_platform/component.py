#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

import os.path
import re

try:
    from sonic_platform_base.component_base import ComponentBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FPGA_VERSION_PATH = "/sys/devices/platform/cls_sw_fpga/FPGA/version"
UNKNOWN_VER = "Unknown"

BMC_EXIST = APIHelper().get_bmc_status()

COMPONENT_LIST = [
    ("BIOS",         "Basic Input/Output System"),
    ("ONIE",         "Open Network Install Environment"),
    ("FPGA",         "FPGA for transceiver EEPROM access and other component I2C access"),
    ("CPLD COMe",    "COMe board CPLD"),
    ("CPLD BASE",    "CPLD for board functions and watchdog"),
    ("CPLD SW1",     "CPLD for port control OSFP(1-32)"),
    ("CPLD SW2",     "CPLD for port control OSFP(33-64)"),
    ("SSD",          "Solid State Drive firmware"),
]

if BMC_EXIST:
    COMPONENT_LIST.insert(2,("BMC","Baseboard Management Controller"))

class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.get_name()
        self._api_helper = APIHelper()

    def __get_switch_cpld1_ver(self):
        cmd = "cat /sys/devices/platform/cls_sw_fpga/CPLD1/version"
        status, sw_cpld1_ver = self._api_helper.run_command(cmd)
        if status:
           return sw_cpld1_ver
        else:
           return UNKNOWN_VER

    def __get_switch_cpld2_ver(self):
        cmd = "cat /sys/devices/platform/cls_sw_fpga/CPLD2/version"
        status, sw_cpld2_ver = self._api_helper.run_command(cmd)
        if status:
           return sw_cpld2_ver
        else:
           return UNKNOWN_VER

    def __get_base_cpld_ver(self):
        status, ver = self._api_helper.cpld_lpc_read(0xA100)
        if status:
           return ver
        else:
           return UNKNOWN_VER

    def __get_come_cpld_ver(self):
        status, ver = self._api_helper.cpld_lpc_read(0xA1E0)
        if status:
           return ver
        else:
           return UNKNOWN_VER

    def __get_bmc_ver(self):
        ver = "Unknown"
        cmd = "ipmitool mc info | grep 'Firmware Revision'"
        status, result = self._api_helper.run_command(cmd)
        if status:
            ver_data = result.split(":")
            ver = ver_data[-1].strip() if len(ver_data) > 1 else ver
            return ver 
        else:
           return "N/A"

    def __get_ssd_ver(self):
        ver = "Unknown"
        cmd = "ssdutil -v | grep 'Firmware'"
        status, result = self._api_helper.run_command(cmd)
        if status:
            ver_data = result.split(":")
            ver = ver_data[-1].strip() if len(ver_data) > 1 else ver
        return ver

    def __get_onie_ver(self):
        ver = "Unknown"
        cmd = "cat /host/machine.conf | grep 'onie_version'"
        status, result = self._api_helper.run_command(cmd)
        if status:
            ver_data = result.split("=")
            ver = ver_data[-1].strip() if len(ver_data) > 1 else ver
        return ver

    def __get_fpga_ver(self):
        status, fpga_version = self._api_helper.run_command("cat %s" % FPGA_VERSION_PATH)
        if not status:
            return UNKNOWN_VER
        return fpga_version.replace("0x", "")

    def __get_bios_ver(self):
        cmd = "dmidecode -s bios-version"
        status, result = self._api_helper.run_command(cmd)
        return result

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
        elif "CPLD" in self.name:
            fw_version = self.__get_cpld_ver()
        elif self.name == "FPGA":
            fw_version = self.__get_fpga_ver()
        elif "BMC" in self.name:
            fw_version = self.__get_bmc_ver()
        elif "SSD" in self.name:
            fw_version = self.__get_ssd_ver()
        return fw_version

    def __get_cpld_ver(self):
        version = "N/A"
        cpld_version_dict = dict()
        cpld_ver_info = {
            'CPLD BASE': self.__get_base_cpld_ver(),
            'CPLD SW1': self.__get_switch_cpld1_ver(),
            'CPLD SW2': self.__get_switch_cpld2_ver(),
            'CPLD COMe': self.__get_come_cpld_ver(),
        }
        if self.name in cpld_ver_info.keys():
            version = cpld_ver_info[self.name]
            ver = version.lstrip("0x")
            version1 = int(ver.strip()) / 10
            version2 = int(ver.strip()) % 10
            version = "%d.%d" % (version1, version2)
            return version

    @staticmethod
    def install_firmware(image_path):
        """
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """
        return False

    @staticmethod
    def update_firmware(image_path):
        # Not support
        return False
 