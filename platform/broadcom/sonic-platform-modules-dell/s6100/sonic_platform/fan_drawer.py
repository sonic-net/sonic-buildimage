#!/usr/bin/env python

########################################################################
# DellEMC S6100
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fan-Drawers' information available in the platform.
#
########################################################################

try:
    import os

    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class FanDrawer(FanDrawerBase):
    """DellEMC Platform-specific Fan Drawer class"""

    HWMON_DIR = "/sys/devices/platform/SMF.512/hwmon/"
    HWMON_NODE = os.listdir(HWMON_DIR)[0]
    MAILBOX_DIR = HWMON_DIR + HWMON_NODE

    def __init__(self, fantray_index):
        FanDrawerBase.__init__(self)
        # FanTray is 1-based in DellEMC platforms
        self.index = fantray_index + 1
        self.presence_reg = "fan{}_fault".format(2 * self.index - 1)
        self.serialno_reg = "fan{}_serialno".format(2 * self.index - 1)

        self._fan_list.append(Fan(self.index, dependency=self))

    def _get_pmc_register(self, reg_name):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'
        mb_reg_file = self.MAILBOX_DIR+'/'+reg_name

        if (not os.path.isfile(mb_reg_file)):
            return rv
        try:
            with open(mb_reg_file, 'r') as fd:
                rv = fd.read()
        except Exception as error:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

    def get_name(self):
        """
        Retrieves the fan drawer name
        Returns:
            string: The name of the device
        """
        return "FanTray{}".format(self.index)

    def get_model(self):
        """
        Retrieves the part number of Fandrawer
        Returns:
            string: Part number of Fandrawer
        """
        # For Serial number "US-01234D-54321-25A-0123-A00", the part
        # number is "01234D"
        fantray_serialno = self._get_pmc_register(self.serialno_reg)
        if (fantray_serialno != 'ERR') and self.get_presence():
            if (len(fantray_serialno.split('-')) > 1):
                fantray_partno = fantray_serialno.split('-')[1]
            else:
                fantray_partno = 'NA'
        else:
            fantray_partno = 'NA'

        return fantray_partno

    def get_serial(self):
        """
        Retrieves the serial number of Fandrawer
        Returns:
            string: Serial number of Fandrawer
        """
        # Sample Serial number format "US-01234D-54321-25A-0123-A00"
        fantray_serialno = self._get_pmc_register(self.serialno_reg)
        if (fantray_serialno == 'ERR') or not self.get_presence():
            fantray_serialno = 'NA'

        return fantray_serialno

    def get_presence(self):
        """
        Retrieves the presence of the Fandrawer
        Returns:
            bool: True if fan is present, False if not
        """
        presence = False
        fantray_presence = self._get_pmc_register(self.presence_reg)
        if (fantray_presence != 'ERR'):
            fantray_presence = int(fantray_presence, 10)
            if (~fantray_presence & 0b1):
                presence = True

        return presence

    def get_status(self):
        """
        Retrieves the operational status of the Fandrawer

        Returns:
            bool: True if Fandrawer is operating properly, False if not
        """
        return self.get_fan(0).get_status()

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
        Indicate whether this Fandrawer is replaceable.
        Returns:
            bool: True if it is replaceable.
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
        # Leds are controlled by Smart-fussion FPGA.
        # Return True to avoid thermalctld alarm.
        return True

    def get_status_led(self):
        """
        Gets the state of the Fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        if self.get_presence():
            if self.get_fan(0).get_status():
                return self.STATUS_LED_COLOR_GREEN
            else:
                return self.STATUS_LED_COLOR_AMBER
        else:
            return self.STATUS_LED_COLOR_OFF

    def get_maximum_consumed_power(self):
        """
        Retrives the maximum power drawn by Fan Drawer

        Returns:
            A float, with value of the maximum consumable power of the
            component.
        """
        return 54.0
