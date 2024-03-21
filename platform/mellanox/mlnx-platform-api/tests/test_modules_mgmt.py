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
import queue
import sys
import threading
import time
import types
import unittest

from mock import MagicMock, patch, mock_open, Mock
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform.device_data import DeviceDataManager
from sonic_py_common import device_info
from sonic_platform import modules_mgmt
from sonic_platform.modules_mgmt import ModulesMgmtTask
from sonic_platform_base.sonic_xcvr.api.public.cmis import CmisApi
from sonic_platform_base.sonic_xcvr.xcvr_eeprom import XcvrEeprom
from sonic_platform_base.sonic_xcvr.codes.public.cmis import CmisCodes
from sonic_platform_base.sonic_xcvr.mem_maps.public.cmis import CmisMemMap
from sonic_platform_base.sonic_xcvr.fields import consts

DEFAULT_NUM_OF_PORTS_1 = 1
DEFAULT_NUM_OF_PORTS_3 = 3
DEFAULT_NUM_OF_PORTS_32 = 32
POLLER_EXECUTED = False

def _mock_sysfs_default_file_content():
    return {
        modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format("0"): "1",
        modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format("1"): "1",
        modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format("2"): "1",
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format("0"): "1",
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format("1"): "1",
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format("2"): "1",
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format("0"): "1",
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format("1"): "1",
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format("2"): "1",
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT.format("0"): "48",
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT.format("1"): "48",
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT.format("2"): "48",
        modules_mgmt.SYSFS_INDEPENDENT_FD_FREQ_SUPPORT.format("0"): "0",
        modules_mgmt.SYSFS_INDEPENDENT_FD_FREQ_SUPPORT.format("1"): "0",
        modules_mgmt.SYSFS_INDEPENDENT_FD_FREQ_SUPPORT.format("2"): "0",
        modules_mgmt.SYSFS_INDEPENDENT_FD_HW_RESET: "",
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT: "48",
        modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("0"): "1",
        modules_mgmt.SYSFS_INDEPENDENT_FD_FW_CONTROL.format("0"): "1",
        modules_mgmt.SYSFS_INDEPENDENT_FD_FW_CONTROL.format("1"): "1",
        modules_mgmt.SYSFS_INDEPENDENT_FD_FW_CONTROL.format("2"): "1",
        modules_mgmt.SYSFS_LEGACY_FD_PRESENCE: "1",
        modules_mgmt.PROC_CMDLINE: ""
    }


mock_file_content = _mock_sysfs_default_file_content()


class MockPoller:

    def __init__(self, modules_mgmt_task_stopping_event, modules_mgmt_thrd=None, num_of_ports=3, port_plug_out=False
                 , feature_enabled=True, warm_reboot=False, port_plug_in=False, sleep_timeout=False):
        self.fds_dict = {}
        self.poller_iteration_count = 0
        self.modules_mgmt_task_stopping_event = modules_mgmt_task_stopping_event
        self.modules_mgmt_thrd = modules_mgmt_thrd
        self.num_of_ports = num_of_ports
        self.port_plug_out = port_plug_out
        self.port_plug_in = port_plug_in
        self.feature_enabled = feature_enabled
        self.warm_reboot = warm_reboot
        self.port_plug_out_changed = False
        self.port_plug_in_changed = False
        self.sleep_timeout = sleep_timeout

    def register(self, fd, attrs):
        self.fds_dict[fd.fileno()] = { fd : attrs }
        assert fd.fileno() in self.fds_dict

    def unregister(self, fd):
        if fd.fileno() in self.fds_dict.keys():
            del self.fds_dict[fd.fileno()]
        assert fd.fileno() not in self.fds_dict.keys()

    def poll(self, timeout=1000):
        global POLLER_EXECUTED
        assert len(self.modules_mgmt_thrd.sfp_port_dict_initial) == self.num_of_ports
        assert self.modules_mgmt_thrd.is_supported_indep_mods_system == self.feature_enabled
        # counting the number of poller iterations to know when to do the checks after plug out (and plug in)
        # have to check at least on iteration 7 to let ports reach final state
        self.poller_iteration_count += 1
        if self.num_of_ports > 0:
            if not self.port_plug_out_changed:
                if self.port_plug_out:
                    # return first fd registered with some made up event number 870
                    fd_no_to_return = list(self.fds_dict.keys())[0]
                    fd = list(self.fds_dict[fd_no_to_return].keys())[0]
                    fd.set_file_int_content(0)
                    event_to_return = 870
                    self.port_plug_out_changed = True
                    return [(fd_no_to_return, event_to_return)]
            if not self.port_plug_in_changed:
                if self.port_plug_in:
                    # return first fd registered with some made up event number 871
                    fd_no_to_return = list(self.fds_dict.keys())[0]
                    fd = list(self.fds_dict[fd_no_to_return].keys())[0]
                    fd.set_file_int_content(1)
                    event_to_return = 871
                    self.port_plug_in_changed = True
                    return [(fd_no_to_return, event_to_return)]
            if 7 == self.poller_iteration_count:
                # when feature is enabled, need to check for each port both power_good and hw_present sysfs for
                # cmis non-flat memory cables
                num_of_sysfs_to_check = self.num_of_ports if (not self.port_plug_out or not self.feature_enabled
                                                              or self.warm_reboot) else self.num_of_ports * 2
                for i in range(num_of_sysfs_to_check):
                    # when feature is enabled, power_good sysfs is also registered for cmis non-flat memory cables
                    # so each SW controlled port has 2 fds registered
                    port_to_test = i if not self.feature_enabled else int(i / 2)
                    assert self.modules_mgmt_thrd.sfp_port_dict_initial[port_to_test].port_num == port_to_test
                    assert self.modules_mgmt_thrd.sfp_port_dict_initial[
                               port_to_test].initial_state == modules_mgmt.STATE_HW_NOT_PRESENT
                    if self.feature_enabled:
                        module_obj = self.modules_mgmt_thrd.fds_mapping_to_obj[list(self.fds_dict.keys())[i]][
                            'module_obj']
                        assert module_obj.port_num == port_to_test
                        if not self.warm_reboot:
                            # in tests other than warm reboot it creates only SW control ports
                            if not self.port_plug_out:
                                assert module_obj.final_state == modules_mgmt.STATE_SW_CONTROL
                            else:
                                assert module_obj.final_state == modules_mgmt.STATE_HW_NOT_PRESENT
                        else:
                            if not self.port_plug_out:
                                assert module_obj.final_state == modules_mgmt.STATE_HW_PRESENT
                            # in warm reboot test with plug out plug in test creates only FW control ports
                            elif self.port_plug_out and self.port_plug_in:
                                assert module_obj.final_state == modules_mgmt.STATE_FW_CONTROL
                            else:
                                assert module_obj.final_state == modules_mgmt.STATE_HW_NOT_PRESENT
                        POLLER_EXECUTED = True
                self.modules_mgmt_task_stopping_event.set()
        if self.sleep_timeout:
            time.sleep(timeout/1000)
        return []


