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
    import smbus2
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

A0 = -2.72435E+02
A1 = 5.31083E-02
A2 = -3.504515E-06
A3 = 1.405146E-10
A4 = -2.299616E-15

SENSORS_THRESHOLD_MAP = {
    "PSU 1 Temp1":  {"high_crit_threshold": 70},
    # "PSU 1 Temp2":  {"high_crit_threshold": 116}, none pddf!
    # "PSU 1 Temp3":  {"high_crit_threshold": 92}, none pddf!
    "PSU 2 Temp1":  {"high_crit_threshold": 70},
    # "PSU 2 Temp2":  {"high_crit_threshold": 116}, none pddf!
    # "PSU 2 Temp3":  {"high_crit_threshold": 92}, none pddf!
    "Switch Chip Internal Temp":  {"high_threshold": 105, "high_crit_threshold": 115},
    "Base Board Temp":  {"high_crit_threshold": 100},
    "Fan Center Temp":  {"high_crit_threshold": 100},
    "Fan Right Temp":  {"high_crit_threshold": 100},
    "Switch Board Front Center Temp":  {"high_crit_threshold": 100},
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
            thermal_data = NONPDDF_THERMAL_SENSORS.get("PSU 1 Temp3", None)
            thermal_data.update({"high_crit_threshold":78})
            thermal_data = NONPDDF_THERMAL_SENSORS.get("PSU 2 Temp3", None)
            thermal_data.update({"high_crit_threshold":78})


class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal, psu_index)
        self._helper = APIHelper()
        self.thermals_psu_index = psu_index

    # Provide the functions/variables below for which implementation is to be overwritten
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

        return super().get_high_critical_threshold()


