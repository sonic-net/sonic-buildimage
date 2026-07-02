try:
    from . import helper
    from sonic_platform_base.component_base import ComponentBase
    import re
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

bcm_exist = helper.APIHelper().is_bmc_present()
GETREG_PATH = "/sys/devices/platform/sys_cpld/getreg"

if bcm_exist:
    FPGA_VERSION_PATH = ["cat", "/sys/bus/platform/devices/fpga_sysfs/version"]
    Bios_Version_Cmd = ['dmidecode', '-t', 'bios']
    ONIE_Version_Cmd = ["cat", "/host/machine.conf"]
    SSD_VERSION_CMD = ["smartctl", "-i", "/dev/sda"]
    Sys_Cpld_Cmd = ["cat", "/sys/devices/platform/sys_cpld/version"]
    Come_Cpld_Cmd = "echo '0xae0' > {} && cat {}".format(GETREG_PATH, GETREG_PATH)
    Switch_Cpld_Cmd = ["i2cget", "-y", "-f", "1", "0x30", "0x00"]

    BMC_Cmd = ["ipmitool","mc", "info"]
    COMPONENT_NAME_LIST = ["BIOS", "ONIE", "BMC", "FPGA", "CPLD COMe", "CPLD BASE", "CPLD SW", "SSD"]
    num_of_components = len(COMPONENT_NAME_LIST)

    COMPONENT_DES_LIST = ["Basic Input/Output System",
                            "Open Network Install Environment",
                            "Baseboard Management Controller",
                            "FPGA for transceiver EEPROM access and other component I2C access",
                            "COMe board CPLD",
                            "CPLD for base board",
                            "CPLD for switch board",
                            "Solid State Drive"
                          ]
else:
    FPGA_VERSION_PATH = "cat /sys/bus/platform/devices/fpga_sysfs/version"
    Bios_Version_Cmd = "dmidecode -t bios"
    ONIE_Version_Cmd = "cat /host/machine.conf"
    SSD_VERSION_CMD = "smartctl -i /dev/sda"
    Sys_Cpld_Cmd = "cat /sys/devices/platform/sys_cpld/version"
    Come_Cpld_Cmd = "echo '0xae0' > {} && cat {}".format(GETREG_PATH, GETREG_PATH)
    Switch_Cpld_Cmd = "i2cget -y -f 1 0x30 0x00"
    COMPONENT_NAME_LIST = ["BIOS", "ONIE", "FPGA", "CPLD COMe", "CPLD BASE", "CPLD SW", "SSD"]
    num_of_components = len(COMPONENT_NAME_LIST)

    COMPONENT_DES_LIST = ["Basic Input/Output System",
                            "Open Network Install Environment",
                            "FPGA for transceiver EEPROM access and other component I2C access",
                            "COMe board CPLD",
                            "CPLD for base board",
                            "CPLD for switch board",
                            "Solid State Drive"
                          ]


class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
        ComponentBase.__init__(self)
        self.index = component_index
        self.helper = helper.APIHelper()
        self.name = self.get_name()

    def _get_num_components(self):
        return self.num_of_components

    def __get_bios_version(self):
        """
        Get Bios version by command 'dmidecode -t bios | grep Version'
        return: Bios Version
        """
        status_ver, version_str = self.helper.run_command(Bios_Version_Cmd)
        bios_version = re.findall(r"Version:(.*)", version_str)[0]
        return bios_version.strip()

    def __get_cpld_version(self, cmd):
        if self.name == "CPLD COMe":
            status, output = self.helper.get_status_output(cmd)
        else:
            status, output = self.helper.run_command(cmd)
            if self.name == "CPLD BASE":
                return output

        output = output[2:]
        ver = int(output, 16)
        version1 = ver >> 4
        version2 = ver & 0b1111
        version = "%d.%d" % (version1, version2)
        return version

    def __get_fpga_version(self):
        """
        Get fpga version by fpga version bus path.
        """
        status, fpga_version = self.helper.run_command(FPGA_VERSION_PATH)
        if not status:
            return "N/A"
        # fpga_version is 0x00010002
        fpga_version = fpga_version[2:]
        major_ver = int(fpga_version[0:4],16)
        minor_ver = int(fpga_version[4:],16)
        version = "{}.{}".format(major_ver, minor_ver)
        return version

    def __get_bmc_version(self):
        ver = "Unknown"
        status, result = self.helper.run_command(BMC_Cmd)
        for line in result.splitlines():
            if "Firmware" in line:
                firmware_revision = line.split(":")[1].strip()
                break
        return firmware_revision


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

        if "BIOS" in self.name:
            fw_version = self.__get_bios_version()
        elif "CPLD BASE" in self.name:
            fw_version = self.__get_cpld_version(Sys_Cpld_Cmd)
        elif "CPLD COMe" in self.name:
            fw_version = self.__get_cpld_version(Come_Cpld_Cmd)
        elif "CPLD SW" in self.name:
            fw_version = self.__get_cpld_version(Switch_Cpld_Cmd)            
        elif self.name == "FPGA":
            fw_version = self.__get_fpga_version()
        elif "BMC" in self.name:
            fw_version = self.__get_bmc_version()
        elif "ONIE" in self.name:
            fw_version = self.__get_onie_ver()
        elif "SSD" == self.name:
            fw_version = self.__get_ssd_ver()
        return fw_version

    def install_firmware(self, image_path):
        """
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """
        return False

    def update_firmware(self, image_path):
        # Not support
        return False

    def __get_onie_ver(self):
        onie_ver = "N/A"
        status, raw_onie_data = self.helper.run_command(ONIE_Version_Cmd)
        if status:
            ret = re.search(r"(?<=onie_version=).+[^\n]", raw_onie_data)
            if ret != None:
                onie_ver = ret.group(0)
        return onie_ver

    def __get_ssd_ver(self):
        ssd_ver = "N/A"
        status, raw_ssd_data = self.helper.run_command(SSD_VERSION_CMD)
        if status:
            pattern = r"Firmware Version:\s+(\S+)"
            match = re.search(pattern, raw_ssd_data)
            firmware_version = match.group(1)
            if firmware_version != None:
                ssd_ver = firmware_version
        return ssd_ver

