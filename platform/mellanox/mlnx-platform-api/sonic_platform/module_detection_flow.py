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
"""The module detection flow: bringing a module from plugged to usable.

MDF is the state machine that decides, for a module that has just appeared,
whether the host or the firmware drives it, and gets it powered and out of
reset either way. It is the one concern that spans both directions - it reads
the driver's view through the module-sysfs layer and the module's own answers
through the xcvr layer - which is why it is its own component rather than a
member of either.

A port owns its `state`, and the polling members branch on it: which file
descriptors a module is watched through depends on whether it ended up under
host or firmware control.

Nothing here is inherited. `ModuleDetectionFlow` is a class of static
functions that take the port they act on, so a CPO port and a pluggable port
run one flow off one state machine and one action table without either class
having to be in the other's ancestry.
"""

import select
import time

from sonic_py_common.logger import Logger

from . import utils
from .device_data import DeviceDataManager
from .module_xcvr import SFP_FW_CONTROL, SFP_SW_CONTROL
from .module_sysfs import (
    SFP_STATUS_INSERTED,
    SFP_STATUS_REMOVED,
    SFP_SYSFS_HW_PRESENT,
    SFP_SYSFS_POWER_GOOD,
    SFP_SYSFS_PRESENT,
)

logger = Logger()

STATE_DOWN = 'Down'                             # Initial state
STATE_INIT = 'Initializing'                     # Module starts initializing, check module present, also power on the module if need
STATE_RESETTING = 'Resetting'                   # Module is resetting the firmware
STATE_POWERED_ON = 'Power On'                   # Module is powered on, module firmware has been loaded, check module power is in good state
STATE_SW_CONTROL = 'Software Control'           # Module is under software control
STATE_FW_CONTROL = 'Firmware Control'           # Module is under firmware control
STATE_POWER_BAD = 'Power Bad'                   # Module power_good returns 0
STATE_POWER_LIMIT_ERROR = 'Exceed Power Limit'  # Module power exceeds cage power limit
STATE_NOT_PRESENT = 'Not Present'               # Module is not present

EVENT_START = 'Start'
EVENT_NOT_PRESENT = 'Not Present'
EVENT_RESET = 'Reset'
EVENT_POWER_ON = 'Power On'
EVENT_RESET_DONE = 'Reset Done'
EVENT_POWER_BAD = 'Power Bad'
EVENT_SW_CONTROL = 'Software Control'
EVENT_FW_CONTROL = 'Firmware Control'
EVENT_POWER_LIMIT_EXCEED = 'Power Limit Exceed'
EVENT_POWER_GOOD = 'Power Good'
EVENT_PRESENT = 'Present'

ACTION_ON_START = 'On Start'
ACTION_ON_RESET = 'On Reset'
ACTION_ON_POWERED = 'On Powered'
ACTION_ON_SW_CONTROL = 'On Software Control'
ACTION_ON_FW_CONTROL = 'On Firmware Control'
ACTION_ON_POWER_LIMIT_ERROR = 'On Power Limit Error'
ACTION_ON_CANCEL_WAIT = 'On Cancel Wait'

# States/actions for always firmware control ports
STATE_FCP_DOWN = 'Down(Firmware Control)'
STATE_FCP_INIT = 'Initializing(Firmware Control)'
STATE_FCP_NOT_PRESENT = 'Not Present(Firmware Control)'
STATE_FCP_PRESENT = 'Present(Firmware Control)'

ACTION_FCP_ON_START = 'On Start(Firmware Control)'

# A module in one of these states is parked waiting for an SDK poll event
# rather than mid-transition.
STABLE_STATES = (STATE_NOT_PRESENT, STATE_SW_CONTROL, STATE_FW_CONTROL,
                 STATE_POWER_BAD, STATE_POWER_LIMIT_ERROR, STATE_FCP_NOT_PRESENT,
                 STATE_FCP_PRESENT)

