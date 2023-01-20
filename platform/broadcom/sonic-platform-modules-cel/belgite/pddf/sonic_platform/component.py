#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

import subprocess
import time

try:
    from sonic_platform_base.component_base import ComponentBase
    #from helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SWCPLD_VERSION_PATH = ['i2cget', '-y', '-f', '2', '0x32', '0']
BIOS_VERSION_PATH = ['dmidecode', '-s', 'bios-version']
COMPONENT_NAME_LIST = ["SWCPLD", "BIOS"]
COMPONENT_DES_LIST = ["Used for managing the chassis and SFP+ ports (49-56)",
                      "Basic Input/Output System"]


class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.get_name()

    def __get_bios_version(self):
        # Retrieves the BIOS firmware version
        version = "N/A"

        try:
            p = subprocess.Popen(BIOS_VERSION_PATH, stdout=subprocess.PIPE, universal_newlines=True)
            data = p.communicate()
            version = data[0].strip()
        except IOError:
            pass

        return version

    def __get_cpld_version(self):
        version = "N/A"
        try:
            p = subprocess.Popen(SWCPLD_VERSION_PATH, stdout=subprocess.PIPE, universal_newlines=True)
            data = p.communicate()
            ver = int(data[0].strip(), 16)
            version = "{0}.{1}".format(ver >> 4, ver & 0x0F)
        except (IOError, ValueError):
            pass

        return version
                
    def get_name(self):
        """
        Retrieves the name of the component
         Returns:
            A string containing the name of the component
        """
        return COMPONENT_NAME_LIST[self.index]

    def get_description(self):
        """
        Retrieves the description of the component
            Returns:
            A string containing the description of the component
        """
        return COMPONENT_DES_LIST[self.index]

    def get_firmware_version(self):
        """
        Retrieves the firmware version of module
        Returns:
            string: The firmware versions of the module
        """
        fw_version = None
        
        if self.name == "BIOS":
            fw_version = self.__get_bios_version()
        elif "CPLD" in self.name:
            fw_version = self.__get_cpld_version()
            
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
