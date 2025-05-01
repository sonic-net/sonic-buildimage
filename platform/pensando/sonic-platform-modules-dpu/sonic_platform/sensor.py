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
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

# [ Sensor-Name, sysfs]
VOLTAGE_SENSOR_MAPPING = [
    ["Voltage sensor 1", "/sys/class/hwmon/hwmon0/in2_input"],
    ["Voltage sensor 2", "/sys/class/hwmon/hwmon1/in2_input"],
    ["Voltage sensor 3", "/sys/class/hwmon/hwmon2/in2_input"]
]

# [ Sensor-Name, sysfs]
CURRENT_SENSOR_MAPPING = [
    ["Current sensor 1", "/sys/class/hwmon/hwmon0/curr1_input"],
    ["Current sensor 2", "/sys/class/hwmon/hwmon1/curr1_input"],
    ["Current sensor 3", "/sys/class/hwmon/hwmon2/curr1_input"]
]

class VoltageSensor(SensorBase):
    """
    Abstract base class for interfacing with a voltage sensor module
    """
    @classmethod
    def _validate_voltage_sensors(cls):
        for [sensor_name, sensor_hwmon] in VOLTAGE_SENSOR_MAPPING:
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
        self.index = voltage_sensor_index
        self.sensor_hwmon_path = None
        sensor_hwmon = VOLTAGE_SENSOR_MAPPING[self.index][1]
        if os.path.exists(sensor_hwmon):
            self.sensor_hwmon_path = sensor_hwmon

    def get_name(self):
        return VOLTAGE_SENSOR_MAPPING[self.index][0]

    def get_value(self):
        voltage = 0.0
        try:
            with open(self.sensor_hwmon_path, 'r') as file:
                voltage = float(int(file.read())/1000)
        except Exception:
            return None
        return voltage

class CurrentSensor(SensorBase):
    """
    Abstract base class for interfacing with a current sensor module
    """
    @classmethod
    def _validate_current_sensors(cls):
        for [sensor_name, sensor_hwmon] in CURRENT_SENSOR_MAPPING:
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
        self.index = current_sensor_index
        self.sensor_hwmon_path = None
        sensor_hwmon = CURRENT_SENSOR_MAPPING[self.index][1]
        if os.path.exists(sensor_hwmon):
            self.sensor_hwmon_path = sensor_hwmon

    def get_name(self):
        return CURRENT_SENSOR_MAPPING[self.index][0]

    def get_value(self):
        current = 0
        try:
            with open(self.sensor_hwmon_path, 'r') as file:
                current = int(file.read())
        except Exception:
            return None
        return current
