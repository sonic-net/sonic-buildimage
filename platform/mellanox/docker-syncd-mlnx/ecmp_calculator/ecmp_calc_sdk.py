#!/usr/bin/env python3
#
# Copyright (c) 2022-2023 NVIDIA CORPORATION & AFFILIATES.
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
from port_utils import sx_check_rc, SWITCH_ID
import struct
import socket
import ctypes
import sys

PORT='port'
VPORT='vport'
VLAN='vlan'
SX_ENTRY_NOT_FOUND = -1
SDK_DEFAULT_LOG_LEVEL=SX_VERBOSITY_LEVEL_ERROR

def severity_to_verbosity(severity):
    value = severity + 1
    verbosity = 0
    while not (value & 1):
        value = value >> 1
        verbosity = verbosity + 1
    return verbosity

def log_cb(severity, module_name, msg):
    verbosity = severity_to_verbosity(severity)
    log_level = {SX_VERBOSITY_LEVEL_INFO: 'LOG_INFO', SX_VERBOSITY_LEVEL_ERROR: 'LOG_ERR', 
                 SX_VERBOSITY_LEVEL_WARNING: 'LOG_WARN', SX_VERBOSITY_LEVEL_DEBUG: 'LOG_DEBUG', 
                 SX_VERBOSITY_LEVEL_NOTICE: 'LOG_NOTICE'}
    if verbosity in log_level.keys():
        level = log_level[verbosity]
    else:
        level = 'LOG'
    if verbosity <= SDK_DEFAULT_LOG_LEVEL:
        print('{}: {}'.format(level, ctypes.cast(msg,ctypes.c_char_p).value.decode('utf-8')))
 
def sx_open_sdk_connection():
    """ Open connection to SDK
    
    Args:
        None
    
    Returns:
        SDK handle
    """     
    sx_api = ctypes.CDLL('libsxapi.so', mode=ctypes.RTLD_GLOBAL)
    log_cb_type = ctypes.CFUNCTYPE(None, ctypes.c_int, 
                                   ctypes.POINTER(ctypes.c_char), 
                                   ctypes.POINTER(ctypes.c_char))
    log_cb_fn = log_cb_type(log_cb)
    handle_p = ctypes.pointer(ctypes.c_int64())
    
    rc = sx_api.sx_api_open(log_cb_fn, handle_p)
    sx_check_rc(rc)

    return handle_p.contents.value

def sx_close_sdk_connection(handle):
    """ Close connection to SDK
    
    Args:
        SDK handle
    """ 
    rc = sx_api_close(handle)  
    sx_check_rc(rc)

def sx_get_active_vrids(handle):
    """ Get existing virtual router IDs
    
    Args:
        handle (sx_api_handle_t): SDK handle
    
    Returns:
        list : List of virtual routers ids
    """      
    try:
        vrid_list = []
        
        vrid_cnt_p = new_uint32_t_p()
        uint32_t_p_assign(vrid_cnt_p, 0)
        vrid_key_p = new_sx_router_id_t_p()
        sx_router_id_t_p_assign(vrid_key_p, 0)
        vrid_key = sx_router_id_t_p_value(vrid_key_p)
        
        rc = sx_api_router_vrid_iter_get(handle, SX_ACCESS_CMD_GET, vrid_key, None, None, vrid_cnt_p)
        sx_check_rc(rc)
        
        vrid_cnt = uint32_t_p_value(vrid_cnt_p)
        vrid_list_p = new_sx_router_id_t_arr(vrid_cnt)
        
        rc = sx_api_router_vrid_iter_get(handle, SX_ACCESS_CMD_GET_FIRST, vrid_key, None, vrid_list_p, vrid_cnt_p)
        sx_check_rc(rc)
        
        vrid_cnt = uint32_t_p_value(vrid_cnt_p)
        for i in range(0, vrid_cnt):
            vrid = sx_router_id_t_arr_getitem(vrid_list_p, i)
            vrid_list.append(vrid)

        return vrid_list

    finally:
        delete_sx_router_id_t_arr(vrid_list_p)
        delete_sx_router_id_t_p(vrid_key_p)
        delete_uint32_t_p(vrid_cnt_p)

