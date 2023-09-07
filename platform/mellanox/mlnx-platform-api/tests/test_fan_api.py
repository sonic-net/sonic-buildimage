#
# Copyright (c) 2020-2023 NVIDIA CORPORATION & AFFILIATES.
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
from mock import MagicMock, patch

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform import utils
from sonic_platform.fan import Fan, PsuFan, FAN_DIR_VALUE_INTAKE, FAN_DIR_VALUE_EXHAUST
from sonic_platform.fan_drawer import RealDrawer, VirtualDrawer
from sonic_platform.psu import Psu


class TestFan:
    def test_fan_drawer_basic(self):
        # Real drawer
        fan_drawer = RealDrawer(0)
        assert fan_drawer.get_index() == 1
        assert fan_drawer.get_name() == 'drawer1'
        utils.read_int_from_file = MagicMock(return_value=1)
        assert fan_drawer.get_presence() is True
        utils.read_int_from_file = MagicMock(return_value=0)
        assert fan_drawer.get_presence() is False
        assert fan_drawer.get_position_in_parent() == 1
        assert fan_drawer.is_replaceable() is True
        fan_drawer.get_presence = MagicMock(return_value=False)
        assert fan_drawer.get_direction() == Fan.FAN_DIRECTION_NOT_APPLICABLE
        fan_drawer.get_presence = MagicMock(return_value=True)
        assert fan_drawer.get_direction() == Fan.FAN_DIRECTION_EXHAUST
        utils.read_int_from_file = MagicMock(return_value=1)
        assert fan_drawer.get_direction() == Fan.FAN_DIRECTION_INTAKE
        # Invalid fan dir value
        utils.read_int_from_file = MagicMock(return_value=2)
        assert fan_drawer.get_direction() == Fan.FAN_DIRECTION_NOT_APPLICABLE

        utils.read_int_from_file = MagicMock(side_effect=ValueError(''))
        assert fan_drawer.get_direction() == Fan.FAN_DIRECTION_NOT_APPLICABLE

        utils.read_int_from_file = MagicMock(side_effect=IOError(''))
        assert fan_drawer.get_direction() == Fan.FAN_DIRECTION_NOT_APPLICABLE

        # Virtual drawer
        fan_drawer = VirtualDrawer(0)
        assert fan_drawer.get_name() == 'N/A'
        assert fan_drawer.get_presence() is True
        assert fan_drawer.is_replaceable() is False

    def test_system_fan_basic(self):
        fan_drawer = RealDrawer(0)
        fan = Fan(2, fan_drawer, 1)
        assert fan.get_position_in_parent() == 1
        assert fan.is_replaceable() is False
        assert fan.get_speed_tolerance() == 50
        assert fan.get_name() == 'fan3'

        mock_sysfs_content = {
            fan.fan_speed_get_path: 50,
            fan.fan_max_speed_path: 100,
            fan.fan_status_path: 0,
            fan.fan_speed_set_path: 153
        }

        def mock_read_int_from_file(file_path, default=0, raise_exception=False):
            return mock_sysfs_content[file_path]

        utils.read_int_from_file = mock_read_int_from_file
        assert fan.get_speed() == 50
        mock_sysfs_content[fan.fan_speed_get_path] = 101
        assert fan.get_speed() == 100
        mock_sysfs_content[fan.fan_max_speed_path] = 0
        assert fan.get_speed() == 101

        assert fan.get_status() is True
        mock_sysfs_content[fan.fan_status_path] = 1
        assert fan.get_status() is False

        assert fan.get_target_speed() == 60

        fan.fan_drawer.get_direction = MagicMock(return_value=Fan.FAN_DIRECTION_EXHAUST)
        assert fan.get_direction() == Fan.FAN_DIRECTION_EXHAUST
        fan.fan_drawer.get_presence = MagicMock(return_value=True)
        assert fan.get_presence() is True

    @patch('sonic_platform.utils.write_file')
    def test_system_fan_set_speed(self, mock_write_file):
        fan_drawer = RealDrawer(0)
        fan = Fan(2, fan_drawer, 1)
        fan.set_speed(60)
        mock_write_file.assert_called_with(fan.fan_speed_set_path, 153, raise_exception=True)

    @patch('sonic_platform.utils.read_int_from_file')
    @patch('sonic_platform.psu.Psu.get_presence')
    @patch('sonic_platform.psu.Psu.get_powergood_status')
    @patch('os.path.exists')
    def test_psu_fan_basic(self, mock_path_exists, mock_powergood, mock_presence, mock_read_int):
        mock_path_exists.return_value = False
        psu = Psu(0)
        fan = PsuFan(0, 1, psu)
        assert fan.get_direction() == Fan.FAN_DIRECTION_NOT_APPLICABLE
        assert fan.get_status() is True
        assert fan.get_presence() is False
        mock_presence.return_value = True
        assert fan.get_presence() is False
        mock_powergood.return_value = True
        assert fan.get_presence() is False
        mock_path_exists.return_value = True
        assert fan.get_presence() is True
        mock_read_int.return_value = 7
        assert fan.get_target_speed() == 70
        mock_read_int.return_value = FAN_DIR_VALUE_INTAKE
        assert fan.get_direction() == Fan.FAN_DIRECTION_INTAKE
        mock_read_int.return_value = FAN_DIR_VALUE_EXHAUST
        assert fan.get_direction() == Fan.FAN_DIRECTION_EXHAUST
        mock_read_int.return_value = -1 # invalid value
        assert fan.get_direction() == Fan.FAN_DIRECTION_NOT_APPLICABLE

    def test_psu_fan_set_speed(self):
        psu = Psu(0)
        fan = PsuFan(0, 1, psu)
        subprocess.check_call = MagicMock()
        mock_file_content = {
            fan.psu_i2c_bus_path: 'bus',
            fan.psu_i2c_addr_path: 'addr',
            fan.psu_i2c_command_path: 'command'
        }
        def mock_read_str_from_file(file_path, default='', raise_exception=False):
            return mock_file_content[file_path]
        utils.read_str_from_file = mock_read_str_from_file
        fan.set_speed(60)
        assert subprocess.check_call.call_count == 0
        fan.get_presence = MagicMock(return_value=True)
        assert fan.set_speed(60)
        subprocess.check_call.assert_called_with(["i2cset", "-f", "-y", "bus", "addr", "command", hex(60), "wp"], universal_newlines=True)
        subprocess.check_call = MagicMock(side_effect=subprocess.CalledProcessError('', ''))
        assert not fan.set_speed(60)
        subprocess.check_call = MagicMock()
        utils.read_str_from_file = MagicMock(side_effect=RuntimeError(''))
        assert not fan.set_speed(60)
