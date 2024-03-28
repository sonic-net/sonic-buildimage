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
    import os
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

# Global logger class instance
logger = Logger()

MLXBF_BASE_PATH = '/sys/kernel/debug/mlxbf-ptm/monitors/status'

SENSORS = [
    {'name': 'CPU', 'mlxbf_sensor_name': 'core_temp', 'ht': 95, 'cht': 100},
    {'name': 'DDR', 'mlxbf_sensor_name': 'ddr_temp', 'ht': 95, 'cht': 100},
    {'name': 'SFP0', 'iface': 'Ethernet0', 'hwmon_path': None},
    {'name': 'SFP1', 'iface': 'Ethernet4', 'hwmon_path': None},
]

def set_hwmon_path(sensor):
    base = f'/sys/class/net/{sensor["iface"]}/device/hwmon'
    dirs = os.listdir(base)
    if len(dirs) != 1 or not dirs[0].startswith('hwmon'):
        logger.log_error(f'Failed to find hwmon path for {sensor["iface"]}')
        return
    sensor['hwmon_path'] = f'{base}/{dirs[0]}'

def initialize_chassis_thermals():
    thermal_list = []
    for s in SENSORS:
        if 'hwmon_path' in s:
            set_hwmon_path(s)
        thermal_list.append(Thermal(**s))
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
    def __init__(self, name, mlxbf_sensor_name=None, iface=None, hwmon_path=None, ht='N/A', cht='N/A'):
        super(Thermal, self).__init__()
        self.name = name
        self.mlxbf_sensor_name = mlxbf_sensor_name
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
        if self.mlxbf_sensor_name:
            return read_temp_mlxbf(self.mlxbf_sensor_name)
        else:
            return read_temp_hwmon(self.hwmon_path, 'temp1_input')

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
