try:
    import subprocess
    import time
    import threading
    from collections import namedtuple
    import json
    from bfn_extensions.platform_sensors import platform_sensors_get
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_py_common import device_info
    import logging
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

'''
data argument is in "sensors -A -u" format, example:
coretemp-isa-0000
Package id 0:
  temp1_input: 37.000
  temp1_max: 82.000
  temp1_crit: 104.000
  temp1_crit_alarm: 0.000
Core 0:
  temp2_input: 37.000
  ...
'''
Threshold = namedtuple('Threshold', ['crit', 'max', 'min', 'alarm'])


# Thermal -> ThermalBase -> DeviceBase
class Thermal(ThermalBase):
    __sensors_info = None
    __timestamp = 0
    __lock = threading.Lock()
    _thresholds = dict()
    _max_temperature = 100.0
    _min_temperature = 0.0
    _min_high_threshold_temperature = 35.0

    def __init__(self, chip, label, index = 0):
        self.__chip = chip
        self.__label = label
        self.__name = f"{chip}:{label}".lower().replace(' ', '-')
        self.__collect_temp = []
        self.__index = index
        self.__high_threshold = None
        self.__low_threshold = None
        f = None
        try:
            path = device_info.get_path_to_platform_dir() + '/' + 'thermal_thresholds.json'
            f = open(path)
        except FileNotFoundError:
            logging.warning('can not open the file')

        if f is not None:
            self.__get_thresholds(f)

    @staticmethod
    def __sensors_chip_parsed(data: str):
        def kv(line):
            k, v, *_ = [t.strip(': ') for t in line.split(':') if t] + ['']
            return k, v

        chip, *data = data.strip().split('\n')
        chip = chip.strip(': ')

        sensors = []
        for line in data:
            if not line.startswith(' '):
                sensor_label = line.strip(': ')
                sensors.append((sensor_label, {}))
                continue

            if len(sensors) == 0:
                raise RuntimeError(f'invalid data to parse: {data}')

            attr, value = kv(line)
            sensor_label, sensor_data = sensors[-1]
            sensor_data.update({attr: value})

        return chip, dict(sensors)

    @classmethod
    def __sensors_get(cls, cached=True) -> dict:
        cls.__lock.acquire()
        if time.time() > cls.__timestamp + 15:
            # Update cache once per 15 seconds
            try:
                data = platform_sensors_get(['-A', '-u']) or ''
                data += subprocess.check_output("/usr/bin/sensors -A -u",
                                                shell=True, text=True)
                data = data.split('\n\n')
                data = [cls.__sensors_chip_parsed(chip_data) for chip_data in data if chip_data]
                cls.__sensors_info = dict(data)
                cls.__timestamp = time.time()
            except Exception as e:
                logging.warning("Failed to update sensors cache: " + str(e))
        info = cls.__sensors_info
        cls.__lock.release()
        return info

    @staticmethod
    def __sensor_value_get(d: dict, key_prefix, key_suffix=''):
        for k, v in d.items():
            if k.startswith(key_prefix) and k.endswith(key_suffix):
                return v
        return None

    @staticmethod
    def __get_platform_json():
        hwsku_path = device_info.get_path_to_platform_dir()
        platform_json_path = "/".join([hwsku_path, "platform.json"])
        f = open(platform_json_path)
        return json.load(f)

    @staticmethod
    def get_chassis_thermals():
        try:
            platform_json = Thermal.__get_platform_json()
            return platform_json["chassis"]["thermals"]
        except Exception as e:
            logging.exception("Failed to collect chassis thermals: " + str(e))
        return None

    @staticmethod
    def get_psu_thermals(psu_name):
        try:
            platform_json = Thermal.__get_platform_json()
            for psu in platform_json["chassis"]["psus"]:
                if psu["name"] == psu_name:
                    return psu["thermals"]
        except Exception as e:
            logging.exception("Failed to collect chassis thermals: " + str(e))
        return None

    def __get_thresholds(self, f):
        def_threshold_json = json.load(f)
        all_data = def_threshold_json["thermals"]
        for i in all_data:
            for key, value in i.items():
                self._thresholds[key] = Threshold(*value)

    def check_in_range(self, temperature):
        temp_f = float(temperature)
        return temp_f > self._min_temperature and temp_f <= self._max_temperature
    
    def check_high_threshold(self, temperature, attr_suffix):
        temp_f = float(temperature)
        check_range = True
        if attr_suffix == 'max':
            if temp_f < self._min_high_threshold_temperature:
                if self.__name in self._thresholds:
                    temp = self._thresholds[self.__name].max
                    self.set_high_threshold(temp)
                check_range = False
        return check_range

    def __get(self, attr_prefix, attr_suffix):
        chip_data = Thermal.__sensors_get().get(self.__chip, {})
        sensor_data = {}
        for sensor, data in chip_data.items():
            if sensor.lower().replace(' ', '-') == self.__label:
                sensor_data = data
                break
        value = Thermal.__sensor_value_get(sensor_data, attr_prefix, attr_suffix)

        # Can be float value or None
        if attr_prefix == 'temp' and attr_suffix == 'input':
            return value

        if value is not None and self.check_in_range(value) and self.check_high_threshold(value, attr_suffix):
            return value
        elif self.__name in self._thresholds and attr_prefix == 'temp':
            if attr_suffix == 'crit':
                return self._thresholds[self.__name].crit
            elif attr_suffix == 'max':
                if self.__high_threshold is None:
                    return self._thresholds[self.__name].max
                else:
                    return self.__high_threshold
            elif attr_suffix == 'min':
                if self.__low_threshold is None:
                    return self._thresholds[self.__name].min
                else:
                    return self.__low_threshold
            elif attr_suffix == 'alarm':
                return self._thresholds[self.__name].alarm
            else:
                return 1.0
        else:
            return 0.05

    # ThermalBase interface methods:
    def get_temperature(self) -> float:
        temp = self.__get('temp', 'input')
        if temp is None:
            return None
        self.__collect_temp.append(float(temp))
        self.__collect_temp.sort()
        if len(self.__collect_temp) == 3:
            del self.__collect_temp[1]
        return float(temp)

    def get_high_threshold(self) -> float:
        if self.__high_threshold is None:
            return float(self.__get('temp', 'max'))
        return float(self.__high_threshold)

    def get_high_critical_threshold(self) -> float:
        return float(self.__get('temp', 'crit'))

    def get_low_critical_threshold(self) -> float:
        return float(self.__get('temp', 'alarm'))

    def get_model(self):
        return f"{self.__label}".lower()

    # DeviceBase interface methods:
    def get_name(self):
        return self.__name

    def get_presence(self):
        return True

    def get_status(self):
        return True

    def is_replaceable(self):
        return False

    def get_low_threshold(self) -> float:
        if self.__low_threshold is None:
            return float(self.__get('temp', 'min'))
        return float(self.__low_threshold)

    def get_serial(self):
        return 'N/A'

    def get_minimum_recorded(self) -> float:
        temp = self.__collect_temp[0] if len(self.__collect_temp) > 0 else self.get_temperature()
        temp = temp if temp <= 100.0 else 100.0
        temp = temp if temp > 0.0 else 0.1
        return float(temp)

    def get_maximum_recorded(self) -> float:
        temp = self.__collect_temp[-1] if len(self.__collect_temp) > 0 else self.get_temperature()
        temp = temp if temp <= 100.0 else 100.0
        temp = temp if temp > 0.0 else 0.1
        return float(temp)

    def get_position_in_parent(self):
        return self.__index

    def set_high_threshold(self, temperature):
        if self.check_in_range(temperature):
            self.__high_threshold = temperature
            return True
        return False

    def set_low_threshold(self, temperature):
        if self.check_in_range(temperature):
            self.__low_threshold = temperature
            return True
        return False


def chassis_thermals_list_get():
    thermal_list = []
    thermals = Thermal.get_chassis_thermals()
    for index, thermal in enumerate(thermals):
        thermal = thermal["name"].split(':')
        thermal_list.append(Thermal(thermal[0], thermal[1], index))
    return thermal_list

def psu_thermals_list_get(psu_name):
    thermal_list = []
    thermals = Thermal.get_psu_thermals(psu_name)
    for index, thermal in enumerate(thermals):
        thermal = thermal["name"].split(':')
        thermal_list.append(Thermal(thermal[0], thermal[1], index))
    return thermal_list