def sx_router_get_ecmp_id(handle, vrid, ip_prefix):
    """ Get ECMP id for a given IP prefix
    
    Args:
        handle (sx_api_handle_t): SDK handle
        vrid (sx_router_id_t): Virtual router id
        ip_prefix (sx_ip_prefix_t): Network address 
    
    Returns:
        int: ECMP id
    """
    try:
        ip_prefix_p = new_sx_ip_prefix_t_p()
        sx_ip_prefix_t_p_assign(ip_prefix_p, ip_prefix)
        entries_cnt_p = new_uint32_t_p()
        uint32_t_p_assign(entries_cnt_p, 1)
        entries_array = new_sx_uc_route_get_entry_t_arr(1)
        
        rc = sx_api_router_uc_route_get(handle, SX_ACCESS_CMD_GET, vrid, ip_prefix_p, None, entries_array, entries_cnt_p)
        if rc == SX_STATUS_ENTRY_NOT_FOUND:
            return SX_ENTRY_NOT_FOUND
        sx_check_rc(rc)

        entry = sx_uc_route_get_entry_t_arr_getitem(entries_array, 0)
        if entry.route_data.type == SX_UC_ROUTE_TYPE_NEXT_HOP:
            return entry.route_data.uc_route_param.ecmp_id

        return SX_ENTRY_NOT_FOUND

    finally:
        delete_sx_uc_route_get_entry_t_arr(entries_array)
        delete_uint32_t_p(entries_cnt_p)
        delete_sx_ip_prefix_t_p(ip_prefix_p)

def sx_router_ecmp_nexthops_get(handle, ecmp_id):
    """ Get next hops for a given ECMP id
    
    Args:
        handle (sx_api_handle_t): SDK handle
        ecmp_id (int): ECMP id
    
    Returns:
        list: List of next hops
    """    
    try:
        next_hops = []
        
        next_hop_count_p = new_uint32_t_p()
        uint32_t_p_assign(next_hop_count_p, 0)
        
        rc = sx_api_router_operational_ecmp_get(handle, ecmp_id, None, next_hop_count_p)
        sx_check_rc(rc)
        
        next_hop_count  = uint32_t_p_value(next_hop_count_p)
        next_hop_list_p = new_sx_next_hop_t_arr(next_hop_count)
        
        rc = sx_api_router_operational_ecmp_get(handle, ecmp_id, next_hop_list_p, next_hop_count_p)
        sx_check_rc(rc)
        
        next_hop_count  = uint32_t_p_value(next_hop_count_p)
        for i in range(next_hop_count):
            next_hop = sx_next_hop_t_arr_getitem(next_hop_list_p, i)
            if next_hop.next_hop_key.type == SX_NEXT_HOP_TYPE_IP:
                next_hops.append(next_hop)
       
        return next_hops

    finally:
        delete_sx_next_hop_t_arr(next_hop_list_p)
        delete_uint32_t_p(next_hop_count_p)

def sx_get_router_interface(handle, vrid, rif):
    """ Get router interface information
    
    Args:
        handle (sx_api_handle_t): SDK handle
        vrid (sx_router_id_t): virtual router id
        rif (sx_router_interface_t): router interface id
    
    Returns:
        dict : Dictionary contains interface parameters
    """       
    try:
        vrid_p = new_sx_router_id_t_p()
        sx_router_id_t_p_assign(vrid_p, vrid)
    
        ifc_p = new_sx_router_interface_param_t_p()
        ifc_attr_p = new_sx_interface_attributes_t_p()
        rif_params = {}
        
        rc = sx_api_router_interface_get(handle, rif, vrid_p, ifc_p, ifc_attr_p)
        sx_check_rc(rc)
        
        if ifc_p.type == SX_L2_INTERFACE_TYPE_PORT_VLAN:
            rif_params[PORT] = ifc_p.ifc.port_vlan.port

        if ifc_p.type == SX_L2_INTERFACE_TYPE_VPORT:
            rif_params[VPORT] = ifc_p.ifc.vport.vport        

        if ifc_p.type == SX_L2_INTERFACE_TYPE_VLAN:
            rif_params[VLAN] = ifc_p.ifc.vlan.vlan

        return rif_params
        
    finally:
        delete_sx_interface_attributes_t_p(ifc_attr_p)
        delete_sx_router_interface_param_t_p(ifc_p)
        delete_sx_router_id_t_p(vrid_p)

