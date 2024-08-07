#!/usr/bin/env python

#############################################################################
#
# Thermal contains an implementation of SONiC Platform Base API and
# provides the thermal device status which are available in the platform
#
#############################################################################

import os
import os.path
import subprocess

try:
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    THERMAL_NAME_LIST = []
    SYSFS_THERMAL_DIR = ["/sys/bus/i2c/devices/0-004f/",
                         "/sys/bus/i2c/devices/0-0049/",
                         "/sys/bus/i2c/devices/0-004a/",
                         "/sys/bus/i2c/devices/0-004b/",
                         "/sys/bus/i2c/devices/0-004c/",
                         "/sys/bus/i2c/devices/0-004d/",
                         "/sys/bus/i2c/devices/0-004e/"]

    IPMI_SENSOR_NR  = ["0x30", "0x31", "0x32", "0x33", "0x34", "0x35", "0x36"]

    def __init__(self, thermal_index):
        self.index = thermal_index
        self.lnc = None
        self.lcr = None
        self.unc = None
        self.ucr = None

        # Add thermal name
        self.THERMAL_NAME_LIST.append("Top-Rear")
        self.THERMAL_NAME_LIST.append("Top-Front")
        self.THERMAL_NAME_LIST.append("Right-Front")
        self.THERMAL_NAME_LIST.append("Top-Center")
        self.THERMAL_NAME_LIST.append("Left-Front")
        self.THERMAL_NAME_LIST.append("Bottom-Front")
        self.THERMAL_NAME_LIST.append("Bottom-Rear")
        ThermalBase.__init__(self)
        self.minimum_thermal = self.get_temperature()
        self.maximum_thermal = self.get_temperature()
        self.__initialize_threshold()

    def __initialize_threshold(self):
        cmd = ["ipmitool", "raw", "0x4", "0x27"]
        if self.lnc is None:
            cmd.append(self.IPMI_SENSOR_NR[self.index])
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
            out, err = p.communicate()
            self.unc = float(int(out.split()[4],16))
            self.ucr = float(int(out.split()[5],16))
            self.lnc = float(int(out.split()[1],16) if int(out.split()[1],16) != 0 else 2)
            self.lcr = float(int(out.split()[2],16))

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return ""

    def __get_temp(self, temp_file):
        temp_file_path = os.path.join(self.SYSFS_THERMAL_DIR[self.index], temp_file)
        raw_temp = self.__read_txt_file(temp_file_path)
        temp = float(raw_temp)/1000
        return "{:.3f}".format(temp)

    def __set_threshold(self, file_name, temperature):
        temp_file_path = os.path.join(self.SYSFS_THERMAL_DIR[self.index], file_name)
        try:
            with open(temp_file_path, 'w') as fd:
                fd.write(str(temperature))
            return True
        except IOError:
            return False

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        temp_file = "temp1_input"
        return float(self.__get_temp(temp_file))

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal
        :return: A float number, the low threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """

        return self.lnc

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal
        :return: A float number, the low critical threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """

        return self.lcr

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """

        return self.unc

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal
        :return: A float number, the high critical threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """

        return self.ucr

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        return self.THERMAL_NAME_LIST[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the sensor
        Returns:
            bool: True if sensor is present, False if not
        """
        temp_file = "temp1_input"
        temp_file_path = os.path.join(self.SYSFS_THERMAL_DIR[self.index], temp_file)
        return os.path.isfile(temp_file_path)

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if not self.get_presence():
            return False

        return True

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
        if tmp > self.maximum_thermal:
            self.maximum_thermal = tmp
        return self.maximum_thermal

