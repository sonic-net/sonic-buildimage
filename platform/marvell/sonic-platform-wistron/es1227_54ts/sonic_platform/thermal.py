#!/usr/bin/env python

#############################################################################
#
# Thermal contains an implementation of SONiC Platform Base API and
# provides the thermal device status which are available in the platform
#
#############################################################################

import os
import os.path
import telnetlib

try:
    from sonic_platform.sfp import Sfp
    from sonic_platform.fan import Fan
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

    def __init__(self, thermal_index):
        self.index = thermal_index
        self.tn = None
        self.is_connected = False
        self.fan = Fan(0, 0)
        self.direction = self.fan.get_direction()
        if thermal_index >= 9:
            self.sfp_module = Sfp(49 + (thermal_index - 9), 'SFP')

        # Add thermal name
        if self.direction == "exhaust":
            self.THERMAL_NAME_LIST.append("XFMR Ambient")
            self.THERMAL_NAME_LIST.append("DDR Ambient")
            self.THERMAL_NAME_LIST.append("System Ambient")
        else:
            self.THERMAL_NAME_LIST.append("System Ambient")
            self.THERMAL_NAME_LIST.append("FAN Ambient")
            self.THERMAL_NAME_LIST.append("CPU Ambient")

        self.THERMAL_NAME_LIST.append("PSU 1 Temp")
        self.THERMAL_NAME_LIST.append("PSU 2 Temp")
        self.THERMAL_NAME_LIST.append("CPU Temp")
        self.THERMAL_NAME_LIST.append("Dimm Temp")
        self.THERMAL_NAME_LIST.append("MAC Temp")
        self.THERMAL_NAME_LIST.append("PHY Temp")

        self.THERMAL_NAME_LIST.append("XCVR 1 Temp")
        self.THERMAL_NAME_LIST.append("XCVR 2 Temp")
        self.THERMAL_NAME_LIST.append("XCVR 3 Temp")
        self.THERMAL_NAME_LIST.append("XCVR 4 Temp")
        self.THERMAL_NAME_LIST.append("XCVR 5 Temp")
        self.THERMAL_NAME_LIST.append("XCVR 6 Temp")
        ThermalBase.__init__(self)
        self.minimum_thermal = 150.0
        self.maximum_thermal = 0.0

    def connect(self):
        try:
            if self.tn is not None:
                return
            print("Connecting...")
            self.tn = telnetlib.Telnet('localhost', '12345', 5)
            print("Connection Establised")
            self.is_connected  = True
        except Exception:
            print("Connection Failed")
            self.is_connected  = False

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
        temp = float(raw_temp)/1000
        return "{:.3f}".format(temp)

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
                return float(self.__get_temp(temp_file))
        elif self.index == 5:
            temp_file = "temp"
            temp_file_path = os.path.join(self.SYSFS_THERMAL_DIR[self.index], temp_file)
            raw_temp = self.__read_txt_file(temp_file_path)
            temp = float(raw_temp)/1000
            return float("{:.3f}".format(temp))
        elif self.index == 7:
            try:
                self.connect()
                if self.is_connected:
                    self.tn.write("do cpss-api call cpssDxChDiagDeviceTemperatureGet devNum 0".encode('ascii') + b"\r\n")
                    _ = self.tn.read_until(b"temperature=", 3)
                    rawtemp = self.tn.read_lazy()
                    temp = float(rawtemp.decode().split("\r")[0])
                else:
                    temp = 0
                return float("{:.3f}".format(temp))
            except:
                return float("{:.3f}".format(0))

        elif self.index == 8:
            try:
                self.connect()
                if self.is_connected:
                    self.tn.write(" ".encode('ascii') + b"\r\n")
                    self.tn.write("dbg xsmi register read device 0 portgroup 0 interface 1 address 0x8 register 0x8042 phydev 0x3".encode('ascii') + b"\r\n")
                    _ = self.tn.read_until(b"Value:", 3)
                    rawtemp = self.tn.read_lazy()
                    temp = int(rawtemp.decode().rstrip().split("\t")[1],16) % 256 - 75
                else:
                    temp = 0
                return float("{:.3f}".format(temp))
            except:
                return float("{:.3f}".format(0))
        else:
            if self.get_presence():
                return float("{:.3f}".format(self.sfp_module.get_temperature()))

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
            return float(self.__get_temp(temp_file))
        elif self.index == 5:
            return float("{:.3f}".format(90))
        elif self.index == 7:
            return float("{:.3f}".format(100))
        elif self.index == 8:
            return float("{:.3f}".format(105))
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
            return float(self.__get_temp(temp_file)) + 2
        elif self.index == 5:
            return float("{:.3f}".format(92))
        elif self.index == 7:
            return float("{:.3f}".format(105))
        elif self.index == 8:
            return float("{:.3f}".format(108))
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
            return float(self.__get_temp(temp_file))
        elif self.index < 5:
            temp_file = "temp1_crit"
            return float(self.__get_temp(temp_file))
        elif self.index == 5:
            return float("{:.3f}".format(95))
        elif self.index == 6:
            return float("{:.3f}".format(85))
        elif self.index == 7:
            return float("{:.3f}".format(108))
        elif self.index == 8:
            return float("{:.3f}".format(110))
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
