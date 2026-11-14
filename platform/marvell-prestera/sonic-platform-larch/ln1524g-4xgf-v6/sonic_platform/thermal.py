#!/usr/bin/env python

#############################################################################
#
# Thermal contains an implementation of SONiC Platform Base API and
# provides the thermal device status which are available in the platform
#
#############################################################################

import os
import os.path

try:
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    THERMAL_NAME_LIST = []

    def __init__(self, thermal_index):
        self.index = thermal_index

        # Add thermal name
        if not self.THERMAL_NAME_LIST:
            self.THERMAL_NAME_LIST.append("Main board temperature sensor : 1")
            self.THERMAL_NAME_LIST.append("Main board temperature sensor : 2")
            self.THERMAL_NAME_LIST.append("Network module 1 temperature sensor")
            self.THERMAL_NAME_LIST.append("Network module 2 temperature sensor")
            self.THERMAL_NAME_LIST.append("Network module 3 temperature sensor")

        # i2c device backing each thermal sensor. The hwmon directory is
        # resolved through the i2c device (see __get_hwmon_dir) instead of a
        # fixed /sys/class/hwmon/hwmonN index, so the mapping stays correct
        # regardless of hwmon enumeration/probe order.
        #   index 0/1   : main board lm75a on i2c bus 0 (0x48 / 0x49)
        #   index 2/3/4 : network module lm75a behind the mlxcpld i2c-mux
        #                 (mux channels 0x05/0x06/0x07 -> i2c buses 7/8/9)
        self.i2c_dev = {
            0: "0-0048",
            1: "0-0049",
            2: "7-0048",
            3: "8-0048",
            4: "9-0048",
        }.get(self.index, None)

        # hwmon attribute base names (resolved lazily against the i2c device)
        self.cur_temp_name = "temp1_input"
        self.max_temp_name = "temp1_max"

        # Critical high temprature default value
        self.crit_temp_def_val = 100.0

        self.ss_key = self.THERMAL_NAME_LIST[self.index]
        self.ss_index = 1

    def __get_hwmon_dir(self):
        """Resolve the hwmon directory of the backing i2c device, if present."""
        if self.i2c_dev is None:
            return None
        hwmon_root = "/sys/bus/i2c/devices/{}/hwmon".format(self.i2c_dev)
        try:
            for entry in sorted(os.listdir(hwmon_root)):
                if entry.startswith("hwmon"):
                    return os.path.join(hwmon_root, entry)
        except OSError:
            return None
        return None

    @property
    def cur_temp_file(self):
        hwmon = self.__get_hwmon_dir()
        return "{}/{}".format(hwmon, self.cur_temp_name) if hwmon else None

    @property
    def max_temp_file(self):
        hwmon = self.__get_hwmon_dir()
        return "{}/{}".format(hwmon, self.max_temp_name) if hwmon else None

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            raise IOError("Unable to open %s file !" % file_path)

    def __get_temp(self, temp_file):
        raw_temp = self.__read_txt_file(temp_file)

        temp = float(raw_temp)/1000

        return float("{:.3f}".format(temp))

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        return self.__get_temp(self.cur_temp_file)

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal sensor
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.__get_temp(self.max_temp_file)

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal sensor
        Actions should be taken immediately if the temperature becomes higher than the high critical
        threshold otherwise the device will be damaged.

        :return: A float number, the high critical threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.crit_temp_def_val

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal sensor
        Actions should be taken immediately if the temperature becomes lower than the low critical
        threshold otherwise the device will be damaged.

        :return: A float number, the low critical threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return 0.0

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal sensor
        Actions should be taken if the temperature becomes lower than the low threshold.

        :return: A float number, the low threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return 0.0

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        return self.THERMAL_NAME_LIST[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the thermal sensor
        Returns:
            bool: True if thermal sensor is present, False if not
        """
        cur_temp_file = self.cur_temp_file
        return bool(cur_temp_file) and os.path.isfile(cur_temp_file)

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if not self.get_presence():
            return False

        return True
