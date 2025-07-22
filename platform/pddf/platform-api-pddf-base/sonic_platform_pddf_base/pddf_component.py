#############################################################################
# PDDF
# Module contains an implementation of SONiC PDDF Component Base API and
# provides the component firmware information and update support
#
#############################################################################

try:
    import subprocess
    import os
    from sonic_platform_base.component_base import (ComponentBase, FW_AUTO_ERR_BOOT_TYPE, FW_AUTO_ERR_IMAGE,
                                                    FW_AUTO_UPDATED, FW_AUTO_SCHEDULED, FW_AUTO_ERR_UNKNOWN)
    from sonic_platform_base.device_base import DeviceBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class PddfComponent(ComponentBase, DeviceBase):
    """
    PDDF generic Component class
    """

    pddf_obj = {}
    plugin_data = {}
    pddf_conf = {}

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        if not pddf_data or not pddf_plugin_data:
            raise ValueError('PDDF JSON data error')

        self.index = index
        self.pddf_obj = pddf_data
        self.plugin_data = pddf_plugin_data

        key = f"COMPONENT{index+1}"
        if key in pddf_data.data.keys():
            self.pddf_conf = pddf_data.data[key]

        self.name = self.get_name()

    def _get_attr(self, attr_name):
        if "comp_attr" in self.pddf_conf and attr_name in self.pddf_conf["comp_attr"]:
            return self.pddf_conf["comp_attr"][attr_name]
        else:
            return None

    def _get_cmd(self, cmd_name):
        if "attr_list" in self.pddf_conf:
            for attr in self.pddf_conf["attr_list"]:
                if attr["attr_name"] != cmd_name:
                    continue
                if "cmd" in attr:
                    return attr["cmd"]
                else:
                    return attr["get_cmd"]

        return None

    def get_name(self):
        """
        Retrieves the name of the component

        Returns:
            A string containing the name of the component
        """
        return self._get_attr("name")

    def get_description(self):
        """
        Retrieves the description of the component

        Returns:
            A string containing the description of the component
        """
        return self._get_attr("description")

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
        type = self._get_attr("type").upper()

        post_flash_reboot = self._get_attr("post_flash_reboot") # Options are args to auto_update_firmware()
        if post_flash_reboot is None or post_flash_reboot == "none":
            action_msg = ""
        elif post_flash_reboot == "cold":
            action_msg = " and power reboot device to take effect"
        elif post_flash_reboot == "warm":
            action_msg = " and reboot device to take effect"
        elif post_flash_reboot == "fast":
            action_msg = " and fast reboot device to take effect"
        else:
            raise RuntimeError("Invalid post_flash_reboot value")

        msg = f"{type} will be updated, please wait for completion{action_msg}!"

    def get_firmware_version(self):
        """
        Retrieves the firmware version of the component

        Returns:
            A string containing the firmware version of the component
        """
        version_cmd = self._get_cmd("version")
        if version_cmd is None:
            return "Unknown"

        result = self.pddf_obj.get_cmd_output(version_cmd)
        if result.returncode != 0:
            return "Unknown"

        return result.stdout.decode("utf-8").strip()

    def get_available_firmware_version(self, image_path):
        """
        Retrieves the available firmware version of the component

        Note: the firmware version will be read from image

        Args:
            image_path: A string, path to firmware image

        Returns:
            A string containing the available firmware version of the component
        """
        # Used only if "version" is not in platform_components.json
        return "Unknown"

    def _install_firmware(self, image_path):
        update_cmd = self._get_cmd("update")
        if update_cmd is None:
            raise RuntimeError("Component missing update command")

        if not os.path.exists(image_path):
            return False

        result = self.pddf_obj.runcmd(update_cmd.format(image_path))
        if result != 0:
            raise RuntimeError(f"Component firmware update failed")

        return True

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
        try:
            return self._install_firmware(image_path)
        except RuntimeError as e:
            return False

    def update_firmware(self, image_path):
        """
        Updates firmware of the component

        This API performs firmware update: it assumes firmware installation and loading in a single call.
        In case platform component requires some extra steps (apart from calling Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.) - this will be done automatically by API

        Args:
            image_path: A string, path to firmware image

        Returns:
            Boolean False if image_path doesn't exist instead of throwing an exception error
            Boolean True when the update is successful

        Raises:
            RuntimeError: update failed
        """
        if not self._install_firmware(image_path):
            return False # firmware image file not found

        boot_type = self._get_attr("boot_type") # Options are args to auto_update_firmware()
        if boot_type is None or boot_type == "none":
            pass
        elif boot_type == "cold":
            subprocess.run("reboot")
        elif boot_type == "warm":
            subprocess.run("warm-reboot")
        elif boot_type == "fast":
            subprocess.run("/usr/bin/fast-reboot")
        else:
            raise RuntimeError("Invalid boot_type value")
        return True

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
                      FW_AUTO_INSTALLED = 1
                      FW_AUTO_UPDATED = 2
                      FW_AUTO_SCHEDULED = 3
                  - return_code of a negative number indicates failed auto-update
                      FW_AUTO_ERR_BOOT_TYPE = -1
                      FW_AUTO_ERR_IMAGE = -2
                      FW_AUTO_ERR_UNKNOWN = -3

        Raises:
          RuntimeError: auto-update failure cause
        """
        reboot_power_order = ["none", "fast", "warm", "cold"]
        update_boot_type = self._get_attr("boot_type") # Options are args to auto_update_firmware()
        try:
            idx = reboot_power_order.index(update_boot_type)
        except ValueError:
            return FW_AUTO_ERR_BOOT_TYPE
        if boot_type not in reboot_power_order[idx:]:
            return FW_AUTO_ERR_BOOT_TYPE

        # Let RuntimeError bubble up
        success = self._install_firmware(image_path)
        if not success:
            return FW_AUTO_ERR_IMAGE

        if update_boot_type == "none":
            return FW_AUTO_UPDATED
        else:
            return FW_AUTO_SCHEDULED

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        return True # Default to unremovable components

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        model_cmd = self._get_cmd("model")
        if model_cmd is None:
            return "Unknown"

        result = self.pddf_obj.get_cmd_output(model_cmd)
        if result.returncode != 0:
            return "Unknown"

        return result.stdout.decode("utf-8").strip()

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        serial_cmd = self._get_cmd("serial")
        if serial_cmd is None:
            return "Unknown"

        result = self.pddf_obj.get_cmd_output(serial_cmd)
        if result.returncode != 0:
            return "Unknown"

        return result.stdout.decode("utf-8").strip()

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        revision_cmd = self._get_cmd("revision")
        if revision_cmd is None:
            return "Unknown"

        result = self.pddf_obj.get_cmd_output(revision_cmd)
        if result.returncode != 0:
            return "Unknown"

        return result.stdout.decode("utf-8").strip()

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False # Default to unremovable components
