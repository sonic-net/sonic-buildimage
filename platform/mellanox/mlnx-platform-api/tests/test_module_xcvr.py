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
"""Layer C: the module describing itself over its own EEPROM.

Two things are worth pinning here. First, which port classes carry the mixin -
an RJ45 port has a cage but no module, so it must not. Second, that the
aggregate getters written out in the mixin still say what the community bodies
they replace would have said, for a pluggable port and a CPO port alike.
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

from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase

from sonic_platform.module_xcvr import ModuleXcvrMixin
from sonic_platform.module_sysfs import ModuleSysfsMixin
from sonic_platform.cpo import CpoPort
from sonic_platform.sfp import SFP, RJ45Port


# The members the mixin must never define: the transport, because the mixin
# sits ahead of it in the port MRO, and the api lifecycle, because CpoBase's
# per-device version has to keep winning on a CPO port.
NOT_THE_MIXIN_S_TO_ANSWER = (
    'read_eeprom',
    '_read_eeprom',
    'write_eeprom',
    'get_xcvr_api',
    'refresh_xcvr_api',
    'remove_xcvr_api',
    'set_power',
)

# The aggregate getters written out in the mixin so that a CPO port answers
# them without inheriting SfpOptoeBase.
AGGREGATE_GETTERS = (
    'get_transceiver_info',
    'get_transceiver_info_firmware_versions',
    'get_transceiver_status',
    'get_transceiver_status_flags',
    'get_transceiver_dom_real_value',
    'get_transceiver_dom_flags',
    'get_transceiver_threshold_info',
    'get_transceiver_vdm_real_value_basic',
    'get_transceiver_vdm_real_value_statistic',
    'get_transceiver_vdm_flags',
    'get_transceiver_vdm_thresholds',
    'get_transceiver_pm',
    'is_transceiver_vdm_supported',
    'is_vdm_statistic_supported',
)


class TestAttachment:
    def test_ports_that_carry_a_module_get_the_xcvr_layer(self):
        assert issubclass(SFP, ModuleXcvrMixin)
        assert issubclass(CpoPort, ModuleXcvrMixin)

    def test_an_rj45_port_has_no_module_to_interrogate(self):
        assert not issubclass(RJ45Port, ModuleXcvrMixin)
        assert issubclass(RJ45Port, ModuleSysfsMixin)

    def test_xcvr_precedes_the_module_layer(self):
        """Both define get_temperature-shaped members; the xcvr layer asks first."""
        mro = SFP.__mro__
        assert mro.index(ModuleXcvrMixin) < mro.index(ModuleSysfsMixin)

    def test_power_stays_with_the_module_layer(self):
        """set_power is a host action, not something the module answers."""
        assert SFP.set_power is ModuleSysfsMixin.set_power

    def test_the_mixin_never_defines_read_eeprom(self):
        """A forwarder of that name on SFP would forward to itself."""
        assert 'read_eeprom' not in vars(ModuleXcvrMixin)
        assert '_read_eeprom' not in vars(ModuleXcvrMixin)


class TestSharedXcvrSurface:
    """The surface both branches answer, written out rather than inherited."""

    def test_the_platform_reimplements_what_it_answers_differently(self):
        """These are the platform's own, not upstream's."""
        for name in ('get_lpmode', 'set_lpmode', 'reset', 'get_error_description',
                     'get_rx_los', 'get_tx_fault', 'get_temperature'):
            assert vars(ModuleXcvrMixin)[name] is not vars(SfpOptoeBase).get(name), name

    def test_the_transport_and_api_lifecycle_stay_out_of_the_mixin(self):
        for name in NOT_THE_MIXIN_S_TO_ANSWER:
            assert name not in vars(ModuleXcvrMixin), name

    @pytest.mark.parametrize('name', AGGREGATE_GETTERS)
    def test_both_branches_answer_it_from_the_same_place(self, name):
        """One body serves a pluggable port and a CPO port alike."""
        assert getattr(SFP, name) is vars(ModuleXcvrMixin)[name], name
        assert getattr(CpoPort, name) is vars(ModuleXcvrMixin)[name], name

    @pytest.mark.parametrize('name', AGGREGATE_GETTERS)
    def test_it_says_the_same_as_the_community_body_it_replaces(self, name):
        """Written out, but not rewritten: identical for api and for no api."""
        mine, theirs = vars(ModuleXcvrMixin)[name], vars(SfpOptoeBase)[name]
        api = mock.MagicMock()
        port = mock.MagicMock(get_xcvr_api=mock.MagicMock(return_value=api))
        assert mine(port) is theirs(port)

        port.get_xcvr_api.return_value = None
        assert mine(port) is theirs(port) is None


class TestXcvrBehaviour:
    @mock.patch('sonic_platform.module_sysfs.ModuleSysfsMixin.is_sw_control')
    @mock.patch('sonic_platform.sfp.SFP.get_xcvr_api')
    def test_temperature_comes_from_the_driver_under_firmware_control(
            self, mock_api, mock_sw_control):
        mock_sw_control.return_value = False
        sfp = SFP(0)
        with mock.patch.object(sfp, 'get_temperature_from_sysfs', return_value=42.0):
            assert sfp.get_temperature() == 42.0
        mock_api.assert_not_called()

    @mock.patch('sonic_platform.module_sysfs.ModuleSysfsMixin.is_sw_control')
    @mock.patch('sonic_platform.sfp.SFP.get_xcvr_api')
    def test_temperature_comes_from_the_module_under_software_control(
            self, mock_api, mock_sw_control):
        mock_sw_control.return_value = True
        mock_api.return_value.get_module_temperature.return_value = 51.0
        sfp = SFP(0)
        with mock.patch.object(sfp, 'reinit_if_sn_changed', return_value=False):
            assert sfp.get_temperature() == 51.0

    @mock.patch('sonic_platform.module_sysfs.ModuleSysfsMixin.get_power_limit')
    @mock.patch('sonic_platform.sfp.SFP.get_module_max_power')
    def test_power_capability_compares_eeprom_against_the_cage(
            self, mock_max_power, mock_limit):
        sfp = SFP(0)
        mock_limit.return_value = 20
        mock_max_power.return_value = 16
        assert sfp.check_power_capability()
        mock_max_power.return_value = 24
        assert not sfp.check_power_capability()

    @mock.patch('sonic_platform.sfp.SFP.get_xcvr_api')
    def test_copper_linear_modules_are_refused_software_control(self, mock_api):
        sfp = SFP(0)
        with mock.patch.object(sfp, 'read_eeprom', return_value=bytearray([0x0F])):
            assert not sfp.check_media_interface_technology(mock_api.return_value)
        with mock.patch.object(sfp, 'read_eeprom', return_value=bytearray([0x01])):
            assert sfp.check_media_interface_technology(mock_api.return_value)
