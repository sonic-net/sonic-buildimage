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
    "BRD_MID_TSEN":         { "high_threshold": 90, "high_crit_threshold": 100},
    "BRD_LT_TSEN":         { "high_threshold": 90, "high_crit_threshold": 100},
    "BRD_RT_TSEN":         { "high_threshold": 90, "high_crit_threshold": 100},
    "BRD_TH6_TSEN":        { "high_threshold": 90, "high_crit_threshold": 100},
    "OSFP_BOT_TSEN":       { "high_threshold": 90, "high_crit_threshold": 100},
    "OSFP_TOP_TSEN":       { "high_threshold": 90, "high_crit_threshold": 100},
    "DIMM0_TEMP":          { "high_threshold": 85, "high_crit_threshold": 125},
    "DIMM1_TEMP":          { "high_threshold": 85, "high_crit_threshold": 125},
    "TEMP_SNS_L":          { "high_threshold": 90, "high_crit_threshold": 100},
    "TEMP_SNS_R":          { "high_threshold": 90, "high_crit_threshold": 100},
    "ANI0_PCB_MNT":        { "high_threshold": 125},
    "ANI1_PCB_MNT":        { "high_threshold": 125},
    "FMB_FAN1_SEN":        { "high_threshold": 90, "high_crit_threshold": 100},
    "FMB_FAN2_SEN":        { "high_threshold": 90, "high_crit_threshold": 100},
    "FMB_FAN3_SEN":        { "high_threshold": 90, "high_crit_threshold": 100},
    "FMB_FAN4_SEN":        { "high_threshold": 90, "high_crit_threshold": 100},
    "FMB_FAN5_SEN":        { "high_threshold": 90, "high_crit_threshold": 100},
    "FIB_SEN0_R":          {"high_threshold": 90, "high_crit_threshold": 100},
    "FIB_SEN1_R":          {"high_threshold": 90, "high_crit_threshold": 100}
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



