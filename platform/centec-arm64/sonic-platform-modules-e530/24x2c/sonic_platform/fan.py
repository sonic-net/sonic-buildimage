#!/usr/bin/env python

#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################

import math
import os.path

try:
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FAN_PATH = "/sys/class/hwmon/hwmon1/"
FAN_MAX_PWM = 255
FAN_FAN_PWM = "pwm{}"
FAN_FAN_INPUT = "fan{}_input"
FAN_MAX_RPM = 9000
FAN_NAME_LIST = ["FAN-1", "FAN-2", "FAN-3", "FAN-4"]

class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_tray_index, fan_index=0):
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index

        FanBase.__init__(self)

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return ""

    def __write_txt_file(self, file_path, value):
        try:
            with open(file_path, 'w') as fd:
                fd.write(str(value))
        except Exception:
            return False
        return True

    def __search_file_by_name(self, directory, file_name):
        for dirpath, dirnames, files in os.walk(directory):
            for name in files:
                file_path = os.path.join(dirpath, name)
                if name in file_name:
                    return file_path
        return None

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        direction = self.FAN_DIRECTION_EXHAUST
        return direction

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)

        Note:
            speed = pwm_in/255*100
        """
        speed = 0
        fan_speed_sysfs_name = "fan{}_input".format(self.fan_index+1)
        fan_speed_sysfs_path = self.__search_file_by_name(
            FAN_PATH, fan_speed_sysfs_name)
        fan_speed_rpm = self.__read_txt_file(fan_speed_sysfs_path) or 0
        speed = math.ceil(float(fan_speed_rpm) * 100 / FAN_MAX_RPM)

        return int(speed)

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)

        Note:
            speed_pc = pwm_target/255*100

            0   : when PWM mode is use
            pwm : when pwm mode is not use
        """
        # target = 0
        # fan_target_sysfs_name = "pwm{}".format(self.fan_index+1)
        # fan_target_sysfs_path = self.__search_file_by_name(
        #     FAN_PATH, fan_target_sysfs_name)
        # fan_target_pwm = self.__read_txt_file(fan_target_sysfs_path) or 0
        # target = math.ceil(float(fan_target_pwm) * 100 / FAN_MAX_PWM)

        # return target
        speed = 0
        fan_speed_sysfs_name = "fan{}_input".format(self.fan_index+1)
        fan_speed_sysfs_path = self.__search_file_by_name(
            FAN_PATH, fan_speed_sysfs_name)
        fan_speed_rpm = self.__read_txt_file(fan_speed_sysfs_path) or 0
        speed = math.ceil(float(fan_speed_rpm) * 100 / FAN_MAX_RPM)

        return speed

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return 10

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

        """
        pwm = speed * 255 / 100
        fan_target_sysfs_name = "pwm{}".format(self.fan_index+1)
        fan_target_sysfs_path = self.__search_file_by_name(
            FAN_PATH, fan_target_sysfs_name)
        return self.__write_txt_file(fan_target_sysfs_path, int(pwm))

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: always True
        """
        return True

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        fan_name = FAN_NAME_LIST[self.fan_index]

        return fan_name

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: always True
        """

        return True

    def get_status(self):
        """
        Retrieves the status of the FAN
        Returns:
            bool: always True
        """
        return True
