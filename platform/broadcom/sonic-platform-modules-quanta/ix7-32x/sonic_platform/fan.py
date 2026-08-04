#!/usr/bin/env python

#############################################################################
# Quanta IX7
#
# Module contains an implementation of SONiC Platform Base API and
# provides the FAN information
#
#############################################################################

try:
    import logging
    import os
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


###############
# Global
###############
HWMON_DIR = "/sys/class/hwmon/hwmon1/"
FAN_INDEX_START = 21
NUM_FANTRAYS = 6
FANS_PERTRAY = 2

class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, index, is_psu_fan=False):
        self.is_psu_fan = is_psu_fan
        self.fan_index = index
        self.psu_fan_index_mapping = {
            1:40,
            2:50,
        }
        self.psu_index_mapping = {
            1:42,
            2:52,
        }
        if self.is_psu_fan:
            self.fan_presence_attr = "power{}_present".format(self.psu_index_mapping[index])
            self.fan_pwm_attr = "fan{}_pwm".format(self.psu_fan_index_mapping[index])
            self.fan_rpm_attr = "fan{}_input".format(self.psu_fan_index_mapping[index])
            self.fan_direction_attr = "fan{}_direction".format(self.psu_fan_index_mapping[index])
        else:
            self.fan_presence_attr = "fan{}_present".format(FAN_INDEX_START+(index-1))
            self.fan_pwm_attr = "fan{}_pwm".format(FAN_INDEX_START+(index-1))
            self.fan_rpm_attr = "fan{}_input".format(FAN_INDEX_START+(index-1))
            self.fan_direction_attr = "fan{}_direction".format(FAN_INDEX_START+(index-1))


#######################
# private function
#######################

    def __get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open " + attr_path + " file !")

        retval = retval.rstrip(' \t\n\r')
        return retval


    ####################
    # Device base
    ####################

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        if self.is_psu_fan:
            return "PSU-{}_FAN".format(self.fan_index)
        else:
            fantray_index = (self.fan_index-1)//FANS_PERTRAY+1
            fan_index_intray = self.fan_index - ((fantray_index-1)*FANS_PERTRAY)
            return "Fantray{}_{}".format(fantray_index, fan_index_intray)

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        attr_path = HWMON_DIR + self.fan_presence_attr
        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            if (attr_rv == '1'):
                return True
            else:
                return False

        return None

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        attr_path = HWMON_DIR + self.fan_rpm_attr
        attr_rv = self.__get_attr_value(attr_path)

        if (attr_rv != 'ERR' and attr_rv != '0.0'):
            return True
        else:
            return False

    #################
    # fan base
    #################

    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        attr_path = HWMON_DIR + self.fan_direction_attr
        attr_rv = self.__get_attr_value(attr_path)

        if attr_rv == '2':
            return self.FAN_DIRECTION_INTAKE
        else:
            return self.FAN_DIRECTION_EXHAUST

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        attr_path = HWMON_DIR + self.fan_pwm_attr
        attr_rv = self.__get_attr_value(attr_path)

        if (attr_rv != 'ERR'):
            return int(float(attr_rv))
        else:
            return False

    def get_speed_rpm(self):
        """
        Retrieves the speed of fan in revolutions per minute (RPM)

        Returns:
            An integer, speed of the fan in RPM
        """
        attr_path = HWMON_DIR + self.fan_rpm_attr
        attr_rv = self.__get_attr_value(attr_path)

        if (attr_rv != 'ERR'):
            return int(float(attr_rv))
        else:
            return False

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        attr_path = HWMON_DIR + self.fan_pwm_attr
        attr_rv = self.__get_attr_value(attr_path)

        if (attr_rv != 'ERR'):
            return int(float(attr_rv))
        else:
            return False

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return 25

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED

        Args:
            color: A string representing the color with which to set the
                   fan module status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        return True

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        return None
