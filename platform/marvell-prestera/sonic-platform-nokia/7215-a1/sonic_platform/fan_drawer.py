#############################################################################
# Nokia
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fan Drawer status which is available in the platform
#
#############################################################################

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

sonic_logger = logger.Logger('fan_drawer')

class NokiaFanDrawer(FanDrawerBase):
    def __init__(self, index):
        super(NokiaFanDrawer, self).__init__()
        self._index = index + 1
        self._led = None

    def get_index(self):
        return self._index

    def get_presence(self):
        return self._fan_list[0].get_presence()

    def get_model(self):
        """
        Retrieves the model number of the Fan Drawer
        Returns:
            string: Part number of Fan Drawer
        """
        return self._fan_list[0].get_model()

    def get_serial(self):
        """
        Retrieves the serial number of the Fan Drawer
        Returns:
            string: Serial number of Fan
        """
        return self._fan_list[0].get_serial()

    def get_status(self):
        """
        Retrieves the operational status of the Fan Drawer
        Returns:
            bool: True if Fan is operating properly, False if not
        """
        return self._fan_list[0].get_status()

    def get_direction(self):
        return 'intake'

    def set_status_led(self, color):
        """
        Sets the state of the fan drawer status LED

        Args:
            color: A string representing the color with which to set the
                   fan drawer status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        return self._fan_list[0].set_status_led(color)

    def get_status_led(self):
        """
        Gets the state of the fan drawer LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings
        """
        return self._fan_list[0].get_status_led()

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return self._index


# For Nokia platforms with fan drawer(s)
class RealDrawer(NokiaFanDrawer):
    def __init__(self, index):
        super(RealDrawer, self).__init__(index)
        self._name = 'drawer{}'.format(self._index)

    def get_name(self):
        return self._name