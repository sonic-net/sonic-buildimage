#
# Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES.
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

import threading
import time
import queue
import os
import select
import traceback

try:
    from sonic_py_common.logger import Logger
    from sonic_py_common import device_info, multi_asic
    from .device_data import DeviceDataManager
    from sonic_platform_base.sonic_xcvr.fields import consts
    from sonic_platform_base.sonic_xcvr.api.public import cmis
    from . import sfp as sfp_module
    from . import utils
    from swsscommon.swsscommon import SonicV2Connector
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

# Global logger class instance
logger = Logger()

STATE_HW_NOT_PRESENT = "Initial state. module is not plugged to cage."
STATE_HW_PRESENT = "Module is plugged to cage"
STATE_MODULE_AVAILABLE = "Module hw present and power is good"
STATE_POWERED = "Module power is already loaded"
STATE_NOT_POWERED = "Module power is not loaded"
STATE_FW_CONTROL = "The module is not CMIS and FW needs to handle"
STATE_SW_CONTROL = "The module is CMIS and SW needs to handle"
STATE_ERROR_HANDLER = "An error occurred - read/write error, power limit or power cap."
STATE_POWER_LIMIT_ERROR = "The cage has not enough power for the plugged module"
STATE_SYSFS_ERROR = "An error occurred while writing/reading SySFS."

SAI_PROFILE_FILE = "/{}/sai.profile"
SAI_INDEP_MODULE_MODE = "SAI_INDEPENDENT_MODULE_MODE"
SAI_INDEP_MODULE_MODE_DELIMITER = "="
SAI_INDEP_MODULE_MODE_TRUE_STR = "1"
SYSFS_LEGACY_FD_PRESENCE = "/sys/module/sx_core/asic0/module{}/present"
ASIC_NUM = 0
SYSFS_INDEPENDENT_FD_PREFIX_WO_MODULE = "/sys/module/sx_core/asic{}".format(ASIC_NUM)
SYSFS_INDEPENDENT_FD_PREFIX = SYSFS_INDEPENDENT_FD_PREFIX_WO_MODULE + "/module{}"
SYSFS_INDEPENDENT_FD_PRESENCE = os.path.join(SYSFS_INDEPENDENT_FD_PREFIX, "hw_present")
SYSFS_INDEPENDENT_FD_POWER_GOOD = os.path.join(SYSFS_INDEPENDENT_FD_PREFIX, "power_good")
SYSFS_INDEPENDENT_FD_POWER_ON = os.path.join(SYSFS_INDEPENDENT_FD_PREFIX, "power_on")
SYSFS_INDEPENDENT_FD_HW_RESET = os.path.join(SYSFS_INDEPENDENT_FD_PREFIX, "hw_reset")
SYSFS_INDEPENDENT_FD_POWER_LIMIT = os.path.join(SYSFS_INDEPENDENT_FD_PREFIX, "power_limit")
SYSFS_INDEPENDENT_FD_FW_CONTROL = os.path.join(SYSFS_INDEPENDENT_FD_PREFIX, "control")
# echo <val>  /sys/module/sx_core/$asic/$module/frequency   //  val: 0 - up to 400KHz, 1 - up to 1MHz
SYSFS_INDEPENDENT_FD_FREQ = os.path.join(SYSFS_INDEPENDENT_FD_PREFIX, "frequency")
SYSFS_INDEPENDENT_FD_FREQ_SUPPORT = os.path.join(SYSFS_INDEPENDENT_FD_PREFIX, "frequency_support")
IS_INDEPENDENT_MODULE = 'is_independent_module'

MAX_EEPROM_ERROR_RESET_RETRIES = 4

