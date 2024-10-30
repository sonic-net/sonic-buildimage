#
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

try:
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_py_common.logger import Logger
    from .device_data import DeviceDataManager
    from enum import Enum, auto
    import os
    import json
    import subprocess
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

# Global logger class instance
logger = Logger()

MLXBF_BASE_PATH = '/sys/kernel/debug/mlxbf-ptm/monitors/status'
SSD_DEV='nvme0'

class ThermalType(Enum):
    MLXBF = auto()
    SFP = auto()
    SSD = auto()

MLXBF_SENSORS = [
    {'name': 'CPU', 'thermal_type': ThermalType.MLXBF, 'mlxbf_sensor_name': 'core_temp', 'ht': 95, 'cht': 100},
    {'name': 'DDR', 'thermal_type': ThermalType.MLXBF, 'mlxbf_sensor_name': 'ddr_temp', 'ht': 95, 'cht': 100},
]

def get_hwmon_path(iface):
    base = f'/sys/class/net/{iface}/device/hwmon'
    dirs = os.listdir(base)
    if len(dirs) != 1 or not dirs[0].startswith('hwmon'):
        logger.log_error(f'Failed to find hwmon path for {iface}')
        return
    return f'{base}/{dirs[0]}'

def initialize_sfp_thermals():
    sfps = []
    sfp_count = DeviceDataManager().get_sfp_count()
    for i in range(sfp_count):
        iface = f'Ethernet{i * 4}'
        sfp_thermal = Thermal(name=f'SFP{i}', thermal_type=ThermalType.SFP, hwmon_path=get_hwmon_path(iface))
        sfps.append(sfp_thermal)
    return sfps

def read_smartctl(dev, all=False):
    all_flag = 'u' if all else ''
    cmd = f'smartctl -x /dev/{dev} --json=v{all_flag}'
    try:
        output = subprocess.check_output(cmd.split(' ')).decode().strip()
        return json.loads(output)
    except:
        logger.log_error('Failed to read smartctl output')
        return {}

def read_ssd_temperatrue(dev):
    try:
        return int(read_smartctl(dev)['temperature']['current'])
    except:
        logger.log_error('Failed to read nvme0 temperature')
        return 'N/A'

def read_ssd_thresholds(dev):
    higt_th = None
    crit_th = None
    def parse_value(v):
        return int(v.split(':')[1].strip().split(' ')[0])

    output = read_smartctl(dev, all=True)
    for k,v in output.items():
        if "smartctl" in k:
            if not higt_th and "Warning  Comp. Temp. Threshold" in v:
                higt_th = parse_value(v)
            if not crit_th and "Critical Comp. Temp. Threshold" in v:
                crit_th = parse_value(v)
    return higt_th or 'N/A', crit_th or 'N/A'

def initialize_ssd_thermals():
    higt_th, crit_th = read_ssd_thresholds(SSD_DEV)
    return [Thermal(name=f'NVME', thermal_type=ThermalType.SSD, dev=SSD_DEV, ht=higt_th, cht=crit_th)]

def initialize_chassis_thermals():
    thermal_list = [Thermal(**x) for x in MLXBF_SENSORS]
    thermal_list += initialize_sfp_thermals()
    thermal_list += initialize_ssd_thermals()
    return thermal_list

def read_fs(path, name):
    try:
        with open(path) as f:
            return float(f.readline().strip())
    except Exception as e:
        logger.log_error(f'Failed to read {name} - {str(e)}')
        return 'N/A'

def read_temp_mlxbf(sensor_name):
    path = f'{MLXBF_BASE_PATH}/{sensor_name}'
    return read_fs(path, sensor_name)

def read_temp_hwmon(hwmon_path, sensor):
    if not hwmon_path:
        return 'N/A'
    path = f'{hwmon_path}/{sensor}'
    v = read_fs(path, sensor)
    if v == 'N/A':
        return v
    return v / 1000

class Thermal(ThermalBase):
    def __init__(self, name, thermal_type=None, mlxbf_sensor_name=None, dev=None, hwmon_path=None, ht='N/A', cht='N/A'):
        super(Thermal, self).__init__()
        self.name = name
        self.thermal_type = thermal_type
        self.mlxbf_sensor_name = mlxbf_sensor_name
        self.dev = dev
        self.hwmon_path = hwmon_path
        self.ht = ht
        self.cht = cht

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self.name

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        match self.thermal_type:
            case ThermalType.MLXBF:
                return read_temp_mlxbf(self.mlxbf_sensor_name)
            case ThermalType.SFP:
                return read_temp_hwmon(self.hwmon_path, 'temp1_input')
            case ThermalType.SSD:
                return read_ssd_temperatrue(self.dev)

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.ht

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.hwmon_path:
            return read_temp_hwmon(self.hwmon_path, 'temp1_crit')
        else:
            return self.cht

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return 'N/A'

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal

        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return 'N/A'
