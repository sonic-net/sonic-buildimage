import sys

try:
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_py_common import logger
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

if sys.version_info[0] < 3:
    import commands as cmd
else:
    import subprocess as cmd

smbus_present = 1
try:
    import smbus
except ImportError as e:
    smbus_present = 0

STATE_DB_INDEX = 6
SYSLOG_IDENTIFIER = "thermal"
sonic_logger=logger.Logger(SYSLOG_IDENTIFIER)

class Thermal(ThermalBase):

    THERMALS_PER_DEVICE = 3

    __name_of_thermal = [
        "TEMP SENSOR 1",
        "TEMP SENSOR 2",
        "TEMP SENSOR 3",
        "TEMP SENSOR 4",
        "TEMP SENSOR 5",
        "TEMP SENSOR 6"
    ]

    __dev_idx_hw_address = {
        1: "2c",
        2: "2d"
    }

    def __init__(self, thermal_index):
        self.thermal_index = thermal_index
        self.minimum_recorded = 100
        self.maximum_recorded = 0
        self.dev_idx = self.thermal_index//self.THERMALS_PER_DEVICE + 1
        self.temp_idx = self.thermal_index%self.THERMALS_PER_DEVICE + 1
        self.dev_addr = self.__dev_idx_hw_address[self.dev_idx]
        self._api_helper = APIHelper()

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A int number, the current temperature of thermal in Celsius e.g. 30
        """
        temp = self.__get_board_temp()

        if temp < self.minimum_recorded:
            self.minimum_recorded = temp

        if temp > self.maximum_recorded:
            self.maximum_recorded = temp

        return temp

    def __get_board_temp(self):
        file_path = f"/sys/bus/i2c/devices/0-00{self.dev_addr}/hwmon/hwmon{self.dev_idx}/temp{self.temp_idx}_input"
        raw_temp_value = self._api_helper.read_txt_file(file_path)
        if raw_temp_value is not None:
            return float(raw_temp_value)/1000
        else:
            sonic_logger.log_error("Thermal: failed to read the temperature from {}".format(file_path))
            return 0

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self.__name_of_thermal[self.thermal_index]

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
        return "adt7476"

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        return "N/A"

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return "N/A"

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True

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
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        temp = None

        temp_sensor_name = self.__name_of_thermal[self.thermal_index]
        if 'ASIC' in temp_sensor_name.upper():
            temp = self.__get_asic_temp()
        else:
            temp = self.__get_board_temp()

        if temp < self.minimum_recorded:
            self.minimum_recorded = temp

        if temp > self.maximum_recorded:
            self.maximum_recorded = temp

        return temp


    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """

        file_path = f"/sys/bus/i2c/devices/0-00{self.dev_addr}/hwmon/hwmon{self.dev_idx}/temp{self.temp_idx}_max"
        raw_temp_value = self._api_helper.read_txt_file(file_path)
        if raw_temp_value is not None:
            return float(raw_temp_value)/1000
        else:
            sonic_logger.log_error("Thermal: failed to read the value from {}".format(file_path))
            return 0

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        file_path = f"/sys/bus/i2c/devices/0-00{self.dev_addr}/hwmon/hwmon{self.dev_idx}/temp{self.temp_idx}_min"
        raw_temp_value = self._api_helper.read_txt_file(file_path)
        if raw_temp_value is not None:
            return float(raw_temp_value)/1000
        else:
            sonic_logger.log_error("Thermal: failed to read the value from {}".format(file_path))
            return 0

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        raise NotImplementedError

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        raise NotImplementedError

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        file_path = f"/sys/bus/i2c/devices/0-00{self.dev_addr}/hwmon/hwmon{self.dev_idx}/temp{self.temp_idx}_crit"
        raw_temp_value = self._api_helper.read_txt_file(file_path)
        if raw_temp_value is not None:
            return float(raw_temp_value)/1000
        else:
            sonic_logger.log_error("Thermal: failed to read the value from {}".format(file_path))
            return 0

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal

        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.get_low_threshold()

    def set_high_critical_threshold(self, temperature):
        """
        Sets the critical high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        raise NotImplementedError

    def set_low_critical_threshold(self, temperature):
        """
        Sets the critical low threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        raise NotImplementedError

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal

        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.minimum_recorded

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal

        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.maximum_recorded
