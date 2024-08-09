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

COMPONENT_LIST = [
    ("SwitchCPLD1",  "To implement OSFP1-32 control"),
    ("SwitchCPLD2",  "To implement OSFP33-64 control"),
    ("BaseCPLD",     "To implement PSU, thermal, fan, LED, watchdog control"),
    ("ComeCPLD",     "To implement managing power sequence, reset, BIOS selection etc"),
    ("FPGA",         "To implement PCIe to I2C, SFP28 control"),
    ("BIOS",         "Basic Input/Output System"),
    ("ONIE",         "Open Network Install Environment"),
    ("SSD",          "Solid State Drive firmware"),
    ("BMC",          "Baseboard Management Controller"),
]

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
        return sw_cpld1_ver

    def __get_switch_cpld2_ver(self):
        cmd = "cat /sys/devices/platform/cls_sw_fpga/CPLD2/version"
        status, sw_cpld2_ver = self._api_helper.run_command(cmd)
        return sw_cpld2_ver

    def __get_base_cpld_ver(self):
        status, ver = self._api_helper.cpld_lpc_read(0xA100)
        return ver

    def __get_come_cpld_ver(self):
        status, ver = self._api_helper.cpld_lpc_read(0xA1E0)
        return ver

    def __get_bmc_ver(self):
        ver = "Unknown"
        cmd = "ipmitool mc info | grep 'Firmware Revision'"
        status, result = self._api_helper.run_command(cmd)
        if status:
            ver_data = result.split(":")
            ver = ver_data[-1].strip() if len(ver_data) > 1 else ver
        return ver

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
        cmd = "cat /sys/devices/platform/cls_sw_fpga/FPGA/version"
        status, result = self._api_helper.run_command(cmd)
        return result

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
        fw_version = {
            "SwitchCPLD1": self.__get_switch_cpld1_ver(),
            "SwitchCPLD2": self.__get_switch_cpld2_ver(),
            "BaseCPLD": self.__get_base_cpld_ver(),
            "ComeCPLD": self.__get_come_cpld_ver(),
            "FPGA": self.__get_fpga_ver(),
            "BIOS": self.__get_bios_ver(),
            "ONIE": self.__get_onie_ver(),
            "SSD": self.__get_ssd_ver(),
            "BMC": self.__get_bmc_ver(),
        }.get(self.name, "Unknown")

        return fw_version

    def get_available_firmware_version(self, image_path):
        """
        Retrieves the available firmware version of the component

        Note: the firmware version will be read from image

        Args:
            image_path: A string, path to firmware image

        Returns:
            A string containing the available firmware version of the component
        """
        image_path = image_path.lower()

        if self.name == 'FPGA':
            pattern = "v([a-z0-9]+)"
        elif self.name == 'BIOS':
            pattern = "([a-z0-9]+\.[a-z0-9]+\.[a-z0-9]+)"
        elif self.name == 'BaseCPLD':
            pattern = "v([a-z0-9]+)"
        elif self.name == 'SwitchCPLD1' or self.name == 'SwitchCPLD2':
            pattern = "v([a-z0-9]+)"
        else:
            raise NotImplementedError

        if not os.path.isfile(image_path):
            raise NotImplementedError

        m = re.search(pattern, image_path)
        if m and m.group(1):
            if self.name == 'FPGA':
                ver = "0x%s" % format(int(m.group(1), 16),'0>8x')
            elif self.name == 'BIOS':
                ver = m.group(1)
            elif self.name == 'BaseCPLD':
                ver = "0x%s" % format(int(m.group(1), 16),'0>2x')
            elif name == 'SwitchCPLD1' or name == 'SwitchCPLD2':
                ver = "0x%s" % format(int(m.group(1), 16),'0>2x')
        else:
            raise NotImplementedError
        return ver

    def install_firmware(self, image_path):
        """
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """

        if self.name == 'FPGA':
            FW_UPGRADE_CMD = "fpga_prog /sys/bus/pci/devices/0000\:11\:00.0/resource0 %s" % image_path
        elif self.name == 'BIOS':
            FW_UPGRADE_CMD = "afulnx_64 %s /n /b /p /k /x /me" % image_path
        elif self.name == 'BaseCPLD':
            FW_UPGRADE_CMD = "ispvm -f 2900 %s" % image_path
        elif self.name == 'SwitchCPLD1' or self.name == 'SwitchCPLD2':
            FW_UPGRADE_CMD = "ispvm -f 2900 %s" % image_path
        else:
            return False

        if not os.path.isfile(image_path):
            return False

        # print(install_command)
        status, ouput = self._api_helper.run_command(FW_UPGRADE_CMD)
        return status
