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
        # Should only schedule module update (no ASIC update)
        assert updater._timer._timestamp_queue.qsize() == 1

    def test_load_tc_config_mocked(self):
        """Test loading TC config with mocked file"""
        updater = ThermalUpdater(None)
        mock_os_open = mock.mock_open(read_data=mock_tc_config)
        with mock.patch('sonic_platform.utils.open', mock_os_open):
            updater.load_tc_config()
        # Should only schedule module update (no ASIC update)
        assert updater._timer._timestamp_queue.qsize() == 1

    @mock.patch('sonic_platform.thermal_updater.ThermalUpdater.update_module', mock.MagicMock())
    @mock.patch('sonic_platform.utils.write_file')
    def test_start_stop(self, mock_write):
        """Test start and stop functionality"""
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

        # Second call - module still present, should not call set_module again
        updater.update_single_module(mock_sfp)
        hw_management_independent_mode_update.thermal_data_set_module.assert_not_called()

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
