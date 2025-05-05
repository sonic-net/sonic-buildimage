#!/usr/bin/env python

########################################################################
# Dell Z9664F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Thermals' information which are available in the platform
#
########################################################################


try:
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_platform.ipmihelper import IpmiSensor
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Thermal(ThermalBase):
    """Dell Platform-specific Thermal class"""

    # [ Sensor-Name, Sensor-ID, high threshold, high critical_threshold ]
    # TBD : 
    #       high thershold/hich crit threshold 
    #       need to be modified as True in case if it is supported
    #
    SENSOR_MAPPING = [
        ['CPU Temp', 0xd, True, True],
        ['FAN Right Temp', 0x9, True, True],
        ['FAN Left Temp', 0x8, True, True],
        ['NPU Front Temp', 0x6, True, True],
        ['NPU Temp', 0xb, True, True],
        ['PSU1 AF Temp', 0x46, False, True],
        ['PSU1 Front Temp', 0x47, False, True],
        ['PSU1 Rear Temp', 0x48, False, True],
        ['PSU2 AF Temp', 0x36, False, True],
        ['PSU2 Front Temp', 0x37, False, True],
        ['PSU2 Rear Temp', 0x38, False, True],
        ['PT Left Top Temp', 0x1, True, True],
        ['PT Left Bottom Temp', 0x4, True, True],
        ['PT Right Top Temp', 0x3, True, True],
        ['PT Right Bottom Temp', 0x5, True, True],
        ['CPUCD Right Temp', 0xa, True, True],
        ['TC Rear Temp', 0x2, True, True],
        ['PT Front Temp', 0x7, True, True]
        
    ]

    def __init__(self, thermal_index=0):
        ThermalBase.__init__(self)
        self.index = thermal_index + 1
        self.sensor = IpmiSensor(self.SENSOR_MAPPING[self.index - 1][1])
        self.has_high_threshold = self.SENSOR_MAPPING[self.index - 1][2]
        self.has_high_crit_threshold = self.SENSOR_MAPPING[self.index - 1][3]

    def get_name(self):
        """
        Retrieves the name of the thermal

        Returns:
            string: The name of the thermal
        """
        return self.SENSOR_MAPPING[self.index - 1][0]

    def get_presence(self):
        """
        Retrieves the presence of the thermal

        Returns:
            bool: True if thermal is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the Thermal

        Returns:
            string: Model/part number of Thermal
        """
        return 'NA'

    def get_serial(self):
        """
        Retrieves the serial number of the Thermal

        Returns:
            string: Serial number of Thermal
        """
        return 'NA'

    def get_status(self):
        """
        Retrieves the operational status of the thermal

        Returns:
            A boolean value, True if thermal is operating properly,
            False if not
        """
        return True

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to
            nearest thousandth of one degree Celsius, e.g. 30.125
        """
        is_valid, temperature = self.sensor.get_reading()
        if not is_valid:
            temperature = 0

        return float(temperature)

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        if self.has_high_threshold:
            is_valid, high_threshold = self.sensor.get_threshold("UpperNonCritical")
            if is_valid:
                return float(high_threshold)

        return super(Thermal, self).get_high_threshold()

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        return 0.0

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal
        Returns:
            A float number, the high critical threshold temperature of
            thermal in Celsius up to nearest thousandth of one degree
            Celsius, e.g. 30.125
        """
        if self.has_high_crit_threshold:
            is_valid, high_crit_threshold = self.sensor.get_threshold("UpperCritical")
            if is_valid:
                return float(high_crit_threshold)

        return super(Thermal, self).get_high_critical_threshold()

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one
            degree Celsius, e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if
            not
        """
        # Thermal threshold values are pre-defined based on HW.
        return False

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one
            degree Celsius, e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if
            not
        """
        # Thermal threshold values are pre-defined based on HW.
        return False

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.index

    def is_replaceable(self):
        """
        Indicate whether this Thermal is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
