#!/usr/bin/env python
#
# SONiC Platform API implementation for FAN drawer/tray
#
try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

QFX5210_FANS_PER_FANTRAY = 2

# Fan tray to fan number mapping
qfx5210_fans = {1: {'fan1': 1, 'fan2': 11}, 2: {'fan1': 2, 'fan2': 12}, 3: {'fan1': 3, 'fan2': 13}, 4: {'fan1': 4, 'fan2': 14}}

class FanDrawer(FanDrawerBase):

    def __init__(self, fantray_index):

        FanDrawerBase.__init__(self)
        self.fantrayindex = fantray_index
        self._fan_list.append(Fan(fantray_index, qfx5210_fans[fantray_index]['fan1'], False, 0)) #lgtm [py/call/wrong-number-class-arguments]
        self._fan_list.append(Fan(fantray_index, qfx5210_fans[fantray_index]['fan2'], False, 0)) #lgtm [py/call/wrong-number-class-arguments]

    def get_name(self):
        """
        Retrieves the fan drawer name
        Returns:
            string: The name of the device
        """
        return "FanTray{}".format(self.fantrayindex)

    def get_presence(self):
        """
        Retrieves the presence of the fan drawer
        Returns:
            bool: True if fan_tray is present, False if not
        """
        return self.get_fan(0).get_presence()

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

    def get_status(self):
        """
        Retrieves the operational status of the fan drawer
        Returns:
            bool: True if fan drawer is operating properly, False if not
        """
        status = True
        for fan in self.get_all_fans():
            status &= fan.get_status()
        return status

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
        return True

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
        # Return True to avoid thermalctld alarm
        return True
