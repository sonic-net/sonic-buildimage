try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
    from sonic_platform_base.thermal_base import ThermalBase
    from multiprocessing import Lock
    from .helper import APIHelper
    import subprocess
    import syslog
    import os
    import re
    import os.path
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

A0 = -2.72435E+02
A1 = 5.31083E-02
A2 = -3.504515E-06
A3 = 1.405146E-10
A4 = -2.299616E-15

SENSORS_THRESHOLD_MAP = {
    "PSU 1 Temp1":  {"high_threshold": 60},
    "PSU 1 Temp2":  {"high_threshold": 104},
    "PSU 1 Temp3":  {"high_threshold": 80},
    "PSU 2 Temp1":  {"high_threshold": 60},
    "PSU 2 Temp2":  {"high_threshold": 104},
    "PSU 2 Temp3":  {"high_threshold": 80},
    "Base Board Temp":  {"high_threshold": 90, "high_crit_threshold": 100},
    "Fan Center Temp":  {"high_threshold": 90, "high_crit_threshold": 100},
    "Fan Right Temp":  {"high_threshold": 90, "high_crit_threshold": 100},
    "Switch Board Front Center Temp":  {"high_threshold": 90, "high_crit_threshold": 100},
    "XP0R85V Temp":  {"high_threshold": 100, "high_crit_threshold": 125},
    "XP0R9VTRVD0 Temp":  {"high_threshold": 100, "high_crit_threshold": 125},
    "XP0R9VTRVD1 Temp":  {"high_threshold": 100, "high_crit_threshold": 125},
    "Switch Board Front Right Temp":  {"high_threshold": 90, "high_crit_threshold": 100},
    "Switch Board Rear Center Temp":  {"high_threshold": 90, "high_crit_threshold": 100},
    "Switch Board Center Left Temp":  {"high_threshold": 90, "high_crit_threshold": 100},
    "Switch Board Center Right Temp":  {"high_threshold": 90, "high_crit_threshold": 100}
}

class UpdateThermalInfo():
    def __init__(self, airflow="EXHAUST"):
        if airflow == "INTAKE":
            thermal_data = SENSORS_THRESHOLD_MAP.get("Fan Center Temp", None)
            thermal_data.update({"high_threshold":43, "high_crit_threshold":48})
            thermal_data = SENSORS_THRESHOLD_MAP.get("Fan Right Temp", None)
            thermal_data.update({"high_threshold":43, "high_crit_threshold":48})
            thermal_data = NONPDDF_THERMAL_SENSORS.get("Inlet 3 Temp", None)
            thermal_data.update({"high_threshold":90, "high_crit_threshold":100})
            thermal_data = NONPDDF_THERMAL_SENSORS.get("Inlet 4 Temp", None)
            thermal_data.update({"high_threshold":90, "high_crit_threshold":100})
            thermal_data = NONPDDF_THERMAL_SENSORS.get("PSU 1 Temp3", None)
            thermal_data.update({"high_threshold":63})
            thermal_data = NONPDDF_THERMAL_SENSORS.get("PSU 2 Temp3", None)
            thermal_data.update({"high_threshold":63})
            thermal_data = SENSORS_THRESHOLD_MAP.get("PSU 1 Temp3", None)
            thermal_data.update({"high_threshold":63})
            thermal_data = SENSORS_THRESHOLD_MAP.get("PSU 2 Temp3", None)
            thermal_data.update({"high_threshold":63})




class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal, psu_index)
        self._helper = APIHelper()

    # Provide the functions/variables below for which implementation is to be overwritten
    def get_name(self):
        if self.is_psu_thermal:
            return "PSU {} Temp{}".format(self.thermals_psu_index, self.thermal_index)
        else:
            return PddfThermal.get_name(self)

    def set_high_threshold(self, temperature):
        return False

    def set_low_threshold(self, temperature):
        return False

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

        if self.is_psu_thermal:
            device = "PSU{}".format(self.thermals_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, "psu_temp1_high_threshold")
            if not output:
                return None
            temp1 = output['status']
            return float(temp1)/1000
        else:
            return super().get_high_critical_threshold()


