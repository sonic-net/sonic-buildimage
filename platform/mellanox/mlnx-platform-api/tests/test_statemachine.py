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
import pytest
import sys

from mock import MagicMock
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform import state_machine

STATE_DOWN = 'Down'
STATE_INIT = 'Initializing'
STATE_UP = 'Up'

ACTION_LEAVE_DOWN = 'Leave Down'
ACTION_INIT = 'Initializing'
ACTION_UP = 'Up'

EVENT_START = 'Start'
EVENT_INIT_DONE = 'Initialize Done'
EVENT_STOP = 'Stop'

class StateEntity:
    def __init__(self):
        self.state = STATE_DOWN
        self.current_action = None
        self.triggered_actions = []
        
    def get_state(self):
        return self.state
    
    def change_state(self, new_state):
        self.state = new_state
    
    def on_event(self, event):
        pass
    
    def on_action(self, action_name):
        self.current_action = action_name
        self.triggered_actions.append(action_name)


class TestStateMachine:
    sm = None
    @classmethod
    def setup_class(cls):
        sm = state_machine.StateMachine()
        sm.add_state(STATE_DOWN).set_leave_action(ACTION_LEAVE_DOWN) \
          .add_transition(EVENT_START, STATE_INIT)
        sm.add_state(STATE_INIT).set_entry_action(ACTION_INIT) \
          .add_transition(EVENT_INIT_DONE, STATE_UP) \
          .add_transition(EVENT_STOP, STATE_DOWN)
        sm.add_state(STATE_UP).set_entry_action(ACTION_UP) \
          .add_transition(EVENT_STOP, STATE_DOWN)
        cls.sm = sm
        
    def test_state_machine(self):
        state_entity = StateEntity()
        
        # Start
        self.sm.on_event(state_entity, EVENT_START)
        assert state_entity.triggered_actions == [ACTION_LEAVE_DOWN, ACTION_INIT]
        assert state_entity.get_state() == STATE_INIT
        
        # Initialize done
        self.sm.on_event(state_entity, EVENT_INIT_DONE)
        assert state_entity.current_action == ACTION_UP
        assert state_entity.get_state() == STATE_UP
        
        # Stop
        self.sm.on_event(state_entity, EVENT_STOP)
        assert state_entity.get_state() == STATE_DOWN
        
        # Quick start/stop
        self.sm.on_event(state_entity, EVENT_START)
        self.sm.on_event(state_entity, EVENT_STOP)
        assert state_entity.get_state() == STATE_DOWN
        
        # Event not defined for this state, state machine should ignore it
        self.sm.on_event(state_entity, EVENT_STOP)
        assert state_entity.get_state() == STATE_DOWN
        
    def test_unknown_state(self):
        state_entity = StateEntity()
        state_entity.state = 'unknown'
        with pytest.raises(RuntimeError):
            # Trigger unknown event
            self.sm.on_event(state_entity, EVENT_START)
            
    def test_duplicate_state(self):
        sm = state_machine.StateMachine()
        sm.add_state(STATE_DOWN)
        with pytest.raises(RuntimeError):
            # Add duplicate state
            sm.add_state(STATE_DOWN)
            
    def test_duplicate_transition(self):
        sm = state_machine.StateMachine()
        with pytest.raises(RuntimeError):
            # Add duplicate transition
            sm.add_state(STATE_DOWN) \
              .add_transition(EVENT_START, STATE_INIT) \
              .add_transition(EVENT_START, STATE_INIT)

    def test_unknown_transition_target(self):
        sm = state_machine.StateMachine()
        # Add unknown transition target
        sm.add_state(STATE_DOWN) \
          .add_transition(EVENT_START, 'unknown')
          
        state_entity = StateEntity()
        with pytest.raises(RuntimeError):
            sm.on_event(state_entity, EVENT_START)
    