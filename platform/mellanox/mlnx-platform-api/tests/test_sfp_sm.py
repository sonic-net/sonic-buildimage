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
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform import sfp
from sonic_platform import utils

origin_read = utils.read_from_file
origin_write = utils.write_file


class TestSfpStateMachine:
    PATH_PREFIX = '/sys/module/sx_core/asic0/module0'
    mock_file_content = {}
    
    @classmethod
    def setup_class(cls):
        utils.read_from_file = cls.mock_read
        utils.write_file = cls.mock_write
        
    @classmethod
    def teardown_class(cls):
        utils.read_from_file = origin_read
        utils.write_file = origin_write
    
    @classmethod
    def mock_value(cls, file_name, value):
        cls.mock_file_content[f'{cls.PATH_PREFIX}/{file_name}'] = value
        
    @classmethod
    def get_value(cls, file_name):
        return cls.mock_file_content[f'{cls.PATH_PREFIX}/{file_name}']
        
    @classmethod
    def mock_write(cls, file_path, value, *args, **kwargs):
        cls.mock_file_content[file_path] = value
        
    @classmethod
    def mock_read(cls, file_path, *args, **kwargs):
        return cls.mock_file_content[file_path]
    
    def test_warm_reboot_from_fw_control(self):
        self.mock_value('control', 0)
        s = sfp.SFP(0)
        s.on_event(sfp.EVENT_START)
        assert s.get_state() == sfp.STATE_FW_CONTROL
        
    def test_no_hw_present(self):
        self.mock_value('control', 1)
        self.mock_value('hw_present', 0)
        s = sfp.SFP(0)
        s.on_event(sfp.EVENT_START)
        assert s.get_state() == sfp.STATE_NOT_PRESENT
        
    def test_not_powered(self):
        self.mock_value('control', 1)
        self.mock_value('hw_present', 1)
        self.mock_value('power_on', 0)
        s = sfp.SFP(0)
        s.on_event(sfp.EVENT_START)
        assert s.get_state() == sfp.STATE_RESETTING
        assert self.get_value('power_on') == 1
        assert self.get_value('hw_reset') == 1
        assert 0 in sfp.SFP.get_wait_ready_task()._wait_dict
        sfp.SFP.get_wait_ready_task()._wait_dict.pop(0)
        
    def test_in_reset_state(self):
        self.mock_value('control', 1)
        self.mock_value('hw_present', 1)
        self.mock_value('power_on', 1)
        self.mock_value('hw_reset', 0)
        s = sfp.SFP(0)
        s.on_event(sfp.EVENT_START)
        assert s.get_state() == sfp.STATE_RESETTING
        assert self.get_value('hw_reset') == 1
        assert 0 in sfp.SFP.get_wait_ready_task()._wait_dict
        s.on_event(sfp.EVENT_NOT_PRESENT)
        assert s.get_state() == sfp.STATE_NOT_PRESENT
        assert 0 not in sfp.SFP.get_wait_ready_task()._wait_dict
        
    def test_reset_done(self):
        self.mock_value('control', 1)
        self.mock_value('hw_present', 1)
        self.mock_value('power_on', 1)
        self.mock_value('hw_reset', 0)
        self.mock_value('power_good', 1)
        s = sfp.SFP(0)
        s.determine_control_type = mock.MagicMock(return_value=sfp.SFP_FW_CONTROL)
        s.on_event(sfp.EVENT_START)
        assert s.get_state() == sfp.STATE_RESETTING
        s.on_event(sfp.EVENT_RESET_DONE)
        assert s.get_state() == sfp.STATE_FW_CONTROL
        
    def test_no_power_good(self):
        self.mock_value('control', 1)
        self.mock_value('hw_present', 1)
        self.mock_value('power_on', 1)
        self.mock_value('hw_reset', 1)
        self.mock_value('power_good', 0)
        s = sfp.SFP(0)
        s.on_event(sfp.EVENT_START)
        assert s.get_state() == sfp.STATE_POWER_BAD
        s.on_event(sfp.EVENT_NOT_PRESENT)
        assert s.get_state() == sfp.STATE_NOT_PRESENT

    def test_fw_control(self):
        self.mock_value('control', 1)
        self.mock_value('hw_present', 1)
        self.mock_value('power_on', 1)
        self.mock_value('hw_reset', 1)
        self.mock_value('power_good', 1)
        s = sfp.SFP(0)
        s.determine_control_type = mock.MagicMock(return_value=sfp.SFP_FW_CONTROL)
        s.on_event(sfp.EVENT_START)
        assert s.get_state() == sfp.STATE_FW_CONTROL
        assert self.get_value('control') == sfp.SFP_FW_CONTROL
        
    def test_power_exceed(self):
        self.mock_value('control', 1)
        self.mock_value('hw_present', 1)
        self.mock_value('power_on', 1)
        self.mock_value('hw_reset', 1)
        self.mock_value('power_good', 1)
        s = sfp.SFP(0)
        s.determine_control_type = mock.MagicMock(return_value=sfp.SFP_SW_CONTROL)
        s.check_power_capability = mock.MagicMock(return_value=False)
        s.on_event(sfp.EVENT_START)
        assert s.get_state() == sfp.STATE_POWER_LIMIT_ERROR
        assert self.get_value('power_on') == 0
        assert self.get_value('hw_reset') == 0
        s.on_event(sfp.EVENT_NOT_PRESENT)
        assert s.get_state() == sfp.STATE_NOT_PRESENT
        
    def test_sw_control(self):
        self.mock_value('control', 1)
        self.mock_value('hw_present', 1)
        self.mock_value('power_on', 1)
        self.mock_value('hw_reset', 1)
        self.mock_value('power_good', 1)
        s = sfp.SFP(0)
        s.determine_control_type = mock.MagicMock(return_value=sfp.SFP_SW_CONTROL)
        s.check_power_capability = mock.MagicMock(return_value=True)
        s.update_i2c_frequency = mock.MagicMock()
        s.disable_tx_for_sff_optics = mock.MagicMock()
        s.on_event(sfp.EVENT_START)
        assert s.get_state() == sfp.STATE_SW_CONTROL
