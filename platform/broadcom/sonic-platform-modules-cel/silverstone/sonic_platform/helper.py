import fcntl
import os
import struct
import subprocess
from mmap import *

from sonic_py_common import device_info

HOST_CHK_CMD = "docker > /dev/null 2>&1"
EMPTY_STRING = ""


class APIHelper():

    def __init__(self):
        (self.platform, self.hwsku) = device_info.get_platform_and_hwsku()

    def is_host(self):
        return os.system(HOST_CHK_CMD) == 0

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
