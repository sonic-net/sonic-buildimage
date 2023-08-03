#
# Copyright (c) 2021-2023 NVIDIA CORPORATION & AFFILIATES.
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
from sonic_platform.psu import FixedPsu, Psu


class TestPsu:
    def test_fixed_psu(self):
        psu = FixedPsu(0)
        assert psu.get_name() == 'PSU 1'
        assert psu.get_model() == 'N/A'
        assert psu.get_serial() == 'N/A'
        assert psu.get_revision() == 'N/A'
        avail, msg = psu.get_power_available_status()
        assert not avail
        assert msg == 'absence of power'
        utils.read_int_from_file = mock.MagicMock(return_value=1)
        assert psu.get_powergood_status()
        avail, msg = psu.get_power_available_status()
        assert avail
        assert msg == ''
        utils.read_int_from_file = mock.MagicMock(return_value=0)
        assert not psu.get_powergood_status()
        assert psu.get_presence()
        assert psu.get_voltage() is None
        assert psu.get_current() is None
        assert psu.get_power() is None
        assert psu.get_position_in_parent() == 1
        assert psu.is_replaceable() is False
        assert psu.get_temperature() is None
        assert psu.get_temperature_high_threshold() is None
        assert psu.get_input_voltage() is None
        assert psu.get_input_current() is None

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_psu(self):
        psu = Psu(0)
        assert len(psu._fan_list) == 1
        assert psu.get_fan(0).get_name() == 'psu1_fan1'
        mock_sysfs_content = {
            psu.psu_presence: 1,
            psu.psu_oper_status: 1,
            psu.psu_voltage: 10234,
            psu.psu_voltage_min: 9000,
            psu.psu_voltage_max: 12000,
            psu.psu_current: 20345,
            psu.psu_power: 30456,
            psu.psu_temp: 40567,
            psu.psu_temp_threshold: 50678,
            psu.psu_voltage_in: 102345,
            psu.psu_current_in: 676,
            psu.psu_power_max: 1234567
        }

        def mock_read_int_from_file(file_path, **kwargs):
            return mock_sysfs_content[file_path]

        utils.read_int_from_file = mock_read_int_from_file
        utils.read_str_from_file = mock.MagicMock(return_value='min max')
        assert psu.get_presence() is True
        assert psu.get_maximum_supplied_power() == 1.234567
        mock_sysfs_content[psu.psu_presence] = 0
        assert psu.get_presence() is False
        avail, msg = psu.get_power_available_status()
        assert not avail
        assert msg == 'absence of PSU'

        assert psu.get_powergood_status() is True
        mock_sysfs_content[psu.psu_oper_status] = 0
        assert psu.get_powergood_status() is False

        assert psu.get_voltage() is None
        assert psu.get_current() is None
        assert psu.get_power() is None
        assert psu.get_temperature() is None
        assert psu.get_temperature_high_threshold() is None
        assert psu.get_input_voltage() is None
        assert psu.get_input_current() is None
        assert psu.get_maximum_supplied_power() is None

        mock_sysfs_content[psu.psu_oper_status] = 1
        assert psu.get_voltage() == 10.234
        assert psu.get_voltage_high_threshold() == 12.0
        assert psu.get_voltage_low_threshold() == 9.0
        assert psu.get_current() == 20.345
        assert psu.get_power() == 0.030456
        assert psu.get_temperature() == 40.567
        assert psu.get_temperature_high_threshold() == 50.678
        assert psu.get_input_voltage() == 102.345
        assert psu.get_input_current() == 0.676

        assert psu.get_position_in_parent() == 1
        assert psu.is_replaceable() is True

    def test_psu_vpd(self):
        psu = Psu(0)
        psu.vpd_parser.vpd_file = os.path.join(test_path, 'mock_psu_vpd')

        assert psu.get_model() == 'MTEF-PSF-AC-C'
        assert psu.get_serial() == 'MT1946X07684'
        assert psu.get_revision() == 'A3'

        psu.vpd_parser.vpd_file = 'not exists'
        assert psu.get_model() == 'N/A'
        assert psu.get_serial() == 'N/A'
        assert psu.get_revision() == 'N/A'

        psu.vpd_parser.vpd_file_last_mtime = None
        psu.vpd_parser.vpd_file = os.path.join(test_path, 'mock_psu_vpd')
        assert psu.get_model() == 'MTEF-PSF-AC-C'
        assert psu.get_serial() == 'MT1946X07684'
        assert psu.get_revision() == 'A3'

        assert psu.vpd_parser.get_entry_value('MFR_NAME') == 'DELTA'

    @mock.patch('sonic_platform.utils.read_int_from_file', mock.MagicMock(return_value=9999))
    @mock.patch('sonic_platform.utils.run_command')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_platform_name')
    @mock.patch('sonic_platform.vpd_parser.VpdParser.get_entry_value')
    def test_psu_workaround(self, mock_get_entry_value, mock_get_platform_name, mock_run_command):
        from sonic_platform.psu import InvalidPsuVolWA
        psu = Psu(0)
        # Threshold value is not InvalidPsuVolWA.INVALID_VOLTAGE_VALUE
        assert InvalidPsuVolWA.run(psu, 9999, '') == 9999

        # Platform name is not in InvalidPsuVolWA.EXPECT_PLATFORMS
        mock_get_platform_name.return_value = 'some platform'
        assert InvalidPsuVolWA.run(psu, InvalidPsuVolWA.INVALID_VOLTAGE_VALUE, '') == InvalidPsuVolWA.INVALID_VOLTAGE_VALUE

        # PSU vendor is not InvalidPsuVolWA.EXPECT_VENDOR_NAME
        vpd_info = {
            InvalidPsuVolWA.MFR_FIELD: 'some psu',
            InvalidPsuVolWA.CAPACITY_FIELD: 'some capacity'
        }
        def get_entry_value(key):
            return vpd_info[key]

        mock_get_entry_value.side_effect = get_entry_value
        mock_get_platform_name.return_value = 'x86_64-mlnx_msn3700-r0'
        assert InvalidPsuVolWA.run(psu, InvalidPsuVolWA.INVALID_VOLTAGE_VALUE, '') == InvalidPsuVolWA.INVALID_VOLTAGE_VALUE

        # PSU capacity is not InvalidPsuVolWA.EXPECT_CAPACITY
        vpd_info[InvalidPsuVolWA.MFR_FIELD] = InvalidPsuVolWA.EXPECT_VENDOR_NAME
        assert InvalidPsuVolWA.run(psu, InvalidPsuVolWA.INVALID_VOLTAGE_VALUE, '') == InvalidPsuVolWA.INVALID_VOLTAGE_VALUE

        # Normal
        vpd_info[InvalidPsuVolWA.CAPACITY_FIELD] = InvalidPsuVolWA.EXPECT_CAPACITY
        assert InvalidPsuVolWA.run(psu, InvalidPsuVolWA.INVALID_VOLTAGE_VALUE, '') == 9999
        mock_run_command.assert_called_with(['sensors', '-s'])

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    @mock.patch('sonic_platform.utils.read_int_from_file')
    def test_psu_power_threshold(self, mock_read_int_from_file):
        Psu.all_psus_support_power_threshold = True
        psu = Psu(0)
        common_info = {
            psu.psu_oper_status: 1,
            psu.psu_power_max_capacity: 100000000,
            psu.AMBIENT_TEMP_CRITICAL_THRESHOLD: 65000,
            psu.AMBIENT_TEMP_WARNING_THRESHOLD: 55000,
            psu.psu_power_slope: 2
            }
        normal_data = {
            psu.PORT_AMBIENT_TEMP: 55000,
            psu.FAN_AMBIENT_TEMP: 50000,
            'warning_threshold': 98.0,
            'critical_threshold': 100.0
            }
        warning_data = {
            psu.PORT_AMBIENT_TEMP: 65000,
            psu.FAN_AMBIENT_TEMP: 60000,
            'warning_threshold': 88.0,
            'critical_threshold': 100.0
            }
        critical_data = {
            psu.PORT_AMBIENT_TEMP: 70000,
            psu.FAN_AMBIENT_TEMP: 75000,
            'warning_threshold': 68.0,
            'critical_threshold': 90.0
            }
        test_data = {}
        def mock_side_effect(value):
            if value in common_info:
                return common_info[value]
            else:
                return test_data[value]

        mock_read_int_from_file.side_effect = mock_side_effect
        test_data = normal_data
        assert psu.get_psu_power_warning_suppress_threshold() == normal_data['warning_threshold']
        assert psu.get_psu_power_critical_threshold() == normal_data['critical_threshold']

        test_data = warning_data
        assert psu.get_psu_power_warning_suppress_threshold() == warning_data['warning_threshold']
        assert psu.get_psu_power_critical_threshold() == warning_data['critical_threshold']

        test_data = critical_data
        assert psu.get_psu_power_warning_suppress_threshold() == critical_data['warning_threshold']
        assert psu.get_psu_power_critical_threshold() == critical_data['critical_threshold']

    def test_psu_not_support_power_threshold(self):
        psu = Psu(0)
        assert psu.get_psu_power_warning_suppress_threshold() is None
        assert psu.get_psu_power_critical_threshold() is None
