#
# fan_drawer_base.py
#
# Abstract base class for implementing a platform-specific class with which
# to interact with a fan drawer module in SONiC
#

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class FanDrawer(FanDrawerBase):
    """
    Abstract base class for interfacing with a fan drawer
    """
    # Device type definition. Note, this is a constant.
    DEVICE_TYPE = "fan_drawer"

    def __init__(self, index, fan_list):
        FanDrawerBase.__init__(self)

        self._fan_list = fan_list
        self._index = index

    def get_name(self):
        """
        Retrieves the name of the device
        Returns:
            string: The name of the device
        """
        
        return "fan {}".format(self._index)

    def get_num_fans(self):
        """
        Retrieves the number of fans available on this fan drawer
        Returns:
            An integer, the number of fan modules available on this fan drawer
        """
        return len(self._fan_list)

    def get_all_fans(self):
        """
        Retrieves all fan modules available on this fan drawer
        Returns:
            A list of objects derived from FanBase representing all fan
            modules available on this fan drawer
        """
        return self._fan_list

    def set_status_led(self, color):
        """
        Sets the state of the fan drawer status LED
        Args:
            color: A string representing the color with which to set the
                   fan drawer status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        return self._fan_list[self._index].set_status_led(color)

    def get_status_led(self, color):
        """
        Gets the state of the fan drawer LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        return self._fan_list[self._index].get_status_led(color)

