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
    @mock.patch('sonic_platform.pcie.Pcie._create_device_id_to_bus_map', mock.MagicMock())
    @mock.patch('sonic_platform.pcie.Pcie.load_config_file', mock.MagicMock())
    def test_get_pcie_check(self):
        p = Pcie('')
        p._device_id_to_bus_map = {}
        p.confInfo = [
            {
                'id': '1f0b',
                'dev': '00',
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

    @mock.patch('sonic_platform.pcie.os.listdir')
    @mock.patch('sonic_platform.pcie.Pcie.load_config_file', mock.MagicMock())
    def test_create_device_id_to_bus_map(self, mock_dir):
        p = Pcie('')
        assert not p._device_id_to_bus_map
        mock_dir.return_value = ['0000:01:00.0']

        mock_os_open = mock.mock_open(read_data='0x23')
        with mock.patch('sonic_platform.pcie.open', mock_os_open):
            p._create_device_id_to_bus_map()
            assert p._device_id_to_bus_map == {'23':'01'}
