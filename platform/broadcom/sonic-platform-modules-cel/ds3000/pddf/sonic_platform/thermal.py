try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
    from sonic_platform_base.thermal_base import ThermalBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


SENSORS_LOW_THRESHOLD_MAP = {"Base_Temp_U5": -5, "Base_Temp_U56": -5,
                             "Switch_Temp_U28": -5, "Switch_Temp_U29": -5}

class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal, psu_index)

    # Provide the functions/variables below for which implementation is to be overwritten
    def get_name(self):
        thermal_name = None
        if self.is_psu_thermal and "thermal_name" in self.plugin_data['PSU']:
            thermal_name = self.plugin_data['PSU']['thermal_name'][str(self.thermals_psu_index)]
        return super().get_name() if thermal_name is None else thermal_name

    def get_high_threshold(self):
        if self.is_psu_thermal:
            device = "PSU{}".format(self.thermals_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, "psu_temp1_high_threshold")
            if not output:
                return None

            temp1 = output['status']
            # temperature returned is in milli celcius
            return float(temp1)/1000
        else:
            return super().get_high_threshold()

    def get_low_threshold(self):
        low_threshold = SENSORS_LOW_THRESHOLD_MAP.get(self.get_name(), None)
        if low_threshold != None:
            return low_threshold
        return super().get_low_threshold()


BCM_TEMP_GET_CMD = "cat /sys/devices/platform/fpga_sysfs/bcm_temp"
THERMAL_MONITOR_SENSORS = ["CPU_Temp", "BCM_SW_Temp", "VDD_CORE_Temp", "VDD_ANLG_Temp"]
THERMAL_THRESHOLDS = { "CPU_Temp":      { "high_threshold": 89, "low_threshold": 'N/A', "high_crit_threshold": 93,
                                          "temp_cmd": "r=$(cat /sys/class/thermal/thermal_zone1/temp) && printf '%.1f' $(($r / 1000))" },
                       "BCM_SW_Temp":   { "high_threshold": 110, "low_threshold": 'N/A', "high_crit_threshold": 120,
                                          "temp_cmd": "r=$(cat /sys/devices/platform/fpga_sysfs/bcm_temp) && printf '%.1f' $(((434100 - ((12500000 / $r - 1) * 535) + 5000) / 1000))"},
                       "VDD_CORE_Temp": { "high_threshold": 120, "low_threshold": 'N/A', "high_crit_threshold": 'N/A',
                                          "temp_cmd": "r=$(cat /sys/class/hwmon/hwmon45/temp1_input) && printf '%.1f' $(($r / 1000))" },
                       "VDD_ANLG_Temp": { "high_threshold": 120, "low_threshold": 'N/A', "high_crit_threshold": 'N/A',
                                         "temp_cmd": "r=$(cat /sys/class/hwmon/hwmon44/temp1_input) && printf '%.1f' $(($r / 1000))" }}

class ThermalMon(ThermalBase):
    def __init__(self, index, name):
        self.thermal_index = index + 1
        self.thermal_name = name
        self._helper = APIHelper()

