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
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

COMPONENT_LIST_BMC = [
    ("BIOS",                "Basic input/output System"),
    ("BMC",                 "Baseboard Management Controller"),
    ("BaseBoard_CPLD",      "Base board CPLD"),
    ("SwitchBoard_CPLD1",   "Switch board CPLD 1"),
    ("SwitchBoard_CPLD2",   "Switch board CPLD 2"),
    ("COMeBoard_CPLD",      "COMe board CPLD"),
    ("FPGA",                "Baseboard FPGA"),
    ("PCIe",                "ASIC PCIe Firmware"),
    ("SSD",                 "Solid State Drive firmware version")
]

COMPONENT_LIST_NONBMC = [
    ("BIOS",                "Basic input/output System"),
    ("BaseBoard_CPLD",      "Base board CPLD"),
    ("SwitchBoard_CPLD1",   "Switch board CPLD 1"),
    ("SwitchBoard_CPLD2",   "Switch board CPLD 2"),
    ("COMeBoard_CPLD",      "COMe board CPLD"),
    ("FPGA",                "Baseboard FPGA"),
    ("PCIe",                "ASIC PCIe Firmware"),
    ("SSD",                 "Solid State Drive firmware version")
]

NAME_INDEX = 0
DESCRIPTION_INDEX = 1

BIOS_VERSION_CMD = "dmidecode -s bios-version"
BASECPLD_VERSION_CMD="cat /sys/devices/platform/baseboard/version"
SWCPLD1_VERSION_CMD = "i2cget -y -f 102 0x30 0x0"
SWCPLD2_VERSION_CMD = "i2cget -y -f 102 0x31 0x0"
COMECPLD_VERSION_CMD="cat /sys/devices/platform/baseboard/come_cpld_version"
FPGA_VERSION_CMD="cat /sys/devices/platform/fpga_sysfs/version"
ASIC_PCIE_VERSION_CMD = "bcmcmd 'pciephy fw version' | grep 'PCIe FW version' | cut -d ' ' -f 4"
SSD_VERSION_CMD = "smartctl -i /dev/sda"

UNKNOWN_VER = "unknown"

class Component():
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
        ComponentBase.__init__(self)
        self._api_helper = APIHelper()
        self.index = component_index
        self.name = self.get_name()

    def _get_asic_pcie_ver(self):
        status, raw_ver=self.run_command(ASIC_PCIE_VERSION_CMD)
        if status:
            return raw_ver
        else:
            return UNKNOWN_VER

    def _get_bios_ver(self):
        status, raw_ver=self.run_command(BIOS_VERSION_CMD)
        if status:
            return raw_ver
        else:
            return UNKNOWN_VER

    def _get_basecpld_ver(self):
        status, raw_ver=self.run_command(BASECPLD_VERSION_CMD)
        if status:
            ver_str = "{}.{}".format(int(raw_ver[2], 16), int(raw_ver[3], 16))
            return ver_str
        else:
            return UNKNOWN_VER

    def _get_swcpld1_ver(self):
        status, raw_ver=self.run_command(SWCPLD1_VERSION_CMD)
        if status:
            ver_str = "{}.{}".format(int(raw_ver[2], 16), int(raw_ver[3], 16))
            return ver_str
        else:
            return UNKNOWN_VER

    def _get_swcpld2_ver(self):
        status, raw_ver=self.run_command(SWCPLD2_VERSION_CMD)
        if status:
            ver_str = "{}.{}".format(int(raw_ver[2], 16), int(raw_ver[3], 16))
            return ver_str
        else:
            return UNKNOWN_VER

    def _get_comecpld_ver(self):
        status, raw_ver=self.run_command(COMECPLD_VERSION_CMD)
        if status:
            ver_str = "{}.{}".format(int(raw_ver[2], 16), int(raw_ver[3], 16))
            return ver_str
        else:
            return UNKNOWN_VER

    def _get_fpga_ver(self):
        status, raw_ver=self.run_command(FPGA_VERSION_CMD)
        if status:
            iver = int(raw_ver, 16)
            ver = "{}.{}".format(iver >> 16, iver & 0xffff)
            return ver
        else:
            return UNKNOWN_VER

    def _get_bmc_ver(self):
        cmd="ipmitool mc info | grep 'Firmware Revision'"
        status, raw_ver=self.run_command(cmd)
        if status:
            bmc_ver=raw_ver.split(':')[-1].strip()
            return bmc_ver
        else:
            return UNKNOWN_VER

    def _get_ssd_ver(self):
        ssd_ver = "N/A"
        status, raw_ssd_data = self.run_command(SSD_VERSION_CMD)
        if status:
            ret = re.search(r"Firmware Version: +(.*)[^\\]", raw_ssd_data)
            if ret != None:
                ssd_ver = ret.group(1)
        return ssd_ver

    def get_name(self):
        """
        Retrieves the name of the component
         Returns:
            A string containing the name of the component
        """
        if self._api_helper.is_bmc_present():
            return COMPONENT_LIST_BMC[self.index][NAME_INDEX]
        else:
            return COMPONENT_LIST_NONBMC[self.index][NAME_INDEX]

    def get_description(self):
        """
        Retrieves the description of the component
            Returns:
            A string containing the description of the component
        """
        if self._api_helper.is_bmc_present():
            return COMPONENT_LIST_BMC[self.index][DESCRIPTION_INDEX]
        else:
            return COMPONENT_LIST_NONBMC[self.index][DESCRIPTION_INDEX]

    def get_firmware_version(self):
        """
        Retrieves the firmware version of module
        Returns:
            string: The firmware versions of the module
        """
        fw_func_map = {
            "BIOS":              "_get_bios_ver",
            "BMC":               "_get_bmc_ver",
            "BaseBoard_CPLD":    "_get_basecpld_ver",
            "SwitchBoard_CPLD1": "_get_swcpld1_ver",
            "SwitchBoard_CPLD2": "_get_swcpld2_ver",
            "COMeBoard_CPLD":    "_get_comecpld_ver",
            "FPGA":              "_get_fpga_ver",
            "PCIe":              "_get_asic_pcie_ver",
            "SSD":               "_get_ssd_ver"
        }
        fw_method = getattr(self, fw_func_map[self.name], None)
        if fw_method:
            fw_ver = fw_method()
            return fw_ver
        else:
            return UNKNOWN_VER

    def run_command(self, cmd):
        status = True
        result = ""
        try:
            p = subprocess.Popen( # nosemgrep
                cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE) # nosemgrep
            raw_data, err = p.communicate()
            if err.decode('UTF-8') == '':
                result = raw_data.strip().decode('UTF-8')
        except Exception:
            status = False
        return status, result
