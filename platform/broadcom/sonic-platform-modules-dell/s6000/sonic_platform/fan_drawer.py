#!/usr/bin/env python

########################################################################
# DellEMC S6000
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fan-Drawers' information available in the platform.
#
########################################################################

try:
    import os

    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MAX_S6000_FANS_PER_FANTRAY = 2


class FanDrawer(FanDrawerBase):
    """DellEMC Platform-specific Fan Drawer class"""

    CPLD_DIR = "/sys/devices/platform/dell-s6000-cpld.0/"

    def __init__(self, fantray_index):
        FanDrawerBase.__init__(self)
        # FanTray is 1-based in DellEMC platforms
        self.index = fantray_index + 1
        self.eeprom = Eeprom(is_fantray=True, fantray_index=self.index)
        self.fantray_presence_reg = "fan_prs"
        self.fantray_led_reg = "fan{}_led".format(self.index - 1)
        self.supported_led_color = ['off', 'green', 'amber']

        for i in range(1, MAX_S6000_FANS_PER_FANTRAY+1):
            self._fan_list.append(Fan(fantray_index=self.index, fan_index=i, dependency=self))

    def _get_cpld_register(self, reg_name):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'
        cpld_reg_file = self.CPLD_DIR + reg_name

        if (not os.path.isfile(cpld_reg_file)):
            return rv

        try:
            with open(cpld_reg_file, 'r') as fd:
                rv = fd.read()
        except IOError:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

    def _set_cpld_register(self, reg_name, value):
        # On successful write, returns the value will be written on
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'
        cpld_reg_file = self.CPLD_DIR + reg_name

        if (not os.path.isfile(cpld_reg_file)):
            return rv

        try:
            with open(cpld_reg_file, 'w') as fd:
                rv = fd.write(str(value))
        except IOError:
            rv = 'ERR'

        return rv

    def get_name(self):
        """
        Retrieves the Fandrawer name
        Returns:
            string: The name of the device
        """
        return "FanTray{}".format(self.index)

    def get_presence(self):
        """
        Retrieves the presence of the Fandrawer

        Returns:
            bool: True if Fandrawer is present, False if not
        """
        presence = False

        fantray_presence = self._get_cpld_register(self.fantray_presence_reg)
        if (fantray_presence != 'ERR'):
            fantray_presence = int(fantray_presence, 16) & (1 << (self.index - 1))
            if fantray_presence:
                presence = True

        return presence

    def get_model(self):
        """
        Retrieves the part number of the Fandrawer

        Returns:
            string: Part number of Fandrawer
        """
        return self.eeprom.get_part_number()

    def get_serial(self):
        """
        Retrieves the serial number of the Fandrawer

        Returns:
            string: Serial number of Fandrawer
        """
        # Sample Serial number format "US-01234D-54321-25A-0123-A00"
        return self.eeprom.get_serial_number()

    def get_status(self):
        """
        Retrieves the operational status of the Fandrawer

        Returns:
            bool: True if Fandrawer is operating properly, False if not
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
        return self.index

    def is_replaceable(self):
        """
        Indicate whether this fan drawer is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def set_status_led(self, color):
        """
        Set led to expected color
        Args:
            color: A string representing the color with which to set the
                   fandrawer status LED
        Returns:
            bool: True if set success, False if fail.
        """
        if color not in self.supported_led_color:
            return False
        if color == self.STATUS_LED_COLOR_AMBER:
            color = 'yellow'

        rv = self._set_cpld_register(self.fantray_led_reg, color)
        if (rv != 'ERR'):
            return True
        else:
            return False

    def get_status_led(self):
        """
        Gets the state of the fandrawer status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        fantray_led = self._get_cpld_register(self.fantray_led_reg)
        if (fantray_led != 'ERR'):
            if (fantray_led == 'yellow'):
                return self.STATUS_LED_COLOR_AMBER
            else:
                return fantray_led
        else:
            return self.STATUS_LED_COLOR_OFF

    def get_maximum_consumed_power(self):
        """
        Retrives the maximum power drawn by Fan Drawer

        Returns:
            A float, with value of the maximum consumable power of the
            component.
        """
        return 18.0
