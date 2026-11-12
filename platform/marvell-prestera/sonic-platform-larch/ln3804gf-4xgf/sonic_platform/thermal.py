try:
    import redis
    import time
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

STATE_DB_INDEX = 6
SYSLOG_IDENTIFIER = "thermal"
sonic_logger=logger.Logger(SYSLOG_IDENTIFIER)

class Thermal(ThermalBase):
    __name_of_thermal = [
        "ASIC Temperature"
    ]

    redis_client = redis.Redis(db=STATE_DB_INDEX)

    def __init__(self, thermal_index):
        self.thermal_index = thermal_index
        self.minimum_recorded = 100
        self.maximum_recorded = 0
        self.low_critical_threshold = 0
        self.low_threshold = 5
        self.high_critical_threshold = 80
        self.high_threshold = 70

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A int number, the current temperature of thermal in Celsius e.g. 30
        """
        temp_field_name = 'temperature_'+str(self.thermal_index)

        if self.redis_client.exists('ASIC_TEMPERATURE_INFO') != 1:
            sonic_logger.log_error("Thermal: ASIC_TEMPERATURE_INFO doesn`t exist")
            return 0
        if self.redis_client.hexists('ASIC_TEMPERATURE_INFO', temp_field_name) != 1:
            sonic_logger.log_error("Thermal: KEY ASIC_TEMPERATURE_INFO doesn`t have field {}".format(temp_field_name))
            return 0

        temp = int(self.redis_client.hget('ASIC_TEMPERATURE_INFO', temp_field_name))

        if temp < self.minimum_recorded:
            self.minimum_recorded = temp

        if temp > self.maximum_recorded:
            self.maximum_recorded = temp


        return temp

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal

        Returns:
            A int number, the minimum recorded temperature of thermal in Celsius e.g. 30
        """

        return self.minimum_recorded

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal

        Returns:
            A int number, the maximum recorded temperature of thermal in Celsius e.g. 30
        """

        return self.maximum_recorded
    
    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """

        return self.high_threshold

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal
        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.high_critical_threshold

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal
        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.low_critical_threshold

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal
        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """

        return self.low_threshold

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal
        Args :
            temperature: A float number up self.maximumto nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        self.high_threshold = temperature

        return True

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """

        self.low_threshold = temperature

        return True

    def set_high_critical_threshold(self, temperature):
        """
        Sets the critical high threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        self.high_critical_threshold = temperature
        return True

    def set_low_critical_threshold(self, temperature):
        """
        Sets the critical low threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        self.low_critical_threshold = temperature
        return True

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """

        return self.__name_of_thermal[self.thermal_index]

    def get_presence(self):
        """
        Retrieves the presence of the Thermal
        Returns:
            bool: True if Thermal is present, False if not
        """
        return True

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """

        return "N/A"

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return "N/A"

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.thermal_index+1

    def is_replaceable(self):
        """
        Retrieves whether thermal module is replaceable
        Returns:
            A boolean value, True if replaceable, False if not
        """
        return False
