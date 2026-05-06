"""
    Dell S3248T

    Module contains an implementation of SONiC Platform Base API and
    provides the Fan-Drawers' information available in the platform.

"""

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_platform.fan import Fan
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")

S3248T_FANS_PER_FANTRAY = 1


class FanDrawer(FanDrawerBase):
    """Dell Platform-specific fan drawer class"""

    FANTRAY_LED_COLOR_MAPPING = {
        "off"   : "off",
        "green" : "green",
        "red"   : "amber"
        }

    def __init__(self, fantray_index):

        FanDrawerBase.__init__(self)
        # FanTray is 1-based in Dell platforms
        self.fantrayindex = fantray_index + 1
        self.fantray_led_reg = "fan{}_led".format(fantray_index)
        for i in range(S3248T_FANS_PER_FANTRAY):
            self._fan_list.append(Fan(fantray_index, i))

    @staticmethod
    def _get_cpld_register(reg_name):
        # On successful read, returns the value read from given
        # reg name and on failure rethrns 'ERR'
        cpld_dir = "/sys/devices/platform/dell-s3248t-cpld.0/"
        cpld_reg_file = cpld_dir + '/' + reg_name
        try:
            with open(cpld_reg_file, 'r') as file_desc:
                ret_val = file_desc.read()
        except IOError:
            return 'ERR'
        return ret_val.strip('\r\n').lstrip(' ')

    @staticmethod
    def _set_cpld_register(reg_name, value):
        # On successful write, returns the value will be written on
        # reg_name and on failure returns 'ERR'
        ret_val = 'ERR'
        cpld_dir = "/sys/devices/platform/dell-s3248t-cpld.0/"
        cpld_reg_file = cpld_dir + '/' + reg_name

        try:
            with open(cpld_reg_file, 'w') as file_desc:
                ret_val = file_desc.write(str(value))
        except IOError:
            ret_val = 'ERR'

        return ret_val

    def get_name(self):
        """
        Retrieves the fan drawer name
        Returns:
            string: The name of the device
        """
        return "FanTray{}".format(self.fantrayindex)

    def get_status_led(self):
        """
        Gets the current Fantray LED color

        Returns:
            A string that represents the supported color
        """

        color = self._get_cpld_register(self.fantray_led_reg)

        return color

    def set_status_led(self, color):
        """
        Set Fantray LED status based on the color type passed in the argument.
        Argument: Color to be set
        Returns:
          bool: True is specified color is set, Otherwise return False
        """

        if (not self._set_cpld_register(self.fantray_led_reg,
                                        self.FANTRAY_LED_COLOR_MAPPING.get(color))):
            return False

        return True

    def get_presence(self):
        """
        Retrives the presence of the fan drawer
        Returns:
            bool: True if fan_tray is present, False if not
        """
        return self.get_fan(0).get_presence()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.fantrayindex

    @staticmethod
    def get_model():
        """
        Retrieves the part number of the fan drawer
        Returns:
            string: Part number of fan drawer
        """
        return "NA"

    @staticmethod
    def get_serial():
        """
        Retrieves the serial number of the fan drawer
        Returns:
            string: Serial number of the fan drawer
        """
        return "NA"

    @staticmethod
    def get_status():
        """
        Retrieves the operational status of the fan drawer
        Returns:
            bool: True if fan drawer is operating properly, False if not
        """
        return True

    @staticmethod
    def is_replaceable():
        """
        Indicate whether this fan drawer is replaceable.
        Returns:
            bool: True if it is replaceable, False if not
        """
        return True

    @staticmethod
    def get_maximum_consumed_power():
        """
        Retrives the maximum power drawn by Fan Drawer
        Returns:
            A float, with value of the maximum consumable power of the
            component.
        """
        return 0.0
