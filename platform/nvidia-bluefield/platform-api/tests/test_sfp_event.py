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
@patch('sonic_platform.chassis.Chassis.get_num_sfps', MagicMock(return_value=2))
class TestSfpEvent:

    def test_sfp_event_full_flow(self, *args):
        chassis = Chassis()

        # Verify that no events are generated if SFPs are not present
        with patch('sonic_platform.sfp.Sfp.read_eeprom', MagicMock(return_value=None)) as p:
            rc, event = chassis.get_change_event(1000)

            assert rc == True
            assert 'sfp' in event
            assert event['sfp'] == {}

        # Verify that events for both ports are generated
        with patch('sonic_platform.sfp.Sfp.read_eeprom', MagicMock(return_value=bytearray([0x11]))) as p:
            rc, event = chassis.get_change_event(1000)

            assert rc == True
            assert 'sfp' in event
            assert event['sfp'] == {1: '1', 2: '1'}

        # Verify that events are not generated if port presence didn't change
        with patch('sonic_platform.sfp.Sfp.read_eeprom', MagicMock(return_value=bytearray([0x11]))) as p:
            rc, event = chassis.get_change_event(1000)

            assert rc == True
            assert 'sfp' in event
            assert event['sfp'] == {}

        # Verify that events are generated for both ports when SFPs are removed
        with patch('sonic_platform.sfp.Sfp.read_eeprom', MagicMock(return_value=None)) as p:
            rc, event = chassis.get_change_event(1000)

            assert rc == True
            assert 'sfp' in event
            assert event['sfp'] == {1: '0', 2: '0'}

    def test_sfp_event(self, *args):
        chassis = Chassis()

        # Verify that no events are generated if SFPs are not present
        with patch('sonic_platform.sfp_event.SfpEvent.get_presence_bitmap', MagicMock(return_value=0)) as p:
            rc, event = chassis.get_change_event(1000)

            assert rc == True
            assert 'sfp' in event
            assert event['sfp'] == {}

        # Insert SFP for port 1. Verify that event for only port 1 is generated
        with patch('sonic_platform.sfp_event.SfpEvent.get_presence_bitmap', MagicMock(return_value=0x1)) as p:
            rc, event = chassis.get_change_event(1000)

            assert rc == True
            assert 'sfp' in event
            assert event['sfp'] == {1: '1'}

        # Insert SFP for port 2, remove SFP for port 1. Verify that correct events are generated
        with patch('sonic_platform.sfp_event.SfpEvent.get_presence_bitmap', MagicMock(return_value=0x2)) as p:
            rc, event = chassis.get_change_event(1000)

            assert rc == True
            assert 'sfp' in event
            assert event['sfp'] == {1: '0', 2: '1'}

        # Remove SFP for port 2. Verify that event only for port
        with patch('sonic_platform.sfp_event.SfpEvent.get_presence_bitmap', MagicMock(return_value=0x0)) as p:
            rc, event = chassis.get_change_event(1000)

            assert rc == True
            assert 'sfp' in event
            assert event['sfp'] == {2: '0'}
