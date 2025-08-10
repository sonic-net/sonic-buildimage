#!/usr/bin/env python
#
# Name: thermal.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs
#

try:
    import os
    import os.path
    from sonic_platform_base.thermal_base import ThermalBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NULL_VAL = "N/A"

class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    SS_CONFIG_PATH = "/etc/sensors.d/sensors.conf"

    def __init__(self, thermal_index,thermal_conf):
        self.__index = thermal_index
        self.__conf = thermal_conf
        
        if self.__conf[self.__index]['container'] == 'psu':
            self.__attr_path_prefix = '/sys/switch/psu/psu{}/temp{}/'.format(self.__conf[self.__index]['container_index']+1,self.__index)
            self.__test_path_prefix = '/sys/switch/psu/psu{}/temp{}/'.format(self.__conf[self.__index]['container_index']+1,self.__index)
        else:
            self.__attr_path_prefix = '/sys/switch/sensor/temp{}/'.format(self.__index)
            self.__test_path_prefix = '/run/platform_cache/thermal_test/sensor/temp{}/'.format(self.__index)
            
        self.__api_helper = APIHelper()

        self.name = self.get_name()

        self.__minimum_thermal = self.get_temperature()
        self.__maximum_thermal = self.get_temperature()
        ThermalBase.__init__(self)

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        temperature = 0.0

       #check whether test mode is set or not
        if (os.path.isdir(self.__test_path_prefix)):
            attr_rv = self.__api_helper.read_one_line_file(self.__test_path_prefix + 'temp_input')
        else:
            attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'temp_input')

        if (attr_rv != None):
            attr_rv = int(attr_rv, 10)
            temperature = float(attr_rv/1000)
        return temperature

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        high_threshold = 0.0
       #check whether test mode is set or not
        if (os.path.isdir(self.__test_path_prefix)):
            attr_rv = self.__api_helper.read_one_line_file(self.__test_path_prefix + 'temp_max_hyst')
        else:
            attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'temp_max_hyst')

        if (attr_rv != None):
            attr_rv = int(attr_rv, 10)
            high_threshold = float(attr_rv/1000)
        return high_threshold

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal
        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return 0.001

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        is_set = False
        is_set = self.__api_helper.write_txt_file(self.__attr_path_prefix + 'temp_max_hyst',int(temperature*1000))
        return is_set

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return False

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal
        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        high_threshold = 0.0
        #check whether test mode is set or not
        if (os.path.isdir(self.__test_path_prefix)):
            attr_rv = self.__api_helper.read_one_line_file(self.__test_path_prefix + 'temp_max')
        else:
            attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'temp_max')

        if (attr_rv != None):
            attr_rv = int(attr_rv, 10)
            high_threshold = float(attr_rv/1000)
        return high_threshold

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal
        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return 0.001

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal
        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        tmp = self.get_temperature()
        if tmp < self.__minimum_thermal:
            self.__minimum_thermal = tmp
        return self.__minimum_thermal

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal
        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        tmp = self.get_temperature()
        if tmp > self.__maximum_thermal:
            self.__maximum_thermal = tmp
        return self.__maximum_thermal

    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        return self.__conf[self.__index]['name']

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        return os.path.isfile(self.__attr_path_prefix + 'temp_max_hyst')

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return NULL_VAL

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return NULL_VAL

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()

    def is_replaceable(self):
        """
        Retrieves whether thermal module is replaceable
        Returns:
            A boolean value, True if replaceable, False if not
        """
        return False

    def get_position_in_parent(self):
        """
        Retrieves the thermal position information
        Returns:
            A int value, 0 represent ASIC thermal, 1 represent CPU thermal info
        """
        if self.__conf[self.__index]['position'] == "cpu":
            return 1
        return 0
