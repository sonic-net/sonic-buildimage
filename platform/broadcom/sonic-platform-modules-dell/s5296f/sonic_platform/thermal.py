#!/usr/bin/env python

########################################################################
# DellEMC S5296F
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
    """DellEMC Platform-specific Thermal class"""

    # [ Sensor-Name, Sensor-ID ]
    SENSOR_MAPPING = [
        ['PT Middle Sensor', 0x1],
        ['ASIC On-board', 0x2],
        ['PT Left Sensor', 0x3],
        ['PT Right Sensor', 0x4],
        ['Inlet Airflow Sensor', 0x5],
        ['PSU1 Airflow Sensor', 0x7],
        ['PSU2 Airflow Sensor', 0x8],
        ['PSU1 Sensor', 0xc],
        ['PSU2 Sensor', 0xd],
        ['CPU On-board', 0xe]
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
        is_valid, high_threshold = self.sensor.get_threshold("UpperNonCritical")
        if not is_valid:
            return 0.0

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
            return 0.0

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
