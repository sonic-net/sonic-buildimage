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
"""Layer B: sx_core module control over sysfs.

The point of these tests is what the mixin is *not* allowed to touch. The host
below carries nothing but the three sysfs coordinates, so any member that
reaches for the xcvr api, the module's EEPROM or the detection flow's `state`
raises AttributeError here rather than on a switch.
"""

import os
import sys
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

import pytest

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform import module_sysfs
from sonic_platform.module_sysfs import ModuleSysfsMixin
from sonic_platform.nvidia_sysfs_eeprom import module_attr_path
from sonic_platform.cpo import CpoPort
from sonic_platform.sfp import SFP, RJ45Port


class _Coordinates:
    """All a module-control caller is allowed to know about a module."""

    def __init__(self, asic_index=0, sdk_index=0, bank=0):
        self.asic_index = asic_index
        self.sdk_index = sdk_index
        self.bank = bank

    def module_attr_path(self, name):
        return module_attr_path(self.sdk_index, name)


class _BareHost(ModuleSysfsMixin):
    """A host with no api, no EEPROM and no state - only coordinates."""

    def __init__(self, asic_index=0, sdk_index=0, bank=0):
        self._access = _Coordinates(asic_index, sdk_index, bank)

    @property
    def module_access(self):
        return self._access


# Every member of the mixin, as (name, args), so that a member added later
# without a decision about which layer it belongs to shows up as a gap.
READERS = [
    ('_get_module_info', ()),
    ('get_module_status', ()),
    ('get_error_info_from_sdk_error_type', ()),
    ('get_control_type', ()),
    ('get_hw_present', ()),
    ('get_power_on', ()),
    ('get_reset_state', ()),
    ('get_power_good', ()),
    ('get_power_limit', ()),
    ('get_frequency_support', ()),
    ('get_temperature_thresholds_from_sysfs', ()),
]

WRITERS = [
    ('set_control_type', (0,), 'control'),
    ('set_power', (True,), 'power_on'),
    ('set_hw_reset', (1,), 'hw_reset'),
    ('set_frequency', (1,), 'frequency'),
]


class TestAbstractAccess:
    def test_host_without_module_access_cannot_be_built(self):
        class _Forgetful(ModuleSysfsMixin):
            pass

        with pytest.raises(TypeError):
            _Forgetful()

    def test_coordinates_come_from_module_access(self):
        host = _BareHost(asic_index=3, sdk_index=17)
        assert host.get_asic_index() == 3
        assert host.get_sdk_index() == 17
        # The ASIC index is identity, not a path component: sx_core keeps the
        # modules of every ASIC under 'asic0'.
        assert host._module_attr_path('present') == '/sys/module/sx_core/asic0/module17/present'


