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
    from sonic_platform.sfp import Sfp
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    THERMAL_NAME_LIST = []
    SYSFS_THERMAL_DIR = ["/sys/bus/i2c/devices/2-004a/hwmon/",
                         "/sys/bus/i2c/devices/2-0049/hwmon/",
                         "/sys/bus/i2c/devices/2-004b/hwmon/",
                         "/sys/bus/i2c/devices/3-0059/hwmon/",
                         "/sys/bus/i2c/devices/3-0058/hwmon/",
                         "/sys/devices/virtual/thermal/thermal_zone1/",
                         "/sys/bus/i2c/devices/0-001b/hwmon/"]
    THERMAL_NAME_LIST = [
        "XFMR Ambient",
        "DDR Ambient",
        "System Ambient",
        "PSU 1 Temp",
        "PSU 2 Temp",
        "CPU Temp",
        "Dimm Temp",
        "PoE Temp",
        "MAC Temp",
        "XCVR 1 Temp",
        "XCVR 2 Temp",
        "XCVR 3 Temp",
        "XCVR 4 Temp",
        "XCVR 5 Temp",
        "XCVR 6 Temp"
    ]

    def __init__(self, thermal_index):
        self.index = thermal_index
        if thermal_index >= 9:
            self.sfp_module = Sfp(49 + (thermal_index - 9), 'SFP')

        ThermalBase.__init__(self)
        self.minimum_thermal = 150.0
        self.maximum_thermal = 0.0

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return ""

    def __search_hwmon_dir_name(self, directory):
        try:
            dirs = os.listdir(directory)
            for file in dirs:
                if file.startswith("hwmon"):
                    return file
        except:
            pass
        return ''

    def __get_temp(self, temp_file):
        hwmon_dir = self.__search_hwmon_dir_name(self.SYSFS_THERMAL_DIR[self.index])
        temp_file_path = os.path.join(self.SYSFS_THERMAL_DIR[self.index], hwmon_dir, temp_file)
        raw_temp = self.__read_txt_file(temp_file_path)
        if not raw_temp:
            return None
        try:
            temp = float(raw_temp)/1000
            return "{:.3f}".format(temp)
        except (ValueError, TypeError):
            return None

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        if self.index < 5 or self.index == 6:
            temp_file = "temp1_input"
            if self.get_presence():
                val = self.__get_temp(temp_file)
                return float(val) if val is not None else 0.0
        elif self.index == 5:
            temp_file = "temp"
            temp_file_path = os.path.join(self.SYSFS_THERMAL_DIR[self.index], temp_file)
            raw_temp = self.__read_txt_file(temp_file_path)
            if not raw_temp:
                return 0.0
            try:
                temp = float(raw_temp)/1000
                return float("{:.3f}".format(temp))
            except (ValueError, TypeError):
                return 0.0
        elif self.index == 7:
            raw_temp = self.__read_txt_file("/poe_temp")
            if not raw_temp:
                return 0.0
            try:
                temp = float(raw_temp)
                return float("{:.3f}".format(temp))
            except (ValueError, TypeError):
                return 0.0
        elif self.index == 8:
            from swsscommon.swsscommon import DBConnector
            temp = 0
            try:
                stateDB = DBConnector('STATE_DB', 0, True, '')
                val = stateDB.hget('ASIC_TEMPERATURE_INFO', 'maximum_temperature')
                if val is None or int(float(val)) == 0:
                    val = stateDB.hget('ASIC_TEMPERATURE_INFO', 'average_temperature')
                
                if val is not None:
                    temp = float(val)
            except Exception as E:
                print("get_temperature (MAC) failed, cause by {}".format(E))
            return float("{:.3f}".format(temp))
        else:
            if self.get_presence():
                temp = self.sfp_module.get_temperature()
                if temp is None:
                    return 0.0
                return float("{:.3f}".format(temp))

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.index < 3 or self.index == 6:
            return float("{:.3f}".format(80))
        elif self.index < 5:
            temp_file = "temp1_max"
            val = self.__get_temp(temp_file)
            return float(val) if val is not None else 80.0
        elif self.index == 5:
            return float("{:.3f}".format(90))
        elif self.index == 7:
            return float("{:.3f}".format(110))
        elif self.index == 8:
            return float("{:.3f}".format(100))
        else:
            return float("{:.3f}".format(68))

    def get_caution2_threshold(self):
        """
        Retrieves the T-caution2 threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.index < 3 or self.index == 6:
            return float("{:.3f}".format(82))
        elif self.index < 5:
            temp_file = "temp1_max"
            val = self.__get_temp(temp_file)
            return (float(val) + 2) if val is not None else 82.0
        elif self.index == 5:
            return float("{:.3f}".format(92))
        elif self.index == 7:
            return float("{:.3f}".format(115))
        elif self.index == 8:
            return float("{:.3f}".format(105))
        else:
            return float("{:.3f}".format(69))

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal
        :return: A float number, the high critical threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """

        if self.index < 3:
            temp_file = "temp1_max"
            val = self.__get_temp(temp_file)
            return float(val) if val is not None else 100.0
        elif self.index < 5:
            temp_file = "temp1_crit"
            val = self.__get_temp(temp_file)
            return float(val) if val is not None else 110.0
        elif self.index == 5:
            return float("{:.3f}".format(95))
        elif self.index == 6:
            return float("{:.3f}".format(85))
        elif self.index == 7:
            return float("{:.3f}".format(120))
        elif self.index == 8:
            return float("{:.3f}".format(108))
        else:
            return float("{:.3f}".format(70))

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        return self.THERMAL_NAME_LIST[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        if self.index < 5 or self.index == 6:
            temp_file = "temp1_input"
            hwmon_dir = self.__search_hwmon_dir_name(self.SYSFS_THERMAL_DIR[self.index])
            temp_file_path = os.path.join(self.SYSFS_THERMAL_DIR[self.index], hwmon_dir, temp_file)
            return os.path.isfile(temp_file_path)
        elif self.index == 5:
            temp_file = "temp"
            temp_file_path = os.path.join(self.SYSFS_THERMAL_DIR[self.index], temp_file)
            return os.path.isfile(temp_file_path)
        elif self.index > 8:
            return self.sfp_module.get_presence()

        return True

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if self.index > 6 and self.index < 9:
            return True
        if not self.get_presence():
            return False

        return True

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal
        :return: A float number, the low threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        # work temperatur is 0~40, hyst is 2
        return 2.0

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal
        :return: A float number, the low critical threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        # work temperatur is 0~40
        return 0.0

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return "None"

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return "None"

    def is_replaceable(self):
        """
        Retrieves whether thermal module is replaceable
        Returns:
            A boolean value, True if replaceable, False if not
        """
        return False

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of
        entPhysicalContainedIn is'0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device
            or -1 if cannot determine the position
        """
        return self.index + 1

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal
        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        tmp = self.get_temperature()
        if tmp is None:
            return self.minimum_thermal
        if tmp < self.minimum_thermal:
            self.minimum_thermal = tmp
        return self.minimum_thermal

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal
        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        tmp = self.get_temperature()
        if tmp is None:
            return self.maximum_thermal
        if tmp > self.maximum_thermal:
            self.maximum_thermal = tmp
        return self.maximum_thermal
