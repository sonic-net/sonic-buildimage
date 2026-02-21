"""
    Dell S3248T

    Module contains an implementation of SONiC Platform Base API and
    provides the Thermals' information which are available in the platform

"""

try:
    import os
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


class Thermal(ThermalBase):
    """Dell Platform-specific Thermal class"""

    # { Sensor name, Sensor-ID, high threshold, high critical threshold, Tlow , Thigh }
    SENSOR_MAPPING_NOR_AIRFLOW = {
        1: {"Sensor name": 'CPU On-board Temperature', "Id": '7-0048',
            "high-thr": 50, "high-crit-thr": 52, "Tlow": 71, "Thigh": 76},
        2: {"Sensor name": 'Main board Temperature', "Id": '7-0049',
            "high-thr": 68, "high-crit-thr": 70, "Tlow": 78, "Thigh": 83},
        3: {"Sensor name": 'Underneath CPU Temperature', "Id": '7-004a',
            "high-thr": 56, "high-crit-thr": 58, "Tlow": 75, "Thigh": 80},
        4: {"Sensor name": 'PSU inlet Temperature', "Id": '7-004b',
            "high-thr": 46, "high-crit-thr": 52, "Tlow": 74, "Thigh": 79},
        5: {"Sensor name": 'Front plate inlet Temperature', "Id": '7-004c',
            "high-thr": 46, "high-crit-thr": 50, "Tlow": 71, "Thigh": 76},
        6: {"Sensor name": 'Fan board Temperature', "Id": '7-004f',
            "high-thr": 46, "high-crit-thr": 52, "Tlow": 74, "Thigh": 79}
    }

    TEMP_LEVEL_UP_THRESHOLD_NOR_AIRFLOW = {
        1: {"LEVEL1": 46, "LEVEL2": 48, "LEVEL3": 50, "LEVEL4": 52},
        2: {"LEVEL1": 64, "LEVEL2": 66, "LEVEL3": 68, "LEVEL4": 70},
        3: {"LEVEL1": 52, "LEVEL2": 54, "LEVEL3": 56, "LEVEL4": 58},
        4: {"LEVEL1": 42, "LEVEL2": 44, "LEVEL3": 46, "LEVEL4": 52},
        5: {"LEVEL1": 42, "LEVEL2": 44, "LEVEL3": 46, "LEVEL4": 50},
        6: {"LEVEL1": 42, "LEVEL2": 44, "LEVEL3": 46, "LEVEL4": 52}
    }

    TEMP_LEVEL_DOWN_THRESHOLD_NOR_AIRFLOW = {
        1: {"LEVEL0": 34, "LEVEL1": 38, "LEVEL2": 41, "LEVEL3": 46},
        2: {"LEVEL0": 45, "LEVEL1": 48, "LEVEL2": 50, "LEVEL3": 53},
        3: {"LEVEL0": 40, "LEVEL1": 44, "LEVEL2": 47, "LEVEL3": 50},
        4: {"LEVEL0": 36, "LEVEL1": 40, "LEVEL2": 45, "LEVEL3": 49},
        5: {"LEVEL0": 33, "LEVEL1": 38, "LEVEL2": 42, "LEVEL3": 46},
        6: {"LEVEL0": 34, "LEVEL1": 39, "LEVEL2": 44, "LEVEL3": 49}
    }

    SENSOR_MAPPING_REV_AIRFLOW = {
        1: {"Sensor name": 'CPU On-board Temperature', "Id": '7-0048',
            "high-thr": 44, "high-crit-thr": 48, "Tlow": 69, "Thigh": 74},
        2: {"Sensor name": 'Main board Temperature', "Id": '7-0049',
            "high-thr": 52, "high-crit-thr": 55, "Tlow": 74, "Thigh": 79},
        3: {"Sensor name": 'Underneath CPU Temperature', "Id": '7-004a',
            "high-thr": 44, "high-crit-thr": 49, "Tlow": 70, "Thigh": 75},
        4: {"Sensor name": 'PSU inlet Temperature', "Id": '7-004b',
            "high-thr": 52, "high-crit-thr": 56, "Tlow": 77, "Thigh": 82},
        5: {"Sensor name": 'Front plate inlet Temperature', "Id": '7-004c',
            "high-thr": 51, "high-crit-thr": 54, "Tlow": 73, "Thigh": 78},
        6: {"Sensor name": 'Fan board Temperature', "Id": '7-004f',
            "high-thr": 41, "high-crit-thr": 46, "Tlow": 68, "Thigh": 73}
    }

    TEMP_LEVEL_UP_THRESHOLD_REV_AIRFLOW = {
        1: {"LEVEL1": 36, "LEVEL2": 39, "LEVEL3": 44, "LEVEL4": 48},
        2: {"LEVEL1": 48, "LEVEL2": 50, "LEVEL3": 52, "LEVEL4": 55},
        3: {"LEVEL1": 34, "LEVEL2": 40, "LEVEL3": 44, "LEVEL4": 49},
        4: {"LEVEL1": 47, "LEVEL2": 50, "LEVEL3": 52, "LEVEL4": 56},
        5: {"LEVEL1": 47, "LEVEL2": 49, "LEVEL3": 51, "LEVEL4": 54},
        6: {"LEVEL1": 31, "LEVEL2": 36, "LEVEL3": 41, "LEVEL4": 46}
    }

    TEMP_LEVEL_DOWN_THRESHOLD_REV_AIRFLOW = {
        1: {"LEVEL0": 29, "LEVEL1": 33, "LEVEL2": 38, "LEVEL3": 43},
        2: {"LEVEL0": 37, "LEVEL1": 39, "LEVEL2": 45, "LEVEL3": 49},
        3: {"LEVEL0": 29, "LEVEL1": 33, "LEVEL2": 38, "LEVEL3": 44},
        4: {"LEVEL0": 40, "LEVEL1": 43, "LEVEL2": 47, "LEVEL3": 51},
        5: {"LEVEL0": 36, "LEVEL1": 38, "LEVEL2": 42, "LEVEL3": 47},
        6: {"LEVEL0": 26, "LEVEL1": 31, "LEVEL2": 37, "LEVEL3": 43}
    }

    def __init__(self, thermal_index, airflow_dir):
        ThermalBase.__init__(self)
        self.index = thermal_index + 1

        if airflow_dir == 'exhaust':
            self.SENSOR_MAPPING = self.SENSOR_MAPPING_NOR_AIRFLOW
            self.TEMP_LEVEL_UP_THRESHOLD = self.TEMP_LEVEL_UP_THRESHOLD_NOR_AIRFLOW
            self.TEMP_LEVEL_DOWN_THRESHOLD = self.TEMP_LEVEL_DOWN_THRESHOLD_NOR_AIRFLOW
        else:
            self.SENSOR_MAPPING = self.SENSOR_MAPPING_REV_AIRFLOW
            self.TEMP_LEVEL_UP_THRESHOLD = self.TEMP_LEVEL_UP_THRESHOLD_REV_AIRFLOW
            self.TEMP_LEVEL_DOWN_THRESHOLD = self.TEMP_LEVEL_DOWN_THRESHOLD_REV_AIRFLOW

        self.is_driver_initialized = True

        temp_hwmon = '/sys/bus/i2c/devices/' + self.SENSOR_MAPPING[self.index]["Id"] + '/hwmon'
        try:
            hwmon_node = os.listdir(temp_hwmon)[0]
        except OSError:
            hwmon_node = "hwmon*"
            self.is_driver_initialized = False

        self.temp_file = temp_hwmon + '/' + hwmon_node + '/' + 'temp1_input'
        self.temp_max_file = temp_hwmon + '/' + hwmon_node + '/' + 'temp1_max'
        self.temp_max_hyst_file = temp_hwmon + '/' + hwmon_node + '/' + 'temp1_max_hyst'

    def get_name(self):
        """
        Retrieves the name of the thermal

        Returns:
            string: The name of the thermal
        """
        return self.SENSOR_MAPPING[self.index]["Sensor name"]

    @staticmethod
    def get_presence():
        """
        Retrieves the presence of the thermal

        Returns:
            bool: True if thermal is present, False if not
        """
        return True

    @staticmethod
    def get_model():
        """
        Retrieves the model number (or part number) of the Thermal

        Returns:
            string: Model/part number of Thermal
        """
        return 'NA'

    @staticmethod
    def get_serial():
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
        if self.get_presence() and self.get_temperature() > 0:
            return True

        return False

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to
            nearest thousandth of one degree Celsius, e.g. 30.125
        """
        temperature = 0.0
        if not os.path.exists(self.temp_file):
            return temperature

        try:
            with open(self.temp_file, 'r') as file_desc:
                temperature = float(file_desc.read()) / 1000.0
        except IOError:
            pass

        return temperature

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        return 0.0

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        return float(self.SENSOR_MAPPING[self.index]["high-thr"])

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        return float(self.SENSOR_MAPPING[self.index]["high-crit-thr"])

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
        # Thermal threshold values are pre-defined based on HW.
        return False

    def get_num_of_thermals(self):
        """
        Retrieves the number of thermal sensors

        Returns: number of thermal sensors
            e.g. 5
        """
        return len(self.SENSOR_MAPPING)

    def set_tlow_threshold(self):
        """
        Set the thermal sensor's TLow value

        """
        ret_val = 'ERR'

        if not os.path.exists(self.temp_max_hyst_file):
            return ret_val

        value = int(self.SENSOR_MAPPING[self.index]["Tlow"]) * 1000

        try:
            with open(self.temp_max_hyst_file, 'w') as file_desc:
                ret_val = file_desc.write(str(value))
        except IOError:
            ret_val = 'ERR'

        return ret_val

    def set_thigh_threshold(self):
        """
        Set the thermal sensor's THigh value

        """
        ret_val = 'ERR'

        if not os.path.exists(self.temp_max_file):
            return ret_val

        value = int(self.SENSOR_MAPPING[self.index]["Thigh"]) * 1000

        try:
            with open(self.temp_max_file, 'w') as file_desc:
                ret_val = file_desc.write(str(value))
        except IOError:
            ret_val = 'ERR'

        return ret_val

    def get_system_thermal_level(self, system_temperature, temp_up):
        """
        Get the system thermal level from the given thermal sensor reading

        Args:
            system_temperature: Current temperature
            temp_up : True - temperature increased

        Returns:
            Thermal level for the given temperature sensor value
        """
        index = self.index
        if temp_up:
            if system_temperature < self.TEMP_LEVEL_UP_THRESHOLD[index]["LEVEL1"]:
                curr_thermal_level = 0
            elif self.TEMP_LEVEL_UP_THRESHOLD[index]["LEVEL1"] <= system_temperature < self.TEMP_LEVEL_UP_THRESHOLD[index]["LEVEL2"]:
                curr_thermal_level = 1
            elif self.TEMP_LEVEL_UP_THRESHOLD[index]["LEVEL2"] <= system_temperature < self.TEMP_LEVEL_UP_THRESHOLD[index]["LEVEL3"]:
                curr_thermal_level = 2
            elif self.TEMP_LEVEL_UP_THRESHOLD[index]["LEVEL3"] <= system_temperature < self.TEMP_LEVEL_UP_THRESHOLD[index]["LEVEL4"]:
                curr_thermal_level = 3
            else:
                curr_thermal_level = 4
        else:
            if system_temperature <= self.TEMP_LEVEL_DOWN_THRESHOLD[index]["LEVEL0"]:
                curr_thermal_level = 0
            elif self.TEMP_LEVEL_DOWN_THRESHOLD[index]["LEVEL0"] < system_temperature <= self.TEMP_LEVEL_DOWN_THRESHOLD[index]["LEVEL1"]:
                curr_thermal_level = 1
            elif self.TEMP_LEVEL_DOWN_THRESHOLD[index]["LEVEL1"] < system_temperature <= self.TEMP_LEVEL_DOWN_THRESHOLD[index]["LEVEL2"]:
                curr_thermal_level = 2
            elif self.TEMP_LEVEL_DOWN_THRESHOLD[index]["LEVEL2"] < system_temperature <= self.TEMP_LEVEL_DOWN_THRESHOLD[index]["LEVEL3"]:
                curr_thermal_level = 3
            else:
                curr_thermal_level = 4

        return curr_thermal_level

    def is_thermal_level_lower_than(self, therm_lvl, temperature):
        """
        Check sensor temperature value equal or below the lower threshold of the given thermal level value

        Args:
            therm_lvl : Current maximum computed thermal level
            temperature: Current temperature

        Returns:
            True - Temperature value is equal or below the down thermal, otherwise False
        """
        return temperature <= self.TEMP_LEVEL_DOWN_THRESHOLD[self.index]["LEVEL{}".format(therm_lvl)]

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
