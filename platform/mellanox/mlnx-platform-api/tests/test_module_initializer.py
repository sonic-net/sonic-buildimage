#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
    @mock.patch('os.path.exists')
    @mock.patch('sonic_platform.utils.wait_until')
    @mock.patch('sonic_platform.utils.is_host')
    def test_wait_module_ready(self, mock_is_host, mock_wait, mock_exists):
        initializer = module_host_mgmt_initializer.ModuleHostMgmtInitializer()
        mock_is_host.return_value = True
        mock_exists.return_value = False
        mock_wait.return_value = True
        initializer.wait_module_ready()
        mock_exists.assert_called_with(module_host_mgmt_initializer.MODULE_READY_HOST_FILE)
        assert initializer.initialized
        
        initializer.initialized = False
        mock_is_host.return_value = False
        initializer.wait_module_ready()
        mock_exists.assert_called_with(module_host_mgmt_initializer.MODULE_READY_CONTAINER_FILE)
        
        initializer.initialized = False
        mock_exists.return_value = True
        initializer.wait_module_ready()
        assert initializer.initialized
        
        initializer.initialized = False
        mock_wait.return_value = False
        mock_exists.return_value = False
        initializer.wait_module_ready()
        assert not initializer.initialized

    @mock.patch('sonic_platform.chassis.extract_RJ45_ports_index', mock.MagicMock(return_value=[]))
    @mock.patch('sonic_platform.chassis.extract_cpo_ports_index', mock.MagicMock(return_value=[]))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_sfp_count', mock.MagicMock(return_value=1))
    @mock.patch('sonic_platform.sfp.SFP.initialize_sfp_modules', mock.MagicMock())
    @mock.patch('sonic_platform.module_host_mgmt_initializer.ModuleHostMgmtInitializer.is_initialization_owner')
    @mock.patch('sonic_platform.module_host_mgmt_initializer.ModuleHostMgmtInitializer.wait_module_ready')
    @mock.patch('sonic_platform.utils.is_host')
    def test_initialize(self, mock_is_host, mock_wait_ready, mock_owner):
        c = chassis.Chassis()
        initializer = module_host_mgmt_initializer.ModuleHostMgmtInitializer()
        mock_is_host.return_value = True
        mock_owner.return_value = False
        # called from host side, just wait
        initializer.initialize(c)
        mock_wait_ready.assert_called_once()
        mock_wait_ready.reset_mock()
        
        mock_is_host.return_value = False
        # non-initializer-owner called from container side, just wait
        initializer.initialize(c)
        mock_wait_ready.assert_called_once()
        mock_wait_ready.reset_mock()
        
        mock_owner.return_value = True
        initializer.initialize(c)
        mock_wait_ready.assert_not_called()
        assert initializer.initialized
        assert module_host_mgmt_initializer.initialization_owner
        assert os.path.exists(module_host_mgmt_initializer.MODULE_READY_CONTAINER_FILE)
        
        module_host_mgmt_initializer.clean_up()
        assert not os.path.exists(module_host_mgmt_initializer.MODULE_READY_CONTAINER_FILE)

    def test_is_initialization_owner(self):
        initializer = module_host_mgmt_initializer.ModuleHostMgmtInitializer()
        assert not initializer.is_initialization_owner()
