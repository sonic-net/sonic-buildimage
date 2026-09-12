#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

import subprocess
import re

try:
    from sonic_platform_base.component_base import ComponentBase
    #from helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

COMPONENT_NAME = 0
COMPONENT_DESC = 1
COMPONENT_VER_CMD = 2
COMPONENT_VER_FN = 3
BIOS_VERSION_CMD = ['dmidecode', '-s', 'bios-version']
ONIE_VERSION_CMD = ['cat', '/host/machine.conf']
SWCPLD_VERSION_CMD = ['i2cget', '-y', '-f', '2', '0x32', '0']
SSD_VERSION_CMD = ['smartctl', '-i', '/dev/sda']


class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index, pddf_data=None, pddf_plugin_data=None):
        self.component_list = [["BIOS", "Basic Input/Output System", BIOS_VERSION_CMD, self.__get_cmd_output],\
                               ["ONIE", "Open Network Install Environment", ONIE_VERSION_CMD, self.__get_onie_version],\
                               ["CPLD SW", "CPLD for board functions, watchdog and port control SFP(49-56)", SWCPLD_VERSION_CMD, self.__get_cpld_version],\
                               ["SSD", "Solid State Drive - {}", SSD_VERSION_CMD, self.__get_ssd_version]]

        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.get_name()

    def __get_cmd_output(self):
        cmd = self.component_list[self.index][COMPONENT_VER_CMD]
        version = "N/A"

        try:
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True)
            data = p.communicate()
            version = data[0].strip()
        except IOError:
            pass

        return version

    def __get_onie_version(self):
        version = "N/A"

        ret = re.search(r"(?<=onie_version=).+[^\n]", self.__get_cmd_output())
        if ret != None:
            version = ret.group(0)

        return version

    def __get_ssd_version(self):
        version = "N/A"

        ret = re.search(r"Firmware Version: +(.*)[^\\]", self.__get_cmd_output())
        if ret != None:
            try:
               version = ret.group(1)
            except (IndexError):
                pass

        return version

    def __get_cpld_version(self):
        version = "N/A"

        try:
            ver = int(self.__get_cmd_output(), 16)
            version = "{0}.{1}".format(ver >> 4, ver & 0x0F)
        except (ValueError):
            pass

        return version

    def get_name(self):
        """
        Retrieves the name of the component
         Returns:
            A string containing the name of the component
        """
        return self.component_list[self.index][COMPONENT_NAME]

    def __get_ssd_desc(self, desc_format):
        description = "N/A"

        ret = re.search(r"Device Model: +(.*)[^\\]", self.__get_cmd_output())
        if ret != None:
            try:
               description = desc_format.format(ret.group(1))
            except (IndexError):
                pass

        return description

    def get_description(self):
        """
        Retrieves the description of the component
            Returns:
            A string containing the description of the component
        """
        # For SSD get the model name from device
        if self.get_name() == "SSD":
            return self.__get_ssd_desc(self.component_list[self.index][COMPONENT_DESC])

        return self.component_list[self.index][COMPONENT_DESC]

    def get_firmware_version(self):
        """
        Retrieves the firmware version of module
        Returns:
            string: The firmware versions of the module
        """
        fw_version = None
        fw_version = self.component_list[self.index][COMPONENT_VER_FN]()

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

    def get_model(self):
        return 'N/A'

    def get_position_in_parent(self):
        return -1

    def get_presence(self):
        return True

    def get_serial(self):
        return 'N/A'

    def get_status(self):
        return True

    def is_replaceable(self):
        return False
