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
    "12V_ENTRY_LEFT":      { "high_threshold": 90, "high_crit_threshold": 93},
    "12V_ENTRY_RIGHT":     { "high_threshold": 90, "high_crit_threshold": 93},
    "BB_BUSBAR_TEMP":      { "high_threshold": 90, "high_crit_threshold": 93},
    "BB_OUTLET_TEMP":      { "high_threshold": 90, "high_crit_threshold": 93},
    "TH5_REAR_LEFT":       { "high_threshold": 90, "high_crit_threshold": 93},
    "TH5_REAR_RIGHT":      { "high_threshold": 90, "high_crit_threshold": 93},
    "PSU 1 Temp1":         { "high_threshold": 60},
    "PSU 2 Temp1":         { "high_threshold": 60},
    "PSU 3 Temp1":         { "high_threshold": 60},
    "PSU 4 Temp1":         { "high_threshold": 60},
    "PSU 1 Temp2":         { "high_threshold": 60},
    "PSU 2 Temp2":         { "high_threshold": 60},
    "PSU 3 Temp2":         { "high_threshold": 60},
    "PSU 4 Temp2":         { "high_threshold": 60},
    "PSU 1 Temp3":         { "high_threshold": 60},
    "PSU 2 Temp3":         { "high_threshold": 60},
    "PSU 3 Temp3":         { "high_threshold": 60},
    "PSU 4 Temp3":         { "high_threshold": 60},
    "PSU 1 Temp4":         { "high_threshold": 60},
    "PSU 2 Temp4":         { "high_threshold": 60},
    "PSU 3 Temp4":         { "high_threshold": 60},
    "PSU 3 Temp4":         { "high_threshold": 60},
    "DIMM0_TEMP":          { "high_threshold": 85, "high_crit_threshold": 88},
    "DIMM1_TEMP":          { "high_threshold": 85, "high_crit_threshold": 88},
    "XP0R8V_TEMP":         { "high_threshold": 90},
    "XP3R3V_E_TEMP":       { "high_threshold": 90},
    "XP3R3V_W_TEMP":       { "high_threshold": 90},
    "XP0R9V_0_TEMP":       { "high_threshold": 90},
    "XP1R2V_0_TEMP":       { "high_threshold": 90},
    "XP0R9V_1_TEMP":       { "high_threshold": 90},
    "XP1R2V_1_TEMP":       { "high_threshold": 90},
    "XP0R75V_0_TEMP":      { "high_threshold": 90},
    "XP0R75V_1_TEMP":      { "high_threshold": 90}}

class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal, psu_index)
        self._api_helper = APIHelper()      

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_name(self):
        if self.is_psu_thermal:
            if 'thermal_name' in self.plugin_data['PSU']:
                return self.plugin_data['PSU']['thermal_name'][str(self.thermals_psu_index)]

            return "PSU{}_TEMP{}".format(self.thermals_psu_index, self.thermal_index)
        else:
            if 'dev_attr' in self.thermal_obj.keys():
                if 'display_name' in self.thermal_obj['dev_attr']:
                    return str(self.thermal_obj['dev_attr']['display_name'])
            # In case of errors
            return (self.thermal_obj_name)    
	
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
    "CPU_TEMP":       { "label": "coretemp-isa-0000", "high_threshold": 105, "high_crit_threshold": 108,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/class/hwmon/hwmon0/temp1_input"},
    "STORAGE_TEMP":   { "label": "storage-max-temperature",
                        "temp_cmd": storage_max_temp_cmd},
    "OSFP_TEMP":      { "label": "osfp-modules-max-temperature", "high_threshold": 71, "high_crit_threshold": 73},
    "INLET1":         { "label": "icp20100-i2c-77-63", "high_threshold": 42, "high_crit_threshold": 45,
                        "temp_cmd": "awk '{printf $1/262144*65+25}' /sys/bus/iio/devices/iio:device0/in_temp_raw"},
    "INLET2":         { "label": "icp20100-i2c-78-63", "high_threshold": 42, "high_crit_threshold": 45,
                        "temp_cmd": "awk '{printf $1/262144*65+25}' /sys/bus/iio/devices/iio:device1/in_temp_raw"},
    "DIMM0_TEMP":     { "label": "jc42-i2c-71-18", "high_threshold": 85, "high_crit_threshold": 88,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-71/71-0018/hwmon/hwmon*/temp1_input"},
    "DIMM1_TEMP":     { "label": "jc42-i2c-71-1a", "high_threshold": 85, "high_crit_threshold": 88,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-71/71-001a/hwmon/hwmon*/temp1_input"},
    "TH5_CORE_TEMP":  { "label": "coretemp-th5", "high_threshold": 103, "high_crit_threshold": 110,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/devices/platform/cls_sw_fpga/FPGA/TH5_max_temp"},
    "XP0R8V_TEMP":    { "label": "raa228228-i2c-103-20", "high_threshold": 90,
                        "temp_cmd": "i2cget -f -y 103 0x20 0x8d | awk '{printf $1/1.0}'"},
    "XP3R3V_E_TEMP":  { "label": "isl68225-i2c-103-60", "high_threshold": 90,
                        "temp_cmd": "i2cget -f -y 103 0x60 0x8d | awk '{printf $1/1.0}'"},
    "XP3R3V_W_TEMP":  { "label": "isl68225-i2c-103-61", "high_threshold": 90,
                        "temp_cmd": "i2cget -f -y 103 0x61 0x8d | awk '{printf $1/1.0}'"},
    "XP0R9V_0_TEMP":  { "label": "isl68222-i2c-103-62", "high_threshold": 90,
                        "temp_cmd": "i2cget -f -y 103 0x62 0x8d | awk '{printf $1/1.0}'"},
    "XP1R2V_0_TEMP":  { "label": "isl68222-i2c-103-62", "high_threshold": 90,
                        "temp_cmd": "i2cget -f -y 103 0x62 0x8e | awk '{printf $1/1.0}'"},
    "XP0R9V_1_TEMP":  { "label": "isl68222-i2c-103-63", "high_threshold": 90,
                        "temp_cmd": "i2cget -f -y 103 0x63 0x8d | awk '{printf $1/1.0}'"},
    "XP1R2V_1_TEMP":  { "label": "isl68222-i2c-103-63", "high_threshold": 90,
                        "temp_cmd": "i2cget -f -y 103 0x63 0x8e | awk '{printf $1/1.0}'"},
    "XP0R75V_0_TEMP": { "label": "isl68225-i2c-103-67", "high_threshold": 90,
                        "temp_cmd": "i2cget -f -y 103 0x67 0x8d | awk '{printf $1/1.0}'"},
    "XP0R75V_1_TEMP": { "label": "isl68225-i2c-103-67", "high_threshold": 90,
                        "temp_cmd": "i2cget -f -y 103 0x67 0x8e | awk '{printf $1/1.0}'"}}

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
            if sfp.get_presence() and sfp.get_device_type().startswith('QSFP-DD'):
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
