#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2019-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
#############################################################################
# Mellanox
#
# Module contains an implementation of SONiC Platform Base API and
# provides the thermals data which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_py_common.logger import Logger
    import copy
    import glob
    import os
    import re
    import time

    from .device_data import DeviceDataManager
    from . import utils
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

# Global logger class instance
logger = Logger()

"""
The most important information for creating a Thermal object is 3 sysfs files: temperature file, high threshold file and
high critical threshold file. There is no common naming rule for thermal objects on Nvidia platform. There are two types
of thermal object: single and indexable:
    1. Single. Such as asic, port_amb...
    2. Indexablt. Such as cpu_core0, cpu_core1, psu1_temp, psu2_temp

Thermal objects can be created according to a pre-defined naming rule. The naming rules contains following fields

Field Name                Mandatory   Default   Description
name                      M                     Thermal object name template
temperature               M                     Temperature file name
high_threshold            O           None      High threshold file name
high_critical_threshold   O           None      High critical threshold file name
type                      O           single    Thermal object type
start_index               O           1         Thermal object start index, only used by indexable thermal object
"""
THERMAL_NAMING_RULE = {
    "sfp thermals":
    {
        "name": "xSFP module {} Temp",
        "temperature": "module{}_temp_input",
        "high_threshold": "module{}_temp_crit",
        "high_critical_threshold": "module{}_temp_emergency",
        "type": "indexable",
        "allow_delay_create": True
    },
    "psu thermals":
    {
        "name": "PSU-{} Temp",
        "temperature": "psu{}_temp1",
        "high_threshold": "psu{}_temp1_max",
        "type": "indexable"
    },
    "chassis thermals": [
        {
            "name": "ASIC",
            "temperature": "asic",
            "high_threshold": "asic_temp_crit",
            "high_critical_threshold": "asic_temp_emergency",
            "allow_delay_create": True
        },
        {
            "name": "Ambient Port Side Temp",
            "temperature": "port_amb",
            "default_present": True
        },
        {
            "name": "Ambient Fan Side Temp",
            "temperature": "fan_amb",
            "default_present": True
        },
        {
            "name": "Ambient COMEX Temp",
            "temperature": "comex_amb"
        },
        {
            "name": "CPU Pack Temp",
            "temperature": "cpu_pack",
            "high_threshold": "cpu_pack_max",
            "high_critical_threshold": "cpu_pack_crit"
        },
        {
            "name": "CPU Core {} Temp",
            "temperature": "cpu_core{}",
            "high_threshold": "cpu_core{}_max",
            "high_critical_threshold": "cpu_core{}_crit",
            "type": "indexable",
            "start_index": 0
        },
        {
            "name": "Gearbox {} Temp",
            "temperature": "gearbox{}_temp_input",
            "high_threshold": "gearbox{}_temp_emergency",
            "high_critical_threshold": "gearbox{}_temp_trip_crit",
            "type": "indexable"
        },
        {
            "name": "Ambient CPU Board Temp",
            "temperature": "cpu_amb",
            "default_present": False
        },
        {
            "name": "Ambient Switch Board Temp",
            "temperature": "swb_amb",
            "default_present": False
        },
        {
            "name": "PCH Temp",
            "temperature": "pch_temp",
            "default_present": False
        },
        {
            "name": "SODIMM {} Temp",
            "temperature": "sodimm{}_temp_input",
            "high_threshold": "sodimm{}_temp_max",
            "high_critical_threshold": "sodimm{}_temp_crit",
            "search_pattern": '/run/hw-management/thermal/sodimm*_temp_input',
            'index_pattern': r'sodimm(\d+)_temp_input',
            "type": "discrete",
        },
        {
            "name": "PMIC {} Temp",
            "temperature": "voltmon{}_temp1_input",
            "high_threshold": "voltmon{}_temp1_max",
            "high_critical_threshold": "voltmon{}_temp1_crit",
            "search_pattern": '/run/hw-management/thermal/voltmon*_temp1_input',
            'index_pattern': r'voltmon(\d+)_temp1_input',
            "type": "discrete",
        }
    ],
    'linecard thermals': {
        "name": "Gearbox {} Temp",
        "temperature": "gearbox{}_temp_input",
        "high_threshold": "gearbox{}_temp_emergency",
        "high_critical_threshold": "gearbox{}_temp_trip_crit",
        "type": "indexable"
    }
}

CHASSIS_THERMAL_SYSFS_FOLDER = '/run/hw-management/thermal'


