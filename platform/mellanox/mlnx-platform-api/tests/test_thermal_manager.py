#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

from unittest import mock
from sonic_platform.thermal_manager import ThermalManager


class TestThermalManager:

    @mock.patch('sonic_platform.chassis.Chassis.chassis_instance', new_callable=mock.MagicMock)
    @mock.patch('sonic_platform.device_data.DeviceDataManager.is_module_host_management_mode')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_dpus_data')
    def test_updater_init(self, mock_dpus_data, mock_management_mode, mock_chassis_instance):
        mock_dpus_data.return_value = {}
        mock_management_mode.return_value = True
        sfp_mock = mock.MagicMock()
        mod_mock = mock.MagicMock()
        mock_chassis_instance.get_all_sfps = sfp_mock
        mock_chassis_instance.get_all_modules = mod_mock
        sfp_mock.return_value = ['sfp1', 'sfp2']
        mod_mock.return_value = ['dpu1', 'dpu2']

        with mock.patch('sonic_platform.thermal_updater.ThermalUpdater') as mock_thermal, \
          mock.patch('sonic_platform.smartswitch_thermal_updater.SmartswitchThermalUpdater') as mock_sm_thermal:
            # Host mgmt mode, no DPUs are used for init
            mgr = ThermalManager()
            mgr.initialize()
            mock_thermal.assert_called_once_with(sfp_list=['sfp1', 'sfp2'], update_asic=False)
            mgr.deinitialize()
            mgr.thermal_updater_task.stop.assert_called_once()
            # Not initialized if no DPUs and not in host mgmt mode
            mock_management_mode.return_value = False
            mock_thermal.reset_mock()
            mgr.initialize()
            mock_thermal.assert_not_called()
            mgr.deinitialize()
            mgr.thermal_updater_task.stop.assert_called_once()
            # Initialized with DPUs if DPUs are present
            mock_dpus_data.return_value = {'DPUS': 'dpu1'}
            mock_thermal.reset_mock()
            mgr.initialize()
            mock_sm_thermal.assert_called_once_with(sfp_list=['sfp1', 'sfp2'], dpu_list=['dpu1', 'dpu2'], is_host_mgmt_mode=False)
            mgr.deinitialize()
            mgr.thermal_updater_task.stop.assert_called_once()
            # Host mgmt mode, with DPUS
            mock_thermal.reset_mock()
            mock_sm_thermal.reset_mock()
            mock_management_mode.return_value = True
            mgr.initialize()
            mock_sm_thermal.assert_called_once_with(sfp_list=['sfp1', 'sfp2'], dpu_list=['dpu1', 'dpu2'], is_host_mgmt_mode=True)
            mgr.deinitialize()
            mgr.thermal_updater_task.stop.assert_called_once()
