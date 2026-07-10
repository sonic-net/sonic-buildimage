#!/usr/bin/env python
#
# Name: fan.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs
#

try:
    import logging
    import os
    from sonic_platform_base.fan_base import FanBase
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MAX_FAN_RPM = 22000
MAX_PSU_FAN_RPM = 18000

MAX_PWM_OF_FAN = 255


FAN_SYS_FS = "/sys/bus/i2c/devices/16-002f/fan{}_input"
FAN_ALARM_SYS_FS = "/sys/bus/i2c/devices/16-002f/fan{}_alarm"

PSU_FAN_SYS_FS = "/sys/class/hwmon/hwmon{}/fan1_input"
PSU_FAN_ALARM_SYS_FS = "/sys/class/hwmon/hwmon{}/fan1_alarm"


logger = Logger('sonic-platform-fan')


class Fan(FanBase):

    def __init__(self, index, psu_fan=False):
        self.__psu_fan = psu_fan

        if psu_fan:
            self.__index = index
            self.__rpm = PSU_FAN_SYS_FS.format(9 + self.__index)
            self.__alarm = PSU_FAN_ALARM_SYS_FS.format(9 + self.__index)
        else:
            self.__index = 1 + 2*index 
            self.__rpm = FAN_SYS_FS.format(self.__index)
            self.__alarm = FAN_ALARM_SYS_FS.format(self.__index)

    def __get_attr_value(self, attr_path):

        retval = 'ERR'
        if not os.path.isfile(attr_path):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open file: %s", attr_path)

        retval = retval.rstrip(' \t\n\r')
        return retval


##############################################
# Device methods
##############################################

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "FAN{}".format(self.__index) if not self.__psu_fan else "PSU{}_FAN".format(self.__index+1)

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        return self.get_status()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        return 'N/A'

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        status = False

        rv = self.__get_attr_value(self.__alarm)
        if rv != 'ERR':
            if rv == '0':
                status = True
        else:
            raise SyntaxError

        return status

    def is_replaceable(self):
        """
        Indicate whether Fan is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_position_in_parent(self):
        return self.__index


##############################################
# FAN methods
##############################################

    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        return 'N/A'

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        speed = 44
        rv = self.__get_attr_value(self.__rpm)

        if rv != 'ERR':
            speed = int(
                rv) * 100 // (MAX_PSU_FAN_RPM if self.__psu_fan else MAX_FAN_RPM)

        return speed

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        return self.get_speed()

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        raise NotImplementedError

    def set_speed(self, speed):
        """
        Sets the fan speed

        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)

        Returns:
            A boolean, True if speed is set successfully, False if not
        """
        return False

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED

        Args:
            color: A string representing the color with which to set the
                   fan module status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        return False

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        return None
