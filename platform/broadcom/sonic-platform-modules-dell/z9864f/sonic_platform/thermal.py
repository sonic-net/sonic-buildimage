#!/usr/bin/env python
"""
########################################################################
# DELL Z9864F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Thermals' information which are available in the platform
#
########################################################################
"""

try:
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_platform.ipmihelper import IpmiSensor
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


class Thermal(ThermalBase):
    """DELL Platform-specific Thermal class"""

    # [ Sensor-Name, Sensor-ID ]
    SENSOR_MAPPING = [
        ['CPU Temp', 0xd],
        ['NPU Temp', 0xb],
        ['NPU Rear Temp', 0x0],
        ['INLET Left Temp', 0x4],
        ['INLET Right Temp', 0x1],
        ['OUTLET Left Temp', 0x5],
        ['OUTLET Right Temp', 0x7],
        ['OSFP Rear Temp', 0x2],
        ['CPUCD Front Temp', 0x6],
        ['PSU1 AF Temp', 0x46],
        ['PSU1 MID Temp', 0x47],
        ['PSU1 Rear Temp', 0x48],
        ['PSU2 AF Temp', 0x36],
        ['PSU2 MID Temp', 0x37],
        ['PSU2 Rear Temp', 0x38],
    ]

    def __init__(self, thermal_index, name=None, parent=None):
        ThermalBase.__init__(self)
        self.index = thermal_index + 1
        self.sensor = None
        if name is None:
            self.sensor = IpmiSensor(self.SENSOR_MAPPING[self.index - 1][1])
        self.sensor_name = name
        self.parent = parent
        self.sensor_val = 0
        self.sensor_fail_count = 0

    def get_name(self):
        """
        Retrieves the name of the thermal

        Returns:
            string: The name of the thermal
        """
        if self.sensor_name is not None:
            return self.sensor_name
        return self.SENSOR_MAPPING[self.index - 1][0]

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to
            nearest thousandth of one degree Celsius, e.g. 30.125
        """
        temperature = 0
        # checking to see if SFP and getting its temperature
        if self.parent is not None:
            if self.parent.get_presence():
                data = self.parent.get_transceiver_temperature()
                temperature = data.replace("C", "")
                if temperature == 'N/A':
                    temperature = float(0)
        else:
            is_valid, temperature = self.sensor.get_reading()
            if not is_valid or float(temperature) == float(0):
                if self.sensor_fail_count < 2:
                    self.sensor_fail_count = (self.sensor_fail_count + 1)
                    temperature = self.sensor_val
                else:
                    temperature = 0
            else:
                self.sensor_fail_count = 0
                self.sensor_val = temperature

        return float(temperature)

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        # checking to see if SFP and reading its high threshold value
        if self.parent is not None:
            data = self.parent.get_transceiver_threshold_info()
            high_threshold = data.get("temphighwarning", "0").replace("C", "")
            if high_threshold == 'N/A':
                high_threshold = float(100)
            return float(high_threshold)

        is_valid, high_threshold = self.sensor.get_threshold("UpperNonCritical")
        if not is_valid:
            return super(Thermal, self).get_high_threshold()

        return float(high_threshold)

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        # checking to see if SFP and getting its high critical threshold value
        if self.parent is not None:
            data = self.parent.get_transceiver_threshold_info()
            high_crit_threshold = data.get("temphighalarm", "0").replace("C", "")
            if high_crit_threshold == 'N/A':
                high_crit_threshold = float(1000)
            return float(high_crit_threshold)

        is_valid, high_crit_threshold = self.sensor.get_threshold("UpperCritical")
        if not is_valid:
            return super(Thermal, self).get_high_critical_threshold()

        return float(high_crit_threshold)

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        if self.sensor is None:
            return float(0)

        is_valid, low_threshold = self.sensor.get_threshold("LowerNonRecoverable")
        if not is_valid:
            low_threshold = 0

        return float(low_threshold)

    @staticmethod
    def set_high_threshold(temperature):
        """
        Sets the high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one
            degree Celsius, e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if
            not
        """
        del temperature
        # Thermal threshold values are pre-defined based on HW.
        return False

    @staticmethod
    def set_low_threshold(temperature):
        """
        Sets the low threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one
            degree Celsius, e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if
            not
        """
        del temperature
        # Thermal threshold values are pre-defined based on HW.
        return False
