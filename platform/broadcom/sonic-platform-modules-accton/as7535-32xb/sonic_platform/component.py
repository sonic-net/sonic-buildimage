#############################################################################
# Edgecore
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

import shlex
import subprocess

try:
    from sonic_platform_base.component_base import ComponentBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

CPLD_PATH_MAPPING = {
    "MAIN_CPLD": "/sys/bus/i2c/devices/12-0061/version",
    "FAN_CPLD": "/sys/devices/platform/as7535_32xb_fan/version",
}

FPGA_PATH_MAPPING = {
    "FPGA_MAJOR": "/sys/devices/platform/as7535_32xb_sys/fpga_version"
}

BIOS_VERSION_PATH = "/sys/class/dmi/id/bios_version"
COMPONENT_LIST= [
   ("MAIN_FPGA", "Main board FPGA"),
   ("MAIN_CPLD", "Main board CPLD"),
   ("FAN_CPLD", "Fan board CPLD"),
   ("BIOS", "Basic Input/Output System")
]

class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index=0):
        self._api_helper=APIHelper()
        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.get_name()

    def __run_command(self, command):
        # Run bash command and print output to stdout
        try:
            process = subprocess.Popen(
                shlex.split(command), stdout=subprocess.PIPE)
            while True:
                output = process.stdout.readline()
                if output == '' and process.poll() is not None:
                    break
            rc = process.poll()
            if rc != 0:
                return False
        except Exception:
            return False
        return True

    def __get_bios_version(self):
        # Retrieves the BIOS firmware version
        try:
            with open(BIOS_VERSION_PATH, 'r') as fd:
                return fd.read().strip()
        except Exception as e:
            print('Get exception when read bios')
            return None

    def __get_cpld_version(self):
        # Retrieves the CPLD firmware version
        cpld_version = dict()
        for cpld_name in CPLD_PATH_MAPPING:
            try:
                cpld_path = "{}".format(CPLD_PATH_MAPPING[cpld_name])
                cpld_version_raw= self._api_helper.read_txt_file(cpld_path)
                cpld_version[cpld_name] = "{:x}".format(int(cpld_version_raw,10))
            except Exception as e:
                print('Get exception when read cpld (%s)', cpld_path)
                cpld_version[cpld_name] = 'None'

        return cpld_version

    def __get_fpga_version(self):
        # Retrieves the CPLD firmware version
        fpga_version = dict()
        for fpga_name in FPGA_PATH_MAPPING:
            try:
                fpga_path = "{}".format(FPGA_PATH_MAPPING[fpga_name])
                fpga_version_raw= self._api_helper.read_txt_file(fpga_path)
                fpga_version[fpga_name] = "{:x}".format(int(fpga_version_raw,10))
            except Exception as e:
                print('Get exception when read cpld (%s)', fpga_path)
                fpga_version[fpga_name] = 'None'

        return "{}".format(fpga_version["FPGA_MAJOR"])

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
        elif "CPLD" in self.name:
            cpld_version = self.__get_cpld_version()
            fw_version = cpld_version.get(self.name)
        elif "FPGA" in self.name:
            fw_version = self.__get_fpga_version()

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
