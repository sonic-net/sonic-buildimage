import glob
import sys

try:
    import redis
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_py_common import logger
    from .helper import APIHelper
    from .bridges import bridge_bus, mux_child
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


if sys.version_info[0] < 3:
    import commands as cmd
else:
    import subprocess as cmd

# LM75-class sensors on channel 3 of the PCA9548 0x70 behind the CP2112
# system bridge (HW spec "M_I2C2_SYS-Device-Master": EM_AD2 temp/ADC)
THERMAL_MUX_ADDR = 0x70
THERMAL_MUX_CHANNEL = 3

STATE_DB_INDEX = 6
SYSLOG_IDENTIFIER = "thermal"

sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)

class Thermal(ThermalBase):
    __name_of_thermal = [
        "Thermal SENSOR 1",
        "Thermal SENSOR 2",
        "ASIC Temperature"
    ]

    __dev_idx_hw_address = {
        1: "48",
        2: "49"
    }

    __redis_client = redis.Redis(db=STATE_DB_INDEX)

    def __init__(self, thermal_index):
        self.thermal_index = thermal_index
        self.minimum_recorded = 100
        self.maximum_recorded = 0
        self.dev_idx = thermal_index + 1
        self.temp_idx = 1
        self._api_helper = APIHelper()
        self.bus_num = None
        self.dev_addr = self.__dev_idx_hw_address.get(self.dev_idx)

    def _hwmon_file(self, name):
        """sysfs path of a hwmon attribute of this LM75, or None (ASIC / no device)."""
        if self.dev_addr is None:
            return None
        if self.bus_num is None:
            self.bus_num = mux_child(bridge_bus("sys"), THERMAL_MUX_ADDR, THERMAL_MUX_CHANNEL)
        if self.bus_num is None:
            return None
        hwmons = glob.glob(f"/sys/bus/i2c/devices/{self.bus_num}-00{self.dev_addr}/hwmon/hwmon*")
        return f"{hwmons[0]}/temp{self.temp_idx}_{name}" if hwmons else None

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number, the temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
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

    def __get_asic_temp(self):
        # orchagent publishes ASIC_TEMPERATURE_INFO with average_temperature /
        # maximum_temperature (no per-sensor fields on Falcon)
        if self.__redis_client.exists('ASIC_TEMPERATURE_INFO') != 1:
            sonic_logger.log_warning("Thermal: ASIC_TEMPERATURE_INFO doesn`t exist yet")
            return 0
        for field in ('average_temperature', 'maximum_temperature', 'temperature_' + str(self.thermal_index)):
            value = self.__redis_client.hget('ASIC_TEMPERATURE_INFO', field)
            if value is not None:
                try:
                    return int(float(value))
                except ValueError:
                    return 0
        return 0
    
    def __get_board_temp(self):
        file_path = self._hwmon_file("input")
        if file_path is None:
            sonic_logger.log_error("Thermal: sensor {} not available (CP2112 system bridge / lm75)".format(self.dev_addr))
            return 0
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
        return "lm75a"

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

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """

        if self.dev_addr is None:
            return 105.0            # ASIC: Falcon junction limit, no sysfs threshold
        file_path = self._hwmon_file("max")
        raw_temp_value = self._api_helper.read_txt_file(file_path) if file_path else None
        if raw_temp_value is not None:
            return float(raw_temp_value)/1000
        sonic_logger.log_error("Thermal: failed to read the high threshold of {}".format(self.get_name()))
        return 0

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        raise NotImplementedError

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        file_path = self._hwmon_file("max")
        if file_path is None:
            return False
        return self._api_helper.write_txt_file(file_path, int(temperature*1000))

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
        raise NotImplementedError

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
