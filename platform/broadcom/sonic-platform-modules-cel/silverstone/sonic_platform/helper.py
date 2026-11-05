import fcntl
import os
import struct
import subprocess
import fcntl

from mmap import *
from sonic_py_common.general import check_output_pipe
from sonic_py_common.device_info import get_platform_and_hwsku

from sonic_py_common import device_info

HOST_CHK_CMD = "docker > /dev/null 2>&1"
EMPTY_STRING = ""


class APIHelper():

    def __init__(self):
        (self.platform, self.hwsku) = get_platform_and_hwsku()

    def is_host(self):
        ret,data = subprocess.getstatusoutput(HOST_CHK_CMD)
        if ret != 0:
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

    def run_command(self, cmd1_args, cmd2_args=None):
        status = True
        result = ""
        args = [cmd1_args] + ([cmd2_args] if cmd2_args is not None else [])
        try:
            result = check_output_pipe(*args)
        except subprocess.CalledProcessError:
            status = False
        return status, result

    def run_interactive_command(self, cmd):
        try:
            subprocess.call(cmd)
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
        cmd = "ipmitool raw {} {}".format(netfn, cmd)
        ret, data = subprocess.getstatusoutput(cmd)
        if ret != 0:
            status = False
        else:
            result = data.strip()
        
        return status, result

    def ipmi_fru_id(self, id, key=None):
        status = True
        result = ""
        cmd1_args = ["ipmitool", "fru", "print", str(id)]
        if not key:
            ret, data = subprocess.getstatusoutput(cmd)
            if ret != 0:
                status = False
            else:
                result = data.strip()
        else:
            cmd2_args = ["grep", str(key)]
            status, result = self.run_command(cmd1_args, cmd2_args)
        return status, result

    def ipmi_set_ss_thres(self, id, threshold_key, value):
        status = True
        result = ""
        cmd = "ipmitool sensor thresh '{}' {} {}".format(
            str(id), str(threshold_key), str(value))
        ret, data = subprocess.getstatusoutput(cmd)
        if ret != 0:
            status = False
        else:
            result = data.strip()
        
        return status, result

    def lpc_getreg(self, getreg_path, reg):
        """
        Get the cpld reg through lpc interface

        Args:
            getreg_path: getreg sysfs path
            reg: 16 bits reg addr in hex str format

        Returns:
            A str, register value in hex str format
        """
        try:
            file = open(getreg_path, 'w+')
            # Acquire an exclusive lock on the file
            fcntl.flock(file, fcntl.LOCK_SH)

            file.write(reg)
            file.flush()

            # Seek to the beginning of the file
            file.seek(0)

            # Read the content of the file
            result = file.readline().strip()
        except (OSError, IOError, FileNotFoundError):
            result = None
        finally:
            # Release the lock and close the file
            fcntl.flock(file, fcntl.LOCK_UN)
            file.close()

        return result

    def lpc_setreg(self, setreg_path, reg, val):
        """
        Set the cpld reg through lpc interface

        Args:
            setreg_path: setreg sysfs path
            reg: 16 bits reg addr in hex str format
            val: 8 bits register value in hex str format

        Returns:
            A boolean, True if speed is set successfully, False if not
        """
        status = True
        try:
            file = open(setreg_path, 'w')
            # Acquire an exclusive lock on the file
            fcntl.flock(file, fcntl.LOCK_EX)

            data = "{} {}".format(reg, val)
            file.write(data)
            file.flush()
        except (OSError, IOError, FileNotFoundError):
            status = False
        finally:
            # Release the lock and close the file
            fcntl.flock(file, fcntl.LOCK_UN)
            file.close()

        return status
