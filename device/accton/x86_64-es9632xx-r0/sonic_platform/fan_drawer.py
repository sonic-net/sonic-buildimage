#!/usr/bin/env python

########################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fan-Drawers' information available in the platform.
#
########################################################################

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_platform.fan import Fan
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FANS_PER_FANTRAY = 2
FAN_LED_FILE = "/sys/class/leds/es9632_led::fan/brightness"

class FanDrawer(FanDrawerBase):
    """Platform-specific Fan class"""

    def __init__(self, fantray_index):

        FanDrawerBase.__init__(self)
        # FanTray is 0-based in platforms
        self._api_helper=APIHelper()
        self.fantrayindex = fantray_index
        self.status_led_state = None
        for i in range(FANS_PER_FANTRAY):
            self._fan_list.append(Fan(fantray_index, i))

    def get_name(self):
        """
        Retrieves the fan drawer name
        Returns:
            string: The name of the device
        """
        return "FanTray {}".format(self.fantrayindex)

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_presence(self):
        """
        Indicate presence of the device.
        Returns:
            bool: True if it is presented.
        """
        if FANS_PER_FANTRAY == len(self._fan_list):
            return True
        else:
            return Flase

    def get_model(self):
        """
        Retrieves the fan_draver model
        Returns:
            string: The model of the device

        """
        return "R40W12BGNL9-07T17"

    def get_serial(self):
        """
        Retrieves the fan_draver serial
        Returns:
            string: The serial of the device

        """
        return "N/A"

    def get_status(self):
        """
        Returns Fan Status.
        Returns:
            bool: True if status Ok, False if not
        """
        return True

    def get_maximum_consumed_power(self):
        """
        Returns the maximum power could be consumed by fans in drawer.
        Returns:
            flat: maximum power

        """
        return 46.4 * FANS_PER_FANTRAY

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return (self.fantrayindex + 1)

    def set_status_led(self, color):
        """
        Sets the state of the fan drawer status LED
        Args:
            color: A string representing the color with which to set the
                   fan drawer status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        self.status_led_state = color
        set_status = 0
        if "off" == self.status_led_state:
            set_status = 0
        elif "amber" == self.status_led_state:
            set_status = 3
        elif "red" == self.status_led_state:
            set_status = 3
        elif "green" == self.status_led_state:
            set_status = 1
        else:
            return False
        self._api_helper.write_txt_file(FAN_LED_FILE, set_status)
        return True

