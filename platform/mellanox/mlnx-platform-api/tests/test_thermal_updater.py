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

    # ---- SFP.get_temperature_info publishes vendor info on module change ----
    def _make_sfp_for_publish(self, sn_changed=True, vendor=('Innolight', 'TR-iQ13L-NVS')):
        # Import locally to avoid any potential name resolution issues in test scope
        from sonic_platform.sfp import SFP as _SFP
        sfp = object.__new__(_SFP)
        sfp.sdk_index = 10
        sfp.retry_read_vendor = 5 if sn_changed else 0
        sfp.is_sw_control = mock.MagicMock(return_value=True)
        sfp.reinit_if_sn_changed = mock.MagicMock(return_value=sn_changed)
        if vendor is None:
            sfp.get_vendor_info = mock.MagicMock(return_value=(None, None))
        else:
            sfp.get_vendor_info = mock.MagicMock(return_value=vendor)
        api = mock.MagicMock()
        api.get_transceiver_thresholds_support = mock.MagicMock(return_value=False)
        sfp.get_xcvr_api = mock.MagicMock(return_value=api)
        return sfp

    def test_sfp_get_temperature_info_publishes_vendor_on_sn_change(self):
        from sonic_platform.sfp import hw_management_independent_mode_update as sfp_hw_management_independent_mode_update
        sfp = self._make_sfp_for_publish(sn_changed=True, vendor=('Innolight', 'TR-iQ13L-NVS'))
        with mock.patch('sonic_platform.sfp.SfpOptoeBase.get_temperature', return_value=55.0):
            sfp_hw_management_independent_mode_update.reset_mock()
            sfp.get_temperature_info()
            sfp_hw_management_independent_mode_update.vendor_data_set_module.assert_called_once_with(
                0, 11, {'manufacturer': 'Innolight', 'part_number': 'TR-iQ13L-NVS'}
            )

    def test_sfp_get_temperature_info_no_publish_when_no_change(self):
        from sonic_platform.sfp import hw_management_independent_mode_update as sfp_hw_management_independent_mode_update
        sfp = self._make_sfp_for_publish(sn_changed=False, vendor=('Innolight', 'TR-iQ13L-NVS'))
        with mock.patch('sonic_platform.sfp.SfpOptoeBase.get_temperature', return_value=55.0):
            sfp_hw_management_independent_mode_update.reset_mock()
            sfp.get_temperature_info()
            sfp_hw_management_independent_mode_update.vendor_data_set_module.assert_not_called()

    def test_sfp_get_temperature_info_no_publish_when_vendor_missing(self):
        from sonic_platform.sfp import hw_management_independent_mode_update as sfp_hw_management_independent_mode_update
        sfp = self._make_sfp_for_publish(sn_changed=True, vendor=None)
        with mock.patch('sonic_platform.sfp.SfpOptoeBase.get_temperature', return_value=55.0):
            sfp_hw_management_independent_mode_update.reset_mock()
            sfp.get_temperature_info()
            sfp_hw_management_independent_mode_update.vendor_data_set_module.assert_not_called()

