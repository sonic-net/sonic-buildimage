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
    from multiprocessing import Lock    
    from .helper import APIHelper
    import subprocess
    import syslog
    import time
    import os
    import re
    import os.path  
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SENSORS_THRESHOLD_MAP = {
    "12V_ENTRY_LEFT":      { "high_threshold": 90.0, "high_crit_threshold": 93.0 },
    "12V_ENTRY_RIGHT":     { "high_threshold": 90.0, "high_crit_threshold": 93.0 },
    "BB_BUSBAR_TEMP":      { "high_threshold": 90.0, "high_crit_threshold": 93.0 },
    "BB_OUTLET_TEMP":      { "high_threshold": 90.0, "high_crit_threshold": 93.0 },
    "TH5_REAR_LEFT":       { "high_threshold": 90.0, "high_crit_threshold": 93.0 },
    "TH5_REAR_RIGHT":      { "high_threshold": 90.0, "high_crit_threshold": 93.0 },
    "PSU 1 TEMP 1":        { "high_threshold": 60.0 },
    "PSU 2 TEMP 1":        { "high_threshold": 60.0 },
    "PSU 3 TEMP 1":        { "high_threshold": 60.0 },
    "PSU 4 TEMP 1":        { "high_threshold": 60.0 },
    "PSU 1 SR TEMP":       { "high_threshold": 60.0 },
    "PSU 2 SR TEMP":       { "high_threshold": 60.0 },
    "PSU 3 SR TEMP":       { "high_threshold": 60.0 },
    "PSU 4 SR TEMP":       { "high_threshold": 60.0 },
    "PSU 1 PFC TEMP":      { "high_threshold": 60.0 },
    "PSU 2 PFC TEMP":      { "high_threshold": 60.0 },
    "PSU 3 PFC TEMP":      { "high_threshold": 60.0 },
    "PSU 4 PFC TEMP":      { "high_threshold": 60.0 },
    "CPU_TEMP":            { "high_threshold": 105.0, "high_crit_threshold": 108.0 },
    "DIMM0_TEMP":          { "high_threshold": 85.0, "high_crit_threshold": 88.0 },
    "DIMM1_TEMP":          { "high_threshold": 85.0, "high_crit_threshold": 88.0 },
    "TH5_CORE_TEMP":       { "high_threshold": 103.0, "high_crit_threshold": 110.0 },
    "XP0R8V_TEMP":         { "high_threshold": 90.0 },
    "XP3R3V_E_TEMP":       { "high_threshold": 90.0 },
    "XP3R3V_W_TEMP":       { "high_threshold": 90.0 },
    "XP0R9V_0_TEMP":       { "high_threshold": 90.0 },
    "XP1R2V_0_TEMP":       { "high_threshold": 90.0 },
    "XP0R9V_1_TEMP":       { "high_threshold": 90.0 },
    "XP1R2V_1_TEMP":       { "high_threshold": 90.0 },
    "XP0R75V_0_TEMP":      { "high_threshold": 90.0 },
    "XP0R75V_1_TEMP":      { "high_threshold": 90.0 }
}

class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal, psu_index)
        self._api_helper = APIHelper()

    # Provide the functions/variables below for which implementation is to be overwritten

    # Using static thresholds as it is not allowed to change the thresholds
    def get_high_threshold(self):
        threshold = None
        thermal_data = SENSORS_THRESHOLD_MAP.get(self.get_name(), None)
        if thermal_data != None:
            threshold = thermal_data.get("high_threshold", None)

        return threshold

    def get_high_critical_threshold(self):
        thermal_data = SENSORS_THRESHOLD_MAP.get(self.get_name(), None)
        if thermal_data != None:
            threshold = thermal_data.get("high_crit_threshold", None)

        return threshold

    def get_low_threshold(self):
        return None

    def get_low_critical_threshold(self):
        return None

    def get_temp_label(self):
        label = super().get_temp_label()
        if label == None:
            label = "pddf-sensor"
        return label

