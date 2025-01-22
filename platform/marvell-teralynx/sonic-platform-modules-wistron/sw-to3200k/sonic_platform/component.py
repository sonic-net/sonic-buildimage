#!/usr/bin/env python

import subprocess
from shlex import split
from collections import namedtuple
from functools import reduce


try:
    from sonic_platform_base.component_base import ComponentBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

CPLD_ADDR_MAPPING = {
    "CPLD1": "0-0061",
    "CPLD2": "0-0062",
    "MB_FPGA": "0-0060",
    "FAN_CPLD" : "0-0066"
}

proc_output = namedtuple('proc_output', 'stdout stderr')
SYSFS_PATH = "/sys/bus/i2c/devices/"
#GET_BMC_VER_CMD= "ipmitool mc info | grep 'Firmware Revision' | awk '{printf $4}'"
BIOS_VERSION_PATH = "/sys/class/dmi/id/bios_version"
COMPONENT_LIST= [
   ("BIOS", "Basic Input/Output System"),
   ("CPLD1", "CPLD 1"),
   ("CPLD2", "CPLD 2"),
   ("MB_FPGA", "MB FPGA"),
   ("FAN_CPLD", "FAN CPLD"),
   ("BMC", "baseboard management controller")
]

class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index=0):
        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.get_name()

    def pipeline(self, starter_command, *commands):
        if not commands:
            try:
                starter_command, *commands = starter_command.split('|')
            except AttributeError:
                pass
        starter_command = self._parse(starter_command)
        starter = subprocess.Popen(starter_command, stdout=subprocess.PIPE)
        last_proc = reduce(self._create_pipe, map(self._parse, commands), starter)
        return proc_output(*last_proc.communicate())

    def _create_pipe(self, previous, command):
        proc = subprocess.Popen(command, stdin=previous.stdout, stdout=subprocess.PIPE)
        previous.stdout.close()
        return proc

    def _parse(self, cmd):
        try:
            return split(cmd)
        except Exception:
            return cmd

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return ""

    def __get_bios_version(self):
        # Retrieves the BIOS firmware version
        try:
            with open(BIOS_VERSION_PATH, 'r') as fd:
                bios_version = fd.read()
                return bios_version.strip()
        except Exception as e:
            return None

    def __get_bmc_version(self):
        try:
            #GET_BMC_VER_CMD
            out, err = self.pipeline("ipmitool mc info", "grep 'Firmware Revision'", "awk '{printf $4}'")
            return out.decode().rstrip('\n')
        except Exception as e:
            print('Get exception when read bmc')
            return 'None'

    def __get_cpld_version(self):
        # Retrieves the CPLD firmware version
        cpld_version = dict()
        for cpld_name in CPLD_ADDR_MAPPING:
            try:
                cpld_path = "{}{}{}".format(SYSFS_PATH, CPLD_ADDR_MAPPING[cpld_name], '/version')
                cpld_version_raw= int(self.__read_txt_file(cpld_path), 10)
                cpld_version[cpld_name] = "{}".format(hex(cpld_version_raw))
            except Exception as e:
                print('Get exception when read cpld')
                cpld_version[cpld_name] = 'None'

        return cpld_version

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
        elif "BMC" in self.name:
            fw_version = self.__get_bmc_version()
        elif "CPLD" in self.name:
            cpld_version = self.__get_cpld_version()
            fw_version = cpld_version.get(self.name)
        elif "FPGA" in self.name:
            fpga_version = self.__get_cpld_version()
            fw_version = fpga_version.get(self.name)

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
