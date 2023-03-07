#
# Copyright (c) 2019-2022 NVIDIA CORPORATION & AFFILIATES.
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
'''
listen to the SDK for the SFP change event and return to chassis.
'''


import sys, errno
import os
import time
import select

from .device_data import DeviceDataManager
try:
    if 'PLATFORM_API_UNIT_TESTING' not in os.environ:
        from python_sdk_api.sx_api import *
    else:
        from mock import MagicMock
        class MockSxFd(object):
            fd = 99
        new_sx_fd_t_p = MagicMock(return_value=MockSxFd())
        new_sx_user_channel_t_p = MagicMock()
except KeyError:
    pass
from sonic_py_common.logger import Logger
from .sfp import SFP

# SFP status from PMAOS register
# 0x1 plug in
# 0x2 plug out
# 0x3 plug in with error
# 0x4 disabled, at this status SFP eeprom is not accessible, 
#     and presence status also will be not present, 
#     so treate it as plug out.
SDK_SFP_STATE_IN  = 0x1
SDK_SFP_STATE_OUT = 0x2
SDK_SFP_STATE_ERR = 0x3
SDK_SFP_STATE_DIS = 0x4
SDK_SFP_STATE_UNKNOWN = 0x5

# SFP status used in this file only, will not expose to XCVRD
# STATUS_ERROR will be mapped to different status according to the error code
STATUS_UNKNOWN = '-1'
STATUS_ERROR   = '-2'

# SFP error code, only valid when SFP at SDK_SFP_STATE_ERR status
# Only 0x2, 0x3, 0x5, 0x6 and 0x7 will block the eeprom access,
# so will only report above errors to XCVRD and other errors will be 
# printed to syslog. 

'''
0x0: "Power_Budget_Exceeded",
0x1: "Long_Range_for_non_MLNX_cable_or_module",
0x2: "Bus_stuck",
0x3: "bad_or_unsupported_EEPROM",
0x4: "Enforce_part_number_list",
0x5: "unsupported_cable",
0x6: "High_Temperature",
0x7: "bad_cable",
0x8: "PMD_type_is_not_enabled",
0x9: "[internal]Laster_TEC_failure",
0xa: "[internal]High_current",
0xb: "[internal]High_voltage",
0xd: "[internal]High_power",
0xe: "[internal]Module_state_machine_fault",
0xc: "pcie_system_power_slot_Exceeded"
'''

# SFP errors that will block eeprom accessing
SDK_SFP_BLOCKING_ERRORS = [
    0x2, # SFP.SFP_ERROR_BIT_I2C_STUCK,
    0x3, # SFP.SFP_ERROR_BIT_BAD_EEPROM,
    0x5, # SFP.SFP_ERROR_BIT_UNSUPPORTED_CABLE,
    0x6, # SFP.SFP_ERROR_BIT_HIGH_TEMP,
    0x7, # SFP.SFP_ERROR_BIT_BAD_CABLE
]

SDK_ERRORS_TO_ERROR_BITS = {
    0x0: SFP.SFP_ERROR_BIT_POWER_BUDGET_EXCEEDED,
    0x1: SFP.SFP_MLNX_ERROR_BIT_LONGRANGE_NON_MLNX_CABLE,
    0x2: SFP.SFP_ERROR_BIT_I2C_STUCK,
    0x3: SFP.SFP_ERROR_BIT_BAD_EEPROM,
    0x4: SFP.SFP_MLNX_ERROR_BIT_ENFORCE_PART_NUMBER_LIST,
    0x5: SFP.SFP_ERROR_BIT_UNSUPPORTED_CABLE,
    0x6: SFP.SFP_ERROR_BIT_HIGH_TEMP,
    0x7: SFP.SFP_ERROR_BIT_BAD_CABLE,
    0x8: SFP.SFP_MLNX_ERROR_BIT_PMD_TYPE_NOT_ENABLED,
    0xc: SFP.SFP_MLNX_ERROR_BIT_PCIE_POWER_SLOT_EXCEEDED
}

SDK_ERRORS_TO_DESCRIPTION = {
    0x1: SFP.SFP_MLNX_ERROR_DESCRIPTION_LONGRANGE_NON_MLNX_CABLE,
    0x4: SFP.SFP_MLNX_ERROR_DESCRIPTION_ENFORCE_PART_NUMBER_LIST,
    0x8: SFP.SFP_MLNX_ERROR_DESCRIPTION_PMD_TYPE_NOT_ENABLED,
    0xc: SFP.SFP_MLNX_ERROR_DESCRIPTION_PCIE_POWER_SLOT_EXCEEDED
}

