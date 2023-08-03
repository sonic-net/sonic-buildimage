#
# Copyright (c) 2021-2023 NVIDIA CORPORATION & AFFILIATES.
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

import glob
import os
import pytest
import sys
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

import sonic_platform.chassis
from sonic_platform.chassis import Chassis
from sonic_platform.device_data import DeviceDataManager

sonic_platform.chassis.extract_RJ45_ports_index = mock.MagicMock(return_value=[])

class TestThermal:
    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_gearbox_count', mock.MagicMock(return_value=2))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_cpu_thermal_count', mock.MagicMock(return_value=2))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_sodimm_thermal_count', mock.MagicMock(return_value=2))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_name', mock.MagicMock(return_value='x86_64-mlnx_msn2700-r0'))
    def test_chassis_thermal(self):
        from sonic_platform.thermal import THERMAL_NAMING_RULE
        chassis = Chassis()
        thermal_list = chassis.get_all_thermals()
        assert thermal_list
        thermal_dict = {thermal.get_name(): thermal for thermal in thermal_list}
        gearbox_thermal_rule = None
        cpu_thermal_rule = None
        sodimm_thermal_rule = None
        for rule in THERMAL_NAMING_RULE['chassis thermals']:
            thermal_type = rule.get('type', 'single')
            if thermal_type == 'single':
                thermal_name = rule['name']
                if rule['temperature'] == 'comex_amb':
                    assert thermal_name not in thermal_dict
                    continue
                default_present = rule.get('default_present', True)
                if not default_present:
                    assert thermal_name not in thermal_dict
                    continue
                assert thermal_name in thermal_dict
                thermal = thermal_dict[thermal_name]
                assert rule['temperature'] in thermal.temperature
                assert rule['high_threshold'] in thermal.high_threshold if 'high_threshold' in rule else thermal.high_threshold is None
                assert rule['high_critical_threshold'] in thermal.high_critical_threshold if 'high_critical_threshold' in rule else thermal.high_critical_threshold is None
            else:
                if 'Gearbox' in rule['name']:
                    gearbox_thermal_rule = rule
                elif 'CPU Core' in rule['name']:
                    cpu_thermal_rule = rule
                elif 'SODIMM' in rule['name']:
                    sodimm_thermal_rule = rule

        gearbox_thermal_count = 0
        cpu_thermal_count = 0
        sodimm_thermal_count = 0
        for thermal in thermal_list:
            if 'Gearbox' in thermal.get_name():
                start_index = gearbox_thermal_rule.get('start_index', 1)
                start_index += gearbox_thermal_count
                assert thermal.get_name() == gearbox_thermal_rule['name'].format(start_index)
                assert gearbox_thermal_rule['temperature'].format(start_index) in thermal.temperature
                assert gearbox_thermal_rule['high_threshold'].format(start_index) in thermal.high_threshold
                assert gearbox_thermal_rule['high_critical_threshold'].format(start_index) in thermal.high_critical_threshold
                gearbox_thermal_count += 1
            elif 'CPU Core' in thermal.get_name():
                start_index = cpu_thermal_rule.get('start_index', 1)
                start_index += cpu_thermal_count
                assert thermal.get_name() == cpu_thermal_rule['name'].format(start_index)
                assert cpu_thermal_rule['temperature'].format(start_index) in thermal.temperature
                assert cpu_thermal_rule['high_threshold'].format(start_index) in thermal.high_threshold
                assert cpu_thermal_rule['high_critical_threshold'].format(start_index) in thermal.high_critical_threshold
                cpu_thermal_count += 1
            elif 'SODIMM' in thermal.get_name():
                start_index = sodimm_thermal_rule.get('start_index', 1)
                start_index += sodimm_thermal_count
                assert thermal.get_name() == sodimm_thermal_rule['name'].format(start_index)
                assert sodimm_thermal_rule['temperature'].format(start_index) in thermal.temperature
                assert sodimm_thermal_rule['high_threshold'].format(start_index) in thermal.high_threshold
                assert sodimm_thermal_rule['high_critical_threshold'].format(start_index) in thermal.high_critical_threshold
                sodimm_thermal_count += 1

        assert gearbox_thermal_count == 2
        assert cpu_thermal_count == 2
        assert sodimm_thermal_count == 2

    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_name', mock.MagicMock(return_value='x86_64-nvidia_sn2201-r0'))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_thermal_capability')
    def test_chassis_thermal_includes(self, mock_capability):
        from sonic_platform.thermal import THERMAL_NAMING_RULE
        thermal_capability = {'comex_amb': False, 'cpu_amb': True, 'swb_amb': True}
        mock_capability.return_value = thermal_capability
        chassis = Chassis()
        thermal_list = chassis.get_all_thermals()
        assert thermal_list
        thermal_dict = {thermal.get_name(): thermal for thermal in thermal_list}
        for rule in THERMAL_NAMING_RULE['chassis thermals']:
            default_present = rule.get('default_present', True)
            if not default_present and thermal_capability.get(rule['temperature']):
                thermal_name = rule['name']
                assert thermal_name in thermal_dict

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_psu_thermal(self):
        from sonic_platform.thermal import initialize_psu_thermal, THERMAL_NAMING_RULE
        presence_cb = mock.MagicMock(return_value=(True, ''))
        thermal_list = initialize_psu_thermal(0, presence_cb)
        assert len(thermal_list) == 1
        thermal = thermal_list[0]
        rule = THERMAL_NAMING_RULE['psu thermals']
        start_index = rule.get('start_index', 1)
        assert thermal.get_name() == rule['name'].format(start_index)
        assert rule['temperature'].format(start_index) in thermal.temperature
        assert rule['high_threshold'].format(start_index) in thermal.high_threshold
        assert thermal.high_critical_threshold is None
        assert thermal.get_position_in_parent() == 1
        assert thermal.is_replaceable() == False

        presence_cb = mock.MagicMock(return_value=(False, 'Not present'))
        thermal_list = initialize_psu_thermal(0, presence_cb)
        assert len(thermal_list) == 1
        thermal = thermal_list[0]
        assert thermal.get_temperature() is None
        assert thermal.get_high_threshold() is None
        assert thermal.get_high_critical_threshold() is None

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_sfp_thermal(self):
        from sonic_platform.thermal import initialize_sfp_thermal, THERMAL_NAMING_RULE
        thermal_list = initialize_sfp_thermal(0)
        assert len(thermal_list) == 1
        thermal = thermal_list[0]
        rule = THERMAL_NAMING_RULE['sfp thermals']
        start_index = rule.get('start_index', 1)
        assert thermal.get_name() == rule['name'].format(start_index)
        assert rule['temperature'].format(start_index) in thermal.temperature
        assert rule['high_threshold'].format(start_index) in thermal.high_threshold
        assert rule['high_critical_threshold'].format(start_index) in thermal.high_critical_threshold
        assert thermal.get_position_in_parent() == 1
        assert thermal.is_replaceable() == False

    @mock.patch('sonic_platform.utils.read_float_from_file')
    def test_get_temperature(self, mock_read):
        from sonic_platform.thermal import Thermal
        thermal = Thermal('test', 'temp_file', None, None, 1)
        mock_read.return_value = 35727
        assert thermal.get_temperature() == 35.727

        mock_read.return_value = 0.0
        assert thermal.get_temperature() is None

        mock_read.return_value = None
        assert thermal.get_temperature() is None

    @mock.patch('sonic_platform.utils.read_float_from_file')
    def test_get_high_threshold(self, mock_read):
        from sonic_platform.thermal import Thermal
        thermal = Thermal('test', None, None, None, 1)
        assert thermal.get_high_threshold() is None

        thermal.high_threshold = 'high_th_file'
        mock_read.return_value = 25833
        assert thermal.get_temperature() == 25.833

        mock_read.return_value = 0.0
        assert thermal.get_temperature() is None

        mock_read.return_value = None
        assert thermal.get_temperature() is None

    @mock.patch('sonic_platform.utils.read_float_from_file')
    def test_get_high_critical_threshold(self, mock_read):
        from sonic_platform.thermal import Thermal
        thermal = Thermal('test', None, None, None, 1)
        assert thermal.get_high_critical_threshold() is None

        thermal.high_critical_threshold = 'high_th_file'
        mock_read.return_value = 120839
        assert thermal.get_high_critical_threshold() == 120.839

        mock_read.return_value = 0.0
        assert thermal.get_high_critical_threshold() is None

        mock_read.return_value = None
        assert thermal.get_high_critical_threshold() is None

    @mock.patch('glob.iglob', mock.MagicMock(return_value=['thermal_zone1', 'thermal_zone2']))
    @mock.patch('sonic_platform.utils.write_file')
    def test_set_thermal_algorithm_status(self, mock_write):
        from sonic_platform.thermal import Thermal, THERMAL_ZONE_FOLDER_WILDCARD, THERMAL_ZONE_POLICY_FILE, THERMAL_ZONE_MODE_FILE
        assert Thermal.set_thermal_algorithm_status(True, False)

        for folder in glob.iglob(THERMAL_ZONE_FOLDER_WILDCARD):
            mock_write.assert_any_call(os.path.join(folder, THERMAL_ZONE_POLICY_FILE), 'step_wise')
            mock_write.assert_any_call(os.path.join(folder, THERMAL_ZONE_MODE_FILE), 'enabled')

        assert Thermal.set_thermal_algorithm_status(False, False)
        for folder in glob.iglob(THERMAL_ZONE_FOLDER_WILDCARD):
            mock_write.assert_any_call(os.path.join(folder, THERMAL_ZONE_POLICY_FILE), 'user_space')
            mock_write.assert_any_call(os.path.join(folder, THERMAL_ZONE_MODE_FILE), 'disabled')

        assert not Thermal.set_thermal_algorithm_status(False, False)

        assert Thermal.set_thermal_algorithm_status(False)

    @mock.patch('glob.iglob', mock.MagicMock(return_value=['thermal_zone1', 'thermal_zone2']))
    @mock.patch('sonic_platform.utils.read_int_from_file')
    def test_get_min_allowed_cooling_level_by_thermal_zone(self, mock_read_file):
        from sonic_platform.thermal import Thermal, THERMAL_ZONE_TEMP_FILE, THERMAL_ZONE_HIGH_THRESHOLD, THERMAL_ZONE_NORMAL_THRESHOLD, MIN_COOLING_LEVEL_FOR_HIGH, MIN_COOLING_LEVEL_FOR_NORMAL
        mock_read_file.side_effect = Exception('')
        assert Thermal.get_min_allowed_cooling_level_by_thermal_zone() is None

        mock_file_content = {}
        def mock_read_int_from_file(file_path, default=0, raise_exception=False):
            return mock_file_content[file_path]

        mock_read_file.side_effect = mock_read_int_from_file
        mock_file_content[os.path.join('thermal_zone1', THERMAL_ZONE_NORMAL_THRESHOLD)] = 75000
        mock_file_content[os.path.join('thermal_zone1', THERMAL_ZONE_HIGH_THRESHOLD)] = 85000
        mock_file_content[os.path.join('thermal_zone1', THERMAL_ZONE_TEMP_FILE)] = 69000
        mock_file_content[os.path.join('thermal_zone2', THERMAL_ZONE_NORMAL_THRESHOLD)] = 75000
        mock_file_content[os.path.join('thermal_zone1', THERMAL_ZONE_HIGH_THRESHOLD)] = 85000
        mock_file_content[os.path.join('thermal_zone2', THERMAL_ZONE_TEMP_FILE)] = 24000
        assert Thermal.get_min_allowed_cooling_level_by_thermal_zone() == MIN_COOLING_LEVEL_FOR_NORMAL

        mock_file_content[os.path.join('thermal_zone1', THERMAL_ZONE_TEMP_FILE)] = 71000
        assert Thermal.get_min_allowed_cooling_level_by_thermal_zone() == MIN_COOLING_LEVEL_FOR_HIGH

        mock_file_content[os.path.join('thermal_zone1', THERMAL_ZONE_TEMP_FILE)] = 79000
        assert Thermal.get_min_allowed_cooling_level_by_thermal_zone() == MIN_COOLING_LEVEL_FOR_HIGH

        mock_file_content[os.path.join('thermal_zone1', THERMAL_ZONE_TEMP_FILE)] = 81000
        assert Thermal.get_min_allowed_cooling_level_by_thermal_zone() is None

    @mock.patch('glob.iglob', mock.MagicMock(return_value=['thermal_zone1', 'thermal_zone2']))
    @mock.patch('sonic_platform.utils.read_int_from_file')
    def test_no_sensor_thermal_zone(self, mock_read_file):
        from sonic_platform.thermal import Thermal, THERMAL_ZONE_TEMP_FILE, THERMAL_ZONE_HIGH_THRESHOLD, THERMAL_ZONE_NORMAL_THRESHOLD, MIN_COOLING_LEVEL_FOR_HIGH, MIN_COOLING_LEVEL_FOR_NORMAL

        mock_file_content = {}
        def mock_read_int_from_file(file_path, **kwargs):
            return mock_file_content[file_path]

        mock_read_file.side_effect = mock_read_int_from_file
        mock_file_content[os.path.join('thermal_zone1', THERMAL_ZONE_NORMAL_THRESHOLD)] = 0
        mock_file_content[os.path.join('thermal_zone1', THERMAL_ZONE_HIGH_THRESHOLD)] = 0
        mock_file_content[os.path.join('thermal_zone1', THERMAL_ZONE_TEMP_FILE)] = 0
        mock_file_content[os.path.join('thermal_zone2', THERMAL_ZONE_NORMAL_THRESHOLD)] = 75000
        mock_file_content[os.path.join('thermal_zone2', THERMAL_ZONE_HIGH_THRESHOLD)] = 85000
        mock_file_content[os.path.join('thermal_zone2', THERMAL_ZONE_TEMP_FILE)] = 24000
        assert Thermal.get_min_allowed_cooling_level_by_thermal_zone() == MIN_COOLING_LEVEL_FOR_NORMAL

        mock_file_content[os.path.join('thermal_zone2', THERMAL_ZONE_TEMP_FILE)] = 71000
        assert Thermal.get_min_allowed_cooling_level_by_thermal_zone() == MIN_COOLING_LEVEL_FOR_HIGH

        mock_file_content[os.path.join('thermal_zone2', THERMAL_ZONE_TEMP_FILE)] = 79000
        assert Thermal.get_min_allowed_cooling_level_by_thermal_zone() == MIN_COOLING_LEVEL_FOR_HIGH

        mock_file_content[os.path.join('thermal_zone2', THERMAL_ZONE_TEMP_FILE)] = 81000
        assert Thermal.get_min_allowed_cooling_level_by_thermal_zone() is None

    @mock.patch('glob.iglob', mock.MagicMock(return_value=['thermal_zone1', 'thermal_zone2']))
    def test_check_module_temperature_trustable(self):
        from sonic_platform.thermal import Thermal
        from sonic_platform import utils

        utils.read_int_from_file = mock.MagicMock(return_value=1)
        assert Thermal.check_module_temperature_trustable() == 'untrust'

        utils.read_int_from_file = mock.MagicMock(return_value=0)
        assert Thermal.check_module_temperature_trustable() == 'trust'

    def test_get_min_amb_temperature(self):
        from sonic_platform.thermal import Thermal, MAX_AMBIENT_TEMP, CHASSIS_THERMAL_SYSFS_FOLDER
        from sonic_platform import utils

        utils.read_int_from_file = mock.MagicMock(side_effect=Exception(''))
        assert Thermal.get_min_amb_temperature() == MAX_AMBIENT_TEMP

        mock_file_content = {}
        def mock_read_int_from_file(file_path, default=0, raise_exception=False):
            return mock_file_content[file_path]

        utils.read_int_from_file = mock_read_int_from_file
        mock_file_content[os.path.join(CHASSIS_THERMAL_SYSFS_FOLDER, 'fan_amb')] = 50
        mock_file_content[os.path.join(CHASSIS_THERMAL_SYSFS_FOLDER, 'port_amb')] = 40
        assert Thermal.get_min_amb_temperature() == 40

    @mock.patch('sonic_platform.utils.write_file')
    def test_set_cooling_level(self, mock_write_file):
        from sonic_platform.thermal import Thermal, COOLING_STATE_PATH
        Thermal.set_cooling_level(10)
        calls = [mock.call(COOLING_STATE_PATH, 20, raise_exception=True)]
        mock_write_file.assert_has_calls(calls)

        pre_call_count = mock_write_file.call_count
        Thermal.set_cooling_level(10)
        assert pre_call_count == mock_write_file.call_count

        Thermal.set_cooling_level(9)
        calls = [mock.call(COOLING_STATE_PATH, 19, raise_exception=True)]
        mock_write_file.assert_has_calls(calls)

    @mock.patch('sonic_platform.utils.write_file')
    def test_set_cooling_state(self, mock_write_file):
        from sonic_platform.thermal import Thermal, COOLING_STATE_PATH
        Thermal.set_cooling_state(10)
        calls = [mock.call(COOLING_STATE_PATH, 10, raise_exception=True)]
        mock_write_file.assert_has_calls(calls)

        pre_call_count = mock_write_file.call_count
        Thermal.set_cooling_state(10)
        assert pre_call_count == mock_write_file.call_count

        Thermal.set_cooling_state(9)
        calls = [mock.call(COOLING_STATE_PATH, 9, raise_exception=True)]
        mock_write_file.assert_has_calls(calls)

    @mock.patch('sonic_platform.utils.read_int_from_file')
    def test_get_cooling_level(self, mock_read_file):
        from sonic_platform.thermal import Thermal, COOLING_STATE_PATH
        Thermal.get_cooling_level()
        mock_read_file.assert_called_with(COOLING_STATE_PATH, raise_exception=True)

        mock_read_file.side_effect = IOError('')
        with pytest.raises(RuntimeError):
            Thermal.get_cooling_level()

        mock_read_file.side_effect = ValueError('')
        with pytest.raises(RuntimeError):
            Thermal.get_cooling_level()
