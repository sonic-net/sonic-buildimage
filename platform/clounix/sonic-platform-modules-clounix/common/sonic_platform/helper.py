import os
import struct
import subprocess
import json
import shlex
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
        try:
            with open(HOST_CHK_FILE, "r") as fd:
                data = fd.read()
        except IOError:
            return True
        return HOST_CHK_STR not in data

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
            command = cmd if isinstance(cmd, list) else shlex.split(str(cmd))
            proc = subprocess.run(
                command,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
            )
            if proc.returncode == 0:
                result = proc.stdout.strip()
            else:
                status = False
        except:
            status = False
        return status, result

    def run_interactive_command(self, cmd):
        try:
            command = cmd if isinstance(cmd, list) else shlex.split(str(cmd))
            subprocess.run(command, check=False)
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
        try:
            with open(getreg_path, "w") as fd:
                fd.write(str(register))
            with open(getreg_path, "r") as fd:
                return fd.read().strip()
        except IOError:
            return None

    def ipmi_raw(self, netfn, cmd):
        status = True
        result = ""
        try:
            raw_args = ["ipmitool", "raw"] + str(netfn).split() + str(cmd).split()
            proc = subprocess.run(
                raw_args,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
            )
            if proc.returncode == 0:
                result = proc.stdout.strip()
            else:
                status = False
        except:
            status = False
        return status, result

    def ipmi_fru_id(self, id, key=None):
        status = True
        result = ""
        try:
            proc = subprocess.run(
                ["ipmitool", "fru", "print", str(id)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
            )
            if proc.returncode != 0:
                status = False
            else:
                raw_data = proc.stdout.strip()
                if key:
                    result_lines = [line for line in raw_data.splitlines() if str(key) in line]
                    result = "\n".join(result_lines).strip()
                else:
                    result = raw_data
        except:
            status = False
        return status, result

    def ipmi_set_ss_thres(self, id, threshold_key, value):
        status = True
        result = ""
        try:
            proc = subprocess.run(
                ["ipmitool", "sensor", "thresh", str(id), str(threshold_key), str(value)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
            )
            if proc.returncode == 0:
                result = proc.stdout.strip()
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
