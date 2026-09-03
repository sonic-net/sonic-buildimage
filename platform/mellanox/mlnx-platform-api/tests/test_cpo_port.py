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
"""Layer D: a CPO port composed rather than inherited.

The risk in dropping SFP from CpoPort's ancestry is silent loss of surface:
a method xcvrd or sfputil calls that used to arrive from SfpBase and now does
not, showing up as an AttributeError in pmon rather than at build time. So the
centre of this file is a surface check - every method of the community
contract must resolve, and the ones a CPO port cannot answer must raise rather
than quietly return None.

The rest pins the composition itself: identity held once on the optical
engine, EEPROM reaching the module through it, and both devices carrying the
port's bank.
"""

import inspect
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

from sonic_platform_base.sfp_base import SfpBase
from sonic_platform_base.sonic_xcvr.cpo.cpo_base import CpoBase, OeId
from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase

from sonic_platform.cpo_device import NvidiaSysfsElsfp, NvidiaSysfsOe
from sonic_platform.module_detection_flow import ModuleDetectionFlow
from sonic_platform.module_xcvr import ModuleXcvrMixin
from sonic_platform.module_sysfs import ModuleSysfsMixin
from sonic_platform.cpo import CPO_TYPE, CpoPort
from sonic_platform.sfp import SFP


# Everything xcvrd and sfputil invoke on a port object. Adding a call site
# upstream means adding it here; the point of the list is that the addition is
# a deliberate edit rather than a surprise in pmon.
CALLED_BY_XCVRD = (
    'get_lpmode',
    'get_presence',
    'get_temperature',
    'get_transceiver_dom_flags',
    'get_transceiver_dom_real_value',
    'get_transceiver_status',
    'get_transceiver_status_flags',
    'get_transceiver_threshold_info',
    'get_transceiver_vdm_flags',
    'get_transceiver_vdm_real_value_basic',
    'get_transceiver_vdm_real_value_statistic',
    'get_transceiver_pm',
    'get_transceiver_vdm_thresholds',
    'get_vdm_freeze_status',
    'get_vdm_unfreeze_status',
    'get_xcvr_api',
    'is_transceiver_vdm_supported',
    'is_vdm_statistic_supported',
    'remove_xcvr_api',
    'set_lpmode',
    'write_eeprom',
)

CALLED_BY_SFPUTIL = (
    'get_error_description',
    'get_lpmode',
    'get_lpmode_via_pin',
    'get_platform_specific_dom_format_map',
    'get_platform_specific_transceiver_info_format_map',
    'get_platform_specific_transceiver_status_format_map',
    'get_presence',
    'get_transceiver_info',
    'get_transceiver_info_firmware_versions',
    'get_transceiver_threshold_info',
    'get_xcvr_api',
    'read_eeprom',
    'reset',
    'set_lpmode',
    'set_lpmode_via_pin',
    'set_optoe_write_max',
    'set_power',
    'write_eeprom',
)

# A CPO port has no optoe driver behind it and no reset pin, so these cannot
# be answered. sfputil guards set_optoe_write_max with `except
# NotImplementedError` only, which is why absence would be worse than refusal.
UNSUPPORTED = (
    ('get_eeprom_path', ()),
    ('get_lpmode_via_pin', ()),
    ('get_reset_status', ()),
    ('set_lpmode_via_pin', (True,)),
    ('set_optoe_write_max', (64,)),
)


def _upstream_api_forwarders():
    """Upstream members whose whole body is 'ask the xcvr api'.

    Read out of the source rather than listed, so that a forwarder added
    upstream shows up here on the next run without anyone maintaining a list.
    """
    found = []
    for name, member in vars(SfpOptoeBase).items():
        if name.startswith('_') or not callable(member):
            continue
        try:
            body = [line.strip() for line in inspect.getsource(member).split('\n')
                    if line.strip()]
        except OSError:
            continue
        body = [line for line in body
                if not line.startswith(('def ', '#')) and '"""' not in line
                and "'''" not in line]
        if body and len(body) <= 3 and any('get_xcvr_api()' in l for l in body):
            found.append(name)
    return sorted(found)


