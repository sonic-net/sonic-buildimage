#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

import subprocess
import re

try:
    from sonic_platform_base.component_base import ComponentBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

COMPONENT_LIST = [
    ("BIOS",       "Basic input/output System"),
    ("ONIE",       "Open Network Install Environment"),
    ("BMC",        "Baseboard Management Controller"),
    ("FPGA",       "FPGA for transceiver EEPROM access and other component I2C access"),
    ("CPLD COMe",  "COMe board CPLD"),
    ("CPLD BASE",  "CPLD for board functions, fan control and watchdog"),
    ("CPLD SW1",   "CPLD for port control SFP(1-24)"),
    ("CPLD SW2",   "CPLD for port control SFP(25-48), QSFP(49-56)"),
    ("ASIC PCIe",  "ASIC PCIe Firmware"),
    ("SSD",        "Solid State Drive - {}")
]
NAME_INDEX = 0
DESCRIPTION_INDEX = 1

BIOS_VERSION_CMD = "dmidecode -s bios-version"
ONIE_VERSION_CMD = "cat /host/machine.conf"
FPGA_VERSION_PATH = "/sys/bus/platform/devices/fpga_sysfs/version"
COME_CPLD_VERSION_CMD = "cat /sys/devices/platform/sys_cpld/come_cpld_version"
SWCPLD1_VERSION_CMD = "i2cget -y -f 102 0x30 0x0"
SWCPLD2_VERSION_CMD = "i2cget -y -f 102 0x31 0x0"
GETREG_PATH="/sys/devices/platform/sys_cpld/getreg"
BASECPLD_VERSION_CMD="echo '0xA100' > {} && cat {}".format(GETREG_PATH, GETREG_PATH)
BMC_PRESENCE="echo '0xA108' > {} && cat {}".format(GETREG_PATH, GETREG_PATH)
SSD_VERSION_CMD = "smartctl -i /dev/sda"
ASIC_PCIE_VERSION_CMD = "(bcmcmd 'echo' >/dev/null 2>&1) && bcmcmd 'pciephy fw version' | grep 'PCIe FW version' | cut -d ' ' -f 4"

UNKNOWN_VER = "Unknown"

class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index, pddf_data=None, pddf_plugin_data=None):
        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.get_name()

    def __get_cpld_ver(self):
        cpld_version_dict = dict()
        cpld_ver_info = {
            'CPLD BASE': self.__get_basecpld_ver(),
            'CPLD SW1': self.__get_swcpld1_ver(),
            'CPLD SW2': self.__get_swcpld2_ver(),
            'CPLD COMe': self.__get_comecpld_ver()
        }
        for cpld_name, cpld_ver in cpld_ver_info.items():
            cpld_ver_str = "{}.{}".format(int(cpld_ver[2], 16), int(
                cpld_ver[3], 16)) if cpld_ver else UNKNOWN_VER
            cpld_version_dict[cpld_name] = cpld_ver_str

        return cpld_version_dict

    def __get_asic_pcie_ver(self):
        status, raw_ver=self.run_command(ASIC_PCIE_VERSION_CMD)
        if status:
            return raw_ver
        else:
            return UNKNOWN_VER

    def __get_bios_ver(self):
        status, raw_ver=self.run_command(BIOS_VERSION_CMD)
        if status:
            return raw_ver
        else:
            return UNKNOWN_VER

    def __get_comecpld_ver(self):
        status, raw_ver=self.run_command(COME_CPLD_VERSION_CMD)
        if status:
           return raw_ver
        else:
           return UNKNOWN_VER

    def __get_basecpld_ver(self):
        status, raw_ver=self.run_command(BASECPLD_VERSION_CMD)
        if status:
            return raw_ver
        else:
            return UNKNOWN_VER

    def __get_swcpld1_ver(self):
        status, raw_ver=self.run_command(SWCPLD1_VERSION_CMD)
        if status:
            return raw_ver
        else:
            return UNKNOWN_VER

    def __get_swcpld2_ver(self):
        status, raw_ver=self.run_command(SWCPLD2_VERSION_CMD)
        if status:
            return raw_ver
        else:
            return UNKNOWN_VER

    def __get_bmc_presence(self):
        status, raw_ver=self.run_command(BMC_PRESENCE)
        if status and raw_ver == "0x00":
            return True
        else:
            return False

    def __get_bmc_ver(self):
        cmd="ipmitool mc info | grep 'Firmware Revision'"
        status, raw_ver=self.run_command(cmd)
        if status:
            bmc_ver=raw_ver.split(':')[-1].strip()
            return {"BMC":bmc_ver}
        else:
            return {"BMC":"N/A"}

    def __get_fpga_version(self):
        status, fpga_version = self.run_command("cat %s" % FPGA_VERSION_PATH)
        if not status:
            return UNKNOWN_VER
        return fpga_version.replace("0x", "")

    def __get_onie_ver(self):
        onie_ver = "N/A"
        status, raw_onie_data = self.run_command(ONIE_VERSION_CMD)
        if status:
            ret = re.search(r"(?<=onie_version=).+[^\n]", raw_onie_data)
            if ret != None:
                onie_ver = ret.group(0)
        return onie_ver

    def __get_ssd_ver(self):
        ssd_ver = "N/A"
        status, raw_ssd_data = self.run_command(SSD_VERSION_CMD)
        if status:
            ret = re.search(r"Firmware Version: +(.*)[^\\]", raw_ssd_data)
            if ret != None:
                ssd_ver = ret.group(1)
        return ssd_ver

    def __get_ssd_desc(self, desc_format):
        description = "N/A"
        status, raw_ssd_data = self.run_command(SSD_VERSION_CMD)
        if status:
            ret = re.search(r"Device Model: +(.*)[^\\]", raw_ssd_data)
            if ret != None:
                try:
                    description = desc_format.format(ret.group(1))
                except (IndexError):
                    pass
        return description

    def get_name(self):
        """
        Retrieves the name of the component
         Returns:
            A string containing the name of the component
        """
        return COMPONENT_LIST[self.index][NAME_INDEX]

    def get_description(self):
        """
        Retrieves the description of the component
            Returns:
            A string containing the description of the component
        """
        # For SSD get the model name from device
        if self.get_name() == "SSD":
            return self.__get_ssd_desc(COMPONENT_LIST[self.index][1])

        return COMPONENT_LIST[self.index][DESCRIPTION_INDEX]

    def get_firmware_version(self):
        """
        Retrieves the firmware version of module
        Returns:
            string: The firmware versions of the module
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
   
    def run_command(self, cmd):
        status = True
        result = ""
        ret, data = subprocess.getstatusoutput(cmd)
        if ret != 0:
            status = False
        else:
            result = data

        return status, result
