#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################


try:
    import os.path
    import shutil
    import subprocess
    from sonic_platform_base.component_base import ComponentBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MMC_CPLD_ADDR = '0x100'
BIOS_VERSION_PATH = "/sys/class/dmi/id/bios_version"
CONFIG_DB_PATH = "/etc/sonic/config_db.json"
SMC_CPLD_PATH = "/sys/devices/platform/e1031.smc/version"
GETREG_PATH = "/sys/devices/platform/e1031.smc/getreg"
COMPONENT_NAME_LIST = ["SMC_CPLD", "MMC_CPLD", "BIOS"]
COMPONENT_DES_LIST = ["System Management Controller",
                      "Module Management CPLD", "Basic Input/Output System"]


class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.get_name()

    def __run_command(self, command):
        # Run bash command and print output to stdout
        try:
            process = subprocess.Popen(command, universal_newlines=True, stdout=subprocess.PIPE)
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

    def get_register_value(self, register):
        # Retrieves the cpld register value
        with open(GETREG_PATH, 'w') as file:
            file.write(register + '\n')
        with open(GETREG_PATH, 'r') as file:
            raw_data = file.readline()
        return raw_data.strip()

    def __get_cpld_version(self):
        # Retrieves the CPLD firmware version
        cpld_version = dict()
        with open(SMC_CPLD_PATH, 'r') as fd:
            smc_cpld_version = fd.read()
        smc_cpld_version = 'None' if smc_cpld_version == 'None' else "{}.{}".format(
            int(smc_cpld_version[2], 16), int(smc_cpld_version[3], 16))

        mmc_cpld_version = self.get_register_value(MMC_CPLD_ADDR)
        mmc_cpld_version = 'None' if mmc_cpld_version == 'None' else "{}.{}".format(
            int(mmc_cpld_version[2], 16), int(mmc_cpld_version[3], 16))

        cpld_version["SMC_CPLD"] = smc_cpld_version
        cpld_version["MMC_CPLD"] = mmc_cpld_version
        return cpld_version

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

        if self.name == "BIOS":
            fw_version = self.__get_bios_version()
        elif "CPLD" in self.name:
            cpld_version = self.__get_cpld_version()
            fw_version = cpld_version.get(self.name)

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
        if not os.path.isfile(image_path):
            return False

        if "CPLD" in self.name:
            img_name = os.path.basename(image_path)
            root, ext = os.path.splitext(img_name)
            ext = ".vme" if ext == "" else ext
            new_image_path = os.path.join("/tmp", (root.lower() + ext))
            shutil.copy(image_path, new_image_path)
            install_command = ["ispvm", str(new_image_path)]
        # elif self.name == "BIOS":
        #     install_command = "afulnx_64 %s /p /b /n /x /r" % image_path
        return self.__run_command(install_command)

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
