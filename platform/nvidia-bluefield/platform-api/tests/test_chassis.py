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
import socket
from collections import namedtuple

from unittest.mock import patch
from unittest.mock import mock_open
from unittest.mock import MagicMock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform.chassis import Chassis
from sonic_platform.device_data import DeviceDataManager
from .utils import platform_sample

@patch('sonic_py_common.device_info.get_platform', MagicMock(return_value=""))
@patch('sonic_py_common.device_info.get_path_to_platform_dir', MagicMock(return_value=""))
@patch('builtins.open', new_callable=mock_open, read_data=platform_sample)
@patch('os.path.isfile', MagicMock(return_value=True))
class TestChassis:
    """
    Test class to test chassis.py. The test cases covers:
        - SFP related API
    """

    def test_psu(self, *args):
        chassis = Chassis()

        assert chassis.get_num_psus() == 0
        assert chassis.get_all_psus() == []

    def test_fan(self, *args):
        chassis = Chassis()

        assert chassis.get_num_fans() == 0
        assert chassis.get_all_fans() == []
        assert chassis.get_num_fan_drawers() == 0
        assert chassis.get_all_fan_drawers() == []

    def test_sfp(self, *args):
        # Test get_num_sfps, it should not create any SFP objects
        DeviceDataManager.get_sfp_count = MagicMock(return_value=2)

        chassis = Chassis()
        assert chassis.get_num_sfps() == 2
        assert len(chassis.get_all_sfps()) == 2

        # Index out of bound, return None
        sfp = chassis.get_sfp(4)
        assert sfp is None

        # Get one SFP, other SFP list should be initialized to None
        sfp = chassis.get_sfp(1)
        assert sfp is not None

        # Get the SFP again, no new SFP created
        sfp1 = chassis.get_sfp(1)
        assert id(sfp) == id(sfp1)

        # Get another SFP, sfp_initialized_count increase
        sfp2 = chassis.get_sfp(2)
        assert sfp2 is not None

        # Get all SFPs, but there are SFP already created, only None SFP created
        sfp_list = chassis.get_all_sfps()
        assert len(sfp_list) == 2
        assert id(sfp1) == id(sfp_list[0])
        assert id(sfp2) == id(sfp_list[1])


    @patch('psutil.net_if_addrs', return_value={
        'eth0-midplane': [namedtuple('snicaddr', ['family', 'address'])(family=socket.AF_INET, address='169.254.200.1')]})
    def test_get_dpu_id(self, *args):
        chassis = Chassis()

        assert chassis.get_dpu_id() == 0
