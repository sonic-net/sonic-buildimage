#!/usr/bin/python

import os
import re
import subprocess
import json

BMC_PRESENCE_SYSFS_PATH = '/sys/devices/platform/sys_cpld/bmc_presence'
policy_json = "/usr/share/sonic/platform/thermal_policy.json"

class APIHelper():

    def __init__(self):
        self._bmc_presence = None

    def run_command(self, command):
        try:
            output = subprocess.check_output(command.split(' '), universal_newlines=True)
            return (True, output.strip())
        except subprocess.CalledProcessError:
            return (False, None)
        
    def get_register_value(self, getreg_path, register):
        try:
            with open(getreg_path, "w+") as fd:
                fd.write(register)
                fd.flush()
                fd.seek(0)
                return (True, fd.read().strip())
        except (FileNotFoundError, IOError):
            pass

        return (False, None)
        
    def set_register_value(self, setreg_path, register, value):
        try:
            with open(setreg_path, "w") as fd:
                set_str = register + " " + value
                if fd.write() == len(set_str):
                    fd.flush()
                    return True
        except (FileNotFoundError, IOError):
            pass

        return False

    def cpld_lpc_read(self, reg):
        register = "0x{:X}".format(reg)
        return self.get_register_value("/sys/devices/platform/sys_cpld/getreg", register)

    def cpld_lpc_write(self, reg, val):
        register = "0x{:X}".format(reg)
        value = "0x{:X}".format(val)
        return self.set_register_value("/sys/devices/platform/sys_cpld/setreg", register, value)

    def read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return None

    def with_bmc(self):
        if self._bmc_presence == None:
            try:
               with open(BMC_PRESENCE_SYSFS_PATH) as fd:
                   data = fd.read().strip()
               if data == "1":
                   self._bmc_presence = True
               else:
                   self._bmc_presence = False
            except (FileNotFoundError, IOError):
                pass

        return self._bmc_presence

    def fsc_enable(self, enable=True):
        if self.with_bmc():
            if enable:
                status, result = self.run_command('/usr/bin/ipmitool raw 0x2e 0x04 0xcf 0xc2 0x00 1 0 0')
            else:
                status, result = self.run_command('/usr/bin/ipmitool raw 0x2e 0x04 0xcf 0xc2 0x00 1 0 1')
            return status
        else:
            if os.path.isfile(policy_json):
                keyword = 'True' if enable else 'False'
                cmd = "grep 'run_at_boot_up' {0} | grep {1}".format(policy_json, keyword)
                status, result = self.run_command(cmd)
                if status: 
                    return True
                else:
                    if keyword == 'True':
                        cmd = 'sed -i "4s/False/True/g" {1}'.format(policy_json)
                    else:
                        cmd = 'sed -i "4s/True/False/g" {1}'.format(policy_json)
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
                cmd = "grep 'run_at_boot_up' {0} | grep 'True'".format(policy_json)
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
