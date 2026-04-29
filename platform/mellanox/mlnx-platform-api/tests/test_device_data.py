#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2023-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
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

from sonic_platform.device_data import DeviceDataManager, DpuInterfaceEnum, dpu_interface_values


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

    @mock.patch('sonic_platform.utils.get_path_to_hwsku_directory', mock.MagicMock(return_value='/tmp'))
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

    @mock.patch('sonic_py_common.device_info.get_path_to_platform_dir', mock.MagicMock(return_value='/tmp'))
    @mock.patch('sonic_platform.device_data.utils.load_json_file')
    def test_dpu_count(self, mock_load_json):
        mock_value = {
            "DPUS": {
                "dpu1": {
                    "interface": {"Ethernet224": "Ethernet0"}
                },
                "dpu2": {
                    "interface": {"Ethernet232": "Ethernet0"}
                },
                "dpu3": {
                    "interface": {"EthernetX": "EthernetY"}
                }
            },
        }
        mock_load_json.return_value = mock_value
        return_dict = DeviceDataManager.get_platform_dpus_data()
        dpu_data = mock_value["DPUS"]
        assert dpu_data == return_dict
        mock_load_json.return_value = {}
        # Data is Cached
        assert DeviceDataManager.get_platform_dpus_data() == mock_value["DPUS"]
        assert DeviceDataManager.get_dpu_count() == 3

    @mock.patch('sonic_py_common.device_info.get_path_to_platform_dir', mock.MagicMock(return_value='/tmp'))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_dpus_data')
    def test_dpu_interface_data(self, mock_load_json):
        mock_value = {
            "dpu0": {
                "midplane_interface": "dpu0",
                "interface": {
                    "Ethernet224": "Ethernet0"
                },
                "rshim_info": "rshim0",
                "bus_info": "0000:08:00.0",
                "rshim_bus_info": "0000:08:00.1"
            },
            "dpu1": {
                "midplane_interface": "dpu1",
                "interface": {
                    "Ethernet232": "Ethernet0"
                },
                "rshim_info": "rshim1",
                "bus_info": "0000:07:00.0",
                "rshim_bus_info": "0000:07:00.1"
            },
            "dpu2": {
                "midplane_interface": "dpu2",
                "interface": {
                    "Ethernet240": "Ethernet0"
                },
                "rshim_info": "rshim2",
                "bus_info": "0000:01:00.0",
                "rshim_bus_info": "0000:01:00.1"
            },
            "dpu3": {
                "midplane_interface": "dpu3",
                "interface": {
                    "Ethernet248": "Ethernet0"
                },
                "rshim_info": "rshim3",
                "bus_info": "0000:02:00.0",
                "rshim_bus_info": "0000:02:00.1"
            }
        }
        mock_load_json.return_value = mock_value
        for dpu_name in mock_value:
            for dpu_interface in dpu_interface_values:
                assert DeviceDataManager.get_dpu_interface(dpu_name, dpu_interface) == mock_value[dpu_name][dpu_interface]
        invalid_dpu_names = ["dpu4", "", "dpu"]
        invalid_interface_names = ["midplane", "rshim", "bus"]
        for interface_name in invalid_interface_names:
            assert not DeviceDataManager.get_dpu_interface("dpu0", interface_name)
        for dpu_name in invalid_dpu_names:
            assert not DeviceDataManager.get_dpu_interface(dpu_name, DpuInterfaceEnum.MIDPLANE_INT.value)
        assert not DeviceDataManager.get_dpu_interface("", "")
