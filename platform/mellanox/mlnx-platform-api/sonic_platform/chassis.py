#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2019-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
#############################################################################
# Mellanox
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Chassis information which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_py_common.logger import Logger
    import os
    from sonic_py_common import device_info
    from functools import reduce
    from .utils import extract_RJ45_ports_index
    from .utils import extract_cpo_ports_index
    from . import module_host_mgmt_initializer
    from . import utils
    from .device_data import DeviceDataManager
    import re
    import select
    import threading
    import time
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


RJ45_TYPE = "RJ45"

VPD_DATA_FILE = "/var/run/hw-management/eeprom/vpd_data"
REVISION = "REV"

HWMGMT_SYSTEM_ROOT = '/var/run/hw-management/system/'

#reboot cause related definitions
REBOOT_CAUSE_ROOT = HWMGMT_SYSTEM_ROOT
REBOOT_CAUSE_MAX_WAIT_TIME = 45
REBOOT_CAUSE_CHECK_INTERVAL = 5
REBOOT_CAUSE_READY_FILE = '/run/hw-management/config/reset_attr_ready'

REBOOT_TYPE_KEXEC_FILE = "/proc/cmdline"
REBOOT_TYPE_KEXEC_PATTERN_WARM = ".*SONIC_BOOT_TYPE=(warm|fastfast).*"
REBOOT_TYPE_KEXEC_PATTERN_FAST = ".*SONIC_BOOT_TYPE=(fast|fast-reboot).*"

SYS_DISPLAY = "SYS_DISPLAY"

# Global logger class instance
logger = Logger()