@pytest.fixture
def cpo():
    return CpoPort(8, 2, 3, 1)


class TestAncestry:
    def test_a_cpo_port_is_composed_from_the_shared_layers(self):
        for layer in (ModuleXcvrMixin, ModuleSysfsMixin, CpoBase):
            assert issubclass(CpoPort, layer)

    def test_the_detection_flow_is_called_into_rather_than_inherited(self):
        assert not issubclass(CpoPort, ModuleDetectionFlow)
        # What a port owes the flow: the accessors StateMachine drives it through.
        for hook in ('get_state', 'change_state', 'on_action'):
            assert callable(getattr(CpoPort, hook, None)), hook

    def test_it_no_longer_borrows_the_pluggable_branch(self):
        assert not issubclass(CpoPort, SFP)
        assert not issubclass(CpoPort, SfpBase)
        assert not issubclass(CpoPort, SfpOptoeBase)


class TestSurface:
    @pytest.mark.parametrize('name', sorted(set(CALLED_BY_XCVRD + CALLED_BY_SFPUTIL)))
    def test_every_call_the_daemons_make_resolves(self, name):
        assert callable(getattr(CpoPort, name, None)), name

    def test_the_whole_api_forwarding_surface_is_covered(self):
        """Every question upstream answers by just asking the api, we answer too.

        The lists above are what the daemons call *today*, and a port that
        answers only those breaks in pmon the first time upstream adds a call
        site. This is the guard that catches that here instead: a forwarder
        appearing in SfpOptoeBase must appear in `module_xcvr` as well.
        """
        missing = [n for n in _upstream_api_forwarders() if not hasattr(CpoPort, n)]
        assert missing == [], 'write these out in module_xcvr: %s' % missing

    def test_what_needs_more_than_the_api_is_left_to_the_platform(self):
        """Upstream's non-forwarders reach for optoe state a CPO port lacks.

        Absent rather than inherited, so that wanting one is a deliberate edit
        rather than a silent read of the wrong hardware.
        """
        for name in ('get_rx_power', 'get_tx_bias', 'get_voltage',
                     'get_transceiver_bulk_status'):
            assert not hasattr(CpoPort, name), name

    @pytest.mark.parametrize('name,args', UNSUPPORTED)
    def test_what_it_cannot_answer_refuses_rather_than_lying(self, name, args, cpo):
        with pytest.raises(NotImplementedError):
            getattr(cpo, name)(*args)

    @pytest.mark.parametrize('name', [n for n in CALLED_BY_XCVRD
                                      if n.startswith('get_transceiver')])
    def test_an_aggregate_getter_asks_the_api(self, name, cpo):
        """The written-out forwarders answer from the api, like upstream's do."""
        api = mock.MagicMock()
        with mock.patch.object(cpo, 'get_xcvr_api', return_value=api):
            assert getattr(cpo, name)() is getattr(api, name).return_value

    @pytest.mark.parametrize('name', [n for n in CALLED_BY_XCVRD
                                      if n.startswith('get_transceiver')])
    def test_an_aggregate_getter_answers_nothing_without_an_api(self, name, cpo):
        with mock.patch.object(cpo, 'get_xcvr_api', return_value=None):
            assert getattr(cpo, name)() is None

    def test_the_api_lifecycle_comes_from_cpo_base(self):
        # get_xcvr_api must never be adopted from the community classes, or
        # the OE-defaulting version stops winning here.
        assert CpoPort.get_xcvr_api is CpoBase.get_xcvr_api
        assert CpoPort.refresh_xcvr_api is CpoBase.refresh_xcvr_api
        assert CpoPort.remove_xcvr_api is CpoBase.remove_xcvr_api

    def test_module_control_still_comes_from_the_sysfs_layer(self):
        assert CpoPort.set_power is ModuleSysfsMixin.set_power


