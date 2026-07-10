import fcntl
import os
import struct
import subprocess
from mmap import *

from sonic_py_common import device_info

HOST_CHK_CMD = ["docker"]
EMPTY_STRING = ""


class APIHelper():

    def __init__(self):
        (self.platform, self.hwsku) = device_info.get_platform_and_hwsku()

    def is_host(self):
        try:
            subprocess.call(HOST_CHK_CMD, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except FileNotFoundError:
            return False
        return True

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
                cmd, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            raw_data, err = p.communicate()
            if err == '':
                result = raw_data.strip()
        except:
            status = False
        return status, result

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
        file = open(getreg_path, 'w+')
        # Acquire an exclusive lock on the file
        fcntl.flock(file, fcntl.LOCK_EX)

        try:
            file.write(register + '\n')
            file.flush()

            # Seek to the beginning of the file
            file.seek(0)

            # Read the content of the file
            result = file.readline().strip()
        finally:
            # Release the lock and close the file
            fcntl.flock(file, fcntl.LOCK_UN)
            file.close()

        return result

