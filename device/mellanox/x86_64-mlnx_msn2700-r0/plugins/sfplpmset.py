#!/usr/bin/env python3
#
# Copyright (c) 2018-2021 NVIDIA CORPORATION & AFFILIATES.
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

"""
This utility set the power mode of a given module.
"""

import sys
import errno
from python_sdk_api.sx_api import *


DEVICE_ID = 1
SWITCH_ID = 0
SX_PORT_ATTR_ARR_SIZE = 64

PORT_TYPE_CPU = 4
PORT_TYPE_NVE = 8
PORT_TYPE_OFFSET = 28
PORT_TYPE_MASK = 0xF0000000
NVE_MASK = PORT_TYPE_MASK & (PORT_TYPE_NVE << PORT_TYPE_OFFSET)
CPU_MASK = PORT_TYPE_MASK & (PORT_TYPE_CPU << PORT_TYPE_OFFSET)


def is_nve(port):
    return (port & NVE_MASK) != 0


def is_cpu(port):
    return (port & CPU_MASK) != 0


def is_port_admin_status_up(log_port):
    oper_state_p = new_sx_port_oper_state_t_p()
    admin_state_p = new_sx_port_admin_state_t_p()
    module_state_p = new_sx_port_module_state_t_p()
    rc = sx_api_port_state_get(handle, log_port, oper_state_p, admin_state_p, module_state_p)
    assert rc == SX_STATUS_SUCCESS, "sx_api_port_state_get failed, rc = %d" % rc

    admin_state = sx_port_admin_state_t_p_value(admin_state_p)
    if admin_state == SX_PORT_ADMIN_STATUS_UP:
        return True
    else:
        return False


def set_port_admin_status_by_log_port(handle, log_port, admin_status):
    rc = sx_api_port_state_set(handle, log_port, admin_status)
    assert rc == SX_STATUS_SUCCESS, "sx_api_port_state_set failed, rc = %d" % rc

# Get all the ports related to the sfp, if port admin status is up, put it to list


def get_log_ports(handle, sfp_module):
    port_attributes_list = new_sx_port_attributes_t_arr(SX_PORT_ATTR_ARR_SIZE)
    port_cnt_p = new_uint32_t_p()
    uint32_t_p_assign(port_cnt_p, SX_PORT_ATTR_ARR_SIZE)

    rc = sx_api_port_device_get(handle, DEVICE_ID, SWITCH_ID, port_attributes_list,  port_cnt_p)
    assert rc == SX_STATUS_SUCCESS, "sx_api_port_device_get failed, rc = %d" % rc

    port_cnt = uint32_t_p_value(port_cnt_p)
    log_port_list = []
    for i in range(0, port_cnt):
        port_attributes = sx_port_attributes_t_arr_getitem(port_attributes_list, i)
        if not is_nve(int(port_attributes.log_port)) \
           and not is_cpu(int(port_attributes.log_port)) \
           and port_attributes.port_mapping.module_port == sfp_module \
           and is_port_admin_status_up(port_attributes.log_port):
            log_port_list.append(port_attributes.log_port)

    return log_port_list


def mgmt_phy_mod_pwr_attr_set(handle, module_id, power_attr_type, admin_pwr_mode):
    sx_mgmt_phy_mod_pwr_attr = sx_mgmt_phy_mod_pwr_attr_t()
    sx_mgmt_phy_mod_pwr_mode_attr = sx_mgmt_phy_mod_pwr_mode_attr_t()
    sx_mgmt_phy_mod_pwr_attr.power_attr_type = power_attr_type
    sx_mgmt_phy_mod_pwr_mode_attr.admin_pwr_mode_e = admin_pwr_mode
    sx_mgmt_phy_mod_pwr_attr.pwr_mode_attr = sx_mgmt_phy_mod_pwr_mode_attr
    sx_mgmt_phy_mod_pwr_attr_p = new_sx_mgmt_phy_mod_pwr_attr_t_p()
    sx_mgmt_phy_mod_pwr_attr_t_p_assign(sx_mgmt_phy_mod_pwr_attr_p, sx_mgmt_phy_mod_pwr_attr)
    try:
        rc = sx_mgmt_phy_mod_pwr_attr_set(handle, SX_ACCESS_CMD_SET, module_id, sx_mgmt_phy_mod_pwr_attr_p)
        assert SX_STATUS_SUCCESS == rc, "sx_mgmt_phy_mod_pwr_attr_set failed"
    finally:
        delete_sx_mgmt_phy_mod_pwr_attr_t_p(sx_mgmt_phy_mod_pwr_attr_p)


