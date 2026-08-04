#!/usr/bin/env python

import time

try:
    from sonic_platform_base.fan_base import FanBase
    from .redfish_api import Redfish_Api
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FAN_NAME_LIST = ["1", "2"]

class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_tray_index, fan_index=0):
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index
        self.redfish = Redfish_Api()
        self.pinf = {}
        self._fan_list = []
        FanBase.__init__(self)
        self.begin = time.time()

    def get_power_3s(self):
        self.elapsed = time.time()
        if not self.pinf or self.elapsed - self.begin >= 3:
            self.begin = time.time()
            self.pinf = self.redfish.get_thermal()

    def get_speed_pwm(self):
        self.get_power_3s()
        ctrl = self.pinf["Fans"]
        output = ctrl[self.fan_tray_index]
        speed = output.get("Oem").get("OpenBmc").get("FanSpeedLevelPercents")
        return int(speed)

    def get_speed_rpm(self):
        self.get_power_3s()
        ctrl = self.pinf["Fans"]
        output = ctrl[self.fan_tray_index]
        speed = output.get("Reading")
        return int(speed)

    def get_high_critical_threshold(self):
        self.get_power_3s()
        ctrl = self.pinf["Fans"]
        output = ctrl[self.fan_tray_index]
        high = output.get("UpperThresholdFatal")
        return int(high)

    def get_low_critical_threshold(self):
        self.get_power_3s()
        ctrl = self.pinf["Fans"]
        output = ctrl[self.fan_tray_index]
        low = output.get("LowerThresholdFatal")
        return int(low)

    def set_speed_pwm(self, speed):
        post_url = '/redfish/v1/Chassis/1/Thermal/Actions/Oem/OpenBmc/Fan.SetSpeed'
        playload = {}
        playload["FanName"] = "Fan0"
        playload["FanSpeedLevelPercents"] = str(speed)
        return self.redfish.post_odata(post_url, playload)

    def get_status_led(self):
        self.get_power_3s()
        ctrl = self.pinf["Fans"]
        output = ctrl[self.fan_tray_index]
        led = output.get("Oem").get("OpenBmc").get("IndicatorLEDColor")
        return led

    def set_status_led(self, color):
        playload = {}
        led = {}
        led_list = []
        led["IndicatorLEDColor"] = color
        led["LEDType"] = "fan"
        led_list.append(led)
        playload["LEDs"] = led_list
        # boardsLed
        return self.redfish.post_boardLed(playload)

    def get_direction(self):
        return "exhaust"

    def get_name(self):
        fan_name = FAN_NAME_LIST[self.fan_index]
        return "Fantray{}_{}".format(self.fan_tray_index+1, fan_name)

    def get_model(self):
        """
        Retrieves the part number of the FAN
        Returns:
            string: Part number of FAN
        """
        return 'N/A'

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

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the FAN
        Returns:
            string: Serial number of FAN
        """
        return 'N/A'

    def get_presence(self):
        self.get_power_3s()
        ctrl = self.pinf["Fans"]
        output = ctrl[self.fan_tray_index]
        state = output.get("Status").get("Status").get("State")
        if state == "Enabled" or state == "UnavailableOffline":
            return True
        return False

    def get_status(self):
        self.get_power_3s()
        ctrl = self.pinf["Fans"]
        output = ctrl[self.fan_tray_index]
        if output.get("Status").get("Status").get("Health") == "OK":
            return True
        return False

    def get_speed(self):
        self.get_power_3s()
        ctrl = self.pinf["Fans"]
        output = ctrl[self.fan_tray_index]
        speed = output.get("Reading")
        speed_percentage = round((speed*100)/17500)
        if speed_percentage > 100:
            speed_percentage = 100
            return speed_percentage
        else:
            return speed_percentage

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
        considered tolerable
        """
        return 30

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        return self.get_speed_pwm()
