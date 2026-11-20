#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
from datetime import datetime

import pytest

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform import chassis
from sonic_platform import sfp
from sonic_platform.cpo import CpoPort
from sonic_platform.module_sysfs import SFP_SYSFS_INTERRUPT


class TestChangeEventSeekFailure:
    """Cover OSError from fd.seek(0) in change-event polling loops."""

    @mock.patch('sonic_platform.chassis.Chassis.get_asic_change_event')
    @mock.patch('sonic_platform.sfp.SFP.get_fd_for_polling_legacy')
    @mock.patch('select.poll')
    @mock.patch('sonic_platform.chassis.time.sleep')
    @mock.patch('time.monotonic')
    @mock.patch(
        'sonic_platform.device_data.DeviceDataManager.is_module_host_management_mode',
        mock.MagicMock(return_value=False),
    )
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_sfp_count', mock.MagicMock(return_value=1))
    @mock.patch('sonic_platform.chassis.extract_RJ45_ports_index', mock.MagicMock(return_value=[]))
    @mock.patch('sonic_platform.chassis.build_cpo_port_map', mock.MagicMock(return_value=None))
    @mock.patch('sonic_platform.chassis.build_cpo_service_port_map', mock.MagicMock(return_value=None))
    @mock.patch('sonic_platform.sfp.SFP.get_module_status')
    @mock.patch('sonic_platform.chassis.Chassis.wait_sfp_ready_for_use', mock.MagicMock(return_value=True))
    def test_get_change_event_legacy_seek_fails(self, mock_status, mock_time, mock_sleep, mock_create_poll, mock_get_fd, mock_get_asic_event):
        c = chassis.Chassis()
        c.get_sfp(1)
        mock_status.return_value = sfp.SFP_STATUS_INSERTED
        mock_get_asic_event.return_value = {}

        mock_poll = mock.MagicMock()
        mock_create_poll.return_value = mock_poll
        mock_poll.poll = mock.MagicMock(side_effect=[[(1, 10)], []])

        mock_file = mock.MagicMock()
        mock_get_fd.return_value = mock_file
        mock_file.fileno = mock.MagicMock(return_value=1)
        mock_file.seek.side_effect = OSError(5, 'seek failed')

        mock_time.side_effect = [0, 0, 0.1, 0.9, 1000]

        _, change_event = c.get_change_event(1000)
        assert 'sfp' in change_event and not change_event['sfp']
        mock_file.seek.assert_called_with(0)
        mock_file.read.assert_not_called()
        assert abs(mock_sleep.call_args[0][0] - 0.9) < 0.000001

    @mock.patch('sonic_platform.chassis.Chassis.get_asic_change_event')
    @mock.patch('sonic_platform.sfp.SFP.get_fd_for_polling_legacy')
    @mock.patch('select.poll')
    @mock.patch('sonic_platform.chassis.time.sleep')
    @mock.patch('time.monotonic')
    @mock.patch(
        'sonic_platform.device_data.DeviceDataManager.is_module_host_management_mode',
        mock.MagicMock(return_value=False),
    )
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_sfp_count', mock.MagicMock(return_value=1))
    @mock.patch('sonic_platform.chassis.extract_RJ45_ports_index', mock.MagicMock(return_value=[]))
    @mock.patch('sonic_platform.chassis.build_cpo_port_map', mock.MagicMock(return_value=None))
    @mock.patch('sonic_platform.sfp.SFP.get_module_status')
    @mock.patch('sonic_platform.chassis.Chassis.wait_sfp_ready_for_use', mock.MagicMock(return_value=True))
    def test_get_change_event_legacy_seek_fails_without_sleep_after_timeout(
        self, mock_status, mock_time, mock_sleep, mock_create_poll, mock_get_fd, mock_get_asic_event,
    ):
        c = chassis.Chassis()
        c.get_sfp(1)
        mock_status.return_value = sfp.SFP_STATUS_INSERTED
        mock_get_asic_event.return_value = {}

        mock_poll = mock.MagicMock()
        mock_create_poll.return_value = mock_poll
        mock_poll.poll = mock.MagicMock(side_effect=[[(1, 10)], []])

        mock_file = mock.MagicMock()
        mock_get_fd.return_value = mock_file
        mock_file.fileno = mock.MagicMock(return_value=1)
        mock_file.seek.side_effect = OSError(5, 'seek failed')

        mock_time.side_effect = [0, 0, 1000, 1000, 1000]

        _, change_event = c.get_change_event(1000)
        assert 'sfp' in change_event and not change_event['sfp']
        mock_sleep.assert_not_called()

    @mock.patch('sonic_platform.chassis.Chassis.get_asic_change_event')
    @mock.patch('sonic_platform.wait_sfp_ready_task.WaitSfpReadyTask.get_ready_set')
    @mock.patch('sonic_platform.sfp.SFP.get_fd')
    @mock.patch('select.poll')
    @mock.patch('sonic_platform.chassis.time.sleep')
    @mock.patch('time.monotonic')
    @mock.patch(
        'sonic_platform.device_data.DeviceDataManager.is_module_host_management_mode',
        mock.MagicMock(return_value=True),
    )
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_sfp_count', mock.MagicMock(return_value=1))
    @mock.patch('sonic_platform.chassis.extract_RJ45_ports_index', mock.MagicMock(return_value=[]))
    @mock.patch('sonic_platform.chassis.build_cpo_port_map', mock.MagicMock(return_value=None))
    @mock.patch('sonic_platform.chassis.build_cpo_service_port_map', mock.MagicMock(return_value=None))
    @mock.patch('sonic_platform.module_host_mgmt_initializer.ModuleHostMgmtInitializer.initialize', mock.MagicMock())
    def test_get_change_event_module_host_management_seek_fails(
        self, mock_time, mock_sleep, mock_create_poll, mock_get_fd, mock_ready, mock_get_asic_event,
    ):
        c = chassis.Chassis()
        c.initialize_sfp()
        s = c._sfp_list[0]
        s.state = sfp.STATE_SW_CONTROL

        mock_poll = mock.MagicMock()
        mock_create_poll.return_value = mock_poll
        mock_poll.poll = mock.MagicMock(side_effect=[[(1, 10)], []])
        mock_get_asic_event.return_value = {}

        mock_hw_present_file = mock.MagicMock()
        mock_power_good_file = mock.MagicMock()
        mock_hw_present_file.read = mock.MagicMock(return_value=sfp.SFP_STATUS_INSERTED)
        mock_hw_present_file.fileno = mock.MagicMock(return_value=1)
        mock_hw_present_file.seek.side_effect = OSError(5, 'seek failed')
        mock_power_good_file.read = mock.MagicMock(return_value=1)
        mock_power_good_file.fileno = mock.MagicMock(return_value=2)

        def get_fd(fd_type):
            if fd_type == 'hw_present':
                return mock_hw_present_file
            if fd_type == 'power_good':
                return mock_power_good_file
            return mock.MagicMock()

        mock_get_fd.side_effect = get_fd
        mock_ready.return_value = set()

        mock_time.side_effect = [0, 0, 0.1, 0.9, 1000]

        _, change_event = c.get_change_event(1000)
        assert 'sfp' in change_event and not change_event['sfp']
        mock_hw_present_file.seek.assert_called_with(0)
        assert abs(mock_sleep.call_args[0][0] - 0.9) < 0.000001

    @mock.patch('sonic_platform.chassis.Chassis.get_asic_change_event')
    @mock.patch('sonic_platform.wait_sfp_ready_task.WaitSfpReadyTask.get_ready_set')
    @mock.patch('sonic_platform.sfp.SFP.get_fd')
    @mock.patch('select.poll')
    @mock.patch('sonic_platform.chassis.time.sleep')
    @mock.patch('time.monotonic')
    @mock.patch(
        'sonic_platform.device_data.DeviceDataManager.is_module_host_management_mode',
        mock.MagicMock(return_value=True),
    )
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_sfp_count', mock.MagicMock(return_value=1))
    @mock.patch('sonic_platform.chassis.extract_RJ45_ports_index', mock.MagicMock(return_value=[]))
    @mock.patch('sonic_platform.chassis.build_cpo_port_map', mock.MagicMock(return_value=None))
    @mock.patch('sonic_platform.module_host_mgmt_initializer.ModuleHostMgmtInitializer.initialize', mock.MagicMock())
    def test_get_change_event_module_host_management_seek_fails_without_sleep_after_timeout(
        self, mock_time, mock_sleep, mock_create_poll, mock_get_fd, mock_ready, mock_get_asic_event,
    ):
        c = chassis.Chassis()
        c.initialize_sfp()
        s = c._sfp_list[0]
        s.state = sfp.STATE_SW_CONTROL

        mock_poll = mock.MagicMock()
        mock_create_poll.return_value = mock_poll
        mock_poll.poll = mock.MagicMock(side_effect=[[(1, 10)], []])
        mock_get_asic_event.return_value = {}

        mock_hw_present_file = mock.MagicMock()
        mock_power_good_file = mock.MagicMock()
        mock_hw_present_file.read = mock.MagicMock(return_value=sfp.SFP_STATUS_INSERTED)
        mock_hw_present_file.fileno = mock.MagicMock(return_value=1)
        mock_hw_present_file.seek.side_effect = OSError(5, 'seek failed')
        mock_power_good_file.read = mock.MagicMock(return_value=1)
        mock_power_good_file.fileno = mock.MagicMock(return_value=2)

        def get_fd(fd_type):
            if fd_type == 'hw_present':
                return mock_hw_present_file
            if fd_type == 'power_good':
                return mock_power_good_file
            return mock.MagicMock()

        mock_get_fd.side_effect = get_fd
        mock_ready.return_value = set()

        mock_time.side_effect = [0, 0, 1000, 1000, 1000]

        _, change_event = c.get_change_event(1000)
        assert 'sfp' in change_event and not change_event['sfp']
        mock_sleep.assert_not_called()


