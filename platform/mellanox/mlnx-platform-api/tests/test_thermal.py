#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2021-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
import time
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
from sonic_platform.sfp import SFP

sonic_platform.chassis.extract_RJ45_ports_index = mock.MagicMock(return_value=[])

class TestThermal:
    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_gearbox_count', mock.MagicMock(return_value=2))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_cpu_thermal_count', mock.MagicMock(return_value=2))
    @mock.patch('sonic_platform.thermal.glob.iglob', mock.MagicMock(
        return_value=['/run/hw-management/thermal/sodimm1_temp_input',
                      '/run/hw-management/thermal/sodimm2_temp_input']))
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

    def test_thermal_allow_delay_create_default(self):
        """Test that Thermal objects default to allow_delay_create=False"""
        from sonic_platform.thermal import Thermal
        thermal = Thermal('test', 'temp_file', 'high_th_file', 'high_crit_th_file', 1)
        assert thermal.allow_delay_create is False

    def test_thermal_allow_delay_create_true(self):
        """Test that Thermal objects can be created with allow_delay_create=True"""
        from sonic_platform.thermal import Thermal
        thermal = Thermal('test', 'temp_file', 'high_th_file', 'high_crit_th_file', 1, allow_delay_create=True)
        assert thermal.allow_delay_create is True

    @mock.patch('os.path.exists')
    @mock.patch('time.sleep')
    @mock.patch('sonic_platform.thermal.logger')
    def test_retry_until_file_exists_success_first_try(self, mock_logger, mock_sleep, mock_exists):
        """Test _retry_until_file_exists when file exists on first try"""
        from sonic_platform.thermal import Thermal
        thermal = Thermal('test', 'temp_file', 'high_th_file', 'high_crit_th_file', 1, allow_delay_create=True)

        # File exists on first try
        mock_exists.return_value = True

        result = thermal._retry_until_file_exists('test_file')

        assert result is True
        assert thermal.allow_delay_create is False  # Should be disabled after success
        mock_exists.assert_called_once_with('test_file')
        mock_sleep.assert_not_called()
        mock_logger.log_debug.assert_called_once()

    @mock.patch('os.path.exists')
    @mock.patch('time.sleep')
    @mock.patch('sonic_platform.thermal.logger')
    def test_retry_until_file_exists_success_after_retries(self, mock_logger, mock_sleep, mock_exists):
        """Test _retry_until_file_exists when file exists after retries"""
        from sonic_platform.thermal import Thermal
        thermal = Thermal('test', 'temp_file', 'high_th_file', 'high_crit_th_file', 1, allow_delay_create=True)

        # File doesn't exist for first 2 attempts, then exists
        mock_exists.side_effect = [False, False, True]

        result = thermal._retry_until_file_exists('test_file')

        assert result is True
        assert thermal.allow_delay_create is False  # Should be disabled after success
        assert mock_exists.call_count == 3
        assert mock_sleep.call_count == 2  # Should sleep twice (between attempts 1-2 and 2-3)
        mock_logger.log_warning.assert_called()
        mock_logger.log_debug.assert_called_once()

    @mock.patch('os.path.exists')
    @mock.patch('time.sleep')
    @mock.patch('sonic_platform.thermal.logger')
    def test_retry_until_file_exists_failure_after_all_retries(self, mock_logger, mock_sleep, mock_exists):
        """Test _retry_until_file_exists when file never exists after all retries"""
        from sonic_platform.thermal import Thermal
        thermal = Thermal('test', 'temp_file', 'high_th_file', 'high_crit_th_file', 1, allow_delay_create=True)

        # File never exists
        mock_exists.return_value = False

        result = thermal._retry_until_file_exists('test_file')

        assert result is False
        assert thermal.allow_delay_create is True  # Should remain True after failure
        assert mock_exists.call_count == 5  # Should try 5 times
        assert mock_sleep.call_count == 4  # Should sleep 4 times (between attempts)
        mock_logger.log_warning.assert_called()
        mock_logger.log_error.assert_called_once()

    @mock.patch('os.path.exists')
    @mock.patch('time.sleep')
    @mock.patch('sonic_platform.thermal.logger')
    def test_retry_until_file_exists_disabled(self, mock_logger, mock_sleep, mock_exists):
        """Test _retry_until_file_exists when allow_delay_create is False"""
        from sonic_platform.thermal import Thermal
        thermal = Thermal('test', 'temp_file', 'high_th_file', 'high_crit_th_file', 1, allow_delay_create=False)

        result = thermal._retry_until_file_exists('test_file')

        assert result is True  # Should return True immediately when disabled
        mock_exists.assert_not_called()
        mock_sleep.assert_not_called()
        mock_logger.log_warning.assert_not_called()
        mock_logger.log_error.assert_not_called()

    @mock.patch('os.path.exists')
    @mock.patch('sonic_platform.utils.read_float_from_file')
    @mock.patch('sonic_platform.thermal.logger')
    def test_get_temperature_with_retry_success(self, mock_logger, mock_read, mock_exists):
        """Test get_temperature with retry mechanism when file exists after retry"""
        from sonic_platform.thermal import Thermal
        thermal = Thermal('test', 'temp_file', 'high_th_file', 'high_crit_th_file', 1, allow_delay_create=True)

        # File doesn't exist for first 2 attempts, then exists
        mock_exists.side_effect = [False, False, True]
        mock_read.return_value = 35727

        result = thermal.get_temperature()

        assert result == 35.727
        assert thermal.allow_delay_create is False  # Should be disabled after success

    @mock.patch('os.path.exists')
    @mock.patch('sonic_platform.utils.read_float_from_file')
    @mock.patch('sonic_platform.thermal.logger')
    def test_get_temperature_with_retry_failure(self, mock_logger, mock_read, mock_exists):
        """Test get_temperature with retry mechanism when file never exists"""
        from sonic_platform.thermal import Thermal
        thermal = Thermal('test', 'temp_file', 'high_th_file', 'high_crit_th_file', 1, allow_delay_create=True)

        # File never exists
        mock_exists.return_value = False

        result = thermal.get_temperature()

        assert result is None
        assert thermal.allow_delay_create is True  # Should remain True after failure

    @mock.patch('os.path.exists')
    @mock.patch('sonic_platform.utils.read_float_from_file')
    @mock.patch('sonic_platform.thermal.logger')
    def test_get_high_threshold_with_retry(self, mock_logger, mock_read, mock_exists):
        """Test get_high_threshold with retry mechanism"""
        from sonic_platform.thermal import Thermal
        thermal = Thermal('test', 'temp_file', 'high_th_file', 'high_crit_th_file', 1, allow_delay_create=True)

        # File exists on second try
        mock_exists.side_effect = [False, True]
        mock_read.return_value = 25833

        result = thermal.get_high_threshold()

        assert result == 25.833
        assert thermal.allow_delay_create is False  # Should be disabled after success

    @mock.patch('os.path.exists')
    @mock.patch('sonic_platform.utils.read_float_from_file')
    @mock.patch('sonic_platform.thermal.logger')
    def test_get_high_critical_threshold_with_retry(self, mock_logger, mock_read, mock_exists):
        """Test get_high_critical_threshold with retry mechanism"""
        from sonic_platform.thermal import Thermal
        thermal = Thermal('test', 'temp_file', 'high_th_file', 'high_crit_th_file', 1, allow_delay_create=True)

        # File exists on third try
        mock_exists.side_effect = [False, False, True]
        mock_read.return_value = 120839

        result = thermal.get_high_critical_threshold()

        assert result == 120.839
        assert thermal.allow_delay_create is False  # Should be disabled after success

    def test_thermal_naming_rule_allow_delay_create(self):
        """Test that THERMAL_NAMING_RULE has allow_delay_create field for sfp thermals and ASIC"""
        from sonic_platform.thermal import THERMAL_NAMING_RULE

        # Check sfp thermals
        sfp_rule = THERMAL_NAMING_RULE.get('sfp thermals')
        assert sfp_rule is not None
        assert sfp_rule.get('allow_delay_create') is True

        # Check ASIC in chassis thermals
        chassis_thermals = THERMAL_NAMING_RULE.get('chassis thermals', [])
        asic_thermal = None
        for thermal in chassis_thermals:
            if thermal.get('name') == 'ASIC':
                asic_thermal = thermal
                break

        assert asic_thermal is not None
        assert asic_thermal.get('allow_delay_create') is True

    @mock.patch('os.path.exists')
    @mock.patch('sonic_platform.thermal.logger')
    def test_check_thermal_sysfs_existence_with_allow_delay_create_true(self, mock_logger, mock_exists):
        """Test _check_thermal_sysfs_existence when allow_delay_create is True"""
        from sonic_platform.thermal import _check_thermal_sysfs_existence

        # File doesn't exist
        mock_exists.return_value = False

        _check_thermal_sysfs_existence('test_file', None, allow_delay_create=True)

        # Should log notice when allow_delay_create is True
        mock_logger.log_notice.assert_called_once_with('Thermal sysfs test_file does not exist')
        mock_logger.log_error.assert_not_called()

    @mock.patch('os.path.exists')
    @mock.patch('sonic_platform.thermal.logger')
    def test_check_thermal_sysfs_existence_with_allow_delay_create_false(self, mock_logger, mock_exists):
        """Test _check_thermal_sysfs_existence when allow_delay_create is False"""
        from sonic_platform.thermal import _check_thermal_sysfs_existence

        # File doesn't exist
        mock_exists.return_value = False

        _check_thermal_sysfs_existence('test_file', None, allow_delay_create=False)

        # Should log error when allow_delay_create is False
        mock_logger.log_error.assert_called_once_with('Thermal sysfs test_file does not exist')
        mock_logger.log_notice.assert_not_called()

    @mock.patch('os.path.exists')
    @mock.patch('sonic_platform.thermal.logger')
    def test_check_thermal_sysfs_existence_with_file_exists(self, mock_logger, mock_exists):
        """Test _check_thermal_sysfs_existence when file exists"""
        from sonic_platform.thermal import _check_thermal_sysfs_existence

        # File exists
        mock_exists.return_value = True

        _check_thermal_sysfs_existence('test_file', None, allow_delay_create=True)
        _check_thermal_sysfs_existence('test_file', None, allow_delay_create=False)

        # Should not log anything when file exists
        mock_logger.log_notice.assert_not_called()
        mock_logger.log_error.assert_not_called()

    @mock.patch('os.path.exists')
    @mock.patch('sonic_platform.thermal.logger')
    def test_check_thermal_sysfs_existence_with_presence_cb_false(self, mock_logger, mock_exists):
        """Test _check_thermal_sysfs_existence when presence_cb returns False"""
        from sonic_platform.thermal import _check_thermal_sysfs_existence

        # Mock presence callback that returns False
        presence_cb = mock.MagicMock(return_value=(False, 'Not present'))

        _check_thermal_sysfs_existence('test_file', presence_cb, allow_delay_create=True)
        _check_thermal_sysfs_existence('test_file', presence_cb, allow_delay_create=False)

        # Should not log anything when presence_cb returns False
        mock_exists.assert_not_called()
        mock_logger.log_notice.assert_not_called()
        mock_logger.log_error.assert_not_called()

    @mock.patch('os.path.exists')
    @mock.patch('sonic_platform.thermal.logger')
    def test_check_thermal_sysfs_existence_with_presence_cb_true(self, mock_logger, mock_exists):
        """Test _check_thermal_sysfs_existence when presence_cb returns True"""
        from sonic_platform.thermal import _check_thermal_sysfs_existence

        # Mock presence callback that returns True
        presence_cb = mock.MagicMock(return_value=(True, 'Present'))

        # File doesn't exist
        mock_exists.return_value = False

        _check_thermal_sysfs_existence('test_file', presence_cb, allow_delay_create=True)
        _check_thermal_sysfs_existence('test_file', presence_cb, allow_delay_create=False)

        # Should check file existence and log accordingly
        assert mock_exists.call_count == 2
        mock_logger.log_notice.assert_called_once_with('Thermal sysfs test_file does not exist')
        mock_logger.log_error.assert_called_once_with('Thermal sysfs test_file does not exist')
