#!/usr/bin/env python

try:
    import os
    import subprocess
    import logging
    from sonic_platform_base.component_base import ComponentBase

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


OS_SYSTEM_SUCCESS = 0

MAIN_BIOS_INDEX = 0

COMPONENT_NAME_LIST = [
    "Main BIOS",
]

COMPONENT_DESC_LIST = [
    "Main BIOS",
]


class Component(ComponentBase):

    def __get_attr_value(self, attr_path):

        retval = 'ERR'
        if not os.path.isfile(attr_path):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open file: %s", attr_path)

        retval = retval.rstrip(' \t\n\r')
        return retval

    def __set_attr_value(self, attr_path, value):
        try:
            with open(attr_path, 'r+') as reg_file:
                reg_file.write(value)
        except IOError as e:
            logging.error("Error: unable to open file: %s", str(e))
            return False

        return True

    def __get_bios_version(self):
        """
        Retrieves the firmware version of the BIOS
        Returns:
        A string containing the firmware version of the BIOS
        """
        try:
            cmd = ['dmidecode', '-s', 'bios-version']
            if os.geteuid() != 0:
                cmd.insert(0, 'sudo')
            return subprocess.check_output(cmd).strip().decode()
        except subprocess.CalledProcessError as e:
            raise RuntimeError("Failed to get BIOS version")

    __get_version_callback_list = {
        MAIN_BIOS_INDEX: __get_bios_version,
    }

    def __init__(self, component_index):
        self.index = component_index

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
        return COMPONENT_DESC_LIST[self.index]

    def get_firmware_version(self):
        """
        Retrieves the firmware version of the component
        Returns:
            A string containing the firmware version of the component
        """
        return self.__get_version_callback_list[self.index](self)

    def install_firmware(self, image_path):
        """
        Installs firmware to the component
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install was successful, False if not
        """
        return self.__install_firmware_callback_list[self.index](self, image_path)
