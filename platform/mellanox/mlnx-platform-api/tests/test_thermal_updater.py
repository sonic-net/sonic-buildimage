#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2023-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
from sonic_platform.thermal_updater import ThermalUpdater, hw_management_independent_mode_update
from sonic_platform.thermal_updater import ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD, \
                                           ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD


mock_tc_config = """
{
    "dev_parameters": {
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
    def setup_method(self):
        """Reset mocks before each test"""
        hw_management_independent_mode_update.reset_mock()
        hw_management_independent_mode_update.module_data_set_module_counter.reset_mock()
        hw_management_independent_mode_update.thermal_data_set_asic.reset_mock()
        hw_management_independent_mode_update.thermal_data_set_module.reset_mock()
        hw_management_independent_mode_update.thermal_data_clean_asic.reset_mock()
        hw_management_independent_mode_update.thermal_data_clean_module.reset_mock()

    def test_init(self):
        """Test ThermalUpdater initialization"""
        sfp_list = [mock.MagicMock()]
        updater = ThermalUpdater(sfp_list)
        assert updater._sfp_list == sfp_list
        assert updater._sfp_status == {}
        assert updater._timer is not None

    def test_load_tc_config_non_exists(self):
        """Test loading TC config when file doesn't exist"""
        updater = ThermalUpdater(None)
        updater.load_tc_config()
        assert updater._timer._timestamp_queue.qsize() == 2

    def test_load_tc_config_mocked(self):
        """Test loading TC config with mocked file"""
        updater = ThermalUpdater(None)
        mock_os_open = mock.mock_open(read_data=mock_tc_config)
        with mock.patch('sonic_platform.utils.open', mock_os_open):
            updater.load_tc_config()
        assert updater._timer._timestamp_queue.qsize() == 2

    @mock.patch('sonic_platform.thermal_updater.ThermalUpdater.update_module', mock.MagicMock())
    @mock.patch('sonic_platform.thermal_updater.ThermalUpdater.wait_for_sysfs_nodes')
    @mock.patch('sonic_platform.utils.write_file')
    def test_start_stop(self, mock_write, mock_wait_sysfs):
        """Test start and stop functionality"""
        mock_sfp = mock.MagicMock()
        mock_sfp.sdk_index = 1
        updater = ThermalUpdater([mock_sfp])
        mock_wait_sysfs.return_value = True

        updater.start()
        mock_wait_sysfs.assert_called_once()
        mock_write.assert_called_once_with('/run/hw-management/config/suspend', 0)
        utils.wait_until(updater._timer.is_alive, timeout=5)

        mock_write.reset_mock()
        updater.stop()
        assert not updater._timer.is_alive()
        mock_write.assert_called_once_with('/run/hw-management/config/suspend', 1)

    def test_control_tc(self):
        """Test thermal control functionality"""
        updater = ThermalUpdater(None)
        with mock.patch('sonic_platform.utils.write_file') as mock_write:
            updater.control_tc(False)
            mock_write.assert_called_once_with('/run/hw-management/config/suspend', 0)

            mock_write.reset_mock()
            updater.control_tc(True)
            mock_write.assert_called_once_with('/run/hw-management/config/suspend', 1)

    def test_clean_thermal_data(self):
        """Test thermal data cleaning"""
        mock_sfp1 = mock.MagicMock()
        mock_sfp1.sdk_index = 0
        mock_sfp2 = mock.MagicMock()
        mock_sfp2.sdk_index = 1
        sfp_list = [mock_sfp1, mock_sfp2]

        updater = ThermalUpdater(sfp_list)
        updater.clean_thermal_data()

        hw_management_independent_mode_update.module_data_set_module_counter.assert_called_once_with(2)
        hw_management_independent_mode_update.thermal_data_clean_asic.assert_called_once_with(0)
        assert hw_management_independent_mode_update.thermal_data_clean_module.call_count == 2
        hw_management_independent_mode_update.thermal_data_clean_module.assert_any_call(0, 1)
        hw_management_independent_mode_update.thermal_data_clean_module.assert_any_call(0, 2)

    @mock.patch('sonic_platform.utils.read_int_from_file')
    def test_get_asic_temp(self, mock_read):
        """Test ASIC temperature reading"""
        updater = ThermalUpdater(None)

        # Test successful read
        mock_read.return_value = 8
        assert updater.get_asic_temp() == 1000  # 8 * 125

        # Test failed read
        mock_read.return_value = None
        assert updater.get_asic_temp() is None

    @mock.patch('sonic_platform.utils.read_int_from_file')
    def test_get_asic_temp_warning_threshold(self, mock_read):
        """Test ASIC warning threshold reading"""
        updater = ThermalUpdater(None)

        # Test successful read
        mock_read.return_value = 8
        assert updater.get_asic_temp_warning_threshold() == 1000  # 8 * 125

        # Test failed read - should return default
        mock_read.return_value = None
        assert updater.get_asic_temp_warning_threshold() == ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD

    @mock.patch('sonic_platform.utils.read_int_from_file')
    def test_get_asic_temp_critical_threshold(self, mock_read):
        """Test ASIC critical threshold reading"""
        updater = ThermalUpdater(None)

        # Test successful read
        mock_read.return_value = 8
        assert updater.get_asic_temp_critical_threshold() == 1000  # 8 * 125

        # Test failed read - should return default
        mock_read.return_value = None
        assert updater.get_asic_temp_critical_threshold() == ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD

    def test_update_single_module_present(self):
        """Test updating single module when present"""
        mock_sfp = mock.MagicMock()
        mock_sfp.sdk_index = 10
        mock_sfp.get_presence = mock.MagicMock(return_value=True)
        mock_sfp.get_temperature_info = mock.MagicMock(return_value=(55.0, 70.0, 80.0))

        updater = ThermalUpdater([mock_sfp])
        updater.update_single_module(mock_sfp)

        hw_management_independent_mode_update.thermal_data_set_module.assert_called_once_with(0, 11, 55000, 80000, 70000, 0)
        assert updater._sfp_status[10] is True

    def test_update_single_module_not_present(self):
        """Test updating single module when not present"""
        mock_sfp = mock.MagicMock()
        mock_sfp.sdk_index = 10
        mock_sfp.get_presence = mock.MagicMock(return_value=False)

        updater = ThermalUpdater([mock_sfp])
        updater.update_single_module(mock_sfp)

        hw_management_independent_mode_update.thermal_data_set_module.assert_called_once_with(0, 11, 0, 0, 0, 0)
        assert updater._sfp_status[10] is False

    def test_update_single_module_presence_change(self):
        """Test updating single module when presence changes"""
        mock_sfp = mock.MagicMock()
        mock_sfp.sdk_index = 10
        mock_sfp.get_presence = mock.MagicMock(return_value=True)
        mock_sfp.get_temperature_info = mock.MagicMock(return_value=(55.0, 70.0, 80.0))

        updater = ThermalUpdater([mock_sfp])
        # First call - module not present initially
        updater.update_single_module(mock_sfp)
        hw_management_independent_mode_update.reset_mock()

        # Second call
        updater.update_single_module(mock_sfp)
        hw_management_independent_mode_update.thermal_data_set_module.assert_called_once_with(0, 11, 55000, 80000, 70000, 0)

    def test_update_single_module_with_none_temperature(self):
        """Test updating single module with None temperature values"""
        mock_sfp = mock.MagicMock()
        mock_sfp.sdk_index = 10
        mock_sfp.get_presence = mock.MagicMock(return_value=True)
        mock_sfp.get_temperature_info = mock.MagicMock(return_value=(None, None, None))

        updater = ThermalUpdater([mock_sfp])
        updater.update_single_module(mock_sfp)

        hw_management_independent_mode_update.thermal_data_set_module.assert_called_once_with(0, 11, 0, 0, 0, 254000)

    def test_update_single_module_exception(self):
        """Test updating single module when exception occurs"""
        mock_sfp = mock.MagicMock()
        mock_sfp.sdk_index = 10
        mock_sfp.get_presence = mock.MagicMock(side_effect=Exception("Test exception"))

        updater = ThermalUpdater([mock_sfp])
        updater.update_single_module(mock_sfp)

        hw_management_independent_mode_update.thermal_data_set_module.assert_called_once_with(0, 11, 0, 0, 0, 254000)

    def test_load_tc_config_asic_no_poll_time_logging(self):
        """Test logging when ASIC parameter exists but has no poll_time"""
        updater = ThermalUpdater(None)
        mock_data = {
            'dev_parameters': {
                'asic\\d*': {}
            }
        }
        with mock.patch('sonic_platform.utils.load_json_file') as mock_load:
            mock_load.return_value = mock_data
            with mock.patch('sonic_platform.thermal_updater.logger') as mock_logger:
                updater.load_tc_config()
                # Verify logging message for ASIC poll_time not configured
                mock_logger.log_notice.assert_any_call('ASIC poll_time not configured in "asic\\d*", using default 60s')

    def test_find_matching_key(self):
        """Test the regex key matching function"""
        updater = ThermalUpdater(None)

        # Test data with various key patterns
        # /run/hw-management/config/tc_config.json has the following data:
        # "dev_parameters" : {
        #         "asic\\d*":           {"pwm_min": 30, "pwm_max" : 100, "val_min":"!70000", "val_max":"!105000", "poll_time": 3, "sensor_read_error":100},
        #         "(cpu_pack|cpu_core\\d+)": {"pwm_min": 30, "pwm_max" : 100,  "val_min": "!70000", "val_max": "!100000", "poll_time": 3, "sensor_read_error":100},
        #         "module\\d+":     {"pwm_min": 30, "pwm_max" : 100, "val_min":60000, "val_max":80000, "poll_time": 20},
        #         "sensor_amb":     {"pwm_min": 30, "pwm_max" : 50, "val_min": 30000, "val_max": 55000, "poll_time": 30},
        #         "voltmon\\d+_temp": {"pwm_min": 30, "pwm_max": 100, "val_min": "!85000", "val_max": "!125000",  "poll_time": 60},
        #         "sodimm\\d_temp" :{"pwm_min": 30, "pwm_max" : 70, "val_min": "!70000", "val_max": 95000, "poll_time": 60},
        #         "drivetemp":      {"pwm_min": 30, "pwm_max": 70, "val_min": "!70000", "val_max": "!95000", "poll_time": 60},
        #         "ibc\\d+":         {"pwm_min": 30, "pwm_max": 100, "val_min": "!80000", "val_max": "!110000", "poll_time": 60}
        # },
        dev_parameters = {
            'asic\\d*': {'poll_time': 3},  # This should match the pattern
            'module\\d+': {'poll_time': 25},  # This should match the pattern
            'cpu_core0': {'poll_time': 10},
            'other_key': {'poll_time': 15}
        }

        # Test ASIC pattern matching
        key, value = updater._find_matching_key(dev_parameters, r'asic\\d*')
        assert key == 'asic\\d*'
        assert value == {'poll_time': 3}

        # Test module pattern matching
        key, value = updater._find_matching_key(dev_parameters, r'module\\d+')
        assert key == 'module\\d+'
        assert value == {'poll_time': 25}

        # Test pattern that doesn't match
        key, value = updater._find_matching_key(dev_parameters, r'nonexistent\\d+')
        assert key is None
        assert value is None

    def test_load_tc_config_module_no_poll_time_logging(self):
        """Test logging when module parameter exists but has no poll_time"""
        updater = ThermalUpdater(None)
        mock_data = {
            'dev_parameters': {
                'module\\d+': {}
            }
        }
        with mock.patch('sonic_platform.utils.load_json_file') as mock_load:
            mock_load.return_value = mock_data
            with mock.patch('sonic_platform.thermal_updater.logger') as mock_logger:
                updater.load_tc_config()
                # Verify logging message for module poll_time not configured
                mock_logger.log_notice.assert_any_call('Module poll_time not configured in "module\\d+", using default 60s')

    @mock.patch('sonic_platform.utils.wait_until_conditions')
    @mock.patch('sonic_platform.thermal_updater.logger')
    def test_wait_for_sysfs_nodes_success(self, mock_logger, mock_wait_until):
        """Test wait_for_sysfs_nodes when all nodes are ready"""
        updater = ThermalUpdater(None)
        mock_wait_until.return_value = True

        result = updater.wait_for_sysfs_nodes()

        assert result is True
        # Should be called twice: once for "Waiting..." and once for "ready" message
        assert mock_logger.log_notice.call_count == 2
        mock_logger.log_notice.assert_any_call('Waiting for temperature sysfs nodes to be present...')
        mock_wait_until.assert_called_once_with(mock.ANY, 300, 1)

    @mock.patch('sonic_platform.utils.wait_until_conditions')
    @mock.patch('sonic_platform.thermal_updater.logger')
    def test_wait_for_sysfs_nodes_timeout(self, mock_logger, mock_wait_until):
        """Test wait_for_sysfs_nodes when timeout occurs"""
        updater = ThermalUpdater(None)
        mock_wait_until.return_value = False

        result = updater.wait_for_sysfs_nodes()

        assert result is False
        mock_logger.log_notice.assert_called_once_with('Waiting for temperature sysfs nodes to be present...')
        mock_wait_until.assert_called_once_with(mock.ANY, 300, 1)

    @mock.patch('os.path.exists')
    def test_wait_for_sysfs_nodes_conditions_creation(self, mock_exists):
        """Test that wait_for_sysfs_nodes creates correct conditions"""
        # Create updater with 2 SFPs for testing
        mock_sfp1 = mock.MagicMock()
        mock_sfp2 = mock.MagicMock()
        updater = ThermalUpdater([mock_sfp1, mock_sfp2])
        mock_exists.return_value = True

        with mock.patch('sonic_platform.utils.wait_until_conditions') as mock_wait_until:
            mock_wait_until.return_value = True
            updater.wait_for_sysfs_nodes()

            # Verify that conditions were created for each temperature node
            args, kwargs = mock_wait_until.call_args
            conditions = args[0]
            # 1 ASIC node + (2 SFPs * (3 module temp nodes + 1 eeprom dir)) = 1 + (2 * 4) = 9
            assert len(conditions) == 9

            # Test that each condition calls os.path.exists
            for condition in conditions:
                condition()

            # Should be called once for each condition
            assert mock_exists.call_count == 9

            # Verify the expected paths are checked
            expected_calls = [
                # ASIC temperature nodes
                mock.call('/sys/module/sx_core/asic0/temperature/input'),
                # Module temperature nodes for module 0
                mock.call('/sys/module/sx_core/asic0/module0/temperature/input'),
                mock.call('/sys/module/sx_core/asic0/module0/temperature/threshold_hi'),
                mock.call('/sys/module/sx_core/asic0/module0/temperature/threshold_critical_hi'),
                # Module EEPROM directory for module 0
                mock.call('/sys/module/sx_core/asic0/module0/eeprom/'),
                # Module temperature nodes for module 1
                mock.call('/sys/module/sx_core/asic0/module1/temperature/input'),
                mock.call('/sys/module/sx_core/asic0/module1/temperature/threshold_hi'),
                mock.call('/sys/module/sx_core/asic0/module1/temperature/threshold_critical_hi'),
                # Module EEPROM directory for module 1
                mock.call('/sys/module/sx_core/asic0/module1/eeprom/'),
            ]
            mock_exists.assert_has_calls(expected_calls, any_order=True)

    @mock.patch('os.path.exists')
    def test_wait_for_sysfs_nodes_no_sfps(self, mock_exists):
        """Test that wait_for_sysfs_nodes works correctly with no SFPs"""
        updater = ThermalUpdater([])  # Empty SFP list
        mock_exists.return_value = True

        with mock.patch('sonic_platform.utils.wait_until_conditions') as mock_wait_until:
            mock_wait_until.return_value = True
            updater.wait_for_sysfs_nodes()

            # Verify that conditions were created only for ASIC nodes
            args, kwargs = mock_wait_until.call_args
            conditions = args[0]
            # Only 1 ASIC node when no SFPs
            assert len(conditions) == 1

            # Test that each condition calls os.path.exists
            for condition in conditions:
                condition()

            # Should be called once for each ASIC condition
            assert mock_exists.call_count == 1

            # Verify only ASIC paths are checked
            expected_calls = [
                mock.call('/sys/module/sx_core/asic0/temperature/input'),
            ]
            mock_exists.assert_has_calls(expected_calls, any_order=True)

    @mock.patch('sonic_platform.thermal_updater.ThermalUpdater.wait_for_sysfs_nodes')
    @mock.patch('sonic_platform.thermal_updater.ThermalUpdater.clean_thermal_data')
    @mock.patch('sonic_platform.thermal_updater.ThermalUpdater.control_tc')
    @mock.patch('sonic_platform.thermal_updater.ThermalUpdater.load_tc_config')
    @mock.patch('sonic_platform.thermal_updater.logger')
    def test_start_with_sysfs_wait_success(self, mock_logger, mock_load_config, mock_control_tc,
                                          mock_clean_data, mock_wait_sysfs):
        """Test start method when sysfs nodes are available"""
        updater = ThermalUpdater(None)
        updater._timer = mock.MagicMock()
        mock_wait_sysfs.return_value = True

        result = updater.start()

        assert result is True
        mock_wait_sysfs.assert_called_once()
        mock_clean_data.assert_called_once()
        mock_control_tc.assert_called_once_with(False)
        mock_load_config.assert_called_once()
        updater._timer.start.assert_called_once()