class TestComposition:
    def test_it_builds_one_optical_engine_and_one_laser_source(self, cpo):
        assert isinstance(cpo.oe, NvidiaSysfsOe)
        assert isinstance(cpo.elsfp, NvidiaSysfsElsfp)
        assert cpo.hardware_id.oe_id is OeId.NVIDIA_SPC6_CPO
        # The laser source declares no id of its own; the engine's id is what
        # the factories dispatch on for both halves.
        assert cpo.hardware_id.elsfp_id is None

    def test_both_devices_address_the_same_module_and_bank(self, cpo):
        for device in (cpo.oe, cpo.elsfp):
            assert device.sdk_index == 3
            assert device.asic_index == 0
            assert device.bank == 2

    def test_instantiating_the_devices_is_what_wires_the_api_factories(self, cpo):
        assert type(cpo.oe._api_factory).__name__ == 'OeApiFactory'
        assert type(cpo.elsfp._api_factory).__name__ == 'ElsfpApiFactory'

    def test_the_port_records_its_own_position_not_the_engines(self, cpo):
        assert cpo.index == 9
        assert cpo.els_id == 1
        assert cpo.sfp_type == CPO_TYPE

    def test_identity_is_a_view_on_the_engine_not_a_second_copy(self, cpo):
        # The duplicated sdk_index is what addressed the wrong module before.
        assert cpo.sdk_index == 3
        assert cpo.get_sdk_index() == 3
        assert cpo.oe_id == 3
        assert cpo.bank_id == 2
        assert 'sdk_index' not in vars(cpo)
        assert 'bank_id' not in vars(cpo)

        cpo.oe.sdk_index = 11
        assert cpo.sdk_index == 11
        assert cpo._module_attr_path('present') == '/sys/module/sx_core/asic0/module11/present'

    def test_a_multi_asic_port_keeps_its_asic_but_renders_under_asic0(self):
        with mock.patch('sonic_py_common.multi_asic.get_asic_index_from_namespace',
                        return_value=1):
            port = CpoPort(0, 0, 5, 0, asic_id='asic1')
        # sx_core gathers every ASIC's modules under 'asic0'; the index is only
        # identity, which hw-management's per-ASIC tree consumes.
        assert port.get_asic_index() == 1
        assert port._module_attr_path('status') == '/sys/module/sx_core/asic0/module5/status'


class TestEeprom:
    def test_reads_reach_the_module_through_the_optical_engine(self, cpo):
        with mock.patch.object(cpo.oe, 'read_eeprom', return_value=b'\x18') as read:
            assert cpo.read_eeprom(128, 1) == b'\x18'
        read.assert_called_once_with(128, 1)

    def test_the_quiet_read_keeps_its_log_switch(self, cpo):
        # check_eeprom_ready_if_present depends on being able to ask quietly.
        with mock.patch.object(cpo.oe, '_read_eeprom', return_value=None) as read:
            assert cpo._read_eeprom(0, 1, log_on_error=False) is None
        read.assert_called_once_with(0, 1, False)

    def test_writes_reach_the_module_through_the_optical_engine(self, cpo):
        with mock.patch.object(cpo.oe, 'write_eeprom', return_value=True) as write:
            assert cpo.write_eeprom(0, 1, bytearray([1]))
        write.assert_called_once_with(0, 1, bytearray([1]))

    def test_the_engine_renders_the_banked_path(self, cpo):
        assert cpo.oe._get_eeprom_path(bank_id=2) == \
            '/sys/module/sx_core/asic0/module3/bank2/eeprom/pages'


class TestDetectionFlow:
    def test_a_cpo_port_starts_the_flow_at_its_engine(self, cpo):
        assert cpo.get_state() is not None
        assert cpo.processing_insert_event is False

    def test_one_engine_reports_for_all_the_ports_it_carries(self, cpo):
        from sonic_platform import module_detection_flow as mdf

        cpo.state = mdf.STATE_SW_CONTROL
        port_dict = {}
        cpo.fill_change_event(port_dict)
        assert port_dict == {i: mdf.SFP_STATUS_INSERTED
                             for i in range(cpo.index, cpo.index + CpoPort.NUMBER_OF_BANKS)}
