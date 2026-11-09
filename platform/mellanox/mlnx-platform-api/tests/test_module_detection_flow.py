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
"""Layer E: the flow that brings a module from plugged to usable.

The behaviour of the individual transitions is already covered by
tests/test_sfp_sm.py, which drives the same state machine through an SFP. What
is worth pinning here is what the extraction changed: that the flow is called
into rather than inherited, that there is one state machine for the whole
system rather than one per port class, that a port joins the flow by calling
init_detection_flow, that the polling members branch on state, and that the
last inline asic0 path is gone.
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

from sonic_platform import module_detection_flow as mdf
from sonic_platform.module_detection_flow import ModuleDetectionFlow
from sonic_platform.module_sysfs import (
    SFP_SYSFS_HW_PRESENT,
    SFP_SYSFS_INTERRUPT,
    SFP_SYSFS_POWER_GOOD,
    SFP_SYSFS_PRESENT,
)
from sonic_platform import sfp as sfp_module
from sonic_platform.cpo import CpoPort
from sonic_platform.sfp import SFP, RJ45Port

FW_OWNED_STATES = (mdf.STATE_FW_CONTROL, mdf.STATE_FCP_NOT_PRESENT, mdf.STATE_FCP_PRESENT)


class _Port:
    """A port with nothing but what the flow reaches for."""

    #: As on ModuleSysfsMixin: no extra nodes unless a subclass asks.
    extra_poll_types = ()

    def __init__(self, sdk_index=0, asic_index=0, control_type=mdf.SFP_FW_CONTROL):
        # A plain attribute, as on the real ports: the flow logs sfp.sdk_index.
        self.sdk_index = sdk_index
        self._asic_index = asic_index
        self.index = sdk_index + 1

    def get_sdk_index(self):
        return self.sdk_index

    def _module_attr_path(self, name):
        return f'/sys/module/sx_core/asic{self._asic_index}/module{self.sdk_index}/{name}'


class TestAttachment:
    @pytest.mark.parametrize('port_class', [SFP, CpoPort])
    def test_a_port_calls_into_the_flow_rather_than_inheriting_it(self, port_class):
        assert not issubclass(port_class, ModuleDetectionFlow)
        # All a port owes the flow: hold the state and expose the accessors
        # StateMachine drives an entity through.
        for hook in ('get_state', 'change_state', 'on_action'):
            assert callable(getattr(port_class, hook, None)), hook

    def test_an_rj45_port_runs_no_detection_flow(self):
        for hook in ('get_state', 'change_state', 'on_action'):
            assert getattr(RJ45Port, hook, None) is None, hook

    def test_the_states_stay_reachable_through_sfp(self):
        # chassis.py and the existing tests read them as sfp.STATE_*.
        assert sfp_module.STATE_SW_CONTROL is mdf.STATE_SW_CONTROL
        assert sfp_module.EVENT_START is mdf.EVENT_START


class TestSharedMachine:
    def test_one_state_machine_serves_every_port_class(self):
        # The tables are class attributes of the flow, so a CpoPort and an SFP
        # step through the same object rather than each branch building its own.
        assert ModuleDetectionFlow.get_state_machine() is ModuleDetectionFlow.sm

    @pytest.mark.parametrize('port_class', [SFP, CpoPort])
    def test_a_port_dispatches_its_actions_through_that_one_table(self, port_class):
        ModuleDetectionFlow.get_state_machine()
        port = _Port()
        action = mock.MagicMock()
        with mock.patch.dict(ModuleDetectionFlow.action_table,
                             {mdf.ACTION_ON_START: action}):
            port_class.on_action(port, mdf.ACTION_ON_START)
        action.assert_called_once_with(port)

    def test_every_entry_action_has_an_implementation(self):
        ModuleDetectionFlow.get_state_machine()
        for action in (mdf.ACTION_ON_START, mdf.ACTION_ON_RESET, mdf.ACTION_ON_POWERED,
                       mdf.ACTION_ON_SW_CONTROL, mdf.ACTION_ON_FW_CONTROL,
                       mdf.ACTION_ON_CANCEL_WAIT, mdf.ACTION_ON_POWER_LIMIT_ERROR,
                       mdf.ACTION_FCP_ON_START):
            assert callable(ModuleDetectionFlow.action_table[action])


class TestEntry:
    def test_a_normal_port_starts_at_the_head_of_the_flow(self):
        port = _Port()
        with mock.patch('sonic_platform.device_data.DeviceDataManager.get_always_fw_control_ports',
                        return_value=None):
            ModuleDetectionFlow.init_detection_flow(port, 0)
        assert port.state == mdf.STATE_DOWN
        assert port.processing_insert_event is False

    def test_an_always_firmware_port_starts_on_the_firmware_branch(self):
        port = _Port(sdk_index=3)
        with mock.patch('sonic_platform.device_data.DeviceDataManager.get_always_fw_control_ports',
                        return_value={3}):
            ModuleDetectionFlow.init_detection_flow(port, 3)
        assert port.state == mdf.STATE_FCP_DOWN


class TestPresencePath:
    def test_the_firmware_start_action_asks_the_port_for_its_path(self):
        # This is the one path the extraction had to rewrite: it used to name
        # asic0 inline, which is wrong on a multi-asic box and carries no bank.
        port = _Port(sdk_index=5, asic_index=1)
        with mock.patch.object(ModuleDetectionFlow, 'on_event') as on_event, \
             mock.patch('sonic_platform.utils.read_int_from_file', return_value=1) as read:
            ModuleDetectionFlow.action_fcp_on_start(port)
        read.assert_called_once_with('/sys/module/sx_core/asic1/module5/present')
        on_event.assert_called_once_with(port, mdf.EVENT_PRESENT)

    def test_an_absent_module_raises_the_absence_event(self):
        port = _Port()
        with mock.patch.object(ModuleDetectionFlow, 'on_event') as on_event, \
             mock.patch('sonic_platform.utils.read_int_from_file', return_value=0):
            ModuleDetectionFlow.action_fcp_on_start(port)
        on_event.assert_called_once_with(port, mdf.EVENT_NOT_PRESENT)


class TestPolling:
    def test_a_host_driven_module_is_watched_on_presence_and_power(self):
        port = _Port()
        port.state = mdf.STATE_SW_CONTROL
        port.get_fd = mock.MagicMock(side_effect=lambda name: f'fd:{name}')
        assert ModuleDetectionFlow.get_fds_for_poling(port) == {
            SFP_SYSFS_HW_PRESENT: f'fd:{SFP_SYSFS_HW_PRESENT}',
            SFP_SYSFS_POWER_GOOD: f'fd:{SFP_SYSFS_POWER_GOOD}',
        }

    @pytest.mark.parametrize('state', FW_OWNED_STATES)
    def test_a_firmware_driven_module_is_watched_on_presence_only(self, state):
        port = _Port()
        port.state = state
        port.get_fd = mock.MagicMock(side_effect=lambda name: f'fd:{name}')
        assert ModuleDetectionFlow.get_fds_for_poling(port) == {
            SFP_SYSFS_PRESENT: f'fd:{SFP_SYSFS_PRESENT}',
        }

    def test_a_settled_module_is_pending_on_an_sdk_event(self):
        port = _Port()
        for state in mdf.STABLE_STATES:
            port.state = state
            assert ModuleDetectionFlow.in_stable_state(port)
        port.state = mdf.STATE_RESETTING
        assert not ModuleDetectionFlow.in_stable_state(port)


class TestExtraPollTypes:
    """A port adds sysfs nodes of its own to the set the flow derives from
    its state. This is how a CPO port gets its fault interrupt into the main
    poll object - the flow cannot name the port classes itself without an
    import cycle, so the port declares them instead.

    The node is an ordinary one: opened, registered and closed like any
    other. All that is special is which ports ask for it.
    """

    @staticmethod
    def _port(state=mdf.STATE_SW_CONTROL, extra=(SFP_SYSFS_INTERRUPT,), fds=None):
        port = _Port()
        port.state = state
        port.extra_poll_types = extra
        port.get_fd = mock.MagicMock(
            side_effect=lambda name: (fds or {}).get(name, f'fd:{name}'))
        return port

    def test_an_extra_node_is_polled_alongside_the_state_driven_ones(self):
        assert ModuleDetectionFlow.get_fds_for_poling(self._port()) == {
            SFP_SYSFS_HW_PRESENT: f'fd:{SFP_SYSFS_HW_PRESENT}',
            SFP_SYSFS_POWER_GOOD: f'fd:{SFP_SYSFS_POWER_GOOD}',
            SFP_SYSFS_INTERRUPT:  f'fd:{SFP_SYSFS_INTERRUPT}',
        }

    def test_a_port_that_declares_none_polls_only_the_state_driven_ones(self):
        # Every other port class keeps the set it had: the interrupt is opened
        # for the one class that asked for it and nothing else.
        assert ModuleDetectionFlow.get_fds_for_poling(self._port(extra=())) == {
            SFP_SYSFS_HW_PRESENT: f'fd:{SFP_SYSFS_HW_PRESENT}',
            SFP_SYSFS_POWER_GOOD: f'fd:{SFP_SYSFS_POWER_GOOD}',
        }

    def test_a_missing_extra_node_leaves_the_state_driven_ones_alone(self):
        # The chassis tears down the whole poll object when an fd comes back
        # None, so an interrupt node that is not there must be left out rather
        # than handed up and taken for an unready module.
        assert ModuleDetectionFlow.get_fds_for_poling(
            self._port(fds={SFP_SYSFS_INTERRUPT: None})) == {
            SFP_SYSFS_HW_PRESENT: f'fd:{SFP_SYSFS_HW_PRESENT}',
            SFP_SYSFS_POWER_GOOD: f'fd:{SFP_SYSFS_POWER_GOOD}',
        }

    @pytest.mark.parametrize('state', [mdf.STATE_SW_CONTROL] + list(FW_OWNED_STATES))
    def test_the_presence_fds_leave_the_extra_nodes_out(self, state):
        # The laser source poller follows plug state only; the interrupt
        # stays with get_change_event, whose read clears it.
        port = self._port(state=state)
        assert set(ModuleDetectionFlow.get_presence_fds_for_polling(port)) == \
            set(ModuleDetectionFlow.get_fds_for_poling(port)) - {SFP_SYSFS_INTERRUPT}

    @pytest.mark.parametrize('state', FW_OWNED_STATES)
    def test_an_extra_node_survives_the_firmware_branch(self, state):
        # The interrupt is the SDK's, not the host's, so it stays whichever
        # side owns the module.
        assert SFP_SYSFS_INTERRUPT in ModuleDetectionFlow.get_fds_for_poling(
            self._port(state=state))

    def test_an_extra_node_is_kept_across_a_control_type_transition(self):
        # refresh_poll_obj closes whatever it does not recognize as wanted.
        # Deriving both the initial set and the refresh from the same helper
        # is what stops it reading the interrupt as a leftover.
        port = self._port(state=mdf.STATE_FW_CONTROL)
        intr_fd = mock.MagicMock(name='interrupt_fd')
        intr_fd.fileno.return_value = 3
        present_fd = mock.MagicMock(name='present_fd')
        present_fd.fileno.return_value = 1
        port.get_fd = mock.MagicMock(side_effect=lambda name: {
            SFP_SYSFS_PRESENT: present_fd, SFP_SYSFS_INTERRUPT: intr_fd}[name])

        all_registered_fds = {
            1: (port, present_fd, SFP_SYSFS_PRESENT),
            3: (port, intr_fd, SFP_SYSFS_INTERRUPT),
        }
        poll_obj = mock.MagicMock()

        ModuleDetectionFlow.refresh_poll_obj(port, poll_obj, all_registered_fds)

        assert all_registered_fds[3] == (port, intr_fd, SFP_SYSFS_INTERRUPT)
        poll_obj.unregister.assert_not_called()
        intr_fd.close.assert_not_called()

    def test_a_node_not_yet_registered_is_picked_up_on_a_refresh(self):
        # The refresh reconciles against the same target set, so a node the
        # poll object does not hold yet gets registered rather than ignored.
        port = self._port()
        intr_fd = mock.MagicMock(name='interrupt_fd')
        intr_fd.fileno.return_value = 3
        hw_fd = mock.MagicMock(name='hw_present_fd')
        hw_fd.fileno.return_value = 1
        power_fd = mock.MagicMock(name='power_good_fd')
        power_fd.fileno.return_value = 2
        port.get_fd = mock.MagicMock(side_effect=lambda name: {
            SFP_SYSFS_HW_PRESENT: hw_fd, SFP_SYSFS_POWER_GOOD: power_fd,
            SFP_SYSFS_INTERRUPT: intr_fd}[name])

        all_registered_fds = {
            1: (port, hw_fd, SFP_SYSFS_HW_PRESENT),
            2: (port, power_fd, SFP_SYSFS_POWER_GOOD),
        }
        poll_obj = mock.MagicMock()

        ModuleDetectionFlow.refresh_poll_obj(port, poll_obj, all_registered_fds)

        assert all_registered_fds[3] == (port, intr_fd, SFP_SYSFS_INTERRUPT)
        poll_obj.register.assert_called_once()
        assert poll_obj.register.call_args[0][0] is intr_fd

    def test_a_node_a_port_stopped_asking_for_is_closed_like_any_other(self):
        # Nothing about the interrupt is exempt from the reconciliation: a
        # port that no longer declares it has it deregistered and closed.
        port = self._port(extra=())
        hw_fd = mock.MagicMock(); hw_fd.fileno.return_value = 1
        power_fd = mock.MagicMock(); power_fd.fileno.return_value = 2
        intr_fd = mock.MagicMock(); intr_fd.fileno.return_value = 3
        port.get_fd = mock.MagicMock(side_effect=lambda name: {
            SFP_SYSFS_HW_PRESENT: hw_fd, SFP_SYSFS_POWER_GOOD: power_fd}[name])

        all_registered_fds = {
            1: (port, hw_fd, SFP_SYSFS_HW_PRESENT),
            2: (port, power_fd, SFP_SYSFS_POWER_GOOD),
            3: (port, intr_fd, SFP_SYSFS_INTERRUPT),
        }
        poll_obj = mock.MagicMock()

        ModuleDetectionFlow.refresh_poll_obj(port, poll_obj, all_registered_fds)

        poll_obj.unregister.assert_called_once_with(intr_fd)
        intr_fd.close.assert_called_once_with()
        assert set(all_registered_fds) == {1, 2}


class TestChangeEvent:
    """fill_change_event stays on the port classes: what a state means for a
    pluggable module differs from what it means for one OE feeding four ports.
    """

    @pytest.mark.parametrize('state,expected', [
        (mdf.STATE_NOT_PRESENT, mdf.SFP_STATUS_REMOVED),
        (mdf.STATE_FCP_NOT_PRESENT, mdf.SFP_STATUS_REMOVED),
        (mdf.STATE_SW_CONTROL, mdf.SFP_STATUS_INSERTED),
        (mdf.STATE_FW_CONTROL, mdf.SFP_STATUS_INSERTED),
        (mdf.STATE_FCP_PRESENT, mdf.SFP_STATUS_INSERTED),
    ])
    def test_presence_states_report_presence(self, state, expected):
        port = _Port(sdk_index=7)
        port.state = state
        port_dict = {}
        SFP.fill_change_event(port, port_dict)
        assert port_dict == {8: expected}

    def test_a_module_over_its_power_budget_reports_the_error_bit(self):
        from sonic_platform_base.sfp_base import SfpBase

        port = _Port(sdk_index=7)
        port.state = mdf.STATE_POWER_LIMIT_ERROR
        port_dict = {}
        SFP.fill_change_event(port, port_dict)
        assert port_dict == {8: str(SfpBase.SFP_ERROR_BIT_POWER_BUDGET_EXCEEDED
                                    | SfpBase.SFP_STATUS_BIT_INSERTED)}

    def test_a_module_mid_transition_reports_nothing(self):
        port = _Port(sdk_index=7)
        port.state = mdf.STATE_RESETTING
        port_dict = {}
        SFP.fill_change_event(port, port_dict)
        assert port_dict == {}


class TestEepromReadyWait:
    """The initialization wait stays firmware-control only.

    A software-controlled port is sequenced by xcvrd, which reads the EEPROM
    itself when it powers the module up, so this wait has nothing to add. That
    holds for a CPO engine exactly as it holds for a pluggable module under
    host control; boot-time readiness is the chassis gate's business.
    """

    @staticmethod
    def _port(state, eeprom):
        port = _Port()
        port.state = state
        # The report names the port by sdk_index.
        port.sdk_index = port.get_sdk_index()
        port._read_eeprom = mock.MagicMock(return_value=eeprom)
        return port

    def test_a_firmware_controlled_port_is_read(self):
        port = self._port(mdf.STATE_FW_CONTROL, bytearray(b'\x80S'))
        ModuleDetectionFlow.wait_sfp_eeprom_ready([port], 0.1)
        port._read_eeprom.assert_called_once_with(0, 2, False)

    @pytest.mark.parametrize('state', [mdf.STATE_SW_CONTROL, mdf.STATE_DOWN])
    def test_a_software_controlled_port_is_not_read(self, state):
        port = self._port(state, None)
        ModuleDetectionFlow.wait_sfp_eeprom_ready([port], 0.1)
        port._read_eeprom.assert_not_called()

    def test_an_unreadable_port_is_retried_and_then_reported(self):
        port = self._port(mdf.STATE_FW_CONTROL, None)
        with mock.patch.object(mdf.logger, 'log_error') as log_error:
            ModuleDetectionFlow.wait_sfp_eeprom_ready([port], 0.2)
        assert port._read_eeprom.call_count > 1
        assert log_error.call_count == 1

    def test_a_readable_port_is_not_reported(self):
        port = self._port(mdf.STATE_FW_CONTROL, bytearray(b'\x80S'))
        with mock.patch.object(mdf.logger, 'log_error') as log_error:
            ModuleDetectionFlow.wait_sfp_eeprom_ready([port], 0.2)
        log_error.assert_not_called()
