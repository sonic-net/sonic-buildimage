
########################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Thermals' information which are available in the platform
#
########################################################################

try:
    import os
    from sonic_platform_base.device_base import DeviceBase
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class SYSLED(DeviceBase):
    """
    Platform-specific SYSLED class

    SYSLED status:
        0: dark
        1: green
        2: amber/yellow
        3: red
        4: green light flashing
        5: amber/yellow light flashing
        6: red light flashing
        7: blue
        8: blue light flashing
    """

    STATUS_LED_COLOR_OFF = "off"
    STATUS_LED_COLOR_GREEN = "green"
    STATUS_LED_COLOR_AMBER = "amber"
    STATUS_LED_COLOR_RED = "red"
    STATUS_LED_COLOR_GREEN_FLASHING = "green_blink"
    STATUS_LED_COLOR_AMBER_FLASHING = "amber_blink"
    STATUS_LED_COLOR_RED_FLASHING = "red_blink"
    STATUS_LED_COLOR_BLUE = "blue"
    STATUS_LED_COLOR_BLUE_FLASHING = "blue_blink"

    led_dict_id = {
        STATUS_LED_COLOR_OFF: '0',
        STATUS_LED_COLOR_GREEN: '1',
        STATUS_LED_COLOR_AMBER: '2',
        STATUS_LED_COLOR_RED: '3',
        STATUS_LED_COLOR_GREEN_FLASHING: '4',
        STATUS_LED_COLOR_AMBER_FLASHING: '5',
        STATUS_LED_COLOR_RED_FLASHING: '6',
        STATUS_LED_COLOR_BLUE: '7',
        STATUS_LED_COLOR_BLUE_FLASHING: '8'
    }

    led_dict_str = {
       "0" : STATUS_LED_COLOR_OFF,
       "1" : STATUS_LED_COLOR_GREEN,
       "2" : STATUS_LED_COLOR_AMBER,
       "3" : STATUS_LED_COLOR_RED,
       "4" : STATUS_LED_COLOR_GREEN_FLASHING,
       "5" : STATUS_LED_COLOR_AMBER_FLASHING,
       "6" : STATUS_LED_COLOR_RED_FLASHING,
       "7" : STATUS_LED_COLOR_BLUE,
       "8" : STATUS_LED_COLOR_BLUE_FLASHING
    }

    # Device type definition. Note, this is a constant.
    DEVICE_TYPE = "sysled"

    # List of FanBase-derived objects representing all fans
    # available on the PSU
    _sysled_list = None

    DEVICE_NAME = ['sys_led_status', 'bmc_led_status', 'fan_led_status', 'psu_led_status', 'id_led_status']

    def __init__(self):
        self._sysled_path = '/sys/switch/sysled/'
        self.logger = Logger()

    def _is_ascii(self, s):
        try:
            s.encode('ascii')
        except UnicodeEncodeError:
            return False
        else:
            return True

    def _get_attr_val(self, attr_file, default=None):
        """
        Retrieves the sysfs file content(int).
        :param attr_file: sysfs file
        :param default: Default return value if exception occur
        :return: Default return value if exception occur else return value of the callback
        """
        attr_path = self._sysled_path + attr_file
        attr_value = default
        if not os.path.isfile(attr_path):
            return attr_value

        try:
            with open(attr_path, 'r') as fd:
                attr_value = fd.read().strip('\n')
                valtype = type(default)
                if valtype == int:
                    attr_value = int(attr_value)
                elif valtype == str:
                    if not self._is_ascii(attr_value) or attr_value == '':
                        attr_value = 'N/A'
        except Exception as e:
            self.logger.log_error(str(e))

        return attr_value

    def _get_led_status(self, attr_file, default=None):
        value = self._get_attr_val(attr_file, default)

        if value not in self.led_dict_str:
            return 'N/A'

        return self.led_dict_str[value]


    def _set_attr_val(self, attr_file, value):
        """
        Retrieves the sysfs file content(int).
        :param attr_file: sysfs file
        :param default: Default return value if exception occur
        :return: Default return value if exception occur else return value of the callback
        """
        attr_path = self._sysled_path + attr_file
        if not os.path.isfile(attr_path):
            return False

        try:
            with open(attr_path, 'w') as fd:
                fd.write(str(value))
        except Exception as e:
            self.logger.log_error(str(e))
            return False

        return True

    def get_debug(self):
        """
        Retrieves the debug of sensors

        Returns:
            String, the debug of sensors on this chassis
        """
        return self._get_attr_val('debug', 'N/A')

    def get_loglevel(self):
        """
        Retrieves the loglevel of sensors

        Returns:
            Int, the loglevel of sensors on this chassis
        """
        return self._get_attr_val('loglevel', 'N/A')

    def set_loglevel(self, loglevel):
        """
        Set the loglevel of sensors

        Args:
            loglevel: An integer, the loglevel of sensors

        Returns:
            A boolean, True if loglevel is set successfully, False if not
        """
        return self._set_attr_val('loglevel', loglevel)

    def get_sys_led_status(self):
        """
        Retrieves the sysled status of the sysled

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """

        return self._get_led_status('sys_led_status', 'N/A')

    def set_sys_led_status(self, color):
        """
        Set the sysled status rear color

        Args:
            color: A string representing the color with which to set the
                   system LED

        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        if color not in self.led_dict_id:
            return False

        return self._set_attr_val('sys_led_status', self.led_dict_id[color])

    def get_bmc_led_status(self):
        """
        Retrieves the bmc led status rear of the sysled

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        return self._get_led_status('bmc_led_status', 'N/A')

    def get_fan_led_status(self):
        """
        Retrieves the fan led status rear of the sysled

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        return self._get_led_status('fan_led_status', 'N/A')

    def get_psu_led_status(self):
        """
        Retrieves the psu led status rear of the sysled

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        return self._get_led_status('psu_led_status', 'N/A')

    def get_id_led_status(self):
        """
        Retrieves the location led status of the sysled

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        return self._get_led_status('id_led_status', 'N/A')

    def set_id_led_status(self, color):
        """
        Set the location led status front color

        Args:
            color: A string representing the color with which to set the
                   system LED

        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        if color not in self.led_dict_id:
            return False

        return self._set_attr_val('id_led_status', self.led_dict_id[color])

    def set_psu_led_status(self, color):
        """
        Set the psu status rear color

        Args:
            color: A string representing the color with which to set the
                   psu LED

        Returns:
            bool: True if psu LED state is set successfully, False if not
        """
        if color not in self.led_dict_id:
            return False

        return self._set_attr_val('psu_led_status', self.led_dict_id[color])
