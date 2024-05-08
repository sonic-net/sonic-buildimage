#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

import os.path
import subprocess
import re

try:
    from sonic_platform_base.component_base import ComponentBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NAME_IDX = 0
DESC_IDX = 1
VER_API_IDX = 2

BASE_CPLD_VER_CMD = "0x03 0x00 0x01 0x00"
FAN_CPLD_VER_CMD = "0x03 0x01 0x01 0x00"
BIOS_VER_PATH = "/sys/class/dmi/id/bios_version"
BMC_VER_CMD1 = ["ipmitool", "mc", "info"]
BMC_VER_CMD2 = ["grep", "Firmware Revision"]
ONIE_VER_CMD = ["cat", "/host/machine.conf"]
SSD_VER_CMD = ["smartctl", " -i", "/dev/sda"]
BASE_GETREG_PATH = "/sys/devices/platform/baseboard-lpc/getreg"
MEM_PCI_RESOURCE = "/sys/bus/pci/devices/0000:09:00.0/resource0"
FPGA_VER_MEM_OFFSET = 0
NETFN = "0x3A"
COME_CPLD_VER_REG = "0xA1E0"

class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
        COMPONENT_LIST = [
            ("BIOS",        "Basic Input/Output System", self.__get_bios_ver),
            ("ONIE",        "Open Network Install Environment", self.__get_onie_ver),
            ("BMC",         "Baseboard Management Controller", self.__get_bmc_ver),
            ("FPGA",        "FPGA for transceiver EEPROM access and other component I2C access", self.__get_fpga_ver),
            ("CPLD COMe",   "COMe board CPLD", self.__get_come_cpld_ver),
            ("CPLD BASE",   "CPLD for board functions and watchdog", self.__get_cpld_ver_from_bmc),
            ("CPLD SW1",    "CPLD for port control QSFP(1-16)", self.__get_sw_cpld1_ver),
            ("CPLD SW2",    "CPLD for port control QSFP(17-32) SFP(33-34)", self.__get_sw_cpld2_ver),
            ("CPLD FAN",    "CPLD for fan control and status", self.__get_cpld_ver_from_bmc),
            ("SSD",         "Solid State Drive - {}", self.__get_ssd_ver)
        ]
        self.index = component_index
        self.name = COMPONENT_LIST[self.index][NAME_IDX]
        self.description = COMPONENT_LIST[self.index][DESC_IDX]
        self.__get_version = COMPONENT_LIST[self.index][VER_API_IDX]
        self._api_helper = APIHelper()

    def __i2c_get(self, bus, i2caddr, offset):
        try:
            return int(subprocess.check_output(['/usr/sbin/i2cget', '-y', '-f', str(bus), str(i2caddr), str(offset)]), 16)
        except (FileNotFoundError, subprocess.CalledProcessError):
            return -1

    def __get_bios_ver(self):
        return self._api_helper.read_txt_file(BIOS_VER_PATH)

    def __get_onie_ver(self):
        onie_ver = "N/A"
        status, raw_onie_data = self._api_helper.run_command(ONIE_VER_CMD)
        if status:
            ret = re.search(r"(?<=onie_version=).+[^\n]", raw_onie_data)
            if ret != None:
                onie_ver = ret.group(0)
        return onie_ver

    def __get_bmc_ver(self):
        bmc_ver = "Unknown"
        status, raw_bmc_data = self._api_helper.run_command(BMC_VER_CMD1, BMC_VER_CMD2)
        if status:
            bmc_ver_data = raw_bmc_data.split(":")
            bmc_ver = bmc_ver_data[-1].strip() if len(
                bmc_ver_data) > 1 else bmc_ver
        return bmc_ver

    def __get_fpga_ver(self):
        fpga_ver = "Unknown"
        status, reg_val = self._api_helper.pci_get_value(
            MEM_PCI_RESOURCE, FPGA_VER_MEM_OFFSET)
        if status:
            major = reg_val[0] >> 16
            minor = int(bin(reg_val[0])[16:32], 2)
            fpga_ver = '{}.{}'.format(major, minor)
        return fpga_ver

    def __get_come_cpld_ver(self):
        cpld_ver_str = self._api_helper.lpc_getreg(BASE_GETREG_PATH, COME_CPLD_VER_REG)
        if not cpld_ver_str:
            return "N/A"

        cpld_ver = int(cpld_ver_str, 16)
        return "{:x}.{:x}".format((cpld_ver >> 4) & 0xF, cpld_ver & 0xF)
 
    def __get_cpld_ver_from_bmc(self):
        cpld_ver = "N/A"
        cmd = BASE_CPLD_VER_CMD

        if self.name == "CPLD FAN":
            cmd = FAN_CPLD_VER_CMD

        (rc, op) = self._api_helper.ipmi_raw(NETFN, cmd)
        if rc and len(op) == 2:
            cpld_ver = op[0] + '.' + op[1]

        return cpld_ver
       
    def __get_sw_cpld1_ver(self):
        val = self.__i2c_get(4, 0x30, 0)
        if val != -1:
            return '{:x}.{:x}'.format((val >> 4) & 0xf, val & 0xf)
        else:
            return 'N/A'

    def __get_sw_cpld2_ver(self):
        val = self.__i2c_get(4, 0x31, 0)
        if val != -1:
            return '{:x}.{:x}'.format((val >> 4) & 0xf, val & 0xf)
        else:
            return 'N/A'

    def __get_ssd_ver(self):
        ssd_ver = "N/A"
        status, raw_ssd_data = self._api_helper.run_command(SSD_VER_CMD)
        if status:
            ret = re.search(r"Firmware Version: +(.*)[^\\]", raw_ssd_data)
            if ret != None:
                ssd_ver = ret.group(1)
        return ssd_ver

    def __get_ssd_model(self):
        model = "N/A"

        status, raw_ssd_data = self._api_helper.run_command(SSD_VER_CMD)
        if status:
            ret = re.search(r"Device Model: +(.*)[^\\]", raw_ssd_data)
            if ret != None:
                try:
                    model = ret.group(1)
                except (IndexError):
                    pass

        return model

    def get_name(self):
        """
        Retrieves the name of the component
         Returns:
            A string containing the name of the component
        """
        return self.name

    def get_description(self):
        """
        Retrieves the description of the component
            Returns:
            A string containing the description of the component
        """
        # For SSD get the model name from device
        if self.get_name() == "SSD":
            return self.description.format(self.__get_ssd_model())

        return self.description

    def get_firmware_version(self):
        """
        Retrieves the firmware version of module
        Returns:
            string: The firmware versions of the module
        """
        return self.__get_version()
