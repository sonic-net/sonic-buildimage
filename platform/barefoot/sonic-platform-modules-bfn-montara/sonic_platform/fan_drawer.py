try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_py_common import device_info
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

# FanDrawer -> FanDrawerBase -> DeviceBase
class FanDrawer(FanDrawerBase):
    def __init__(self, fantray_index, max_fan):
        # For now we return only present fans
        self.fantrayindex = fantray_index
        self._fan_list = [Fan(i, max_fan, self.fantrayindex) for i in range(1, max_fan + 1)]

    # DeviceBase interface methods:
    def get_name(self):
        return f"fantray-{self.fantrayindex}"

    def get_presence(self):
        return True

    def get_status(self):
        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.fantrayindex

    def is_replaceable(self):
        """
        Indicate whether this fan drawer is replaceable.
        Returns:
            bool: True if it is replaceable, False if not
        """
        return False

    def get_model(self):
        """
        Retrieves the part number of the fan drawer
        Returns:
            string: Part number of fan drawer
        """
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the fan drawer
        Returns:
            string: Serial number of the fan drawer
        """
        return 'N/A'

    def set_status_led(self, color):
        """
        Set led to expected color
        Args:
            color: A string representing the color with which to set the
            fan module status LED
        Returns:
            bool: True if set success, False if fail.
        """
        # Fan tray status LED controlled by BMC
        return False

    def get_status_led(self):
        """
        Gets the state of the fan drawer LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        return "N/A"

    def get_maximum_consumed_power(self):
        """
        Retrives the maximum power drawn by Fan Drawer
        Returns:
            A float, with value of the maximum consumable power of the
            component.
        """
        return 36.0

def fan_drawer_list_get():
    platform = device_info.get_platform()
    if platform in ["x86_64-accton_as9516_32d-r0", "x86_64-accton_as9516bf_32d-r0"]:
        max_fantray = 1
        max_fan = 6
    elif platform == "x86_64-accton_wedge100bf_65x-r0":
        max_fantray = 2
        max_fan = 5
    else:
        max_fantray = 1
        max_fan = 5

    return [FanDrawer(i, max_fan) for i in range(1, max_fantray + 1)]
