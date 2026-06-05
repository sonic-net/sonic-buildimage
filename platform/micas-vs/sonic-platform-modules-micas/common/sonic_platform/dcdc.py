#!/usr/bin/env python3
#
# Copyright (C) 2024 Micas Networks Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import time
from sonic_platform_base.sensor_base import SensorBase

class Dcdc(SensorBase):

    def __init__(self, interface_obj, index):
        self.dcdc_dict = {}
        self.int_case = interface_obj
        self.index = index
        self.update_time = 0
        self.dcdc_id = "DCDC" + str(index)
        self.minimum = None
        self.maximum = None

    @classmethod
    def get_type(cls):
        return "SENSOR_TYPE"

    def dcdc_dict_update(self):
        local_time = time.time()
        if not self.dcdc_dict or (local_time - self.update_time) >= 1:  # update data every 1 seconds
            self.update_time = local_time
            self.dcdc_dict = self.int_case.get_dcdc_by_id(self.dcdc_id)

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return "N/A"

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        return "N/A"

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return "N/A"

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        self.dcdc_dict_update()
        if self.dcdc_dict["Max"] is not None and self.dcdc_dict["Value"] > self.dcdc_dict["Max"]:
            return False

        if self.dcdc_dict["Min"] is not None and self.dcdc_dict["Value"] < self.dcdc_dict["Min"]:
            return False

        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.index

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False


    def get_name(self):
        """
        Retrieves the name of the sensor

        Returns:
            string: The name of the sensor
        """
        self.dcdc_dict_update()
        return self.dcdc_dict["Name"]

    def get_unit(self):
        """
        Retrieves unit of measurement reported by sensor

        Returns:
            Sensor measurement unit
        """
        return self.int_case.get_dcdc_unit_by_id(self.dcdc_id)

    def get_value(self):
        """
        Retrieves current value reading from sensor
        """
        self.dcdc_dict_update()
        tmp_value = self.dcdc_dict["Value"]
        if tmp_value is None or tmp_value == self.int_case.error_ret:
            return "N/A"
        value = round(float(tmp_value), 3)
        if self.minimum is None or value < self.minimum:
            self.minimum = value

        if self.maximum is None or value > self.maximum:
            self.maximum = value

        return value

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of sensor
        """
        self.dcdc_dict_update()
        value = self.dcdc_dict["High"]
        if value is None or value == self.int_case.error_ret:
            return self.get_high_critical_threshold()
        return round(float(value), 3)

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of sensor
        """
        self.dcdc_dict_update()
        value = self.dcdc_dict["Low"]
        if value is None or value == self.int_case.error_ret:
            return self.get_low_critical_threshold()
        return round(float(value), 3)

    def set_high_threshold(self, value):
        """
        Sets the high threshold value of sensor

        Args:
            value: High threshold value to set

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return False

    def set_low_threshold(self, value):
        """
        Sets the low threshold value of sensor

        Args:
            value: Value

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return False


    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of sensor
        """
        self.dcdc_dict_update()
        value = self.dcdc_dict["Max"]
        if value is None or value == self.int_case.error_ret:
            return "N/A"
        return round(float(value), 3)

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of sensor
        """
        self.dcdc_dict_update()
        value = self.dcdc_dict["Min"]
        if value is None or value == self.int_case.error_ret:
            return "N/A"
        return round(float(value), 3)

    def set_high_critical_threshold(self, value):
        """
        Sets the critical high threshold value of sensor

        Args:
            value: Critical high threshold Value

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return False

    def set_low_critical_threshold(self, value):
        """
        Sets the critical low threshold value of sensor

        Args:
            value: Critial low threshold Value

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return False

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded value of sensor

        Returns:
            The minimum recorded value of sensor
        """
        if self.minimum is None:
            self.get_value()
        if self.minimum is None:
            return "N/A"
        return self.minimum

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded value of sensor

        Returns:
            The maximum recorded value of sensor
        """
        if self.maximum is None:
            self.get_value()
        if self.maximum is None:
            return "N/A"
        return self.maximum

class VoltageSensor(Dcdc):
    def __init__(self, interface_obj, dcdc_index, vol_index):
        super(VoltageSensor, self).__init__(interface_obj, dcdc_index)
        self.index = vol_index

    @classmethod
    def get_type(cls):
        return "SENSOR_TYPE_VOLTAGE"

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.index

class CurrentSensor(Dcdc):
    def __init__(self, interface_obj, dcdc_index, curr_index):
        super(CurrentSensor, self).__init__(interface_obj, dcdc_index)
        self.index = curr_index

    @classmethod
    def get_type(cls):
        return "SENSOR_TYPE_CURRENT"

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.index
