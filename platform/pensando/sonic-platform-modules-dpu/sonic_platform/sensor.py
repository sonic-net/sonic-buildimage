#!/usr/bin/env python
# {C} Copyright 2023 AMD Systems Inc. All rights reserved
########################################################################
# Pensando
#
# Module contains an implementation of SONiC Platform Base API and
# provides the voltage and current information which are available in the platform
#
########################################################################


try:
    from sonic_platform_base.sensor_base import SensorBase
    import os
    import syslog
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NOT_AVAILABLE = "N/A"
# [ Sensor-Name, sysfs, low_threshold, high_threshold, critical_low, critical_high]
VOLTAGE_SENSOR_MAPPING_V1 = [
    ["VP0P85_VDD_DDR_DPU0", "/sys/bus/i2c/devices/0-0044/hwmon/hwmon2/in2_input", "0.816", "0.884", "0.7905", "0.9095"],
    ["VP1P2_DDR_VDDQ_DPU0", "/sys/bus/i2c/devices/0-0044/hwmon/hwmon2/in3_input", "1.152", "1.248", "1.116", "1.284"],
    ["VP0P75_VDD_CORE_DPU0 1", "/sys/bus/i2c/devices/0-0055/hwmon/hwmon1/in2_input", "0.72", "0.78", "0.6975", "0.8025"],
    ["VP0P75_VDD_CORE_DPU0 2", "/sys/bus/i2c/devices/0-0055/hwmon/hwmon1/in3_input", "0.72", "0.78", "0.6975", "0.8025"],
    ["VP0P75_VDD_CORE_DPU0 3", "/sys/bus/i2c/devices/0-0066/hwmon/hwmon0/in2_input", "0.72", "0.78", "0.6975", "0.8025"],
    ["VP0P85_VDD_ARM_DPU0", "/sys/bus/i2c/devices/0-0066/hwmon/hwmon0/in3_input", "0.816", "0.884", "0.7905", "0.9095"]
]

# [ Sensor-Name, sysfs, low_threshold, high_threshold, critical_low, critical_high]
CURRENT_SENSOR_MAPPING_V1 = [
    ["VP0P85_VDD_DDR_DPU0", "/sys/bus/i2c/devices/0-0044/hwmon/hwmon2/curr1_input", "0", "15100", NOT_AVAILABLE, "30000"],
    ["VP1P2_DDR_VDDQ_DPU0", "/sys/bus/i2c/devices/0-0044/hwmon/hwmon2/curr2_input", "0", "13800", NOT_AVAILABLE, "30000"],
    ["VP0P75_VDD_CORE_DPU0 1", "/sys/bus/i2c/devices/0-0055/hwmon/hwmon1/curr1_input", "0", "25000", NOT_AVAILABLE, "30000"],
    ["VP0P75_VDD_CORE_DPU0 2", "/sys/bus/i2c/devices/0-0055/hwmon/hwmon1/curr2_input", "0", "25000", NOT_AVAILABLE, "30000"],
    ["VP0P75_VDD_CORE_DPU0 3", "/sys/bus/i2c/devices/0-0066/hwmon/hwmon0/curr1_input", "0", "25000", NOT_AVAILABLE, "30000"],
    ["VP0P85_VDD_ARM_DPU0", "/sys/bus/i2c/devices/0-0066/hwmon/hwmon0/curr2_input", "0", "29100", NOT_AVAILABLE, "30000"],
]

# [ Sensor-Name, sysfs, low_threshold, high_threshold, critical_low, critical_high]
VOLTAGE_SENSOR_MAPPING_V2 = [
    ["VP0P85_VDD_DDR_DPU0", "/sys/bus/i2c/devices/0-0072/hwmon/hwmon0/in2_input", "0.816", "0.884", "0.7905", "0.9095"],
    ["VP1P2_DDR_VDDQ_DPU0", "/sys/bus/i2c/devices/0-0072/hwmon/hwmon0/in3_input", "1.152", "1.248", "1.116", "1.284"],
    ["VP0P75_VDD_CORE_DPU0", "/sys/bus/i2c/devices/0-0062/hwmon/hwmon1/in2_input", "0.72", "0.78", "0.6975", "0.8025"],
    ["VP0P85_VDD_ARM_DPU0", "/sys/bus/i2c/devices/0-0062/hwmon/hwmon1/in3_input", "0.816", "0.884", "0.7905", "0.9095"]
]

# [ Sensor-Name, sysfs, low_threshold, high_threshold, critical_low, critical_high]
CURRENT_SENSOR_MAPPING_V2 = [
    ["VP0P85_VDD_DDR_DPU0", "/sys/bus/i2c/devices/0-0072/hwmon/hwmon0/curr1_input", "0", "15100", NOT_AVAILABLE, "30000"],
    ["VP1P2_DDR_VDDQ_DPU0", "/sys/bus/i2c/devices/0-0072/hwmon/hwmon0/curr2_input", "0", "13800", NOT_AVAILABLE, "30000"],
    ["VP0P75_VDD_CORE_DPU0", "/sys/bus/i2c/devices/0-0062/hwmon/hwmon1/curr1_input", "0", "25000", NOT_AVAILABLE, "30000"],
    ["VP0P85_VDD_ARM_DPU0", "/sys/bus/i2c/devices/0-0062/hwmon/hwmon1/curr2_input", "0", "29100", NOT_AVAILABLE, "30000"],
]