class MockOpen:

    def __init__(self, name='', file_no=None, indep_mode_supported=True):
        self.name = name
        self.file_no = file_no
        self.indep_mode_supported = indep_mode_supported
        self.retint = None
        self.curr = 0

    def read(self):
        if self.fileno() in [SAI_PROFILE_FD_FILENO]:
            pass
        else:
            # if return value was changed, i.e. sysfs content changed from 1 to 0 to simulate plug out
            if self.retint is not None:
                return str(self.retint)
            # return default values (can be changed per test)
            else:
                return mock_file_content[self.name]

    def readline(self):
        # if trying to read sai profile file, according to fd fileno
        if self.fileno() in [SAI_PROFILE_FD_FILENO]:
            if self.indep_mode_supported:
                return "SAI_INDEPENDENT_MODULE_MODE=1"
            else:
                return ""
        else:
            return mock_file_content[self.name]

    def fileno(self):
        return self.file_no

    def seek(self, seek_val):
        self.curr = seek_val

    def close(self):
        pass

    def write(self, write_val):
        self.set_file_int_content(write_val)

    def set_file_int_content(self, retint):
        self.retint = str(retint)
        mock_file_content[self.name] = str(retint)

    def __enter__(self):
        return self

    def __exit__(self, filename, *args, **kwargs):
        pass

class MockPollerStopEvent:

    def __init__(self, modules_mgmt_task_stopping_event, modules_mgmt_thrd=None, num_of_ports=DEFAULT_NUM_OF_PORTS_3
                 , feature_enabled=True, ports_connected=True, fw_controlled_ports=False, sleep_timeout=False):
        self.fds_dict = {}
        self.modules_mgmt_task_stopping_event = modules_mgmt_task_stopping_event
        self.modules_mgmt_thrd = modules_mgmt_thrd
        self.num_of_ports = num_of_ports
        self.feature_enabled = feature_enabled
        self.ports_connected = ports_connected
        self.sleep_timeout = sleep_timeout
        self.fw_controlled_ports = fw_controlled_ports

    def register(self, fd, attrs):
        self.fds_dict[fd.fileno()] = 1 & attrs
        assert fd.fileno() in self.fds_dict

    def poll(self, timeout=0):
        assert len(self.modules_mgmt_thrd.sfp_port_dict_initial) == self.num_of_ports
        assert self.modules_mgmt_thrd.is_supported_indep_mods_system == self.feature_enabled
        global POLLER_EXECUTED
        if self.num_of_ports > 0:
            # when feature is enabled, need to check for each port both power_good and hw_present sysfs for
            # cmis non-flat memory cables
            ports_to_test = self.num_of_ports if (not self.feature_enabled or not self.ports_connected
                                                  or self.fw_controlled_ports) else self.num_of_ports * 2
            for i in range(ports_to_test):
                # when feature is enabled, power_good sysfs is also registered for cmis non-flat memory cables
                port_to_test = i if (not self.feature_enabled or not self.ports_connected
                                     or self.fw_controlled_ports) else int(i / 2)
                assert self.modules_mgmt_thrd.sfp_port_dict_initial[port_to_test].port_num == port_to_test
                assert self.modules_mgmt_thrd.sfp_port_dict_initial[port_to_test].initial_state == modules_mgmt.STATE_HW_NOT_PRESENT
                module_obj = self.modules_mgmt_thrd.fds_mapping_to_obj[list(self.fds_dict.keys())[i]]['module_obj']
                assert module_obj.port_num == port_to_test
                if self.ports_connected:
                    if self.feature_enabled:
                        if self.fw_controlled_ports:
                            assert module_obj.final_state == modules_mgmt.STATE_FW_CONTROL
                        else:
                            assert module_obj.final_state == modules_mgmt.STATE_SW_CONTROL
                    else:
                        assert module_obj.final_state == modules_mgmt.STATE_HW_PRESENT
                else:
                    assert module_obj.final_state == modules_mgmt.STATE_HW_NOT_PRESENT
                POLLER_EXECUTED = True
        else:
            POLLER_EXECUTED = True
        self.modules_mgmt_task_stopping_event.set()
        if self.sleep_timeout:
            time.sleep(timeout/1000)
        return []