class Chassis(ChassisBase):
    """Platform-specific Chassis class"""

    # System status LED
    _led = None

    # System UID LED
    _led_uid = None

    chassis_instance = None

    def __init__(self):
        super(Chassis, self).__init__()

        # Initialize vpd data
        self.vpd_data = None

        # move the initialization of each components to their dedicated initializer
        # which will be called from platform
        #
        # Multiple scenarios need to be taken into consideration regarding the SFP modules initialization.
        # - Platform daemons
        #   - Can access multiple or all SFP modules
        # - sfputil
        #   - Sometimes can access only one SFP module
        #   - Call get_sfp to get one SFP module object
        #
        # We should initialize all SFP modules only if it is necessary because initializing SFP module is time-consuming.
        # This means,
        # - If get_sfp is called,
        #    - If the _sfp_list isn't initialized, initialize it first.
        #    - Only the SFP module being required should be initialized.
        # - If get_all_sfps is called,
        #    - If the _sfp_list isn't initialized, initialize it first.
        #    - All SFP modules need to be initialized.
        #      But the SFP modules that have already been initialized should not be initialized for the second time.
        #      This can caused by get_sfp being called before.
        #
        # Due to the complexity of SFP modules initialization, we have to introduce two initialized flags for SFP modules
        # - sfp_module_partial_initialized:
        #    - False: The _sfp_list is [] (SFP stuff has never been accessed)
        #    - True: The _sfp_list is a list whose length is number of SFP modules supported by the platform
        # - sfp_module_full_initialized:
        #    - False: All SFP modules have not been created
        #    - True: All SFP modules have been created
        #
        self.sfp_initialized_count = 0
        self.sfp_event = None
        self.reboot_cause_initialized = False

        self.sfp_module = None
        self.sfp_lock = threading.Lock()

        # Build the RJ45 port list from platform.json and hwsku.json
        self._RJ45_port_inited = False
        self._RJ45_port_list = None

        # Build the CPO port list from platform.json and hwsku.json
        self._cpo_port_inited = False
        self._cpo_port_list = None

        Chassis.chassis_instance = self

        self.module_host_mgmt_initializer = module_host_mgmt_initializer.ModuleHostMgmtInitializer()
        self.poll_obj = None
        self.registered_fds = None

        logger.log_info("Chassis loaded successfully")

    def __del__(self):
        if self.sfp_event:
            self.sfp_event.deinitialize()

    @property
    def RJ45_port_list(self):
        if not self._RJ45_port_inited:
            self._RJ45_port_list = extract_RJ45_ports_index()
            self._RJ45_port_inited = True
        return self._RJ45_port_list

    @property
    def cpo_port_list(self):
        if not self._cpo_port_inited:
            self._cpo_port_list = extract_cpo_ports_index()
            self._cpo_port_inited = True
        return self._cpo_port_list

    ##############################################
    # PSU methods
    ##############################################

    def initialize_psu(self):
        if not self._psu_list:
            from .psu import Psu, FixedPsu
            psu_count = DeviceDataManager.get_psu_count()
            if psu_count == 0:
                # For system with no PSU, for example, PDU system.
                return
            hot_swapable = DeviceDataManager.is_psu_hotswapable()

            # Initialize PSU list
            for index in range(psu_count):
                if hot_swapable:
                    psu = Psu(index)
                else:
                    psu = FixedPsu(index)
                self._psu_list.append(psu)

    def get_num_psus(self):
        """
        Retrieves the number of power supply units available on this chassis

        Returns:
            An integer, the number of power supply units available on this
            chassis
        """
        self.initialize_psu()
        return len(self._psu_list)

    def get_all_psus(self):
        """
        Retrieves all power supply units available on this chassis

        Returns:
            A list of objects derived from PsuBase representing all power
            supply units available on this chassis
        """
        self.initialize_psu()
        return self._psu_list

    def get_psu(self, index):
        """
        Retrieves power supply unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the power supply unit to
            retrieve

        Returns:
            An object dervied from PsuBase representing the specified power
            supply unit
        """
        self.initialize_psu()
        return super(Chassis, self).get_psu(index)

    ##############################################
    # Fan methods
    ##############################################

    def initialize_fan(self):
        if not self._fan_drawer_list:
            from .fan import Fan
            from .fan_drawer import RealDrawer, VirtualDrawer

            drawer_num = DeviceDataManager.get_fan_drawer_count()
            if drawer_num == 0:
                # For system with no fan, for example, liquid cooling system.
                return
            hot_swapable = DeviceDataManager.is_fan_hotswapable()
            fan_num = DeviceDataManager.get_fan_count()
            fan_num_per_drawer = fan_num // drawer_num
            drawer_ctor = RealDrawer if hot_swapable else VirtualDrawer
            fan_index = 0
            for drawer_index in range(drawer_num):
                drawer = drawer_ctor(drawer_index)
                self._fan_drawer_list.append(drawer)
                for index in range(fan_num_per_drawer):
                    fan = Fan(fan_index, drawer, index + 1)
                    fan_index += 1
                    drawer._fan_list.append(fan)

    def get_num_fan_drawers(self):
        """
        Retrieves the number of fan drawers available on this chassis

        Returns:
            An integer, the number of fan drawers available on this chassis
        """
        return DeviceDataManager.get_fan_drawer_count()

    def get_all_fan_drawers(self):
        """
        Retrieves all fan drawers available on this chassis

        Returns:
            A list of objects derived from FanDrawerBase representing all fan
            drawers available on this chassis
        """
        self.initialize_fan()
        return self._fan_drawer_list

    def get_fan_drawer(self, index):
        """
        Retrieves fan drawers represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the fan drawer to
            retrieve

        Returns:
            An object dervied from FanDrawerBase representing the specified fan
            drawer
        """
        self.initialize_fan()
        return super(Chassis, self).get_fan_drawer(index)

    ##############################################
    # SFP methods
    ##############################################

    def _import_sfp_module(self):
        if not self.sfp_module:
            from . import sfp as sfp_module
            self.sfp_module = sfp_module
        return self.sfp_module

    def initialize_single_sfp(self, index):
        sfp_count = self.get_num_sfps()
        # Use double checked locking mechanism for:
        #     1. protect shared resource self._sfp_list
        #     2. performance (avoid locking every time)
        if index < sfp_count:
            if not self._sfp_list or not self._sfp_list[index]:
                with self.sfp_lock:
                    if not self._sfp_list:
                        self._sfp_list = [None] * sfp_count

                    if not self._sfp_list[index]:
                        sfp_module = self._import_sfp_module()
                        if self.RJ45_port_list and index in self.RJ45_port_list:
                            self._sfp_list[index] = sfp_module.RJ45Port(index)
                        elif self.cpo_port_list and index in self.cpo_port_list:
                            self._sfp_list[index] = sfp_module.CpoPort(index)
                        else:
                            self._sfp_list[index] = sfp_module.SFP(index)
                        self.sfp_initialized_count += 1

    def initialize_sfp(self):
        sfp_count = self.get_num_sfps()
        # Use double checked locking mechanism for:
        #     1. protect shared resource self._sfp_list
        #     2. performance (avoid locking every time)
        if sfp_count != self.sfp_initialized_count:
            with self.sfp_lock:
                if sfp_count != self.sfp_initialized_count:
                    if not self._sfp_list:
                        sfp_module = self._import_sfp_module()
                        for index in range(sfp_count):
                            if self.RJ45_port_list and index in self.RJ45_port_list:
                                sfp_object = sfp_module.RJ45Port(index)
                            elif self.cpo_port_list and index in self.cpo_port_list:
                                sfp_object = sfp_module.CpoPort(index)
                            else:
                                sfp_object = sfp_module.SFP(index)
                            self._sfp_list.append(sfp_object)
                        self.sfp_initialized_count = sfp_count
                    elif self.sfp_initialized_count != len(self._sfp_list):
                        sfp_module = self._import_sfp_module()
                        for index in range(len(self._sfp_list)):
                            if self._sfp_list[index] is None:
                                if self.RJ45_port_list and index in self.RJ45_port_list:
                                    self._sfp_list[index] = sfp_module.RJ45Port(index)
                                elif self.cpo_port_list and index in self.cpo_port_list:
                                    self._sfp_list[index] = sfp_module.CpoPort(index)
                                else:
                                    self._sfp_list[index] = sfp_module.SFP(index)
                        self.sfp_initialized_count = len(self._sfp_list)

    def get_num_sfps(self):
        """
        Retrieves the number of sfps available on this chassis

        Returns:
            An integer, the number of sfps available on this chassis
        """
        num_sfps = 0
        if not self._RJ45_port_inited:
            self._RJ45_port_list = extract_RJ45_ports_index()
            self._RJ45_port_inited = True
        
        if not self._cpo_port_inited:
            self._cpo_port_list = extract_cpo_ports_index()
            self._cpo_port_inited = True
        
        num_sfps = DeviceDataManager.get_sfp_count()
        if self._RJ45_port_list is not None:
            num_sfps += len(self._RJ45_port_list)
        if self._cpo_port_list is not None:
            num_sfps += len(self._cpo_port_list)
        
        return num_sfps

    def get_all_sfps(self):
        """
        Retrieves all sfps available on this chassis

        Returns:
            A list of objects derived from SfpBase representing all sfps
            available on this chassis
        """    
        if DeviceDataManager.is_module_host_management_mode():
            self.module_host_mgmt_initializer.initialize(self)
        else:
            self.initialize_sfp()
        return self._sfp_list

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>

        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
                   The index should be the sequence of a physical port in a chassis,
                   starting from 1.
                   For example, 1 for Ethernet0, 2 for Ethernet4 and so on.

        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        index = index - 1
        if DeviceDataManager.is_module_host_management_mode():
            self.module_host_mgmt_initializer.initialize(self)
        else:
            self.initialize_single_sfp(index)
        return super(Chassis, self).get_sfp(index)

    def get_port_or_cage_type(self, index):
        """
        Retrieves sfp port or cage type corresponding to physical port <index>

        Args:
            index: An integer (>=0), the index of the sfp to retrieve.
                   The index should correspond to the physical port in a chassis.
                   For example:-
                   1 for Ethernet0, 2 for Ethernet4 and so on for one platform.
                   0 for Ethernet0, 1 for Ethernet4 and so on for another platform.

        Returns:
            The masks of all types of port or cage that can be supported on the port
            Types are defined in sfp_base.py
            Eg.
                Both SFP and SFP+ are supported on the port, the return value should be 0x0a
                which is 0x02 | 0x08
        """
        index = index - 1
        if self.RJ45_port_list and index in self.RJ45_port_list:
            from sonic_platform_base.sfp_base import SfpBase
            return SfpBase.SFP_PORT_TYPE_BIT_RJ45
        raise NotImplementedError

    def get_change_event(self, timeout=0):
        """
        Returns a nested dictionary containing all devices which have
        experienced a change at chassis level

        Args:
            timeout: Timeout in milliseconds (optional). If timeout == 0,
                this method will block until a change is detected.

        Returns:
            (bool, dict):
                - True if call successful, False if not; - Deprecated, will always return True
                - A nested dictionary where key is a device type,
                  value is a dictionary with key:value pairs in the format of
                  {'device_id':'device_event'},
                  where device_id is the device ID for this device and
                        device_event,
                             status='1' represents device inserted,
                             status='0' represents device removed.
                  Ex. {'fan':{'0':'0', '2':'1'}, 'sfp':{'11':'0'}}
                      indicates that fan 0 has been removed, fan 2
                      has been inserted and sfp 11 has been removed.
        """
        if DeviceDataManager.is_module_host_management_mode():
            self.module_host_mgmt_initializer.initialize(self)
            return self.get_change_event_for_module_host_management_mode(timeout)
        else:
            self.initialize_sfp()
            return self.get_change_event_legacy(timeout)
            
    def get_change_event_for_module_host_management_mode(self, timeout):
        """Get SFP change event when module host management mode is enabled.

        Args:
            timeout: Timeout in milliseconds (optional). If timeout == 0,
                this method will block until a change is detected.

        Returns:
            (bool, dict):
                - True if call successful, False if not; - Deprecated, will always return True
                - A nested dictionary where key is a device type,
                  value is a dictionary with key:value pairs in the format of
                  {'device_id':'device_event'},
                  where device_id is the device ID for this device and
                        device_event,
                             status='1' represents device inserted,
                             status='0' represents device removed.
                  Ex. {'fan':{'0':'0', '2':'1'}, 'sfp':{'11':'0'}}
                      indicates that fan 0 has been removed, fan 2
                      has been inserted and sfp 11 has been removed.
        """
        if not self.poll_obj:
            self.poll_obj = select.poll()
            self.registered_fds = {}
            for s in self._sfp_list:
                fds = s.get_fds_for_poling()
                for fd_type, fd in fds.items():
                    self.poll_obj.register(fd, select.POLLERR | select.POLLPRI)
                    self.registered_fds[fd.fileno()] = (s.sdk_index, fd, fd_type)

            logger.log_debug(f'Registered SFP file descriptors for polling: {self.registered_fds}')
                    
        from . import sfp
        
        wait_forever = (timeout == 0)
        # poll timeout should be no more than 1000ms to ensure fast shutdown flow
        timeout = 1000.0 if timeout >= 1000 else float(timeout)
        port_dict = {}
        error_dict = {}
        begin = time.monotonic()
        wait_ready_task = sfp.SFP.get_wait_ready_task()
        
        while True:        
            fds_events = self.poll_obj.poll(timeout)
            for fileno, _ in fds_events:
                if fileno not in self.registered_fds:
                    logger.log_error(f'Unknown file no {fileno} from poll event, registered files are {self.registered_fds}')
                    continue
                
                sfp_index, fd, fd_type = self.registered_fds[fileno]
                s = self._sfp_list[sfp_index]
                fd.seek(0)
                fd_value = int(fd.read().strip())

                # Detecting dummy event
                if s.is_dummy_event(fd_type, fd_value):
                    # Ignore dummy event for the first poll, assume SDK only provide 1 dummy event
                    logger.log_debug(f'Ignore dummy event {fd_type}:{fd_value} for SFP {sfp_index}')
                    continue

                logger.log_notice(f'Got SFP event: index={sfp_index}, type={fd_type}, value={fd_value}')
                if fd_type == 'hw_present':
                    # event could be EVENT_NOT_PRESENT or EVENT_PRESENT
                    event = sfp.EVENT_NOT_PRESENT if fd_value == 0 else sfp.EVENT_PRESENT
                    if fd_value == 1:
                        s.processing_insert_event = True
                    s.on_event(event)
                elif fd_type == 'present':
                    if str(fd_value) == sfp.SFP_STATUS_ERROR:
                        # FW control cable got an error, no need trigger state machine
                        sfp_status, error_desc = s.get_error_info_from_sdk_error_type()
                        port_dict[sfp_index + 1] = sfp_status
                        if error_desc: 
                            error_dict[sfp_index + 1] = error_desc
                        continue
                    elif str(fd_value) == sfp.SFP_STATUS_INSERTED:
                        # FW control cable got present, only case is that the cable is recovering
                        # from an error. FW control cable has no transition from "Not Present" to "Present"
                        # because "Not Present" cable is always "software control" and should always poll
                        # hw_present sysfs instead of present sysfs.
                        port_dict[sfp_index + 1] = sfp.SFP_STATUS_INSERTED
                        continue
                    else:
                        s.on_event(sfp.EVENT_NOT_PRESENT)
                else:
                    # event could be EVENT_POWER_GOOD or EVENT_POWER_BAD
                    event = sfp.EVENT_POWER_BAD if fd_value == 0 else sfp.EVENT_POWER_GOOD
                    s.on_event(event)
                    
                if s.in_stable_state():
                    self.sfp_module.SFP.wait_sfp_eeprom_ready([s], 2)
                    s.fill_change_event(port_dict)
                    s.refresh_poll_obj(self.poll_obj, self.registered_fds)
                else:
                    logger.log_debug(f'SFP {sfp_index} does not reach stable state, state={s.state}')
                    
            ready_sfp_set = wait_ready_task.get_ready_set()
            for sfp_index in ready_sfp_set:
                s = self._sfp_list[sfp_index]
                s.on_event(sfp.EVENT_RESET_DONE)
                if s.in_stable_state():
                    self.sfp_module.SFP.wait_sfp_eeprom_ready([s], 2)
                    s.fill_change_event(port_dict)
                    s.refresh_poll_obj(self.poll_obj, self.registered_fds)
                else:
                    logger.log_error(f'SFP {sfp_index} failed to reach stable state, state={s.state}')
                    
            if port_dict:
                logger.log_notice(f'Sending SFP change event: {port_dict}, error event: {error_dict}')
                self.reinit_sfps(port_dict)
                return True, {
                    'sfp': port_dict,
                    'sfp_error': error_dict
                }
            else:
                if not wait_forever:
                    elapse = time.monotonic() - begin
                    if elapse * 1000 >= timeout:
                        return True, {'sfp': {}}

    def get_change_event_legacy(self, timeout):
        """Get SFP change event when module host management is disabled.

        Args:
            timeout (int): polling timeout in ms

        Returns:
            (bool, dict):
                - True if call successful, False if not; - Deprecated, will always return True
                - A nested dictionary where key is a device type,
                  value is a dictionary with key:value pairs in the format of
                  {'device_id':'device_event'},
                  where device_id is the device ID for this device and
                        device_event,
                             status='1' represents device inserted,
                             status='0' represents device removed.
                  Ex. {'fan':{'0':'0', '2':'1'}, 'sfp':{'11':'0'}}
                      indicates that fan 0 has been removed, fan 2
                      has been inserted and sfp 11 has been removed.
        """
        if not self.poll_obj:
            self.poll_obj = select.poll()
            self.registered_fds = {}
            # SDK always sent event for the first time polling. Such event should not be sent to xcvrd.
            # Store SFP state before first time polling so that we can detect dummy event.
            self.sfp_states_before_first_poll = {}
            for s in self._sfp_list:
                fd = s.get_fd_for_polling_legacy()
                self.poll_obj.register(fd, select.POLLERR | select.POLLPRI)
                self.registered_fds[fd.fileno()] = (s.sdk_index, fd)
                self.sfp_states_before_first_poll[s.sdk_index] = s.get_module_status()

            logger.log_debug(f'Registered SFP file descriptors for polling: {self.registered_fds}')
            
        from . import sfp
        
        wait_forever = (timeout == 0)
        # poll timeout should be no more than 1000ms to ensure fast shutdown flow
        timeout = 1000.0 if timeout >= 1000 else float(timeout)
        port_dict = {}
        error_dict = {}
        begin = time.monotonic()
        
        while True:
            fds_events = self.poll_obj.poll(timeout)
            for fileno, _ in fds_events:
                if fileno not in self.registered_fds:
                    logger.log_error(f'Unknown file no {fileno} from poll event, registered files are {self.registered_fds}')
                    continue
                
                sfp_index, fd = self.registered_fds[fileno]
                fd.seek(0)
                fd.read()
                s = self._sfp_list[sfp_index]
                sfp_status = s.get_module_status()

                if sfp_index in self.sfp_states_before_first_poll:
                    # Detecting dummy event
                    sfp_state_before_poll = self.sfp_states_before_first_poll[sfp_index]
                    self.sfp_states_before_first_poll.pop(sfp_index)
                    if sfp_state_before_poll == sfp_status:
                        # Ignore dummy event for the first poll, assume SDK only provide 1 dummy event
                        logger.log_debug(f'Ignore dummy event {sfp_status} for SFP {sfp_index}')
                        continue

                logger.log_notice(f'Got SFP event: index={sfp_index}, value={sfp_status}')
                if sfp_status == sfp.SFP_STATUS_UNKNOWN:
                    # in the following sequence, STATUS_UNKNOWN can be returned.
                    # so we shouldn't raise exception here.
                    # 1. some sfp module is inserted
                    # 2. sfp_event gets stuck and fails to fetch the change event instantaneously
                    # 3. and then the sfp module is removed
                    # 4. sfp_event starts to try fetching the change event
                    logger.log_notice("unknown module state, maybe the port suffers two adjacent insertion/removal")
                    continue

                if sfp_status == sfp.SFP_STATUS_ERROR:
                    sfp_status, error_desc = s.get_error_info_from_sdk_error_type()
                    if error_desc:
                        error_dict[sfp_index + 1] = error_desc
                port_dict[sfp_index + 1] = sfp_status

            if port_dict:
                logger.log_notice(f'Sending SFP change event: {port_dict}, error event: {error_dict}')
                self.reinit_sfps(port_dict)
                return True, {
                    'sfp': port_dict,
                    'sfp_error': error_dict
                }
            else:
                if not wait_forever:
                    elapse = time.monotonic() - begin
                    if elapse * 1000 >= timeout:
                        return True, {'sfp': {}}

    def reinit_sfps(self, port_dict):
        """
        Re-initialize SFP if there is any newly inserted SFPs
        :param port_dict: SFP event data
        :return:
        """
        from . import sfp
        for index, status in port_dict.items():
            if status == sfp.SFP_STATUS_REMOVED:
                try:
                    self._sfp_list[int(index) - 1].reinit()
                except Exception as e:
                    logger.log_error("Fail to re-initialize SFP {} - {}".format(index, repr(e)))

    def _show_capabilities(self):
        """
        This function is for debug purpose
        Some features require a xSFP module to support some capabilities but it's unrealistic to
        check those modules one by one.
        So this function is introduce to show some capabilities of all xSFP modules mounted on the device.
        """
        self.initialize_sfp()
        for s in self._sfp_list:
            try:
                print("index {} tx disable {} dom {} calibration {} temp {} volt {} power (tx {} rx {})".format(s.index,
                    s.dom_tx_disable_supported,
                    s.dom_supported,
                    s.calibration,
                    s.dom_temp_supported,
                    s.dom_volt_supported,
                    s.dom_rx_power_supported,
                    s.dom_tx_power_supported
                    ))
            except:
                print("fail to retrieve capabilities for module index {}".format(s.index))

    ##############################################
    # THERMAL methods
    ##############################################

    def initialize_thermals(self):
        if not self._thermal_list:
            from .thermal import initialize_chassis_thermals
            # Initialize thermals
            self._thermal_list = initialize_chassis_thermals()

    def get_num_thermals(self):
        """
        Retrieves the number of thermals available on this chassis

        Returns:
            An integer, the number of thermals available on this chassis
        """
        self.initialize_thermals()
        return len(self._thermal_list)

    def get_all_thermals(self):
        """
        Retrieves all thermals available on this chassis

        Returns:
            A list of objects derived from ThermalBase representing all thermals
            available on this chassis
        """
        self.initialize_thermals()
        return self._thermal_list

    def get_thermal(self, index):
        """
        Retrieves thermal unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the thermal to
            retrieve

        Returns:
            An object dervied from ThermalBase representing the specified thermal
        """
        self.initialize_thermals()
        return super(Chassis, self).get_thermal(index)

    ##############################################
    # EEPROM methods
    ##############################################

    def initialize_eeprom(self):
        if not self._eeprom:
            from .eeprom import Eeprom
            # Initialize EEPROM
            self._eeprom = Eeprom()

    def get_eeprom(self):
        """
        Retreives eeprom device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            eeprom device
        """
        self.initialize_eeprom()
        return self._eeprom

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        self.initialize_eeprom()
        return self._eeprom.get_product_name()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        model = None
        if self._read_model_from_vpd():
            if not self.vpd_data:
                self.vpd_data = self._parse_vpd_data(VPD_DATA_FILE)
            model = self.vpd_data.get(SYS_DISPLAY, "N/A")
        else:
            self.initialize_eeprom()
            model = self._eeprom.get_part_number()
        return model

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        self.initialize_eeprom()
        return self._eeprom.get_base_mac()

    def get_serial(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this chassis.
        """
        self.initialize_eeprom()
        return self._eeprom.get_serial_number()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        self.initialize_eeprom()
        return self._eeprom.get_system_eeprom_info()

    ##############################################
    # Component methods
    ##############################################

    def initialize_components(self):
        if not utils.is_host():
            return
        if not self._component_list:
            # Initialize component list
            from .component import ComponentONIE, ComponentSSD, ComponentBIOS, ComponentCPLD
            self._component_list.append(ComponentONIE())
            self._component_list.append(ComponentSSD())
            self._component_list.append(DeviceDataManager.get_bios_component())
            self._component_list.extend(DeviceDataManager.get_cpld_component_list())

    def get_num_components(self):
        """
        Retrieves the number of components available on this chassis

        Returns:
            An integer, the number of components available on this chassis
        """
        self.initialize_components()
        return len(self._component_list)

    def get_all_components(self):
        """
        Retrieves all components available on this chassis

        Returns:
            A list of objects derived from ComponentBase representing all components
            available on this chassis
        """
        self.initialize_components()
        return self._component_list

    def get_component(self, index):
        """
        Retrieves component represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the component to retrieve

        Returns:
            An object dervied from ComponentBase representing the specified component
        """
        self.initialize_components()
        return super(Chassis, self).get_component(index)

    ##############################################
    # System LED methods
    ##############################################

    def initizalize_system_led(self):
        if not Chassis._led:
            from .led import SystemLed, \
                SystemUidLed
            Chassis._led = SystemLed()
            Chassis._led_uid = SystemUidLed()

    def set_status_led(self, color):
        """
        Sets the state of the system LED

        Args:
            color: A string representing the color with which to set the
                   system LED

        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        self.initizalize_system_led()
        return False if not Chassis._led else Chassis._led.set_status(color)

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        self.initizalize_system_led()
        return None if not Chassis._led else Chassis._led.get_status()

    def set_uid_led(self, color):
        """
        Sets the state of the system UID LED

        Args:
            color: A string representing the color with which to set the
                   system UID LED

        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        self.initizalize_system_led()
        return False if not Chassis._led_uid else Chassis._led_uid.set_status(color)

    def get_uid_led(self):
        """
        Gets the state of the system UID LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        self.initizalize_system_led()
        return None if not Chassis._led_uid else Chassis._led_uid.get_status()

    def get_watchdog(self):
        """
        Retrieves hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device

        Note:
            We overload this method to ensure that watchdog is only initialized
            when it is referenced. Currently, only one daemon can open the watchdog.
            To initialize watchdog in the constructor causes multiple daemon
            try opening watchdog when loading and constructing a chassis object
            and fail. By doing so we can eliminate that risk.
        """
        try:
            if self._watchdog is None:
                from .watchdog import get_watchdog
                self._watchdog = get_watchdog()
        except Exception as e:
            logger.log_info("Fail to load watchdog due to {}".format(repr(e)))

        return self._watchdog


    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        if not self.vpd_data:
            self.vpd_data = self._parse_vpd_data(VPD_DATA_FILE)

        return self.vpd_data.get(REVISION, "N/A")

    def _parse_vpd_data(self, filename):
        """
        Read vpd_data and returns a dictionary of values

        Returns:
            A dictionary containing the dmi table of the switch chassis info
        """
        result = {}
        try:
            if not os.access(filename, os.R_OK):
                return result

            result = utils.read_key_value_file(filename, delimeter=": ")
                
        except Exception as e:
            logger.log_error("Fail to decode vpd_data {} due to {}".format(filename, repr(e)))

        return result

    def _read_model_from_vpd(self):
        """
        Returns if model number should be returned from VPD file

        Returns:
            Returns True if spectrum version is higher than Spectrum-4 according to sku number
        """
        sku = device_info.get_hwsku()
        sku_num = re.search('[0-9]{4}', sku).group()
        # fallback to spc1 in case sku number is not available
        if sku_num is None:
            sku_num = 2700
        return int(sku_num) >= 5000

    def _verify_reboot_cause(self, filename):
        '''
        Open and read the reboot cause file in
        /var/run/hwmanagement/system (which is defined as REBOOT_CAUSE_ROOT)
        If a reboot cause file doesn't exists, returns '0'.
        '''
        return bool(utils.read_int_from_file(os.path.join(REBOOT_CAUSE_ROOT, filename), log_func=None))

    def initialize_reboot_cause(self):
        self.reboot_major_cause_dict = {
            'reset_main_pwr_fail'       :   self.REBOOT_CAUSE_POWER_LOSS,
            'reset_aux_pwr_or_ref'      :   self.REBOOT_CAUSE_POWER_LOSS,
            'reset_aux_pwr_or_fu'       :   self.REBOOT_CAUSE_POWER_LOSS,
            'reset_comex_pwr_fail'      :   self.REBOOT_CAUSE_POWER_LOSS,
            'reset_asic_thermal'        :   self.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC,
            'reset_comex_thermal'       :   self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU,
            'reset_hotswap_or_wd'       :   self.REBOOT_CAUSE_WATCHDOG,
            'reset_comex_wd'            :   self.REBOOT_CAUSE_WATCHDOG,
            'reset_swb_wd'              :   self.REBOOT_CAUSE_WATCHDOG,
            'reset_sff_wd'              :   self.REBOOT_CAUSE_WATCHDOG,
            'reset_hotswap_or_halt'     :   self.REBOOT_CAUSE_HARDWARE_OTHER,
            'reset_voltmon_upgrade_fail':   self.REBOOT_CAUSE_HARDWARE_OTHER,
            'reset_reload_bios'         :   self.REBOOT_CAUSE_HARDWARE_BIOS,
            'reset_fw_reset'            :   self.REBOOT_CAUSE_HARDWARE_RESET_FROM_ASIC,
            'reset_from_asic'           :   self.REBOOT_CAUSE_HARDWARE_RESET_FROM_ASIC,
            'reset_long_pb'             :   self.REBOOT_CAUSE_HARDWARE_BUTTON,
            'reset_short_pb'            :   self.REBOOT_CAUSE_HARDWARE_BUTTON
        }
        self.reboot_minor_cause_dict = {}
        self.reboot_by_software = 'reset_sw_reset'
        self.reboot_cause_initialized = True

    def _parse_warmfast_reboot_from_proc_cmdline(self):
        if os.path.isfile(REBOOT_TYPE_KEXEC_FILE):
            with open(REBOOT_TYPE_KEXEC_FILE) as cause_file:
                cause_file_kexec = cause_file.readline()
            m = re.search(REBOOT_TYPE_KEXEC_PATTERN_WARM, cause_file_kexec)
            if m and m.group(1):
                return 'warm-reboot'
            m = re.search(REBOOT_TYPE_KEXEC_PATTERN_FAST, cause_file_kexec)
            if m and m.group(1):
                return 'fast-reboot'
        return None

    def _wait_reboot_cause_ready(self):
        max_wait_time = REBOOT_CAUSE_MAX_WAIT_TIME
        while max_wait_time > 0:
            if utils.read_int_from_file(REBOOT_CAUSE_READY_FILE, log_func=None) == 1:
                return True
            time.sleep(REBOOT_CAUSE_CHECK_INTERVAL)
            max_wait_time -= REBOOT_CAUSE_CHECK_INTERVAL

        return False

    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot

        Returns:
            A tuple (string, string) where the first element is a string
            containing the cause of the previous reboot. This string must be
            one of the predefined strings in this class. If the first string
            is "REBOOT_CAUSE_HARDWARE_OTHER", the second string can be used
            to pass a description of the reboot cause.
        """
        #read reboot causes files in the following order

        # To avoid the leftover hardware reboot cause confusing the reboot cause determine service
        # Skip the hardware reboot cause check if warm/fast reboot cause found from cmdline
        if utils.is_host():
            reboot_cause = self._parse_warmfast_reboot_from_proc_cmdline()
            if reboot_cause:
                return self.REBOOT_CAUSE_NON_HARDWARE, ''

        if not self._wait_reboot_cause_ready():
            logger.log_error("Hardware reboot cause is not ready")
            return self.REBOOT_CAUSE_NON_HARDWARE, ''

        if not self.reboot_cause_initialized:
            self.initialize_reboot_cause()

        for reset_file, reset_cause in self.reboot_major_cause_dict.items():
            if self._verify_reboot_cause(reset_file):
                return reset_cause, ''

        for reset_file, reset_cause in self.reboot_minor_cause_dict.items():
            if self._verify_reboot_cause(reset_file):
                return self.REBOOT_CAUSE_HARDWARE_OTHER, reset_cause

        if self._verify_reboot_cause(self.reboot_by_software):
            logger.log_info("Hardware reboot cause: the system was rebooted due to software requesting")
        else:
            logger.log_info("Hardware reboot cause: no hardware reboot cause found")

        return self.REBOOT_CAUSE_NON_HARDWARE, ''

    def get_thermal_manager(self):
        from .thermal_manager import ThermalManager
        return ThermalManager

    def get_position_in_parent(self):
        """
		Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
		Returns:
		    integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
		"""
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False


class ModularChassis(Chassis):
    def __init__(self):
        super(ModularChassis, self).__init__()
        self.module_initialized_count = 0

    def is_modular_chassis(self):
        """
        Retrieves whether the sonic instance is part of modular chassis

        Returns:
            A bool value, should return False by default or for fixed-platforms.
            Should return True for supervisor-cards, line-cards etc running as part
            of modular-chassis.
        """
        return True

    ##############################################
    # Module methods
    ##############################################
    def initialize_single_module(self, index):
        count = self.get_num_modules()
        if index < count:
            if not self._module_list:
                self._module_list = [None] * count

            if not self._module_list[index]:
                from .module import Module
                self._module_list[index] = Module(index + 1)
                self.module_initialized_count += 1

    def initialize_modules(self):
        if not self._module_list:
            from .module import Module
            count = self.get_num_modules()
            for index in range(1, count + 1):
                self._module_list.append(Module(index))
            self.module_initialized_count = count
        elif self.module_initialized_count != len(self._module_list):
            from .module import Module
            for index in range(len(self._module_list)):
                if self._module_list[index] is None:
                    self._module_list[index] = Module(index + 1)
            self.module_initialized_count = len(self._module_list)

    def get_num_modules(self):
        """
        Retrieves the number of modules available on this chassis

        Returns:
            An integer, the number of modules available on this chassis
        """
        return DeviceDataManager.get_linecard_count()

    def get_all_modules(self):
        """
        Retrieves all modules available on this chassis

        Returns:
            A list of objects derived from ModuleBase representing all
            modules available on this chassis
        """
        self.initialize_modules()
        return self._module_list

    def get_module(self, index):
        """
        Retrieves module represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the module to
            retrieve

        Returns:
            An object dervied from ModuleBase representing the specified
            module
        """
        self.initialize_single_module(index)
        return super(ModularChassis, self).get_module(index)

    @utils.default_return(-1)
    def get_module_index(self, module_name):
        """
        Retrieves module index from the module name

        Args:
            module_name: A string, prefixed by SUPERVISOR, LINE-CARD or FABRIC-CARD
            Ex. SUPERVISOR0, LINE-CARD1, FABRIC-CARD5

        Returns:
            An integer, the index of the ModuleBase object in the module_list
        """
        return int(module_name[len('LINE-CARD')-1:])

    ##############################################
    # SFP methods
    ##############################################

    def get_num_sfps(self):
        """
        Retrieves the number of sfps available on this chassis

        Returns:
            An integer, the number of sfps available on this chassis
        """
        return reduce(lambda x, y: x + y, (x.get_num_sfps() for x in self.get_all_modules()))

    def get_all_sfps(self):
        """
        Retrieves all sfps available on this chassis

        Returns:
            A list of objects derived from SfpBase representing all sfps
            available on this chassis
        """
        return reduce(lambda x, y: x + y, (x.get_all_sfps() for x in self.get_all_modules()))

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>

        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
                   The index should be the sequence of a physical port in a chassis,
                   starting from 1.
                   For example, 1 for Ethernet0, 2 for Ethernet4 and so on.

        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp_index = index % DeviceDataManager.get_linecard_max_port_count() - 1
        slot_id = int((index - sfp_index - 1) / 16) + 1
        module = self.get_module(slot_id - 1)
        if not module:
            return None

        return module.get_sfp(sfp_index - 1)

class SmartSwitchChassis(Chassis):
    def __init__(self):
        super(SmartSwitchChassis, self).__init__()
        self.module_initialized_count = 0
        self.module_name_index_map = {}
        self.initialize_modules()

    def is_modular_chassis(self):
        """
        Retrieves whether the sonic instance is part of modular chassis

        Returns:
            A bool value, should return False by default or for fixed-platforms.
            Should return True for supervisor-cards, line-cards etc running as part
            of modular-chassis.
            For SmartSwitch platforms this should return True even if they are
            fixed-platforms, as they are treated like a modular chassis as the
            DPU cards are treated like line-cards of a modular-chassis.
        """
        return False

    ##############################################
    # Module methods
    ##############################################
    def initialize_single_module(self, index):
        count = self.get_num_modules()
        if index < 0:
            raise RuntimeError(f"Invalid index = {index} for module initialization with total module count = {count}")
        if index >= count:
            return
        if not self._module_list:
            self._module_list = [None] * count
        if not self._module_list[index]:
            from .module import DpuModule
            module = DpuModule(index)
            self._module_list[index] = module
            self.module_name_index_map[module.get_name()] = index
            self.module_initialized_count += 1

    def initialize_modules(self):
        count = self.get_num_modules()
        for index in range(count):
            self.initialize_single_module(index=index)

    def get_num_modules(self):
        """
        Retrieves the number of modules available on this chassis
        On a SmarSwitch chassis this includes the number of DPUs.

        Returns:
            An integer, the number of modules available on this chassis
        """
        return DeviceDataManager.get_dpu_count()

    def get_all_modules(self):
        """
        Retrieves all modules available on this chassis. On a SmarSwitch
        chassis this includes the number of DPUs.

        Returns:
            A list of objects derived from ModuleBase representing all
            modules available on this chassis
        """
        self.initialize_modules()
        return self._module_list

    def get_module(self, index):
        """
        Retrieves module represented by (0-based) index <index>
        On a SmartSwitch index:0 will fetch switch, index:1 will fetch
        DPU0 and so on

        Args:
            index: An integer, the index (0-based) of the module to
            retrieve

        Returns:
            An object dervied from ModuleBase representing the specified
            module
        """
        self.initialize_single_module(index)
        return super(SmartSwitchChassis, self).get_module(index)

    def get_module_index(self, module_name):
        """
        Retrieves module index from the module name

        Args:
            module_name: A string, prefixed by SUPERVISOR, LINE-CARD or FABRIC-CARD
            Ex. SUPERVISOR0, LINE-CARD1, FABRIC-CARD5
            SmartSwitch Example: SWITCH, DPU1, DPU2 ... DPUX

        Returns:
            An integer, the index of the ModuleBase object in the module_list
        """
        return self.module_name_index_map[module_name.upper()]

    ##############################################
    # SmartSwitch methods
    ##############################################

    def get_dpu_id(self, name):
        """
        Retrieves the DPU ID for the given dpu-module name.
        Returns None for non-smartswitch chassis.
        Returns:
            An integer, indicating the DPU ID Ex: name:DPU0 return value 1,
            name:DPU1 return value 2, name:DPUX return value X+1
        """
        module = self.get_module(self.get_module_index(name))
        return module.get_dpu_id()

    def is_smartswitch(self):
        """
        Retrieves whether the sonic instance is part of smartswitch
        Returns:
            Returns:True for SmartSwitch and False for other platforms
        """
        return True

    def init_midplane_switch(self):
        """
        Initializes the midplane functionality of the modular chassis. For
        example, any validation of midplane, populating any lookup tables etc
        can be done here. The expectation is that the required kernel modules,
        ip-address assignment etc are done before the pmon, database dockers
        are up.

        Returns:
            A bool value, should return True if the midplane initialized
            successfully.
        """
        return True

    def get_module_dpu_data_port(self, index):
        """
        Retrieves the DPU data port NPU-DPU association represented for
        the DPU index. Platforms that need to overwrite the platform.json
        file will use this API. This is valid only on the Switch and not on DPUs
        Args:
        index: An integer, the index of the module to retrieve
        Returns:
            A string giving the NPU-DPU port association:
            Ex: For index: 1 will return the dup0 port association which is
            "Ethernet-BP0: Ethernet0" where the string left of ":" (Ethernet-BP0)
            is the NPU port and the string right of ":" (Ethernet0) is the DPU port
        """
        platform_dpus_data = DeviceDataManager.get_platform_dpus_data()
        module = self._module_list[index]
        module_name = module.get_name()
        return platform_dpus_data[module_name.lower()]["interface"]
