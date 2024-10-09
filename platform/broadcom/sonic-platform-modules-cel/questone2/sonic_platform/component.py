#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

import os.path

try:
    #from sonic_platform_base.component_base import ComponentBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

COMPONENT_LIST = [
    ("BIOS",       "Basic input/output System"),
    ("BASE_CPLD",  "Base board CPLD"),
    ("BMC",        "Used for monitoring and managing whole system"),
    ("CPU_CPLD",   "CPU board CPLD"),
    ("SW_CPLD1",   "Switch board CPLD 1"),
    ("SW_CPLD2",   "Switch board CPLD 2"),
    ("FPGA",       "Field-programmable gate array")
]
NAME_INDEX = 0
DESCRIPTION_INDEX = 1

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
#FAN_CPLD_VER_PATH = "/sys/bus/i2c/drivers/fancpld/66-000d/version"

BASE_CPLD_VER_REG = "0xA100"
CPU_CPLD_VER_REG = "0xA1E0"
SW_CPLD_VER_REG = "0x00"
#FPGA_VER_REG = "0x00"

UNKNOWN_VER = "Unknown"
FPGA_UPGRADE_CMD = "fpga_prog {}"
CPLD_UPGRADE_CMD = "ispvm {}"


class Component():
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
        #ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.get_name()
        self._api_helper = APIHelper()

    def __get_fpga_ver(self):
        version_raw = self._api_helper.get_register_value(
            FPGA_GETREG_PATH, '0x00')
        return "{}.{}".format(int(version_raw[2:][:4], 16), int(version_raw[2:][4:], 16)) if version_raw else UNKNOWN_VER

    def __get_cpld_ver(self):
        cpld_version_dict = dict()
        cpld_ver_info = {
            'BASE_CPLD': self._api_helper.get_register_value(BASE_GETREG_PATH, BASE_CPLD_VER_REG),
            'CPU_CPLD': self._api_helper.get_register_value(BASE_GETREG_PATH, CPU_CPLD_VER_REG),
            'SW_CPLD1': self._api_helper.get_register_value(SW_CPLD1_GETREG_PATH, SW_CPLD_VER_REG),
            'SW_CPLD2': self._api_helper.get_register_value(SW_CPLD2_GETREG_PATH, SW_CPLD_VER_REG),
        }
        for cpld_name, cpld_ver in cpld_ver_info.items():
            cpld_ver_str = "{}.{}".format(int(cpld_ver[2], 16), int(
                cpld_ver[3], 16)) if cpld_ver else UNKNOWN_VER
            cpld_version_dict[cpld_name] = cpld_ver_str

        return cpld_version_dict

    def __get_bmc_ver(self):
        cmd="ipmitool mc info | grep 'Firmware Revision'"
        status, raw_ver=self._api_helper.run_command(cmd)
        if status:
            bmc_ver=raw_ver.split(':')[-1].strip()
            return {"BMC":bmc_ver}
        else:
            return {"BMC":"N/A"}

    def get_name(self):
        """
        Retrieves the name of the component
         Returns:
            A string containing the name of the component
        """
        return COMPONENT_LIST[self.index][NAME_INDEX]

    def get_description(self):
        """
        Retrieves the description of the component
            Returns:
            A string containing the description of the component
        """
        return COMPONENT_LIST[self.index][DESCRIPTION_INDEX]

    def get_firmware_version(self):
        """
        Retrieves the firmware version of module
        Returns:
            string: The firmware versions of the module
        """
        fw_version_info = {
            'BIOS': self._api_helper.read_one_line_file(BIOS_VER_PATH),
            'FPGA': self.__get_fpga_ver(),
        }
        fw_version_info.update(self.__get_cpld_ver())
        fw_version_info.update(self.__get_bmc_ver())
        return fw_version_info.get(self.name, UNKNOWN_VER)

    def install_firmware(self, image_path):
        """
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """

        install_command = {
            "BASE_CPLD": CPLD_UPGRADE_CMD.format(image_path),
            "FAN_CPLD": CPLD_UPGRADE_CMD.format(image_path),
            "CPU_CPLD": CPLD_UPGRADE_CMD.format(image_path),
            "SW_CPLD1": CPLD_UPGRADE_CMD.format(image_path),
            "SW_CPLD2": CPLD_UPGRADE_CMD.format(image_path),
            "FPGA": FPGA_UPGRADE_CMD.format(image_path),
        }.get(self.name, None)

        if not os.path.isfile(str(image_path)) or (install_command is None) or (not self._api_helper.is_host()):
            return False

        status = self._api_helper.run_interactive_command(install_command)
        return status
