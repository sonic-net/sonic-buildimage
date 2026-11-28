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
    import glob
    import time
    from sonic_platform_base.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found") from e


class PsuFan:
    """Hardcoded PSU fan object used by the platform API tests."""

    def __init__(self, psu_index, fan_index):
        self.psu_index = psu_index
        self.fan_index = fan_index
        self.speed = 50
        self.led_color = "off"

    def get_name(self):
        return "PSU{}_FAN{}".format(self.psu_index, self.fan_index)

    def get_presence(self):
        return True

    def get_model(self):
        return "N/A"

    def get_serial(self):
        return "N/A"

    def get_status(self):
        return True

    def get_position_in_parent(self):
        return self.fan_index

    def is_replaceable(self):
        return False

    def get_direction(self):
        return "N/A"

    def get_speed(self):
        return int(self.speed)

    def get_target_speed(self):
        return int(self.speed)

    def get_speed_tolerance(self):
        return 30

    def set_speed(self, speed):
        self.speed = int(speed)
        return True

    def get_status_led(self):
        return self.led_color

    def set_status_led(self, color):
        self.led_color = color
        return True

    def get_vendor(self):
        return "N/A"

    def get_revision(self):
        return "N/A"

    def is_under_speed(self):
        return False

    def is_over_speed(self):
        return False


class Psu(PsuBase):
    """Platform-specific PSU class"""

    MAX_SUPPLIED_POWER = 77.2
    VOLTAGE_HIGH_THRESHOLD = 12.60
    VOLTAGE_LOW_THRESHOLD = 11.40

    def __init__(self, interface_obj, index):
        self.psu_dict = {}
        self.psu_status_dict = {}
        self.psu_power_dict = {}
        self._fan_list = []
        self._thermal_list = []
        self.int_case = interface_obj
        self.index = index
        self.name = "PSU" + str(index)
        self.led_color = "off"

        self.psu_dict_update_time = 0
        self.psu_status_dict_update_time = 0
        self.psu_power_dict_update_time = 0

        self._fan_list.append(PsuFan(index, 1))

    def get_num_fans(self):
        return len(self._fan_list)

    def get_all_fans(self):
        return self._fan_list

    def get_fan(self, index):
        if 0 <= index < len(self._fan_list):
            return self._fan_list[index]
        return None

    def psu_dict_update(self):
        local_time = time.time()
        if not self.psu_dict or (local_time - self.psu_dict_update_time) >= 1:  # update data every 1 seconds
            self.psu_dict_update_time = local_time
            self.psu_dict = self.int_case.get_psu_fru_info(self.name)

    def psu_status_dict_update(self):
        local_time = time.time()
        if not self.psu_status_dict or (
                local_time - self.psu_status_dict_update_time) >= 1:  # update data every 1 seconds
            self.psu_status_dict_update_time = local_time
            self.psu_status_dict = self.int_case.get_psu_status(self.name)

    def psu_power_dict_update(self):
        local_time = time.time()
        if not self.psu_power_dict or (
                local_time - self.psu_power_dict_update_time) >= 1:  # update data every 1 seconds
            self.psu_power_dict_update_time = local_time
            self.psu_power_dict = self.int_case.get_psu_power_status(self.name)

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "PSU{}".format(self.index)

    def get_presence(self):
        """
        Retrieves the presence of the Power Supply Unit (PSU)

        Returns:
            bool: True if PSU is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """
        return "ADO-70W1 12N"

    def get_serial(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: "MSN0000" plus the last six digits of the base MAC address
        """
        try:
            e2 = self.int_case.get_onie_e2_obj("ONIE_E2")
            mac = e2.macbase if e2 is not None else ""
        except Exception:
            mac = ""

        mac_hex = mac.strip().replace(":", "").replace("-", "").replace("0x", "").replace("0X", "").upper()
        if len(mac_hex) < 6:
            return "NA"
        return "MSN0000" + mac_hex[-6:]

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
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

    def _read_psu_output(self, node):
        pattern = "/sys/bus/i2c/devices/10-0041/hwmon/hwmon*/{}".format(node)
        for path in sorted(glob.glob(pattern)):
            try:
                with open(path, "r") as fd:
                    return round(float(fd.read().strip()) / 1000.0, 3)
            except (IOError, OSError, ValueError):
                continue
        return None

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        channel = 1 if self.index == 1 else 2
        value = self._read_psu_output("in{}_input".format(channel))
        if value is None:
            return None
        return round(value, 1)

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, electric current in amperes,
            e.g. 15.4
        """
        channel = 1 if self.index == 1 else 2
        value = self._read_psu_output("curr{}_input".format(channel))
        if value is None:
            return None
        return round(value, 1)

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts,
            e.g. 302.6
        """
        voltage = self.get_voltage()
        current = self.get_current()
        if voltage is None or current is None:
            return None
        return round(float(voltage) * float(current), 2)

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and
            passed all its internal self-tests, False if not.
        """
        return True

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        return self.led_color

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED
        Args:
            color: A string representing the color with which to set the
                   PSU status LED
        Returns:
            bool: True if status LED state is set successfully, False if
                  not
        """
        self.led_color = color
        return True

    def get_status_master_led(self):
        return self.led_color

    def set_status_master_led(self, color):
        self.led_color = color
        return True

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        try:
            value = self.int_case.get_monitor_temp("OUTLET_TEMP")["Value"]
            if value is None or value == self.int_case.error_ret:
                return None
            return round(float(value), 1)
        except (AttributeError, KeyError, TypeError, ValueError):
            return None

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU

        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return 75.0

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output

        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        return self.VOLTAGE_HIGH_THRESHOLD

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output

        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        return self.VOLTAGE_LOW_THRESHOLD

    def get_input_voltage(self):
        """
        Get the input voltage of the PSU

        Returns:
            A float number, the input voltage in volts,
        """
        value = self.get_voltage()
        if value is None:
            return None
        return round(value + 1.5, 1)

    def get_input_current(self):
        """
        Get the input electric current of the PSU

        Returns:
            A float number, the input current in amperes, e.g 220.3
        """
        value = self.get_current()
        if value is None:
            return None
        return round(value + 1.5, 1)

    def get_input_power(self):
        """
        Get the input current energy of the PSU

        Returns:
            A float number, the input power in watts, e.g. 302.6
        """
        value = self.get_power()
        if value is None:
            return None
        return round(value + 1.5 * 1.5, 2)

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return "V1.00"

    # def get_vendor(self):
    #     """
    #     Retrieves the vendor name of the psu

    #     Returns:
    #         string: Vendor name of psu
    #     """
    #     self.psu_dict_update()
    #     return self.psu_dict["VENDOR"]

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU

        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        return self.MAX_SUPPLIED_POWER

    def get_thermal(self, index):
        """
        Retrieves thermal unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the thermal to
            retrieve

        Returns:
            An object dervied from ThermalBase representing the specified thermal
        """
        return False

    def get_psu_power_warning_suppress_threshold(self):
        """
        Retrieve the warning suppress threshold of the power on this PSU
        The value can be volatile, so the caller should call the API each time it is used.
    
        Returns:
            A float number, the warning suppress threshold of the PSU in watts.
        """
        return 60.0
    
    def get_psu_power_critical_threshold(self):
        """
        Retrieve the critical threshold of the power on this PSU
        The value can be volatile, so the caller should call the API each time it is used.
    
        Returns:
            A float number, the critical threshold of the PSU in watts.
        """
        return 65.0