class TestBareHostConformance:
    """No member may reach past the three coordinates."""

    @pytest.mark.parametrize('name, args', READERS)
    @mock.patch('sonic_platform.utils.read_int_from_file')
    def test_readers_only_read_sysfs(self, mock_read_int, name, args):
        mock_read_int.return_value = 1
        getattr(_BareHost(), name)(*args)
        assert mock_read_int.called
        for call in mock_read_int.call_args_list:
            assert call.args[0].startswith('/sys/module/sx_core/asic0/module0/')

    @pytest.mark.parametrize('name, args, leaf', WRITERS)
    @mock.patch('sonic_platform.utils.write_file')
    def test_writers_only_write_sysfs(self, mock_write, name, args, leaf):
        getattr(_BareHost(), name)(*args)
        assert mock_write.call_args.args[0] == f'/sys/module/sx_core/asic0/module0/{leaf}'

    @mock.patch('sonic_platform.utils.read_int_from_file')
    def test_module_status_maps_sdk_codes(self, mock_read_int):
        for sdk_code, expected in module_sysfs.SDK_STATUS_TO_SONIC_STATUS.items():
            mock_read_int.return_value = sdk_code
            assert _BareHost().get_module_status() == expected

    @mock.patch('sonic_platform.utils.read_int_from_file')
    def test_module_info_reads_status_then_statuserror(self, mock_read_int):
        mock_read_int.side_effect = [1, 3]
        assert _BareHost()._get_module_info() == (1, 3)
        assert [c.args[0] for c in mock_read_int.call_args_list] == [
            '/sys/module/sx_core/asic0/module0/status',
            '/sys/module/sx_core/asic0/module0/statuserror',
        ]

    @mock.patch('sonic_platform.utils.read_int_from_file')
    def test_unknown_error_type_is_reported_not_guessed(self, mock_read_int):
        mock_read_int.return_value = 0xdead
        state, description = _BareHost().get_error_info_from_sdk_error_type()
        assert state == module_sysfs.SFP_STATUS_ERROR
        assert '57005' in description

    @mock.patch('sonic_platform.utils.read_int_from_file')
    def test_blocking_errors_carry_the_blocking_bit(self, mock_read_int):
        for error_type in module_sysfs.SDK_SFP_BLOCKING_ERRORS:
            mock_read_int.return_value = error_type
            state, _ = _BareHost().get_error_info_from_sdk_error_type()
            assert int(state) & module_sysfs.SfpBase.SFP_ERROR_BIT_BLOCKING

    @mock.patch('sonic_platform.module_sysfs.open')
    def test_get_fd_opens_the_named_leaf(self, mock_open):
        _BareHost().get_fd('present')
        assert mock_open.call_args.args[0] == '/sys/module/sx_core/asic0/module0/present'

    @mock.patch('sonic_platform.module_sysfs.open')
    def test_missing_leaf_yields_no_fd(self, mock_open):
        mock_open.side_effect = FileNotFoundError
        assert _BareHost().get_fd('present') is None

    @mock.patch('sonic_platform.module_sysfs.open')
    def test_legacy_polling_watches_present(self, mock_open):
        _BareHost().get_fd_for_polling_legacy()
        assert mock_open.call_args.args[0] == '/sys/module/sx_core/asic0/module0/present'

    @mock.patch('sonic_platform.utils.read_int_from_file')
    @mock.patch('sonic_platform.module_sysfs.os.path.exists')
    def test_temperature_is_scaled(self, mock_exists, mock_read_int):
        mock_exists.return_value = True
        mock_read_int.return_value = 8 * 42
        assert _BareHost().get_temperature_from_sysfs() == 42.0

    @mock.patch('sonic_platform.module_sysfs.os.path.exists')
    def test_absent_temperature_node_yields_none(self, mock_exists):
        mock_exists.return_value = False
        assert _BareHost().get_temperature_from_sysfs() is None

    @mock.patch('sonic_platform.utils.read_int_from_file')
    def test_thresholds_are_scaled(self, mock_read_int):
        mock_read_int.side_effect = [8 * 70, 8 * 80]
        assert _BareHost().get_temperature_thresholds_from_sysfs() == (70.0, 80.0)

    @mock.patch('sonic_platform.device_data.DeviceDataManager.is_module_host_management_mode')
    def test_sw_control_is_false_off_host_management(self, mock_mode):
        mock_mode.return_value = False
        assert _BareHost().is_sw_control() is False

    @mock.patch('sonic_platform.utils.read_int_from_file')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.is_module_host_management_mode')
    def test_missing_control_node_is_an_error_not_a_default(self, mock_mode, mock_read_int):
        mock_mode.return_value = True
        mock_read_int.side_effect = OSError
        with pytest.raises(Exception):
            _BareHost().is_sw_control()


class TestAttachment:
    """Which port classes carry module control, and in what order."""

    @pytest.mark.parametrize('cls', [SFP, RJ45Port, CpoPort])
    def test_every_port_class_drives_a_module(self, cls):
        assert issubclass(cls, ModuleSysfsMixin)

    @pytest.mark.parametrize('cls', [SFP, RJ45Port])
    def test_module_control_precedes_the_community_bases(self, cls):
        """set_power exists on both sides; the module-side one has to win."""
        mro = cls.__mro__
        from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
        assert mro.index(ModuleSysfsMixin) < mro.index(SfpOptoeBase)
        assert cls.set_power is ModuleSysfsMixin.set_power

    def test_a_cpo_port_takes_the_module_side_set_power_too(self):
        # It has no SfpOptoeBase to lose to, but the assertion is the same one.
        assert CpoPort.set_power is ModuleSysfsMixin.set_power

    def test_a_pluggable_port_is_its_own_access_object(self):
        sfp = SFP(7)
        assert sfp.module_access is sfp
        assert sfp._module_attr_path('present') == '/sys/module/sx_core/asic0/module7/present'

    def test_a_cpo_port_defers_to_its_optical_engine(self):
        cpo = CpoPort(8, 2, 3, 1)
        assert cpo.module_access is cpo.oe
        # The OE it points at, not the port's own chassis position.
        assert cpo._module_attr_path('present') == '/sys/module/sx_core/asic0/module3/present'
