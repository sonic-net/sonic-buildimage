#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2023-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

import time
from unittest import mock

from sonic_platform import utils
from sonic_platform.thermal_updater import ThermalUpdater, clean_thermal_data, hw_management_independent_mode_update
from sonic_platform.thermal_updater import ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD, \
                                           ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD


mock_tc_config = """
{
    "dev_parameters": {
        "asic\\\\d*": {
            "pwm_min": 20,
            "pwm_max": 100,
            "val_min": "!70000",
            "val_max": "!105000",
            "poll_time": 3
        },
        "module\\\\d+": {
            "pwm_min": 20,
            "pwm_max": 100,
            "val_min": 60000,
            "val_max": 80000,
            "poll_time": 20
        }
    }
}
"""


class TestThermalUpdater:
    @mock.patch('sonic_platform.thermal_updater.logger')
    def test_load_tc_config_non_exists(self, mock_logger):
        updater = ThermalUpdater(None)
        updater.load_tc_config()
        assert updater._timer._timestamp_queue.qsize() == 2

    @mock.patch('sonic_platform.thermal_updater.logger')
    def test_load_tc_config_mocked(self, mock_logger):
        updater = ThermalUpdater(None)
        mock_os_open = mock.mock_open(read_data=mock_tc_config)
        with mock.patch('sonic_platform.utils.open', mock_os_open):
            updater.load_tc_config()
        assert updater._timer._timestamp_queue.qsize() == 2
        # Verify that debug logs were called with the correct parameters
        assert mock_logger.log_notice.call_count >= 2  # At least ASIC and Module parameter logs

    def test_find_matching_key(self):
        """Test _find_matching_key method for regex pattern matching"""
        updater = ThermalUpdater(None)

        # Test with asic pattern - should match 'asic\d*' keys
        dev_parameters = {
            'asic\\d*': {'poll_time': 3},
            'module\\d+': {'poll_time': 20},
            'sensor_amb': {'poll_time': 30}
        }

        # Test matching asic pattern
        key, value = updater._find_matching_key(dev_parameters, r'asic\\d*')
        assert key == 'asic\\d*'
        assert value == {'poll_time': 3}

        # Test matching module pattern
        key, value = updater._find_matching_key(dev_parameters, r'module\\d+')
        assert key == 'module\\d+'
        assert value == {'poll_time': 20}

        # Test non-matching pattern
        key, value = updater._find_matching_key(dev_parameters, r'nonexistent')
        assert key is None
        assert value is None

        # Test with empty dict
        key, value = updater._find_matching_key({}, r'asic\\d*')
        assert key is None
        assert value is None

    @mock.patch('sonic_platform.thermal_updater.logger')
    @mock.patch('sonic_platform.thermal_updater.ThermalUpdater.update_asic', mock.MagicMock())
    @mock.patch('sonic_platform.thermal_updater.ThermalUpdater.update_module', mock.MagicMock())
    @mock.patch('sonic_platform.utils.write_file')
    def test_start_stop(self, mock_write, mock_logger):
        mock_sfp = mock.MagicMock()
        mock_sfp.sdk_index = 1
        updater = ThermalUpdater([mock_sfp])
        updater.start()
        mock_write.assert_called_once_with('/run/hw-management/config/suspend', 0)
        utils.wait_until(updater._timer.is_alive, timeout=5)

        mock_write.reset_mock()
        updater.stop()
        assert not updater._timer.is_alive()
        mock_write.assert_called_once_with('/run/hw-management/config/suspend', 1)

    @mock.patch('sonic_platform.utils.read_int_from_file')
    def test_update_asic(self, mock_read):
        hw_management_independent_mode_update.reset_mock()
        mock_read.return_value = 8
        updater = ThermalUpdater(None)
        assert updater.get_asic_temp() == 1000
        assert updater.get_asic_temp_warning_threshold() == 1000
        assert updater.get_asic_temp_critical_threshold() == 1000
        updater.update_asic()
        hw_management_independent_mode_update.thermal_data_set_asic.assert_called_once()

        mock_read.return_value = None
        assert updater.get_asic_temp() is None
        assert updater.get_asic_temp_warning_threshold() == ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD
        assert updater.get_asic_temp_critical_threshold() == ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD

    def test_update_module(self):
        hw_management_independent_mode_update.reset_mock()
        mock_sfp = mock.MagicMock()
        mock_sfp.sdk_index = 10
        mock_sfp.get_presence = mock.MagicMock(return_value=True)
        mock_sfp.get_temperature_info = mock.MagicMock(return_value=(True, 55.0, 70.0, 80.0))
        updater = ThermalUpdater([mock_sfp])
        updater.update_module()
        hw_management_independent_mode_update.thermal_data_set_module.assert_called_once_with(0, 11, 55000, 80000, 70000, 0)

        mock_sfp.get_temperature_info = mock.MagicMock(return_value=(True, 0.0, 0.0, 0.0))
        hw_management_independent_mode_update.reset_mock()
        updater.update_module()
        hw_management_independent_mode_update.thermal_data_set_module.assert_called_once_with(0, 11, 0, 0, 0, 0)

        mock_sfp.get_presence = mock.MagicMock(return_value=False)
        hw_management_independent_mode_update.reset_mock()
        updater.update_module()
        hw_management_independent_mode_update.thermal_data_set_module.assert_called_once_with(0, 11, 0, 0, 0, 0)

    @mock.patch('sonic_platform.thermal_updater.clean_thermal_data')
    @mock.patch('sonic_platform.thermal_updater.atexit.register')
    def test_registers_exit_cleanup(self, mock_register, mock_clean):
        hw_management_independent_mode_update.reset_mock()
        sfp = mock.MagicMock()
        updater = ThermalUpdater([sfp])

        mock_register.assert_called_once()
        exit_callback = mock_register.call_args[0][0]

        # Ensure clean routine is not run during construction/start
        mock_clean.assert_not_called()

        # Simulate process exit and confirm cleanup uses the bound SFP list
        exit_callback()
        mock_clean.assert_called_once_with([sfp])

    def test_clean_thermal_data_only_sw_control_modules(self):
        hw_management_independent_mode_update.reset_mock()

        sfp_sw = mock.MagicMock()
        sfp_sw.sdk_index = 3
        sfp_sw.is_sw_control = mock.MagicMock(return_value=True)

        sfp_no_sw = mock.MagicMock()
        sfp_no_sw.sdk_index = 4
        sfp_no_sw.is_sw_control = mock.MagicMock(return_value=False)

        clean_thermal_data([sfp_sw, sfp_no_sw])

        hw_management_independent_mode_update.module_data_set_module_counter.assert_called_once_with(2)
        hw_management_independent_mode_update.thermal_data_clean_module.assert_called_once_with(0, sfp_sw.sdk_index + 1)
