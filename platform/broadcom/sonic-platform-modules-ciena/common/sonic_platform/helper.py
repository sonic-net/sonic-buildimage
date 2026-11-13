import os
import struct
from mmap import *
from sonic_py_common.general import getstatusoutput_noshell

HOST_CHK_CMD = ["docker"]
EMPTY_STRING = ""


class APIHelper():

    def __init__(self):
        pass

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

