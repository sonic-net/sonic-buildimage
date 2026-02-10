#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2022-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

from python_sdk_api.sx_api import *
import inspect
import re

DEVICE_ID = 1
SWITCH_ID = 0
PORT_TABLE = 'PORT'
FIRST_LANE_INDEX = 0
ETHERNET_PREFIX = 'Ethernet'


def get_sonic_ports(config_db):
    """
    Get sorted list of SONiC ports from config DB
    """
    config_db.connect()
    ports_table = config_db.get_table(PORT_TABLE)
    if ports_table is None:
        raise Exception("Can't read {} table".format(PORT_TABLE))

    ports_table_keys = config_db.get_keys(PORT_TABLE)
    ports = [port for port in ports_table_keys if ETHERNET_PREFIX in port]
    ports.sort(key=lambda p: int(p.replace(ETHERNET_PREFIX, "")))
    return ports

def get_port_max_width(handle):
    """ Get max number of lanes in port according to chip type

    Args:
        handle (sx_api_handle_t): SDK handle

    Returns:
        int: max lanes in port
    """
    # Get chip type
    chip_type = sx_get_chip_type(handle)

    limits = rm_resources_t()
    modes = rm_modes_t()

    rc = rm_chip_limits_get(chip_type, limits)
    sx_check_rc(rc)
    max_width = limits.port_map_width_max

    # SPC2 ports have 8 lanes but SONiC is using 4
    if chip_type == SX_CHIP_TYPE_SPECTRUM2:
        max_width = 4

    return max_width

def sx_get_ports_map(handle, config_db):
    """ Get ports map from SDK logical index to SONiC index
    
    Args:
        handle (sx_api_handle_t): SDK handle
        config_db (ConfigDBConnector): Config DB connector

    Returns:
        dict: key is SDK logical index, value is SONiC index (4 for Ethernet4)
    """         
    try:
        ports_map = {}
        port_attributes_list = None
        port_cnt_p = None
        
        # Get ports count
        port_cnt_p = new_uint32_t_p()
        rc = sx_api_port_device_get(handle, DEVICE_ID, SWITCH_ID, None,  port_cnt_p)
        sx_check_rc(rc)
        
        # Get ports from SDK
        port_cnt = uint32_t_p_value(port_cnt_p)
        port_attributes_list = new_sx_port_attributes_t_arr(port_cnt)
        rc = sx_api_port_device_get(handle, DEVICE_ID, SWITCH_ID, port_attributes_list,  port_cnt_p)
        sx_check_rc(rc)
        
        # Get ports attributes, sort them by module_id, submodule id and lane bmap (control panel order)
        ports = [sx_port_attributes_t_arr_getitem(port_attributes_list, i) for i in range(port_cnt)]
        ports_attributes = [port_attr for port_attr in ports if is_phy_port(port_attr.log_port)]
        ports_attributes.sort(key=lambda p: (p.port_mapping.module_port, getattr(p.port_mapping, "submodule_id", 0), p.port_mapping.lane_bmap))

        # Get sorted list of SONiC ports from config DB
        sonic_ports = get_sonic_ports(config_db)

        assert len(ports_attributes) == len(sonic_ports), "Number of ports in SDK and SONiC do not match"

        # Map SDK logical ports to SONiC ports
        for i, port in enumerate(sonic_ports):
            ports_map[ports_attributes[i].log_port] = port
        return ports_map
    
    finally:
        delete_sx_port_attributes_t_arr(port_attributes_list)
        delete_uint32_t_p(port_cnt_p)
        
def sx_get_chip_type(handle):
    """ Get system ASIC type
    
    Args:
        handle (sx_api_handle_t): SDK handle
    
    Returns:
        sx_chip_types_t: Chip type
    """       
    try:
        device_info_cnt_p = new_uint32_t_p()
        uint32_t_p_assign(device_info_cnt_p, 1)
        device_info_cnt = uint32_t_p_value(device_info_cnt_p)
        device_info_list_p = new_sx_device_info_t_arr(device_info_cnt)
        
        rc = sx_api_port_device_list_get(handle, device_info_list_p, device_info_cnt_p)
        sx_check_rc(rc)
    
        device_info = sx_device_info_t_arr_getitem(device_info_list_p, SWITCH_ID)
        chip_type = device_info.dev_type
        if chip_type == SX_CHIP_TYPE_SPECTRUM_A1:
            chip_type = SX_CHIP_TYPE_SPECTRUM
            
        return chip_type
    
    finally:
        delete_sx_device_info_t_arr(device_info_list_p)
        delete_uint32_t_p(device_info_cnt_p)

def get_lane_index(lane_bmap, max_lanes):
   """ Get index of first lane in use (2 for 00001100)
   
   Args:
       lane_bmap (int): bitmap indicating module lanes in use
       max_lanes (int): Max lanes in module

   Returns:
       int: index of the first bit set to 1 in lane_bmap
   """       
   for lane_idx in range(0, max_lanes):
       if (lane_bmap & 0x1 == 1):
           return lane_idx
       lane_bmap = lane_bmap >> 1
       
def sx_check_rc(rc):
    if rc is not SX_STATUS_SUCCESS:
        # Get the calling function name from the last frame
        cf = inspect.currentframe().f_back
        func_name = inspect.getframeinfo(cf).function
        error_info = func_name + ' failed with rc = ' + str(rc)
    
        raise Exception(error_info)       

def get_port_type(log_port_id):
    return (log_port_id & SX_PORT_TYPE_ID_MASK) >> SX_PORT_TYPE_ID_OFFS

def is_phy_port(log_port_id):
    return get_port_type(log_port_id) == SX_PORT_TYPE_NETWORK

def is_lag(log_port_id):
    return get_port_type(log_port_id) == SX_PORT_TYPE_LAG
