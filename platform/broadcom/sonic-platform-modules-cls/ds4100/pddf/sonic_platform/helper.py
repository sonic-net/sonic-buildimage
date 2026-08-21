import fcntl
import os
import struct
import subprocess
import shlex
from mmap import *

BMC_PRES_SYS_PATH = '/sys/bus/platform/devices/sys_cpld/bmc_present'

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
        try:
            data = subprocess.check_output(shlex.split(cmd) if isinstance(cmd, str) else cmd,
                    shell=False, universal_newlines=True, stderr=subprocess.STDOUT).strip()
            status = 0
        except subprocess.CalledProcessError as ex:
            data = ex.output
            status = ex.returncode
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

    @staticmethod
    def ipmi_raw(cmd):
        status = True
        result = ""
        try:
            cmd = ["ipmitool", "raw"] + cmd
            p = subprocess.Popen(cmd, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            raw_data, err = p.communicate()
            if err.decode("utf-8") == "":
                result = raw_data.decode("utf-8").strip()
            else:
                status = False
        except Exception:
            status = False
        return status, result

    def is_bmc_present(self):
        """
        Get the BMC card present status

        Returns:
            A boolean, True if present, False if absent
        """

        presence = self.read_txt_file(BMC_PRES_SYS_PATH)
        if presence == None:
            print("Failed to get BMC card presence status")
        return True if presence == "1" else False

    @staticmethod
    def run_command(cmd):
        status = True
        bmc_present = APIHelper().is_bmc_present()
        if bmc_present:
            result = ""
            try:
                p = subprocess.Popen(cmd, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                raw_data, err = p.communicate()
                if err.decode("utf-8") == "":
                    result = raw_data.decode("utf-8").strip()
            except Exception:
                status = False
            return status, result
        else:
            try:
                data = subprocess.check_output(shlex.split(cmd) if isinstance(cmd, str) else cmd, shell=False, universal_newlines=True, stderr=subprocess.STDOUT).strip()
            except subprocess.CalledProcessError as ex:
                data = ex.output
                status = False
            return status, data

    def get_status_output(self, cmd):
        try:
            data = subprocess.check_output(shlex.split(cmd) if isinstance(cmd, str) else cmd,
                    shell=False, universal_newlines=True, stderr=subprocess.STDOUT).strip()
            status = 0
        except subprocess.CalledProcessError as ex:
            data = ex.output
            status = ex.returncode
        if data[-1:] == '\n':
            data = data[:-1]
        return status, data

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
