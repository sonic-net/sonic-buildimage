#!/usr/bin/env python3

########################################################################
# DellEMC S5448F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fan-Drawers' information available in the platform.
#
########################################################################

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_platform.fan import Fan
    from sonic_platform.ipmihelper import IpmiFru
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

S5448F_FANS_PER_FANTRAY = 2


class FanDrawer(FanDrawerBase):
    """DellEMC Platform-specific Fan class"""

    FAN_FRU_MAPPING = {1: 3, 2: 4, 3: 5, 4: 6, 5: 7, 6: 8}
    def __init__(self, fantray_index):

        FanDrawerBase.__init__(self)
        # FanTray is 1-based in DellEMC platforms
        self.fantrayindex = fantray_index + 1
        for i in range(S5448F_FANS_PER_FANTRAY):
            self._fan_list.append(Fan(fantray_index, i))
        self.fru = IpmiFru(self.FAN_FRU_MAPPING[self.fantrayindex])

    def get_name(self):
        """
        Retrieves the fan drawer name
        Returns:
            string: The name of the device
        """
        return "FanTray{}".format(self.fantrayindex)

    def get_presence(self):
        """
        Retrives the presence of the fan drawer
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
        return self.fru.get_board_part_number()

    def get_serial(self):
        """
        Retrieves the serial number of the fan drawer
        Returns:
            string: Serial number of the fan drawer
        """
        return self.fru.get_board_serial()

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

    def get_status_led(self):
        """
        Gets the state of the fan drawer LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        # Fan tray status LED controlled by BMC
        # Return "off" to avoid thermalctld alarm
        return self.STATUS_LED_COLOR_OFF

    def get_maximum_consumed_power(self):
        """
        Retrives the maximum power drawn by Fan Drawer
        Returns:
            A float, with value of the maximum consumable power of the
            component.
        """
        return 0.0
