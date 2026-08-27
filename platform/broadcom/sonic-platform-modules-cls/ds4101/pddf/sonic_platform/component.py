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
ONIE_Version_Cmd = ['cat', '/host/machine.conf']
SSD_VERSION_CMD = ['smartctl', '-i', '/dev/sda']

if bcm_exist:
    Check_Bios_Boot = ["ipmitool", "raw", "0x3a", "0x64", "0x00", "0x01", "0x70"]
    Fan_CPLD_Cmd = ["ipmitool", "raw", "0x3a", "0x64", "0x02", "0x01", "0x00"]
    COME_CPLD_Cmd = ["ipmitool", "raw", "0x3a", "0x3e", "0x01", "0x1a", "0x01", "0xe0"]
    Sys_Cpld_Cmd = ["ipmitool", "raw", "0x3a", "0x64", "0x00", "0x01", "0x00"]
    BMC_Cmd = ["0x32", "0x8f", "0x08", "0x01"]
    COMPONENT_NAME_LIST = ["BIOS", "ONIE", "BMC", "FPGA","CPLD COMe", "CPLD FAN", "CPLD SYS", "SSD"]
                           
    COMPONENT_DES_LIST = ["Basic Input/Output System",
                            "Open Network Install Environment",
                            "Baseboard Management Controller",
                            "FPGA for transceiver EEPROM access and other component I2C access",
                            "COMe board CPLD",
                            "Used for managing fans",
                            "Used for managing the CPU",
                            "Solid State Drive",
                    ]
else:
    FPGA_VERSION_PATH = "/sys/bus/platform/devices/fpga_sysfs/version"
    Check_Bios_Boot = ["i2cget", "-y", "-f", "6", "0x0d", "0x70"]
    Fan_CPLD_Cmd = ["i2cget", "-y", "-f", "11", "0x0d", "0x00"]
    COME_CPLD_Cmd = ["i2cget", "-y", "-f", "4", "0x0d", "0xe0"]
    Sys_Cpld_Cmd = ["i2cget", "-y", "-f", "6", "0x0d", "0x00"]
    COMPONENT_NAME_LIST = ["BIOS", "ONIE", "FPGA", "CPLD COMe",
                          "CPLD FAN", "CPLD SYS", "SSD"]
    COMPONENT_DES_LIST = ["Basic Input/Output System",
                               "Open Network Install Environment",
                               "FPGA for transceiver EEPROM access and other component I2C access",
                               "COMe board CPLD",
                               "Used for managing fans",
                               "Used for managing the CPU",
                               "Solid State Drive"]

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
        if "01" in result.strip() and self.name == "BIOS":
            return bios_version.strip()
        else:
            return "N/A"

    def __get_cpld_version(self):
        """
        Get Come cpld/Fan cpld/Sys cpld version
        """
        version = "N/A"
        cpld_version_dict = {
            "CPLD COMe": COME_CPLD_Cmd,
            "CPLD FAN": Fan_CPLD_Cmd,
            "CPLD SYS": Sys_Cpld_Cmd
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
        Get bmc version
        """
        version = "N/A"
        cmd = BMC_Cmd
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
            ret = re.search(r"Firmware Version: +(.*)[^\\]", raw_ssd_data)
            if ret != None:
                ssd_ver = ret.group(1)
        return ssd_ver