def sx_port_vport_base_get(handle, vport):
    """ Get SDK logical index and vlan for given vport
    
    Args:
        handle (sx_api_handle_t): SDK handle
        vport (sx_port_id_t): SDK vport id

    Returns:
        sx_port_log_id_t : SDK logical index 
    """      
    try:
        vlan_id_p = new_sx_vlan_id_t_p()
        logical_port_p = new_sx_port_log_id_t_p()
        
        rc = sx_api_port_vport_base_get(handle, vport, vlan_id_p, logical_port_p)
        sx_check_rc(rc)
        
        logical_port = sx_port_log_id_t_p_value(logical_port_p)
        vlan_id = sx_vlan_id_t_p_value(vlan_id_p)
        
        return logical_port, vlan_id
      
    finally:
        delete_sx_port_log_id_t_p(logical_port_p) 
        delete_sx_vlan_id_t_p(vlan_id_p)

def sx_router_neigh_get_mac(handle, rif, addr):
    """ Get neighbour mac address
    
    Args:
        handle (sx_api_handle_t): SDK handle
        rif (sx_port_id_t): SDK vport id
        addr (sx_ip_addr_t): Neighbour IP address

    Returns:
        str : Neighbour mac address
    """     
    try:
        neigh_entry_cnt_p = new_uint32_t_p()
        neigh_entry_list_p = new_sx_neigh_get_entry_t_arr(1)

        filter_p = new_sx_neigh_filter_t_p()
        neigh_filter = sx_neigh_filter_t()
        neigh_filter.filter_by_rif = SX_KEY_FILTER_FIELD_NOT_VALID
        neigh_filter.rif = 0
        sx_neigh_filter_t_p_assign(filter_p, neigh_filter)
        
        rc = sx_api_router_neigh_get(handle, SX_ACCESS_CMD_GET, rif, addr, filter_p, neigh_entry_list_p, neigh_entry_cnt_p)
        if rc == SX_STATUS_ENTRY_NOT_FOUND:
            return None        
        sx_check_rc(rc)

        neighbor_entry = sx_neigh_get_entry_t_arr_getitem(neigh_entry_list_p, 0)
        
        return neighbor_entry.neigh_data.mac_addr.to_str()
    
    finally:
        delete_sx_neigh_filter_t_p(filter_p)
        delete_sx_neigh_get_entry_t_arr(neigh_entry_list_p)    
        delete_uint32_t_p(neigh_entry_cnt_p)
        
