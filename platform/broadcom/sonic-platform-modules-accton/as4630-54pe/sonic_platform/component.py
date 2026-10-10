#!/usr/bin/env python

#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the component information (such as CPLD, FPGA, BIOS etc) which are available in the platform
#
#############################################################################

try:
    import os
    import json
    from sonic_platform_base.component_base import ComponentBase
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

BIOS_VERSION_PATH = "/sys/class/dmi/id/bios_version"
#COMPONENT_NAME_LIST = ["CPLD1"]
COMPONENT_LIST =  [
   ("CPLD1", "CPLD 1"),
   ("BIOS", "Basic Input/Output System")
]

class Component(ComponentBase):
    """Platform-specific Component class"""


    def __init__(self, component_index):
        self.index = component_index
        self.name = self.get_name()
        self.cpld_mapping = {
                0: ['3', '0x60']                
            }


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
    
    def __get_bios_version(self):
        # Retrieves the BIOS firmware version
        try:
            with open(BIOS_VERSION_PATH, 'r') as fd:
                bios_version = fd.read()
                return bios_version.strip()
        except Exception as e:
            return None
    
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
            cmd = ["i2cget", "-f", "-y", self.cpld_mapping[self.index][0], self.cpld_mapping[self.index][1], "0x1"]
            status, value = getstatusoutput_noshell(cmd)
            if not status:
                fw_version = value.rstrip()

        return fw_version

    def install_firmware(self, image_path):
        """
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """
        ret, output = getstatusoutput_noshell(["tar", "-C", "/tmp", "-xzf", image_path ] )
        if ret != 0 :
            print("Installation failed because of wrong image package")
            return False

        if os.path.exists("/tmp/install.json") is False:
            print("Installation failed without jsonfile")
            return False

        input_file = open ('/tmp/install.json')
        json_array = json.load(input_file)
        ret = 1
        for item in json_array:
            if item.get('id')==None or item.get('path')==None:
                continue
            if self.name == item['id'] and item['path'] and item.get('cpu'):
                print( "Find", item['id'], item['path'], item['cpu'] )
                ret, output = getstatusoutput_noshell(["/tmp/run_install.sh", item['id'], item['path'], item['cpu'] ])
                if ret==0:
                    break
            elif self.name == item['id'] and item['path']:
                print( "Find", item['id'], item['path'] )
                ret, output = getstatusoutput_noshell(["/tmp/run_install.sh", item['id'], item['path'] ])
                if ret==0:
                    break

        if ret==0:
            return True
        else :
            return False

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return 'N/A'

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of
        entPhysicalContainedIn is'0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device
            or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
