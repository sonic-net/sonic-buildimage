#!/usr/bin/env python

#############################################################################
# Quanta IX8A_BDE
#
# Module contains an implementation of SONiC Platform Base API and
# provides the FAN information
#
#############################################################################

try:
    import logging
    import os
    import glob
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


###############
# Global
###############
HWMON_IPMI_DIR = "/sys/devices/platform/quanta_hwmon_ipmi/hwmon/hwmon*/"
FANS_PERTRAY = 2

class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, index, is_psu_fan=False):
        self.is_psu_fan = is_psu_fan
        self.fan_index = index
        hwmon_dir=glob.glob(HWMON_IPMI_DIR)[0]

        if self.is_psu_fan:
            power_out_prefix = self.__get_hwmon_attr_prefix(hwmon_dir, "PSU{}_POWER_OUT".format(self.fan_index), 'power')
            fan_prefix = self.__get_hwmon_attr_prefix(hwmon_dir, "PSU{}_Fan".format(self.fan_index), 'fan')
            self.fan_presence_attr = power_out_prefix + 'present'
            self.fan_pwm_attr = fan_prefix + 'pwm'
            self.fan_rpm_attr = fan_prefix + 'input'
            self.fan_direction_attr = fan_prefix + 'direction'
        else:
            fantray_index = (self.fan_index-1)//FANS_PERTRAY+1
            fan_index_intray = self.fan_index - ((fantray_index-1)*FANS_PERTRAY)
            fan_prefix = self.__get_hwmon_attr_prefix(hwmon_dir, "Fan_SYS_{}_{}".format(fantray_index, fan_index_intray), 'fan')
            self.fan_presence_attr = fan_prefix + 'present'
            self.fan_pwm_attr = fan_prefix + 'pwm'
            self.fan_rpm_attr = fan_prefix + 'input'
            self.fan_direction_attr = fan_prefix + 'direction'


#######################
# private function
#######################

    def __get_hwmon_attr_prefix(self, dir, label, type):

        retval = 'ERR'
        if not os.path.isdir(dir):
            return retval

        try:
            for filename in os.listdir(dir):
                if filename[-5:] == 'label' and type in filename:
                    file_path = os.path.join(dir, filename)
                    if os.path.isfile(file_path) and label == self.__get_attr_value(file_path):
                        return file_path[0:-5]
        except Exception as error:
            logging.error("Error when getting {} label path: {}".format(label, error))

        return retval

    def __get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open {} file: {}".format(attr_path, error))

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
        attr_rv = self.__get_attr_value(self.fan_presence_attr)
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
        if self.get_presence():
            attr_rv = self.__get_attr_value(self.fan_rpm_attr)

            if (attr_rv != 'ERR' and attr_rv != '0.0'):
                return True
            else:
                return False
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
        attr_rv = self.__get_attr_value(self.fan_direction_attr)

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
        if self.get_presence():
            attr_rv = self.__get_attr_value(self.fan_pwm_attr)

            if (attr_rv != 'ERR'):
                return int(float(attr_rv))
            else:
                return False
        else:
            return 0

    def get_speed_rpm(self):
        """
        Retrieves the speed of fan in revolutions per minute (RPM)

        Returns:
            An integer, speed of the fan in RPM
        """
        attr_rv = self.__get_attr_value(self.fan_rpm_attr)

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
        attr_rv = self.__get_attr_value(self.fan_pwm_attr)

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
