#!/usr/bin/env python
# @Company ：Celestica
# @Time   : 2023/7/21 17:09
# @Mail   : yajiang@celestica.com
# @Author : jiang tao
try:
    from sonic_platform_base.component_base import ComponentBase
    from . import helper
    import re
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")
BMC_EXIST = helper.APIHelper().get_bmc_status()
FPGA_VERSION_PATH = "/sys/bus/platform/devices/fpga_sysfs/version"
Bios_Version_Cmd = "dmidecode -t bios | grep Version"
ONIE_Version_Cmd = "cat /host/machine.conf"
SSD_Version_Cmd = "smartctl -i /dev/sda"
ASIC_PCIE_VERSION_CMD = "bcmcmd 'echo' >/dev/null 2>&1 && bcmcmd 'pciephy fw version' | grep 'PCIe FW version' | cut -d ' ' -f 4"

if BMC_EXIST:
    Check_Bios_Boot = "ipmitool raw 0x3a 0x25 0x02"
    Fan_CPLD_Cmd = "ipmitool raw 0x3a 0x64 02 01 00"
    COME_CPLD_Cmd = "ipmitool raw 0x3a 0x3e 1 0x1a 1 0xe0"
    Sys_Cpld_Cmd = "ipmitool raw 0x3a 0x64 0x00 0x01 0x00"
    Sw_Cpld1_Cmd = "i2cget -y -f 108 0x30 0 | tr a-z A-Z | cut -d 'X' -f 2"
    Sw_Cpld2_Cmd = "i2cget -y -f 108 0x31 0 | tr a-z A-Z | cut -d 'X' -f 2"
    Main_BMC_Cmd = "0x32 0x8f 0x08 0x01"
    Backup_BMC_Cmd = "0x32 0x8f 0x08 0x01"

    COMPONENT_NAME_LIST = ["BIOS", "ONIE", "BMC", "FPGA", "CPLD COMe", "CPLD BASE",
                           "CPLD SW1", "CPLD SW2", "CPLD FAN", "ASIC PCIe", "SSD"]
    COMPONENT_DES_LIST = ["Basic Input/Output system",
                          "Open Network Install Environment",
                          "Baseboard Management Controller",
                          "FPGA for transceiver EEPROM access and other component I2C access",
                          "COMe board CPLD",
                          "CPLD for board functions and watchdog",
                          "CPLD for port control QSFP(1-16)",
                          "CPLD for port control QSFP(17-32) SFP(33-34)",
                          "CPLD for fan control and status",
                          "ASIC PCIe Firmware",
                          "Solid State Drive"]
else:
    Check_Bios_Boot = "i2cget -y -f 100 0x0d 0x70 | tr a-z A-Z | cut -d 'X' -f 2"
    Fan_CPLD_Cmd = "i2cget -y -f 107 0x0d 0x00 | tr a-z A-Z | cut -d 'X' -f 2"
    COME_CPLD_Cmd = "i2cget -y -f 104 0x0d 0xe0 | tr a-z A-Z | cut -d 'X' -f 2"
    Sys_Cpld_Cmd = "i2cget -y -f 100 0x0d 0x00 | tr a-z A-Z | cut -d 'X' -f 2"
    Sw_Cpld1_Cmd = "i2cget -y -f 108 0x30 0x00 | tr a-z A-Z | cut -d 'X' -f 2"
    Sw_Cpld2_Cmd = "i2cget -y -f 108 0x31 0x00 | tr a-z A-Z | cut -d 'X' -f 2"

    COMPONENT_NAME_LIST = ["BIOS", "ONIE", "BMC", "FPGA", "CPLD COMe", "CPLD BASE",
                           "CPLD SW1", "CPLD SW2", "CPLD FAN", "SSD"]

    COMPONENT_DES_LIST = ["Basic Input/Output system",
                          "Open Network Install Environment",
                          "Baseboard Management Controller",
                          "FPGA for transceiver EEPROM access and other component I2C access",
                          "COMe board CPLD",
                          "CPLD for board functions and watchdog",
                          "CPLD for port control QSFP(1-16)",
                          "CPLD for port control QSFP(17-32) SFP(33-34)",
                          "CPLD for fan control and status",
                          "Solid State Drive"]



