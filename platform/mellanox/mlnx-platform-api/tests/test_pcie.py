#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2023-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
import sys
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform.pcie import Pcie


class TestPcie:
    def setup_method(self):
        os.environ['UNITTEST'] = '1'

    def teardown_method(self):
        if 'UNITTEST' in os.environ:
            del os.environ['UNITTEST']

    @mock.patch('sonic_platform.pcie.Pcie._create_device_id_to_bus_map', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.Pcie.load_config_file', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.Pcie.get_dpu_pcie_devices', mock.MagicMock(return_value=[]))
    def test_get_pcie_check(self):
        p = Pcie('')
        p._device_id_to_bus_map = {}
        p.confInfo = [
            {
                'id': '1f0b',
                'name': 'Some other device',
                'dev': '00',
                'bus': '00',
                'fn': '00'
            }
        ]
        info = p.get_pcie_check()
        assert info[0]['result'] == 'Failed'

        p.check_pcie_sysfs = mock.MagicMock(return_value=False)
        p._device_id_to_bus_map = {'1f0b': '00'}
        info = p.get_pcie_check()
        assert info[0]['result'] == 'Failed'

        p.check_pcie_sysfs = mock.MagicMock(return_value=True)
        info = p.get_pcie_check()
        assert info[0]['result'] == 'Passed'

    @mock.patch('sonic_platform.pcie.Pcie.load_config_file', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.Pcie.get_dpu_pcie_devices', mock.MagicMock(return_value=['0000:02:00.0']))
    def test_get_pcie_check_bluefield_detaching(self):
        p = Pcie('')
        p.confInfo = [
            {
                'id': 'c2d5',  # BULEFIELD_SOC_ID
                'bus': '02',
                'dev': '00',
                'fn': '0'
            }
        ]
        # Mock the DB response
        mock_db_instance = mock.MagicMock()
        mock_db_instance.hgetall.return_value = {'dpu_state': 'detaching'}
        p.state_db = mock_db_instance
        info = p.get_pcie_check()
        assert len(info) == 0
        # Verify the correct key was used
        expected_key = f"PCIE_DETACH_INFO|0000:02:00.0"
        mock_db_instance.hgetall.assert_called_with(expected_key)

    @mock.patch('sonic_platform.pcie.Pcie.load_config_file', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.Pcie.get_dpu_pcie_devices', mock.MagicMock(return_value=['0000:03:00.0']))
    def test_get_pcie_check_bluefield_not_detaching(self):
        p = Pcie('')
        p.confInfo = [
            {
                'id': 'a2dc',  # BLUEFIELD_CONNECTX_ID
                'bus': '03',
                'dev': '00',
                'fn': '0'
            }
        ]
        mock_db_instance = mock.MagicMock()
        mock_db_instance.hgetall.return_value = {'dpu_state': 'attached'}
        p.state_db = mock_db_instance

        # Test when check_pcie_sysfs returns False
        p.check_pcie_sysfs = mock.MagicMock(return_value=False)
        info = p.get_pcie_check()
        assert len(info) == 1
        assert info[0]['result'] == 'Failed'

        # Test when check_pcie_sysfs returns True
        p.check_pcie_sysfs = mock.MagicMock(return_value=True)
        info = p.get_pcie_check()
        assert len(info) == 1
        assert info[0]['result'] == 'Passed'

    @mock.patch('sonic_platform.pcie.Pcie.load_config_file', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.Pcie.get_dpu_pcie_devices', mock.MagicMock(return_value=[]))
    def test_get_pcie_check_bluefield_db_error(self):
        p = Pcie('')
        p.confInfo = [
            {
                'id': 'c2d5',  # BULEFIELD_SOC_ID
                'bus': '02',
                'dev': '00',
                'fn': '0'
            }
        ]
        # Mock the DB to raise an exception
        mock_db_instance = mock.MagicMock()
        mock_db_instance.hgetall.side_effect = Exception("DB Error")
        p.state_db = mock_db_instance
        info = p.get_pcie_check()
        assert info[0]['result'] == 'Failed'

    @mock.patch('sonic_platform.pcie.Pcie.load_config_file', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.Pcie.get_dpu_pcie_devices', mock.MagicMock(return_value=[]))
    def test_get_pcie_check_non_dpu_device(self):
        p = Pcie('')
        p.dpu_pcie_devices = []
        p.confInfo = [
            {
                'id': '1f0b',
                'bus': '01',
                'dev': '00',
                'fn': '00'
            }
        ]
        p._device_id_to_bus_map = {'1f0b': '01'}
        p.check_pcie_sysfs = mock.MagicMock(return_value=True)
        info = p.get_pcie_check()
        assert len(info) == 1
        assert info[0]['result'] == 'Passed'

    @mock.patch('sonic_platform.pcie.Pcie.load_config_file', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.Pcie.get_dpu_pcie_devices', mock.MagicMock(return_value=[]))
    def test_get_pcie_check_device_not_in_map(self):
        p = Pcie('')
        p.dpu_pcie_devices = []
        p.confInfo = [
            {
                'id': '1f0b',
                'name': 'Some other device',
                'dev': '00',
                'bus': '01',
                'fn': '00'
            }
        ]
        p._device_id_to_bus_map = {}
        info = p.get_pcie_check()
        assert len(info) == 1
        assert info[0]['result'] == 'Failed'

    @mock.patch('sonic_platform.pcie.os.listdir')
    @mock.patch('sonic_platform.pcie.Pcie.load_config_file', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.Pcie.get_dpu_pcie_devices', mock.MagicMock(return_value=[]))
    def test_create_device_id_to_bus_map(self, mock_dir):
        p = Pcie('')
        assert not p._device_id_to_bus_map
        mock_dir.return_value = ['0000:01:00.0']

        mock_os_open = mock.mock_open(read_data='0x23')
        with mock.patch('sonic_platform.pcie.open', mock_os_open):
            p._create_device_id_to_bus_map()
            assert p._device_id_to_bus_map == {'23':'01'}

    @mock.patch('sonic_platform.pcie.Pcie.load_config_file', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.DeviceDataManager.get_dpu_count')
    @mock.patch('sonic_platform.pcie.DeviceDataManager.get_dpu_interface')
    def test_get_dpu_pcie_devices_no_dpu(self, mock_get_interface, mock_get_count):
        mock_get_count.return_value = 0
        p = Pcie('')
        result = p.get_dpu_pcie_devices()
        assert result == []
        mock_get_interface.assert_not_called()

    @mock.patch('sonic_platform.pcie.Pcie.load_config_file', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.DeviceDataManager.get_dpu_count')
    @mock.patch('sonic_platform.pcie.DeviceDataManager.get_dpu_interface')
    def test_get_dpu_pcie_devices_with_dpu(self, mock_get_interface, mock_get_count):
        p = Pcie('')
        mock_get_count.return_value = 1
        mock_get_interface.return_value = '0000:01:00.0'
        mock_get_interface.reset_mock()
        result = p.get_dpu_pcie_devices()
        assert result == ['0000:01:00.0', '0000:01:00.0'] # 1 DPU, 2 interfaces
        assert mock_get_interface.call_count == 2

    @mock.patch('sonic_platform.pcie.Pcie.load_config_file', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.DeviceDataManager.get_dpu_count')
    @mock.patch('sonic_platform.pcie.DeviceDataManager.get_dpu_interface')
    def test_get_dpu_pcie_devices_missing_interface(self, mock_get_interface, mock_get_count):
        p = Pcie('')
        mock_get_count.return_value = 1
        mock_get_interface.reset_mock()
        mock_get_interface.side_effect = ['0000:01:00.0', None]  # Second interface is None
        result = p.get_dpu_pcie_devices()
        assert result == []
        assert mock_get_interface.call_count == 2