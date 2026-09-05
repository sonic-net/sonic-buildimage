#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

from sonic_platform import utils
from sonic_platform.pdb import LED_POWER_STATE_FILE, Pdb


class TestPdb:
    def test_pdb_basic(self):
        pdb = Pdb(0)
        assert pdb.get_name() == 'PDB 1'
        assert pdb.get_model() == 'N/A'
        assert pdb.get_serial() == 'N/A'
        assert pdb.get_revision() == 'N/A'
        assert pdb.get_presence() is True
        assert pdb.is_replaceable() is False

    @mock.patch('os.path.exists', mock.MagicMock(return_value=False))
    def test_pdb_no_thermal(self):
        pdb = Pdb(0)
        assert not pdb._thermal_list
        assert pdb.get_temperature() is None

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_pdb(self):
        pdb = Pdb(0)
        assert len(pdb._thermal_list) == 1
        assert pdb._thermal_list[0].get_name() == 'PDB-1 Temp'

        mock_sysfs_content = {
            pdb._pwr_status: 1,
            pdb._in_voltage: 48000,
            pdb._in_current: 5000,
            pdb._in_current_scale: 2,
            pdb._in_power: 240000,
            pdb._in_power_scale: 2,
            pdb._power_max: 500000000,
        }

        def mock_read_int_from_file(file_path, **kwargs):
            return mock_sysfs_content.get(file_path)

        def mock_read_float_from_file(file_path, *args, **kwargs):
            if file_path == pdb._thermal_list[0].temperature:
                return 40567
            if file_path in (pdb._in_current_scale, pdb._in_power_scale):
                return mock_sysfs_content.get(file_path)
            return None

        utils.read_int_from_file = mock_read_int_from_file
        utils.read_float_from_file = mock_read_float_from_file

        assert pdb.get_status() is True
        assert pdb.get_powergood_status() is True
        mock_sysfs_content[pdb._pwr_status] = 0
        assert pdb.get_status() is False
        assert pdb.get_powergood_status() is False
        mock_sysfs_content[pdb._pwr_status] = 1

        assert pdb.get_input_voltage() == 48.0
        assert pdb.get_voltage() == 48.0
        assert pdb.get_input_current() == 10.0
        assert pdb.get_current() == 10.0
        assert pdb.get_input_power() == 0.48
        assert pdb.get_power() == 0.48
        assert pdb.get_maximum_supplied_power() == 500.0
        assert pdb.get_temperature() == 40.567

        mock_sysfs_content[pdb._in_voltage] = None
        assert pdb.get_input_voltage() is None
        mock_sysfs_content[pdb._in_current] = None
        assert pdb.get_input_current() is None
        mock_sysfs_content[pdb._in_power] = None
        assert pdb.get_input_power() is None
        mock_sysfs_content[pdb._power_max] = None
        assert pdb.get_maximum_supplied_power() is None

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_pdb_scaled_sensor_without_scale_file(self):
        pdb = Pdb(0)

        def mock_read_int_from_file(file_path, **kwargs):
            if file_path == pdb._in_current:
                return 3000
            return None

        utils.read_int_from_file = mock_read_int_from_file

        assert pdb.get_input_current() == 3.0

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_pdb_status_led(self):
        pdb = Pdb(0)
        mock_led_content = {
            LED_POWER_STATE_FILE: 'green',
        }

        def mock_read_str_from_file(file_path, **kwargs):
            return mock_led_content.get(file_path, '')

        utils.read_str_from_file = mock_read_str_from_file

        assert pdb.get_status_led() == 'green'

        mock_led_content[LED_POWER_STATE_FILE] = ' none '
        assert pdb.get_status_led() == 'N/A'

        mock_led_content[LED_POWER_STATE_FILE] = ''
        assert pdb.get_status_led() == 'N/A'

        mock_led_content[LED_POWER_STATE_FILE] = 'red'
        assert pdb.get_status_led() == 'red'
