#!/usr/bin/env python

#############################################################################
# psuutil.py
# Platform-specific PSU status interface for SONiC
#############################################################################

import os.path
import sonic_platform

try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PSU_NAME_LIST = ["PSU-1", "PSU-2"]

PSU_HWMON_PATH = "/sys/bus/i2c/devices/{0}-00{1}/hwmon"
PSU_SYSFS_PATH = "/sys/bus/i2c/devices/{0}-00{1}"
CPLD_SYSFS_DIR = "/sys/bus/i2c/devices/0-0033"

PSU_I2C_MAPPING = {
    0: {
        "bus": 3,
        "addr": "59"
    },
    1: {
        "bus": 3,
        "addr": "58"
    },
}


class Psu(PsuBase):
    """Platform-specific Psu class"""

    def __init__(self, psu_index):
        self.PSU_TEMP_MAX = 70000
        self.PSU_OUTPUT_POWER_MAX = 1920000000
        self.PSU_OUTPUT_VOLTAGE_MIN = 40000
        self.PSU_OUTPUT_VOLTAGE_MAX = 62000
        self.index = psu_index
        PsuBase.__init__(self)
        self.i2c_num = PSU_I2C_MAPPING[self.index]["bus"]
        self.i2c_addr = PSU_I2C_MAPPING[self.index]["addr"]
        self.hwmon_path = PSU_HWMON_PATH.format(self.i2c_num, self.i2c_addr)
        self.sysfs_path = PSU_SYSFS_PATH.format(self.i2c_num, str(
            int(self.i2c_addr) - 8))  # eeprom addr always be mcu addr - 8
        self.__initialize_fan()

    def __initialize_fan(self):
        from sonic_platform.fan import Fan
        for fan_index in range(0, 1):
            fan = Fan(fan_index, 0, is_psu_fan=True, psu_index=self.index)
            self._fan_list.append(fan)

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return None

    def __search_hwmon_dir_name(self, directory):
        try:
            dirs = os.listdir(directory)
            for file in dirs:
                if file.startswith("hwmon"):
                    return file
        except:
            pass
        return ''

    def get_fan(self, index):
        """
        Retrieves object representing the fan module contained in this PSU
        Returns:
            An object dervied from FanBase representing the fan module
            contained in this PSU
        """
        return self._fan_list[index]

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU
        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        return self.get_status()

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED
        Args:
            color: A string representing the color with which to set the PSU status LED
                   Note: Only support green and off
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        # Hardware not supported
        return False

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        if self.get_presence():
            if self.get_powergood_status():
                return self.STATUS_LED_COLOR_GREEN
            else:
                return self.STATUS_LED_COLOR_RED
        else:
            return None

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return PSU_NAME_LIST[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        attr_file = 'psu{}_present'.format(self.index + 1)
        attr_path = CPLD_SYSFS_DIR + '/' + attr_file
        status = 0
        try:
            with open(attr_path, 'r') as psu_prs:
                status = int(psu_prs.read())
        except IOError:
            return False

        return status == 1

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        attr_file = 'psu{}_power_good'.format(self.index + 1)
        attr_path = CPLD_SYSFS_DIR + '/' + attr_file
        status = 0
        if self.get_presence():
            try:
                with open(attr_path, 'r') as power_status:
                    status = int(power_status.read())
            except IOError:
                return False

        return status == 1

    def get_model(self):
        """
        Retrieves the model name of the PSU
        Returns:
            model name
        """
        if self.get_presence():
            file_path = os.path.join(self.sysfs_path, "product_name")
            if os.path.exists(file_path):
                model = self.__read_txt_file(file_path)
                return model

        return None

    def get_serial(self):
        """
        Retrieves the serial number of the PSU
        Returns:
            model serial
        """
        if self.get_presence():
            file_path = os.path.join(self.sysfs_path, "serial_number")
            if os.path.exists(file_path):
                serial = self.__read_txt_file(file_path)
                return serial

        return None

    def get_revision(self):
        """
        Retrieves the hw rev of the PSU
        Returns:
            model rev
        """
        if self.get_presence():
            file_path = os.path.join(self.sysfs_path, "hw_revision")
            if os.path.exists(file_path):
                rev = self.__read_txt_file(file_path)
                return rev

        return None

    def get_voltage(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        hwmon_dir = self.__search_hwmon_dir_name(self.hwmon_path)
        if hwmon_dir == '':
            return None

        file_path = os.path.join(self.hwmon_path, hwmon_dir,
                                 "in2_input")
        voltage = self.__read_txt_file(file_path)
        if not voltage:
            return None
        return float(voltage) / 1000

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        hwmon_dir = self.__search_hwmon_dir_name(self.hwmon_path)
        if hwmon_dir == '':
            return None

        file_path = os.path.join(self.hwmon_path, hwmon_dir,
                                 "temp1_input")
        temp = self.__read_txt_file(file_path)
        if not temp:
            return None
        return float(temp) / 1000

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU
        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        hwmon_dir = self.__search_hwmon_dir_name(self.hwmon_path)
        if hwmon_dir == '':
            return None

        file_path = os.path.join(self.hwmon_path, hwmon_dir,
                                 "curr2_input")
        current = self.__read_txt_file(file_path)
        if not current:
            return None
        return float(current) / 1000

    def get_power(self):
        """
        Retrieves current energy supplied by PSU
        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        hwmon_dir = self.__search_hwmon_dir_name(self.hwmon_path)
        if hwmon_dir == '':
            return None

        file_path = os.path.join(self.hwmon_path, hwmon_dir,
                                 "power2_input")
        power = self.__read_txt_file(file_path)
        if not power:
            return None
        return float(power) / 1000000

    def get_voltage_vin(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A float number, the output voltage in volts,
            e.g. 220.0
        """
        hwmon_dir = self.__search_hwmon_dir_name(self.hwmon_path)
        if hwmon_dir == '':
            return None

        file_path = os.path.join(self.hwmon_path, hwmon_dir,
                                 "in1_input")
        voltage = self.__read_txt_file(file_path)
        if not voltage:
            return None
        return float(voltage) / 1000

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU
        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return float(self.PSU_TEMP_MAX/1000)

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output
        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        return float(self.PSU_OUTPUT_VOLTAGE_MAX/1000)

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output
        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        return float(self.PSU_OUTPUT_VOLTAGE_MIN/1000)

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU
        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        return float(self.PSU_OUTPUT_POWER_MAX/(1000 * 1000))

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.index + 1

    def is_replaceable(self):
        return True

    def get_num_fans(self):
        """
        Retrieves the number of fan modules available on this PSU

        Returns:
            An integer, the number of fan modules available on this PSU
        """
        return len(self._fan_list)

    def get_all_fans(self):
        """
        Retrieves all fan modules available on this PSU

        Returns:
            A list of objects derived from FanBase representing all fan
            modules available on this PSU
        """
        return self._fan_list