sfp_value_status_dict = {
    SDK_SFP_STATE_IN:  str(SFP.SFP_STATUS_BIT_INSERTED),
    SDK_SFP_STATE_OUT: str(SFP.SFP_STATUS_BIT_REMOVED),
    SDK_SFP_STATE_ERR: STATUS_ERROR,
    SDK_SFP_STATE_DIS: str(SFP.SFP_STATUS_BIT_REMOVED),
}

# system level event/error
EVENT_ON_ALL_SFP = '-1'
SYSTEM_NOT_READY = 'system_not_ready'
SYSTEM_READY = 'system_become_ready'
SYSTEM_FAIL = 'system_fail'

SDK_DAEMON_READY_FILE = '/tmp/sdk_ready'

PMPE_PACKET_SIZE = 2000

logger = Logger()

class sfp_event:
    ''' Listen to plugin/plugout cable events '''

    SX_OPEN_RETRIES = 30
    SX_OPEN_TIMEOUT = 5
    SELECT_TIMEOUT = 1

    def __init__(self, rj45_port_list=None):
        self.swid = 0
        self.handle = None

        # Allocate SDK fd and user channel structures
        self.rx_fd_p = new_sx_fd_t_p()
        self.user_channel_p = new_sx_user_channel_t_p()
        if rj45_port_list:
            self.RJ45_port_set = set(rj45_port_list)
        else:
            self.RJ45_port_set = set()

    def initialize(self):
        swid_cnt_p = None

        try:
            # Wait for SDK daemon to be started with detect the sdk_ready file
            retry = 0
            while not os.path.exists(SDK_DAEMON_READY_FILE):  
                if retry >= self.SX_OPEN_RETRIES:
                    raise RuntimeError("SDK daemon failed to start after {} retries and {} seconds waiting, exiting..."
                        .format(retry, self.SX_OPEN_TIMEOUT * self.SX_OPEN_RETRIES))
                else:
                    logger.log_info("SDK daemon not started yet, retry {} times".format(retry))
                    retry += 1
                    time.sleep(self.SX_OPEN_TIMEOUT)

            # After SDK daemon started, sx_api_open and sx_api_host_ifc_open is ready for call
            rc, self.handle = sx_api_open(None)
            if rc != SX_STATUS_SUCCESS:
                raise RuntimeError("failed to call sx_api_open with rc {}, exiting...".format(rc))

            rc = sx_api_host_ifc_open(self.handle, self.rx_fd_p)
            if rc != SX_STATUS_SUCCESS:
                raise RuntimeError("failed to call sx_api_host_ifc_open with rc {}, exiting...".format(rc))

            self.user_channel_p.type = SX_USER_CHANNEL_TYPE_FD
            self.user_channel_p.channel.fd = self.rx_fd_p

            # Wait for switch to be created and initialized inside SDK
            retry = 0
            swid_cnt_p = new_uint32_t_p()
            uint32_t_p_assign(swid_cnt_p, 0)
            swid_cnt = 0
            while True:
                if retry >= self.SX_OPEN_RETRIES:
                    raise RuntimeError("switch not created after {} retries and {} seconds waiting, exiting..."
                        .format(retry, self.SX_OPEN_RETRIES * self.SX_OPEN_TIMEOUT))
                else:
                    rc = sx_api_port_swid_list_get(self.handle, None, swid_cnt_p)
                    if rc == SX_STATUS_SUCCESS:
                        swid_cnt = uint32_t_p_value(swid_cnt_p)
                        if swid_cnt > 0:
                            delete_uint32_t_p(swid_cnt_p)
                            swid_cnt_p = None
                            break
                        else:
                            logger.log_info("switch not created yet, swid_cnt {}, retry {} times and wait for {} seconds"
                                .format(swid_cnt, retry, self.SX_OPEN_TIMEOUT * retry))
                    else:
                        raise RuntimeError("sx_api_port_swid_list_get fail with rc {}, retry {} times and wait for {} seconds".
                            format(rc, retry, self.SX_OPEN_TIMEOUT * retry))

                    retry += 1
                    time.sleep(self.SX_OPEN_TIMEOUT)

            # After switch was created inside SDK, sx_api_host_ifc_trap_id_register_set is ready to call
            rc = sx_api_host_ifc_trap_id_register_set(self.handle,
                                                    SX_ACCESS_CMD_REGISTER,
                                                    self.swid,
                                                    SX_TRAP_ID_PMPE,
                                                    self.user_channel_p)

            if rc != SX_STATUS_SUCCESS:
                raise RuntimeError("sx_api_host_ifc_trap_id_register_set failed with rc {}, exiting...".format(rc))
        except Exception as e:
            logger.log_error("sfp_event initialization failed due to {}, exiting...".format(repr(e)))
            if swid_cnt_p is not None:
                delete_uint32_t_p(swid_cnt_p)
            self.deinitialize()

    def deinitialize(self):
        if self.handle is None:
            return

        # unregister trap id
        rc = sx_api_host_ifc_trap_id_register_set(self.handle,
                                                  SX_ACCESS_CMD_DEREGISTER,
                                                  self.swid,
                                                  SX_TRAP_ID_PMPE,
                                                  self.user_channel_p)
        if rc != SX_STATUS_SUCCESS:
            logger.log_error("sx_api_host_ifc_trap_id_register_set exited with error, rc {}".format(rc))

        rc = sx_api_host_ifc_close(self.handle, self.rx_fd_p)
        if rc != SX_STATUS_SUCCESS:
            logger.log_error("sx_api_host_ifc_close exited with error, rc {}".format(rc))

        rc = sx_api_close(self.handle)
        if rc != SX_STATUS_SUCCESS:
            logger.log_error("sx_api_close exited with error, rc {}".format(rc))

        delete_sx_fd_t_p(self.rx_fd_p)
        delete_sx_user_channel_t_p(self.user_channel_p)

    def check_sfp_status(self, port_change, error_dict, timeout):
        """
        the meaning of timeout is aligned with select.select, which has the following meaning:
            0: poll, returns without blocked
            arbitrary positive value: doesn't returns until at least fd in the set is ready or
                                        <timeout> seconds elapsed
        Note:
            check_sfp_status makes the use of select to retrieve the notifications, which means
            it should has the logic of reading out all the notifications in the fd selected without blocked.
            However, it fails to do that due to some sdk API's characteristics:
                sx_lib_host_ifc_recv can only read one notification each time and will block when no notification in that fd.
                sx_lib_host_ifc_recv_list can return all notification in the fd via a single reading operation but
                                         not supported by PMPE register (I've tested it but failed)
            as a result the only way to satisfy the logic is to call sx_lib_host_ifc_recv in a loop until all notifications
            has been read and we have to find a way to check that. it seems the only way to check that is via using select. 
            in this sense, we return one notification each time check_sfp_status called and let the caller, get_change_event,
            to repeat calling it with timeout = 0 in a loop until no new notification read (in this case it returns false).
            by doing so all the notifications in the fd can be retrieved through a single call to get_change_event.
        """
        found = 0

        try:
            read, _, _ = select.select([self.rx_fd_p.fd], [], [], float(timeout) / 1000)
            print(read)
        except select.error as err:
            rc, msg = err
            if rc == errno.EAGAIN or rc == errno.EINTR:
                return False
            else:
                raise

        for fd in read:
            if fd == self.rx_fd_p.fd:
                success, port_list, module_state, error_type = self.on_pmpe(self.rx_fd_p)
                print('success = ', success)
                if not success:
                    logger.log_error("failed to read from {}".format(fd))
                    break

                sfp_state = sfp_value_status_dict.get(module_state, STATUS_UNKNOWN)
                error_description = None
                if sfp_state == STATUS_UNKNOWN:
                    # in the following sequence, STATUS_UNKNOWN can be returned.
                    # so we shouldn't raise exception here.
                    # 1. some sfp module is inserted
                    # 2. sfp_event gets stuck and fails to fetch the change event instantaneously
                    # 3. and then the sfp module is removed
                    # 4. sfp_event starts to try fetching the change event
                    # in this case found is increased so that True will be returned
                    logger.log_info("unknown module state {}, maybe the port suffers two adjacent insertion/removal".format(module_state))
                    found += 1
                    continue

                # If get SFP status error(0x3) from SDK, then need to read the error_type to get the detailed error
                if sfp_state == STATUS_ERROR:
                    sfp_state_bits = SDK_ERRORS_TO_ERROR_BITS.get(error_type)
                    if sfp_state_bits is None:
                        logger.log_error("Unrecognized error {} detected on ports {}".format(error_type, port_list))
                        found += 1
                        continue

                    if error_type in SDK_SFP_BLOCKING_ERRORS:
                        # In SFP at error status case, need to overwrite the sfp_state with the exact error code
                        sfp_state_bits |= SFP.SFP_ERROR_BIT_BLOCKING

                    # An error should be always set along with 'INSERTED'
                    sfp_state_bits |= SFP.SFP_STATUS_BIT_INSERTED

                    # For vendor specific errors, the description should be returned as well
                    error_description = SDK_ERRORS_TO_DESCRIPTION.get(error_type)

                    sfp_state = str(sfp_state_bits)

                for port in port_list:
                    logger.log_info("SFP on port {} state {}".format(port, sfp_state))
                    port_change[port+1] = sfp_state
                    if error_description:
                        error_dict[port+1] = error_description
                    found += 1

        return found != 0

    def on_pmpe(self, fd_p):
        ''' on port module plug event handler '''

        # recv parameters
        pkt_size = PMPE_PACKET_SIZE
        pkt_size_p = new_uint32_t_p()
        uint32_t_p_assign(pkt_size_p, pkt_size)
        pkt = new_uint8_t_arr(pkt_size)
        recv_info_p = new_sx_receive_info_t_p()
        pmpe_t = sx_event_pmpe_t()
        port_cnt_p = new_uint32_t_p()
        uint32_t_p_assign(port_cnt_p, 0)
        label_port_list = []
        module_state = 0

        rc = sx_lib_host_ifc_recv(fd_p, pkt, pkt_size_p, recv_info_p)
        if rc != 0:
            logger.log_error("sx_lib_host_ifc_recv exited with error, rc %d" % rc)
            status = False
        else:
            status = True
            unknown = False
            pmpe_t = recv_info_p.event_info.pmpe
            port_list_size = pmpe_t.list_size
            logical_port_list = pmpe_t.log_port_list
            module_state = pmpe_t.module_state
            error_type = pmpe_t.error_type
            module_id = pmpe_t.module_id
            slot_id = pmpe_t.slot_id  # For non-modular chassis, it should return 0

            if module_state == SDK_SFP_STATE_ERR:
                logger.log_error("Receive PMPE error event on module {}: status {} error type {}".format(module_id, module_state, error_type))
            elif module_state == SDK_SFP_STATE_DIS:
                logger.log_notice("Receive PMPE disable event on module {}: status {}".format(module_id, module_state))
            elif module_state == SDK_SFP_STATE_IN or module_state == SDK_SFP_STATE_OUT:
                logger.log_notice("Receive PMPE plug in/out event on module {}: status {}".format(module_id, module_state))
            elif module_state == SDK_SFP_STATE_UNKNOWN:
                unknown = True
            else:
                logger.log_error("Receive PMPE unknown event on module {}: status {}".format(module_id, module_state))

            # Call sx_api_port_device_get with port_cnt_p=0, SDK will return the logical port number
            rc = sx_api_port_device_get(self.handle, 1, 0, None,  port_cnt_p)
            if rc != SX_STATUS_SUCCESS:
                logger.log_error("Failed to get logical port number")
                status = False
            else:
                port_cnt = uint32_t_p_value(port_cnt_p)
                port_attributes_list = new_sx_port_attributes_t_arr(port_cnt)
                rc = sx_api_port_device_get(self.handle, 1, 0, port_attributes_list,  port_cnt_p)
                if rc != SX_STATUS_SUCCESS:
                    logger.log_error("Failed to get logical port attributes")
                    status = False
                else:
                    for i in range(port_list_size):
                        label_port = None
                        logical_port = sx_port_log_id_t_arr_getitem(logical_port_list, i)
                        for j in range(port_cnt):
                            port_attributes = sx_port_attributes_t_arr_getitem(port_attributes_list,j)
                            if port_attributes.log_port == logical_port:
                                label_port = slot_id * DeviceDataManager.get_linecard_max_port_count() + port_attributes.port_mapping.module_port
                                break

                        if label_port is not None:
                            label_port_list.append(label_port)
                delete_sx_port_attributes_t_arr(port_attributes_list)

            if unknown:
                SFP_ports_with_unknown_event = set(label_port_list) - self.RJ45_port_set
                if SFP_ports_with_unknown_event:
                    logger.log_error("Receive PMPE unknown event on module {}: status {}".format(module_id, module_state))
                else:
                    # For RJ45 ports, we treat unknown as disconnect
                    module_state = SDK_SFP_STATE_DIS

        delete_uint32_t_p(pkt_size_p)
        delete_uint8_t_arr(pkt)
        delete_sx_receive_info_t_p(recv_info_p)
        delete_uint32_t_p(port_cnt_p)

        if not label_port_list:
            logger.log_error('Dropping PMPE event due to label port not found')

        return status, label_port_list, module_state, error_type
