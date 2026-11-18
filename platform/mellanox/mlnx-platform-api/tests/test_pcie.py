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
        p._device_id_to_bus_map = {'1f0b:00': '00'}
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
        p._device_id_to_bus_map = {'1f0b:00': '01'}
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
        mock_dir.return_value = ['0000:01:22.0']

        mock_os_open = mock.mock_open(read_data='0x23')
        with mock.patch('sonic_platform.pcie.open', mock_os_open):
            p._create_device_id_to_bus_map()
            assert p._device_id_to_bus_map == {'23:22':'01'}

    @mock.patch('sonic_platform.pcie.os.listdir')
    @mock.patch('sonic_platform.pcie.Pcie.load_config_file', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.Pcie.get_dpu_pcie_devices', mock.MagicMock(return_value=[]))
    def test_create_nvme_device_list(self, mock_dir):
        p = Pcie('')
        # A name that is not a PCI address is skipped without reading its class
        mock_dir.return_value = ['0000:00:01.4', '0000:02:00.0', 'not-a-pci-device', '0000:01:00.0']
        classes = {
            '0000:00:01.4': '0x060400\n',
            '0000:02:00.0': '0x010802\n',
            '0000:01:00.0': '0x010802\n'
        }

        def mock_open(path, mode):
            return mock.mock_open(read_data=classes[path.split('/')[-2]])()

        lspci = {
            # Only the first line names the device
            '0000:01:00.0': '01:00.0 Non-Volatile memory controller: Silicon Motion, Inc. SM2268XT (DRAM-less) '
                            'NVMe SSD Controller (rev 10)\n'
                            '0001:01:00.0 Ethernet controller: Mellanox Technologies Spectrum-6 (rev a0)',
            # lspci failed
            '0000:02:00.0': None
        }
        with mock.patch('sonic_platform.pcie.open', mock_open), \
                mock.patch('sonic_platform.pcie.utils.run_command', side_effect=lambda cmd: lspci[cmd[2]]) as mock_run:
            p._create_nvme_device_list()
        # lspci is given the PCI domain too
        assert [call.args[0] for call in mock_run.call_args_list] == [
            ['lspci', '-s', '0000:01:00.0'], ['lspci', '-s', '0000:02:00.0']
        ]
        assert p._nvme_devices == [('01', '00', '0'), ('02', '00', '0')]
        assert p._nvme_names == {
            ('01', '00', '0'): 'Non-Volatile memory controller: Silicon Motion, Inc. SM2268XT (DRAM-less) '
                               'NVMe SSD Controller (rev 10)'
        }

    @mock.patch('sonic_platform.pcie.Pcie.load_config_file', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.Pcie.get_dpu_pcie_devices', mock.MagicMock(return_value=[]))
    def test_get_pcie_check_nvme(self):
        p = Pcie('')
        p.dpu_pcie_devices = []
        nvme_conf = {
            'id': '5765',
            'name': 'Non-Volatile memory controller: Device 1f9f:5765 (rev 01)',
            'bus': '01',
            'dev': '00',
            'fn': '0'
        }
        phison_name = 'Non-Volatile memory controller: Phison Electronics Corporation PS5029-E29T PCIe4 NVMe ' \
                      'Controller (DRAM-less) (rev 01)'
        # A second source SSD with another device id passes and shows its own name
        p.confInfo = [dict(nvme_conf)]
        p._device_id_to_bus_map = {'5029:00': '01'}
        p._nvme_devices = [('01', '00', '0')]
        p._nvme_names = {('01', '00', '0'): phison_name}
        p.check_pcie_sysfs = mock.MagicMock(return_value=True)
        info = p.get_pcie_check()
        assert info[0]['result'] == 'Passed'
        assert info[0]['name'] == phison_name
        p.check_pcie_sysfs.assert_called_with(bus=1, device=0, func=0)

        # Without a name from lspci the yaml name is shown
        p.confInfo = [dict(nvme_conf)]
        p._nvme_names = {}
        info = p.get_pcie_check()
        assert info[0]['result'] == 'Passed'
        assert info[0]['name'] == nvme_conf['name']

        # An SSD that drops off the bus after the list was created fails with its own name
        p.confInfo = [dict(nvme_conf)]
        p._nvme_names = {('01', '00', '0'): phison_name}
        p.check_pcie_sysfs = mock.MagicMock(return_value=False)
        info = p.get_pcie_check()
        assert info[0]['result'] == 'Failed'
        assert info[0]['name'] == phison_name
        p.check_pcie_sysfs = mock.MagicMock(return_value=True)

        # No NVMe device fails with the yaml name, even if a device with the yaml id exists
        p.confInfo = [dict(nvme_conf)]
        p._device_id_to_bus_map = {'5765:00': '01'}
        p._nvme_devices = []
        info = p.get_pcie_check()
        assert info[0]['result'] == 'Failed'
        assert info[0]['name'] == nvme_conf['name']

        # An NVMe device at another dev or fn does not match the entry
        p.confInfo = [dict(nvme_conf)]
        p._nvme_devices = [('01', '01', '0'), ('01', '00', '1')]
        p._nvme_names = {}
        info = p.get_pcie_check()
        assert info[0]['result'] == 'Failed'
        assert info[0]['name'] == nvme_conf['name']

        # The NVMe device at the entry's dev and fn passes on whatever bus it sits
        p.confInfo = [dict(nvme_conf)]
        p._nvme_devices = [('01', '00', '1'), ('05', '00', '0')]
        info = p.get_pcie_check()
        assert info[0]['result'] == 'Passed'
        p.check_pcie_sysfs.assert_called_with(bus=5, device=0, func=0)

        # Each NVMe entry needs its own NVMe device
        p.confInfo = [dict(nvme_conf), dict(nvme_conf, bus='02')]
        p._nvme_devices = [('03', '00', '0')]
        p._nvme_names = {('03', '00', '0'): phison_name}
        info = p.get_pcie_check()
        assert [item['result'] for item in info] == ['Passed', 'Failed']
        assert [item['name'] for item in info] == [phison_name, nvme_conf['name']]
        assert p._nvme_devices == [('03', '00', '0')]

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


    @mock.patch('sonic_platform.pcie.Pcie._create_device_id_to_bus_map', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.Pcie.load_config_file', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.Pcie.get_dpu_pcie_devices', mock.MagicMock(return_value=[]))
    def test_check_multiple_bus(self):
        p = Pcie('')
        p.check_pcie_sysfs = mock.MagicMock(return_value=True)

        p._device_id_to_bus_map = {'1f0b:00': '00', '1f0b:11': '01'}
        p.confInfo = [
            {
                'bus': '00',
                'id': '1f0b',
                'dev': '00',
                'fn': '00'
            },
            {
                'bus': '01',
                'id': '1f0b',
                'dev': '11',
                'fn': '00'
            },
            {
                'bus': '02',
                'id': '1f0b',
                'dev': '22',
                'fn': '00'
            }
        ]

        info = p.get_pcie_check()
        assert info[0]['result'] == 'Passed'
        assert info[1]['result'] == 'Passed'
        assert info[2]['result'] == 'Failed'
