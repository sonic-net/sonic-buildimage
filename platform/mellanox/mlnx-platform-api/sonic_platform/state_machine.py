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

from sonic_py_common.logger import Logger

logger = Logger()


class State:
    """Represent a state in a state machine
    """
    def __init__(self, name):
        self.name = name
        self.entry_action = None
        self.leave_action = None
        self.transitions = {}
        self.event_actions = {}

    def set_entry_action(self, action_name):
        """Set an action when entering this state

        Args:
            action_name (str): action name

        Returns:
            object: self
        """
        self.entry_action = action_name
        return self

    def set_leave_action(self, action_name):
        """Set a leave action when leaving the state

        Args:
            action_name (str): action name

        Returns:
            object: self
        """
        self.leave_action = action_name
        return self

    def add_transition(self, event, next_state, event_action=None):
        """Add a transition item to this state

        Args:
            event (str): event name
            next_state (str): next state that the state entity will transit to upon this event.
            event_action (str): action called when event arrives

        Raises:
            RuntimeError: raise if the event is already in the transition table

        Returns:
            object: self
        """
        if event in self.transitions:
            raise RuntimeError(f'event {event} already exists in transition table of state {self.name}')

        self.transitions[event] = next_state
        
        if event_action:
            if event in self.event_actions:
                raise RuntimeError(f'event {event} already exists in action table of state {self.name}')
            self.event_actions[event] = event_action
        return self

    def on_enter(self, entity):
        """Called when state entity enters the state

        Args:
            entity (obj): state entity
        """
        if self.entry_action:
            logger.log_debug(f'{entity} entered state [{self.name}] and is triggering action [{self.entry_action}]')
            entity.on_action(self.entry_action)
        else:
            logger.log_debug(f'{entity} entered state [{self.name}]')

    def on_leave(self, entity):
        """Called when state entity leaves the state

        Args:
            entity (obj): state entity
        """
        if self.leave_action:
            entity.on_action(self.leave_action)

    def on_event(self, entity, event):
        """Called when state entity has got an event

        Args:
            entity (object): state entity
            event (str): event name

        Returns:
            str: next event name
        """
        if event not in self.transitions:
            logger.log_error(f'{event} is not defined in state {self.name}')
            return self.name
        else:
            if event in self.event_actions:
                entity.on_action(self.event_actions[event])
            return self.transitions[event]


class StateMachine:
    def __init__(self):
        self.states = {}

    def add_state(self, state_name):
        """Register a state to state machine

        Args:
            state_name (str): name of the state

        Raises:
            RuntimeError: raise if state name already exists

        Returns:
            object: the new state object
        """
        if state_name in self.states:
            raise RuntimeError(f'state {state_name} already exists')

        state = State(state_name)
        self.states[state_name] = state
        return state

    def on_event(self, entity, event):
        """Called when an event occurs

        Args:
            entity (object): state entity
            event (str): event name

        Raises:
            RuntimeError: raise if the current state is not registered
            RuntimeError: raise if next state is not registered
        """
        current_state_name = entity.get_state()
        if current_state_name not in self.states:
            raise RuntimeError(f'Unknown state {current_state_name}')

        current_state = self.states[current_state_name]
        next_state_name = current_state.on_event(entity, event)
        logger.log_debug(f'{entity} has got event [{event}], it is changing from state [{current_state}] to [{next_state_name}]')
        if next_state_name not in self.states:
            raise RuntimeError(f'Unknown next state {next_state_name}')
        if next_state_name != current_state_name:
            current_state.on_leave(entity)
            entity.change_state(next_state_name)
            self.states[next_state_name].on_enter(entity)
