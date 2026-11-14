#!/usr/bin/env python3

#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

import os
import logging
import subprocess

try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.led import LedController
    from sonic_platform.i2c import i2c_read_byte_data
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

# PMBus block-read commands (standard) — not supported by this PSU (returns 0xff)
# PMBUS_MFR_MODEL  = 0x9A
# PMBUS_MFR_SERIAL = 0x9E

# Vendor-specific registers: model and serial stored one char per register.
# Each register address holds one ASCII character of the string.
# 0xff marks the end of the string.
PSU_VENDOR_MODEL_REG_START  = 0xE0
PSU_VENDOR_MODEL_REG_END    = 0xEB
PSU_VENDOR_SERIAL_REG_START = 0xD4
PSU_VENDOR_SERIAL_REG_END   = 0xDE

# Directory where init.sh and _register_i2c_devices() save identity
# files before binding the pmbus driver (direct i2c access blocked after)
PSU_IDENTITY_DIR = "/run/sonic/psu"

PSU_HWMON_PATH = "/sys/bus/i2c/devices/i2c-{0}/{0}-00{1}/hwmon"
PSU_I2C_DEVICE_PATH = "/sys/bus/i2c/devices/{0}-00{1}"
PSU_I2C_MAPPING = {
    1: {
        "i2c_num": 2,
        "psu_addr": "3c",
    },
    0: {
        "i2c_num": 2,
        "psu_addr": "3d",
    },
}