class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index, pddf_data=None, pddf_plugin_data=None):
        ComponentBase.__init__(self)
        self.index = component_index
        self.helper = helper.APIHelper()
        self.name = self.get_name()

    def __get_bios_version(self):
        """
        Get Bios version by command 'dmidecode -t bios | grep Version'
        return: Bios Version
        """
        status, result = self.helper.run_command(Check_Bios_Boot)
        bios_version = "N/A"
        if not status:
            print("Fail! Unable to get the current Main bios or backup bios!")
            return bios_version
        status_ver, version_str = self.helper.run_command(Bios_Version_Cmd)
        if not status:
            print("Fail! Unable to get the bios version!")
            return bios_version

        bios_version = re.findall(r"Version:(.*)", version_str)[0]
        if result.strip() == "01" and self.name == "BIOS":
            return bios_version.strip()

        elif result.strip() == "03" and self.name == "BIOS":
            return bios_version.strip()
        else:
            return "N/A"

    def __get_onie_version(self):
        """
        Get ONIE Version"
        """
        onie_version = "N/A"
        status, raw_onie_data = self.helper.run_command(ONIE_Version_Cmd)
        if status:
            ret = re.search(r"(?<=onie_version=).+[^\n]", raw_onie_data)
            if ret != None:
                onie_version = ret.group(0)
        return onie_version

    def __get_cpld_version(self):
        """
        Get Come cpld/Fan cpld/Sys cpld/Switch 1 cpld/Switch 2 cpld version
        """
        version = "N/A"
        cpld_version_dict = {
            "CPLD COMe": COME_CPLD_Cmd,
            "CPLD FAN": Fan_CPLD_Cmd,
            "CPLD SW1": Sw_Cpld1_Cmd,
            "CPLD SW2": Sw_Cpld2_Cmd,
            "CPLD BASE": Sys_Cpld_Cmd,
        }
        if self.name in cpld_version_dict.keys():
            version_cmd = cpld_version_dict[self.name]
            status, ver = self.helper.run_command(version_cmd)
            if not status:
                print("Fail! Can't get %s version by command:%s" % (self.name, version_cmd))
                return version
            version1 = int(ver.strip()) / 10
            version2 = int(ver.strip()) % 10
            version = "%d.%d" % (version1, version2)
            return version

    def __get_fpga_version(self):
        """
        Get fpga version by fpga version bus path.
        """
        status, fpga_version = self.helper.run_command("cat %s" % FPGA_VERSION_PATH)
        if not status:
            return "N/A"
        return fpga_version.replace("0x", "")

    def __get_bmc_version(self):
        """
        Get main/backup bmc version
        """
        version = "N/A"
        cmd = Main_BMC_Cmd if self.name == "Main_BMC" else Backup_BMC_Cmd
        status, result = self.helper.ipmi_raw(cmd)
        if not status:
            print("Fail! Can't get the %s version by command:%s" % (self.name, cmd))
            return version
        str_1 = str(int(result.strip().split(" ")[0]))
        str_2 = str(int(result.strip().split(" ")[1], 16))
        version = "%s.%s" % (str_1, str_2)
        return version

    def __get_asic_pcie_ver(self):
        status, raw_ver=self.helper.run_command(ASIC_PCIE_VERSION_CMD)
        if status:
            return raw_ver
        else:
            return "N/A"

    def __get_ssd_version(self):
        ssd_version = "N/A"
        status, raw_ssd_data = self.helper.run_command(SSD_Version_Cmd)
        if status:
            ret = re.search(r"Firmware Version: +(.*)[^\\]", raw_ssd_data)
            if ret != None:
                ssd_version = ret.group(1)
        return ssd_version

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
        elif "ONIE" in self.name:
            fw_version = self.__get_onie_version()
        elif "CPLD" in self.name:
            fw_version = self.__get_cpld_version()
        elif self.name == "FPGA":
            fw_version = self.__get_fpga_version()
        elif "BMC" in self.name:
            fw_version = self.__get_bmc_version()
        elif self.name == "ASIC PCIe":
            fw_version = self.__get_asic_pcie_ver()
        elif "SSD" in self.name:
            fw_version = self.__get_ssd_version()
        return fw_version

    @staticmethod
    def install_firmware(image_path):
        """
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """
        return False

    @staticmethod
    def update_firmware(image_path):
        # Not support
        return False