NONPDDF_THERMAL_SENSORS = {
    "CPU Internal Temp":       { "label": "coretemp-isa-0000", "high_threshold": 105, "high_crit_threshold": 108,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/class/hwmon/hwmon0/temp1_input"},
    "DIMM0 Temp":     { "label": "jc42-i2c-31-18", "high_threshold": 85, "high_crit_threshold": 88,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-31/31-0018/hwmon/hwmon*/temp1_input"},
    "DIMM1 Temp":     { "label": "jc42-i2c-31-1a", "high_threshold": 85, "high_crit_threshold": 88,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-31/31-001a/hwmon/hwmon*/temp1_input"},
    "Inlet 3 Temp": { "label": "test", "high_threshold": 48, "high_crit_threshold": 50,
                        "temp_cmd": "awk '{printf $1}' /sys/bus/i2c/devices/57-001b/iio:device0/in_voltage0_raw"},
    "Inlet 4 Temp": { "label": "test", "high_threshold": 48, "high_crit_threshold": 50,
                        "temp_cmd": "awk '{printf $1}' /sys/bus/i2c/devices/57-001b/iio:device0/in_voltage1_raw"},
    "Switch Board Front Left Temp": { "label": "icp20100-i2c-57-63", "high_threshold": 90, "high_crit_threshold": 100},
    "Inlet 1 Temp":    { "label": "icp20100-i2c-61-63", "high_threshold": 90, "high_crit_threshold": 100},
    "Inlet 2 Temp":    { "label": "icp20100-i2c-62-63", "high_threshold": 90, "high_crit_threshold": 100},
    "PSU 1 Temp2":    { "label": "psu-temp-1-2", "high_threshold": 104,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/47-0058/psu_temp2_input"},
    "PSU 1 Temp3":    { "label": "psu-temp-1-3", "high_threshold": 80,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/47-0058/psu_temp3_input"},
    "PSU 2 Temp2":    { "label": "psu-temp-2-2", "high_threshold": 104,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/48-0059/psu_temp2_input"},
    "PSU 2 Temp3":    { "label": "psu-temp-2-3", "high_threshold": 80,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/48-0059/psu_temp3_input"},
    "Switch Chip Internal Temp":  { "label": "coretemp-th4", "high_threshold": 105, "high_crit_threshold": 108,
                        "temp_cmd": "awk '{printf (((1000000000/$1/40/2-1)*(-0.23734)) + 356.07)}' /sys/devices/platform/fpga_sysfs/temp_TH4_core"}}

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
            cmd = 'i2cdetect -y %d | grep -E " %x|%x "' % (self.bus, self.i2c_slave_addr, self.i2c_slave_addr)
            status, output = self._api_helper.run_command(cmd)
            if status == True:
                # Continuous mode, temperature only
                cmd = 'i2cset -f -y %d 0x%x 0xc0 0x0d; sleep 0.1' % (self.bus, self.i2c_slave_addr)
                status, output = self._api_helper.run_command(cmd)
                if status == True:
                    self.ready = True
        return self.ready

    def get_temperature(self):
        if self.chip_ready() == False:
            return None

        self.eeprom_lock.acquire()
        # Clear FIFO
        cmd = 'i2cset -f -y %d 0x%x 0xc4 0x80; sleep 0.1' % (self.bus, self.i2c_slave_addr)
        status, output = self._api_helper.run_command(cmd)
        if status == False:
            self.eeprom_lock.release()
            return None

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
        self.obj = None

        if self.thermal_name == "Switch Board Front Left Temp" or self.thermal_name == "Inlet 1 Temp" or self.thermal_name == "Inlet 2 Temp":
            thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, None)
            if thermal_data != None:
                label = thermal_data.get("label", "N/A")
                m = re.search("i2c-(\d+)-(\d+)", label)
                if m:
                    self.obj = icp20100(int(m.group(1)), int('0x' + m.group(2), 16))

    def get_name(self):
        return self.thermal_name

    def calc_ads_temp(self, value):
        adc_val = (value / 4095) * 3.3;
        I_thermal = (3.3 - adc_val) / 10000;
        R_thermal = adc_val / I_thermal;
        temp_adc = (A4 * (R_thermal**4)) +\
                   (A3 * (R_thermal**3)) +\
		   (A2 * (R_thermal**2)) +\
		   (A1 * R_thermal) + A0;       

        return round(temp_adc, 2)

    def get_temperature(self):
        thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, None)
        if thermal_data == None:
            return None

        if self.thermal_name == "Switch Board Front Left Temp" or self.thermal_name == "Inlet 1 Temp" or self.thermal_name == "Inlet 2 Temp":
            if self.obj:
                return self.obj.get_temperature()
            else:
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
                if not (si.startswith('-') and si[1:] or si).isdigit():
                    return None
        
        if self.thermal_name == "Inlet 3 Temp" or self.thermal_name == "Inlet 4 Temp":
            return self.calc_ads_temp(float(data))
        else:
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
        if self.thermal_name.startswith('PSU'):
            label = thermal_data.get("label", "N/A")
            m = re.search("temp-(\d+)-(\d+)", label)
            if m:
                if int(m.group(1)) == 1:
                    cmd = 'cat /sys/bus/i2c/drivers/psu/47-0058/psu_temp%d_high_threshold' % (int(m.group(2)))
                    status, data = self._helper.run_command(cmd)
                    if status == True:
                        return (int(data)/1000)
                elif int(m.group(1)) == 2:
                    cmd = 'cat /sys/bus/i2c/drivers/psu/48-0059/psu_temp%d_high_threshold' % (int(m.group(2)))
                    status, data = self._helper.run_command(cmd)
                    if status == True:
                        return (int(data)/1000)

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
