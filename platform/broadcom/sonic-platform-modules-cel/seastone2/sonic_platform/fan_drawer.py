#!/usr/bin/env python

#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class FanDrawer(FanDrawerBase):

    def __init__(self, index, fan_list):
        FanDrawerBase.__init__(self)

        self._fan_list = fan_list
        self._index = index + 1

    def set_status_led(self, color):
        """
        Sets the state of the fan drawer status LED
        Args:
            color: A string representing the color with which to set the
                   fan drawer status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        return self._fan_list[0].drawer_set_status_led(color)

    def get_status_led(self):
        """
        Gets the state of the fan drawer LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        return self._fan_list[0].get_status_led()

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return 'Drawer {}'.format(self._index)

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        return self._fan_list[0].get_presence()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return self._fan_list[0].get_model()

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return self._fan_list[0].get_serial()

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self._fan_list[0].get_status()

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

