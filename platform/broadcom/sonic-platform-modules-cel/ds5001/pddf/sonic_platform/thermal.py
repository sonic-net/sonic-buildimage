#!/usr/bin/env python
# @Company : Celestica
# @Project : DS5001
# @Time    : 2025/11
# @Author  : Sandy Li

try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
    from sonic_platform_base.thermal_base import ThermalBase
    from . import helper

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


bmc_exist = helper.APIHelper().get_bmc_status()

THERMAL_THRESHOLDS = {
    "TH5_CORE_TEMP": {
        "label": "coretemp-th5",
        "temp_cmd": ['cat', '/sys/bus/platform/drivers/fpga_sysfs/fpga_sysfs/sw_internal_temp'],
        "high_threshold": 105,
        "low_threshold": 'N/A',
        "high_crit_threshold": 110,
        "low_crit_threshold": 'N/A'
        },

    "PSU1_AMBTEMP":{
        "label": "psu_pmbus-i2c-33-58-8d",
        "temp_cmd": ['i2cget', '-y', '-f', '33', '0x58', '0x8d', 'w'],
        "high_threshold": 61,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A',
        "low_crit_threshold": 'N/A'
    },

    "PSU1_PFCTEMP":{
        "label": "psu_pmbus-i2c-33-58-8f",
        "temp_cmd": ['i2cget', '-y','-f', '33', '0x58', '0x8f', 'w'],
        "high_threshold": 115,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A',
        "low_crit_threshold": 'N/A'
    },

    "PSU1_SRTEMP":{
        "label": "psu_pmbus-i2c-33-58-8e",
        "temp_cmd": ['i2cget', '-y','-f', '33', '0x58', '0x8e', 'w'],
        "high_threshold": 120,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A',
        "low_crit_threshold": 'N/A'
    },

    "PSU2_AMBTEMP":{
        "label": "psu_pmbus-i2c-34-59-8d",
        "temp_cmd": ['i2cget', '-y','-f', '34', '0x59', '0x8d', 'w'],
        "high_threshold": 61,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A',
        "low_crit_threshold": 'N/A'
    },

    "PSU2_PFCTEMP":{
        "label": "psu_pmbus-i2c-34-59-8f",
        "temp_cmd": ['i2cget', '-y','-f', '34', '0x59', '0x8f', 'w'],
        "high_threshold": 115,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A',
        "low_crit_threshold": 'N/A'
    },

    "PSU2_SRTEMP":{
        "label": "psu_pmbus-i2c-34-59-8e",
        "temp_cmd": ['i2cget', '-y','-f', '34', '0x59', '0x8e', 'w'],
        "high_threshold": 120,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A',
        "low_crit_threshold": 'N/A'
    },

    "CPU_TEMP":{
        "label": "coretemp-isa-0000",
        "temp_cmd": ["cat", "/sys/class/hwmon/hwmon2/temp1_input"],
        "high_threshold": 105,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A',
        "low_crit_threshold": 'N/A'
    },

    "DIMM0_TEMP":{
        "label": "dimm-temp",
        "temp_cmd": ["cat", "/sys/class/hwmon/hwmon1/temp1_input"],
        "high_threshold": 85,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A',
        "low_crit_threshold": 'N/A'

    },
    "DIMM1_TEMP":{
        "label": "dimm-temp",
        "temp_cmd": ["cat", "/sys/class/hwmon/hwmon1/temp2_input"],
        "high_threshold": 85,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A',
        "low_crit_threshold": 'N/A'
    },

    "XP0R8V_VDDCORE_T": {
        "label": "mp2891-i2c-110-63",
        "temp_cmd": ["cat", "/sys/bus/i2c/devices/i2c-110/110-0063/temp1_input"],
        "high_threshold": 125,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A',
        "low_crit_threshold": 'N/A'
    },

    "XP0R9V_TRVDD0_T": {
        "label": "mp2975-i2c-110-64",
        "temp_cmd": ["cat", "/sys/bus/i2c/devices/i2c-110/110-0064/temp1_input"],
        "high_threshold": 125,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A',
        "low_crit_threshold": 'N/A'
    },

    "XP0R9V_TRVDD1_T": {
        "label": "mp2975-i2c-110-66",
        "temp_cmd": ["cat", "/sys/bus/i2c/devices/i2c-110/110-0066/temp1_input"],
        "high_threshold": 125,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A',
        "low_crit_threshold": 'N/A'
    },
    "XP3R3_LEFT_T": {
        "label": "mp2975-i2c-110-6b",
        "temp_cmd": ["cat", "/sys/bus/i2c/devices/i2c-110/110-006b/temp1_input"],
        "high_threshold": 125,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A',
        "low_crit_threshold": 'N/A'
    },
    "XP3R3_RIGHT_T": {
        "label": "mp2975-i2c-110-6d",
        "temp_cmd": ["cat", "/sys/bus/i2c/devices/i2c-110/110-006d/temp1_input"],
        "high_threshold": 125,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A',
        "low_crit_threshold": 'N/A'
    }

}

TEMP_OFFSET = {
    "TH5_CORE_TEMP":  3
}


