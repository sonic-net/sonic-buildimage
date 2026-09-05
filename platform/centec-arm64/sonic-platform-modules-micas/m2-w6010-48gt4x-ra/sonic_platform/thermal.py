#!/usr/bin/env python

import os
import re
import os.path
import time

try:
    from sonic_platform_base.thermal_base import ThermalBase
    from .redfish_api import Redfish_Api
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    def __init__(self, thermal_index):
        ThermalBase.__init__(self)
        self.index = thermal_index
        self.high_threshold = float(112)
        self.redfish = Redfish_Api()
        self.pinf = {}
        self.begin = time.time()

    def get_power_3s(self):
        self.elapsed = time.time()
        if not self.pinf or self.elapsed - self.begin >= 3:
            self.begin = time.time()
            self.pinf = self.redfish.get_thermal()

    def get_temperature(self):
        self.get_power_3s()
        ctrl = self.pinf["Temperatures"]
        output = ctrl[self.index]
        temp = output.get("ReadingCelsius")
        return temp

    def get_high_threshold(self):
        self.get_power_3s()
        ctrl = self.pinf["Temperatures"]
        output = ctrl[self.index]
        temp = output.get("UpperThresholdFatal")
        return temp

    def get_low_threshold(self):
        self.get_power_3s()
        ctrl = self.pinf["Temperatures"]
        output = ctrl[self.index]
        temp = output.get("LowerThresholdFatal")
        return temp

    def get_high_critical_threshold(self):
        self.get_power_3s()
        ctrl = self.pinf["Temperatures"]
        output = ctrl[self.index]
        temp = output.get("UpperThresholdFatal")
        return temp

    def get_low_critical_threshold(self):
        self.get_power_3s()
        ctrl = self.pinf["Temperatures"]
        output = ctrl[self.index]
        temp = output.get("LowerThresholdFatal")
        return temp

    def get_name(self):
        self.get_power_3s()
        ctrl = self.pinf["Temperatures"]
        output = ctrl[self.index].get("Name")
        name = output.split("/",3)[2]
        if name == "SWITCH_TEMP":
            name = "ASIC_TEMP"
        return "{}".format(name)

    def get_real_name(self):
        self.get_power_3s()
        ctrl = self.pinf["Temperatures"]
        output = ctrl[self.index].get("Name")
        name = output.split("/",3)[2]
        return "{}".format(name)

    def get_presence(self):
        self.get_power_3s()
        ctrl = self.pinf["Temperatures"]
        output = ctrl[self.index]
        if output.get("Status").get("Status").get("State") == "Enabled":
            return True
        return False

    def get_status(self):
        self.get_power_3s()
        ctrl = self.pinf["Temperatures"]
        output = ctrl[self.index]
        if output.get("Status").get("Status").get("Health") == "OK":
            return True
        return False

    def set_sys_led(self, color):
        playload = {}
        led = {}
        led_list = []
        led["IndicatorLEDColor"] = color
        led["LEDType"] = "sys"
        led_list.append(led)
        playload["LEDs"] = led_list
        # boardsLed
        return self.redfish.post_boardLed(playload)

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal

        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return "N/A"

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal

        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return "N/A"

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
