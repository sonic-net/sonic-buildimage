#!/usr/bin/env python
import time

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_platform.fan import Fan
    from .redfish_api import Redfish_Api
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class FanDrawer(FanDrawerBase):

    FANS_PER_FANTRAY = 1

    def __init__(self, fantray_index=0):
        FanDrawerBase.__init__(self)
        self.fantrayindex = fantray_index
        self.redfish = Redfish_Api()
        self.pinf = {}
        self.begin = time.time()
        for i in range(self.FANS_PER_FANTRAY):
            self._fan_list.append(Fan(fantray_index, i))

    def get_power_3s(self):
        self.elapsed = time.time()
        if not self.pinf or self.elapsed - self.begin >= 3:
            self.begin = time.time()
            self.pinf = self.redfish.get_thermal()

    def get_name(self):
        return "FanTray{}".format(self.fantrayindex+1)

    def get_presence(self):
        self.get_power_3s()
        ctrl = self.pinf["Fans"]
        output = ctrl[self.fantrayindex]
        state = output.get("Status").get("Status").get("State")
        if state == "Enabled" or state == "UnavailableOffline":
            return True
        return False

    def get_model(self):
        """
        Retrieves the part number of the FAN
        Returns:
            string: Part number of FAN
        """
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the FAN
        Returns:
            string: Serial number of FAN
        """
        return 'N/A'

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return 'N/A'

    def get_status(self):
        """
        Retrieves the operational status of the FAN
        Returns:
            bool: True if FAN is operating properly, False if not
        """
        self.get_power_3s()
        ctrl = self.pinf["Fans"]
        output = ctrl[self.fantrayindex]
        if output.get("Status").get("Status").get("Health") == "OK":
            return True
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

    def get_status_led(self):
        self.get_power_3s()
        ctrl = self.pinf["Fans"]
        output = ctrl[self.fantrayindex]
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

    def get_maximum_consumed_power(self):
        """
        Retrives the maximum power drawn by Fan Drawer

        Returns:
            A float, with value of the maximum consumable power of the
            component.
        """
        return 'N/A'
