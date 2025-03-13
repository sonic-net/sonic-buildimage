#!/usr/bin/env python3
"""
########################################################################
# DellEMC S5448F
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
    """DellEMC Platform-specific Thermal class"""

    # [ Sensor-Name, Sensor-ID ]
    SENSOR_MAPPING = [
        ['NPU Rear Temp', 0x1],
        ['PT Left Temp', 0x2],
        ['CPUCD Rear Temp', 0x3],
        ['PT Right Temp', 0x4],
        ['FAN Right Temp', 0x0],
        ['NPU Temp', 0x8],
        ['PSU2 AF Temp', 0x36],
        ['PSU2 Mid Temp', 0x37],
        ['PSU2 Rear Temp', 0x38],
        ['PSU1 AF Temp', 0x46],
        ['PSU1 Mid Temp', 0x47],
        ['PSU1 Rear Temp', 0x48],
        ['CPU Temp', 0xd],
    ]

    def __init__(self, thermal_index):
        ThermalBase.__init__(self)
        self.index = thermal_index + 1
        self.sensor = IpmiSensor(self.SENSOR_MAPPING[self.index - 1][1])

    def get_name(self):
        """
        Retrieves the name of the thermal

        Returns:
            string: The name of the thermal
        """
        return self.SENSOR_MAPPING[self.index - 1][0]

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