class ModulesMgmtTask(threading.Thread):

    def __init__(self, namespaces=None, main_thread_stop_event=None, q=None):
        threading.Thread.__init__(self)
        self.name = "ModulesMgmtTask"
        self.main_thread_stop_event = main_thread_stop_event
        self.sfp_port_dict_initial = {}
        self.sfp_port_dict = {}
        self.sfp_changes_dict = {}
        self.sfp_delete_list_from_port_dict = []
        self.namespaces = namespaces
        self.modules_changes_queue = q
        self.is_supported_indep_mods_system = False
        self.modules_lock_list = []
        # A set to hold those modules waiting 3 seconds since power on and hw reset
        self.waiting_modules_list = set()
        self.timer = threading.Thread()
        self.poll_obj = None
        self.fds_mapping_to_obj = {}
        self.port_to_fds = {}
        self.fds_events_count_dict = {}
        self.delete_ports_and_reset_states_dict = {}
        self.setName("ModulesMgmtTask")
        self.register_hw_present_fds = []

    # SFPs state machine
    def get_sm_func(self, sm, port):
        SFP_SM_ENUM = {STATE_HW_NOT_PRESENT: self.check_if_hw_present
            , STATE_HW_PRESENT: self.check_if_module_available
            , STATE_MODULE_AVAILABLE: self.check_if_power_on
            , STATE_NOT_POWERED: self.power_on_module
            , STATE_POWERED: self.check_module_type
            , STATE_FW_CONTROL: self.save_module_control_mode
            , STATE_SW_CONTROL: self.save_module_control_mode
            , STATE_ERROR_HANDLER: STATE_ERROR_HANDLER
            , STATE_POWER_LIMIT_ERROR: STATE_POWER_LIMIT_ERROR
            , STATE_SYSFS_ERROR: STATE_SYSFS_ERROR
        }
        logger.log_info("getting func for state {} for port {}".format(sm, port))
        try:
            func = SFP_SM_ENUM[sm]
            logger.log_info("got func {} for state {} for port {}".format(func, sm, port))
            return func
        except KeyError as e:
            logger.log_error("exception {} for port {} sm {}".format(e, port, sm))
        return None

    def run(self):
        # check first if the system supports independent mode and set boolean accordingly
        (platform_path, hwsku_dir) = device_info.get_paths_to_platform_and_hwsku_dirs()
        logger.log_info("hwsku_dir {} found, continue to check sai.profile file".format(hwsku_dir))
        independent_file = SAI_PROFILE_FILE.format(hwsku_dir)
        if os.path.isfile(independent_file):
            logger.log_info("file {} found, checking content for independent mode value".format(independent_file))
            with open(independent_file, "r") as independent_file_fd:
                found = False
                independent_file_content = ' '
                logger.log_info("file {} found, checking content for independent mode value".format(independent_file))
                while independent_file_content and not found:
                    independent_file_content = independent_file_fd.readline()
                    if SAI_INDEP_MODULE_MODE in independent_file_content and \
                            SAI_INDEP_MODULE_MODE_DELIMITER in independent_file_content:
                        independent_file_splitted = independent_file_content.split(SAI_INDEP_MODULE_MODE_DELIMITER)
                        if (len(independent_file_splitted) > 1):
                            self.is_supported_indep_mods_system = int(independent_file_splitted[1]) == int(SAI_INDEP_MODULE_MODE_TRUE_STR)
                            logger.log_info("file {} found, system will work in independent mode".format(independent_file))
                            logger.log_info("value of indep mode var: {} found in file".format(independent_file_splitted[1]))
                            found = True
        else:
            logger.log_info("file {} not found, system stays in legacy mode".format(independent_file))

        # static init - at first go over all ports and check each one if it's independent module or legacy
        self.sfp_changes_dict = {}
        # check for each port if the module connected and if it supports independent mode or legacy
        num_of_ports = DeviceDataManager.get_sfp_count()
        # create the modules sysfs fds poller
        self.poll_obj = select.poll()
        for port in range(num_of_ports):
            # check sysfs per port whether it's independent mode or legacy
            temp_module_sm = ModuleStateMachine(port_num=port, initial_state=STATE_HW_NOT_PRESENT
                                              , current_state=STATE_HW_NOT_PRESENT)
            module_fd_indep_path = SYSFS_INDEPENDENT_FD_PRESENCE.format(port)
            logger.log_info("system in indep mode: {} port {}".format(self.is_supported_indep_mods_system, port))
            if self.is_supported_indep_mods_system and os.path.isfile(module_fd_indep_path):
                logger.log_info("system in indep mode: {} port {} reading file {}".format(self.is_supported_indep_mods_system, port, module_fd_indep_path))
                temp_module_sm.set_is_indep_modules(True)
                temp_module_sm.set_module_fd_path(module_fd_indep_path)
                module_fd = open(module_fd_indep_path, "r")
                temp_module_sm.set_module_fd(module_fd)
            else:
                module_fd_legacy_path = self.get_sysfs_ethernet_port_fd(SYSFS_LEGACY_FD_PRESENCE, port)
                temp_module_sm.set_module_fd_path(module_fd_legacy_path)
                module_fd = open(module_fd_legacy_path, "r")
                temp_module_sm.set_module_fd(module_fd)
            # add lock to use with timer task updating next state per module object
            self.modules_lock_list.append(threading.Lock())
            # start SM for this independent module
            logger.log_info("adding temp_module_sm {} to sfp_port_dict".format(temp_module_sm))
            self.sfp_port_dict_initial[port] = temp_module_sm
            self.sfp_port_dict[port] = temp_module_sm

        i = 0
        # need at least 1 module in final state until it makes sense to send changes dict
        is_final_state_module = False
        all_static_detection_done = False
        logger.log_info(f"sfp_port_dict before starting static detection: {self.sfp_port_dict} main_thread_stop_event: "
                    f"{self.main_thread_stop_event.is_set()} all_static_detection_done: {all_static_detection_done}")
        # static detection - loop on different state for all ports until all done
        while not self.main_thread_stop_event.is_set() and not all_static_detection_done:
            logger.log_info("static detection running iteration {}".format(i))
            waiting_list_len = len(self.waiting_modules_list)
            sfp_port_dict_keys_len = len(self.sfp_port_dict.keys())
            if waiting_list_len == sfp_port_dict_keys_len:
                logger.log_info("static detection length of waiting list {}: {} and sfp port dict keys {}:{} is the same, sleeping 1 second..."
                              .format(waiting_list_len, self.waiting_modules_list, sfp_port_dict_keys_len, self.sfp_port_dict.keys()))
                time.sleep(1)
            else:
                logger.log_info("static detectionlength of waiting list {}: {} and sfp port dict keys {}: {} is different, NOT sleeping 1 second"
                              .format(waiting_list_len, self.waiting_modules_list, sfp_port_dict_keys_len, self.sfp_port_dict.keys()))
            for port_num, module_sm_obj in self.sfp_port_dict.items():
                curr_state = module_sm_obj.get_current_state()
                logger.log_info(f'static detection STATE_LOG {port_num}: curr_state is {curr_state}')
                func = self.get_sm_func(curr_state, port_num)
                logger.log_info("static detection got returned func {} for state {}".format(func, curr_state))
                try:
                    if not isinstance(func, str):
                        if func is not None:
                            next_state = func(port_num, module_sm_obj)
                except TypeError as e:
                    logger.log_info("static detection exception {} for port {} traceback:\n{}".format(e, port_num, traceback.format_exc()))
                    module_sm_obj.set_final_state(STATE_ERROR_HANDLER)
                    continue
                logger.log_info(f'static detection STATE_LOG {port_num}: next_state is {next_state}')
                if self.timer.is_alive():
                    logger.log_info("static detection timer threads is alive, acquiring lock")
                    self.modules_lock_list[port_num].acquire()
                # for STATE_NOT_POWERED we dont advance to next state, timerTask is doing it into STATE_POWERED
                if curr_state != STATE_NOT_POWERED or not module_sm_obj.wait_for_power_on:
                    module_sm_obj.set_next_state(next_state)
                    module_sm_obj.advance_state()
                if module_sm_obj.get_final_state():
                    logger.log_info(f'static detection STATE_LOG {port_num}: enter final state {module_sm_obj.get_final_state()}')
                    is_final_state_module = True
                if self.timer.is_alive():
                    self.modules_lock_list[port_num].release()
                is_timer_alive = self.timer.is_alive()
                logger.log_info("static detection timer thread is_alive {} port {}".format(is_timer_alive, port_num))
                if STATE_NOT_POWERED == curr_state:
                    if not is_timer_alive:
                        logger.log_info ("static detection curr_state is {} and timer thread is_alive {}, running timer task thread"
                               .format(curr_state, is_timer_alive))
                        # call timer task
                        self.timer = threading.Timer(1.0, self.timerTask)
                        self.timer.start()
                    if self.timer.is_alive():
                        logger.log_info("timer thread is_alive {}, locking module obj".format(self.timer.is_alive()))
                        self.modules_lock_list[port_num].acquire()
                    module_sm_obj.set_next_state(next_state)
                    if self.timer.is_alive():
                        logger.log_info("timer thread is_alive {}, releasing module obj".format(self.timer.is_alive()))
                        self.modules_lock_list[port_num].release()

            if is_final_state_module:
                self.map_ports_final_state()
                self.delete_ports_from_dict()
                self.send_changes_to_shared_queue()
                self.register_presece_closed_ports(False, self.register_hw_present_fds)
            i += 1
            self.register_hw_present_fds = []
            logger.log_info("sfp_port_dict: {}".format(self.sfp_port_dict))
            for port_num, module_sm_obj in self.sfp_port_dict.items():
                logger.log_info("static detection port_num: {} initial state: {} current_state: {} next_state: {}"
                       .format(port_num, module_sm_obj.initial_state, module_sm_obj.get_current_state()
                               , module_sm_obj.get_next_state()))
            sfp_port_dict_keys_len = len(self.sfp_port_dict.keys())
            if sfp_port_dict_keys_len == 0:
                logger.log_info("static detection len of keys of sfp_port_dict is 0: {}".format(sfp_port_dict_keys_len))
                all_static_detection_done = True
            else:
                logger.log_info("static detection len of keys of sfp_port_dict is not 0: {}".format(sfp_port_dict_keys_len))
            logger.log_info("static detection all_static_detection_done: {}".format(all_static_detection_done))

        logger.log_info(f"sfp_port_dict before dynamic detection: {self.sfp_port_dict} "
                        f"main_thread_stop_event.is_set(): {self.main_thread_stop_event.is_set()}")
        # dynamic detection - loop on polling changes, run state machine for them and put them into shared queue
        i = 0
        # need at least 1 module in final state until it makes sense to send changes dict
        is_final_state_module = False
        # initialize fds events count to 0
        for fd_fileno in self.fds_mapping_to_obj:
            module_obj = self.fds_mapping_to_obj[fd_fileno]['module_obj']
            # for debug purposes
            self.fds_events_count_dict[module_obj.port_num] = { 'presence' : 0 , 'power_good' : 0 }
        while not self.main_thread_stop_event.is_set():
            logger.log_info("dynamic detection running iteration {}".format(i))
            # poll for changes with 1 second timeout
            fds_events = self.poll_obj.poll(1000)
            logger.log_info("dynamic detection polled obj checking fds_events iteration {}".format(i))
            for fd, event in fds_events:
                # get modules object from fd according to saved key-value of fd-module obj saved earlier
                logger.log_info("dynamic detection working on fd {} event {}".format(fd, event))
                module_obj = self.fds_mapping_to_obj[fd]['module_obj']
                module_fd = self.fds_mapping_to_obj[fd]['fd']
                fd_name = self.fds_mapping_to_obj[fd]['fd_name']
                if 'presence' == fd_name:
                    module_fd_path = module_obj.module_fd_path
                elif 'power_good' == fd_name:
                    module_fd_path = module_obj.module_power_good_fd_path
                self.fds_events_count_dict[module_obj.port_num][fd_name] += 1
                try:
                    val = module_fd.read()
                    module_fd.seek(0)
                    logger.log_info("dynamic detection got module_obj {} with port {} from fd number {} path {} val {} count {}"
                                  .format(module_obj, module_obj.port_num, fd, module_fd_path
                                          , val, self.fds_events_count_dict[module_obj.port_num]))
                    # workaround for garbage received after the 0 or 1 value of sysfs i.e. 0#012 or 1#012
                    if len(val) > 1:
                        val = val[0]
                    if self.is_dummy_event(int(val), module_obj):
                        logger.log_info(f"dynamic detection dummy event port {module_obj.port_num} from fd number {fd}")
                        continue
                    if module_obj.port_num not in self.sfp_port_dict.keys():
                        logger.log_info("dynamic detection port {} not found in sfp_port_dict keys: {} adding it"
                                        .format(module_obj.port_num, self.sfp_port_dict.keys()))
                        self.deregister_fd_from_polling(module_obj.port_num)
                        # put again module obj in sfp_port_dict so next loop will work on it
                        self.sfp_port_dict[module_obj.port_num] = module_obj
                        self.delete_ports_and_reset_states_dict[module_obj.port_num] = val
                except Exception as e:
                    logger.log_error("dynamic detection exception on read presence {} for port {} fd name {} traceback:\n{}"
                                    .format(e, module_obj.port_num, module_fd.name, traceback.format_exc()))
            for port, val in self.delete_ports_and_reset_states_dict.items():
                logger.log_info(f"dynamic detection resetting all states for port {port} close_presence_ports {val}")
                module_obj = self.sfp_port_dict[port]
                module_obj.reset_all_states(close_presence_ports=val)
            self.delete_ports_and_reset_states_dict = {}
            for port_num, module_sm_obj in self.sfp_port_dict.items():
                curr_state = module_sm_obj.get_current_state()
                logger.log_info(f'dynamic detection STATE_LOG {port_num}: curr_state is {curr_state}')
                func = self.get_sm_func(curr_state, port)
                logger.log_info("dynamic detection got returned func {} for state {}".format(func, curr_state))
                try:
                    if func is not None:
                        next_state = func(port_num, module_sm_obj, dynamic=True)
                except TypeError as e:
                    logger.log_info("exception {} for port {}".format(e, port_num))
                    continue
                logger.log_info(f'dynamic detection STATE_LOG {port_num}: next_state is {next_state}')
                if self.timer.is_alive():
                    logger.log_info("dynamic detection timer threads is alive, acquiring lock")
                    self.modules_lock_list[port_num].acquire()
                if curr_state != STATE_NOT_POWERED or not module_sm_obj.wait_for_power_on:
                    module_sm_obj.set_next_state(next_state)
                    module_sm_obj.advance_state()
                if module_sm_obj.get_final_state():
                    logger.log_info(f'dynamic detection STATE_LOG {port_num}: enter final state {module_sm_obj.get_final_state()}')
                    is_final_state_module = True
                if self.timer.is_alive():
                    self.modules_lock_list[port_num].release()
                is_timer_alive = self.timer.is_alive()
                logger.log_info("dynamic detection timer thread is_alive {} port {}".format(is_timer_alive, port_num))
                if STATE_NOT_POWERED == curr_state:
                    if not is_timer_alive:
                        logger.log_info("dynamic detection curr_state is {} and timer thread is_alive {}, running timer task thread"
                                      .format(curr_state, is_timer_alive))
                        # call timer task
                        self.timer = threading.Timer(1.0, self.timerTask)
                        self.timer.start()
                    if self.timer.is_alive():
                        logger.log_info("dynamic detection timer thread is_alive {}, locking module obj".format(self.timer.is_alive()))
                        self.modules_lock_list[port_num].acquire()
                    module_sm_obj.set_next_state(next_state)
                    if self.timer.is_alive():
                        logger.log_info(
                            "dynamic detection timer thread is_alive {}, releasing module obj".format(self.timer.is_alive()))
                        self.modules_lock_list[port_num].release()

            if is_final_state_module:
                self.map_ports_final_state(dynamic=True)
                self.delete_ports_from_dict(dynamic=True)
                self.send_changes_to_shared_queue(dynamic=True)
                self.register_presece_closed_ports(True, self.register_hw_present_fds)
            if not self.sfp_port_dict and is_final_state_module:
                is_final_state_module = False
                logger.log_info(f"sft_port_dict is empty {self.sfp_port_dict}, set is_final_state_module to {is_final_state_module}")
            self.register_hw_present_fds = []
            i += 1
            logger.log_info("sfp_port_dict: {}".format(self.sfp_port_dict))
            for port_num, module_sm_obj in self.sfp_port_dict.items():
                logger.log_info("port_num: {} module_sm_obj initial state: {} current_state: {} next_state: {}"
                       .format(port_num, module_sm_obj.initial_state, module_sm_obj.get_current_state(), module_sm_obj.get_next_state()))

    def is_dummy_event(self, val, module_sm_obj):
        if val == 1:
            return module_sm_obj.final_state in (STATE_HW_PRESENT, STATE_SW_CONTROL, STATE_FW_CONTROL)
        elif val == 0:
            return module_sm_obj.final_state in (STATE_HW_NOT_PRESENT,)
        return False

    def check_if_hw_present(self, port, module_sm_obj, dynamic=False):
        detection_method = 'dynamic' if dynamic else 'static'
        logger.log_info(f"{detection_method} detection enter check_if_hw_present port {port} module_sm_obj {module_sm_obj}")
        module_fd_indep_path = module_sm_obj.module_fd_path
        if os.path.isfile(module_fd_indep_path):
            try:
                val_int = utils.read_int_from_file(module_fd_indep_path)
                if 0 == val_int:
                    logger.log_info("returning {} for val {}".format(STATE_HW_NOT_PRESENT, val_int))
                    retval_state = STATE_HW_NOT_PRESENT
                    module_sm_obj.set_final_state(retval_state, detection_method)
                    return retval_state
                elif 1 == val_int:
                    logger.log_info("returning {} for val {}".format(STATE_HW_PRESENT, val_int))
                    retval_state = STATE_HW_PRESENT
                    if not self.is_supported_indep_mods_system:
                        module_sm_obj.set_final_state(retval_state, detection_method)
                        self.register_fd_for_polling(module_sm_obj, module_sm_obj.module_fd, 'presence')
                    return retval_state
            except Exception as e:
                logger.log_info("exception {} for port {} setting final state STATE_ERROR_HANDLER".format(e, port))
                module_sm_obj.set_final_state(STATE_ERROR_HANDLER)
                return STATE_ERROR_HANDLER
        module_sm_obj.set_final_state(STATE_HW_NOT_PRESENT, detection_method)
        return STATE_HW_NOT_PRESENT

    def check_if_module_available(self, port, module_sm_obj, dynamic=False):
        logger.log_info("enter check_if_module_available port {} module_sm_obj {}".format(port, module_sm_obj))
        module_fd_indep_path = SYSFS_INDEPENDENT_FD_POWER_GOOD.format(port)
        if os.path.isfile(module_fd_indep_path):
            try:
                # not using utils.read_int_from_file since need to catch the exception here if no such file or it is
                # not accesible. utils.read_int_from_file will return 0 in such a case
                module_power_good_fd = open(module_fd_indep_path, "r")
                val = module_power_good_fd.read()
                val_int = int(val)
                module_sm_obj.module_power_good_fd_path = module_fd_indep_path
                module_sm_obj.module_power_good_fd = module_power_good_fd

                if 0 == val_int:
                    logger.log_info(f'port {port} power is not good')
                    module_sm_obj.set_final_state(STATE_HW_NOT_PRESENT)
                    return STATE_HW_NOT_PRESENT
                elif 1 == val_int:
                    logger.log_info(f'port {port} power is good')
                    return STATE_MODULE_AVAILABLE
            except Exception as e:
                logger.log_info("exception {} for port {}".format(e, port))
                return STATE_HW_NOT_PRESENT
        logger.log_info(f'port {port} has no power good file {module_fd_indep_path}')
        module_sm_obj.set_final_state(STATE_HW_NOT_PRESENT)
        return STATE_HW_NOT_PRESENT

    def check_if_power_on(self, port, module_sm_obj, dynamic=False):
        logger.log_info(f'enter check_if_power_on for port {port}')
        module_fd_indep_path = SYSFS_INDEPENDENT_FD_POWER_ON.format(port)
        if os.path.isfile(module_fd_indep_path):
            try:
                val_int = utils.read_int_from_file(module_fd_indep_path)
                if 0 == val_int:
                    logger.log_info(f'check_if_power_on port {port} is not powered')
                    return STATE_NOT_POWERED
                elif 1 == val_int:
                    logger.log_info(f'check_if_power_on port {port} is powered')
                    return STATE_POWERED
            except Exception as e:
                logger.log_info(f'check_if_power_on got exception {e}')
                module_sm_obj.set_final_state(STATE_HW_NOT_PRESENT)
                return STATE_HW_NOT_PRESENT

    def power_on_module(self, port, module_sm_obj, dynamic=False):
        logger.log_info(f'enter power_on_module for port {port}')
        if not module_sm_obj.wait_for_power_on:
            module_fd_indep_path_po = SYSFS_INDEPENDENT_FD_POWER_ON.format(port)
            module_fd_indep_path_r = SYSFS_INDEPENDENT_FD_HW_RESET.format(port)
            try:
                if os.path.isfile(module_fd_indep_path_po):
                    logger.log_info("powerOnModule powering on via {} for port {}".format(module_fd_indep_path_po, port))
                    # echo 1 > /sys/module/sx_core/$asic/$module/power_on
                    utils.write_file(module_fd_indep_path_po, "1")
                if os.path.isfile(module_fd_indep_path_r):
                    logger.log_info("powerOnModule resetting via {} for port {}".format(module_fd_indep_path_r, port))
                    # echo 0 > /sys/module/sx_core/$asic/$module/hw_reset
                    utils.write_file(module_fd_indep_path_r, "0")
                self.add_port_to_wait_reset(module_sm_obj)
            except Exception as e:
                logger.log_info("exception in powerOnModule {} for port {}".format(e, port))
                return STATE_HW_NOT_PRESENT
        return STATE_NOT_POWERED

    def check_module_type(self, port, module_sm_obj, dynamic=False):
        logger.log_info("enter check_module_type port {} module_sm_obj {}".format(port, module_sm_obj))
        sfp = sfp_module.SFP(port)
        xcvr_api = sfp.get_xcvr_api()
        if not xcvr_api:
            logger.log_info("check_module_type calling sfp reinit for port {} module_sm_obj {}"
                            .format(port, module_sm_obj))
            sfp.reinit()
            logger.log_info("check_module_type setting as FW control as xcvr_api is empty for port {} module_sm_obj {}"
                            .format(port, module_sm_obj))
            return STATE_FW_CONTROL
        # QSFP-DD ID is 24, OSFP ID is 25 - only these 2 are supported currently as independent module - SW controlled
        if not isinstance(xcvr_api, cmis.CmisApi):
            logger.log_info("check_module_type setting STATE_FW_CONTROL for {} in check_module_type port {} module_sm_obj {}"
                            .format(xcvr_api, port, module_sm_obj))
            return STATE_FW_CONTROL
        else:
            if xcvr_api.is_flat_memory():
                logger.log_info("check_module_type port {} setting STATE_FW_CONTROL module ID {} due to flat_mem device"
                              .format(xcvr_api, port))
                return STATE_FW_CONTROL
            logger.log_info("check_module_type checking power cap for {} in check_module_type port {} module_sm_obj {}"
                               .format(xcvr_api, port, module_sm_obj))
            power_cap = self.check_power_cap(port, module_sm_obj)
            if power_cap is STATE_POWER_LIMIT_ERROR:
                module_sm_obj.set_final_state(STATE_POWER_LIMIT_ERROR)
                return STATE_POWER_LIMIT_ERROR
            else:
                # first read the frequency support - if it's 1 then continue, if it's 0 no need to do anything
                module_fd_freq_support_path = SYSFS_INDEPENDENT_FD_FREQ_SUPPORT.format(port)
                val_int = utils.read_int_from_file(module_fd_freq_support_path)
                if 1 == val_int:
                    # read the module maximum supported clock of Management Comm Interface (MCI) from module EEPROM.
                    # from byte 2 bits 3-2:
                    # 00b means module supports up to 400KHz
                    # 01b means module supports up to 1MHz
                    logger.log_info(f"check_module_type reading mci max frequency for port {port}")
                    read_mci = xcvr_api.xcvr_eeprom.read_raw(2, 1)
                    logger.log_info(f"check_module_type read mci max frequency {read_mci} for port {port}")
                    mci_bits = read_mci & 0b00001100
                    logger.log_info(f"check_module_type read mci max frequency bits {mci_bits} for port {port}")
                    # Then, set it to frequency Sysfs using:
                    # echo <val> > /sys/module/sx_core/$asic/$module/frequency //  val: 0 - up to 400KHz, 1 - up to 1MHz
                    indep_fd_freq = SYSFS_INDEPENDENT_FD_FREQ.format(port)
                    utils.write_file(indep_fd_freq, mci_bits)
                return STATE_SW_CONTROL

    def check_power_cap(self, port, module_sm_obj, dynamic=False):
        logger.log_info("enter check_power_cap port {} module_sm_obj {}".format(port, module_sm_obj))
        sfp = sfp_module.SFP(port)
        xcvr_api = sfp.get_xcvr_api()
        field = xcvr_api.xcvr_eeprom.mem_map.get_field(consts.MAX_POWER_FIELD)
        powercap_ba = xcvr_api.xcvr_eeprom.reader(field.get_offset(), field.get_size())
        logger.log_info("check_power_cap got powercap bytearray {} for port {} module_sm_obj {}".format(powercap_ba, port, module_sm_obj))
        powercap = int.from_bytes(powercap_ba, "big")
        logger.log_info("check_power_cap got powercap {} for port {} module_sm_obj {}".format(powercap, port, module_sm_obj))
        indep_fd_power_limit = self.get_sysfs_ethernet_port_fd(SYSFS_INDEPENDENT_FD_POWER_LIMIT, port)
        cage_power_limit = utils.read_int_from_file(indep_fd_power_limit)
        logger.log_info("check_power_cap got cage_power_limit {} for port {} module_sm_obj {}".format(cage_power_limit, port, module_sm_obj))
        if powercap > int(cage_power_limit):
            logger.log_info("check_power_cap powercap {} != cage_power_limit {} for port {} module_sm_obj {}".format(powercap, cage_power_limit, port, module_sm_obj))
            module_sm_obj.set_final_state(STATE_POWER_LIMIT_ERROR)
            return STATE_POWER_LIMIT_ERROR

    def save_module_control_mode(self, port, module_sm_obj, dynamic=False):
        detection_method = 'dynamic' if dynamic else 'static'
        logger.log_info("{} detection save_module_control_mode setting current state {} for port {} as final state"
                        .format(detection_method, module_sm_obj.get_current_state(), port))
        state = module_sm_obj.get_current_state()
        module_sm_obj.set_final_state(state)
        try:
            if state == STATE_FW_CONTROL:
                # echo 0 > /sys/module/sx_core/$asic/$module/control
                indep_fd_fw_control = SYSFS_INDEPENDENT_FD_FW_CONTROL.format(port)
                utils.write_file(indep_fd_fw_control, "0")
                logger.log_info("save_module_control_mode set FW control for state {} port {}".format(state, port))
                # update the presence sysfs fd to legacy FD presence, first close the previous fd
                module_sm_obj.module_fd.close()
                module_fd_legacy_path = SYSFS_LEGACY_FD_PRESENCE.format(port)
                module_sm_obj.set_module_fd_path(module_fd_legacy_path)
                module_fd = open(module_fd_legacy_path, "r")
                module_sm_obj.set_module_fd(module_fd)
                logger.log_info("save_module_control_mode changed module fd to legacy present for port {}".format(port))
            else:
                # registering power good sysfs even if not good, so we can get an event from poller upon changes
                self.register_fd_for_polling(module_sm_obj, module_sm_obj.module_power_good_fd, 'power_good')
            # register the module's sysfs fd to poller with ERR and PRI attrs
            logger.log_info("save_module_control_mode registering sysfs fd {} number {} path {} for port {}"
                          .format(module_sm_obj.module_fd, module_sm_obj.module_fd.fileno(), module_sm_obj.set_module_fd_path, port))
        except Exception as e:
            logger.log_error("{} detection exception on read presence {} for port {} fd name {} traceback:\n{}"
                             .format(detection_method, e, port, module_sm_obj.module_fd.name, traceback.format_exc()))
        self.register_fd_for_polling(module_sm_obj, module_sm_obj.module_fd, 'presence')
        logger.log_info("save_module_control_mode set current state {} for port {} as final state {}".format(
            module_sm_obj.get_current_state(), port, module_sm_obj.get_final_state()))

    def register_fd_for_polling(self, module_sm_obj, fd, fd_name):
        self.fds_mapping_to_obj[fd.fileno()] = {'module_obj' : module_sm_obj,
                                                'fd': fd,
                                                'fd_name' : fd_name}
        if module_sm_obj.port_num not in self.port_to_fds:
            self.port_to_fds[module_sm_obj.port_num] = [fd]
        else:
            self.port_to_fds[module_sm_obj.port_num].append(fd)
        self.poll_obj.register(fd, select.POLLERR | select.POLLPRI)

    def deregister_fd_from_polling(self, port):
        if port in self.port_to_fds:
            fds = self.port_to_fds[port]
            for fd in fds:
                self.fds_mapping_to_obj.pop(fd.fileno())
                self.poll_obj.unregister(fd)
            self.port_to_fds.pop(port)

    def timerTask(self): # wakes up every 1 second
        logger.log_info("timerTask entered run state")
        empty = False
        i = 0
        while not empty:
            logger.log_info("timerTask while loop itartion {}".format(i))
            empty = True
            port_list_to_delete = []
            for port in self.waiting_modules_list:
                logger.log_info("timerTask working on port {}".format(port))
                empty = False
                module = self.sfp_port_dict[port]
                logger.log_info("timerTask got module with port_num {} from port {}".format(module.port_num, port))
                state = module.get_current_state()
                if module and state == STATE_NOT_POWERED:
                    logger.log_info("timerTask module {} current_state {} counting seconds since reset_start_time"
                                  .format(module, module.get_current_state()))
                    if time.time() - module.reset_start_time >= 3:
                        # set next state as STATE_POWERED state to trigger the function of check module type
                        logger.log_info("timerTask module port {} locking lock of port {}".format(module.port_num, module.port_num))
                        self.modules_lock_list[module.port_num].acquire()
                        logger.log_info("timerTask module port {} setting next state to STATE_POWERED".format(module.port_num))
                        module.set_next_state(STATE_POWERED)
                        logger.log_info("timerTask module port {} advancing next state".format(module.port_num))
                        module.advance_state()
                        logger.log_info("timerTask module port {} releasing lock of port {}".format(port, module.port_num))
                        self.modules_lock_list[module.port_num].release()
                        logger.log_info("timerTask module port {} adding to delete list to remove from waiting_modules_list".format(module.port_num))
                        port_list_to_delete.append(module.port_num)
            logger.log_info("timerTask deleting ports {} from waiting_modules_list...".format(port_list_to_delete))
            for port in port_list_to_delete:
                logger.log_info("timerTask deleting port {} from waiting_modules_list".format(port))
                self.waiting_modules_list.remove(port)
            logger.log_info("timerTask waiting_modules_list after deletion: {}".format(self.waiting_modules_list))
            time.sleep(1)
            i += 1

    def get_sysfs_ethernet_port_fd(self, sysfs_fd, port):
        sysfs_eth_port_fd = sysfs_fd.format(port)
        return sysfs_eth_port_fd

    def add_port_to_wait_reset(self, module_sm_obj):
        module_sm_obj.reset_start_time = time.time()
        logger.log_info("add_port_to_wait_reset reset_start_time {}".format(module_sm_obj.reset_start_time))
        module_sm_obj.wait_for_power_on = True
        logger.log_info("add_port_to_wait_reset wait_for_power_on {}".format(module_sm_obj.wait_for_power_on))
        self.waiting_modules_list.add(module_sm_obj.port_num)
        logger.log_info("add_port_to_wait_reset waiting_list after adding: {}".format(self.waiting_modules_list))

    def map_ports_final_state(self, dynamic=False):
        detection_method = 'dynamic' if dynamic else 'static'
        logger.log_info(f"{detection_method} detection enter map_ports_final_state")
        for port, module_obj in self.sfp_port_dict.items():
            final_state = module_obj.get_final_state()
            if final_state:
                # add port to delete list that we will iterate on later and delete the ports from sfp_port_dict
                self.sfp_delete_list_from_port_dict.append(port)
                if final_state in [STATE_HW_NOT_PRESENT, STATE_POWER_LIMIT_ERROR, STATE_ERROR_HANDLER]:
                    port_status = '0'
                    logger.log_info(f"{detection_method} detection adding port {port} to register_hw_present_fds")
                    self.register_hw_present_fds.append(module_obj)
                else:
                    port_status = '1'
                self.sfp_changes_dict[str(module_obj.port_num + 1)] = port_status

    def delete_ports_from_dict(self, dynamic=False):
        detection_method = 'dynamic' if dynamic else 'static'
        logger.log_info(f"{detection_method} detection sfp_port_dict before deletion: {self.sfp_port_dict}")
        for port in self.sfp_delete_list_from_port_dict:
            del self.sfp_port_dict[port]
        self.sfp_delete_list_from_port_dict = []
        logger.log_info("{} detection sfp_port_dict after deletion: {}".format(detection_method, self.sfp_port_dict))

    def send_changes_to_shared_queue(self, dynamic=False):
        detection_method = 'dynamic' if dynamic else 'static'
        if self.sfp_changes_dict:
            logger.log_info(f"{detection_method} detection putting sfp_changes_dict {self.sfp_changes_dict} "
                               f"in modules changes queue...")
            try:
                self.modules_changes_queue.put(self.sfp_changes_dict, timeout=1)
                self.sfp_changes_dict = {}
                logger.log_info(f"{detection_method} sfp_changes_dict after put changes: {self.sfp_changes_dict}")
            except queue.Full:
                logger.log_info(f"{detection_method} failed to put item from modules changes queue, queue is full")
        else:
            logger.log_info(f"{detection_method} sfp_changes_dict {self.sfp_changes_dict} is empty...")

    def register_presece_closed_ports(self, dynamic=False, module_obj_list=[]):
        detection_method = 'dynamic' if dynamic else 'static'
        logger.log_info(f"{detection_method} detection enter register_presence_closed_ports")
        for module_obj in module_obj_list:
            port = module_obj.port_num
            if self.is_supported_indep_mods_system:
                module_fd_indep_path = SYSFS_INDEPENDENT_FD_PRESENCE.format(port)
            else:
                module_fd_indep_path = SYSFS_LEGACY_FD_PRESENCE.format(port)
            module_obj.set_module_fd_path(module_fd_indep_path)
            module_fd = open(module_fd_indep_path, "r")
            module_obj.set_module_fd(module_fd)
            logger.log_info(f"{detection_method} registering fd {module_fd} fd name {module_fd.name} for port {port}")
            self.register_fd_for_polling(module_obj, module_fd, 'presence')

