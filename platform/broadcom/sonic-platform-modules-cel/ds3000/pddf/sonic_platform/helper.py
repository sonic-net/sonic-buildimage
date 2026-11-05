import fcntl
import os
import struct
import subprocess
from mmap import *

BMC_PRES_SYS_PATH = '/sys/bus/platform/devices/baseboard/bmc_presence'

class APIHelper():
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

    def get_cmd_output(self, cmd):
        status = 0
        ret, data = subprocess.getstatusoutput(cmd)
        if ret != 0:
            status = ret
        
        return status, data

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

    def lpc_getreg(self, getreg_path, reg):
        """
        Get the cpld reg through lpc interface

        Args:
            getreg_path: getreg sysfs path
            reg: 16 bits reg addr in hex str format

        Returns:
            A str, register value in hex str format
        """
        file = open(getreg_path, 'w+')
        # Acquire an exclusive lock on the file
        fcntl.flock(file, fcntl.LOCK_EX)

        try:
            file.write(reg)
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
        file = open(setreg_path, 'w')
        # Acquire an exclusive lock on the file
        fcntl.flock(file, fcntl.LOCK_EX)

        try:
            data = "{} {}".format(reg, val)
            file.write(data)
            file.flush()
        except:
            status = False
        finally:
            # Release the lock and close the file
            fcntl.flock(file, fcntl.LOCK_UN)
            file.close()

        return status

    def is_bmc_present(self):
        """
        Get the BMC card present status

        Returns:
            A boolean, True if present, False if absent
        """
        presence = self.read_txt_file(BMC_PRES_SYS_PATH)
        if presence == None:
            print("Failed to get BMC card presence status")
        return True if presence == "present" else False

    def run_command(self,cmd):
        status = True
        result = ""
        ret, data = subprocess.getstatusoutput(cmd)
        if ret != 0:
            status = False
        else:
            result = data

        return status, result
