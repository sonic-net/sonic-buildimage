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
from sonic_platform.thermal_updater import ThermalUpdater, hw_management_independent_mode_update
from sonic_platform.thermal_updater import ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD, \
                                           ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD


mock_tc_config = """
{
    "dev_parameters": {
        "asic": {
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
    def test_load_tc_config_non_exists(self):
        updater = ThermalUpdater(None)
        updater.load_tc_config()
        assert updater._timer._timestamp_queue.qsize() == 2

    def test_load_tc_config_mocked(self):
        updater = ThermalUpdater(None)
        mock_os_open = mock.mock_open(read_data=mock_tc_config)
        with mock.patch('sonic_platform.utils.open', mock_os_open):
            updater.load_tc_config()
        assert updater._timer._timestamp_queue.qsize() == 2

    @mock.patch('sonic_platform.thermal_updater.ThermalUpdater.update_asic', mock.MagicMock())
    @mock.patch('sonic_platform.thermal_updater.ThermalUpdater.update_module', mock.MagicMock())
    @mock.patch('sonic_platform.thermal_updater.ThermalUpdater.wait_for_sysfs_nodes', mock.MagicMock(return_value=True))
    @mock.patch('sonic_platform.utils.write_file')
    def test_start_stop(self, mock_write):
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
        from sonic_platform.sfp import SFP
        sfp = SFP(0)
        
        hw_management_independent_mode_update.reset_mock()
        sfp.get_presence = mock.MagicMock(return_value=True)
        sfp.get_temperature_from_db = mock.MagicMock(return_value=30.0)
        sfp.get_warning_threshold_from_db = mock.MagicMock(return_value=25.0)
        sfp.get_critical_threshold_from_db = mock.MagicMock(return_value=30.0)
        sfp.get_vendor_name_from_db = mock.MagicMock(return_value='Mellanox')
        sfp.get_part_number_from_db = mock.MagicMock(return_value='SFP-10G-SR')
        
        updater = ThermalUpdater([sfp])
        updater.update_module()
        hw_management_independent_mode_update.thermal_data_set_module.assert_called_with(0, sfp.sdk_index + 1, 30000, 30000, 25000, 0)
        hw_management_independent_mode_update.vendor_data_set_module.assert_called_with(0, sfp.sdk_index + 1, {'manufacturer': 'Mellanox', 'part_number': 'SFP-10G-SR'})
        
        sfp.get_warning_threshold_from_db.return_value = 25.0        
        sfp.get_critical_threshold_from_db.return_value = 20
        updater.update_module()
        hw_management_independent_mode_update.thermal_data_set_module.assert_called_with(0, sfp.sdk_index + 1, 30000, 20000, 25000, 254000)
        
        sfp.get_temperature_from_db.return_value = 0.0
        updater.update_module()
        hw_management_independent_mode_update.thermal_data_set_module.assert_called_with(0, sfp.sdk_index + 1, 0, 0, 0, 0)
        
        sfp.get_temperature_from_db.return_value = -1
        updater.update_module()
        hw_management_independent_mode_update.thermal_data_set_module.assert_called_with(0, sfp.sdk_index + 1, 0, 0, 0, 254000)
        
        sfp.get_presence.return_value = False
        updater.update_module()
        hw_management_independent_mode_update.thermal_data_set_module.assert_called_with(0, sfp.sdk_index + 1, 0, 0, 0, 0)
        hw_management_independent_mode_update.vendor_data_set_module.assert_called_with(0, sfp.sdk_index + 1, {'manufacturer': '', 'part_number': ''})
        
        sfp.get_presence.side_effect = Exception('test exception')
        updater.update_module()
        hw_management_independent_mode_update.thermal_data_set_module.assert_called_with(0, sfp.sdk_index + 1, 0, 0, 0, 254000)

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

        clean_thermal_data([sfp_sw])

        hw_management_independent_mode_update.module_data_set_module_counter.assert_called_once_with(1)
        hw_management_independent_mode_update.thermal_data_clean_module.assert_called_once_with(0, sfp_sw.sdk_index + 1)