class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        self.helper = helper.APIHelper()
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal=is_psu_thermal,
                             psu_index=psu_index)

    def get_high_threshold(self):
        if bmc_exist:
            if not self.is_psu_thermal:
                output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_high_threshold")
                if not output:
                    return None

                if output['status'].isalpha():
                    attr_value = None
                else:
                    attr_value = float(output['status'])

                if output['mode'] == 'bmc':
                    return float(f"{int(attr_value)}.00")
                else:
                    return float(attr_value / 1000)
            else:
                device = "PSU{}".format(self.thermals_psu_index)
                output = self.pddf_obj.get_attr_name_output(device, "psu_temp1_high_threshold")

                try:
                    attr_value = float(output['status'])
                except (TypeError, KeyError, ValueError):
                    return None

                return float(f"{int(attr_value)}.00")
        else:
            if not self.is_psu_thermal:
                thermal_name = self.get_name()
                name = self.plugin_data["THERMAL"]["NONE_BMC"]["temp1_high_threshold"].get(thermal_name)
                if name is not None:
                    value = self.plugin_data["THERMAL"]["NONE_BMC"]["temp1_high_threshold"][thermal_name]
                    return float(value)
                else:
                    return None
            else:
                device = "PSU{}".format(self.thermals_psu_index)
                output = self.pddf_obj.get_attr_name_output(device, "psu_temp1_high_threshold")

                try:
                    temp_high_thresh = float(output['status'])
                except (TypeError, KeyError, ValueError):
                    return None

                return (temp_high_thresh/1000)


    def get_low_threshold(self):
        if not self.is_psu_thermal:
            output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_low_threshold")

            try:
                attr_value = float(output['status'])
            except (TypeError, KeyError, ValueError):
                return None

            if output['mode'] == 'bmc':
                return float(f"{int(attr_value)}.00")
            else:
                return (attr_value/1000)
        else:
            return None

    def get_temperature(self):
        if self.is_psu_thermal:
            device = "PSU{}".format(self.thermals_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, "psu_temp1_input")

            try:
                attr_value = float(output['status'])
            except (TypeError, KeyError, ValueError):
                return None

            if output['mode'] == 'bmc':
                return round((attr_value),2)
            else:
                return (attr_value/1000)

        else:
            output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_input")

            try:
                attr_value = float(output['status'])
            except (TypeError, KeyError, ValueError):
                return None

            if output['mode'] == 'bmc':
                return round((attr_value),2)
            else:
                return (attr_value/1000)

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        try:
            output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_high_crit_threshold")
            attr_value = float(output['status'])
        except (TypeError, KeyError, ValueError):
            return None

        if output['mode'] == 'bmc':
            return float(f"{int(attr_value)}.00")
        else:
            return (attr_value/1000)

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal

        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        try:
            output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_low_crit_threshold")
            attr_value = float(output['status'])
        except (TypeError, KeyError, ValueError):
            return None

        if output['mode'] == 'bmc':
            return float(f"{int(attr_value)}.00")
        else:
            return (attr_value/1000)


class ThermalMon(ThermalBase):
    def __init__(self, index, name):
        self.thermal_index = index + 1
        self.thermal_name = name
        self.helper = helper.APIHelper()

    def get_name(self):
        return self.thermal_name

    def get_presence(self):
        return True

    def get_temperature(self):
        thermal_data = THERMAL_THRESHOLDS.get(self.thermal_name, None)
        if thermal_data == None:
            return None
        temp_cmd = thermal_data.get("temp_cmd")
        status, data = self.helper.run_command(temp_cmd)

        if not status or data is None or data == '':
            return None

        if self.get_name() == "TH5_CORE_TEMP":
            offset = TEMP_OFFSET.get(self.get_name(), 0)
            try:
                data = float(data) + offset
            except ValueError:
                return None

        if "PSU" in self.get_name():
            temp = int(data, 16)
            thermal_temp = self.psu_linear_data(temp)
            return float(thermal_temp)
        else:
            temp = float(data) / 1000.0
            thermal_temp = "{:.1f}".format(temp)
            return float(thermal_temp)

    def get_high_threshold(self):
        thermal_data = THERMAL_THRESHOLDS.get(self.thermal_name, None)
        if thermal_data == None:
            return None
        threshold = thermal_data.get("high_threshold", 'N/A')
        return float(threshold) if threshold != 'N/A' else None

    def get_low_threshold(self):
        thermal_data = THERMAL_THRESHOLDS.get(self.thermal_name, None)
        if thermal_data == None:
            return None
        threshold = thermal_data.get("low_threshold", 'N/A')
        return float(threshold) if threshold != 'N/A' else None

    def get_high_critical_threshold(self):
        thermal_data = THERMAL_THRESHOLDS.get(self.thermal_name, None)
        if thermal_data == None:
            return None
        threshold = thermal_data.get("high_crit_threshold", 'N/A')
        return float(threshold) if threshold != 'N/A' else None

    def get_low_critical_threshold(self):
        thermal_data = THERMAL_THRESHOLDS.get(self.thermal_name, None)
        if thermal_data == None:
            return None
        threshold = thermal_data.get("low_crit_threshold", 'N/A')
        return float(threshold) if threshold != 'N/A' else None

    def psu_linear_data(self, data):
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

    def get_temp_label(self):
        thermal_data = THERMAL_THRESHOLDS.get(self.thermal_name, None)
        if thermal_data == None:
            return None
        return thermal_data.get("label", None)