def mgmt_phy_mod_pwr_attr_get(handle, module_id, power_attr_type):
    sx_mgmt_phy_mod_pwr_attr_p = new_sx_mgmt_phy_mod_pwr_attr_t_p()
    sx_mgmt_phy_mod_pwr_attr = sx_mgmt_phy_mod_pwr_attr_t()
    sx_mgmt_phy_mod_pwr_attr.power_attr_type = power_attr_type
    sx_mgmt_phy_mod_pwr_attr_t_p_assign(sx_mgmt_phy_mod_pwr_attr_p, sx_mgmt_phy_mod_pwr_attr)
    try:
        rc = sx_mgmt_phy_mod_pwr_attr_get(handle, module_id, sx_mgmt_phy_mod_pwr_attr_p)
        assert SX_STATUS_SUCCESS == rc, "sx_mgmt_phy_mod_pwr_attr_get failed"
        sx_mgmt_phy_mod_pwr_attr = sx_mgmt_phy_mod_pwr_attr_t_p_value(sx_mgmt_phy_mod_pwr_attr_p)
        pwr_mode_attr = sx_mgmt_phy_mod_pwr_attr.pwr_mode_attr
        return pwr_mode_attr.admin_pwr_mode_e, pwr_mode_attr.oper_pwr_mode_e
    finally:
        delete_sx_mgmt_phy_mod_pwr_attr_t_p(sx_mgmt_phy_mod_pwr_attr_p)


def pwr_attr_set(handle, module_id, ports, attr_type, power_mode):
    # Check if the module already works in the same mode
    admin_pwr_mode, oper_pwr_mode = mgmt_phy_mod_pwr_attr_get(handle, module_id, attr_type)
    if (power_mode == SX_MGMT_PHY_MOD_PWR_MODE_LOW_E and oper_pwr_mode == SX_MGMT_PHY_MOD_PWR_MODE_LOW_E) \
       or (power_mode == SX_MGMT_PHY_MOD_PWR_MODE_AUTO_E and admin_pwr_mode == SX_MGMT_PHY_MOD_PWR_MODE_AUTO_E):
        return
    try:
        # Bring the port down
        for port in ports:
            set_port_admin_status_by_log_port(handle, port, SX_PORT_ADMIN_STATUS_DOWN)
        # Set the desired power mode
        mgmt_phy_mod_pwr_attr_set(handle, module_id, attr_type, power_mode)
        # Bring the port up
    finally:
        for port in ports:
            set_port_admin_status_by_log_port(handle, port, SX_PORT_ADMIN_STATUS_UP)


def set_lpmode(handle, cmd, module_id):
    # Construct the port module map.
    log_port_list = get_log_ports(handle, module_id)

    if cmd == "enable":
        pwr_attr_set(handle, module_id, log_port_list,
                     SX_MGMT_PHY_MOD_PWR_ATTR_PWR_MODE_E, SX_MGMT_PHY_MOD_PWR_MODE_LOW_E)
        print("Enabled low power mode for module [%d]" % module_id)
    elif cmd == "disable":
        pwr_attr_set(handle, module_id, log_port_list,
                     SX_MGMT_PHY_MOD_PWR_ATTR_PWR_MODE_E, SX_MGMT_PHY_MOD_PWR_MODE_AUTO_E)
        print("Disabled low power mode for module [%d]" % module_id)
    else:
        print("Error: Invalid command")
        sys.exit(0)


if len(sys.argv) < 3:
    print("SFP module number or LPM is missed.")
    print("Usage: sfplpmset.py <SFP module> <on|off>")
    sys.exit(errno.EINVAL)

cmd = None
lpm_enable = None
if sys.argv[2] == 'on':
    lpm_enable = True
    cmd = 'enable'
elif sys.argv[2] == 'off':
    lpm_enable = False
    cmd = 'disable'
else:
    print("Unrecognized LPM parameter. Please use <on> or <off> values")
    sys.exit(errno.EINVAL)

# Get SFP module
sfp_module = int(sys.argv[1]) - 1

print("[+] opening sdk")
rc, handle = sx_api_open(None)

if (rc != SX_STATUS_SUCCESS):
    print("Failed to open api handle.\nPlease check that SDK is running.")
    sys.exit(errno.EACCES)

# Set low power mode
set_lpmode(handle, cmd, sfp_module)

sx_api_close(handle)
