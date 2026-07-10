#!/usr/bin/env python
#
# Name: fan.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs 
#

try:
    import math
    import os
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Fan(FanBase):

    def __init__(self, index, is_psu_fan=False, psu_index=0):
        self.__index = index
        self.__is_psu_fan = is_psu_fan

        if self.__is_psu_fan:
            self.__psu_index      = psu_index
            self.__presence_attr  = "/sys/class/hwmon/hwmon{}/fan{}_input".format((self.__psu_index + 6), self.__index)
            self.__speed_rpm_attr = "/sys/class/hwmon/hwmon{}/fan{}_input".format((self.__psu_index + 6), self.__index)
            self.__pwm_attr       = None
        else:
            self.__presence_attr  = "/sys/class/hwmon/hwmon2/device/fan{}_input".format(self.__index)
            self.__speed_rpm_attr = "/sys/class/hwmon/hwmon2/device/fan{}_input".format(self.__index)
            self.__pwm_attr       = "/sys/class/hwmon/hwmon2/device/pwm{}".format((self.__index + 1)/2)

    def __get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", attr_path, " file !")

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
        if self.__is_psu_fan:
            return "PSU{}-FAN{}".format(self.__psu_index, self.__index)
        else:
            return "FAN{}".format(self.__index)

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        presence = False
        attr_path = self.__presence_attr

        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            if (int(attr_rv) != 0):
                presence = True

        return presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return "N/A"

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        return "N/A"

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()

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
        raise NotImplementedError

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        speed = 0

        if self.__is_psu_fan:
            attr_path = self.__speed_rpm_attr
        else:
            attr_path = self.__pwm_attr

        if self.get_presence() and None != attr_path:
            attr_rv = self.__get_attr_value(attr_path)
            if (attr_rv != 'ERR'):
                if self.__is_psu_fan:
                    fan_speed_rpm = int(attr_rv)
                    speed = math.ceil(float(fan_speed_rpm) * 100 / 11000)
                else:
                    pwm = int(attr_rv)
                    speed = math.ceil(float(pwm * 100 / 255))

        return speed

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        raise NotImplementedError

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
        raise NotImplementedError

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED

        Args:
            color: A string representing the color with which to set the
                   fan module status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        raise NotImplementedError

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        if self.get_status() and self.get_speed() > 0:
            return self.STATUS_LED_COLOR_GREEN
        else:
            return self.STATUS_LED_COLOR_OFF

