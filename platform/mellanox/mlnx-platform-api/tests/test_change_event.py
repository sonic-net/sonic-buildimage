#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

from sonic_platform import chassis
from sonic_platform import sfp


class TestChangeEvent:
    @mock.patch('sonic_platform.sfp.SFP.get_fd_for_polling_legacy')
    @mock.patch('select.poll')
    @mock.patch('time.monotonic')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.is_module_host_management_mode', mock.MagicMock(return_value=False))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_sfp_count', mock.MagicMock(return_value=1))
    @mock.patch('sonic_platform.chassis.extract_RJ45_ports_index', mock.MagicMock(return_value=[]))
    @mock.patch('sonic_platform.chassis.extract_cpo_ports_index', mock.MagicMock(return_value=[]))
    @mock.patch('sonic_platform.sfp.SFP.get_module_status')
    def test_get_change_event_legacy(self, mock_status, mock_time, mock_create_poll, mock_get_fd):
        c = chassis.Chassis()
        s = c.get_sfp(1)
        
        mock_status.return_value = sfp.SFP_STATUS_INSERTED
        
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
        mock_time.side_effect = [0, timeout]

        # no event, expect returning empty change event
        _, change_event = c.get_change_event(timeout)
        assert 'sfp' in change_event and not change_event['sfp']

        # dummy event, expect returning empty change event
        sfp_index = s.sdk_index + 1
        mock_poll.poll.return_value = [(1, 10)]
        mock_time.side_effect = [0, timeout]
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
    
    @mock.patch('sonic_platform.wait_sfp_ready_task.WaitSfpReadyTask.get_ready_set')    
    @mock.patch('sonic_platform.sfp.SFP.get_fd')
    @mock.patch('select.poll')
    @mock.patch('time.monotonic')
    @mock.patch('sonic_platform.device_data.DeviceDataManager.is_module_host_management_mode', mock.MagicMock(return_value=True))
    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_sfp_count', mock.MagicMock(return_value=1))
    @mock.patch('sonic_platform.chassis.extract_RJ45_ports_index', mock.MagicMock(return_value=[]))
    @mock.patch('sonic_platform.chassis.extract_cpo_ports_index', mock.MagicMock(return_value=[]))
    @mock.patch('sonic_platform.module_host_mgmt_initializer.ModuleHostMgmtInitializer.initialize', mock.MagicMock())
    def test_get_change_event_for_module_host_management_mode(self, mock_time, mock_create_poll, mock_get_fd, mock_ready):
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
        mock_time.side_effect = [0, timeout]
        
        # no event, expect returning empty change event
        _, change_event = c.get_change_event(timeout)
        assert 'sfp' in change_event and not change_event['sfp']
        
        # dummy event, expect returning empty change event
        sfp_index = s.sdk_index + 1
        mock_poll.poll.return_value = [(1, 10)]
        mock_time.side_effect = [0, timeout]
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
        mock_time.side_effect = [0, timeout]
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
        mock_time.side_effect = [0, timeout]
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