def sx_fdb_uc_mac_addr_get(handle, vlan_id, mac_addr):
    """ Get UC mac entry from FDB
    
    Args:
        handle (sx_api_handle_t): SDK handle
        vlan_id (sx_vlan_id_t): VLAN id
        mac_addr (str): mac address

    Returns:
        sx_fdb_uc_mac_addr_params_t : FDB mac entry
    """       
    try:
        key = sx_fdb_uc_mac_addr_params_t()
        key.fid_vid = vlan_id
        key.mac_addr = ether_addr(mac_addr)
        key.action = SX_FDB_ACTION_FORWARD
        key_p = copy_sx_fdb_uc_mac_addr_params_t_p(key)
        
        key_filter = sx_fdb_uc_key_filter_t()
        key_filter.filter_by_fid =  SX_FDB_KEY_FILTER_FIELD_VALID
        key_filter.filter_by_mac_addr = SX_FDB_KEY_FILTER_FIELD_VALID
        key_filter.filter_by_log_port = SX_FDB_KEY_FILTER_FIELD_NOT_VALID
        key_filter.fid = vlan_id
        key_filter.mac_addr = ether_addr(mac_addr)
        key_filter_p = copy_sx_fdb_uc_key_filter_t_p(key_filter)
        
        data_cnt_p = copy_uint32_t_p(SX_FDB_MAX_GET_ENTRIES)
        mac_list_p = new_sx_fdb_uc_mac_addr_params_t_arr(SX_FDB_MAX_GET_ENTRIES)
        
        rc = sx_api_fdb_uc_mac_addr_get(handle, 0, SX_ACCESS_CMD_GET_FIRST, SX_FDB_UC_ALL, key_p, key_filter_p, mac_list_p, data_cnt_p)
        if rc == SX_STATUS_ENTRY_NOT_FOUND:
            return None
        sx_check_rc(rc)

        data_cnt = uint32_t_p_value(data_cnt_p)
        if data_cnt == 0:
            return None

        assert data_cnt == 1, "Got unexpected macs amount, mac {} vlan {} data_cnt {}".format(mac_addr, vlan_id, data_cnt)
        
        mac_entry = sx_fdb_uc_mac_addr_params_t_arr_getitem(mac_list_p, 0)
        assert mac_entry.dest_type == SX_FDB_UC_MAC_ADDR_DEST_TYPE_LOGICAL_PORT, "Got unexpected mac entry type {}".format(mac_entry.dest_type)

        return mac_entry

    finally:
        delete_sx_fdb_uc_mac_addr_params_t_arr(mac_list_p)
        delete_uint32_t_p(data_cnt_p)    
        delete_sx_fdb_uc_key_filter_t_p(key_filter_p)
        delete_sx_fdb_uc_mac_addr_params_t_p(key_p)   
        
def sx_lag_port_group_get(handle, lag_id):
    """ Get LAG members
    
    Args:
        handle (sx_api_handle_t): SDK handle
        lag_id (sx_port_log_id_t): LAG id

    Returns:
        list : list of LAG members logical indices
    """      
    try:
        lag_members = []
        port_count_p = new_uint32_t_p()
        uint32_t_p_assign(port_count_p, 0)
        port_arr = None

        rc = sx_api_lag_port_group_get(handle, 0, lag_id, port_arr, port_count_p)
        sx_check_rc(rc)
        
        port_count = uint32_t_p_value(port_count_p)
        if port_count > 0:
            port_arr = new_sx_port_log_id_t_arr(port_count)
            rc = sx_api_lag_port_group_get(handle, 0, lag_id, port_arr, port_count_p)
            sx_check_rc(rc)
            
            for i in range(port_count):
                lag_members.append(sx_port_log_id_t_arr_getitem(port_arr, i))
                                
        return lag_members
    
    finally:
        delete_sx_port_log_id_t_arr(port_arr) 
        delete_uint32_t_p(port_count_p)
        
def sx_vlan_ports_get(handle, vlan_id):
    """ Get VLAN member ports
    Args:
        handle (sx_api_handle_t): SDK handle
        vlan_id (sx_vid_t): VLAN id

    Returns:
        list : list of VLAN members logical indexes
    """       
    try:
        vlan_members = []
        port_cnt_p = new_uint32_t_p()
        uint32_t_p_assign(port_cnt_p, 0)
        
        rc = sx_api_vlan_ports_get(handle, SWITCH_ID, vlan_id, None, port_cnt_p)
        sx_check_rc(rc)
    
        port_cnt = uint32_t_p_value(port_cnt_p)
        vlan_port_list_p = new_sx_vlan_ports_t_arr(port_cnt)
        
        rc = sx_api_vlan_ports_get(handle, SWITCH_ID, vlan_id, vlan_port_list_p, port_cnt_p)
        sx_check_rc(rc)
    
        for i in range(0, port_cnt):
            vlan_port = sx_vlan_ports_t_arr_getitem(vlan_port_list_p, i)
            vlan_members.append(vlan_port.log_port)
        
        return vlan_members
               
    finally:
        delete_sx_vlan_ports_t_arr(vlan_port_list_p)
        delete_uint32_t_p(port_cnt_p)

