#!/usr/bin/python

import os
import re
import struct
import subprocess
import json

BMC_PRES_SYS_PATH = '/sys/devices/platform/sys_cpld1/bmc_present'
policy_json = "/usr/share/sonic/platform/thermal_policy.json"

class APIHelper():

    def __init__(self):
        pass

    @staticmethod
    def run_command(cmd):
        status = True
        result = ""
        try:
            p = subprocess.Popen(cmd, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            raw_data, err = p.communicate()
            if err.decode("utf-8") == "":
                result = raw_data.decode("utf-8").strip()
        except Exception:
            status = False
        return status, result

    def get_register_value(self, getreg_path, register):
        try:
            with open(getreg_path, 'w') as f:
                f.write(register)
            with open(getreg_path, 'r') as f:
                result = f.read().strip()
            return True, result
        except (IOError, ValueError) as e:
            return False, None

    def set_register_value(self, setreg_path, register, value):
        try:
            with open(setreg_path, 'w') as f:
                f.write(f"{register} {value}")
            return True
        except (IOError, ValueError) as e:
            return False

    def cpld_lpc_read(self, reg, cpld_id=None):
        register = "0x{:X}".format(reg)
        if cpld_id:
            path = "/sys/devices/platform/sys_cpld{}/getreg".format(cpld_id)
        else:
            path = "/sys/devices/platform/sys_cpld/getreg"
        return self.get_register_value(path, register)

    def cpld_lpc_write(self, reg, val, cpld_id=None):
        register = "0x{:X}".format(reg)
        value = "0x{:X}".format(val)
        if cpld_id:
            path = "/sys/devices/platform/sys_cpld{}/setreg".format(cpld_id)
        else:
            path = "/sys/devices/platform/sys_cpld/setreg"
        return self.set_register_value(path, register, value)

    def read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return None

    def read_one_line_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.readline()
                return data.strip()
        except IOError:
            pass
        return None

    def write_txt_file(self, file_path, value):
        try:
            with open(file_path, 'w') as fd:
                fd.write(str(value))
        except Exception as E:
            print(str(E))
            return False
        return True

    def ipmi_fru(self, id=0, key=None):
        status = True
        result = ""
        cmd = "ipmitool fru print {0}".format(id)
        if not key:
            try:
                status, result = self.run_command(cmd)
            except:
                status = False
        else:
            status, result = self.grep(cmd, str(key))
        return status, result

    def with_bmc(self):
        """
        Get the BMC card present status

        Returns:
            A boolean, True if present, False if absent
        """
        presence = self.read_txt_file(BMC_PRES_SYS_PATH)
        if presence == None:
            print("Failed to get BMC card presence status")
        return True if presence == "1" else False

    def i2c_read(self, bus, i2c_slave_addr, addr, num_bytes):
        if num_bytes == 0:
            return []

        data = ""
        for i in range(0, num_bytes):
            cmd = 'i2cget -f -y %d 0x%x 0x%x' % (bus, i2c_slave_addr, addr + i)
            status, output = self.run_command(cmd)
            if status == False:
                return []
            data += output
            if i < (num_bytes - 1):
                data += " "
        return data

    @staticmethod
    def get_bmc_status():
        """
        get bmc present by pddf-device.json
        return: True(present), False(absent)
        """
        pddf_device_path = '/usr/share/sonic/platform/pddf/pddf-device.json'
        with open(pddf_device_path) as f:
            json_data = json.load(f)
        bmc_present = json_data["PLATFORM"]["bmc_present"]
        return True if bmc_present == "True" else False
