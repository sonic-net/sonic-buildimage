try:
    import re
    import subprocess
    from .helper import APIHelper
    from sonic_platform_base.component_base import ComponentBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

BMC_EXIST =APIHelper().is_bmc_present()
FPGA_VERSION_PATH = "/sys/bus/platform/devices/fpga_sysfs/version"
Bios_Version_Cmd = "dmidecode -t bios | grep Version"

BIOS_VERSION_CMD = "dmidecode -s bios-version"
ONIE_VERSION_CMD = "cat /host/machine.conf"
SWCPLD1_VERSION_CMD = "i2cget -y -f 102 0x30 0x0 | tr a-z A-Z | cut -d 'X' -f 2"
SWCPLD2_VERSION_CMD = "i2cget -y -f 102 0x31 0x0 | tr a-z A-Z | cut -d 'X' -f 2"
BASECPLD_VERSION_CMD = "cat /sys/devices/platform/baseboard/version | tr a-z A-Z | cut -d 'X' -f 2"
COME_CPLD_VERSION_CMD = "cat /sys/devices/platform/baseboard/come_cpld_version | tr a-z A-Z | cut -d 'X' -f 2"
SSD_VERSION_CMD = "smartctl -i /dev/sda"
ASIC_PCIE_VERSION_CMD = "(bcmcmd 'echo' >/dev/null 2>&1) && bcmcmd 'pciephy fw version' | grep 'PCIe FW version' | cut -d ' ' -f 4"


COMPONENT_NAME_LIST = ["BIOS", "ONIE", "BMC", "FPGA", "CPLD COMe", "CPLD BASE",
                       "CPLD SW1", "CPLD SW2", "ASIC PCIe", "SSD"]
COMPONENT_DES_LIST = ["Basic input/output System", 
                      "Open Network Install Environment",
                      "Baseboard Management Controller",
                      "FPGA for transceiver EEPROM access and other component I2C access",
                      "COMe board CPLD",
                      "CPLD for board functions, fan control and watchdog",
                      "CPLD for port control QSFP(1-16)",
                      "CPLD for port control QSFP(17-32), SFP(33)",
                      "ASIC PCIe Firmware",
                      "Solid State Drive - {}"]

UNKNOWN_VER = "Unknown"

class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index, pddf_data=None, pddf_plugin_data=None):
        ComponentBase.__init__(self)
        self.index = component_index
        self.helper = APIHelper()
        self.name = self.get_name()

    def get_firmware_version(self):
        """
        Retrieves the available firmware version of the component

        Note: the firmware version will be read from image

        Args:
          image_path: A string, path to firmware image

        Returns:
          A string containing the available firmware version of the component
        """
        fw_version_info = {
           "ONIE": self.__get_onie_ver(),
           "SSD": self.__get_ssd_ver(),
           "BIOS": self.__get_bios_ver(),
           "FPGA": self.__get_fpga_version(),
           "ASIC PCIe": self.__get_asic_pcie_ver(),
        }
        fw_version_info.update(self.__get_cpld_ver())
        if self.__get_bmc_presence():
            fw_version_info.update(self.__get_bmc_ver())
        return fw_version_info.get(self.name, UNKNOWN_VER)

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
        #if self.component_data == None:
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
        # For SSD get the model name from device
        if self.get_name() == "SSD":
          return "Solid State Drive - {}".format(self.__get_ssd_model())

        return COMPONENT_DES_LIST[self.index]

    def __get_cpld_ver(self):
        cpld_version_dict = dict()
        cpld_ver_info = {
            'CPLD BASE': self.__get_basecpld_ver(),
            'CPLD SW1': self.__get_swcpld1_ver(),
            'CPLD SW2': self.__get_swcpld2_ver(),
            'CPLD COMe': self.__get_comecpld_ver(),
        }

        for cpld_name, cpld_ver in cpld_ver_info.items():
            ver1 = int(cpld_ver.strip()) / 10
            ver2 = int(cpld_ver.strip()) % 10
            version = "%d.%d" % (ver1,ver2)
            cpld_version_dict[cpld_name] = version
        return cpld_version_dict

    def __get_asic_pcie_ver(self):
        status, raw_ver=self.helper.run_command(ASIC_PCIE_VERSION_CMD)
        if status:
           return raw_ver
        else:
           return UNKNOWN_VER

    def __get_bios_ver(self):
        status, raw_ver=self.helper.run_command(BIOS_VERSION_CMD)
        if status:
           return raw_ver
        else:
           return UNKNOWN_VER

    def __get_basecpld_ver(self):
        status, raw_ver=self.helper.run_command(BASECPLD_VERSION_CMD)
        if status:
           return raw_ver
        else:
           return UNKNOWN_VER

    def __get_comecpld_ver(self):
        status, raw_ver=self.helper.run_command(COME_CPLD_VERSION_CMD)
        if status:
           return raw_ver
        else:
           return UNKNOWN_VER
    
    def __get_swcpld1_ver(self):
        status, raw_ver=self.helper.run_command(SWCPLD1_VERSION_CMD)
        if status:
           return raw_ver
        else:
           return UNKNOWN_VER

    def __get_swcpld2_ver(self):
        status, raw_ver=self.helper.run_command(SWCPLD2_VERSION_CMD)
        if status:
            return raw_ver
        else:
           return UNKNOWN_VER

    def __get_bmc_presence(self):
        if BMC_EXIST:
           return True
        else:
           return False

    def __get_bmc_ver(self):
        cmd="ipmitool mc info | grep 'Firmware Revision'"
        status, raw_ver=self.helper.run_command(cmd)
        if status:
           bmc_ver=raw_ver.split(':')[-1].strip()
           return {"BMC":bmc_ver}
        else:
           return {"BMC":"N/A"}

    def __get_fpga_version(self):
        status, fpga_version = self.helper.run_command("cat %s" % FPGA_VERSION_PATH)
        if not status:
            return UNKNOWN_VER
        return fpga_version.replace("0x", "")

    def __get_onie_ver(self):
        onie_ver = "N/A"
        status, raw_onie_data = self.helper.run_command(ONIE_VERSION_CMD)
        if status:
           ret = re.search(r"(?<=onie_version=).+[^\n]", raw_onie_data)
           if ret != None:
              onie_ver = ret.group(0)
        return onie_ver

    def __get_ssd_ver(self):
        ssd_ver = "N/A"
        status, raw_ssd_data = self.helper.run_command(SSD_VERSION_CMD)
        if status:
           ret = re.search(r"Firmware Version: +(.*)[^\\]", raw_ssd_data)
           if ret != None:
              ssd_ver = ret.group(1)
        return ssd_ver

    def __get_ssd_model(self):
        model = "N/A"

        status, raw_ssd_data = self.helper.run_command(SSD_VERSION_CMD)
        if status:
            ret = re.search(r"Device Model: +(.*)[^\\]", raw_ssd_data)
            if ret != None:
                try:
                    model = ret.group(1)
                except (IndexError):
                    pass
        return model