"""
TBD

    "INLET_TEMP":     { "label": "icp20100-i2c-77-63", "high_threshold": 42, "high_crit_threshold": 45,
                        "temp_cmd": "awk '{printf $1/262144*65+25}' /sys/bus/iio/devices/iio:device0/in_temp_raw"}, 
    "INLET1_TEMP":     { "label": "icp20100-i2c-77-63", "high_threshold": 42, "high_crit_threshold": 45,
                        "temp_cmd": "awk '{printf $1/262144*65+25}' /sys/bus/iio/devices/iio:device0/in_temp_raw"},
    "INLET2_TEMP":      { "label": "icp20100-i2c-78-63", "high_threshold": 42, "high_crit_threshold": 45,
                        "temp_cmd": "awk '{printf $1/262144*65+25}' /sys/bus/iio/devices/iio:device1/in_temp_raw"},
    "DIMM0_TEMP":     { "label": "jc42-i2c-71-18", "high_threshold": 85, "high_crit_threshold": 88,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/class/hwmon/hwmon85/temp1_input"},
    "DIMM1_TEMP":     { "label": "jc42-i2c-71-1a", "high_threshold": 85, "high_crit_threshold": 88,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/class/hwmon/hwmon86/temp1_input"},
    "FSC_TH6":         { "label": "coretemp-th5", "high_threshold": 103, "high_crit_threshold": 110,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/devices/platform/cls_sw_fpga/FPGA/TH5_max_temp"},
"""
NONPDDF_THERMAL_SENSORS = {
    "STORAGE_TEMP":   { "label": "storage-max-temperature",
                        "temp_cmd": storage_max_temp_cmd},
    "CPU_TEMP":       { "label": "coretemp-isa-0000", "high_threshold": 89, "high_crit_threshold": 99,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/class/hwmon/hwmon0/temp1_input"},
    "OSFP_TEMP":      { "label": "osfp-modules-max-temperature"},
    "FMB_FAN1_TSEN":    { "label": "tmp116-i2c-131-48",
                        "temp_cmd": "awk '{printf $1*625/80000}' /sys/bus/iio/devices/iio:device0/in_temp_raw"},
    "FMB_FAN2_TSEN":  { "label": "tmp116-i2c-132-48",
                        "temp_cmd": "awk '{printf $1*625/80000}' /sys/bus/iio/devices/iio:device1/in_temp_raw"},
    "FMB_FAN3_TSEN":  { "label": "tmp116-i2c-133-48",
                        "temp_cmd": "awk '{printf $1*625/80000}' /sys/bus/iio/devices/iio:device2/in_temp_raw"},
    "FMB_FAN4_TSEN":  { "label": "tmp116-i2c-134-48",
                        "temp_cmd": "awk '{printf $1*625/80000}' /sys/bus/iio/devices/iio:device3/in_temp_raw"},
    "FMB_FAN5_TSEN":  { "label": "tmp116-i2c-135-48",
                        "temp_cmd": "awk '{printf $1*625/80000}' /sys/bus/iio/devices/iio:device4/in_temp_raw"},
    "ANI0_PCB_MNT":   {"label": "ads1015-i2c-126-0048-0"},
    "ANI1_PCB_MNT":   {"label": "ads1015-i2c-126-0048-1"}
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
        """
        thermal_data = NONPDDF_THERMAL_SENSORS.get('INLET1', None)
        temp_cmd = thermal_data.get("temp_cmd")
        status, data = self._helper.run_command(temp_cmd)
        air1 = float(data) if status else 0.001
        
        thermal_data = NONPDDF_THERMAL_SENSORS.get('INLET2', None)
        temp_cmd = thermal_data.get("temp_cmd")
        status, data = self._helper.run_command(temp_cmd)
        air2 = float(data) if status else 0.001

        max = min(air1, air2)
        """
        max = 0
        platform_chassis = sonic_platform.platform.Platform().get_chassis()
        _sfp_list = platform_chassis.get_all_sfps()
        for sfp in _sfp_list:
            if sfp.get_presence() and sfp.get_device_type().startswith('OSFP'):
                tmp = sfp.get_temperature()
                if tmp and tmp > max:
                    max = tmp
        return round(float(max), 2)

    def get_thermistor_temp(self, device_path, channel):
        raw_file = os.path.join(device_path, f"in_voltage{channel}_raw")
        scale_file = os.path.join(device_path, f"in_voltage{channel}_scale")

        try:
            with open(raw_file, 'r') as f:
                raw_val = float(f.read().strip())

            with open(scale_file, 'r') as f:
                scale_val = float(f.read().strip())
        except Exception as e:
            return None
        ADC = (raw_val * scale_val) / 1000.0
        temp = -5.475344e+02 + ADC * (7.507511e+02 + ADC * (-4.340839e+02 + ADC * (1.371151e+02 + ADC * (-1.345040e+01))))
        return round(float(temp), 2)

    def get_ani_temp(self):
        device_path = "/sys/bus/i2c/drivers/ads1015/126-0048/iio:device5"
        channel = 0 if self.thermal_name == "ANI0_PCB_MNT" else 1
        return self.get_thermistor_temp(device_path, channel)

    def get_temperature(self):
        if self.thermal_name == 'OSFP_TEMP':
            return self.get_osfp_max_temperature()
        elif (self.thermal_name == 'ANI0_PCB_MNT' or self.thermal_name == 'ANI1_PCB_MNT'):
            return self.get_ani_temp()

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
        thermal_data = SENSORS_THRESHOLD_MAP.get(self.get_name(), None)
        if thermal_data != None:
            threshold = thermal_data.get("high_threshold", None)
            if threshold != None:
                return (threshold/float(1))
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
        thermal_data = SENSORS_THRESHOLD_MAP.get(self.get_name(), None)
        if thermal_data != None:
            threshold = thermal_data.get("high_crit_threshold", None)
            if threshold != None:
                return (threshold/float(1))
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
