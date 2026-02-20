#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

import os.path
import re

try:
    from sonic_platform_base.component_base import ComponentBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

COMPONENT_LIST = [
    ("BIOS",         "Basic Input/Output System"),
    ("ONIE",         "Open Network Install Environment"),
    ("BMC",          "Baseboard Management Controller"),
    ("FPGA",         "FPGA for transceiver EEPROM access, as MUX for I2C access and port control SFP(65-66)"),
    ("CPLD COMe",    "COMe board CPLD"),
    ("CPLD BASE",    "CPLD for board functions LED, fan and watchdog control"),
    ("CPLD SW1",     "CPLD for port control OSFP(1-32)"),
    ("CPLD SW2",     "CPLD for port control OSFP(33-64)"),
    ("SSD1",         "Primary Solid State Drive"),
    ("SSD2",         "Secondary Solid State Drive"),
]

class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
        ComponentBase.__init__(self)
        self.index = component_index
        self._api_helper = APIHelper()
        self._name = None
        self._desc = None

        if not self._api_helper.with_bmc() and component_index >= 2:
            # skip the BMC from the list
            self._component_info = COMPONENT_LIST[component_index + 1]
        else:
            self._component_info = COMPONENT_LIST[component_index]

    def __get_cpld_ver(self, bus, addr):
        ver = "N/A"
        cmd = "/usr/sbin/i2cget -y -f {} {} 0".format(bus, addr)
        status, output = self._api_helper.run_command(cmd)
        if status:
            ver = "{}.{}".format(int(output[2],16), int(output[3],16))
        
        return ver

    def __get_switch_cpld1_ver(self):
        return self.__get_cpld_ver(2, "0x30")

    def __get_switch_cpld2_ver(self):
        return self.__get_cpld_ver(2, "0x31")

    def __get_base_cpld_ver(self):
        ver = "N/A"
        status, output = self._api_helper.cpld_lpc_read(0xA100)
        if status:
            ver = "{}.{}".format(int(output[2],16), int(output[3],16))

        return ver

    def __get_come_cpld_ver(self):
        ver = "N/A"
        status, output = self._api_helper.cpld_lpc_read(0xA1E0)
        if status:
            ver = "{}.{}".format(int(output[2],16), int(output[3],16))

        return ver

    def __get_bmc_ver(self):
        ver = "N/A"
        cmd = "/usr/bin/ipmitool mc info"
        status, output = self._api_helper.run_command(cmd)
        if status:
            for line in output.split('\n'):
                if line.startswith('Firmware Revision'):
                    ver = line.split(':')[1].strip()
                    break

        return ver

    def __get_ssd_ver(self, path):
        ver = "N/A"
        cmd = "/usr/sbin/smartctl -i {}".format(path)
        status, output = self._api_helper.run_command(cmd)
        if status:
            for line in output.split('\n'):
                if line.startswith('Firmware Version'):
                    ver = line.split(':')[1].strip()
                    break

        return ver

    def __get_ssd1_ver(self):
        return self.__get_ssd_ver('/dev/sda')

    def __get_ssd2_ver(self):
        return self.__get_ssd_ver('/dev/sdb')

    def __get_onie_ver(self):
        ver = "N/A"
        try:
            with open("/host/machine.conf") as fd:
                output = fd.read()
            for line in output.split('\n'):
                if line.startswith('onie_version'):
                    ver = line.split('=')[1].strip()
        except (FileNotFoundError, IOError):
            pass

        return ver

    def __get_fpga_ver(self):
        ver = "N/A"
        try:
            with open("/sys/devices/platform/cls_sw_fpga/FPGA/version") as fd:
                output = fd.read().strip()
            output = output[2:]
            ver = output.upper()
        except (FileNotFoundError, IOError):
            pass

        return ver

    def __get_bios_ver(self):
        ver = "N/A"
        cmd = "/usr/sbin/dmidecode -s bios-version"
        status, output = self._api_helper.run_command(cmd)
        if status:
            ver = output

        return ver

    def get_name(self):
        """
        Retrieves the name of the component
         Returns:
            A string containing the name of the component
        """
        if self._name == None:
            self._name = self._component_info[0]

        return self._name

    def get_description(self):
        """
        Retrieves the description of the component
            Returns:
            A string containing the description of the component
        """
        if self._desc == None:
            self._desc = self._component_info[1]

        return self._desc

    def get_firmware_version(self):
        """
        Retrieves the firmware version of module
        Returns:
            string: The firmware versions of the module
        """

        get_fw_version = {
            "BIOS": self.__get_bios_ver,
            "ONIE": self.__get_onie_ver,
            "BMC": self.__get_bmc_ver,
            "FPGA": self.__get_fpga_ver,
            "CPLD COMe": self.__get_come_cpld_ver,
            "CPLD BASE": self.__get_base_cpld_ver,
            "CPLD SW1": self.__get_switch_cpld1_ver,
            "CPLD SW2": self.__get_switch_cpld2_ver,
            "SSD1": self.__get_ssd1_ver,
            "SSD2": self.__get_ssd2_ver,
        }

        fw_version = get_fw_version[self.get_name()]()

        return fw_version