class TestChangeEvent:
    @mock.patch('sonic_platform.chassis.Chassis.get_asic_change_event')
    @mock.patch('sonic_platform.sfp.SFP.get_fd_for_polling_legacy')
    @mock.patch('select.poll')
    @mock.patch('time.monotonic')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.is_module_host_management_mode', mock.MagicMock(return_value=False))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_sfp_count', mock.MagicMock(return_value=1))
    @mock.patch('sonic_platform.chassis.extract_RJ45_ports_index', mock.MagicMock(return_value=[]))
    @mock.patch('sonic_platform.chassis.build_cpo_port_map', mock.MagicMock(return_value=None))
    @mock.patch('sonic_platform.chassis.build_cpo_service_port_map', mock.MagicMock(return_value=None))
    @mock.patch('sonic_platform.sfp.SFP.get_module_status')
    @mock.patch('sonic_platform.chassis.Chassis.wait_sfp_ready_for_use', mock.MagicMock(return_value=True))
    def test_get_change_event_legacy(self, mock_status, mock_time, mock_create_poll, mock_get_fd, mock_get_asic_event):
        c = chassis.Chassis()
        s = c.get_sfp(1)

        mock_status.return_value = sfp.SFP_STATUS_INSERTED
        mock_get_asic_event.return_value = {}
        # mock poll object
        mock_poll = mock.MagicMock()
        mock_create_poll.return_value = mock_poll
        mock_poll.poll = mock.MagicMock(return_value = [])

        # mock file descriptor for polling
        mock_file = mock.MagicMock()
        mock_get_fd.return_value = mock_file
        mock_file.fileno = mock.MagicMock(return_value = 1)

        timeout = 1000
        # mock time function so that the while loop exit early
        mock_time.side_effect = [0, 0, timeout]

        # no event, expect returning empty change event
        _, change_event = c.get_change_event(timeout)
        assert 'sfp' in change_event and not change_event['sfp']

        # dummy event, expect returning empty change event
        sfp_index = s.sdk_index + 1
        mock_poll.poll.return_value = [(1, 10)]
        mock_time.side_effect = [0, 0, timeout]
        _, change_event = c.get_change_event(timeout)
        assert 'sfp' in change_event and not change_event['sfp']

        # plug out event, expect returning remove event
        mock_time.side_effect = [0, timeout]
        mock_status.return_value = sfp.SFP_STATUS_REMOVED
        _, change_event = c.get_change_event(timeout)
        assert 'sfp' in change_event and sfp_index in change_event['sfp'] and change_event['sfp'][sfp_index] == sfp.SFP_STATUS_REMOVED

        # error event, expect returning error event
        mock_time.side_effect = [0, timeout]
        mock_status.return_value = sfp.SFP_STATUS_ERROR
        s.get_error_info_from_sdk_error_type = mock.MagicMock(return_value=('2', 'some error'))
        _, change_event = c.get_change_event(timeout)
        assert 'sfp' in change_event and sfp_index in change_event['sfp'] and change_event['sfp'][sfp_index] == '2'
        assert 'sfp_error' in change_event and sfp_index in change_event['sfp_error'] and change_event['sfp_error'][sfp_index] == 'some error'
    
    @mock.patch('sonic_platform.chassis.Chassis.get_asic_change_event')
    @mock.patch('sonic_platform.wait_sfp_ready_task.WaitSfpReadyTask.get_ready_set')    
    @mock.patch('sonic_platform.sfp.SFP.get_fd')
    @mock.patch('select.poll')
    @mock.patch('time.monotonic')
    @mock.patch('sonic_platform.chassis.utils.read_int_from_file', mock.MagicMock(return_value=1))
    @mock.patch('sonic_platform.chassis.os.path.exists', mock.MagicMock(return_value=True))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.is_module_host_management_mode', mock.MagicMock(return_value=True))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_sfp_count', mock.MagicMock(return_value=1))
    @mock.patch('sonic_platform.chassis.extract_RJ45_ports_index', mock.MagicMock(return_value=[]))
    @mock.patch('sonic_platform.chassis.build_cpo_port_map', mock.MagicMock(return_value=None))
    @mock.patch('sonic_platform.chassis.build_cpo_service_port_map', mock.MagicMock(return_value=None))
    @mock.patch('sonic_platform.module_host_mgmt_initializer.ModuleHostMgmtInitializer.initialize', mock.MagicMock())
    def test_get_change_event_for_module_host_management_mode(self, mock_time, mock_create_poll, mock_get_fd, mock_ready, mock_get_asic_event):
        """Test steps:
            1. Simulate polling with no event
            2. Simulate polling the first dummy event. (SDK always return a event when first polling the fd even if there is no change)
            3. Simulate a plug out event, module transfer from sw control to not present
            4. Simulate plugging in a fw control module, module transfer to fw control
            5. Simulate an error event
            6. Simulate a plug out event, module transfer from fw control to not present
            7. Simulate plugging in a sw control module, module transfer to sw control
            8. Simulate a power bad event, module transfer from sw control to power bad
            9. Simulate a power good event, module transfer from power bad to sw control
        """
        c = chassis.Chassis()
        c.initialize_sfp()
        s = c._sfp_list[0]
        s.state = sfp.STATE_SW_CONTROL
        
        # mock poll object
        mock_poll = mock.MagicMock()
        mock_create_poll.return_value = mock_poll
        mock_poll.poll = mock.MagicMock(return_value = [])
        
        mock_get_asic_event.return_value = {}
        # mock file descriptors for polling
        mock_hw_present_file = mock.MagicMock()
        mock_power_good_file = mock.MagicMock()
        mock_present_file = mock.MagicMock()
        mock_hw_present_file.read = mock.MagicMock(return_value=sfp.SFP_STATUS_INSERTED)
        mock_hw_present_file.fileno = mock.MagicMock(return_value = 1)
        mock_power_good_file.read = mock.MagicMock(return_value=1)
        mock_power_good_file.fileno = mock.MagicMock(return_value = 2)
        mock_present_file.read = mock.MagicMock(return_value=sfp.SFP_STATUS_INSERTED)
        mock_present_file.fileno = mock.MagicMock(return_value = 3)
        def get_fd(fd_type):
            if fd_type == 'hw_present':
                return mock_hw_present_file
            elif fd_type == 'power_good':
                return mock_power_good_file
            else:
                return mock_present_file
        mock_get_fd.side_effect = get_fd

        timeout = 1000
        # mock time function so that the while loop exit early
        mock_time.side_effect = [0, 0, timeout]
        
        # no event, expect returning empty change event
        _, change_event = c.get_change_event(timeout)
        assert 'sfp' in change_event and not change_event['sfp']
        
        # dummy event, expect returning empty change event
        sfp_index = s.sdk_index + 1
        mock_poll.poll.return_value = [(1, 10)]
        mock_time.side_effect = [0, 0, timeout]
        _, change_event = c.get_change_event(timeout)
        assert 'sfp' in change_event and not change_event['sfp']
        
        # plug out event, expect returning remove event
        mock_time.side_effect = [0, timeout]
        mock_hw_present_file.read.return_value = sfp.SFP_STATUS_REMOVED
        _, change_event = c.get_change_event(timeout)
        assert 'sfp' in change_event and sfp_index in change_event['sfp'] and change_event['sfp'][sfp_index] == sfp.SFP_STATUS_REMOVED
        assert s.state == sfp.STATE_NOT_PRESENT
        
        # plug in with a fw control cable, expect returning insert event
        s.get_control_type = mock.MagicMock(return_value=sfp.SFP_SW_CONTROL)
        s.get_hw_present = mock.MagicMock(return_value=True)
        s.get_power_on = mock.MagicMock(return_value=True)
        s.get_reset_state = mock.MagicMock(return_value=True)
        s.get_power_good = mock.MagicMock(return_value=True)
        s.determine_control_type = mock.MagicMock(return_value=sfp.SFP_FW_CONTROL)
        s.set_control_type = mock.MagicMock()
        mock_time.side_effect = [0, 0, timeout]
        mock_ready.return_value = set([0])
        mock_hw_present_file.read.return_value = sfp.SFP_STATUS_INSERTED
        _, change_event = c.get_change_event(timeout)
        assert 'sfp' in change_event and sfp_index in change_event['sfp'] and change_event['sfp'][sfp_index] == sfp.SFP_STATUS_INSERTED
        assert s.state == sfp.STATE_FW_CONTROL
        assert 1 not in c.registered_fds # stop polling hw_present
        assert 2 not in c.registered_fds # stop polling power_good
        assert 3 in c.registered_fds # start polling present because it is firmware control
        print(c.registered_fds)
        
        # error event, expect returning error
        mock_ready.return_value = []
        mock_time.side_effect = [0, timeout]
        mock_poll.poll.return_value = [(3, 10)]
        mock_present_file.read.return_value = sfp.SFP_STATUS_ERROR
        s.get_error_info_from_sdk_error_type = mock.MagicMock(return_value=('2', 'some error'))
        _, change_event = c.get_change_event(timeout)
        assert 'sfp' in change_event and sfp_index in change_event['sfp'] and change_event['sfp'][sfp_index] == '2'
        assert 'sfp_error' in change_event and sfp_index in change_event['sfp_error'] and change_event['sfp_error'][sfp_index] == 'some error'
        
        # plug out the firmware control cable, expect returning remove event
        mock_time.side_effect = [0, timeout]
        mock_present_file.read.return_value = sfp.SFP_STATUS_REMOVED
        _, change_event = c.get_change_event(timeout)
        assert 'sfp' in change_event and sfp_index in change_event['sfp'] and change_event['sfp'][sfp_index] == sfp.SFP_STATUS_REMOVED
        assert s.state == sfp.STATE_NOT_PRESENT
        assert 1 in c.registered_fds # start polling hw_present because cable is not present, always assume software control
        assert 2 in c.registered_fds # start polling power_good because cable is not present, always assume software control
        assert 3 not in c.registered_fds # stop polling present
        
        # plug in a software control cable, expect returning insert event
        mock_time.side_effect = [0, 0, timeout]
        mock_ready.return_value = set([0])
        mock_poll.poll.return_value = [(1, 10)]
        mock_hw_present_file.read.return_value = sfp.SFP_STATUS_INSERTED
        s.determine_control_type.return_value = sfp.SFP_SW_CONTROL
        s.check_power_capability = mock.MagicMock(return_value=True)
        s.update_i2c_frequency = mock.MagicMock()
        s.disable_tx_for_sff_optics = mock.MagicMock()
        _, change_event = c.get_change_event(timeout)
        assert 'sfp' in change_event and sfp_index in change_event['sfp'] and change_event['sfp'][sfp_index] == sfp.SFP_STATUS_INSERTED
        assert s.state == sfp.STATE_SW_CONTROL
        
        # power bad event, expect returning error event
        mock_time.side_effect = [0, timeout]
        mock_poll.poll.return_value = [(2, 10)]
        mock_power_good_file.read.return_value = '0'
        _, change_event = c.get_change_event(timeout)
        assert 'sfp' in change_event and sfp_index in change_event['sfp'] and change_event['sfp'][sfp_index] == '5'
        assert s.state == sfp.STATE_POWER_BAD
        
        # power good event, expect returning insert event
        mock_time.side_effect = [0, timeout]
        mock_poll.poll.return_value = [(2, 10)]
        mock_power_good_file.read.return_value = '1'
        _, change_event = c.get_change_event(timeout)
        assert 'sfp' in change_event and sfp_index in change_event['sfp'] and change_event['sfp'][sfp_index] == '1'
        assert s.state == sfp.STATE_SW_CONTROL


