#############################################################################
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

import shlex
import subprocess
import sys
import os

try:
    from sonic_py_common import logger
    from sonic_platform_base.component_base import ComponentBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

CPLD_ADDR_MAPPING = {
    "CPLD1": "1-0068",
    "CPLD2": "13-0061",
    "CPLD3": "10-0066",
}
SYSFS_PATH = "/sys/bus/i2c/devices/"
BIOS_VERSION_PATH = "/sys/class/dmi/id/bios_version"
COMPONENT_LIST= [
   ("BIOS", "Basic Input/Output System"),
   ("ONIE", "Open Network Install Environment"),
   ("CPLD1", "CPLD 1"),
   ("CPLD2", "CPLD 2"),
   ("CPLD3", "CPLD 3")
]

SYSLOG_IDENTIFIER = "component"
logger = logger.Logger(SYSLOG_IDENTIFIER)
logger.set_min_log_priority_debug()
logger.log_debug("Load {} module".format(__name__))

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
                bios_version = fd.read()
                return bios_version.strip()
        except Exception as e:
            return None

    def __get_onie_version(self):
        onie_mnt = "/tmp/onie"
        user = subprocess.check_output("whoami", shell=True).decode(sys.stdout.encoding).strip()
        sudo = "" if user == "root" else "sudo "
        if not os.path.isdir(onie_mnt):
            subprocess.call(sudo + "mkdir " + onie_mnt, shell=True)
        if not os.path.ismount(onie_mnt):
            subprocess.call(sudo + "mount LABEL=ONIE-BOOT " + onie_mnt, shell=True)
        onie_version = subprocess.check_output("grep onie_version= /tmp/onie/grub/grub.cfg", shell=True)
        onie_version = onie_version.decode(sys.stdout.encoding).strip().split("onie_version=", 1)[-1]
        subprocess.call(sudo + "umount " + onie_mnt + " && " + sudo + "rm -rf " + onie_mnt, shell=True)
        return onie_version

    def __get_cpld_version(self):
        # Retrieves the CPLD firmware version
        cpld_name = COMPONENT_LIST[self.index][0]
        try:
            cpld_path = "{}{}{}".format(SYSFS_PATH, CPLD_ADDR_MAPPING[cpld_name], '/version')
            cpld_version_raw= self._api_helper.read_txt_file(cpld_path)
        except Exception as e:
            print('Get exception when read cpld (%s)', cpld_path)

        return cpld_version_raw

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
        elif "ONIE" in self.name:
            fw_version = self.__get_onie_version()
        elif "CPLD" in self.name:
            fw_version = self.__get_cpld_version()

        return fw_version

    def install_firmware(self, image_path):
        """
        TODO: Need to implement
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """
        raise NotImplementedError

    def update_firmware(self, image_path):
        """
        TODO: Need to implement
        Updates firmware of the component
        This API performs firmware update: it assumes firmware installation and loading in a single call.
        In case platform component requires some extra steps (apart from calling Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.) - this will be done automatically by API
        Args:
            image_path: A string, path to firmware image
        Raises:
            RuntimeError: update failed
        """
        raise NotImplementedError

    def get_presence(self):
        """
        Retrieves the presence of the component
        Returns:
            bool: True if component is present, False if not
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
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return 1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_available_firmware_version(self, image_path):
        """
        TODO: Need to implement
        Retrieves the available firmware version of the component
        Note: the firmware version will be read from image
        Args:
            image_path: A string, path to firmware image
        Returns:
            A string containing the available firmware version of the component
        """
        raise NotImplementedError

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

