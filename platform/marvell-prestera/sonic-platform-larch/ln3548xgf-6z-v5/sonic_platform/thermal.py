#!/usr/bin/env python

#############################################################################
# Pegatron Mercedes
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
            self.THERMAL_NAME_LIST.append("OOB temperature sensor : 1")
            self.THERMAL_NAME_LIST.append("Main board temperature sensor : 1")

        # Set thermal path
        thermal_path = {
            0: "/sys/class/hwmon/hwmon0",
            1: "/sys/class/hwmon/hwmon1",
        }.get(self.index, None)

        # Current temprature file names
        cur_temp = {
            0: "temp1_input",
            1: "temp1_input",
        }.get(self.index, None)

        # Max temprature file names
        max_temp = {
            0: "",
            1: "temp1_max",
        }.get(self.index, None)

        # Critical high temprature file names
        crit_temp = {
            0: "temp1_crit",
            1: "",
        }.get(self.index, None)

        # Max temprature default values
        # 0 - real value from temp1_max
        # 1 - same as temp1_max for hwmon0
        self.max_temp_def_val = {
            0: 80.0,
            1: 80.0,
        }.get(self.index, None)

        # Critical high temprature default values
        # 0 - same as temp1_crit for hwmon1
        # 1 - real value from temp1_crit
        self.crit_temp_def_val = {
            0: 100.0,
            1: 100.0,
        }.get(self.index, None)

        self.mid_low_temp_def_val = 30.0
        self.mid_high_temp_def_val = 65.0

       # Set hwmon path
        self.cur_temp_file = "{}/{}".format(thermal_path, cur_temp)
        self.max_temp_file = "{}/{}".format(thermal_path, max_temp) if max_temp else None
        self.crit_temp_file = "{}/{}".format(thermal_path, crit_temp) if crit_temp else None
        self.ss_key = self.THERMAL_NAME_LIST[self.index]
        self.ss_index = 1

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

    def get_mid_low_threshold(self):
        return self.mid_low_temp_def_val

    def get_mid_high_threshold(self):
        return self.mid_high_temp_def_val

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal sensor
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.index == 1:
            return self.__get_temp(self.max_temp_file)
        else:
            return self.max_temp_def_val


    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal sensor
        Actions should be taken immediately if the temperature becomes higher than the high critical
        threshold otherwise the device will be damaged.

        :return: A float number, the high critical threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.index == 0:
            return self.__get_temp(self.crit_temp_file)
        else:
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
        return os.path.isfile(self.cur_temp_file)

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if not self.get_presence():
            return False

        return True