def _mock_is_file_indep_mode_disabled_content():
    return {
        modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE: True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD: True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON: True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_HW_RESET: True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT: True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_FW_CONTROL: True,
        modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("0"): True,
        modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("1"): True,
        modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("2"): True,
        '//usr/share/sonic/platform/ACS-MSN4700/sai.profile' : True
    }

mock_is_file_indep_mode_disabled_content = _mock_is_file_indep_mode_disabled_content()

def mock_is_file_indep_mode_disabled(file_path, **kwargs):
    return mock_is_file_indep_mode_disabled_content[file_path]

def _mock_is_file_indep_mode_enabled_content():
    return {
        modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE: True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD: True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON: True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_HW_RESET: True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT: True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_FW_CONTROL: True,
        modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("0"): True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format("0"): True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format("1"): True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format("2"): True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format("0"): True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format("1"): True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format("2"): True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format("0"): True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format("1"): True,
        modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format("2"): True,
        '//usr/share/sonic/platform/ACS-MSN4700/sai.profile'    :   True
    }

mock_is_file_indep_mode_enabled_content = _mock_is_file_indep_mode_enabled_content()


def mock_is_file_indep_mode_enabled(file_path, **kwargs):
    return mock_is_file_indep_mode_enabled_content[file_path]


def mock_read_int_from_file(filename, *args):
    return_dict = {
        modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("0") : 1,
        modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("1") : 1,
        modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("2") : 1
    }

    return return_dict[filename]


class MockXcvrEeprom():
    def __init__(self, is_flat_memory, mem_map):
        self.is_flat_memory = is_flat_memory
        self.mem_map = mem_map

    def is_cmis_api(self):
        return self.is_cmis_api

    def is_flat_memory(self):
        return self.is_flat_memory

    def read(self, field):
        if consts.FLAT_MEM_FIELD == field:
            return 0 if self.is_flat_memory else 1
        else:
            return 0


class MockXcvrapi:
    def __init__(self, is_cmis_api=True, is_flat_memory_bool=False):
        self.is_cmis_api = is_cmis_api
        self.is_flat_memory_bool = is_flat_memory_bool
        self.xcvr_eeprom = MagicMock(autospec=XcvrEeprom, return_value=MockXcvrEeprom(is_flat_memory_bool, CmisMemMap(CmisCodes)))

    def is_flat_memory(self):
        return self.is_flat_memory_bool

    def xcvr_eeprom(self):
        return self.xcvr_eeprom


class MockSFPxcvrapi:
    def __init__(self, xcvr_api_is_cmis_api=True, xcvr_eeprom_is_flat_memory=False):
        self.xcvr_api = Mock(spec=CmisApi(MockXcvrEeprom(False, CmisMemMap(CmisCodes))), return_value=MockXcvrapi(xcvr_api_is_cmis_api, xcvr_eeprom_is_flat_memory))
        self.xcvr_api_is_cmis_api = xcvr_api_is_cmis_api
        self.xcvr_eeprom_is_flat_memory = xcvr_eeprom_is_flat_memory
        self.xcvr_api.is_flat_memory = types.MethodType(self.is_flat_memory, self)

    def get_xcvr_api(self):
        return self.xcvr_api

    def is_flat_memory(self, ref):
        return self.xcvr_eeprom_is_flat_memory


def check_power_cap(port, module_sm_obj):
    pass

SAI_PROFILE_FD_FILENO = 99


