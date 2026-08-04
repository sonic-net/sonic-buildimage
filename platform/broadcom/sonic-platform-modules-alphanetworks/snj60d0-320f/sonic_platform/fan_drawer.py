#############################################################################
# Alphanetworks
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fan Drawer status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class FanDrawer(FanDrawerBase):
    def __init__(self, index):
        FanDrawerBase.__init__(self)
        self.index = index + 1

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        
        return "drawer{}".format(self.index)

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
        # fan_drawer led not support
        return True

    def get_status_led(self, color):
        """
        Gets the state of the fan drawer LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        # fan_drawer led not support
        return True
