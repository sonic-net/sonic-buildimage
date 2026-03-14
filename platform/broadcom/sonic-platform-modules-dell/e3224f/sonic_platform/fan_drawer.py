#!/usr/bin/env python

########################################################################
# DellEMC E3224F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fan-Drawers' information available in the platform.
#
########################################################################

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

E3224F_FANS_PER_FANTRAY = 1


class FanDrawer(FanDrawerBase):
    """DellEMC Platform-specific Fan class"""

    FANTRAY_LED_COLORS = {
        "off",
        "green",
        "yellow"
        }

    def __init__(self, fantray_index):

        FanDrawerBase.__init__(self)
        # FanTray is 1-based in DellEMC platforms
        self.fantray_led_reg = "fan{}_led".format(fantray_index)
        self.fantrayindex = fantray_index + 1
        for i in range(E3224F_FANS_PER_FANTRAY):
            self._fan_list.append(Fan(fantray_index, i))

    def _get_cpld_register(self, reg_name):
        # On successful read, returns the value read from given
        # reg name and on failure rethrns 'ERR'
        cpld_dir = "/sys/devices/platform/dell-e3224f-cpld.0/"
        cpld_reg_file = cpld_dir + '/' + reg_name
        try:
            rv = open(cpld_reg_file, 'r').read()
        except IOError : return 'ERR'
        return rv.strip('\r\n').lstrip(' ')

    def _set_cpld_register(self, reg_name, value):
        # On successful write, returns the value will be written on
        # reg_name and on failure returns 'ERR'
        cpld_dir = "/sys/devices/platform/dell-e3224f-cpld.0/"
        cpld_reg_file = cpld_dir + '/' + reg_name

        try:
           with open(cpld_reg_file, 'w') as fd:
                rv = fd.write(str(value))
        except Exception:
            rv = 'ERR'

        return rv

    def get_name(self):
        """
        Retrieves the fan drawer name
        Returns:
            string: The name of the device
        """
        return "FanTray{}".format(self.fantrayindex)

    def get_status_led(self):
        """
        Gets the current system LED color

        Returns:
            A string that represents the supported color
        """

        color = self._get_cpld_register(self.fantray_led_reg)

        #if color not in list(self.FANTRAY_LED_COLORS):
        #    return self.sys_ledcolor

        return color

    def set_status_led(self,color):
        """
        Set system LED status based on the color type passed in the argument.
        Argument: Color to be set
        Returns:
          bool: True is specified color is set, Otherwise return False
        """

        if color not in list(self.FANTRAY_LED_COLORS):
            return False

        if(not self._set_cpld_register(self.fantray_led_reg, color)):
            return False

        return True

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
        return "NA"

    def get_serial(self):
        """
        Retrieves the serial number of the fan drawer
        Returns:
            string: Serial number of the fan drawer
        """
        return "NA"

    def get_status(self):
        """
        Retrieves the operational status of the fan drawer
        Returns:
            bool: True if fan drawer is operating properly, False if not
        """
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
        return True

    def get_maximum_consumed_power(self):
        """
        Retrives the maximum power drawn by Fan Drawer

        Returns:
            A float, with value of the maximum consumable power of the
            component.
        """
        return 0.0
