#
# Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES.
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
import pytest
import subprocess
import sys
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform.device_data import DeviceDataManager


class TestDeviceData:
    @mock.patch('sonic_platform.device_data.utils.read_int_from_file', mock.MagicMock(return_value=1))
    def test_is_fan_hotswapable(self):
        assert DeviceDataManager.is_fan_hotswapable()

    @mock.patch('sonic_platform.device_data.utils.read_int_from_file', mock.MagicMock(return_value=1))
    def test_get_linecard_sfp_count(self):
        assert DeviceDataManager.get_linecard_sfp_count(1) == 1

    @mock.patch('sonic_platform.device_data.utils.read_int_from_file', mock.MagicMock(return_value=1))
    def test_get_gearbox_count(self):
        assert DeviceDataManager.get_gearbox_count('') == 1

    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_name', mock.MagicMock(return_value='x86_64-mlnx_msn3420-r0'))
    def test_get_linecard_max_port_count(self):
        assert DeviceDataManager.get_linecard_max_port_count() == 0

    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_name', mock.MagicMock(return_value='x86_64-nvidia_sn2201-r0'))
    def test_get_bios_component(self):
        assert DeviceDataManager.get_bios_component() is not None

    @mock.patch('sonic_py_common.device_info.get_paths_to_platform_and_hwsku_dirs', mock.MagicMock(return_value=('', '/tmp')))
    @mock.patch('sonic_platform.device_data.utils.read_key_value_file')
    def test_is_module_host_management_mode(self, mock_read):
        mock_read.return_value = {}
        assert not DeviceDataManager.is_module_host_management_mode()
        mock_read.return_value = {'SAI_INDEPENDENT_MODULE_MODE': '1'}
        assert DeviceDataManager.is_module_host_management_mode()

    @mock.patch('sonic_py_common.device_info.get_path_to_platform_dir', mock.MagicMock(return_value='/tmp'))
    @mock.patch('sonic_platform.device_data.utils.load_json_file')
    def test_get_sfp_count(self, mock_load_json):
        mock_load_json.return_value = {
            'chassis': {
                'sfps': [1,2,3]
            }
        }
        assert DeviceDataManager.get_sfp_count() == 3

    @mock.patch('sonic_platform.device_data.time.sleep', mock.MagicMock())
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_sfp_count', mock.MagicMock(return_value=3))
    @mock.patch('sonic_platform.device_data.utils.read_int_from_file', mock.MagicMock(return_value=1))
    @mock.patch('sonic_platform.device_data.os.path.exists')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.is_module_host_management_mode')
    def test_wait_platform_ready(self, mock_is_indep, mock_exists):
        mock_exists.return_value = True
        mock_is_indep.return_value = True
        assert DeviceDataManager.wait_platform_ready()
        mock_is_indep.return_value = False
        assert DeviceDataManager.wait_platform_ready()
        mock_exists.return_value = False
        assert not DeviceDataManager.wait_platform_ready()
