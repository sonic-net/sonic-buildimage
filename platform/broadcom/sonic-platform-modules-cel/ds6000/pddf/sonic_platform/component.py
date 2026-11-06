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

FPGA_VERSION_PATH = "/sys/devices/platform/pddf_custom_fpga_extend/version"
UNKNOWN_VER = "Unknown"

BMC_EXIST = APIHelper().get_bmc_status()

class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"
    COMPONENT_LIST = [
        ("BIOS",         "Basic Input/Output System"),
        ("ONIE",         "Open Network Install Environment"),
        ("FPGA",         "FPGA for transceiver EEPROM access and other component I2C access"),
        ("CPLDCOMe",    "COMe board CPLD"),
        ("CPLDBASE",    "CPLD for board functions and watchdog"),
        ("CPLDSW1",     "CPLD for port control OSFP(1-32)"),
        ("CPLDSW2",     "CPLD for port control OSFP(33-64)"),
        ("CPLDFIB",     "CPLD for fan control"),
        ("SSD",          "Solid State Drive firmware"),
    ]

    if BMC_EXIST:
        COMPONENT_LIST.insert(2,("BMC","Baseboard Management Controller"))

    def __init__(self, component_index):
        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.get_name()
        self._api_helper = APIHelper()

    def __get_switch_cpld1_ver(self):
        #cmd = "cat /sys/devices/platform/cls_sw_fpga/CPLD1/version"
        cmd = "i2cget -f -y 13 0x30 0x0"
        status, sw_cpld1_ver = self._api_helper.run_command(cmd)
        if status:
           return sw_cpld1_ver
        else:
           return UNKNOWN_VER

    def __get_switch_cpld2_ver(self):
        #cmd = "cat /sys/devices/platform/cls_sw_fpga/CPLD2/version"
        cmd = "i2cget -f -y 13 0x31 0x0"
        status, sw_cpld2_ver = self._api_helper.run_command(cmd)
        if status:
           return sw_cpld2_ver
        else:
           return UNKNOWN_VER

    def __get_base_cpld_ver(self):
        status, ver = self._api_helper.cpld_lpc_read(0x0A00)
        if status:
           return ver
        else:
           return UNKNOWN_VER

    def __get_come_cpld_ver(self):
        status, ver = self._api_helper.comecpld_lpc_read(0x0AE0)
        if status:
           return ver
        else:
           return UNKNOWN_VER
    
    def __get_fan_cpld_ver(self):
        if BMC_EXIST:
            cmd = "ipmitool raw 0x3a 0x3e 63 0x0d 0x1 0x00"
            status, ver = self._api_helper.run_command(cmd)
            if status:
                return "0x" + ver
            else:
                return UNKNOWN_VER
        else:
            cmd = "i2cget -f -y 138 0x0d 0x0"
            status, ver = self._api_helper.run_command(cmd)
            if status:
                return ver
            else:
                return UNKNOWN_VER


    def __get_bmc_ver(self):
        ver = "Unknown"
        cmd = "ipmitool mc info | grep 'Firmware Revision'"
        status, result = self._api_helper.run_command(cmd)
        if status:
            ver_data = result.split(":")
            ver = ver_data[-1].strip() if len(ver_data) > 1 else ver
            return ver 
        else:
           return "N/A"

    def __get_ssd_ver(self):
        ver = "Unknown"
        cmd = "ssdutil -v | grep 'Firmware'"
        status, result = self._api_helper.run_command(cmd)
        if status:
            ver_data = result.split(":")
            ver = ver_data[-1].strip() if len(ver_data) > 1 else ver
        return ver

    def __get_onie_ver(self):
        ver = "Unknown"
        cmd = "cat /host/machine.conf | grep 'onie_version'"
        status, result = self._api_helper.run_command(cmd)
        if status:
            ver_data = result.split("=")
            ver = ver_data[-1].strip() if len(ver_data) > 1 else ver
        return ver

    def __get_fpga_ver(self):
        status, fpga_version = self._api_helper.run_command("cat %s" % FPGA_VERSION_PATH)
        if not status:
            return UNKNOWN_VER
        return fpga_version.replace("0x", "")

    def __get_bios_ver(self):
        cmd = "dmidecode -s bios-version"
        status, result = self._api_helper.run_command(cmd)
        return result

    def get_name(self):
        """
        Retrieves the name of the component
         Returns:
            A string containing the name of the component
        """
        return self.COMPONENT_LIST[self.index][0]

    def get_description(self):
        """
        Retrieves the description of the component
            Returns:
            A string containing the description of the component
        """
        return self.COMPONENT_LIST[self.index][1]

    def get_firmware_version(self):
        """
        Retrieves the firmware version of module
        Returns:
            string: The firmware versions of the module
        """
        fw_version = None

        if "BIOS" in self.name:
            fw_version = self.__get_bios_ver()
        elif "ONIE" in self.name:
            fw_version = self.__get_onie_ver()
        elif "CPLD" in self.name:
            fw_version = self.__get_cpld_ver()
        elif self.name == "FPGA":
            fw_version = self.__get_fpga_ver()
        elif "BMC" in self.name:
            fw_version = self.__get_bmc_ver()
        elif "SSD" in self.name:
            fw_version = self.__get_ssd_ver()
        return fw_version

    def __get_cpld_ver(self):
        version = "N/A"
        cpld_version_dict = dict()
        cpld_ver_info = {
            'CPLDBASE': self.__get_base_cpld_ver(),
            'CPLDSW1': self.__get_switch_cpld1_ver(),
            'CPLDSW2': self.__get_switch_cpld2_ver(),
            'CPLDCOMe': self.__get_come_cpld_ver(),
            'CPLDFIB': self.__get_fan_cpld_ver(),
        }

        if self.name in cpld_ver_info.keys():
            version = cpld_ver_info[self.name]
        if "0x" in version:
            ver = version.lstrip("0x")
            version1 = int(ver.strip(), 16) / 16
            version2 = int(ver.strip(), 16) % 16
            version = "%d.%d" % (version1, version2)
        return version

    @staticmethod
    def upgrade_firmware(image_path):
        """
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """
        return False

    def switch_cpld_jtag(self, component_name):
        import time
        jtag_dic = {
            'CPLDBASE': "0xfd",
            'CPLDSW1':  "0xdf",
            'CPLDSW2':  "0xef",
            'CPLDCOMe': "0xfd",
            'CPLDFIB':  "0xfb",
        }
        self._api_helper.cpld_lpc_write(0xa35, 0xe5)
        self._api_helper.run_command("i2cset -y -f 1 0x27 0x03 0")
        time.sleep(1)
        self._api_helper.run_command("i2cset -f -y 1 0x27 0x01 {}".format( jtag_dic[component_name]))
        time.sleep(1)

    def install_firmware(self, image_path):
        if self.name == 'FPGA':
            targetpath = "/sys/devices/pci0000:00/0000:00:14.0/0000:04:00.0/resource0"
            if not os.path.isfile(targetpath):
                targetpath = "/sys/devices/pci0000:00/0000:00:14.0/0000:05:00.0/resource0"
            FW_UPGRADE_CMD = "fpga_prog %s %s" % (targetpath, image_path)
        elif self.name == 'BIOS':
            status,value = self._api_helper.cpld_lpc_read(0xa70)
            print("Upgrade BIOS%s!!!"%((int(value, 16) >> 1) & 1))
            FW_UPGRADE_CMD = "afulnx_64 %s /p /b /n /x /k /me" % image_path
        elif "CPLD" in self.name:
            self.switch_cpld_jtag(self.name)
            FW_UPGRADE_CMD = "ispvm-icelake %s" % image_path
        else:
            print("Don't support install firmware for component: %s" % self.name)
            return False
        if not os.path.isfile(image_path):
            return False
        print('Starting...')
        status = Component.run_command_with_progress(FW_UPGRADE_CMD)
        print('/nFinished!')
        if status:
            print('Please power cycle the chassis before new fw taking effect!')
        return status

    @staticmethod
    def run_command_with_progress(command):
        import subprocess
        import sys
        process = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            shell=True,
            text=True,
            bufsize=0
        )
        while True:
            char = process.stdout.read(1)
            if not char and process.poll() is not None:
                break

            if char == '\n':   #keep only one line for the log
                sys.stdout.write('\r')
            else:
                sys.stdout.write(char)
            sys.stdout.flush()
        return_code = process.wait()
        if return_code == 0:
            return True
        return False