def initialize_chassis_thermals():
    thermal_list = []
    rules = THERMAL_NAMING_RULE['chassis thermals']
    position = 1
    for rule in rules:
        thermal_type = rule.get('type')
        if thermal_type == 'indexable':
            count = 0
            if 'Gearbox' in rule['name']:
                count = DeviceDataManager.get_gearbox_count('/run/hw-management/config')
            elif 'CPU Core' in rule['name']:
                count = DeviceDataManager.get_cpu_thermal_count()
            if count == 0:
                logger.log_debug('Failed to get thermal object count for {}'.format(rule['name']))
                continue

            for index in range(count):
                thermal_list.append(create_indexable_thermal(rule, index, CHASSIS_THERMAL_SYSFS_FOLDER, position))
                position += 1
        elif thermal_type == 'discrete':
            discrete_thermals = create_discrete_thermal(rule, position)
            if discrete_thermals:
                position += len(discrete_thermals)
                thermal_list.extend(discrete_thermals)
        else:
            thermal_object = create_single_thermal(rule, CHASSIS_THERMAL_SYSFS_FOLDER, position)
            if thermal_object:
                thermal_list.append(thermal_object)
                position += 1
    return thermal_list


def initialize_psu_thermal(psu_index, presence_cb):
    """Initialize PSU thermal object

    Args:
        psu_index (int): PSU index, 0-based
        presence_cb (function): A callback function to indicate if the thermal is present. When removing a PSU, the related
            thermal sysfs files will be removed from system, presence_cb is used to check such situation and avoid printing
            error logs.

    Returns:
        [list]: A list of thermal objects
    """
    return [create_indexable_thermal(THERMAL_NAMING_RULE['psu thermals'], psu_index, CHASSIS_THERMAL_SYSFS_FOLDER, 1, presence_cb)]


def initialize_sfp_thermal(sfp_index):
    return [create_indexable_thermal(THERMAL_NAMING_RULE['sfp thermals'], sfp_index, CHASSIS_THERMAL_SYSFS_FOLDER, 1)]


def initialize_linecard_thermals(lc_name, lc_index):
    thermal_list = []
    rule = THERMAL_NAMING_RULE['linecard thermals']
    rule = copy.deepcopy(rule)
    rule['name'] = '{} {}'.format(lc_name, rule['name'])
    sysfs_folder = '/run/hw-management/lc{}/thermal'.format(lc_index)
    count = DeviceDataManager.get_gearbox_count('/run/hw-management/lc{}/config'.format(lc_index))
    for index in range(count):
        thermal_list.append(create_indexable_thermal(rule, index, sysfs_folder, index + 1))
    return thermal_list


def initialize_linecard_sfp_thermal(lc_name, lc_index, sfp_index):
    rule = THERMAL_NAMING_RULE['sfp thermals']
    rule = copy.deepcopy(rule)
    rule['name'] = '{} {}'.format(lc_name, rule['name'])
    sysfs_folder = '/run/hw-management/lc{}/thermal'.format(lc_index)
    return [create_indexable_thermal(rule, sfp_index, sysfs_folder, 1)]


def create_indexable_thermal(rule, index, sysfs_folder, position, presence_cb=None):
    index += rule.get('start_index', 1)
    name = rule['name'].format(index)
    temp_file = os.path.join(sysfs_folder, rule['temperature'].format(index))
    allow_delay_create = rule.get('allow_delay_create', False)
    _check_thermal_sysfs_existence(temp_file, presence_cb, allow_delay_create)
    if 'high_threshold' in rule:
        high_th_file = os.path.join(sysfs_folder, rule['high_threshold'].format(index))
        _check_thermal_sysfs_existence(high_th_file, presence_cb, allow_delay_create)
    else:
        high_th_file = None
    if 'high_critical_threshold' in rule:
        high_crit_th_file = os.path.join(sysfs_folder, rule['high_critical_threshold'].format(index))
        _check_thermal_sysfs_existence(high_crit_th_file, presence_cb, allow_delay_create)
    else:
        high_crit_th_file = None
    if not presence_cb:
        return Thermal(name, temp_file, high_th_file, high_crit_th_file, position, allow_delay_create)
    else:
        return RemovableThermal(name, temp_file, high_th_file, high_crit_th_file, position, presence_cb, allow_delay_create)


def create_single_thermal(rule, sysfs_folder, position, presence_cb=None):
    temp_file = rule['temperature']
    default_present = rule.get('default_present', True)
    thermal_capability = DeviceDataManager.get_thermal_capability()

    if thermal_capability:
        if not thermal_capability.get(temp_file, default_present):
            return None
    elif not default_present:
        return None

    temp_file = os.path.join(sysfs_folder, temp_file)
    allow_delay_create = rule.get('allow_delay_create', False)
    _check_thermal_sysfs_existence(temp_file, presence_cb, allow_delay_create)
    if 'high_threshold' in rule:
        high_th_file = os.path.join(sysfs_folder, rule['high_threshold'])
        _check_thermal_sysfs_existence(high_th_file, presence_cb, allow_delay_create)
    else:
        high_th_file = None
    if 'high_critical_threshold' in rule:
        high_crit_th_file = os.path.join(sysfs_folder, rule['high_critical_threshold'])
        _check_thermal_sysfs_existence(high_crit_th_file, presence_cb, allow_delay_create)
    else:
        high_crit_th_file = None
    name = rule['name']
    if not presence_cb:
        return Thermal(name, temp_file, high_th_file, high_crit_th_file, position, allow_delay_create)
    else:
        return RemovableThermal(name, temp_file, high_th_file, high_crit_th_file, position, presence_cb, allow_delay_create)


