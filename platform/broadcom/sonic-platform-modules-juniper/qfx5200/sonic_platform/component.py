#!/usr/bin/env python
#
# SONiC Platform API implementation for firmware components
#
try:
    from sonic_platform_base.component_base import ComponentBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

BIOS_VERSION_PATH = "/sys/class/dmi/id/bios_version"
TMC_VERSION_MAJOR = "/sys/devices/pci0000:00/0000:00:1c.0/0000:0f:00.0/major"
TMC_VERSION_MINOR = "/sys/devices/pci0000:00/0000:00:1c.0/0000:0f:00.0/minor"

COMPONENT_LIST= [
   ("BIOS", "Basic Input/Output System"),
   ("TMC", "TOR Master Control fpga")

]

class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index=0):
        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.get_name()        

    def __get_bios_version(self):
        # Retrieves the BIOS firmware version
        try:
            with open(BIOS_VERSION_PATH, 'r') as fd:
                bios_version = fd.read()
                return bios_version.strip()
        except Exception as e:
            return None
    
    def __get_tmc_version(self):
        # Retrieves the TMC fpga firmware version
        try:
            with open(TMC_VERSION_MAJOR, 'r') as fd1:
                tmc_major_version = fd1.read()
            with open(TMC_VERSION_MINOR, 'r') as fd2:
                tmc_minor_version = fd2.read()
            return (tmc_major_version+tmc_minor_version)
        except Exception as e:
            return None

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
        fw_version = None

        if self.name == "BIOS":
            fw_version = self.__get_bios_version()
        elif self.name == "TMC":
            fw_version = self.__get_tmc_version()
        else:
            fw_version = "N/A"

        return fw_version

    def install_firmware(self, image_path):
        """
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """
        raise NotImplementedError