def _fake_cpo_port(oe_id, els_id, bank_id=0, sfp_index=0):
    """CpoPort-shaped stub for the enqueue seam, which only reads identity."""
    stub = mock.MagicMock(spec=CpoPort)
    stub.oe_id = oe_id
    stub.els_id = els_id
    stub.bank_id = bank_id
    stub.index = sfp_index
    return stub


class TestEnqueueCpoFaultInterrupt:
    """Chassis._enqueue_cpo_fault_interrupt is the seam Chassis uses to hand a
    fault-interrupt assertion to FaultIndicationTask. It starts the task if it
    is not already running - registering the interrupt fds normally gets there
    first - then forwards (cpo_port, arrival_time) to enqueue_interrupt.
    """

    def _fresh_chassis(self):
        c = chassis.Chassis.__new__(chassis.Chassis)
        c._fault_indication_task = None
        return c

    @mock.patch('sonic_platform.fault_indication_task.FaultIndicationTask')
    def test_the_first_assertion_spawns_the_task_and_enqueues(self, mock_task_cls):
        c = self._fresh_chassis()
        cpo_port = _fake_cpo_port(oe_id=1, els_id=0)
        arrival = datetime(2026, 7, 8, 15, 5)

        c._enqueue_cpo_fault_interrupt(cpo_port, arrival)

        # Constructed with the chassis handle and started.
        mock_task_cls.assert_called_once_with(c)
        task_instance = mock_task_cls.return_value
        task_instance.start.assert_called_once_with()
        task_instance.enqueue_interrupt.assert_called_once_with(cpo_port, arrival)
        assert c._fault_indication_task is task_instance

    @mock.patch('sonic_platform.fault_indication_task.FaultIndicationTask')
    def test_later_assertions_reuse_the_running_task(self, mock_task_cls):
        c = self._fresh_chassis()
        cpo_a = _fake_cpo_port(oe_id=1, els_id=0)
        cpo_b = _fake_cpo_port(oe_id=2, els_id=3)   # different vModule
        arrival_1 = datetime(2026, 7, 8, 15, 5)
        arrival_2 = datetime(2026, 7, 8, 15, 6)
        arrival_3 = datetime(2026, 7, 8, 15, 7)

        c._enqueue_cpo_fault_interrupt(cpo_a, arrival_1)
        c._enqueue_cpo_fault_interrupt(cpo_b, arrival_2)
        c._enqueue_cpo_fault_interrupt(cpo_a, arrival_3)  # same vModule again

        assert mock_task_cls.call_count == 1
        task_instance = mock_task_cls.return_value
        task_instance.start.assert_called_once_with()
        # Every assertion forwarded its own pair, in order.
        assert task_instance.enqueue_interrupt.call_args_list == [
            mock.call(cpo_a, arrival_1),
            mock.call(cpo_b, arrival_2),
            mock.call(cpo_a, arrival_3),
        ]

    def test_no_task_exists_before_anything_asks_for_one(self):
        # A switch with no CPO ports registers no interrupt fd and takes no
        # assertion, so it never pays for the thread or the STATE_DB handles.
        c = self._fresh_chassis()
        assert c._fault_indication_task is None