def create_discrete_thermal(rule, position):
    search_pattern = rule.get('search_pattern')
    index_pattern = rule.get('index_pattern')
    thermal_list = []
    for file_path in glob.iglob(search_pattern):
        file_name = os.path.basename(file_path)
        match = re.search(index_pattern, file_name)
        if not match:
            logger.log_error(f'Failed to extract index from {file_name} using pattern {index_pattern}')
            continue
        index = int(match.group(1))
        thermal_list.append(create_indexable_thermal(rule, index - 1, CHASSIS_THERMAL_SYSFS_FOLDER, position))
        position += 1
    return thermal_list

def _check_thermal_sysfs_existence(file_path, presence_cb, allow_delay_create=False, thermal_obj=None):
    """
    Check if a thermal sysfs file exists and log appropriate messages.
    If thermal_obj is provided and allow_delay_create is True, it will track
    call counts and log warnings for first few attempts, then errors.

    Args:
        file_path (str): Path to the thermal sysfs file
        presence_cb: Presence callback function
        allow_delay_create (bool): Whether to allow delayed creation
        thermal_obj: Thermal object for tracking call counts (optional)

    Returns:
        bool: True if file exists, False otherwise
    """
    if presence_cb:
        status, _ = presence_cb()
        if not status:
            return False

    if os.path.exists(file_path):
        return True

    # If thermal_obj is provided and allow_delay_create is True, use counter-based logging
    if thermal_obj and allow_delay_create:
        # Increment the check count for this file path
        if file_path not in thermal_obj.file_check_counts:
            thermal_obj.file_check_counts[file_path] = 0
        thermal_obj.file_check_counts[file_path] += 1

        # Log warning or error based on attempt count
        if thermal_obj.file_check_counts[file_path] <= thermal_obj.max_attempts:
            logger.log_warning('Thermal sysfs {} does not exist (attempt {})'.format(
                file_path, thermal_obj.file_check_counts[file_path]))
        else:
            logger.log_error('Thermal sysfs {} does not exist (attempt {})'.format(
                file_path, thermal_obj.file_check_counts[file_path]))
    else:
        # Original behavior for initialization-time checks
        if allow_delay_create:
            logger.log_info('Thermal sysfs {} does not exist, wait for thermal obj to be ready'.format(file_path))
        else:
            logger.log_error('Thermal sysfs {} does not exist'.format(file_path))

    return False

class Thermal(ThermalBase):
    def __init__(self, name, temp_file, high_th_file, high_crit_th_file, position, allow_delay_create=False):
        """
        index should be a string for category ambient and int for other categories
        """
        super(Thermal, self).__init__()
        self.name = name
        self.position = position
        self.temperature = temp_file
        self.high_threshold = high_th_file
        self.high_critical_threshold = high_crit_th_file
        self.allow_delay_create = allow_delay_create
        self.max_attempts = 100  # Maximum attempts before logging errors instead of warnings
        self.file_check_counts = {}  # Track check counts for each file path
        self.presence_cb = None  # Presence callback for removable thermal objects

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
        if not _check_thermal_sysfs_existence(self.temperature, self.presence_cb, self.allow_delay_create, self):
            return None

        value = utils.read_float_from_file(self.temperature, None, log_func=logger.log_info)
        return value / 1000.0 if (value is not None and value != 0) else None

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.high_threshold is None:
            return None

        if not _check_thermal_sysfs_existence(self.high_threshold, self.presence_cb, self.allow_delay_create, self):
            return None

        value = utils.read_float_from_file(self.high_threshold, None, log_func=logger.log_info)
        return value / 1000.0 if (value is not None and value != 0) else None

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.high_critical_threshold is None:
            return None

        if not _check_thermal_sysfs_existence(self.high_critical_threshold, self.presence_cb, self.allow_delay_create, self):
            return None

        value = utils.read_float_from_file(self.high_critical_threshold, None, log_func=logger.log_info)
        return value / 1000.0 if (value is not None and value != 0) else None

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return self.position

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False


class RemovableThermal(Thermal):
    def __init__(self, name, temp_file, high_th_file, high_crit_th_file, position, presence_cb, allow_delay_create=False):
        super(RemovableThermal, self).__init__(name, temp_file, high_th_file, high_crit_th_file, position, allow_delay_create)
        self.presence_cb = presence_cb

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        status, hint = self.presence_cb()
        if not status:
            logger.log_debug("get_temperature for {} failed due to {}".format(self.name, hint))
            return None
        return super(RemovableThermal, self).get_temperature()

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        status, hint = self.presence_cb()
        if not status:
            logger.log_debug("get_high_threshold for {} failed due to {}".format(self.name, hint))
            return None
        return super(RemovableThermal, self).get_high_threshold()

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        status, hint = self.presence_cb()
        if not status:
            logger.log_debug("get_high_critical_threshold for {} failed due to {}".format(self.name, hint))
            return None
        return super(RemovableThermal, self).get_high_critical_threshold()
