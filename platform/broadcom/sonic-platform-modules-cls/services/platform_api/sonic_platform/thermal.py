#!/usr/bin/env python

#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the thermal status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.thermal_base import ThermalBase
    from common import Common
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    def __init__(self, index, _thermal_index=0, conf=None):
        ThermalBase.__init__(self)

        self._thermal_index = index
        self._config = conf
        self._api_common = Common(self._config)

    def get_name(self):
        """
        Retrieves the human-readable name of a thermal sensor by 1-based index

        Returns:
        :param index: An integer, 1-based index of the thermal sensor of which to query status
        :return: String,
            A string representing the name of the thermal sensor. 
        """
        return self._api_common.get_output(self._thermal_index, self._config['get_name'], Common.NULL_VAL)

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        by using command ipmitool raw 0x04 0x2D [address]

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125 
        """
        output = self._api_common.get_output(
            self._thermal_index, self._config['get_temperature'], Common.NULL_VAL)
        return float(output) if output != Common.NULL_VAL else output

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        output = self._api_common.get_output(
            self._thermal_index, self._config['get_high_threshold'], Common.NULL_VAL)
        return float(output) if output != Common.NULL_VAL else output

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            lnc as float number and return 0 if the BMC output is na.
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        output = self._api_common.get_output(
            self._thermal_index, self._config['get_low_threshold'], Common.NULL_VAL)
        return float(output) if output != Common.NULL_VAL else output

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal

        For AMI BMC device :
            use ipmitool command
            ipmitool sensor thresh [sensor name] unc [0>= temp_value <=62]
            if the current value of unc is 'na' ipmitool can't be set the value

        Args : 
            temperature: A float number up to nearest thousandth of one degree Celsius, 
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        output = self._api_common.get_output(
            self._thermal_index, self._config['set_high_threshold'], Common.NULL_VAL)
        return float(output) if output != Common.NULL_VAL else output

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal

        For AMI BMC device :
            use ipmitool command
            ipmitool sensor thresh [sensor name] lnc [temp_value]
            if the current value of lnc is 'na' ipmitool can't be set the value

        Args : 
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        output = self._api_common.get_output(
            self._thermal_index, self._config['set_low_threshold'], Common.NULL_VAL)
        return float(output) if output != Common.NULL_VAL else output

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            ucr as float number and return 0 if the BMC output is na.
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        output = self._api_common.get_output(
            self._thermal_index, self._config['get_high_critical_threshold'], Common.NULL_VAL)
        return float(output) if output != Common.NULL_VAL else output

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal

        Returns:
            lnr as float number and return 0 if the BMC output is na.
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        output = self._api_common.get_output(
            self._thermal_index, self._config['get_low_critical_threshold'], Common.NULL_VAL)
        return float(output) if output != Common.NULL_VAL else output

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return Common.NULL_VAL

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        return Common.NULL_VAL

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True
