#!/usr/bin/env python

import time
import imp
import os
import sys

try:
    from sonic_platform_base.psu_base import PsuBase
    from .redfish_api import Redfish_Api
except ImportError as e:
    raise ImportError("%s - required module not found" % e)

class Psu(PsuBase):
    """Platform-specific Psu class"""

    def __init__(self, index=0):
        PsuBase.__init__(self)
        self.redfish = Redfish_Api()
        self.pinf = {}
        self.psu_index = index
        self._fan_list = []
        self._thermal_list = []
        self.begin = time.time()

    def get_power_3s(self):
        self.elapsed = time.time()
        if not self.pinf or self.elapsed - self.begin >= 3:
            self.begin = time.time()
            self.pinf = self.redfish.get_power()

    def get_presence(self):
        return True

    def get_fan(self, index):
        """
        Retrieves fan module represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the fan module to
            retrieve

        Returns:
            An object dervied from FanBase representing the specified fan
            module
        """
        return None

    def get_powergood_status(self):
        self.get_power_3s()
        ctrl = self.pinf["PowerSupplies"]
        output = ctrl[self.psu_index]
        if output.get("Status").get("Health") == "OK":
            return True
        else:
            return False

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "PSU {}".format(self.psu_index + 1)

    def get_serial(self):
        self.get_power_3s()
        ctrl = self.pinf["PowerSupplies"]
        output = ctrl[self.psu_index]
        serial = output.get("SerialNumber")
        return serial

    def get_model(self):
        self.get_power_3s()
        ctrl = self.pinf["PowerSupplies"]
        output = ctrl[self.psu_index]
        model = output.get("Model")
        return model

    def get_revision(self):
        self.get_power_3s()
        ctrl = self.pinf["PowerSupplies"]
        output = ctrl[self.psu_index]
        manufacturer = output.get("Manufacturer")
        return manufacturer

    def get_voltage(self):
        self.get_power_3s()
        ctrl = self.pinf["PowerSupplies"]
        output = ctrl[self.psu_index]
        voltage = output.get("Oem").get("OpenBmc").get("OutputVoltage")
        return voltage

    def get_input_current(self):
        self.get_power_3s()
        ctrl = self.pinf["PowerSupplies"]
        output = ctrl[self.psu_index]
        current = output.get("Oem").get("OpenBmc").get("OutputAmperage")
        return current

    def get_input_voltage(self):
        self.get_power_3s()
        ctrl = self.pinf["PowerSupplies"]
        output = ctrl[self.psu_index]
        voltage = output.get("Oem").get("OpenBmc").get("OutputVoltage")
        return voltage

    def get_current(self):
        self.get_power_3s()
        ctrl = self.pinf["PowerSupplies"]
        output = ctrl[self.psu_index]
        current = output.get("Oem").get("OpenBmc").get("OutputAmperage")
        return current

    def get_power(self):
        self.get_power_3s()
        ctrl = self.pinf["PowerSupplies"]
        output = ctrl[self.psu_index]
        current = output.get("Oem").get("OpenBmc").get("OutputAmperage")
        voltage = output.get("Oem").get("OpenBmc").get("OutputVoltage")
        power = float(current)*float(voltage)
        return round(power,2)

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        # no temperature sensor
        value = 35
        return round(float(value), 1)

    def get_status_led(self):
        return "BuildIn"

    def set_status_led(self, color):
        playload = {}
        led = {}
        led_list = []
        led["IndicatorLEDColor"] = color
        led["LEDType"] = "pwr"
        led_list.append(led)
        playload["LEDs"] = led_list
        # boardsLed
        return self.redfish.post_boardLed(playload)

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU

        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        return False

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

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        return self.get_powergood_status()

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU

        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        value = 75
        return round(float(value), 1)

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output

        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        value = 14.52
        return str(round(float(value), 2))

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output

        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        value = 9.72
        return str(round(float(value), 2))

    def get_thermal(self, index):
        """
        Retrieves thermal unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the thermal to
            retrieve

        Returns:
            An object dervied from ThermalBase representing the specified thermal
        """
        return None
