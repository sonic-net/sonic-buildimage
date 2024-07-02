#!/usr/bin/env python
# @Company ：Celestica
# @Time   : 2023/3/3 17:09
# @Mail   : yajiang@celestica.com
# @Author : jiang tao
try:
    from sonic_platform_base.component_base import ComponentBase
    from . import helper
    import re
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

bcm_exist = helper.APIHelper().get_bmc_status() 
FPGA_VERSION_PATH = "/sys/bus/platform/devices/fpga_sysfs/version"
Bios_Version_Cmd = ['dmidecode', '-t', 'bios']
if bcm_exist:
    Check_Bios_Boot = ["ipmitool", "raw", "0x3a", "0x64", "0x00", "0x01", "0x70"]
    Fan_CPLD_Cmd = ["ipmitool", "raw", "0x3a", "0x64", "0x02", "0x01", "0x00"]
    COME_CPLD_Cmd = ["ipmitool", "raw", "0x3a", "0x3e", "0x01", "0x1a", "0x01", "0xe0"]
    Sys_Cpld_Cmd = ["ipmitool", "raw", "0x3a", "0x64", "0x00", "0x01", "0x00"]
    Main_BMC_Cmd = ["0x32", "0x8f", "0x08", "0x01"]
    Backup_BMC_Cmd = ["0x32", "0x8f", "0x08", "0x01"]
    COMPONENT_NAME_LIST = ["FPGA", "COME_CPLD", "FANCPLD", "SYSCPLD",
                           "Main_BMC", "Backup_BMC", "Main_BIOS", "Backup_BIOS"]
    COMPONENT_DES_LIST = ["Used for managering the CPU and expanding I2C channels",
                               "Used for managing the CPU",
                               "Used for managing fans",
                               "Used for managing control the system power & reset",
                               "Main Baseboard Management Controller",
                               "Backup Baseboard Management Controller",
                               "Main basic Input/Output System",
                                "Backup basic Input/Output System"]
else:
    Check_Bios_Boot = ["i2cget", "-y", "-f", "6", "0x0d", "0x70"]
    Fan_CPLD_Cmd = ["i2cget", "-y", "-f", "11", "0x0d", "0x00"]
    COME_CPLD_Cmd = ["i2cget", "-y", "-f", "4", "0x0d", "0xe0"]
    Sys_Cpld_Cmd = ["i2cget", "-y", "-f", "6", "0x0d", "0x00"]
    COMPONENT_NAME_LIST = ["FPGA", "COME_CPLD", "FANCPLD", "SYSCPLD",
                          "Main_BIOS", "Backup_BIOS"]
    COMPONENT_DES_LIST = ["Used for managering the CPU and expanding I2C channels",
                               "Used for managing the CPU",
                               "Used for managing fans",
                               "Used for managing control the system power & reset",
                               "Main basic Input/Output System",
                               "Backup basic Input/Output System"]

class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
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
        if "01" in result.strip() and self.name == "Main_BIOS":
            return bios_version.strip()

        elif "03" in result.strip() and self.name == "Backup_BIOS":
            return bios_version.strip()
        else:
            return "N/A"

    def __get_cpld_version(self):
        """
        Get Come cpld/Fan cpld/Sys cpld version
        """
        version = "N/A"
        cpld_version_dict = {
            "COME_CPLD": COME_CPLD_Cmd,
            "FANCPLD": Fan_CPLD_Cmd,
            "SYSCPLD": Sys_Cpld_Cmd
        }
        if self.name in cpld_version_dict.keys():
            version_cmd = cpld_version_dict[self.name]
            status, output = self.helper.run_command(version_cmd)
            if not status:
                print("Fail! Can't get %s version by command:%s" % (self.name, version_cmd))
                return version
            if output.startswith('0x') or output.startswith('0X'):
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
        status, fpga_version = self.helper.run_command(["cat", FPGA_VERSION_PATH])
        if not status:
            return "N/A"
        major_ver = (int(fpga_version, 16) & 0xFFFF0000) >> 16
        minor_ver = int(fpga_version, 16) & 0x0000FFFF
        version = "{}.{}".format(major_ver, minor_ver)
        return version

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
        elif "CPLD" in self.name:
            fw_version = self.__get_cpld_version()
        elif self.name == "FPGA":
            fw_version = self.__get_fpga_version()
        elif "BMC" in self.name:
            fw_version = self.__get_bmc_version()
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
