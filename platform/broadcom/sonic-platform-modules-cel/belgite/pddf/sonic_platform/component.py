#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

import subprocess

try:
    from sonic_platform_base.component_base import ComponentBase
    from sonic_py_common.general import getstatusoutput_noshell_pipe
    #from helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SWCPLD_VERSION_PATH = ["i2cget", "-y", "-f", "2", "0x32", "0"]
BIOS_VERSION_PATH_CMD1 = ["dmidecode", "-t", "bios"]
BIOS_VERSION_PATH_CMD2 = ["grep", "Version"]
COMPONENT_NAME_LIST = ["SWCPLD", "Main_BIOS", "Backup_BIOS"]
COMPONENT_DES_LIST = ["Use for boot control and BIOS switch",
                      "Main basic Input/Output System",
                      "Backup basic Input/Output System"]


class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
        ComponentBase.__init__(self)
        self.index = component_index
        #self._api_helper = APIHelper()
        self.name = self.get_name()

    def run_command(self,cmd):
        responses = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True).stdout.read()
        return responses

    def __get_bios_version(self):
        # Retrieves the BIOS firmware version
        result = self.run_command(["i2cget", "-y", "-f", "2", "0x32", "0x19"])
        if result.strip() == "0x01":
            if self.name == "Main_BIOS":
                _, version = getstatusoutput_noshell_pipe(BIOS_VERSION_PATH_CMD1, BIOS_VERSION_PATH_CMD2)
                bios_version = version.strip().split(" ")[1]
                return str(bios_version)
            elif self.name == "Backup_BIOS":
                bios_version = "na"
                return bios_version
                
        elif result.strip() == "0x03":
            if self.name == "Backup_BIOS":
                _, version = getstatusoutput_noshell_pipe(BIOS_VERSION_PATH_CMD1, BIOS_VERSION_PATH_CMD2)
                bios_version = version.strip().split(" ")[1]
                return str(bios_version)
            elif self.name == "Main_BIOS":
                bios_version = "na"
                return bios_version

    def __get_cpld_version(self):
        if self.name == "SWCPLD":
            ver = self.run_command(SWCPLD_VERSION_PATH)
            print("ver is %s" % ver)
            ver = ver.strip().split("x")[1]
            print("ver2 is %s" % ver)
            version = int(ver.strip()) / 10
            return str(version)

                
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
        
        if "BIOS" in self.name:
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