class Psu(PsuBase):
    """Platform-specific Psu class"""

    PSU_NAME_LIST = ["PSU-1", "PSU-2"]
    psu_led_status_list = ["ok", "ok"]

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        self.index = psu_index
        self.led = LedController()
        self.hwmon_path = PSU_HWMON_PATH.format(PSU_I2C_MAPPING[self.index]['i2c_num'], PSU_I2C_MAPPING[self.index]['psu_addr'])
        self._model_cache = None
        self._serial_cache = None

    def get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", attr_path, " file !")

        retval = retval.rstrip('\r\n')

        fd.close()
        return retval

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU
        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        return self.get_presence()

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self.PSU_NAME_LIST[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the PSU.

        Three-stage check to handle boot-time absence and hot-plug:
        1. hwmon exists  → driver fully bound (normal operation).
        2. device dir + driver symlink → driver still binding after hot-plug.
        3. nothing registered → probe raw bus:
             ACK  → hot-plugged; register pmbus device so driver binds.
             NACK → absent; return False.
        """
        # Stage 1: hwmon exists → fully working
        if os.path.exists(self.hwmon_path):
            return True

        i2c_num = PSU_I2C_MAPPING[self.index]['i2c_num']
        psu_addr_str = PSU_I2C_MAPPING[self.index]['psu_addr']
        device_path = PSU_I2C_DEVICE_PATH.format(i2c_num, psu_addr_str)

        # Stage 2: device registered + driver symlink → binding in progress
        if os.path.exists(device_path) and os.path.exists(os.path.join(device_path, "driver")):
            return True

        # Stage 3: nothing registered — probe the raw bus
        try:
            i2c_read_byte_data(i2c_num, int(psu_addr_str, 16))
        except Exception:
            return False

        # PSU responded — register devices so hwmon is created
        self._register_i2c_devices()
        return True

    def _register_i2c_devices(self):
        """
        Write pmbus entry to new_device so the kernel binds the driver and
        creates the hwmon sysfs path. Called on hot-plug detection.
        """
        i2c_num = PSU_I2C_MAPPING[self.index]['i2c_num']
        psu_addr = PSU_I2C_MAPPING[self.index]['psu_addr']
        device_path = PSU_I2C_DEVICE_PATH.format(i2c_num, psu_addr)

        # If the device directory already exists skip registration to avoid
        # an EBUSY error and the resulting log spam.
        if os.path.exists(device_path):
            return

        new_device = "/sys/bus/i2c/devices/i2c-{}/new_device".format(i2c_num)
        try:
            with open(new_device, 'w') as f:
                f.write("pmbus 0x{}".format(psu_addr))
            logging.info("PSU-{}: i2c device registered on hot-plug".format(self.index + 1))
        except Exception as e:
            logging.error("PSU-{}: failed to register i2c device: {}".format(self.index + 1, e))

    def _read_pmbus_string(self, reg_start, reg_end):
        """
        Read vendor-specific registers one byte at a time using i2ctransfer
        with -f (force) flag, which allows access even when the pmbus driver
        is bound. Assembles ASCII chars into a string; 0xff = end of string.
        Result is cached by get_model()/get_serial() callers.
        """
        i2c_num = PSU_I2C_MAPPING[self.index]['i2c_num']
        psu_addr = PSU_I2C_MAPPING[self.index]['psu_addr']
        return self._read_vendor_regs(i2c_num, psu_addr, reg_start, reg_end)

    def _read_vendor_regs(self, i2c_num, psu_addr, reg_start, reg_end):
        """
        Read consecutive vendor-specific registers using i2ctransfer -f.
        The -f flag bypasses driver exclusion so we can read even when
        the pmbus driver is bound to the device.
        """
        result = []
        for reg in range(reg_start, reg_end + 1):
            try:
                out = subprocess.run(
                    ["i2ctransfer", "-y", "-f", str(i2c_num),
                     "w1@0x{}".format(psu_addr), hex(reg), "r1"],
                    capture_output=True, text=True, timeout=2
                )
                if out.returncode != 0:
                    break
                b = int(out.stdout.strip(), 16)
                if b == 0xff:
                    break
                if 0x20 <= b <= 0x7e:  # ASCII printable only
                    result.append(chr(b))
            except Exception:
                break
        return ''.join(result).strip() or "N/A"
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()

    def get_voltage(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        psu_voltage = self._get_hwmon_value("in{}_input", "vout1", "in")
        return float(psu_voltage) / 1000

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU
        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        psu_current = self._get_hwmon_value("curr{}_input", "iout1", "cur")
        return float(psu_current) / 1000

    def get_power(self):
        """
        Retrieves current energy supplied by PSU
        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        psu_power = self._get_hwmon_value("power{}_input", "pout1", "power")
        return float(psu_power) / 1000000

    def get_model(self):
        """
        Retrieves the model from vendor-specific registers 0xE0-0xEA.
        Read once and cached — model is static data.
        """
        if self._model_cache is None:
            self._model_cache = self._read_pmbus_string(
                PSU_VENDOR_MODEL_REG_START, PSU_VENDOR_MODEL_REG_END)
        return self._model_cache

    def get_serial(self):
        """
        Retrieves the serial from vendor-specific registers 0xD0-0xDE.
        Read once and cached — serial is static data.
        """
        if self._serial_cache is None:
            self._serial_cache = self._read_pmbus_string(
                PSU_VENDOR_SERIAL_REG_START, PSU_VENDOR_SERIAL_REG_END)
        return self._serial_cache

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def _get_hwmon_value(self, name, label, start):
        value = 0.0

        label_path = self._search_file_by_contain(
            self.hwmon_path, label, start)
        if label_path:
            dir_name = os.path.dirname(label_path)
            basename = os.path.basename(label_path)
            in_num = ''.join(list(filter(str.isdigit, basename)))
            file_path = os.path.join(dir_name, name.format(in_num))
            val = self.get_attr_value(file_path)
            if val != 'ERR':
                value = float(val)
        return value

    def _search_file_by_contain(self, directory, search_str, file_start):
        for dirpath, dirnames, files in os.walk(directory):
            for name in files:
                file_path = os.path.join(dirpath, name)
                if name.startswith(file_start) and search_str in self.get_attr_value(file_path):
                    return file_path
        return None



    def set_status_led(self, color):
        """
        Set the PSU module LED and update the global LED based on the state of all PSUs.

        Args:
            color (str): LED color for this PSU module ("green", "red", "amber", etc.)

        Returns:
            bool: True if the LED update was successful
        """

        is_ok = (color == self.STATUS_LED_COLOR_GREEN)

        self.psu_led_status_list[self.index] = "ok" if is_ok else "fault"

        if all(status == "ok" for status in self.psu_led_status_list):
            final_color = self.STATUS_LED_COLOR_GREEN
        else:
            final_color = self.STATUS_LED_COLOR_RED

        self.led.set_led("pwr", final_color)

        return True

    def get_temperature(self):
        """
        Retrieves current PSU temperature reading

        Returns:
            A float number of current temperature in Celsius
        """
        temp = 0.0

        try:
            for dirpath, _, files in os.walk(self.hwmon_path):
                for name in files:
                    if name.endswith("_input") and name.startswith("temp"):
                        file_path = os.path.join(dirpath, name)
                        value = self.get_attr_value(file_path)

                        if value != 'ERR':
                            value = float(value) / 1000
                            temp = max(temp, value)

        except Exception as e:
            print("Failed to read PSU temperature: {}".format(str(e)))

        return temp

    def get_temperature_high_threshold(self):
        """
        Retrieves PSU high temperature threshold

        Returns:
            A float number of PSU temperature threshold in Celsius
        """
        threshold = 0.0

        try:
            for dirpath, _, files in os.walk(self.hwmon_path):
                for name in files:
                    if name.endswith("_rated_max") and name.startswith("temp"):
                        file_path = os.path.join(dirpath, name)
                        value = self.get_attr_value(file_path)

                        if value != 'ERR':
                            value = float(value) / 1000
                            threshold = max(threshold, value)

        except Exception as e:
            print("Failed to read PSU temperature threshold: {}".format(str(e)))

        return threshold

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output

        Returns:
            A float number, the high threshold output voltage in volts
        """
        return 12.6

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output

        Returns:
            A float number, the low threshold output voltage in volts
        """
        return 11.4

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        if not self.get_presence():
            return  self.STATUS_LED_COLOR_OFF

        status=self.get_status()
        if status is None:
            return  self.STATUS_LED_COLOR_OFF

        return {
            1: self.STATUS_LED_COLOR_GREEN,
            0: self.STATUS_LED_COLOR_RED
        }.get(status, self.STATUS_LED_COLOR_OFF)