def sx_make_ip_prefix_v4(addr, mask):
    """ Create IPv4 prefix
    
    Args:
        addr (str): IPv4 address
        mask (str): Network mask

    Returns:
        sx_ip_prefix_t : IPv4 prefix
    """       
    ip_prefix = sx_ip_prefix_t()
    ip_prefix.version = SX_IP_VERSION_IPV4
    ip_prefix.prefix.ipv4.addr.s_addr = struct.unpack('>I', socket.inet_pton(socket.AF_INET, addr))[0]
    ip_prefix.prefix.ipv4.mask.s_addr = struct.unpack('>I', socket.inet_pton(socket.AF_INET, mask))[0]
    
    return ip_prefix

def sx_make_ip_prefix_v6(addr, mask):
    """ Create IPv6 prefix

    Args:
        addr (str): IPv6 address
        mask (str): Network mask

    Returns:
        sx_ip_v6_prefix_t : IPv6 prefix
    """       
    addr = ipv6_str_to_bytes(str(addr))
    mask = ipv6_str_to_bytes(str(mask))

    ip_prefix = sx_ip_prefix_t()
    ip_prefix.version = SX_IP_VERSION_IPV6
    for i in range(0, 16):
        uint8_t_arr_setitem(ip_prefix.prefix.ipv6.addr._in6_addr__in6_u._in6_addr___in6_u__u6_addr8, i, addr[i])
        uint8_t_arr_setitem(ip_prefix.prefix.ipv6.mask._in6_addr__in6_u._in6_addr___in6_u__u6_addr8, i, mask[i])
        
    return ip_prefix

def ipv6_str_to_bytes(address):
    # Load the ipv6 string into 4 dwords
    dwords = struct.unpack("!IIII", socket.inet_pton(socket.AF_INET6, address))
    # Convert dwords endian using ntohl
    total_bytes = [socket.ntohl(i) for i in dwords]
    # Finally, convert back to bytes
    out = struct.pack(">IIII", total_bytes[0], total_bytes[1], total_bytes[2], total_bytes[3])
    return [i for i in out]

def sx_ip_addr_to_str(ip_addr):
    if ip_addr.version == SX_IP_VERSION_IPV4:
        return socket.inet_ntop(socket.AF_INET, struct.pack('!I', ip_addr.addr.ipv4.s_addr))
    else:
        bytes = ip_addr.addr.ipv6._in6_addr__in6_u._in6_addr___in6_u__u6_addr8
        byte_arr = []
        for i in range(0, 16):
            byte_arr.append(uint8_t_arr_getitem(bytes, i))
        if sys.byteorder == "little":
            dwords = struct.pack("!BBBBBBBBBBBBBBBB", byte_arr[3], byte_arr[2], byte_arr[1], byte_arr[0], byte_arr[7],
                                 byte_arr[6], byte_arr[5], byte_arr[4], byte_arr[11], byte_arr[10], byte_arr[9],
                                 byte_arr[8], byte_arr[15], byte_arr[14], byte_arr[13], byte_arr[12])
        else:
            dwords = struct.pack("!BBBBBBBBBBBBBBBB", byte_arr[0], byte_arr[1], byte_arr[2], byte_arr[3], byte_arr[4],
                                 byte_arr[5], byte_arr[6], byte_arr[7], byte_arr[8], byte_arr[9], byte_arr[10],
                                 byte_arr[11], byte_arr[12], byte_arr[13], byte_arr[14], byte_arr[15])
        return socket.inet_ntop(socket.AF_INET6, dwords)
