#############################################################################
# Celestica
#
# Thermal contains an implementation of SONiC Platform Base API and
# provides the thermal device status which are available in the platform
#
#############################################################################

import os
import re
import os.path

try:
    from sonic_platform_base.thermal_base import ThermalBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

IPMI_SENSOR_NETFN = "0x04"
IPMI_SS_READ_CMD = "0x2D {}"
IPMI_SS_THRESHOLD_CMD = "0x27 {}"
DEFUALT_LOWER_TRESHOLD = 0.0
HIGH_TRESHOLD_SET_KEY = "unc"


class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    def __init__(self, thermal_index):
        ThermalBase.__init__(self)
        self._api_helper = APIHelper()
        self.index = thermal_index
        self.THERMAL_LIST = [
            ('TEMP_FAN_U52',        'Fan Tray Middle Temperature Sensor',
             '0x00',    'cpu'),
            ('TEMP_FAN_U17',        'Fan Tray Right Temperature Sensor',
             '0x01',    'cpu'),
            ('TEMP_SW_U52',
             'Switchboard Left Inlet Temperature Sensor',  '0x02',    'asic'),
            ('TEMP_SW_U16',
             'Switchboard Right Inlet Temperature Sensor', '0x03',    'asic'),
            ('TEMP_BB_U3',          'Baseboard Temperature Sensor',
             '0x04',    'cpu'),
            ('TEMP_CPU',            'CPU Internal Temperature Sensor',
             '0x05',    'cpu'),
            ('TEMP_SW_Internal',    'ASIC Internal Temperature Sensor',
             '0x61',    'asic'),
            ('SW_U04_Temp',         'IR3595 Chip Left Temperature Sensor',
             '0x4F',    'asic'),
            ('SW_U14_Temp',         'IR3595 Chip Right Temperature Sensor',
             '0x56',    'asic'),
            ('SW_U4403_Temp',       'IR3584 Chip Temperature Sensor',
             '0x5D',    'asic'),
        ]
        self.sensor_id = self.THERMAL_LIST[self.index][0]
        self.sensor_des = self.THERMAL_LIST[self.index][1]
        self.sensor_reading_addr = self.THERMAL_LIST[self.index][2]
        self.position = self.THERMAL_LIST[self.index][3]

        self.minimum_thermal = self.get_temperature()
        self.maximum_thermal = self.get_temperature()

    def __set_threshold(self, key, value):
        print('{} {}'.format(key, value))

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        temperature = 0.0
        status, raw_ss_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_READ_CMD.format(self.sensor_reading_addr))
        if status and len(raw_ss_read.split()) > 0:
            ss_read = raw_ss_read.split()[0]
            temperature = float(int(ss_read, 16))
        return temperature

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        high_threshold = 0.0
        status, raw_up_thres_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_THRESHOLD_CMD.format(self.sensor_reading_addr))
        if status and len(raw_up_thres_read.split()) > 6:
            ss_read = raw_up_thres_read.split()[4]
            high_threshold = float(int(ss_read, 16))
        return high_threshold

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal
        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return DEFUALT_LOWER_TRESHOLD

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        status, ret_txt = self._api_helper.ipmi_set_ss_thres(
            self.sensor_id, HIGH_TRESHOLD_SET_KEY, temperature)
        return status

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
        high_critical_threshold = 0.0
        status, raw_up_thres_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_THRESHOLD_CMD.format(self.sensor_reading_addr))
        if status and len(raw_up_thres_read.split()) > 6:
            ss_read = raw_up_thres_read.split()[5]
            high_critical_threshold = float(int(ss_read, 16))
        return high_critical_threshold

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal
        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        tmp = self.get_temperature()
        if tmp < self.minimum_thermal:
            self.minimum_thermal = tmp
        return self.minimum_thermal

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal
        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        tmp = self.get_temperature()
        if tmp > self.maximum_thermal:
            self.maximum_thermal = tmp
        return self.maximum_thermal

    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        return self.THERMAL_LIST[self.index][0]

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        return True if self.get_temperature() > 0 else False

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return self.sensor_des

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return "Unknown"

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
        if self.position == "cpu":
            return 1
        return 0