class VoltageSensor(SensorBase):
    """
    Abstract base class for interfacing with a voltage sensor module
    """
    @classmethod
    def _validate_voltage_sensors(cls):
        from sonic_platform.helper import APIHelper
        apiHelper = APIHelper()
        board_rev = apiHelper.get_board_rev()
        voltage_sensor_mapping = VOLTAGE_SENSOR_MAPPING_V1
        if board_rev == apiHelper.mtfuji_rev_v2:
            voltage_sensor_mapping = VOLTAGE_SENSOR_MAPPING_V2
        for sensor_name, sensor_hwmon, *_ in voltage_sensor_mapping:
            if not os.path.exists(sensor_hwmon):
                return False
        return True

    @classmethod
    def get_type(cls):
        return "SENSOR_TYPE_VOLTAGE"

    @classmethod
    def get_unit(cls):
        return "V"

    def __init__(self, voltage_sensor_index):
        SensorBase.__init__(self)
        self._api_helper = APIHelper()
        self.index = voltage_sensor_index
        self.sensor_hwmon_path = None
        self.board_rev = self._api_helper.get_board_rev()
        self.voltage_sensor_mapping = VOLTAGE_SENSOR_MAPPING_V1
        if self.board_rev == self._api_helper.mtfuji_rev_v2:
            self.voltage_sensor_mapping = VOLTAGE_SENSOR_MAPPING_V2
        sensor_hwmon = self.voltage_sensor_mapping[self.index][1]
        if os.path.exists(sensor_hwmon):
            self.sensor_hwmon_path = sensor_hwmon

    def get_name(self):
        return self.voltage_sensor_mapping[self.index][0]

    def get_value(self):
        voltage = 0.0
        try:
            with open(self.sensor_hwmon_path, 'r') as file:
                voltage = float(int(file.read())/1000)
        except Exception:
            return None
        return voltage

    def get_high_threshold(self):
        """
        Retrieves the high threshold of sensor

        Returns:
            High threshold 
        """
        value = self.voltage_sensor_mapping[self.index][3]
        if value == NOT_AVAILABLE:
            return NOT_AVAILABLE
        return float(value)

    def get_low_threshold(self):
        """
        Retrieves the low threshold 

        Returns:
            Low threshold 
        """
        value = self.voltage_sensor_mapping[self.index][2]
        if value == NOT_AVAILABLE:
            return NOT_AVAILABLE
        return float(value)

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold value of sensor

        Returns:
            The high critical threshold value of sensor 
        """
        value = self.voltage_sensor_mapping[self.index][5]
        if value == NOT_AVAILABLE:
            return NOT_AVAILABLE
        return float(value)

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold value of sensor

        Returns:
            The low critical threshold value of sensor 
        """
        value = self.voltage_sensor_mapping[self.index][4]
        if value == NOT_AVAILABLE:
            return NOT_AVAILABLE
        return float(value)

class CurrentSensor(SensorBase):
    """
    Abstract base class for interfacing with a current sensor module
    """
    @classmethod
    def _validate_current_sensors(cls):
        from sonic_platform.helper import APIHelper
        apiHelper = APIHelper()
        board_rev = apiHelper.get_board_rev()
        current_sensor_mapping = CURRENT_SENSOR_MAPPING_V1
        if board_rev == apiHelper.mtfuji_rev_v2:
            current_sensor_mapping = CURRENT_SENSOR_MAPPING_V2
        for sensor_name, sensor_hwmon, *_ in current_sensor_mapping:
            if not os.path.exists(sensor_hwmon):
                return False
        return True

    @classmethod
    def get_type(cls):
        return "SENSOR_TYPE_CURRENT"

    @classmethod
    def get_unit(cls):
        return "mA"

    def __init__(self, current_sensor_index):
        SensorBase.__init__(self)
        self._api_helper = APIHelper()
        self.index = current_sensor_index
        self.sensor_hwmon_path = None
        self.board_rev = self._api_helper.get_board_rev()
        self.current_sensor_mapping = CURRENT_SENSOR_MAPPING_V1
        if self.board_rev == self._api_helper.mtfuji_rev_v2:
            self.current_sensor_mapping = CURRENT_SENSOR_MAPPING_V2
        sensor_hwmon = self.current_sensor_mapping[self.index][1]
        if os.path.exists(sensor_hwmon):
            self.sensor_hwmon_path = sensor_hwmon

    def get_name(self):
        return self.current_sensor_mapping[self.index][0]

    def get_value(self):
        current = 0
        try:
            with open(self.sensor_hwmon_path, 'r') as file:
                current = int(file.read())
        except Exception:
            return None
        return current

    def get_high_threshold(self):
        """
        Retrieves the high threshold of sensor

        Returns:
            High threshold 
        """
        value = self.current_sensor_mapping[self.index][3]
        if value == NOT_AVAILABLE:
            return NOT_AVAILABLE
        return float(value)

    def get_low_threshold(self):
        """
        Retrieves the low threshold 

        Returns:
            Low threshold 
        """
        value = self.current_sensor_mapping[self.index][2]
        if value == NOT_AVAILABLE:
            return NOT_AVAILABLE
        return float(value)

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold value of sensor

        Returns:
            The high critical threshold value of sensor 
        """
        value = self.current_sensor_mapping[self.index][5]
        if value == NOT_AVAILABLE:
            return NOT_AVAILABLE
        return float(value)

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold value of sensor

        Returns:
            The low critical threshold value of sensor 
        """
        value = self.current_sensor_mapping[self.index][4]
        if value == NOT_AVAILABLE:
            return NOT_AVAILABLE
        return float(value)

