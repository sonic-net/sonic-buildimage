#!/usr/bin/env python

#############################################################################
# Celestica
#
# Thermal contains an implementation of SONiC Platform Base API and
# provides the thermal device status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.thermal_base import ThermalBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

IPMI_GET_TEM="ipmitool raw 0x04 0x2D "
IPMI_GET_HIGH_THR="ipmitool raw 0x04 0x27 "
IPMI_SET_THR="ipmitool sensor thresh "
IPMI_SDR='ipmitool sdr | grep '
DEFAULT_VAL = 'N/A'
psu_tem=[9,10,11,12]

class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    def __init__(self, thermal_index):
        ThermalBase.__init__(self)
        self.thermal_index = thermal_index
        self._api_helper = APIHelper()

        ######  Thermal list defined #######
        # (NAME , val, set high  threshold)
        self.THERMAL_LIST = [
            ('Base_Temp_U5',       '0x1', ' unc '),
            ('Base_Temp_U7',       '0x2', ' unc '),
            ('CPU_Temp',           '0x7', ' unc '),
            ('Switch_Temp_U1',     '0x3', ' unc '),
            ('Switch_Temp_U18',    '0x4', ' unc '),
            ('Switch_Temp_U28',    '0x5', ' unc '),
            ('Switch_Temp_U29',    '0x6', ' unc '),
            ('Switch_U21_Temp',    '0x56', ' unc '),
            ('Switch_U33_Temp',    '0x4c', ' unc '),
            ('PSUL_Temp1',         '0x0', ' unc '),
            ('PSUL_Temp2',         '0x0', ' unc '),
            ('PSUR_Temp1',         '0x0', ' unc '),
            ('PSUR_Temp2',         '0x0', ' unc ')
        ]

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        if self.thermal_index in psu_tem:
            get_tem=IPMI_SDR+self.THERMAL_LIST[self.thermal_index][0]
        else:
            get_tem=IPMI_GET_TEM+self.THERMAL_LIST[self.thermal_index][1]
        status, raw_tem=self._api_helper.run_command(get_tem)
        if status:
            if self.thermal_index in psu_tem:
                return float(int(raw_tem.split()[2], 10))
            return float(int(raw_tem.split()[0], 16))
        else:
            return DEFAULT_VAL

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        no_thre=[2,7,8,9,10,11,12]
        default_thre=80.0
        if self.thermal_index in no_thre:
            return  default_thre

        get_thre=IPMI_GET_HIGH_THR+self.THERMAL_LIST[self.thermal_index][1]
        status, raw_thre=self._api_helper.run_command(get_thre)
        if status:
            return float(int(raw_thre.split()[4], 16))
        else:
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
        set_thre=IPMI_SET_THR+self.THERMAL_LIST[self.thermal_index][0]+\
                self.THERMAL_LIST[self.thermal_index][2]+str(temperature)
        status, raw_thre=self._api_helper.run_command(set_thre)
        if status:
            return True
        else:
            return False


    def get_low_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return DEFAULT_VAL


    def set_low_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return True

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal
        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        no_thre=[7,8,9,10,11,12]
        if self.thermal_index in no_thre:
            return  DEFAULT_VAL

        get_thre=IPMI_GET_HIGH_THR+self.THERMAL_LIST[self.thermal_index][1]
        status, raw_thre=self._api_helper.run_command(get_thre)
        if status:
            return float(int(raw_thre.split()[5], 16))
        else:
            return DEFAULT_VAL

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal
        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return DEFAULT_VAL

    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        name = self.THERMAL_LIST[self.thermal_index][0]
        if self.thermal_index in psu_tem:
            if 'PSUL' in name:
                name = name.replace('PSUL', 'PSU1')
            elif 'PSUR' in name:
                name = name.replace('PSUR', 'PSU2')
        return name

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return DEFAULT_VAL

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return DEFAULT_VAL

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True
