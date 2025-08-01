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
    from sonic_platform import platform
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FANS_PER_FANTRAY = 2

class FanDrawer(FanDrawerBase):
    """Platform-specific Fan class"""

    def __init__(self, fantray_index):

        FanDrawerBase.__init__(self)
        # FanTray is 0-based in platforms
        self._api_helper=APIHelper()
        self.fantrayindex = fantray_index
        self.status_led_state = self.STATUS_LED_COLOR_OFF
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
            return False

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
        Notes:
            The fan drawer LED status is managed in the `fan.py` module, as its
            output is displayed through the `show platform fan` command.
        """
        self.status_led_state = color
        return True

    def get_status_led(self):
        """
        Gets the state of the fan drawer LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        Notes:
            The fan drawer LED status is managed in the `fan.py` module, as its
            output is displayed through the `show platform fan` command.
        """
        return self.status_led_state

