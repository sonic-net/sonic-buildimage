try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

import subprocess

HIGH_THRESHOLD = 0
LOW_THRESHOLD = 1
HIGH_CRIT_THRESHOLD = 2
LOW_CRIT_THRESHOLD = 3

NUM_SENSORS = 4
CPU_SENSOR_STR = "CPU Internal Temp"
thermal_limits = {
    # <sensor-name>: <high_thresh>, <low_thresh>, <high_crit_thresh>, <low_crit_thresh>
    'Front Right Temp': [50.0, None, None, None],
    'Front Left Temp': [50.0, None, None, None],
    'Rear Right Temp': [50.0, None, None, None],
    'ASIC External Temp': [100.0, None, 105.0, None],
    CPU_SENSOR_STR: [88.0, None, 91.0, None]
}

class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        # PDDF doesn't support CPU internal temperature sensor
        # Hence it is created from chassis init override and
        # handled appropriately in thermal APIs
        self.thermal_index = index + 1
        self.is_psu_thermal = is_psu_thermal
        if self.thermal_index <= NUM_SENSORS:
            PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal, psu_index)
    # Provide the functions/variables below for which implementation is to be overwritten

    def get_name(self):
        if self.thermal_index <= NUM_SENSORS:
            return super().get_name()

        return CPU_SENSOR_STR

    def get_temperature(self):
        if self.thermal_index <= NUM_SENSORS:
            return super().get_temperature()

        temperature = 0.0
        cmd = ['cat', '/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp1_input']
        try:
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True)
            data = p.communicate()
            temperature = int(data[0].strip())/1000.0
        except (IOError, ValueError):
            pass

        return temperature

    def get_low_threshold(self):
        thermal_limit = thermal_limits.get(self.get_name(), None)
        if thermal_limit != None:
            return thermal_limit[LOW_THRESHOLD]

        return None

    def __get_psu_high_threshold(self):
        thermal_limit = None
        try:
            cmd = ['i2cget', '-y', '-f', '4', str(0x58 + (self.thermals_psu_index - 1)), '0x51', 'w']
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True)
            data = p.communicate()
            thermal_limit = int(data[0].strip(), 16)
        except (IOError, ValueError):
            pass

        return thermal_limit

    def get_high_threshold(self):
        if self.is_psu_thermal:
            return self.__get_psu_high_threshold()

        thermal_limit = thermal_limits.get(self.get_name(), None)
        if thermal_limit != None:
            return thermal_limit[HIGH_THRESHOLD]

        return None

    def get_low_critical_threshold(self):
        thermal_limit = thermal_limits.get(self.get_name(), None)
        if thermal_limit != None:
            return thermal_limit[LOW_CRIT_THRESHOLD]

        return None

    def get_high_critical_threshold(self):
        thermal_limit = thermal_limits.get(self.get_name(), None)
        if thermal_limit != None:
            return thermal_limit[HIGH_CRIT_THRESHOLD]

        return None

    def set_high_threshold(self, temperature):
        raise NotImplementedError

    def set_low_threshold(self, temperature):
        raise NotImplementedError
