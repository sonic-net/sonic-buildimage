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
    #from sonic_platform_base.component_base import ComponentBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NAME_IDX = 0
DESC_IDX = 1
VER_API_IDX = 2

BASE_CPLD_PLATFORM = "sys_cpld"
SW_CPLD_PLATFORM = "questone2"
PLATFORM_SYSFS_PATH = "/sys/devices/platform/"

FPGA_GETREG_PATH = "{}/{}/FPGA/getreg".format(
    PLATFORM_SYSFS_PATH, SW_CPLD_PLATFORM)
BASE_GETREG_PATH = "{}/{}/getreg".format(
    PLATFORM_SYSFS_PATH, BASE_CPLD_PLATFORM)
SW_CPLD1_GETREG_PATH = "{}/{}/CPLD1/getreg".format(
    PLATFORM_SYSFS_PATH, SW_CPLD_PLATFORM)
SW_CPLD2_GETREG_PATH = "{}/{}/CPLD2/getreg".format(
    PLATFORM_SYSFS_PATH, SW_CPLD_PLATFORM)
BIOS_VER_PATH = "/sys/class/dmi/id/bios_version"

BASE_CPLD_VER_REG = "0xA100"
COME_CPLD_VER_REG = "0xA1E0"
SW_CPLD_VER_REG = "0x00"
FPGA_VER_REG = "0x00"

UNKNOWN_VER = "N/A"
ONIE_VER_CMD = "cat /host/machine.conf"
SSD_VER_CMD = "smartctl -i /dev/sda"

class Component():
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
        COMPONENT_LIST = [
            ("BIOS",      "Basic input/output System", self.__get_bios_ver),
            ("ONIE",      "Open Network Install Environment", self.__get_onie_ver),
            ("BMC",       "Baseboard Management Controller", self.__get_bmc_ver),
            ("FPGA",      "FPGA for transceiver EEPROM access and other component I2C access", self.__get_fpga_ver),
            ("CPLD COMe", "COMe board CPLD", self.__get_cpld_ver),
            ("CPLD BASE", "CPLD for board functions, fan control and watchdog", self.__get_cpld_ver),
            ("CPLD SW1",  "CPLD for port control SFP(1-24)", self.__get_cpld_ver),
            ("CPLD SW2",  "CPLD for port control SFP(25-48) QSFP(49-56)", self.__get_cpld_ver),
            ("SSD",       "Solid State Drive - {}", self.__get_ssd_ver),
        ]

        self.index = component_index
        self.name = COMPONENT_LIST[self.index][NAME_IDX]
        self.description = COMPONENT_LIST[self.index][DESC_IDX]
        self.__get_version = COMPONENT_LIST[self.index][VER_API_IDX]
        self._api_helper = APIHelper()

    def __get_bios_ver(self):
        return self._api_helper.read_one_line_file(BIOS_VER_PATH)

    def __get_onie_ver(self):
        onie_ver = "N/A"
        status, raw_onie_data = self._api_helper.run_command(ONIE_VER_CMD)
        if status:
            ret = re.search(r"(?<=onie_version=).+[^\n]", raw_onie_data)
            if ret != None:
                onie_ver = ret.group(0)
        return onie_ver

    def __get_bmc_ver(self):
        cmd="ipmitool mc info | grep 'Firmware Revision'"
        status, raw_ver=self._api_helper.run_command(cmd)
        if status:
            bmc_ver=raw_ver.split(':')[-1].strip()
            return bmc_ver
        else:
            return UNKNOWN_VER

    def __get_fpga_ver(self):
        version_raw = self._api_helper.get_register_value(FPGA_GETREG_PATH, FPGA_VER_REG)
        return "{}.{}".format(int(version_raw[2:][:4], 16), int(version_raw[2:][4:], 16)) if version_raw else UNKNOWN_VER

    def __get_cpld_ver(self):
        cpld_api_param = {
            'CPLD COMe': (BASE_GETREG_PATH, COME_CPLD_VER_REG),
            'CPLD BASE': (BASE_GETREG_PATH, BASE_CPLD_VER_REG),
            'CPLD SW1': (SW_CPLD1_GETREG_PATH, SW_CPLD_VER_REG),
            'CPLD SW2': (SW_CPLD2_GETREG_PATH, SW_CPLD_VER_REG),
        }
        api_param = cpld_api_param[self.name]

        cpld_ver = self._api_helper.get_register_value(api_param[0], api_param[1])
        return "{}.{}".format(int(cpld_ver[2], 16), int(cpld_ver[3], 16)) if cpld_ver else UNKNOWN_VER

    def __get_ssd_ver(self):
        ssd_ver = "N/A"
        status, raw_ssd_data = self._api_helper.run_command(SSD_VER_CMD)
        if status:
            ret = re.search(r"Firmware Version: +(.*)[^\\]", raw_ssd_data)
            if ret != None:
                ssd_ver = ret.group(1)
        return ssd_ver

    def __get_ssd_model(self):
        model = "N/A"

        status, raw_ssd_data = self._api_helper.run_command(SSD_VER_CMD)
        if status:
            ret = re.search(r"Device Model: +(.*)[^\\]", raw_ssd_data)
            if ret != None:
                try:
                    model = ret.group(1)
                except (IndexError):
                    pass
        return model

    def get_name(self):
        """
        Retrieves the name of the component
         Returns:
            A string containing the name of the component
        """
        return self.name

    def get_description(self):
        """
        Retrieves the description of the component
            Returns:
            A string containing the description of the component
        """
        # For SSD get the model name from device
        if self.name == "SSD":
            return self.description.format(self.__get_ssd_model())

        return self.description

    def get_firmware_version(self):
        """
        Retrieves the firmware version of module
        Returns:
            string: The firmware versions of the module
        """
        return self.__get_version()