class TestModulesMgmt(unittest.TestCase):
    """Test class to test modules_mgmt.py. The test cases covers:
        1. cables detection for 1 to 3 ports - feature disabled / enabled / poller
        2. cable disconnection - plug out
        3. cable reconnection - plug in
        4. warm reboot normal flow with FW ports
        5. warm reboot flow with FW ports plugged out
        6. warm reboot flow with FW ports plugged out and then plugged in (stays FW controlled, no SFP mock change)
        7. test 32 FW controlled (non cmis flat mem) cables powered off
        8. test 32 SW controlled (cmis active non flat mem) cables powered off
    """

    def _mock_sysfs_file_content(self):
        return {
            modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE : "1",
            modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD : "1",
            modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON : "0",
            modules_mgmt.SYSFS_INDEPENDENT_FD_HW_RESET : "",
            modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT : "48",
            modules_mgmt.SYSFS_INDEPENDENT_FD_FW_CONTROL : "1",
            modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("0") : "1",
            modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("1") : "1",
            modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("2") : "1",
            modules_mgmt.SYSFS_INDEPENDENT_FD_FREQ_SUPPORT.format("0"): "0"
        }

    def mock_open_builtin(self, file_name, feature_enabled=True):
        return_dict = {
            (modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("0"), 'r') : MockOpen(modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("0"), 100),
            (modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("1"), 'r') : MockOpen(modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("1"), 101),
            (modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("2"), 'r') : MockOpen(modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("2"), 102),
            '//usr/share/sonic/platform/ACS-MSN4700/sai.profile' : MockOpen('//usr/share/sonic/platform/ACS-MSN4700/sai.profile'
                                                                            , SAI_PROFILE_FD_FILENO, feature_enabled),
            modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("0") : MockOpen(modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("0"), 100),
            modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("1") : MockOpen(modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("1"), 101),
            modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("2") : MockOpen(modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("2"), 102),
            modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format("0"): MockOpen(modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format("0"), 0),
            modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format("1"): MockOpen(modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format("1"), 1),
            modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format("2"): MockOpen(modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format("2"), 2),
            modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format("0"): MockOpen(modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format("0"), 200),
            modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format("1"): MockOpen(modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format("1"), 201),
            modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format("2"): MockOpen(modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format("2"), 202),
            modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format("0"): MockOpen(modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format("0"), 300),
            modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format("1"): MockOpen(modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format("1"), 301),
            modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format("2"): MockOpen(modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format("2"), 302),
            modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT.format("0"): MockOpen(modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT.format("0"), 500),
            modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT.format("1"): MockOpen(modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT.format("1"), 501),
            modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT.format("2"): MockOpen(modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT.format("2"), 502),
            modules_mgmt.SYSFS_INDEPENDENT_FD_FREQ_SUPPORT.format("0"): MockOpen(modules_mgmt.SYSFS_INDEPENDENT_FD_FREQ_SUPPORT.format("0"), 602),
            modules_mgmt.SYSFS_INDEPENDENT_FD_FREQ_SUPPORT.format("1"): MockOpen(modules_mgmt.SYSFS_INDEPENDENT_FD_FREQ_SUPPORT.format("1"), 602),
            modules_mgmt.SYSFS_INDEPENDENT_FD_FREQ_SUPPORT.format("2"): MockOpen(modules_mgmt.SYSFS_INDEPENDENT_FD_FREQ_SUPPORT.format("2"), 602),
            modules_mgmt.PROC_CMDLINE: MockOpen(modules_mgmt.PROC_CMDLINE, self.fd_number_by_fd_name_dict[modules_mgmt.PROC_CMDLINE])
        }
        return return_dict[file_name]

    # side effects are used in mock when want to create different mocks per variable, i.e. here it's filename
    # see below mock_open_new_side_effect_poller_test where returning a new MockOpen passing it the filename
    def mock_open_new_side_effect_feature_disabled(self, filename, *args, **kwargs):
        mock_context = MagicMock()
        mock_context.__enter__.return_value = self.mock_open_builtin(filename, False)
        mock_context.__exit__.return_value = False
        return mock_context

    def mock_open_new_side_effect_feature_enabled(self, filename, *args, **kwargs):
        mock_context = MagicMock()
        mock_context.__enter__.return_value = self.mock_open_builtin(filename)
        mock_context.__exit__.return_value = False
        return mock_context

    def mock_open_new_side_effect_poller_test(self, filename, *args, **kwargs):
        if filename in ['//usr/share/sonic/platform/ACS-MSN4700/sai.profile']:
            mock_context = MagicMock()
            mock_context.__enter__.return_value = MockOpen(filename, SAI_PROFILE_FD_FILENO)
            mock_context.__exit__.return_value = False
            return mock_context
        else:
            mock_context = MagicMock()
            mock_open_new = MockOpen(filename, self.fd_number_by_fd_name_dict[filename])
            mock_context.return_value = mock_open_new
            mock_context.__enter__.return_value = mock_open_new
            mock_context.__exit__.return_value = False
            if 'hw_present' in filename or 'power_on' in filename or 'freq' in filename or 'control' in filename:
                return mock_context
            else:
                return mock_context.return_value

    def mock_open_new_side_effect_warm_reboot(self, filename, *args, **kwargs):
        if filename in ['//usr/share/sonic/platform/ACS-MSN4700/sai.profile']:
            mock_context = MagicMock()
            mock_context.__enter__.return_value = MockOpen(filename, SAI_PROFILE_FD_FILENO)
            mock_context.__exit__.return_value = False
            return mock_context
        else:
            mock_open_new = MockOpen(filename, self.fd_number_by_fd_name_dict[filename])
            return mock_open_new

    def setUp(cls):
        cls.modules_mgmt_task_stopping_event = threading.Event()
        cls.modules_changes_queue = queue.Queue()
        global POLLER_EXECUTED
        POLLER_EXECUTED = False
        # start modules_mgmt thread and the test in poller part
        cls.modules_mgmt_thrd = ModulesMgmtTask(main_thread_stop_event=cls.modules_mgmt_task_stopping_event,
                                            q=cls.modules_changes_queue)
        cls.modules_mgmt_thrd.check_power_cap = check_power_cap
        assert cls.modules_mgmt_thrd.sfp_port_dict_initial == {}

    @classmethod
    def setup_class(cls):
        os.environ["MLNX_PLATFORM_API_UNIT_TESTING"] = "1"
        cls.fd_number_by_fd_name_dict = {
                modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("0") : 100,
                modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("1") : 101,
                modules_mgmt.SYSFS_LEGACY_FD_PRESENCE.format("2") : 102,
                '//usr/share/sonic/platform/ACS-MSN4700/sai.profile' : SAI_PROFILE_FD_FILENO,
                modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format("0") : 0,
                modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format("1") : 1,
                modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format("2") : 2,
                modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format("0") : 200,
                modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format("1") : 201,
                modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format("2") : 202,
                modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format("0") : 300,
                modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format("1") : 301,
                modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format("2") : 302,
                modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT.format("0") : 500,
                modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT.format("1") : 501,
                modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_LIMIT.format("2") : 502,
                modules_mgmt.SYSFS_INDEPENDENT_FD_FREQ_SUPPORT.format("0") : 600,
                modules_mgmt.SYSFS_INDEPENDENT_FD_FREQ_SUPPORT.format("1") : 601,
                modules_mgmt.SYSFS_INDEPENDENT_FD_FREQ_SUPPORT.format("2") : 602,
                modules_mgmt.SYSFS_INDEPENDENT_FD_FW_CONTROL.format("0") : 700,
                modules_mgmt.SYSFS_INDEPENDENT_FD_FW_CONTROL.format("1") : 701,
                modules_mgmt.SYSFS_INDEPENDENT_FD_FW_CONTROL.format("2") : 702,
                modules_mgmt.PROC_CMDLINE : 800
        }
        # mock the directory holding relevant sai.profile
        device_info.get_paths_to_platform_and_hwsku_dirs = mock.MagicMock(return_value=('', '/usr/share/sonic/platform/ACS-MSN4700'))


    @patch('sonic_platform.device_data.DeviceDataManager.get_sfp_count', MagicMock(return_value=DEFAULT_NUM_OF_PORTS_3))
    @patch('os.path.isfile', MagicMock(side_effect=mock_is_file_indep_mode_disabled))
    @patch('sonic_platform.utils.read_int_from_file', MagicMock(side_effect=mock_read_int_from_file))
    @patch('builtins.open', spec=open)
    def test_mdf_all_ports_feature_disabled(self, mock_open):
        mock_open.side_effect = self.mock_open_new_side_effect_feature_disabled
        num_of_tested_ports = DeviceDataManager.get_sfp_count()
        assert num_of_tested_ports == DEFAULT_NUM_OF_PORTS_3

        # start modules_mgmt thread and the test in poller part
        with patch('select.poll', MagicMock(return_value=MockPollerStopEvent(self.modules_mgmt_task_stopping_event
                                                                    , self.modules_mgmt_thrd, feature_enabled=False))):
            self.modules_mgmt_thrd.run()

    @patch('sonic_platform.device_data.DeviceDataManager.get_sfp_count', MagicMock(return_value=DEFAULT_NUM_OF_PORTS_3))
    @patch('os.path.isfile', MagicMock(side_effect=mock_is_file_indep_mode_enabled))
    @patch('builtins.open', spec=open)
    @patch('sonic_platform.sfp.SFP', MagicMock(return_value=MockSFPxcvrapi()))
    def test_mdf_all_ports_feature_enabled(self, mock_open):
        mock_open.side_effect = self.mock_open_new_side_effect_feature_enabled
        num_of_tested_ports = DeviceDataManager.get_sfp_count()
        assert num_of_tested_ports == DEFAULT_NUM_OF_PORTS_3

        # start modules_mgmt thread and the test in poller part
        with patch('select.poll', MagicMock(return_value=MockPollerStopEvent(self.modules_mgmt_task_stopping_event
                , self.modules_mgmt_thrd))):
            self.modules_mgmt_thrd.run()

    @patch('os.path.isfile', MagicMock(side_effect=mock_is_file_indep_mode_enabled))
    @patch('builtins.open', spec=open)
    @patch('sonic_platform.sfp.SFP', MagicMock(return_value=MockSFPxcvrapi()))
    def test_modules_mgmt_poller_events_3_ports(self, mock_open):
        mock_open.side_effect = self.mock_open_new_side_effect_poller_test
        DeviceDataManager.get_sfp_count = mock.MagicMock(return_value=DEFAULT_NUM_OF_PORTS_3)
        num_of_tested_ports = DeviceDataManager.get_sfp_count()
        assert num_of_tested_ports == DEFAULT_NUM_OF_PORTS_3

        # start modules_mgmt thread and the test in poller part
        with patch('select.poll', MagicMock(return_value=MockPollerStopEvent(self.modules_mgmt_task_stopping_event
                , self.modules_mgmt_thrd))):
            self.modules_mgmt_thrd.run()

    @patch('os.path.isfile', MagicMock(side_effect=mock_is_file_indep_mode_enabled))
    @patch('builtins.open', spec=open)
    @patch('sonic_platform.sfp.SFP', MagicMock(return_value=MockSFPxcvrapi()))
    def test_modules_mgmt_poller_events_single_port(self, mock_open):
        mock_open.side_effect = self.mock_open_new_side_effect_poller_test
        DeviceDataManager.get_sfp_count = mock.MagicMock(return_value=DEFAULT_NUM_OF_PORTS_1)
        num_of_tested_ports = DeviceDataManager.get_sfp_count()
        assert num_of_tested_ports == DEFAULT_NUM_OF_PORTS_1

        # start modules_mgmt thread and the test in poller part
        with patch('select.poll', MagicMock(return_value=MockPollerStopEvent(self.modules_mgmt_task_stopping_event
                , self.modules_mgmt_thrd, num_of_tested_ports))):
            #with patch('builtins.open', MagicMock(side_effect=self.mock_open_new_side_effect_poller_test)):
            self.modules_mgmt_thrd.run()

    @patch('os.path.isfile', MagicMock(side_effect=mock_is_file_indep_mode_enabled))
    @patch('builtins.open', spec=open)
    @patch('sonic_platform.sfp.SFP', MagicMock(return_value=MockSFPxcvrapi(False, True)))
    def test_modules_mgmt_normal_warm_reboot(self, mock_open):
        mock_open.side_effect = self.mock_open_new_side_effect_warm_reboot
        # mock /proc/cmdline with warm reboot boot type key value
        mock_file_content[modules_mgmt.PROC_CMDLINE] = f'{modules_mgmt.CMDLINE_STR_TO_LOOK_FOR}{modules_mgmt.CMDLINE_VAL_TO_LOOK_FOR}'
        DeviceDataManager.get_sfp_count = mock.MagicMock(return_value=DEFAULT_NUM_OF_PORTS_1)
        num_of_tested_ports = DeviceDataManager.get_sfp_count()
        assert num_of_tested_ports == DEFAULT_NUM_OF_PORTS_1
        # set the port to start with FW controlled before warm reboot takes place
        mock_file_content[modules_mgmt.SYSFS_INDEPENDENT_FD_FW_CONTROL.format("0")] = "0"

        # start modules_mgmt thread and the test in poller part
        with patch('select.poll', MagicMock(return_value=MockPoller(self.modules_mgmt_task_stopping_event
                , self.modules_mgmt_thrd, num_of_tested_ports, warm_reboot=True))):
            self.modules_mgmt_thrd.run()

    @patch('os.path.isfile', MagicMock(side_effect=mock_is_file_indep_mode_enabled))
    @patch('builtins.open', spec=open)
    @patch('sonic_platform.sfp.SFP', MagicMock(return_value=MockSFPxcvrapi(False, True)))
    def test_modules_mgmt_plug_out_fw_cable_after_warm_reboot(self, mock_open):
        mock_open.side_effect = self.mock_open_new_side_effect_warm_reboot
        # mock /proc/cmdline with warm reboot boot type key value
        mock_file_content[modules_mgmt.PROC_CMDLINE] = f'{modules_mgmt.CMDLINE_STR_TO_LOOK_FOR}{modules_mgmt.CMDLINE_VAL_TO_LOOK_FOR}'
        DeviceDataManager.get_sfp_count = mock.MagicMock(return_value=DEFAULT_NUM_OF_PORTS_1)
        num_of_tested_ports = DeviceDataManager.get_sfp_count()
        assert num_of_tested_ports == DEFAULT_NUM_OF_PORTS_1

        # set the port to start with FW controlled before warm reboot takes place
        mock_file_content[modules_mgmt.SYSFS_INDEPENDENT_FD_FW_CONTROL.format("0")] = "0"

        # start modules_mgmt thread and the test in poller part
        with patch('select.poll', MagicMock(return_value=MockPoller(self.modules_mgmt_task_stopping_event
                , self.modules_mgmt_thrd, num_of_tested_ports, port_plug_out=True, warm_reboot=True))):
            self.modules_mgmt_thrd.run()

    @patch('os.path.isfile', MagicMock(side_effect=mock_is_file_indep_mode_enabled))
    @patch('builtins.open', spec=open)
    @patch('sonic_platform.sfp.SFP', MagicMock(return_value=MockSFPxcvrapi(False, True)))
    def test_modules_mgmt_plug_out_plug_in_fw_cable_after_warm_reboot(self, mock_open):
        mock_open.side_effect = self.mock_open_new_side_effect_warm_reboot
        # mock /proc/cmdline with warm reboot boot type key value
        mock_file_content[modules_mgmt.PROC_CMDLINE] = f'{modules_mgmt.CMDLINE_STR_TO_LOOK_FOR}{modules_mgmt.CMDLINE_VAL_TO_LOOK_FOR}'
        DeviceDataManager.get_sfp_count = mock.MagicMock(return_value=DEFAULT_NUM_OF_PORTS_1)
        num_of_tested_ports = DeviceDataManager.get_sfp_count()
        assert num_of_tested_ports == DEFAULT_NUM_OF_PORTS_1

        mock_file_content[modules_mgmt.SYSFS_INDEPENDENT_FD_FW_CONTROL.format("0")] = "0"

        # start modules_mgmt thread and the test in poller part
        with patch('select.poll', MagicMock(return_value=MockPoller(self.modules_mgmt_task_stopping_event
                , self.modules_mgmt_thrd, num_of_tested_ports, port_plug_out=True, warm_reboot=True, port_plug_in=True))):
            self.modules_mgmt_thrd.run()

    @patch('os.path.isfile', MagicMock(side_effect=mock_is_file_indep_mode_enabled))
    @patch('builtins.open', spec=open)
    @patch('sonic_platform.sfp.SFP', MagicMock(return_value=MockSFPxcvrapi(False, True)))
    def test_modules_mgmt_no_ports(self, mock_open):
        mock_open.side_effect = self.mock_open_new_side_effect_poller_test
        DeviceDataManager.get_sfp_count = mock.MagicMock(return_value=0)
        num_of_tested_ports = DeviceDataManager.get_sfp_count()
        assert num_of_tested_ports == 0

        # start modules_mgmt thread and the test in poller part
        with patch('select.poll', MagicMock(return_value=MockPollerStopEvent(self.modules_mgmt_task_stopping_event
                , self.modules_mgmt_thrd, num_of_tested_ports))):
            self.modules_mgmt_thrd.run()

    @patch('os.path.isfile', MagicMock(side_effect=mock_is_file_indep_mode_enabled))
    @patch('builtins.open', spec=open)
    @patch('sonic_platform.sfp.SFP', MagicMock(return_value=MockSFPxcvrapi(False, True)))
    def test_modules_mgmt_ports_disconnected(self, mock_open):
        mock_open.side_effect = self.mock_open_new_side_effect_poller_test
        DeviceDataManager.get_sfp_count = mock.MagicMock(return_value=DEFAULT_NUM_OF_PORTS_3)
        num_of_tested_ports = DeviceDataManager.get_sfp_count()
        assert num_of_tested_ports == DEFAULT_NUM_OF_PORTS_3

        # update hw_present sysfs with value of 0 for each port
        for i in range(num_of_tested_ports):
            modules_sysfs = modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format(f"{i}")
            mock_file_content[modules_sysfs] = "0"

        # start modules_mgmt thread and the test in poller part
        with patch('select.poll', MagicMock(return_value=MockPollerStopEvent(self.modules_mgmt_task_stopping_event
                , self.modules_mgmt_thrd, num_of_tested_ports, ports_connected=False))):
            self.modules_mgmt_thrd.run()

    @patch('os.path.isfile', MagicMock(side_effect=mock_is_file_indep_mode_enabled))
    @patch('builtins.open', spec=open)
    @patch('sonic_platform.sfp.SFP', MagicMock(return_value=MockSFPxcvrapi(False, True)))
    def test_modules_mgmt_bad_flows_port_disconnected(self, mock_open):
        mock_open.side_effect = self.mock_open_new_side_effect_poller_test
        DeviceDataManager.get_sfp_count = mock.MagicMock(return_value=DEFAULT_NUM_OF_PORTS_1)
        num_of_tested_ports = DeviceDataManager.get_sfp_count()
        assert num_of_tested_ports == DEFAULT_NUM_OF_PORTS_1

        # update hw_present sysfs with value of 0 for each port
        for i in range(num_of_tested_ports):
            modules_sysfs = modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format(f"{i}")
            mock_file_content[modules_sysfs] = "0"

        # start modules_mgmt thread and the test in poller part
        with patch('select.poll', MagicMock(return_value=MockPollerStopEvent(self.modules_mgmt_task_stopping_event
                , self.modules_mgmt_thrd, num_of_tested_ports, ports_connected=False))):
            self.modules_mgmt_thrd.run()

    @patch('os.path.isfile', MagicMock(side_effect=mock_is_file_indep_mode_enabled))
    @patch('builtins.open', spec=open)
    @patch('sonic_platform.sfp.SFP', MagicMock(return_value=MockSFPxcvrapi(False, True)))
    def test_modules_mgmt_bad_flows_power_good(self, mock_open):
        mock_open.side_effect = self.mock_open_new_side_effect_poller_test
        DeviceDataManager.get_sfp_count = mock.MagicMock(return_value=DEFAULT_NUM_OF_PORTS_1)
        num_of_tested_ports = DeviceDataManager.get_sfp_count()
        assert num_of_tested_ports == DEFAULT_NUM_OF_PORTS_1

        # update power_good sysfs with value of 0 for each port
        for i in range(num_of_tested_ports):
            modules_sysfs = modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format(f"{i}")
            mock_file_content[modules_sysfs] = "0"

        # start modules_mgmt thread and the test in poller part
        with patch('select.poll', MagicMock(return_value=MockPollerStopEvent(self.modules_mgmt_task_stopping_event
                , self.modules_mgmt_thrd, num_of_tested_ports, ports_connected=False))):
            self.modules_mgmt_thrd.run()
        for i in range(num_of_tested_ports):
            modules_sysfs = modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format(f"{i}")
            mock_file_content[modules_sysfs] = "1"

    @patch('os.path.isfile', MagicMock(side_effect=mock_is_file_indep_mode_enabled))
    @patch('builtins.open', spec=open)
    @patch('sonic_platform.sfp.SFP', MagicMock(return_value=MockSFPxcvrapi(False, True)))
    def test_modules_mgmt_bad_flows_ports_powered_off_fw_controlled(self, mock_open):
        mock_open.side_effect = self.mock_open_new_side_effect_poller_test
        DeviceDataManager.get_sfp_count = mock.MagicMock(return_value=DEFAULT_NUM_OF_PORTS_32)
        num_of_tested_ports = DeviceDataManager.get_sfp_count()
        assert num_of_tested_ports == DEFAULT_NUM_OF_PORTS_32

        # create or update different sysfs and is_file mocking with relevant value for each port
        for i in range(num_of_tested_ports):
            # mock power_on sysfs for all ports
            modules_sysfs = modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format(f"{i}")
            mock_file_content[modules_sysfs] = "0"
            mock_is_file_indep_mode_enabled_content[modules_sysfs] = True
            self.fd_number_by_fd_name_dict[modules_sysfs] = 300 + i
            # mock hw_presence sysfs for all ports
            modules_sysfs = modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format(f'{i}')
            mock_file_content[modules_sysfs] = "1"
            mock_is_file_indep_mode_enabled_content[modules_sysfs] = True
            self.fd_number_by_fd_name_dict[modules_sysfs] = i
            # mock power_good sysfs for all ports
            modules_sysfs = modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format(f'{i}')
            mock_file_content[modules_sysfs] = "1"
            mock_is_file_indep_mode_enabled_content[modules_sysfs] = True
            self.fd_number_by_fd_name_dict[modules_sysfs] = 200 + i
            # mock hw_reset sysfs for all ports
            modules_sysfs = modules_mgmt.SYSFS_INDEPENDENT_FD_HW_RESET.format(f'{i}')
            mock_is_file_indep_mode_enabled_content[modules_sysfs] = True
            self.fd_number_by_fd_name_dict[modules_sysfs] = 400 + i

        # start modules_mgmt thread and the test in poller part
        with patch('select.poll', MagicMock(return_value=MockPollerStopEvent(self.modules_mgmt_task_stopping_event
                , self.modules_mgmt_thrd, num_of_tested_ports, fw_controlled_ports=True))):
            self.modules_mgmt_thrd.run()

        # change power_on sysfs values back to the default ones
        for i in range(num_of_tested_ports):
            modules_sysfs = modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format(f"{i}")
            mock_file_content[modules_sysfs] = "1"

    @patch('os.path.isfile', MagicMock(side_effect=mock_is_file_indep_mode_enabled))
    @patch('builtins.open', spec=open)
    @patch('sonic_platform.sfp.SFP', MagicMock(return_value=MockSFPxcvrapi()))
    def test_modules_mgmt_bad_flows_ports_powered_off_sw_controlled(self, mock_open):
        mock_open.side_effect = self.mock_open_new_side_effect_poller_test
        DeviceDataManager.get_sfp_count = mock.MagicMock(return_value=DEFAULT_NUM_OF_PORTS_32)
        num_of_tested_ports = DeviceDataManager.get_sfp_count()
        assert num_of_tested_ports == DEFAULT_NUM_OF_PORTS_32

        # create or update different sysfs and is_file mocking with relevant value for each port
        for i in range(num_of_tested_ports):
            # mock power_on sysfs for all ports
            modules_sysfs = modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format(f"{i}")
            mock_file_content[modules_sysfs] = "0"
            mock_is_file_indep_mode_enabled_content[modules_sysfs] = True
            self.fd_number_by_fd_name_dict[modules_sysfs] = 300 + i
            # mock hw_presence sysfs for all ports
            modules_sysfs = modules_mgmt.SYSFS_INDEPENDENT_FD_PRESENCE.format(f'{i}')
            mock_file_content[modules_sysfs] = "1"
            mock_is_file_indep_mode_enabled_content[modules_sysfs] = True
            self.fd_number_by_fd_name_dict[modules_sysfs] = i
            # mock power_good sysfs for all ports
            modules_sysfs = modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_GOOD.format(f'{i}')
            mock_file_content[modules_sysfs] = "1"
            mock_is_file_indep_mode_enabled_content[modules_sysfs] = True
            self.fd_number_by_fd_name_dict[modules_sysfs] = 200 + i
            # mock hw_reset sysfs for all ports
            modules_sysfs = modules_mgmt.SYSFS_INDEPENDENT_FD_HW_RESET.format(f'{i}')
            mock_is_file_indep_mode_enabled_content[modules_sysfs] = True
            self.fd_number_by_fd_name_dict[modules_sysfs] = 400 + i
            # mock frequency_support sysfs for all ports
            modules_sysfs = modules_mgmt.SYSFS_INDEPENDENT_FD_FREQ_SUPPORT.format(f'{i}')
            mock_file_content[modules_sysfs] = "0"
            mock_is_file_indep_mode_enabled_content[modules_sysfs] = True
            self.fd_number_by_fd_name_dict[modules_sysfs] = 600 + i

        # start modules_mgmt thread and the test in poller part
        with patch('select.poll', MagicMock(return_value=MockPollerStopEvent(self.modules_mgmt_task_stopping_event
                , self.modules_mgmt_thrd, num_of_tested_ports))):
            self.modules_mgmt_thrd.run()

        # change power_on sysfs values back to the default ones
        for i in range(num_of_tested_ports):
            modules_sysfs = modules_mgmt.SYSFS_INDEPENDENT_FD_POWER_ON.format(f"{i}")
            mock_file_content[modules_sysfs] = "1"

    def tearDown(cls):
        mock_file_content[modules_mgmt.PROC_CMDLINE] = ''
        cls.modules_mgmt_thrd = None
        # a check that modules mgmt thread ran and got into the poller part where the tests here has all checks
        assert POLLER_EXECUTED