# Resetting a module reloads its firmware, which the standard allows up to
# three seconds for.
MODULE_RESET_MAX_WAIT_SECONDS = 3.5
MODULE_RESET_POLL_INTERVAL_SECONDS = 0.5

EEPROM_READY_POLL_INTERVAL_SECONDS = 0.1


class ModuleDetectionFlow:
    """The detection flow, shared by every port class that runs one.

    Not a base class: every member is a static function taking the port it
    acts on, so a port runs the flow by calling in rather than by inheriting.
    The state machine, its action table and the wait-ready task are class
    attributes, which is what makes all ports share one of each.

    A port only has to hold `state` and `processing_insert_event`, and to
    expose the three accessors `StateMachine` drives an entity through -
    `get_state`, `change_state` and `on_action`.
    """

    sm = None
    action_table = None
    wait_ready_task = None

    @staticmethod
    def init_detection_flow(sfp, sdk_index):
        """Place a port at the start of the flow it is configured for."""
        fw_control_ports = DeviceDataManager.get_always_fw_control_ports()
        if not fw_control_ports or sdk_index not in fw_control_ports:
            sfp.state = STATE_DOWN
        else:
            sfp.state = STATE_FCP_DOWN
        sfp.processing_insert_event = False

    @classmethod
    def get_state_machine(cls):
        """Get state machine object, create if not exists

        Returns:
            object: state machine object
        """
        if not cls.sm:
            from .state_machine import StateMachine
            sm = StateMachine()
            sm.add_state(STATE_DOWN).add_transition(EVENT_START, STATE_INIT)
            sm.add_state(STATE_INIT).set_entry_action(ACTION_ON_START) \
              .add_transition(EVENT_NOT_PRESENT, STATE_NOT_PRESENT) \
              .add_transition(EVENT_RESET, STATE_RESETTING) \
              .add_transition(EVENT_POWER_ON, STATE_POWERED_ON) \
              .add_transition(EVENT_FW_CONTROL, STATE_FW_CONTROL)  # for warm reboot, cable might be in firmware control at startup
            sm.add_state(STATE_RESETTING).set_entry_action(ACTION_ON_RESET) \
              .add_transition(EVENT_RESET_DONE, STATE_POWERED_ON) \
              .add_transition(EVENT_NOT_PRESENT, STATE_NOT_PRESENT, ACTION_ON_CANCEL_WAIT)
            sm.add_state(STATE_POWERED_ON).set_entry_action(ACTION_ON_POWERED) \
              .add_transition(EVENT_POWER_BAD, STATE_POWER_BAD) \
              .add_transition(EVENT_SW_CONTROL, STATE_SW_CONTROL) \
              .add_transition(EVENT_FW_CONTROL, STATE_FW_CONTROL)
            sm.add_state(STATE_SW_CONTROL).set_entry_action(ACTION_ON_SW_CONTROL) \
              .add_transition(EVENT_NOT_PRESENT, STATE_NOT_PRESENT) \
              .add_transition(EVENT_POWER_LIMIT_EXCEED, STATE_POWER_LIMIT_ERROR) \
              .add_transition(EVENT_POWER_BAD, STATE_POWER_BAD)
            sm.add_state(STATE_FW_CONTROL).set_entry_action(ACTION_ON_FW_CONTROL) \
              .add_transition(EVENT_NOT_PRESENT, STATE_NOT_PRESENT)
            sm.add_state(STATE_POWER_BAD).add_transition(EVENT_POWER_GOOD, STATE_POWERED_ON) \
              .add_transition(EVENT_NOT_PRESENT, STATE_NOT_PRESENT)
            sm.add_state(STATE_NOT_PRESENT).add_transition(EVENT_PRESENT, STATE_INIT)
            sm.add_state(STATE_POWER_LIMIT_ERROR).set_entry_action(ACTION_ON_POWER_LIMIT_ERROR) \
              .add_transition(EVENT_POWER_GOOD, STATE_POWERED_ON) \
              .add_transition(EVENT_NOT_PRESENT, STATE_NOT_PRESENT)

            cls.action_table = {}
            cls.action_table[ACTION_ON_START] = cls.action_on_start
            cls.action_table[ACTION_ON_RESET] = cls.action_on_reset
            cls.action_table[ACTION_ON_POWERED] = cls.action_on_powered
            cls.action_table[ACTION_ON_SW_CONTROL] = cls.action_on_sw_control
            cls.action_table[ACTION_ON_FW_CONTROL] = cls.action_on_fw_control
            cls.action_table[ACTION_ON_CANCEL_WAIT] = cls.action_on_cancel_wait
            cls.action_table[ACTION_ON_POWER_LIMIT_ERROR] = cls.action_on_power_limit_error

            # For always firmware control ports
            sm.add_state(STATE_FCP_DOWN).add_transition(EVENT_START, STATE_FCP_INIT)
            sm.add_state(STATE_FCP_INIT).set_entry_action(ACTION_FCP_ON_START) \
              .add_transition(EVENT_NOT_PRESENT, STATE_FCP_NOT_PRESENT) \
              .add_transition(EVENT_PRESENT, STATE_FCP_PRESENT)
            sm.add_state(STATE_FCP_NOT_PRESENT).add_transition(EVENT_PRESENT, STATE_FCP_PRESENT)
            sm.add_state(STATE_FCP_PRESENT).add_transition(EVENT_NOT_PRESENT, STATE_FCP_NOT_PRESENT)

            cls.action_table[ACTION_FCP_ON_START] = cls.action_fcp_on_start

            cls.sm = sm

        return cls.sm

    @classmethod
    def action_on_start(cls, sfp):
        sdk_index = sfp.get_sdk_index()
        if sfp.get_control_type() == SFP_FW_CONTROL:
            logger.log_info(f'SFP {sdk_index} is already FW control, probably in warm reboot')
            cls.on_event(sfp, EVENT_FW_CONTROL)
            return

        if not sfp.get_hw_present():
            logger.log_info(f'SFP {sdk_index} is not present')
            cls.on_event(sfp, EVENT_NOT_PRESENT)
            return

        if not sfp.get_power_on():
            logger.log_info(f'SFP {sdk_index} is not powered on')
            sfp.set_power(True)
            sfp.set_hw_reset(1)
            cls.on_event(sfp, EVENT_RESET)
        else:
            if not sfp.get_reset_state():
                logger.log_info(f'SFP {sdk_index} is in reset state')
                sfp.set_hw_reset(1)
                cls.on_event(sfp, EVENT_RESET)
            else:
                if not sfp.processing_insert_event:
                    cls.on_event(sfp, EVENT_POWER_ON)
                else:
                    sfp.processing_insert_event = False
                    logger.log_info(f'SFP {sdk_index} is processing insert event and needs to wait module ready')
                    cls.on_event(sfp, EVENT_RESET)

    @classmethod
    def action_fcp_on_start(cls, sfp):
        present = utils.read_int_from_file(sfp._module_attr_path(SFP_SYSFS_PRESENT))
        if present:
            cls.on_event(sfp, EVENT_PRESENT)
        else:
            cls.on_event(sfp, EVENT_NOT_PRESENT)

    @classmethod
    def action_on_reset(cls, sfp):
        logger.log_info(f'SFP {sfp.get_sdk_index()} is scheduled to wait for resetting done')
        cls.get_wait_ready_task().schedule_wait(sfp.get_sdk_index())

    @classmethod
    def action_on_powered(cls, sfp):
        if not sfp.get_power_good():
            logger.log_error(f'SFP {sfp.get_sdk_index()} is not in power good state')
            cls.on_event(sfp, EVENT_POWER_BAD)
            return

        control_type = sfp.determine_control_type()
        if control_type == SFP_SW_CONTROL:
            cls.on_event(sfp, EVENT_SW_CONTROL)
        else:
            cls.on_event(sfp, EVENT_FW_CONTROL)

    @classmethod
    def action_on_sw_control(cls, sfp):
        if not sfp.check_power_capability():
            cls.on_event(sfp, EVENT_POWER_LIMIT_EXCEED)
            return

        sfp.update_i2c_frequency()
        sfp.disable_tx_for_sff_optics()
        logger.log_info(f'SFP {sfp.get_sdk_index()} is set to software control')

    @classmethod
    def action_on_fw_control(cls, sfp):
        if sfp.get_control_type() != SFP_FW_CONTROL:
            logger.log_info(f'SFP {sfp.get_sdk_index()} is set to firmware control')
            sfp.set_control_type(SFP_FW_CONTROL)

    @classmethod
    def action_on_cancel_wait(cls, sfp):
        cls.get_wait_ready_task().cancel_wait(sfp.get_sdk_index())

    @classmethod
    def action_on_power_limit_error(cls, sfp):
        logger.log_info(f'SFP {sfp.get_sdk_index()} is powered off due to exceeding power limit')
        sfp.set_power(False)
        sfp.set_hw_reset(0)

    @classmethod
    def get_wait_ready_task(cls):
        """Get SFP wait ready task. Create if not exists.

        Returns:
            object: an instance of WaitSfpReadyTask
        """
        if not cls.wait_ready_task:
            from .wait_sfp_ready_task import WaitSfpReadyTask
            cls.wait_ready_task = WaitSfpReadyTask()
        return cls.wait_ready_task

    @classmethod
    def on_event(cls, sfp, event):
        """Called when a state machine event arrives

        Args:
            sfp (object): the module the event is for
            event (str): State machine event
        """
        cls.get_state_machine().on_event(sfp, event)

    @staticmethod
    def in_stable_state(sfp):
        """Indicate whether this module is in a stable state. 'Stable state' means the module is pending on a polling event
        from SDK.

        Args:
            sfp (object): the module to check

        Returns:
            bool: True if the module is in a stable state
        """
        return sfp.state in STABLE_STATES

    @staticmethod
    def get_fds_for_poling(sfp):
        if sfp.state == STATE_FW_CONTROL or sfp.state == STATE_FCP_NOT_PRESENT or sfp.state == STATE_FCP_PRESENT:
            return {
                SFP_SYSFS_PRESENT: sfp.get_fd(SFP_SYSFS_PRESENT)
            }
        else:
            return {
                SFP_SYSFS_HW_PRESENT: sfp.get_fd(SFP_SYSFS_HW_PRESENT),
                SFP_SYSFS_POWER_GOOD: sfp.get_fd(SFP_SYSFS_POWER_GOOD)
            }

    @staticmethod
    def refresh_poll_obj(sfp, poll_obj, all_registered_fds):
        """Refresh polling object and registered fds. This function is usually called when a cable plugin
        event occurs. For example, user plugs out a software control module and replaces with a firmware
        control cable. In such case, poll_obj was polling "hw_present" and "power_good" for software control,
        and it needs to be changed to poll "present" for new control type which is firmware control.

        Args:
            sfp (object): the module whose fds are refreshed
            poll_obj (object): poll object
            all_registered_fds (dict): fds that have been registered to poll object
        """
        # find fds registered by this SFP
        current_registered_fds = {item[2]: (fileno, item[1]) for fileno, item in all_registered_fds.items() if item[0] is sfp}
        logger.log_debug(f'SFP {sfp.sdk_index} registered fds are: {current_registered_fds}')
        if sfp.state == STATE_FW_CONTROL or sfp.state == STATE_FCP_NOT_PRESENT or sfp.state == STATE_FCP_PRESENT:
            target_poll_types = [SFP_SYSFS_PRESENT]
        else:
            target_poll_types = [SFP_SYSFS_HW_PRESENT, SFP_SYSFS_POWER_GOOD]

        for target_poll_type in target_poll_types:
            if target_poll_type not in current_registered_fds:
                # need add new fd for polling
                logger.log_debug(f'SFP {sfp.sdk_index} is registering file descriptor: {target_poll_type}')
                fd = sfp.get_fd(target_poll_type)
                poll_obj.register(fd, select.POLLERR | select.POLLPRI)
                all_registered_fds[fd.fileno()] = (sfp, fd, target_poll_type)
            else:
                # the fd is already in polling
                current_registered_fds.pop(target_poll_type)

        for _, item in current_registered_fds.items():
            # Deregister poll, close fd
            logger.log_debug(f'SFP {sfp.sdk_index} is de-registering file descriptor: {item}')
            poll_obj.unregister(item[1])
            all_registered_fds.pop(item[0])
            item[1].close()

    @staticmethod
    def is_dummy_event(sfp, fd_type, fd_value):
        """Check whether an event is dummy event

        Args:
            sfp (object): the module the event arrived on
            fd_type (str): polling sysfs type
            fd_value (int): polling sysfs value

        Returns:
            bool: True if the event is a dummy event
        """
        if fd_type == SFP_SYSFS_HW_PRESENT or fd_type == SFP_SYSFS_PRESENT:
            if fd_value == int(SFP_STATUS_INSERTED):
                return sfp.state in (STATE_SW_CONTROL, STATE_FW_CONTROL, STATE_POWER_BAD,
                                     STATE_POWER_LIMIT_ERROR, STATE_FCP_PRESENT)
            elif fd_value == int(SFP_STATUS_REMOVED):
                return sfp.state in (STATE_NOT_PRESENT, STATE_FCP_NOT_PRESENT)
        elif fd_type == SFP_SYSFS_POWER_GOOD:
            if fd_value == 1:
                return sfp.state in (STATE_SW_CONTROL, STATE_NOT_PRESENT, STATE_RESETTING)
            else:
                return sfp.state in (STATE_POWER_BAD, STATE_POWER_LIMIT_ERROR, STATE_NOT_PRESENT)
        return False

    @staticmethod
    def wait_sfp_eeprom_ready(sfp_list, wait_time):
        not_ready_list = sfp_list

        while wait_time > 0:
            not_ready_list = [s for s in not_ready_list
                              if s.state == STATE_FW_CONTROL and s._read_eeprom(0, 2, False) is None]
            if not_ready_list:
                time.sleep(EEPROM_READY_POLL_INTERVAL_SECONDS)
                wait_time -= EEPROM_READY_POLL_INTERVAL_SECONDS
            else:
                return

        for s in not_ready_list:
            logger.log_error(f'SFP {s.sdk_index} eeprom is not ready')

    @classmethod
    def initialize_sfp_modules(cls, sfp_list):
        """Initialize all modules. Only applicable when module host management is enabled

        Args:
            sfp_list (object): all sfps
        """
        wait_ready_task = cls.get_wait_ready_task()
        wait_ready_task.start_once()

        for s in sfp_list:
            cls.on_event(s, EVENT_START)

        if not wait_ready_task.empty():
            # Wait until wait_ready_task is up
            while not wait_ready_task.is_alive():
                pass

            begin = time.monotonic()
            while True:
                ready_sfp_set = wait_ready_task.get_ready_set()
                for sfp_index in ready_sfp_set:
                    s = sfp_list[sfp_index]
                    logger.log_debug(f'SFP {sfp_index} is recovered from resetting state')
                    cls.on_event(s, EVENT_RESET_DONE)
                elapse = time.monotonic() - begin
                if elapse < MODULE_RESET_MAX_WAIT_SECONDS:
                    time.sleep(MODULE_RESET_POLL_INTERVAL_SECONDS)
                else:
                    break

        # Verify that all modules are in a stable state
        for index, s in enumerate(sfp_list):
            if not cls.in_stable_state(s):
                logger.log_error(f'SFP {index} is not in stable state after initializing, state={s.state}')
            logger.log_notice(f'SFP {index} is in state {s.state} after module initialization')

        cls.wait_sfp_eeprom_ready(sfp_list, 2)
