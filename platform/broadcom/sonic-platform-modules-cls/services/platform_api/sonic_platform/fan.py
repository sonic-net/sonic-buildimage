#!/usr/bin/env python

#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.fan_base import FanBase
    from common import Common
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, index, is_psu_fan=False, psu_index=0, conf=None):
        FanBase.__init__(self)

        self._fan_index = index
        self._config = conf
        self._api_common = Common()

        self._is_psu_fan = is_psu_fan
        if self._is_psu_fan:
            self._initialize_psu_fan(psu_index)

        self._name = self.get_name()

    def _initialize_psu_fan(self, psu_index):
        self._psu_index = psu_index
        self._psu_fan_config = self._config['psu_fan'][self._psu_index]

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        return self._api_common.get_output(self._fan_index, self._config['get_direction'], self.FAN_DIRECTION_NOT_APPLICABLE)

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)

        Note:
            speed = pwm_in/255*100
        """
        config = self._config['get_speed'] if not self._is_psu_fan else self._psu_fan_config['get_speed']
        max_rpm = config['max_rear'] if 'R' in self._name else config['max_front']
        raw_speed = self._api_common.get_output(
            self._fan_index, config, 0)

        return int(float(raw_speed) / max_rpm * 100.0)

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)

        Note:
            0   : when PWM mode is not in use
            pwm : when pwm mode is not use
        """
        return self._api_common.get_output(self._fan_index, self._config['get_target_speed'], Common.NULL_VAL)

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return self._api_common.get_output(self._fan_index, self._config['get_speed_tolerance'], Common.NULL_VAL)

    def set_speed(self, speed):
        """
        Sets the fan speed
        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)
        Returns:
            A boolean, True if speed is set successfully, False if not

        Note:
            Depends on pwm or target mode is selected:
            1) pwm = speed_pc * 255             <-- Currently use this mode.
            2) target_pwm = speed_pc * 100 / 255
             2.1) set pwm{}_enable to 3

        ipmitool raw 0x3a 0x0e 0x00 > enable auto fcs
        ipmitool raw 0x3a 0x0e 0x01 > disable auto fcs
        """
        if speed not in range(1, 101) or self._is_psu_fan:
            return False

        return self._api_common.set_output(self._fan_index, speed, self._config['set_speed'])

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if status LED state is set successfully, False if not


        Note: Required Manual Control LED mode ('ipmitool raw 0x3a 0x0f 0x2 0x0')
        """
        config = self._config['set_status_led']
        avaliable_input = config.get('avaliable_input')
        if (avaliable_input and color not in avaliable_input) or self._is_psu_fan:
            return False

        return self._api_common.set_output(self._fan_index, color, config)

    def get_status_led(self):
        """
        Gets the state of the fan status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above

        Note:
            STATUS_LED_COLOR_GREEN = "green"
            STATUS_LED_COLOR_AMBER = "amber"
            STATUS_LED_COLOR_RED = "red"
            STATUS_LED_COLOR_OFF = "off"
        """

        default = self.STATUS_LED_COLOR_OFF

        if self._is_psu_fan:
            return default

        return self._api_common.get_output(self._fan_index, self._config['get_status_led'], default)

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        config = self._config['get_name'] if not self._is_psu_fan else self._psu_fan_config['get_name']
        return self._api_common.get_output(self._fan_index, config, Common.NULL_VAL)

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        return self._api_common.get_output(self._fan_index, self._config['get_presence'], False)

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        default = Common.NULL_VAL
        if self._is_psu_fan:
            return default
        return self._api_common.get_output(self._fan_index, self._config['get_model'], default)

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        default = Common.NULL_VAL
        if self._is_psu_fan:
            return default
        return self._api_common.get_output(self._fan_index, self._config['get_serial'], default)

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence() and self.get_speed() > 10
