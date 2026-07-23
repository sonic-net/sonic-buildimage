#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the thermal management function
#
#############################################################################

try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
    from sonic_platform_base.thermal_base import ThermalBase
    from .helper import APIHelper
    import subprocess
    import syslog   
    import os
    import re
    import os.path  
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SENSORS_THRESHOLD_MAP = {
}

class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal, psu_index)
        self._api_helper = APIHelper()      

    # Provide the functions/variables below for which implementation is to be overwritten
    
    def set_high_threshold(self, temperature):
        return False

    def set_low_threshold(self, temperature):
        return False

    def get_temperature(self):
        if self._api_helper.with_bmc() and self.is_psu_thermal:
            return PddfThermal.get_temperature(self) * 1000
        else:
            return PddfThermal.get_temperature(self)

    def get_high_threshold(self):
        thermal_data = SENSORS_THRESHOLD_MAP.get(self.get_name(), None)
        if thermal_data != None:
            threshold = thermal_data.get("high_threshold", None)
            if threshold != None:
                return (threshold/float(1))
        return super().get_high_threshold()

    def get_high_critical_threshold(self):
        thermal_data = SENSORS_THRESHOLD_MAP.get(self.get_name(), None)
        if thermal_data != None:
            threshold = thermal_data.get("high_crit_threshold", None)
            if threshold != None:
                return (threshold/float(1))
        return super().get_high_critical_threshold()

    def get_temp_label(self):
        label = super().get_temp_label()
        if label == None:
            label = "pddf-sensor"
        return label

storage_max_temp_cmd = " \
m2_list=$(lsblk -S | grep -E 'sata|nvme' | awk '{print $1}') && \
for m2 in $m2_list; do \
   smartctl -a /dev/$m2 | grep -i temp | awk '{print $10}'; \
done | sort -r | head -n1 | awk '{print $1}'"

NONPDDF_THERMAL_SENSORS = {
    "STORAGE_TEMP":   { "label": "storage-max-temperature",
                        "temp_cmd": storage_max_temp_cmd},
    "TH6_CORE_TEMP":  { "label": "coretemp-th5", "high_threshold": 103, "high_crit_threshold": 110,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/devices/platform/cls_sw_fpga/FPGA/TH5_max_temp"},    
    "OSFP_TEMP":      { "label": "osfp-modules-max-temperature", "high_threshold": 71, "high_crit_threshold": 73},
}

class NonPddfThermal(ThermalBase):
    def __init__(self, index, name):
        self.thermal_index = index + 1
        self.thermal_name = name
        self._helper = APIHelper()
        self.is_psu_thermal = False

    def get_name(self):
        return self.thermal_name
    
    def is_replaceable(self):
        if self.thermal_name == 'OSFP_TEMP':
            return True
        else:
            return False

    def get_osfp_max_temperature(self):
        import sonic_platform   
        global platform_chassis

        thermal_data = NONPDDF_THERMAL_SENSORS.get('INLET1', None)
        temp_cmd = thermal_data.get("temp_cmd")
        status, data = self._helper.run_command(temp_cmd)
        air1 = float(data) if status else 0.001
        
        thermal_data = NONPDDF_THERMAL_SENSORS.get('INLET2', None)
        temp_cmd = thermal_data.get("temp_cmd")
        status, data = self._helper.run_command(temp_cmd)
        air2 = float(data) if status else 0.001

        max = min(air1, air2)
        platform_chassis = sonic_platform.platform.Platform().get_chassis()
        _sfp_list = platform_chassis.get_all_sfps()
        for sfp in _sfp_list:
            if sfp.get_presence() and sfp.get_device_type().startswith('OSFP'):
                tmp = sfp.get_temperature()
                if tmp and tmp > max:
                    max = tmp
        return round(float(max), 2)

    def get_temperature(self):
        if self.thermal_name == 'OSFP_TEMP':
            return self.get_osfp_max_temperature()

        thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, None)
        if thermal_data == None:
            return None
        temp_cmd = thermal_data.get("temp_cmd")
        status, data = self._helper.run_command(temp_cmd)
        if status == False:
            return None

        s = data.split('.')
        if len(s) > 2:
            return None
        else:
            for si in s:
                if not si.isdigit():
                    return None
            return round(float(data), 2)

    def get_high_threshold(self):
        thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, None)
        if thermal_data == None:
            return None
        threshold = thermal_data.get("high_threshold", None)
        return (threshold/float(1)) if threshold != None else None

    def get_low_threshold(self):
        thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, None)
        if thermal_data == None:
            return None
        threshold = thermal_data.get("low_threshold", None)
        return (threshold/float(1)) if threshold != None else None

    def get_high_critical_threshold(self):
        thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, None)
        if thermal_data == None:
            return None
        threshold = thermal_data.get("high_crit_threshold", None)
        return (threshold/float(1)) if threshold != None else None

    def get_low_critical_threshold(self):
        thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, None)
        if thermal_data == None:
            return None
        threshold = thermal_data.get("low_crit_threshold", None)
        return (threshold/float(1)) if threshold != None else None

    def set_high_threshold(self, temperature):
        return False

    def set_low_threshold(self, temperature):
        return False

    def get_temp_label(self):
        thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, None)
        if thermal_data == None:
            return "N/A"
        return thermal_data.get("label", "N/A")
