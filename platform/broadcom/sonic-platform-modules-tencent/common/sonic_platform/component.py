#!/usr/bin/env python3

########################################################################
# Ruijie B6510-48VS8CQ
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
# the platform
#
########################################################################

try:
    import time
    from sonic_platform_base.component_base import ComponentBase
    from sonic_platform.logger import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Component(ComponentBase):
    """Ruijie Platform-specific Component class"""

    def __init__(self, interface_obj, index):
        self.cpld_dict = {}
        self.int_case = interface_obj
        self.index = index
        self.update_time = 0
        self.cpld_id = "CPLD" + str(index)

    def cpld_dict_update(self):
        local_time = time.time()
        if not self.cpld_dict or (local_time - self.update_time) >= 1:  # update data every 1 seconds
            self.update_time = local_time
            self.cpld_dict = self.int_case.get_cpld_version_by_id(self.cpld_id)

    def get_name(self):
        """
        Retrieves the name of the component

        Returns:
            A string containing the name of the component
        """
        self.cpld_dict_update()
        return self.cpld_dict["Name"]

    def get_description(self):
        """
        Retrieves the description of the component

        Returns:
            A string containing the description of the component
        """
        self.cpld_dict_update()
        return self.cpld_dict["Desc"]

    def get_firmware_version(self):
        """
        Retrieves the firmware version of the component

        Returns:
            A string containing the firmware version of the component
        """
        self.cpld_dict_update()
        return self.cpld_dict["Version"]

    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        # not supported
        return False

