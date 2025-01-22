#!/usr/bin/env python
#
# Name: fan.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs
#

try:
    import logging
    import math
    import os
    from sonic_platform_base.fan_base import FanBase
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MAX_SPEED_OF_FAN_FRONT = 23000
MAX_SPEED_OF_FAN_BACK = 20500
MAX_SPEED_OF_FAN_PSU = 18100
MAX_PWM_OF_FAN = 255


class FanConst:
    FAN_PSU_START_INDEX = 4


FAN_SYS_FS = "/sys/devices/virtual/hwmon/hwmon1/"
logger = Logger('sonic-platform-fan')


class Fan(FanBase):

    __name_of_fans = ['FAN1', 'FAN2', 'FAN3', 'FAN4', 'PSU1_FAN1', 'PSU2_FAN1']
    __start_of_psu_fans = FanConst().FAN_PSU_START_INDEX
    __fan_gpi_attr = FAN_SYS_FS + "fan_gpi"

    def __init__(self, index):
        self.__index = index

        if self.__index >= self.__start_of_psu_fans:
            self.__presence_attr = FAN_SYS_FS + \
                "psu{}".format(self.__index - self.__start_of_psu_fans + 1)
            self.__pwm_attr = FAN_SYS_FS + \
                "pwm_psu{}".format(self.__index - self.__start_of_psu_fans + 1)
            self.__rpm1_attr = FAN_SYS_FS + \
                "rpm_psu{}".format(self.__index - self.__start_of_psu_fans + 1)
            self.__rpm2_attr = FAN_SYS_FS + ""
        else:
            self.__rpm1_attr = FAN_SYS_FS + \
                "fan{}_input".format(2*self.__index + 1)
            self.__rpm2_attr = FAN_SYS_FS + \
                "fan{}_input".format(2*self.__index + 2)
            self.__pwm_attr = FAN_SYS_FS + "pwm{}".format(self.__index + 1)

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
        return self.__name_of_fans[self.__index]

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        presence = False

        if self.__index >= self.__start_of_psu_fans:
            # check fan of psu presence if psu presence
            attr_normal = "0 : normal"
            attr_unpowered = "2 : unpowered"
            attr_path = self.__presence_attr
            attr_rv = self.__get_attr_value(attr_path)
            if attr_rv != 'ERR':
                if attr_rv == attr_normal or attr_rv == attr_unpowered:
                    presence = True
            else:
                raise SyntaxError
        else:
            attr_path = self.__fan_gpi_attr
            attr_rv = self.__get_attr_value(attr_path)
            if attr_rv != 'ERR':
                # B[0-3] installed(0)/uninstalled(1)
                if not(int(attr_rv, 16) >> self.__index) & 1:
                    presence = True
            else:
                raise SyntaxError

        return presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        raise NotImplementedError

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        raise NotImplementedError

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        status = False

        if self.__index >= self.__start_of_psu_fans:
            # check fan of psu presence if psu presence
            attr_normal = "0 : normal"
            attr_path = self.__presence_attr
            attr_rv = self.__get_attr_value(attr_path)
            if attr_rv != 'ERR':
                if attr_rv == attr_normal:
                    status = True
            else:
                raise SyntaxError
        else:
            status = self.get_presence()

        return status

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
        direction = 'N/A'
        attr_path = self.__fan_gpi_attr

        if self.__index >= self.__start_of_psu_fans:
            raise NotImplementedError
        else:
            attr_rv = self.__get_attr_value(attr_path)
            if attr_rv != 'ERR':
                # B[4-7] FRtype(0)/RFtype(1)
                if not((int(attr_rv, 16) >> self.__index) & 1):
                    if not((int(attr_rv, 16) >> (self.__index+4)) & 1):
                        direction = 'FAN_DIRECTION_EXHAUST'
                    else:
                        direction = 'FAN_DIRECTION_INTAKE'
            else:
                raise SyntaxError

        return direction

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        speed = 0
        attr_path1 = self.__rpm1_attr
        attr_path2 = self.__rpm2_attr

        if self.get_presence() and attr_path1 is not None:
            attr_rv1 = self.__get_attr_value(attr_path1)
            attr_rv2 = self.__get_attr_value(attr_path2)
            if attr_rv1 != 'ERR' and attr_rv2 != 'ERR':
                fan1_input = int(attr_rv1)
                speed = math.ceil(
                    float(fan1_input * 100 / MAX_SPEED_OF_FAN_FRONT))
                fan2_input = int(attr_rv2)
                speed += math.ceil(float(fan2_input * 100 /
                                         MAX_SPEED_OF_FAN_BACK))
                speed /= 2
            elif attr_rv1 != 'ERR':
                fan1_input = int(attr_rv1)
                if self.__index >= self.__start_of_psu_fans:
                    speed = math.ceil(
                        float(fan1_input * 100 / MAX_SPEED_OF_FAN_PSU))
                else:
                    speed = math.ceil(
                        float(fan1_input * 100 / MAX_SPEED_OF_FAN_FRONT))
            elif attr_rv2 != 'ERR':
                fan2_input = int(attr_rv2)
                speed += math.ceil(float(fan2_input * 100 /
                                         MAX_SPEED_OF_FAN_BACK))
            else:
                raise SyntaxError

        return speed

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        speed = 0
        attr_path = self.__pwm_attr

        if self.get_presence() and attr_path is not None:
            attr_rv = self.__get_attr_value(attr_path)
            if attr_rv != 'ERR':
                pwm = int(attr_rv)
                speed = math.ceil(float(pwm * 100 / MAX_PWM_OF_FAN))
            else:
                raise SyntaxError

        return speed

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
        raise NotImplementedError
