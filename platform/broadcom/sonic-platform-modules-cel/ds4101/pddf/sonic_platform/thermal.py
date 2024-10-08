#!/usr/bin/env python
# @Company ：Celestica
# @Time    : 2023/3/6 09:32
# @Mail    : yajiang@celestica.com
# @Author  : jiang tao

try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
    from sonic_platform_base.thermal_base import ThermalBase
    from . import helper

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


bcm_exist = helper.APIHelper().get_bmc_status()

THERMAL_THRESHOLDS = {
    "TEMP_CPU": {
        "temp_cmd": ["cat", "/sys/class/thermal/thermal_zone0/temp"],
        "high_threshold": 105,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A'
        },
    "PSU1_TEMP2":{
        "temp_cmd": ["i2cget", "-y", "-f", "47", "0x58", "0x8e"],
        "high_threshold": 79,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A'
        },
    "PSU1_TEMP3":{
        "temp_cmd": ["i2cget", "-y", "-f", "47", "0x58", "0x8f"],
        "high_threshold": 78,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A'
        },
    "PSU2_TEMP2":{
        "temp_cmd": ["i2cget", "-y", "-f", "48", "0x59", "0x8e"],
        "high_threshold": 79,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A'
        },
    "PSU2_TEMP3":{
        "temp_cmd": ["i2cget", "-y", "-f", "48", "0x59", "0x8f"],
        "high_threshold": 78,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A'
        },
    "TEMP_SW_Internal": {
        "temp_cmd": ["cat", "/sys/bus/platform/drivers/fpga_sysfs/fpga_sysfs/sw_internal_temp"],
        "high_threshold": 105,
        "low_threshold": 'N/A',
        "high_crit_threshold": 110
        },
    "TEMP_DIMMA0": {
        "temp_cmd": ["cat", "/sys/bus/i2c/devices/71-001a/hwmon/hwmon48/temp1_input"],
        "high_threshold": 85,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A'
        },
    "TEMP_DIMMB0": {
        "temp_cmd": ["cat", "/sys/bus/i2c/devices/71-0018/hwmon/hwmon47/temp1_input"],
        "high_threshold": 85,
        "low_threshold": 'N/A',
        "high_crit_threshold": 'N/A'
        }
}

class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        self.helper = helper.APIHelper()
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal=is_psu_thermal,
                             psu_index=psu_index)

    def get_temperature(self):
        """
        Rewrite the method of obtaining temperature in pddf_thermal
        Avoid changing the value to 0 when the psu temperature value obtained 
        from BMC is character 'na'

        Returns:
            A float number, the temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.is_psu_thermal:
            device = "PSU{}".format(self.thermals_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, "psu_temp1_input")
            if not output:
                return None

            if output['status'].isalpha():
                attr_value = None
            else:
                attr_value = float(output['status'])

            if output['mode'] == 'bmc':
                return attr_value
            else:
                return (attr_value/float(1000))
        else:
            output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_input")
            if not output:
                return None

            if output['status'].isalpha():
                attr_value = None
            else:
                attr_value = float(output['status'])

            if output['mode'] == 'bmc':
                return attr_value
            else:
                return (attr_value/float(1000))

    def get_high_threshold(self):
        if bcm_exist:
            if not self.is_psu_thermal:
                output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_high_threshold")
                if not output:
                    return None

                if output['status'].isalpha():
                    attr_value = None
                else:
                    attr_value = float(output['status'])

                if output['mode'] == 'bmc':
                    return attr_value
                else:
                    return float(attr_value / 1000)
            else:
                if self.thermals_psu_index == 1:
                    cmd = ["ipmitool", "raw", "0x04", "0x27", "0x29"]
                else:
                    cmd = ["ipmitool", "raw", "0x04", "0x27", "0x33"]
                status, result = self.helper.run_command(cmd)
                if not status or result is None or result == '':
                    return None
                value = int(result.split()[5], 16)
                return float(value)
        else:
            thermal_name = self.get_name()
            name = self.plugin_data["THERMAL"]["NONE_BMC"]["temp1_high_threshold"].get(thermal_name)
            if name is not None: 
                value = self.plugin_data["THERMAL"]["NONE_BMC"]["temp1_high_threshold"][thermal_name]
                return float(value) 
            else:
                return None


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
            return 'N/A'
        temp_cmd = thermal_data.get("temp_cmd")
        status, data = self.helper.run_command(temp_cmd)
        if not status or data is None or data == '':
            return 'N/A'
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
            return 'N/A'
        threshold = thermal_data.get("high_threshold", 'N/A')
        return float(threshold) if threshold != 'N/A' else 'N/A'

    def get_low_threshold(self):
        thermal_data = THERMAL_THRESHOLDS.get(self.thermal_name, None)
        if thermal_data == None:
            return 'N/A'
        threshold = thermal_data.get("low_threshold", 'N/A')
        return float(threshold) if threshold != 'N/A' else 'N/A'

    def get_high_critical_threshold(self):
        thermal_data = THERMAL_THRESHOLDS.get(self.thermal_name, None)
        if thermal_data == None:
            return 'N/A'
        threshold = thermal_data.get("high_crit_threshold", 'N/A')
        return float(threshold) if threshold != 'N/A' else 'N/A'

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