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

try:
    import time
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found") from e


class Thermal(ThermalBase):

    def __init__(self, interface_obj, index):
        self.temp_dict = {}
        self.temperature_list = []
        self.int_case = interface_obj
        self.index = index
        self.update_time = 0
        self.temp_id = "TEMP" + str(index)

    def temp_dict_update(self):
        local_time = time.time()
        if not self.temp_dict or (local_time - self.update_time) >= 1:  # update data every 1 seconds
            self.update_time = local_time
            self.temp_dict = self.int_case.get_monitor_temp_by_id(self.temp_id)

    def get_name(self):
        """
        Retrieves the name of the thermal

        Returns:
            string: The name of the thermal
        """
        self.temp_dict_update()
        return self.temp_dict["Api_name"]

    def get_presence(self):
        """
        Retrieves the presence of the thermal

        Returns:
            bool: True if thermal is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the Thermal

        Returns:
            string: Model/part number of Thermal
        """
        return "N/A"

    def get_serial(self):
        """
        Retrieves the serial number of the Thermal

        Returns:
            string: Serial number of Thermal
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
        Retrieves the operational status of the thermal

        Returns:
            A boolean value, True if thermal is operating properly,
            False if not
        """
        self.temp_dict_update()
        if self.temp_dict["High"] is not None and self.temp_dict["Value"] > self.temp_dict["High"]:
            return False

        if self.temp_dict["Low"] is not None and self.temp_dict["Value"] < self.temp_dict["Low"]:
            return False

        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        self.temp_dict_update()
        value = self.temp_dict["Value"]
        if value is None or value == self.int_case.error_ret:
            return "N/A"
        # temp value invalid
        temp_invalid = self.temp_dict.get("Invalid")
        if temp_invalid is not None and int(value) == int(temp_invalid):
            return "N/A"
        # temp value error
        temp_error = self.temp_dict.get("Error")
        if temp_error is not None and int(value) == int(temp_error):
            return "N/A"

        if len(self.temperature_list) >= 1000:
            del self.temperature_list[0]
        self.temperature_list.append(float(value))
        return round(float(value), 1)

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        self.temp_dict_update()
        value = self.temp_dict["High"]
        if value is None or value == self.int_case.error_ret:
            return "N/A"
        return round(float(value), 1)

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        self.temp_dict_update()
        value = self.temp_dict["Low"]
        if value is None or value == self.int_case.error_ret:
            return "N/A"
        return round(float(value), 1)

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        # not supported
        return False

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        # not supported
        return False

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        self.temp_dict_update()
        value = self.temp_dict["Max"]
        if value is None or value == self.int_case.error_ret:
            return "N/A"
        return round(float(value), 1)

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal

        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        self.temp_dict_update()
        value = self.temp_dict["Min"]
        if value is None or value == self.int_case.error_ret:
            return "N/A"
        return round(float(value), 1)

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal

        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if len(self.temperature_list) == 0:
            return "N/A"
        return round(float(min(self.temperature_list)), 1)

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal

        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if len(self.temperature_list) == 0:
            return "N/A"
        return round(float(max(self.temperature_list)), 1)
