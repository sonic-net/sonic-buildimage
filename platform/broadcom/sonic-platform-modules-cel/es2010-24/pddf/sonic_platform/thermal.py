#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_platform.helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SENSORS_THRESHOLD_MAP = {
        "BMC TEMP":            { "high_threshold": 90,  "high_crit_threshold": 100},
        "CPU TEMP":            { "high_threshold": 87,  "high_crit_threshold": 92},
        "SWITCH TEMP":         { "high_threshold": 105, "high_crit_threshold": 109},
        "DIMM0 TEMP":          { "high_threshold": 85,  "high_crit_threshold": 88},
        "INLET TEMP1":         { "high_threshold": 50,  "high_crit_threshold": 55},
        "INLET TEMP2":         { "high_threshold": 50,  "high_crit_threshold": 55},
        "MAINBOARD TEMP1":     { "high_threshold": 50,  "high_crit_threshold": 55},
        "MAINBOARD TEMP2":     { "high_threshold": 50,  "high_crit_threshold": 55},
        "B2F INLET TEMP1":     { "high_threshold": 50,  "high_crit_threshold": 55},
        "B2F INLET TEMP2":     { "high_threshold": 50,  "high_crit_threshold": 55},
        "F2B INLET TEMP1":     { "high_threshold": 50,  "high_crit_threshold": 55},
        "F2B INLET TEMP2":     { "high_threshold": 50,  "high_crit_threshold": 55}
}

NONPDDF_THERMAL_SENSORS = {
    "CPU TEMP":       { "label": "coretemp-isa-0000", "high_threshold": 87, "high_crit_threshold": 92,
                        "temp_cmd": ['cat', '/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp1_input']},
    "ALTITUDE TEMP":   { "label": "mpl3115-i2c-0-60",
                        "temp_cmd": ['cat', '/sys/bus/iio/devices/iio:device0/in_temp_raw']},
    "DIMM0 TEMP":     { "label": "jc42-i2c-1-18", "high_threshold":  85, "high_crit_threshold": 88,
                        "temp_cmd": ['cat','/sys/class/hwmon/hwmon11/temp1_input']},
    "PSU 1 TEMP2":     { "label": "psu_pmbus-i2c-18-58",
                        "temp_cmd": ['i2cget', '-y', '-f', '18', '0x58', '0x8e', 'w']},
    "PSU 1 TEMP3":     { "label": "psu_pmbus-i2c-18-58",
                        "temp_cmd": ['i2cget', '-y', '-f', '18', '0x58', '0x8f', 'w']},
    "PSU 2 TEMP2":     { "label": "psu_pmbus-i2c-19-59",
                        "temp_cmd": ['i2cget', '-y', '-f', '19', '0x59', '0x8e', 'w']},
    "PSU 2 TEMP3":     { "label": "psu_pmbus-i2c-19-59",
                        "temp_cmd": ['i2cget', '-y', '-f', '19', '0x59', '0x8f', 'w']},
    }

TEMP_OFFSET = {
    "B2F INLET TEMP1":  -1,
    "B2F INLET TEMP2":  -2,
    "F2B INLET TEMP1":  -2,
    "F2B INLET TEMP2":  -3
}

class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal=is_psu_thermal,
                             psu_index=psu_index)

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_temperature(self):
        temperature = None
        temperature = super().get_temperature()
        if temperature is None:
            return None

        name = self.get_name()
        if "B2F" in name or "F2B" in name:
            offset = TEMP_OFFSET.get(name, 0)
            temperature += offset

        temperature = round(temperature, 1)
        return float(temperature)

    def get_high_threshold(self):
        thermal_data = SENSORS_THRESHOLD_MAP.get(self.get_name(), {})
        threshold = thermal_data.get("high_threshold")
        if threshold is not None:
            return float(threshold)
        return super().get_high_threshold()

    def get_high_critical_threshold(self):
        thermal_data = SENSORS_THRESHOLD_MAP.get(self.get_name(), {})
        threshold = thermal_data.get("high_crit_threshold")
        if threshold is not None:
            return float(threshold)
        return super().get_high_critical_threshold()

class NonPddfThermal(ThermalBase):
    def __init__(self, index, name):
        self.thermal_index = index + 1
        self.thermal_name = name
        self._api_helper = APIHelper()

    def __get_altitude_temp(self, cmd):
        status, data = self._api_helper.run_command(cmd)
        if not status or not data:
            return None
        return float(data) * 0.0625

    def __get_psu_temp(self, cmd):
        status, data = self._api_helper.run_command(cmd)
        if not status or not data:
            return None
        value = int(data, 16)
        return self.__psu_linear_data(value)

    def __psu_linear_data(self, data):
        data &= 0xFFFF
        expn = data >> 11
        data &= 0x7FF
        if (data & ( 1 << 10 )):
            val = float(data - 2048)
        else:
            val = float(data)

        if (expn & ( 1 << 4 )):
            res = val / (1 << (32 -expn))
        else:
            res = val * (1 << expn)
        return res

    def get_name(self):
        return self.thermal_name

    def get_position_in_parent(self):
        return self.thermal_index

    def get_presence(self):
        return True

    def get_temperature(self):
        thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name)
        if not thermal_data:
            return None

        temp_cmd = thermal_data.get("temp_cmd")
        if not temp_cmd:
            return None

        temperature = None
        try:
            if "PSU" in self.thermal_name:
                temperature = self.__get_psu_temp(temp_cmd)
            elif "ALTITUDE" in self.thermal_name:
                temperature = self.__get_altitude_temp(temp_cmd)
            else:
                status, data = self._api_helper.run_command(temp_cmd)
                if status and data:
                    temperature = float(data) / 1000.0
        except (ValueError, TypeError):
            return None

        if temperature is None:
            return None

        temperature = round(temperature, 1)
        return float(temperature)

    def get_high_threshold(self):
        thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, {})
        threshold = thermal_data.get("high_threshold")
        if threshold is not None:
            return float(threshold)
        return None

    def get_high_critical_threshold(self):
        thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, {})
        threshold = thermal_data.get("high_crit_threshold")
        if threshold is not None:
            return float(threshold)
        return None

    def get_low_threshold(self):
        thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, {})
        threshold = thermal_data.get("low_threshold")
        if threshold is not None:
            return float(threshold)
        return None

    def get_low_critical_threshold(self):
        thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, {})
        threshold = thermal_data.get("low_crit_threshold")
        if threshold is not None:
            return float(threshold)
        return None

    def set_high_threshold(self, temperature):
        return False

    def set_low_threshold(self, temperature):
        return False

    def get_temp_label(self):
        thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, {})
        return thermal_data.get("label", "N/A")
