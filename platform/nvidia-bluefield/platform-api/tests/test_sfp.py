#
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
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

from unittest.mock import patch
from unittest.mock import mock_open
from unittest.mock import MagicMock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform.sfp import Sfp
from sonic_platform.chassis import Chassis

from .utils import platform_sample

@patch('sonic_py_common.device_info.get_platform', MagicMock(return_value=""))
@patch('sonic_py_common.device_info.get_path_to_platform_dir', MagicMock(return_value=""))
@patch('builtins.open', new_callable=mock_open, read_data=platform_sample)
@patch('os.path.isfile', MagicMock(return_value=True))
class TestSfp:

    @patch('sonic_platform.sfp.Sfp.read_eeprom', MagicMock(return_value=bytearray([0x11])))
    @patch('sonic_platform.chassis.Chassis.get_num_sfps', MagicMock(return_value=2))
    def test_get_presence(self, *args):
        chassis = Chassis()

        sfp = chassis.get_sfp(1)

        assert sfp.get_presence() == True

    @patch('sonic_platform.sfp.check_ethtool_link_detected', MagicMock(return_value=False))
    @patch('sonic_platform.chassis.Chassis.get_num_sfps', MagicMock(return_value=2))
    def test_sfp_unpluged(self, *args):
        chassis = Chassis()

        sfp = chassis.get_sfp(1)

        assert sfp.get_presence() == False
