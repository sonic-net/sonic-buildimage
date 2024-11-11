#!/usr/bin/python

import os
import re
import struct
import subprocess
import json

BMC_PRES_SYS_PATH = '/sys/devices/platform/sys_cpld/bmc_present_l'
policy_json = "/usr/share/sonic/platform/thermal_policy.json"

class APIHelper():

    def __init__(self):
        pass
        
    def run_command(self, cmd):
        try:
            data = subprocess.check_output(cmd, shell=True,
                    universal_newlines=True, stderr=subprocess.STDOUT).strip()
            status = True
        except subprocess.CalledProcessError as ex:
            data = ex.output
            status = False
        return status, data
        
    def get_register_value(self, getreg_path, register):
        cmd = "echo {1} > {0}; cat {0}".format(getreg_path, register)
        return self.run_command(cmd)
        
    def set_register_value(self, setreg_path, register, value):
        cmd = "echo {1} {2} > {0}".format(setreg_path, register, value)
        status, result = self.run_command(cmd)
        return status

    def cpld_lpc_read(self, reg):
        register = "0x{:X}".format(reg)
        return self.get_register_value("/sys/devices/platform/sys_cpld/getreg", register)

    def cpld_lpc_write(self, reg, val):
        register = "0x{:X}".format(reg)
        value = "0x{:X}".format(val)
        return self.set_register_value("/sys/devices/platform/sys_cpld/setreg", register, value)

    def grep(self, cmd, key):
        status, result = self.run_command("{0} | grep '{1}'".format(cmd, key))
        m = re.search(key, result)
        if status:
            status = True if m else False
        return status, result

    def read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return None
        
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
        return True if presence == "0" else False

    def fsc_enable(self, enable=True):
        if self.with_bmc():
            if enable:
                status, result = self.run_command('ipmitool raw 0x2e 0x04 0xcf 0xc2 0x00 1 0 0')
            else:
                status, result = self.run_command('ipmitool raw 0x2e 0x04 0xcf 0xc2 0x00 1 0 1')
            return status
        else:
            if os.path.isfile(policy_json):
                keyword = 'True' if enable else 'False'
                cmd = "grep 'run_at_boot_up' {0} | grep {1} > /dev/null 2>&1 ".format(policy_json, keyword)
                status, result = self.run_command(cmd)
                if status: 
                    return True
                else:
                    cmd = 'sed -i "4s/\(True\|False\)/{0}/g" {1}'.format(keyword, policy_json)
                    status, result = self.run_command(cmd)
                    if status:
                        status, result = self.run_command('docker exec -it pmon supervisorctl restart thermalctld')
                        return True if status else False
                    else:
                        return False
            else:
                return True if enable != True else False


    def fsc_enabled(self):
        if self.with_bmc():
            status, result = self.run_command('ipmitool raw 0x2e 0x04 0xcf 0xc2 0x00 0x00 0')
            if status == True:
                data_list = result.split()
                if len(data_list) == 4:
                    if int(data_list[3]) == 0:
                        return True
        else:
            if os.path.isfile(policy_json):
                cmd = "grep 'run_at_boot_up' {0} | grep 'True' > /dev/null 2>&1 ".format(policy_json)
                status, result = self.run_command(cmd)
                if status:
                    return True
        return False

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