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
HIGH_TRESHOLD_SET_KEY = "unc"
DEFAULT_VAL = 'N/A'

class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    def __init__(self, thermal_index):
        ThermalBase.__init__(self)
        self._api_helper = APIHelper()
        self.index = thermal_index
        thermal_list = [\
            ('Base_Temp_U5',    '0x01', 'Baseboard Right Temp'),\
            ('Base_Temp_U7',    '0x02', 'Baseboard Left Temp'),\
            ('Switch_Temp_U31', '0x03', 'ASIC External Front Temp'),\
            ('Switch_Temp_U30', '0x04', 'ASIC External Rear Temp'),\
            ('Switch_Temp_U29', '0x05', 'Switchboard Right Temp'),\
            ('Switch_Temp_U28', '0x06', 'Switchboard Left Temp'),\
            ('CPU_Temp',        '0x07', 'CPU Internal Temp'),\
            ('Switch_U33_Temp', '0x4C', 'IR3595 Chip Temp'),\
            ('Switch_U21_Temp', '0x56', 'IR3584 Chip Temp'),\
            ('PSUL_Temp1',       '0x0D', 'PSU 1 Ambient Temp'),\
            ('PSUL_Temp2',       '0x0E', 'PSU 1 Hotspot Temp'),\
            ('PSUR_Temp1',       '0x17', 'PSU 2 Ambient Temp'),\
            ('PSUR_Temp2',       '0x18', 'PSU 2 Hotspot Temp'),\
        ]
        self.sensor_id = thermal_list[self.index][0]
        self.sensor_reading_addr = thermal_list[self.index][1]
        self.sensor_name = thermal_list[self.index][2]

        temp = self.get_temperature(True)
        self.minimum_thermal = temp
        self.maximum_thermal = temp

    def get_temperature(self, skip_record=False):
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

        if temperature != 0.0:
            if not skip_record:
                # Record maximum
                if temperature > self.maximum_thermal:
                    self.maximum_thermal = temperature

                # Record minimum
                if temperature < self.minimum_thermal:
                    self.minimum_thermal = temperature

            return temperature
        else:
            return DEFAULT_VAL

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
        if high_threshold != 0.0:
            return high_threshold
        else:
            return DEFAULT_VAL

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal
        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return DEFAULT_VAL

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
        if high_critical_threshold != 0.0:
            return high_critical_threshold
        else:
            return DEFAULT_VAL

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal
        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.minimum_thermal

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal
        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
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
        return self.sensor_name

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        return True if self.get_temperature() > 0 else False

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
