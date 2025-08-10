#!/usr/bin/env python3

########################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
# the platform
#
########################################################################

try:
    import time
    import subprocess
    import os
    from sonic_platform_base.component_base import ComponentBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found") from e


FIRMWARE_UPDATE_DIR = "/tmp/.firmwareupdate/"

class Component(ComponentBase):
    """Platform-specific Component class"""

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

    def get_slot(self):
        self.cpld_dict_update()
        return self.cpld_dict["Slot"]

    def get_warm_upgrade_flag(self):
        self.cpld_dict_update()
        return self.cpld_dict["Warm"]

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

        Note: the firmware version will be read from HW

        Returns:
            A string containing the firmware version of the component
        """
        self.cpld_dict_update()
        return self.cpld_dict["Version"]

    def get_available_firmware_version(self, image_path):
        """
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
        return None

    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        This API performs firmware installation only: this may/may not be the same as firmware update.
        In case platform component requires some extra steps (apart from calling Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.) - this must be done manually by user

        Note: in case immediate actions are required to complete the component firmware update
        (e.g., reboot, power cycle, etc.) - will be done automatically by API and no return value provided

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        if not os.path.isfile(image_path):
            print("ERROR: %s not found" % image_path)
            return False
        cmdstr = "upgrade.py cold %s %d" % (image_path, self.get_slot())
        status, output = subprocess.getstatusoutput(cmdstr)
        if status == 0:
            print("INFO: %s firmware install succeeded" % self.get_name())
            return True
        print("%s install failed. status:%d, output:\n%s" % (self.get_name(), status, output))
        return False

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
        if not os.path.isfile(image_path):
            raise RuntimeError("ERROR: %s not found" % image_path)

        if self.get_warm_upgrade_flag() == 1:  # use warm upgrade
            cmdstr = "upgrade.py warm %s %d" % (image_path, self.get_slot())
        else:
            cmdstr = "upgrade.py cold %s %d" % (image_path, self.get_slot())
        status, output = subprocess.getstatusoutput(cmdstr)
        if status == 0:
            if self.get_warm_upgrade_flag() != 1:  # not support warm upgrade, need to cold reboot
                print("INFO: %s firmware install succeeded" % self.get_name())
                print("INFO: please cold reboot to make the %s firmware up-to-date" % self.get_name())
            else:
                print("INFO: %s firmware update succeeded" % self.get_name())
                print("INFO: %s firmware version up-to-date" % self.get_name())
            return None
        raise RuntimeError(output)

    def auto_update_firmware(self, image_path, boot_type):
        """
        Updates firmware of the component

        This API performs firmware update automatically based on boot_type: it assumes firmware installation
        and/or creating a loading task during the reboot, if needed, in a single call.
        In case platform component requires some extra steps (apart from calling Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.) - this will be done automatically during the reboot.
        The loading task will be created by API.

        Args:
            image_path: A string, path to firmware image
            boot_type: A string, reboot type following the upgrade
                         - none/fast/warm/cold

        Returns:
            Output: A return code
                return_code: An integer number, status of component firmware auto-update
                    - return code of a positive number indicates successful auto-update
                        - status_installed = 1
                        - status_updated = 2
                        - status_scheduled = 3
                    - return_code of a negative number indicates failed auto-update
                        - status_err_boot_type = -1
                        - status_err_image = -2
                        - status_err_unknown = -3

        Raises:
            RuntimeError: auto-update failure cause
        """
        if not os.path.isfile(image_path):
            print("ERROR: %s not found" % image_path)
            return -2

        if not os.path.isdir(FIRMWARE_UPDATE_DIR):
            os.mkdir(FIRMWARE_UPDATE_DIR)

        warm_upgrade_flag = self.get_warm_upgrade_flag()
        file_name = os.path.basename(image_path)
        file_path = os.path.join(FIRMWARE_UPDATE_DIR, file_name)
        if os.path.exists(file_path):   # firmware already update
            if warm_upgrade_flag == 1:
                print("INFO: %s firmware update succeeded, firmware version up-to-date" % self.get_name())
                return 2
            print("INFO: %s firmware install succeeded, please cold reboot to make it up-to-date" % self.get_name())
            return 1

        if warm_upgrade_flag == 1:  # use warm upgrade
            cmdstr = "upgrade.py warm %s %d" % (image_path, self.get_slot())
        else:
            cmdstr = "upgrade.py cold %s %d" % (image_path, self.get_slot())
        status, output = subprocess.getstatusoutput(cmdstr)
        if status == 0:
            os.mknod(file_path)
            if warm_upgrade_flag == 1:
                print("INFO: %s firmware update succeeded, firmware version up-to-date" % self.get_name())
                return 2
            print("INFO: %s firmware install succeeded, please cold reboot to make it up-to-date" % self.get_name())
            return 1
        print("%s update failed, status:%d, output:\n%s" % (self.get_name(), status, output))
        return -3
