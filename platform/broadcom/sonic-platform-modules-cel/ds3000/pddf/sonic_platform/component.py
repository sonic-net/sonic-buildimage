try:
    import subprocess
    from .helper import APIHelper
    from sonic_platform_pddf_base.pddf_component import PddfComponent
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Component(PddfComponent):

    def __init__(self, component_index, pddf_data=None, pddf_plugin_data=None):
        PddfComponent.__init__(self, component_index, pddf_data, pddf_plugin_data)
        self._api_helper = APIHelper()

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
        type = self.get_type()
        if type == 'bios':
            return "BIOS will be updated, please wait for completion and reboot the device to take effect!"
        elif type == 'cpld' or type == 'fpga':
            return "{} will be updated, please wait for completion and power reboot device to take effect!".format(type.upper())
        elif type == 'bmc':
            return "BMC image will be updated, please wait for completion!"
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
        if self.component_data == None:
            self.component_data = self._get_component_data()
        pre_cmd = self.component_data['pre-update']
        update_cmd = self.component_data['update']
        post_cmd = self.component_data['post-update']
        if pre_cmd != None:
            status, _ = self._api_helper.get_cmd_output(pre_cmd)
            if status != 0:
                return False
        if update_cmd != None:
            update_cmd = update_cmd.format(image_path)
            status, _ = self._api_helper.get_cmd_output(update_cmd)
            if status != 0:
                return False
        if post_cmd != None:
            status, _ = self._api_helper.get_cmd_output(post_cmd)
            if status != 0:
                return False

        return True

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
          Nothing when the update is successful

        Raises:
          RuntimeError: update failed
        """
        status = self.install_firmware(image_path)
        if not status:
            return status

        type = self.get_type()
        if type == 'fpga' or type == 'cpld':
            # TODO:: power cycle FPGA or CPLD
            pass

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
        raise NotImplementedError
