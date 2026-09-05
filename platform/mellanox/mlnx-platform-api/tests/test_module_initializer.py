#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

import os
import sys

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform import chassis
from sonic_platform import module_host_mgmt_initializer


class TestModuleInitializer:

    @mock.patch('sonic_platform.device_data.DeviceDataManager.wait_sysfs_ready', mock.MagicMock(return_value=True))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_asic_count', mock.MagicMock(return_value=1))
    @mock.patch('sonic_platform.chassis.Chassis.get_num_sfps', mock.MagicMock(return_value=1))
    @mock.patch('sonic_platform.chassis.extract_RJ45_ports_index', mock.MagicMock(return_value=[]))
    @mock.patch('sonic_platform.chassis.extract_cpo_ports_index', mock.MagicMock(return_value=[]))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_sfp_count', mock.MagicMock(return_value=1))
    @mock.patch('sonic_platform.utils.read_int_from_file')
    @mock.patch('sonic_platform.module_host_mgmt_initializer.ModuleHostMgmtInitializer.is_initialization_owner')
    @mock.patch('sonic_platform.utils.is_host')
    def test_initialize(self, mock_is_host, mock_owner, mock_read_int):
        # Mock the SFP.initialize_sfp_modules to avoid thread issues
        mock_init_modules = mock.MagicMock()
        with mock.patch('sonic_platform.sfp.SFP.initialize_sfp_modules', mock_init_modules):
            c = chassis.Chassis()
            initializer = module_host_mgmt_initializer.ModuleHostMgmtInitializer()

            mock_is_host.return_value = True
            mock_owner.return_value = False
            # called from host side, just wait (calls chassis.initialize_sfp())
            initializer.initialize(c)

            mock_is_host.return_value = False
            # non-initializer-owner called from container side, just wait (calls chassis.initialize_sfp())
            initializer.initialize(c)

            mock_owner.return_value = True
            mock_read_int.return_value = 1  # asic1_ready file returns 1
            initializer.initialize(c)
            # Verify initialization occurred
            assert initializer.initialized_list[0] == True  # asic 0 should be initialized
            assert module_host_mgmt_initializer.initialization_owner
            assert os.path.exists(module_host_mgmt_initializer.get_asic_ready_file_path(0))
            # Verify SFP.initialize_sfp_modules was called
            mock_init_modules.assert_called_once()

    def test_is_initialization_owner(self):
        initializer = module_host_mgmt_initializer.ModuleHostMgmtInitializer()
        assert not initializer.is_initialization_owner()
