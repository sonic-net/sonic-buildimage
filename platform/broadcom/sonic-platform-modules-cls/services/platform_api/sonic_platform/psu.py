#!/usr/bin/env python

#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the psu status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.psu_base import PsuBase
    from common import Common
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Psu(PsuBase):
    """Platform-specific Fan class"""

    def __init__(self, index, conf=None, fan_conf=None):
        PsuBase.__init__(self)

        self._psu_index = index
        self._config = conf
        self._api_common = Common(self._config)
        self._fan_conf = fan_conf
        self._initialize_psu_fan()

    def _initialize_psu_fan(self):
        from sonic_platform.fan import Fan

        num_fan = self._fan_conf['psu_fan'][self._psu_index]["num_of_fan"]
        for fan_index in range(0, num_fan):
            fan = Fan(fan_index, is_psu_fan=True,
                      psu_index=self._psu_index, conf=self._fan_conf)
            self._fan_list.append(fan)

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts, 
            e.g. 12.1 
        """
        return self._api_common.get_output(self._psu_index, self._config['get_voltage'], 0.0)

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        return self._api_common.get_output(self._psu_index, self._config['get_current'], 0.0)

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        return self._api_common.get_output(self._psu_index, self._config['get_current'], 0.0)

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        return self._api_common.get_output(self._psu_index, self._config['get_powergood_status'], False)

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED
        Note:
            Seastone2 CPLD able to set only AMBER color.
            This function should be disable auto mode before execute 
            command: ipmitool raw 0x3a 0x0f 0x02 0x00
        Args:
            color: A string representing the color with which to set the
                   PSU status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        return self._api_common.set_output(self._psu_index, color, self._config['set_status_led'])

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Note:
            Seastone2 PSU LED got only 2 mode, AMBER and Hardware control mode.
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        return self._api_common.get_output(self._psu_index, self._config['get_status_led'], Common.NULL_VAL)

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125 
        """
        return self._api_common.get_output(self._psu_index, self._config['get_temperature'], 0.0)

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU
        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self._api_common.get_output(self._psu_index, self._config['get_temperature_high_threshold'], 0.0)

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output

        Returns:
            ucr as float number and return 0 if the BMC output is na.
            A float number, the high threshold output voltage in volts, 
            e.g. 12.1 
        """
        return self._api_common.get_output(self._psu_index, self._config['get_voltage_high_threshold'], 0.0)

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output

        Returns:
            lcr as float number and return 0 if the BMC output is na.
            A float number, the low threshold output voltage in volts, 
            e.g. 12.1 
        """
        return self._api_common.get_output(self._psu_index, self._config['get_voltage_low_threshold'], 0.0)

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self._api_common.get_output(self._psu_index, self._config['get_name'], Common.NULL_VAL)

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        return self._api_common.get_output(self._psu_index, self._config['get_name'], False)

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return self._api_common.get_output(self._psu_index, self._config['get_model'], Common.NULL_VAL)

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return self._api_common.get_output(self._psu_index, self._config['get_serial'], Common.NULL_VAL)

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_powergood_status()