class TestInterruptDispatchInGetChangeEvent:
    """The interrupt branch inside get_change_event_for_module_host_management_mode:
      - fd_value != 0   -> route to _enqueue_cpo_fault_interrupt
      - fd_value == 0   -> ack only (kernel first-poll wake-up); no enqueue
      - either way      -> FaultIndicationTask is already running, started by
                           the registration of the interrupt fd, and the loop
                           continues, so no plug-event side effects
    """

    @mock.patch('sonic_platform.chassis.Chassis.get_asic_change_event')
    @mock.patch('sonic_platform.wait_sfp_ready_task.WaitSfpReadyTask.get_ready_set')
    @mock.patch('sonic_platform.module_sysfs.ModuleSysfsMixin.get_fd')
    @mock.patch('select.poll')
    @mock.patch('time.monotonic')
    @mock.patch(
        'sonic_platform.device_data.DeviceDataManager.is_module_host_management_mode',
        mock.MagicMock(return_value=True),
    )
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_sfp_count', mock.MagicMock(return_value=1))
    @mock.patch('sonic_platform.chassis.extract_RJ45_ports_index', mock.MagicMock(return_value=[]))
    @mock.patch('sonic_platform.chassis.build_cpo_port_map', mock.MagicMock(return_value=None))
    @mock.patch('sonic_platform.chassis.build_cpo_service_port_map', mock.MagicMock(return_value=None))
    @mock.patch('sonic_platform.module_host_mgmt_initializer.ModuleHostMgmtInitializer.initialize', mock.MagicMock())
    @pytest.mark.parametrize('interrupt_value, should_enqueue', [
        ('1', True),    # real HW assertion
        ('0', False),   # kernel first-poll spurious wake-up
    ])
    def test_an_interrupt_event_routes_only_when_its_value_is_nonzero(
        self, mock_time, mock_create_poll, mock_get_fd, mock_ready,
        mock_get_asic_event, interrupt_value, should_enqueue,
    ):
        # One CPO vModule placed directly in _cpo_list - the real
        # initialize_sfp path needs the SDK. The registration loop reaches it
        # through get_sfp_list_for_polling, and the interrupt fd comes from
        # CpoPort.extra_poll_types, so there is no CPO branch on the chassis
        # side of registration.
        c = chassis.Chassis()
        cpo_port = CpoPort(sfp_index=0, bank_id=0, oe_id=5, els_id=7)
        cpo_port.state = sfp.STATE_SW_CONTROL
        c._cpo_list = [cpo_port]
        c._sfp_list = []

        # Plug-event fds plus the interrupt one. Distinct filenos so a poll
        # event can name which fired.
        hw_fd = mock.MagicMock(); hw_fd.fileno.return_value = 1
        hw_fd.read.return_value = sfp.SFP_STATUS_INSERTED
        power_fd = mock.MagicMock(); power_fd.fileno.return_value = 2
        power_fd.read.return_value = 1
        intr_fd = mock.MagicMock(); intr_fd.fileno.return_value = 3
        intr_fd.read.return_value = interrupt_value

        def get_fd(fd_type):
            if fd_type == 'hw_present':        return hw_fd
            if fd_type == 'power_good':        return power_fd
            if fd_type == SFP_SYSFS_INTERRUPT: return intr_fd
            return mock.MagicMock()
        mock_get_fd.side_effect = get_fd

        # Fire only on the interrupt fd this iteration.
        mock_poll = mock.MagicMock()
        mock_create_poll.return_value = mock_poll
        mock_poll.poll.return_value = [(intr_fd.fileno.return_value, 10)]

        mock_ready.return_value = set()
        timeout = 1000
        # begin, iteration_begin, now - the third reading ends the while-True
        # after a single pass.
        mock_time.side_effect = [0, 0, timeout]

        # Freeze utcnow so the arrival_time can be asserted verbatim.
        fixed_arrival = datetime(2026, 7, 8, 15, 5)

        with mock.patch('sonic_platform.fault_indication_task.FaultIndicationTask') as mock_task_cls, \
             mock.patch('sonic_platform.chassis.datetime') as mock_dt:
            mock_dt.utcnow.return_value = fixed_arrival
            c.get_change_event(timeout)

            # Registering an interrupt fd is itself enough to start the task,
            # so it exists either way; what the value decides is whether this
            # event is handed to it.
            mock_task_cls.assert_called_once_with(c)
            mock_task_cls.return_value.start.assert_called_once_with()
            assert c._fault_indication_task is mock_task_cls.return_value

            if should_enqueue:
                mock_task_cls.return_value.enqueue_interrupt.assert_called_once_with(
                    cpo_port, fixed_arrival)
            else:
                mock_task_cls.return_value.enqueue_interrupt.assert_not_called()

        # Either way the fd was seek()ed and read() to consume the poll event.
        intr_fd.seek.assert_called_with(0)
        intr_fd.read.assert_called()