NONPDDF_THERMAL_SENSORS = {
    "CPU_TEMP":       { "label": "coretemp-isa-0000", "high_threshold": 105, "high_crit_threshold": 108,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/class/hwmon/hwmon0/temp1_input"},
    "STORAGE_TEMP":   { "label": "storage-max-temperature"},
    "OSFP_TEMP":      { "label": "osfp-modules-max-temperature", "high_threshold": 71, "high_crit_threshold": 73},
    "INLET1":         { "label": "icp20100-i2c-77-63", "high_threshold": 42, "high_crit_threshold": 45},
    "INLET2":         { "label": "icp20100-i2c-78-63", "high_threshold": 42, "high_crit_threshold": 45},
    "DIMM0_TEMP":     { "label": "jc42-i2c-71-18", "high_threshold": 85, "high_crit_threshold": 88,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-71/71-0018/hwmon/hwmon83/temp1_input"},
    "DIMM1_TEMP":     { "label": "jc42-i2c-71-1a", "high_threshold": 85, "high_crit_threshold": 88,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-71/71-001a/hwmon/hwmon84/temp1_input"},
    "TH5_CORE_TEMP":  { "label": "coretemp-th5", "high_threshold": 103, "high_crit_threshold": 110,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/devices/platform/cls_sw_fpga/FPGA/TH5_max_temp"},
    "XP0R8V_TEMP":    { "label": "raa228228-i2c-103-20", "high_threshold": 90,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-103/103-0020/hwmon/hwmon77/temp1_input"},
    "XP3R3V_E_TEMP":  { "label": "isl68225-i2c-103-60", "high_threshold": 90,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-103/103-0060/hwmon/hwmon78/temp1_input"},
    "XP3R3V_W_TEMP":  { "label": "isl68225-i2c-103-61", "high_threshold": 90,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-103/103-0061/hwmon/hwmon79/temp1_input"},
    "XP0R9V_0_TEMP":  { "label": "isl68222-i2c-103-62", "high_threshold": 90,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-103/103-0062/hwmon/hwmon80/temp1_input"},
    "XP1R2V_0_TEMP":  { "label": "isl68222-i2c-103-62", "high_threshold": 90,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-103/103-0062/hwmon/hwmon80/temp2_input"},
    "XP0R9V_1_TEMP":  { "label": "isl68222-i2c-103-63", "high_threshold": 90,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-103/103-0063/hwmon/hwmon81/temp1_input"},
    "XP1R2V_1_TEMP":  { "label": "isl68222-i2c-103-63", "high_threshold": 90,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-103/103-0063/hwmon/hwmon81/temp2_input"},
    "XP0R75V_0_TEMP": { "label": "isl68225-i2c-103-67", "high_threshold": 90,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-103/103-0067/hwmon/hwmon82/temp1_input"},
    "XP0R75V_1_TEMP": { "label": "isl68225-i2c-103-67", "high_threshold": 90,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-103/103-0067/hwmon/hwmon82/temp2_input"}}

class icp20100():
    def __init__(self, bus=None, i2c_slave_addr=None):
        self._api_helper = APIHelper()
        self.bus = bus
        self.i2c_slave_addr = i2c_slave_addr
        self.ready = False
        self.eeprom_lock = Lock()       
        self.chip_ready()

    def chip_ready(self):
        if self.ready == False:
            cmd = 'i2cdetect -y %d | grep " %x"' % (self.bus, self.i2c_slave_addr)
            status, output = self._api_helper.run_command(cmd)
            if status == False:
                cmd = 'i2cdetect -y %d | grep "%x "' % (self.bus, self.i2c_slave_addr)
                status, output = self._api_helper.run_command(cmd)
                if status == False:
                    return False
            # Continuous mode, temperature only
            cmd = 'i2cset -f -y %d 0x%x 0xc0 0x0d' % (self.bus, self.i2c_slave_addr)
            status, output = self._api_helper.run_command(cmd)
            if status == True:
                time.sleep(0.1)
                self.ready = True
        return self.ready

    def get_temperature(self):
        if self.chip_ready() == False:
            return None

        self.eeprom_lock.acquire()
        # Clear FIFO
        cmd = 'i2cset -f -y %d 0x%x 0xc4 0x80' % (self.bus, self.i2c_slave_addr)
        status, output = self._api_helper.run_command(cmd)
        if status == False:
            self.eeprom_lock.release()      
            return None
        time.sleep(0.1)

        # FIFO data detect
        ready = False
        while (ready != False):
            ret = self._api_helper.i2c_read(self.bus, self.i2c_slave_addr, 0xc4, 1).split(" ")
            if len(ret) != 1:
                self.eeprom_lock.release()
                return None
            if (int(ret[0], 16) & 0xF) != 0x00:
                ready = True
            else:
                time.sleep(0.1)

        # Read first FIFO
        ret = self._api_helper.i2c_read(self.bus, self.i2c_slave_addr, 0xfd, 3).split(" ")
        self.eeprom_lock.release()
        if len(ret) != 3:
            return None
        raw = (int(ret[0], 16)) + (int(ret[1], 16) * 256) + ((int(ret[2], 16) & 0xF) * 65536)

        if raw & 0x80000:
            real = ~(raw - 1) & 0x7ffff
            temp = float(-real) / 262144 * 65 + 25
        else:
            real = raw & 0x7ffff
            temp = float(real) / 262144 * 65 + 25

        return round(float(temp), 2)

class NonPddfThermal(ThermalBase):
    def __init__(self, index, name):
        self.thermal_index = index + 1
        self.thermal_name = name
        self._helper = APIHelper()
        self.is_psu_thermal = False

        if self.thermal_name.startswith('INLET'):
            self.obj = None
            thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, None)
            if thermal_data != None:
                label = thermal_data.get("label", "N/A")
                m = re.search("i2c-(\d+)-(\d+)", label)
                if m:
                    self.obj = icp20100(int(m.group(1)), int('0x' + m.group(2), 16))

    def get_name(self):
        return self.thermal_name
    
    def is_replaceable(self):
        if self.thermal_name == 'OSFP_TEMP':
            return True
        else:
            return False
    
    def get_disk_max_temperature(self):
        disks = []
        temps = []
        status, output = self._helper.run_command('smartctl --scan')
        if status == True:
            for disk in output.split('\n'):
                disks.append(disk.split()[0])
            for disk in disks:
                status, output = self._helper.run_command('smartctl -n standby %s | grep STANDBY' % disk)
                if status == False:
                    status, output = self._helper.run_command('smartctl -A %s | grep Temperature' % disk)
                    temps.append(int(output.split()[9]))
            return round(max(temps), 2)
        else:
            return None

    def get_osfp_max_temperature(self):
        import sonic_platform   
        global platform_chassis
        max = 0.001

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
            
        if self.thermal_name == 'STORAGE_TEMP':
            return self.get_disk_max_temperature()

        if self.thermal_name.startswith('INLET'):
            if self.obj:
                return self.obj.get_temperature()
            else:
                return None

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