NONPDDF_THERMAL_SENSORS = {
    "CPU Internal Temp":       { "label": "coretemp-isa-0000", "high_crit_threshold": 99,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/class/hwmon/hwmon1/temp1_input"},
    "Switch board TMP422":     { "label": "tmp422-i2c-8-4c", "high_crit_threshold": 99,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-8/8-004c/hwmon/hwmon*/temp1_input"},                        
    # "DIMM0 Temp":     { "label": "jc42-i2c-31-18", "high_crit_threshold": 85,
    #                     "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-31/31-0018/hwmon/hwmon*/temp1_input"},  ## i2c-imc.c  !!!
    # "DIMM1 Temp":     { "label": "jc42-i2c-31-1a", "high_crit_threshold": 85,
    #                     "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/i2c-31/31-001a/hwmon/hwmon*/temp1_input"},
                                        
    "U61 6C Rail 0": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 7, "address": 0x6c, "rail": 0},
    "U61 6B Rail 0": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 7, "address": 0x6b, "rail": 0},
    "U61 69 Rail 0": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 7, "address": 0x69, "rail": 0},
    "U61 6E Rail 0": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 7, "address": 0x6e, "rail": 0},
    "U61 63 Rail 0": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 7, "address": 0x63, "rail": 0},
    "U61 6C Rail 1": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 7, "address": 0x6c, "rail": 1},
    "U61 6B Rail 1": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 7, "address": 0x6b, "rail": 1},
    "U61 69 Rail 1": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 7, "address": 0x69, "rail": 1},
    "U61 6E Rail 1": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 7, "address": 0x6e, "rail": 1},
    "U61 63 Rail 1": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 7, "address": 0x63, "rail": 1},

    "U66 72 Rail 0": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 11, "address": 0x72, "rail": 0},
    "U66 73 Rail 0": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 11, "address": 0x73, "rail": 0},
    "U66 74 Rail 0": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 11, "address": 0x74, "rail": 0},
    "U66 75 Rail 0": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 11, "address": 0x75, "rail": 0},
    "U66 7A Rail 0": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 11, "address": 0x7a, "rail": 0},
    "U66 72 Rail 1": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 11, "address": 0x72, "rail": 1},
    "U66 73 Rail 1": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 11, "address": 0x73, "rail": 1},
    "U66 74 Rail 1": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 11, "address": 0x74, "rail": 1},
    "U66 75 Rail 1": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 11, "address": 0x75, "rail": 1},
    "U66 7A Rail 1": { "label": "i2c_direct", "high_crit_threshold": 85, "bus": 11, "address": 0x7a, "rail": 1},   

    # "ICP 1 Temp":    { "label": "icp20100-i2c-55-63"},
    # "ICP 2 Temp":    { "label": "icp20100-i2c-56-63"},
    "PSU 1 Temp2":    { "label": "psu-temp-1-2", "high_crit_threshold": 116,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/53-0058/psu_temp2_input"},
    "PSU 1 Temp3":    { "label": "psu-temp-1-3", "high_crit_threshold": 92,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/53-0058/psu_temp3_input"},
    "PSU 2 Temp2":    { "label": "psu-temp-2-2", "high_crit_threshold": 116,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/54-0058/psu_temp2_input"},
    "PSU 2 Temp3":    { "label": "psu-temp-2-3", "high_crit_threshold": 92,
                        "temp_cmd": "awk '{printf $1/1000}' /sys/bus/i2c/devices/54-0058/psu_temp3_input"},
    # "Switch Chip Internal Temp":  { "label": "coretemp-th4", "high_threshold": 105, "high_crit_threshold": 108,
    #                     "temp_cmd": "awk '{printf (((1000000000/$1/40/2-1)*(-0.23734)) + 356.07)}' /sys/devices/platform/fpga_sysfs/temp_TH4_core"}
    }

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
            # cmd = 'i2cdetect -y %d | grep -E " %x|%x "' % (self.bus, self.i2c_slave_addr, self.i2c_slave_addr)
            # print(f"Running command: {cmd}")
            # status, output = self._api_helper.run_command(cmd)
            # if status == True:
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

def read_pmbus_direct(bus, addr, reg):
    # Read word (2 bytes)
    val = bus.read_word_data(addr, reg)
    # PMBus is Little Endian, smbus2 handles conversion
    return val

def get_metrics(bus_num, addr, rail=0):
    try:
        bus = smbus2.SMBus(bus_num)
        bus.write_byte_data(addr, 0x00, rail) # Switch Rail       
        temp_raw = read_pmbus_direct(bus, addr, 0x8D)
        bus.close()
        return temp_raw
    except Exception as e:
        print(f"Error: {e}")
        return 0

class NonPddfThermal(ThermalBase):
    def __init__(self, index, name):
        self.thermal_index = index + 1
        self.thermal_name = name
        self._helper = APIHelper()
        self.is_psu_thermal = False
        self.obj = None

        thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, None)
        if thermal_data != None:
            label = thermal_data.get("label", "N/A")
            if label.startswith("icp20100-i2c"):
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

    # def get_metrics(self, bus_num, addr, rail=0):
    #     # print(f"bus_num: {bus_num}, addr: {addr}, rail: {rail}")
    #     try:
    #         cmd = f'i2cset -y {bus_num} {addr} 0x00 {rail} && i2cget -y {bus_num} {addr} 0x8d'
    #         # print("cmd: ", cmd)
    #         status, data = self._helper.run_command(cmd)
    #         if status == True:
    #             return (int(data, 0))        
    #     except Exception as e:
    #         print(f"Error: {e}")
    #         return 0

    def get_temperature(self):
        thermal_data = NONPDDF_THERMAL_SENSORS.get(self.thermal_name, None)
        if thermal_data == None:
            return None

        label = thermal_data.get("label", "N/A")
        if label.startswith("icp20100-i2c"):
            if self.obj:
                return self.obj.get_temperature()
            else:
                return None
            
        if label == "i2c_direct":
            bus = int(thermal_data.get("bus"))
            address = thermal_data.get("address")
            rail = thermal_data.get("rail")
            temp_raw = get_metrics(bus, address, rail)
            if temp_raw == 0:
                return None
            else:
                return round(float(temp_raw), 2)   
            
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
