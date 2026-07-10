#!/usr/bin/env python

#############################################################################
# Quanta IX8
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Thermal information
#
#############################################################################

import logging
import os

try:
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

HWMON_DIR = "/sys/class/hwmon/hwmon1/"

thermal_index_mapping = {
    1:44,
    2:45,
    3:46,
    4:55,
    5:56,
    6:57,
    7:70,
    8:71,
    9:72,
   10:73,
   11:74,
   12:75,
   13:76,
   14:77,
   15:78,
}

class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    def __init__(self, thermal_index):
        self.index        = thermal_index
        self.temp_attr    = "temp{}_input".format(thermal_index_mapping[self.index])
        self.high_th_attr = "temp{}_ncrit".format(thermal_index_mapping[self.index])
        self.high_crit_th_attr = "temp{}_crit".format(thermal_index_mapping[self.index])
        self.name_attr    = "temp{}_label".format(thermal_index_mapping[self.index])


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

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        attr_path = HWMON_DIR + self.name_attr
        attr_rv = self.__get_attr_value(attr_path)

        if (attr_rv != 'ERR'):
            return attr_rv
        else:
            return None

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        attr_path = HWMON_DIR + self.name_attr
        attr_rv = self.__get_attr_value(attr_path)

        if (attr_rv != 'ERR'):
            return True
        else:
            return False

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if (self.get_temperature() != None):
            return True
        else:
            return False

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        attr_path = HWMON_DIR + self.temp_attr
        attr_rv = self.__get_attr_value(attr_path)

        if (attr_rv != 'ERR'):
            return float(attr_rv) / 1000
        else:
            return None

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        attr_path = HWMON_DIR + self.high_th_attr
        attr_rv = self.__get_attr_value(attr_path)

        if (attr_rv != 'ERR'):
            return float(attr_rv) / 1000
        else:
            return None

    def get_high_critical_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        attr_path = HWMON_DIR + self.high_crit_th_attr
        attr_rv = self.__get_attr_value(attr_path)

        if (attr_rv != 'ERR'):
            return float(attr_rv) / 1000
        else:
            return None