class ModuleStateMachine(object):

    def __init__(self, port_num=0, initial_state=STATE_HW_NOT_PRESENT, current_state=STATE_HW_NOT_PRESENT
                 , next_state=STATE_HW_NOT_PRESENT, final_state='', is_indep_module=False
                 , module_fd_path='', module_fd=None, reset_start_time=None
                 , eeprom_poweron_reset_retries=1, module_power_good_fd_path=None, module_power_good_fd=None):

        self.port_num = port_num
        self.initial_state = initial_state
        self.current_state = current_state
        self.next_state = next_state
        self.final_state = final_state
        self.is_indep_modules = is_indep_module
        self.module_fd_path = module_fd_path
        self.module_fd = module_fd
        self.reset_start_time = reset_start_time
        self.wait_for_power_on = False
        self.eeprom_poweron_reset_retries = eeprom_poweron_reset_retries
        self.module_power_good_fd_path = module_power_good_fd_path
        self.module_power_good_fd = module_power_good_fd

    def set_initial_state(self, state):
        self.initial_state = state

    def get_current_state(self):
        return self.current_state

    def set_current_state(self, state):
        self.current_state = state

    def get_next_state(self):
        return self.next_state

    def set_next_state(self, state):
        self.next_state = state

    def get_final_state(self):
        return self.final_state

    def set_final_state(self, state, detection_method='static'):
        logger.log_info(f"{detection_method} set_final_state setting {state} port {self.port_num}")
        self.final_state = state

    def advance_state(self):
        self.set_current_state(self.next_state)
        self.next_state = ''

    def set_is_indep_modules(self, is_indep_modules):
        self.is_indep_modules = is_indep_modules

    def set_module_fd_path(self, module_fd_path):
        self.module_fd_path = module_fd_path

    def set_module_fd(self, module_fd):
        self.module_fd = module_fd

    def reset_all_states(self, def_state=STATE_HW_NOT_PRESENT, retries=1, close_presence_ports='0'):
        self.initial_state = def_state
        self.current_state = def_state
        self.next_state = def_state
        self.final_state = ''
        self.wait_for_power_on = False
        self.eeprom_poweron_reset_retries = retries
        if '0' == close_presence_ports:
            self.module_fd.close()
        if self.module_power_good_fd:
            self.module_power_good_fd.close()
