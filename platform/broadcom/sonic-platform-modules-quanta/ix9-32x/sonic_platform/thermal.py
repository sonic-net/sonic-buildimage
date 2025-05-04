#!/usr/bin/env python

#############################################################################
# Quanta IX9
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Thermal information
#
#############################################################################

import logging
import os
import glob

try:
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

HWMON_IPMI_DIR = "/sys/devices/platform/quanta_hwmon_ipmi/hwmon/hwmon*/"

thermal_index_mapping = {
    1:'PSU1_TEMP1',
    2:'PSU1_TEMP2',
    3:'PSU1_TEMP3',
    4:'PSU2_TEMP1',
    5:'PSU2_TEMP2',
    6:'PSU2_TEMP3',
    7:'Temp_1V05_PCH_VR',
    8:'Temp_Ambient_0',
    9:'Temp_Ambient_1',
   10:'Temp_Ambient_2',
   11:'Temp_Ambient_3',
   12:'Temp_Ambient_4',
   13:'Temp_Ambient_5',
   14:'Temp_CPU',
   15:'Temp_DDRAB_VR',
   16:'Temp_Inlet',
   17:'Temp_SOC_DIMMA0',
   18:'Temp_VCCGBE_VR',
   19:'Temp_VCCIN_VR'
}



class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    def __init__(self, thermal_index):
        self.index             = thermal_index
        hwmon_dir=glob.glob(HWMON_IPMI_DIR)[0]
        thermal_prefix         = self.__get_hwmon_attr_prefix(hwmon_dir, thermal_index_mapping[self.index], 'temp')
        self.temp_attr         = "{}input".format(thermal_prefix)
        self.high_th_attr      = "{}ncrit".format(thermal_prefix)
        self.high_crit_th_attr = "{}crit".format(thermal_prefix)
        self.low_th_attr       = "{}lncrit".format(thermal_prefix)
        self.low_crit_th_attr  = "{}lcrit".format(thermal_prefix)
        self.name_attr         = "{}label".format(thermal_prefix)

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

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        attr_rv = self.__get_attr_value(self.name_attr)

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
        attr_rv = self.__get_attr_value(self.name_attr)

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
        attr_rv = self.__get_attr_value(self.temp_attr)

        if (attr_rv != 'ERR'):
            return float(attr_rv) / 1000
        else:
            return None

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        attr_rv = self.__get_attr_value(self.low_th_attr)

        if (attr_rv != 'ERR'):
            return float(attr_rv) / 1000
        else:
            return None

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal

        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        attr_rv = self.__get_attr_value(self.low_crit_th_attr)

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
        attr_rv = self.__get_attr_value(self.high_th_attr)

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
        attr_rv = self.__get_attr_value(self.high_crit_th_attr)

        if (attr_rv != 'ERR'):
            return float(attr_rv) / 1000
        else:
            return None

