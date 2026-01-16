import os
import struct
import subprocess
import json
from mmap import *

from sonic_py_common import device_info

HOST_CHK_CMD = "docker > /dev/null 2>&1"
HOST_CHK_STR = "/docker"
HOST_CHK_FILE = "/proc/1/cgroup"
EMPTY_STRING = ""

HOST_PLATFORM_JSON = '/usr/share/sonic/device/' + str(device_info.get_platform()) + '/platform.json'
PMON_PLATFORM_JSON = '/usr/share/sonic/platform/platform.json'

class APIHelper():

    def __init__(self):
        (self.platform, self.hwsku) = device_info.get_platform_and_hwsku()

    def is_host(self):
        fd = os.popen("cat " + HOST_CHK_FILE + " | grep " + HOST_CHK_STR)
        data = fd.read()
        if data is "" :
            return True
        else :
            return False

    def pci_get_value(self, resource, offset):
        status = True
        result = ""
        try:
            fd = os.open(resource, os.O_RDWR)
            mm = mmap(fd, 0)
            mm.seek(int(offset))
            read_data_stream = mm.read(4)
            result = struct.unpack('I', read_data_stream)
        except:
            status = False
        return status, result

    def run_command(self, cmd):
        status = True
        result = ""
        try:
            p = subprocess.Popen(
                cmd, shell=True, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            raw_data, err = p.communicate()
            if err == '':
                result = raw_data.strip()
        except:
            status = False
        return status, result

    def run_interactive_command(self, cmd):
        try:
            os.system(cmd)
        except:
            return False
        return True

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
        except Exception:
            return False
        return True

    def get_cpld_reg_value(self, getreg_path, register):
        cmd = "echo {1} > {0}; cat {0}".format(getreg_path, register)
        status, result = self.run_command(cmd)
        return result if status else None

    def ipmi_raw(self, netfn, cmd):
        status = True
        result = ""
        try:
            cmd = "ipmitool raw {} {}".format(str(netfn), str(cmd))
            p = subprocess.Popen(
                cmd, shell=True, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            raw_data, err = p.communicate()
            if err == '':
                result = raw_data.strip()
            else:
                status = False
        except:
            status = False
        return status, result

    def ipmi_fru_id(self, id, key=None):
        status = True
        result = ""
        try:
            cmd = "ipmitool fru print {}".format(str(
                id)) if not key else "ipmitool fru print {0} | grep '{1}' ".format(str(id), str(key))

            p = subprocess.Popen(
                cmd, shell=True, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            raw_data, err = p.communicate()
            if err == '':
                result = raw_data.strip()
            else:
                status = False
        except:
            status = False
        return status, result

    def ipmi_set_ss_thres(self, id, threshold_key, value):
        status = True
        result = ""
        try:
            cmd = "ipmitool sensor thresh '{}' {} {}".format(
                str(id), str(threshold_key), str(value))
            p = subprocess.Popen(
                cmd, shell=True, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            raw_data, err = p.communicate()
            if err == '':
                result = raw_data.strip()
            else:
                status = False
        except:
            status = False
        return status, result

    def get_attr_conf(self, attr):
        """
        Retrieves the json object from json file path

        Returns:
            A json object
        """
        json_path = ''
        if self.is_host() is True:
            json_path = HOST_PLATFORM_JSON
        else :
            json_path = PMON_PLATFORM_JSON
        with open(json_path, 'r') as f:
            json_data = json.load(f)

        return json_data[attr]
