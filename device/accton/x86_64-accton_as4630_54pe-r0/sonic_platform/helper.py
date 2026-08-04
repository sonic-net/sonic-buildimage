import os
import struct
from mmap import *
from sonic_py_common import device_info
from sonic_py_common.general import getstatusoutput_noshell

HOST_CHK_CMD = ["docker"]
EMPTY_STRING = ""


class APIHelper():

    def __init__(self):
        (self.platform, self.hwsku) = device_info.get_platform_and_hwsku()

    def is_host(self):
        try:
            status, output = getstatusoutput_noshell(HOST_CHK_CMD)
            return status == 0
        except Exception:
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
        except Exception:
            status = False
        return status, result

    def read_txt_file(self, file_path):
        try:
            with open(file_path, 'r', errors='replace') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return None

    def write_txt_file(self, file_path, value):
        try:
            with open(file_path, 'w') as fd:
                fd.write(str(value))
        except IOError:
            return False
        return True

