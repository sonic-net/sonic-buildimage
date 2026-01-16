#!/usr/bin/env python

########################################################################
# Clounix
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
# the platform
#
########################################################################

try:
    import os
    from sonic_platform_base.component_base import ComponentBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

BIOS_QUERY_VERSION_COMMAND = "dmidecode -s bios-version"


class Component(ComponentBase):
    """Clounix Platform-specific Component class"""
    def __init__(self, component_index,component_conf):
        self.__index = component_index
        self.__conf = component_conf
        self.__api_helper = APIHelper()

    def get_name(self):
        """
        Retrieves the name of the component

        Returns:
            A string containing the name of the component
        """
        return self.__conf[self.__index]['name']

    def get_description(self):
        """
        Retrieves the description of the component

        Returns:
            A string containing the description of the component
        """
        return self.__conf[self.__index]['description']

    def get_firmware_version(self):
        """
        Retrieves the firmware version of the component

        Returns:
            A string containing the firmware version of the component
        """
        firmware_version = 'N/A'
        attr_rv = self.__api_helper.read_one_line_file(self.__conf[self.__index]['firmware_version_path'])
        if attr_rv != None:
            firmware_version = attr_rv
        else:
            if self.get_name == 'BIOS':
                ret,bios_ver = self.__api_helper.run_command(BIOS_QUERY_VERSION_COMMAND)
                if ret:
                    if bios_ver:
                        firmware_version = bios_ver
        return firmware_version

    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        if self.get_name == 'BIOS':
            cmd = "/usr/local/bin/install_firmware.sh bios "+ image_path
        else:
            print(self.get_name + 'frimware upgrade did not implement')

        os.system(cmd)
        return True

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
        model = 'N/A'
        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        serial = 'N/A'
        return serial

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether Fan is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
