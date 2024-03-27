import os
import struct
import subprocess
from mmap import *

from sonic_py_common import device_info


class APIHelper():
    def __init__(self):
        (self.platform, self.hwsku) = device_info.get_platform_and_hwsku()

        self.gpio_dir = "/sys/class/gpio"
        self.gpio_label = "17-0020"
        self.gpio_base = self._gpio_base(984)

        self.cpld_base = "/sys/devices/platform/redstone_xp_cpld"

    def is_host(self):
        return os.system("docker > /dev/null 2>&1") == 0

    def pci_get_value(self, resource, offset):
        status = True
        result = ""
        try:
            fd = os.open(resource, os.O_RDWR)
            mm = mmap(fd, 0)
            mm.seek(int(offset))
            read_data_stream = mm.read(4)
            result = struct.unpack('I', read_data_stream)
        except IOError:
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
        except Exception:
            status = False
        return status, result

    def run_interactive_command(self, cmd):
        try:
            os.system(cmd)
        except Exception:
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
        except IOError:
            return False
        return True

    def get_hwmon_path(self, i2c_adapter, i2c_address):
        base_path = "/sys/bus/i2c/devices/i2c-{0}/{0}-00{1}/hwmon".format(i2c_adapter, i2c_address)
        hwmon_path = os.listdir(base_path)
        hwmon_path = hwmon_path[0] if len(hwmon_path) > 0 else ""
        return os.path.normpath("{}/{}".format(base_path, hwmon_path))

    def _gpio_base(self, default):
        for r in os.listdir(self.gpio_dir):
            label_path = os.path.join(self.gpio_dir, r, "label")
            if "gpiochip" in r and self.gpio_label in self.read_txt_file(label_path):
                return int(r[8:], 10)
        return int(default)

    def get_gpio(self, gpio):
        gpio_dir = self.gpio_dir + '/gpio' + str(self.gpio_base + int(gpio))
        gpio_file = gpio_dir + "/value"
        retval = self.read_txt_file(gpio_file)
        return retval.rstrip('\r\n')

    def set_gpio(self, gpio, value):
        gpio_dir = self.gpio_dir + '/gpio' + str(self.gpio_base + int(gpio))
        gpio_file = gpio_dir + "/value"
        return self.write_txt_file(gpio_file, value)

    def get_cpld(self, path, register=None):
        path = os.path.normpath("{}/{}".format(self.cpld_base, str(path).split("/")[-1]))
        if register:
            cmd = "echo {1} > {0}; cat {0}".format(path, register)
            status, result = self.run_command(cmd)
            return result if status else None
        else:
            return self.read_txt_file(path)

    def set_cpld(self, path, value, register=None):
        path = os.path.normpath("{}/{}".format(self.cpld_base, str(path).split("/")[-1]))
        value = "{} {}".format(register, value) if register else value
        return self.write_txt_file(path, value)
