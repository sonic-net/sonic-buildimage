#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2023-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
import tempfile
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform.device_data import DeviceDataManager, DpuInterfaceEnum, dpu_interface_values, DEVICE_DATA


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

    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_name')
    def test_get_platform_json_config_with_config(self, mock_get_platform):
        """Test get_platform_json_config returns config for platform with platform_json configured."""
        mock_get_platform.return_value = 'x86_64-mlnx_msn4700-r0'
        config = DeviceDataManager.get_platform_json_config()
        assert config is not None
        assert 'sysfs_files' in config
        assert 'revision_map' in config
        assert config['sysfs_files'] == ['/var/run/hw-management/system/config1']
        assert '0' in config['revision_map']
        assert '1' in config['revision_map']

    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_name')
    def test_get_platform_json_config_without_config(self, mock_get_platform):
        """Test get_platform_json_config returns None for platform without platform_json configured."""
        mock_get_platform.return_value = 'x86_64-mlnx_msn3700-r0'
        config = DeviceDataManager.get_platform_json_config()
        assert config is None

    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_name')
    def test_get_platform_json_config_unknown_platform(self, mock_get_platform):
        """Test get_platform_json_config returns None for unknown platform."""
        mock_get_platform.return_value = 'x86_64-unknown-r0'
        config = DeviceDataManager.get_platform_json_config()
        assert config is None

    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_json_config')
    def test_setup_platform_json_symlink_no_config(self, mock_get_config):
        """Test setup_platform_json_symlink returns True when no config exists."""
        mock_get_config.return_value = None
        result = DeviceDataManager.setup_platform_json_symlink()
        assert result is True

    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_json_config')
    def test_setup_platform_json_symlink_empty_config(self, mock_get_config):
        """Test setup_platform_json_symlink returns True when config is empty."""
        mock_get_config.return_value = {}
        result = DeviceDataManager.setup_platform_json_symlink()
        assert result is True

    @mock.patch('sonic_platform.device_data.os.path.exists')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_json_config')
    def test_setup_platform_json_symlink_sysfs_not_exists(self, mock_get_config, mock_exists):
        """Test setup_platform_json_symlink returns True when sysfs file doesn't exist."""
        mock_get_config.return_value = {
            'sysfs_files': ['/var/run/hw-management/system/config1'],
            'revision_map': {'0': 'platform.a0.json', '1': 'platform.a1.json'}
        }
        mock_exists.return_value = False
        result = DeviceDataManager.setup_platform_json_symlink()
        assert result is True

    @mock.patch('sonic_platform.device_data.os.symlink')
    @mock.patch('sonic_platform.device_data.os.path.islink')
    @mock.patch('sonic_platform.device_data.os.path.exists')
    @mock.patch('sonic_platform.device_data.utils.read_str_from_file')
    @mock.patch('sonic_py_common.device_info.get_path_to_platform_dir')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_json_config')
    def test_setup_platform_json_symlink_single_sysfs_success(self, mock_get_config, mock_platform_dir,
                                                               mock_read_str, mock_exists, mock_islink, mock_symlink):
        """Test setup_platform_json_symlink creates symlink successfully with single sysfs file."""
        mock_get_config.return_value = {
            'sysfs_files': ['/var/run/hw-management/system/config1'],
            'revision_map': {'0': 'platform.a0.json', '1': 'platform.a1.json'}
        }
        mock_platform_dir.return_value = '/tmp/platform'
        mock_read_str.return_value = '1'
        # sysfs file exists, target file exists, platform.json doesn't exist
        mock_exists.side_effect = lambda path: path in ['/var/run/hw-management/system/config1', '/tmp/platform/platform.a1.json']
        mock_islink.return_value = False
        mock_symlink.return_value = None

        result = DeviceDataManager.setup_platform_json_symlink()
        assert result is True
        mock_symlink.assert_called_once_with('platform.a1.json', '/tmp/platform/platform.json')

    @mock.patch('sonic_platform.device_data.os.symlink')
    @mock.patch('sonic_platform.device_data.os.path.islink')
    @mock.patch('sonic_platform.device_data.os.path.exists')
    @mock.patch('sonic_platform.device_data.utils.read_str_from_file')
    @mock.patch('sonic_py_common.device_info.get_path_to_platform_dir')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_json_config')
    def test_setup_platform_json_symlink_multiple_sysfs_success(self, mock_get_config, mock_platform_dir,
                                                                 mock_read_str, mock_exists, mock_islink, mock_symlink):
        """Test setup_platform_json_symlink creates symlink successfully with multiple sysfs files."""
        mock_get_config.return_value = {
            'sysfs_files': ['/path/to/config1', '/path/to/config2'],
            'revision_map': {
                '0_0': 'platform.v1.json',
                '0_1': 'platform.v2.json',
                '1_0': 'platform.v3.json',
                '1_1': 'platform.v4.json'
            }
        }
        mock_platform_dir.return_value = '/tmp/platform'
        mock_read_str.side_effect = ['1', '0']  # Returns '1' for config1, '0' for config2
        mock_exists.side_effect = lambda path: path in ['/path/to/config1', '/path/to/config2', '/tmp/platform/platform.v3.json']
        mock_islink.return_value = False
        mock_symlink.return_value = None

        result = DeviceDataManager.setup_platform_json_symlink()
        assert result is True
        mock_symlink.assert_called_once_with('platform.v3.json', '/tmp/platform/platform.json')

    @mock.patch('sonic_platform.device_data.os.readlink')
    @mock.patch('sonic_platform.device_data.os.path.islink')
    @mock.patch('sonic_platform.device_data.os.path.exists')
    @mock.patch('sonic_platform.device_data.utils.read_str_from_file')
    @mock.patch('sonic_py_common.device_info.get_path_to_platform_dir')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_json_config')
    def test_setup_platform_json_symlink_already_correct(self, mock_get_config, mock_platform_dir,
                                                          mock_read_str, mock_exists, mock_islink, mock_readlink):
        """Test setup_platform_json_symlink returns True when symlink already points to correct target."""
        mock_get_config.return_value = {
            'sysfs_files': ['/var/run/hw-management/system/config1'],
            'revision_map': {'0': 'platform.a0.json', '1': 'platform.a1.json'}
        }
        mock_platform_dir.return_value = '/tmp/platform'
        mock_read_str.return_value = '1'
        mock_exists.side_effect = lambda path: path in ['/var/run/hw-management/system/config1', '/tmp/platform/platform.a1.json']
        mock_islink.return_value = True
        mock_readlink.return_value = 'platform.a1.json'

        result = DeviceDataManager.setup_platform_json_symlink()
        assert result is True

    @mock.patch('sonic_platform.device_data.os.symlink')
    @mock.patch('sonic_platform.device_data.os.remove')
    @mock.patch('sonic_platform.device_data.os.readlink')
    @mock.patch('sonic_platform.device_data.os.path.islink')
    @mock.patch('sonic_platform.device_data.os.path.exists')
    @mock.patch('sonic_platform.device_data.utils.read_str_from_file')
    @mock.patch('sonic_py_common.device_info.get_path_to_platform_dir')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_json_config')
    def test_setup_platform_json_symlink_wrong_target(self, mock_get_config, mock_platform_dir,
                                                       mock_read_str, mock_exists, mock_islink,
                                                       mock_readlink, mock_remove, mock_symlink):
        """Test setup_platform_json_symlink updates symlink when pointing to wrong target."""
        mock_get_config.return_value = {
            'sysfs_files': ['/var/run/hw-management/system/config1'],
            'revision_map': {'0': 'platform.a0.json', '1': 'platform.a1.json'}
        }
        mock_platform_dir.return_value = '/tmp/platform'
        mock_read_str.return_value = '1'
        mock_exists.side_effect = lambda path: path in ['/var/run/hw-management/system/config1', '/tmp/platform/platform.a1.json']
        mock_islink.return_value = True
        mock_readlink.return_value = 'platform.a0.json'  # Wrong target
        mock_remove.return_value = None
        mock_symlink.return_value = None

        result = DeviceDataManager.setup_platform_json_symlink()
        assert result is True
        mock_remove.assert_called_once_with('/tmp/platform/platform.json')
        mock_symlink.assert_called_once_with('platform.a1.json', '/tmp/platform/platform.json')

    @mock.patch('sonic_platform.device_data.os.path.islink')
    @mock.patch('sonic_platform.device_data.os.path.exists')
    @mock.patch('sonic_platform.device_data.utils.read_str_from_file')
    @mock.patch('sonic_py_common.device_info.get_path_to_platform_dir')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_json_config')
    def test_setup_platform_json_symlink_regular_file_exists(self, mock_get_config, mock_platform_dir,
                                                              mock_read_str, mock_exists, mock_islink):
        """Test setup_platform_json_symlink returns True without overwriting regular file."""
        mock_get_config.return_value = {
            'sysfs_files': ['/var/run/hw-management/system/config1'],
            'revision_map': {'0': 'platform.a0.json', '1': 'platform.a1.json'}
        }
        mock_platform_dir.return_value = '/tmp/platform'
        mock_read_str.return_value = '1'
        # platform.json exists as regular file
        mock_exists.side_effect = lambda path: path in ['/var/run/hw-management/system/config1',
                                                         '/tmp/platform/platform.a1.json',
                                                         '/tmp/platform/platform.json']
        mock_islink.return_value = False

        result = DeviceDataManager.setup_platform_json_symlink()
        assert result is True

    @mock.patch('sonic_platform.device_data.os.path.exists')
    @mock.patch('sonic_platform.device_data.utils.read_str_from_file')
    @mock.patch('sonic_py_common.device_info.get_path_to_platform_dir')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_json_config')
    def test_setup_platform_json_symlink_target_not_exists(self, mock_get_config, mock_platform_dir,
                                                            mock_read_str, mock_exists):
        """Test setup_platform_json_symlink returns False when target file doesn't exist."""
        mock_get_config.return_value = {
            'sysfs_files': ['/var/run/hw-management/system/config1'],
            'revision_map': {'0': 'platform.a0.json', '1': 'platform.a1.json'}
        }
        mock_platform_dir.return_value = '/tmp/platform'
        mock_read_str.return_value = '1'
        # sysfs exists but target file doesn't
        mock_exists.side_effect = lambda path: path == '/var/run/hw-management/system/config1'

        result = DeviceDataManager.setup_platform_json_symlink()
        assert result is False

    @mock.patch('sonic_platform.device_data.os.path.exists')
    @mock.patch('sonic_platform.device_data.utils.read_str_from_file')
    @mock.patch('sonic_py_common.device_info.get_path_to_platform_dir')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_json_config')
    def test_setup_platform_json_symlink_unknown_revision(self, mock_get_config, mock_platform_dir,
                                                           mock_read_str, mock_exists):
        """Test setup_platform_json_symlink returns True for unknown revision value."""
        mock_get_config.return_value = {
            'sysfs_files': ['/var/run/hw-management/system/config1'],
            'revision_map': {'0': 'platform.a0.json', '1': 'platform.a1.json'}
        }
        mock_platform_dir.return_value = '/tmp/platform'
        mock_read_str.return_value = '2'  # Unknown revision
        mock_exists.return_value = True

        result = DeviceDataManager.setup_platform_json_symlink()
        assert result is True

    @mock.patch('sonic_platform.device_data.os.path.exists')
    @mock.patch('sonic_platform.device_data.utils.read_str_from_file')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_json_config')
    def test_setup_platform_json_symlink_empty_sysfs_value(self, mock_get_config, mock_exists, mock_read_str):
        """Test setup_platform_json_symlink returns True when sysfs file is empty."""
        mock_get_config.return_value = {
            'sysfs_files': ['/var/run/hw-management/system/config1'],
            'revision_map': {'0': 'platform.a0.json', '1': 'platform.a1.json'}
        }
        mock_exists.return_value = True
        mock_read_str.return_value = ''

        result = DeviceDataManager.setup_platform_json_symlink()
        assert result is True

    @mock.patch('sonic_platform.device_data.os.symlink')
    @mock.patch('sonic_platform.device_data.os.path.islink')
    @mock.patch('sonic_platform.device_data.os.path.exists')
    @mock.patch('sonic_platform.device_data.utils.read_str_from_file')
    @mock.patch('sonic_py_common.device_info.get_path_to_platform_dir')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_json_config')
    def test_setup_platform_json_symlink_oserror(self, mock_get_config, mock_platform_dir,
                                                  mock_read_str, mock_exists, mock_islink, mock_symlink):
        """Test setup_platform_json_symlink returns False on OSError during symlink creation."""
        mock_get_config.return_value = {
            'sysfs_files': ['/var/run/hw-management/system/config1'],
            'revision_map': {'0': 'platform.a0.json', '1': 'platform.a1.json'}
        }
        mock_platform_dir.return_value = '/tmp/platform'
        mock_read_str.return_value = '1'
        mock_exists.side_effect = lambda path: path in ['/var/run/hw-management/system/config1', '/tmp/platform/platform.a1.json']
        mock_islink.return_value = False
        mock_symlink.side_effect = OSError("Permission denied")

        result = DeviceDataManager.setup_platform_json_symlink()
        assert result is False
