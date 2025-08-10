#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

import json
import os.path

try:
    from sonic_platform_base.component_base import ComponentBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

COMPONENT_LIST = [
    ("BIOS",        "Basic Input/Output System"),
    ("BMC",         "Baseboard Management Controller"),
    ("SWITCH_CPLD", "Switch board CPLD"),
    ("BASE_CPLD",   "Base board CPLD"),
    ("FPGA",        "Field-programmable gate array")
]
SW_CPLD_VER_PATH = "/sys/module/switch_cpld/version"
BASE_CPLD_VER_PATH = "/sys/module/baseboard_lpc/version"
CPLD_UPGRADE_OPT = 4
BIOS_VER_PATH = "/sys/class/dmi/id/bios_version"
BIOS__UPGRADE_OPT = 2
BMC_VER_CMD = "ipmitool mc info | grep 'Firmware Revision'"
BMC_UPGRADE_OPT = 1
CFUFLASH_FW_UPGRADE_CMD = "CFUFLASH -cd -d {} -mse 3 {}"
MEM_PCI_RESOURCE = "/sys/bus/pci/devices/0000:09:00.0/resource0"
FPGA_VER_MEM_OFFSET = 0


class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.get_name()
        self._api_helper = APIHelper()

    def __get_bmc_ver(self):
        bmc_ver = "Unknown"
        status, raw_bmc_data = self._api_helper.run_command(BMC_VER_CMD)
        if status:
            bmc_ver_data = raw_bmc_data.split(":")
            bmc_ver = bmc_ver_data[-1].strip() if len(
                bmc_ver_data) > 1 else bmc_ver
        return bmc_ver

    def __get_fpga_ver(self):
        fpga_ver = "Unknown"
        status, reg_val = self._api_helper.pci_get_value(
            MEM_PCI_RESOURCE, FPGA_VER_MEM_OFFSET)
        if status:
            major = reg_val[0] >> 16
            minor = int(bin(reg_val[0])[16:32], 2)
            fpga_ver = '{}.{}'.format(major, minor)
        return fpga_ver

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
        fw_version = {
            "BIOS": self._api_helper.read_txt_file(BIOS_VER_PATH),
            "BMC": self.__get_bmc_ver(),
            "FPGA": self.__get_fpga_ver(),
            "SWITCH_CPLD": self._api_helper.read_txt_file(SW_CPLD_VER_PATH),
            "BASE_CPLD": self._api_helper.read_txt_file(BASE_CPLD_VER_PATH),
        }.get(self.name, "Unknown")

        return fw_version

    def get_available_firmware_version(self, image_path):
        """
        Retrieves the available firmware version of the component
        Note: the firmware version will be read from image
        Args:
            image_path: A string, path to firmware image
        Returns:
            A string containing the available firmware version of the component
        """
        return "N/A"

    def get_firmware_update_notification(self, image_path):
        """
        Retrieves a notification on what should be done in order to complete
        the component firmware update
        Args:
            image_path: A string, path to firmware image
        Returns:
            A string containing the component firmware update notification if required.
            By default 'None' value will be used, which indicates that no actions are required
        """
        return "None"

    def install_firmware(self, image_path):
        """
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """
        install_command = {
            "BMC": CFUFLASH_FW_UPGRADE_CMD.format(BMC_UPGRADE_OPT, image_path),
            "BIOS": CFUFLASH_FW_UPGRADE_CMD.format(BIOS__UPGRADE_OPT, image_path),
            "SWITCH_CPLD": CFUFLASH_FW_UPGRADE_CMD.format(CPLD_UPGRADE_OPT, image_path),
            "BASE_CPLD": CFUFLASH_FW_UPGRADE_CMD.format(CPLD_UPGRADE_OPT, image_path)
        }.get(self.name, None)

        if not os.path.isfile(image_path) or install_command is None:
            return False

        # print(install_command)
        status = self._api_helper.run_interactive_command(install_command)
        return status

    def update_firmware(self, image_path):
        """
        Updates firmware of the component
        This API performs firmware update: it assumes firmware installation and loading in a single call.
        In case platform component requires some extra steps (apart from calling Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.) - this will be done automatically by API
        Args:
            image_path: A string, path to firmware image
        Raises:
            RuntimeError: update failed
        """
        return False

    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
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
