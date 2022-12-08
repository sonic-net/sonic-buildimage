#!/usr/bin/env python3

from python_sdk_api.sx_api import *
import inspect

DEVICE_ID = 1
SWITCH_ID = 0
ETHERNET_PREFIX = 'Ethernet'
ASIC_MAX_LANES = {SX_CHIP_TYPE_SPECTRUM: 4, SX_CHIP_TYPE_SPECTRUM2: 4, 
                  SX_CHIP_TYPE_SPECTRUM3: 8, SX_CHIP_TYPE_SPECTRUM4: 8}

def sx_get_ports_map(handle):
    """ Get ports map from SDK logical index to SONiC index
    
    Args:
        handle (sx_api_handle_t): SDK handle
    
    Returns:
        dict : Dictionary of ports indices. Key is SDK logical index, value is SONiC index (4 for Ethernet4)
    """         
    try:
        ports_map = {}
        
        # Get chip type
        chip_type = sx_get_chip_type(handle)
        
        # Get ports count
        port_cnt_p = new_uint32_t_p()
        rc = sx_api_port_device_get(handle, DEVICE_ID, SWITCH_ID, None,  port_cnt_p)
        sx_check_rc(rc)
        
        # Get ports
        port_cnt = uint32_t_p_value(port_cnt_p)
        port_attributes_list = new_sx_port_attributes_t_arr(port_cnt)
        rc = sx_api_port_device_get(handle, DEVICE_ID, SWITCH_ID, port_attributes_list,  port_cnt_p)
        sx_check_rc(rc)
        
        for i in range(0, port_cnt):
            port_attributes = sx_port_attributes_t_arr_getitem(port_attributes_list, i)
            label_port = port_attributes.port_mapping.module_port
            logical_port = port_attributes.log_port;
            lane_bmap = port_attributes.port_mapping.lane_bmap;
            
            if (is_phy_port(port_attributes.log_port) == False):
                continue
            
            # Calculate sonic index (sonic index=4 for Ethernet4)
            lane_index = get_lane_index(lane_bmap, ASIC_MAX_LANES[chip_type])
            assert lane_index != -1, "Failed to calculate port index"
            
            sonic_index = label_port * ASIC_MAX_LANES[chip_type] + lane_index;
            sonic_interface = ETHERNET_PREFIX + str(sonic_index)    
            ports_map[logical_port] = sonic_interface
            
        return ports_map
    
    finally:
        delete_sx_port_attributes_t_arr(port_attributes_list)
        delete_uint32_t_p(port_cnt_p)
        
def sx_get_chip_type(handle):
    """ Get system ASIC type
    
    Args:
        handle (sx_api_handle_t): SDK handle
    
    Returns:
        sx_chip_types_t : Chip type
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
       int : index of the first bit set to 1 in lane_bmap
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

