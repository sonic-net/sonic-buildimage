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

import json, jsonschema
import argparse
import ipaddress
import re
import subprocess
import pprint
import os
import sys

usr_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
lib_path = os.path.join(usr_path, "lib")
ecmp_lib_path = os.path.join(lib_path, "ecmp_calc")
sys.path.append(lib_path)
sys.path.append(ecmp_lib_path)

from ecmp_calc_sdk import sx_open_sdk_connection, sx_get_active_vrids, sx_router_get_ecmp_id, \
                                              sx_router_ecmp_nexthops_get, sx_get_router_interface, \
                                              sx_port_vport_base_get, sx_router_neigh_get_mac, sx_fdb_uc_mac_addr_get, \
                                              sx_lag_port_group_get, sx_make_ip_prefix_v4, sx_make_ip_prefix_v6, \
                                              sx_vlan_ports_get, sx_ip_addr_to_str, sx_close_sdk_connection, \
                                              PORT, VPORT, VLAN, SX_ENTRY_NOT_FOUND
from packet_scheme import PACKET_SCHEME
from port_utils import sx_get_ports_map, is_lag
from swsscommon.swsscommon import ConfigDBConnector, DBConnector, Table

IP_VERSION_IPV4 = 1
IP_VERSION_IPV6 = 2
PORT_CHANNEL_IDX = 0
VRF_NAME_IDX = 1
IP_VERSION_MAX_MASK_LEN = {IP_VERSION_IPV4: 32, IP_VERSION_IPV6: 128}

APPL_DB_NAME = 'APPL_DB'
INTF_TABLE = 'INTF_TABLE'
VRF_TABLE = 'VRF_TABLE'
LAG_MEMBER_TABLE = 'LAG_MEMBER_TABLE'
HASH_CALC_PATH = '/usr/bin/sx_hash_calculator'
HASH_CALC_INPUT_FILE = "/tmp/hash_calculator_input.json"
HASH_CALC_OUTPUT_FILE = "/tmp/hash_calculator_output.json"

def exec_cmd(cmd):
    """ Execute shell command """
    return subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=False).decode("utf-8")

def is_mac_valid(mac):
    return bool(re.match("^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$", mac))

def is_ip_valid(address, ip_version):
    try:
        if ip_version == IP_VERSION_IPV4:
            ip = ipaddress.IPv4Address(address)
            invalid_list = ['0.0.0.0','255.255.255.255']            
        else:
            ip = ipaddress.IPv6Address(address)
            invalid_list = ['0::0']

            if ip.is_link_local:
                print ("Link local IP {} is not valid".format(ip))
                return False

        if ip in invalid_list:
            print ("IP {} is not valid".format(ip))
            return False            

        if ip.is_multicast:
            print ("Multicast IP {} is not valid".format(ip))
            return False

        if ip.is_loopback:
            print ("Loopback IP {} is not valid".format(ip))
            return False

    except ipaddress.AddressValueError:
        return False
    
    return True

def load_json(filename):
    data = None
    with open(filename) as f:
        try:
            data = json.load(f)
        except json.JSONDecodeError as e:
            raise ValueError("Failed to load JSON file '{}', error: '{}'".format(filename, e))
    return data

def create_network_addr(ip_addr, mask_len, ip_version):
    ip_addr_mask = "{}/{}".format(ip_addr, mask_len)

    if ip_version == IP_VERSION_IPV4:
        network_addr = ipaddress.IPv4Network(ip_addr_mask,strict = False)
    else:
        network_addr = ipaddress.IPv6Network(ip_addr_mask,strict = False)

    network_addr_ip = network_addr.with_netmask.split('/')[0]
    network_addr_mask = network_addr.with_netmask.split('/')[1]
    
    if ip_version == IP_VERSION_IPV4:
        network_addr = sx_make_ip_prefix_v4(network_addr_ip, network_addr_mask)
    else:
        network_addr = sx_make_ip_prefix_v6(network_addr_ip, network_addr_mask)
    
    return network_addr

class EcmpCalcExit(Exception):
    pass

class EcmpCalc:
    def __init__(self):
        self.packet = {}
        self.ports_map = {}
        self.ecmp_ids = {}
        self.next_hops = {}
        self.user_vrf = ''
        self.ingress_port = ""
        self.egress_ports = []
        self.debug = False
        
        self.config_db = ConfigDBConnector()
        self.appl_db = DBConnector(APPL_DB_NAME, 0)
        self.open_sdk_connection()
        self.init_ports_map()
        self.get_active_vrids()
        
    def __del__(self):
        self.close_sdk_connection()
        self.cleanup()

    def cleanup(self):
        for filename in [HASH_CALC_INPUT_FILE, HASH_CALC_OUTPUT_FILE]:
            if os.path.exists(filename):
                os.remove(filename)

    def close_sdk_connection(self):
        sx_close_sdk_connection(self.handle)
        
    def open_sdk_connection(self):
        self.handle = sx_open_sdk_connection()
        
    def debug_print(self, *args, **kwargs):
        if self.debug == True:
            print(*args, **kwargs)

    def init_ports_map(self):
        self.ports_map = sx_get_ports_map(self.handle, self.config_db)
        
    def validate_ingress_port(self, interface):
        if interface not in self.ports_map.values():
            raise ValueError("Invalid interface {}".format(interface))   
        self.ingress_port = interface    

    def validate_args(self, interface, packet, vrf, debug):
        if (debug is True):
            self.debug = True
            
        self.validate_ingress_port(interface)
        self.validate_packet_json(packet)
                
        if (vrf is not None):
            self.user_vrf = vrf
            if not self.validate_vrf():
                raise ValueError("VRF validation failed: VRF {} does not exist".format(self.user_vrf))
                        
    def validate_vrf(self):
        vrf_table = Table(self.appl_db, VRF_TABLE)
        vrf_table_keys = vrf_table.getKeys()

        if self.user_vrf in vrf_table_keys:
            return True

        return False

    def get_ecmp_id(self):
        ip_addr = self.dst_ip
        ip_version = self.ip_version
        max_mask_len = IP_VERSION_MAX_MASK_LEN[self.ip_version]
        route_found = False
        
        for vrid in self.vrid_list:
            for mask_len in range(max_mask_len, 0, -1):
                network_addr = create_network_addr(ip_addr, mask_len, ip_version)
                ecmp_id = sx_router_get_ecmp_id(self.handle, vrid, network_addr)
                if ecmp_id != SX_ENTRY_NOT_FOUND:
                    route_found = True
                    self.debug_print("Found route for destination IP {} ECMP id {} VRID {}".format(self.dst_ip, ecmp_id, vrid))
                    self.ecmp_ids[vrid] = ecmp_id
                    
                    # move to next vrid
                    break
                
        if not route_found:
            raise EcmpCalcExit("No route found for given packet")
    
    def get_next_hops(self):
        next_hops = []
        ecmp_found = False
        
        for vrid in self.ecmp_ids.keys():
            ecmp_id = self.ecmp_ids[vrid]
            next_hops = sx_router_ecmp_nexthops_get(self.handle, ecmp_id)
                        
            if len(next_hops) > 1:
                if self.debug:
                    next_hops_ips = []
                    for nh in next_hops:
                        ip = nh.next_hop_key.next_hop_key_entry.ip_next_hop.address
                        next_hops_ips.append(sx_ip_addr_to_str(ip))
                    print("Next hops IPs {}, VRID {}".format(next_hops_ips, vrid))
                    print("Found ECMP for destination IP {} ECMP id {}, now checking if port is member in VRF {}". 
                          format(self.dst_ip, ecmp_id, 'default' if self.user_vrf=='' else self.user_vrf))                    
                
                self.next_hops[vrid] = next_hops
                ecmp_found = True
                
        if not ecmp_found:
            raise EcmpCalcExit("No ECMP for given packet")
            
    def calculate_egress_port(self):    
        for vrid in self.vrid_list:
            if vrid not in self.next_hops.keys():
                continue

            next_hops = self.next_hops[vrid]
            next_hop_idx = self.get_next_hop_index(len(next_hops))
            next_hop = next_hops[next_hop_idx]
            
            rif = next_hop.next_hop_key.next_hop_key_entry.ip_next_hop.rif
            ip = next_hop.next_hop_key.next_hop_key_entry.ip_next_hop.address
            rif_params = sx_get_router_interface(self.handle, vrid, rif)
            
            self.debug_print("Next hop ip to which trafic will egress: {}".format(sx_ip_addr_to_str(ip)))

            # Handle router port
            if PORT in rif_params:
                logical = rif_params[PORT]
                port_type = PORT
                vlan_id = 0
              
            # Handle vlan subinterface
            elif VPORT in rif_params:
                logical, vlan_id = sx_port_vport_base_get(self.handle, rif_params[VPORT])
                port_type = VPORT
                
            # Handle vlan interface               
            elif VLAN in rif_params:
                vlan_id = rif_params[VLAN]
                neigh_mac = sx_router_neigh_get_mac(self.handle, rif, ip)
                if neigh_mac is not None:
                    mac_entry = sx_fdb_uc_mac_addr_get(self.handle, vlan_id, neigh_mac)
                    if mac_entry is not None:
                        logical = mac_entry.log_port
                        port_type = VLAN
                   
                # Handle flood case
                if (neigh_mac is None) or (mac_entry is None):
                    vlan_members = sx_vlan_ports_get(self.handle, rif_params[VLAN])
                    for port in vlan_members:
                        if is_lag(port):
                            port = self.get_lag_member(port, True)
                        self.egress_ports.append(self.ports_map[port])
                    return

            # Check if port is binded to VRF we got from the user
            if is_lag(logical):
                lag_logical = logical
                logical = self.get_lag_member(lag_logical)
                egress_port = self.ports_map[logical]
                
                port_channel = self.get_port_channel_name(egress_port)
                if self.is_port_bind_to_user_vrf(port_type, port_channel, vlan_id):
                    self.egress_ports.append(egress_port)
                    return
            else:
                egress_port = self.ports_map[logical]
                if self.is_port_bind_to_user_vrf(port_type, egress_port, vlan_id):
                    self.egress_ports.append(egress_port)
                    return

    def print_egress_port(self):
        if len(self.egress_ports) == 0:
            print("Egress port not found, check input parameters")
        elif len(self.egress_ports) == 1:
            print("Egress port: {}".format(self.egress_ports[0]))
        else:
            egress_ports = ''
            for port in self.egress_ports:
                egress_ports += ' ' + port
            print("Egress ports:{}".format(egress_ports))

    def is_port_bind_to_user_vrf(self, port_type, port, vlan_id = 0):
        if port_type == PORT:
            # INTF_TABLE:Ethernet0
            entry = '{}'.format(port)
        elif port_type == VPORT:
            # INTF_TABLE:Ethernet0.300
            entry = '{}.{}'.format(port, vlan_id)
        elif port_type == VLAN:
            # INTF_TABLE:Vlan300
            entry = 'Vlan{}'.format(vlan_id)

        vrf_table = Table(self.appl_db, INTF_TABLE)
        (_, port_vrf) = vrf_table.hget(entry, 'vrf_name')

        if self.user_vrf == port_vrf.strip():
            return True
        
        return False
    
    # Get port-channel name for given port-channel member port
    def get_port_channel_name(self, port):
        lag_member_table = Table(self.appl_db, LAG_MEMBER_TABLE)
        lag_member_table_keys = lag_member_table.getKeys()

        for key in lag_member_table_keys:
            if port in key:
                port_channel = key.split(':')[PORT_CHANNEL_IDX]
                return port_channel
        
        raise KeyError("Failed to get port-channel name for interface {}".format(port))

    def get_ingress_port_logical_idx(self):
        for logical_index, sonic_port_name in self.ports_map.items():
            if sonic_port_name == self.ingress_port:
                return logical_index

        raise KeyError("Failed to get logical index for interface {}".format(self.ingress_port))

    # Get index in next hop array from which packet will egress 
    def get_next_hop_index(self, ecmp_size):
        logical = self.get_ingress_port_logical_idx()
        
        ecmp_hash = {
            "ingress_port": str(hex(logical)),
            "packet_info":self.packet['packet_info'],
            "ecmp_size": ecmp_size,
        }    
            
        self.debug_print("Calling hash calculator for ECMP")    
        hash_result = self.call_hash_calculator({'ecmp_hash': ecmp_hash})
        ecmp_hash_result = hash_result['ecmp_hash']
        index = ecmp_hash_result['ecmp_index']  
        
        self.debug_print("Next hop index to which trafic will egress: {}".format(index))
        return index  

    # Get index in LAG memebrs array from which packet will egress
    def get_lag_member_index(self, lag_size, flood_case = False):
        logical = self.get_ingress_port_logical_idx()

        lag_hash = {
            "ingress_port": str(hex(logical)),
            "packet_info": self.packet['packet_info'],
            "lag_size": lag_size,
        }
    
        self.debug_print("Calling hash calculator for LAG, flood case {}".format(True if flood_case else False))
        hash_result = self.call_hash_calculator({"lag_hash": lag_hash})
        lag_hash_result = hash_result["lag_hash"]
        
        if flood_case:
            index = lag_hash_result['lag_mc_index']
        else:
            index = lag_hash_result['lag_index']

        self.debug_print("Lag member index from which trafic will egress: {}".format(index))
        return index
        
    # Get LAG memebr from which packet will egress
    def get_lag_member(self, logical, flood_case = False):
        lag_members = sx_lag_port_group_get(self.handle, logical)
        lag_members.sort()
        
        member_index = self.get_lag_member_index(len(lag_members), flood_case)
        lag_member = lag_members[member_index]
        
        self.debug_print("Lag members: {}\nLag member from which trafic will egress: {}".format(lag_members, lag_member))
        return lag_member
        
    def call_hash_calculator(self, input_dict):
        with open(HASH_CALC_INPUT_FILE, "w") as outfile:
            json.dump(input_dict, outfile)
        
        out = exec_cmd([HASH_CALC_PATH, '-c', HASH_CALC_INPUT_FILE, '-o', HASH_CALC_OUTPUT_FILE, '-d'])
        self.debug_print ("Hash calculator output:\n{}".format(out))
    
        with open(HASH_CALC_OUTPUT_FILE, 'r') as openfile:
            output_dict = json.loads(openfile.read())
            
        return output_dict
                    
    def get_active_vrids(self):
        self.vrid_list = sx_get_active_vrids(self.handle)
        
    def validate_ipv4_header(self, header):
        for ip in ['sip', 'dip']:
            if ip in header and is_ip_valid(header[ip], IP_VERSION_IPV4) == False:
                raise ValueError("Json validation failed: invalid IP {}".format(header[ip]))

    def validate_ipv6_header(self, header):
        for ip in ['sip', 'dip']:
            if ip in header and is_ip_valid(header[ip], IP_VERSION_IPV6) == False:
                raise ValueError("Json validation failed: invalid IP {}".format(header[ip]))
            
    def validate_layer2_header(self, header):
        for mac in ['smac', 'dmac']:
            if mac in header and is_mac_valid(header[mac]) == False:
                raise ValueError("Json validation failed: invalid mac {}".format(header[mac]))        
        
    def validate_header(self, header, is_outer_header=False):
        ipv4_header = False
        ipv6_header = False
                
        # Verify IPv4 and IPv6 headers do not co-exist in header
        if 'ipv4' in header:
            ipv4_header = True
        if 'ipv6' in header:
            ipv6_header = True
        
        if ipv4_header and ipv6_header:
            raise ValueError("Json validation failed: IPv4 and IPv6 headers can not co-exist")

        if ipv4_header:
            # Verify valid IPs in header
            self.validate_ipv4_header(header['ipv4'])

            if is_outer_header:
                if 'dip' not in header['ipv4']:
                    raise ValueError("Json validation failed: destination IP is mandatory")

                self.dst_ip = header['ipv4']['dip']
                self.ip_version = IP_VERSION_IPV4

            if 'tcp_udp' in header and 'proto' not in header['ipv4']:
                raise ValueError("Json validation failed: transport protocol (proto) is mandatory when transport layer port exists")

        elif ipv6_header:
            self.validate_ipv6_header(header['ipv6'])

            if is_outer_header:
                if 'dip' not in header['ipv6']:
                    raise ValueError("Json validation failed: destination IP is mandatory")

                self.dst_ip = header['ipv6']['dip']
                self.ip_version = IP_VERSION_IPV6

            if 'tcp_udp' in header and 'next_header' not in header['ipv6']:
                raise ValueError("Json validation failed: transport protocol (next_header) is mandatory when transport layer port exists")

        # Verify valid macs in header
        if header['layer2']:
            self.validate_layer2_header(header['layer2'])    
                   
    def validate_outer_header(self):
        outer_header = self.packet['packet_info'].get('outer')
        if not outer_header:
            raise ValueError("Json validation failed: outer header is mandatory")

        self.validate_header(outer_header, is_outer_header=True)    
        
    def validate_inner_header(self):        
        inner_header = self.packet['packet_info'].get('inner')
        if not inner_header:
            return
        
        self.validate_header(inner_header) 
        
    def validate_packet_json(self, packet_json):
        # Verify json has valid format              
        self.packet = load_json(packet_json)
        
        # Verify json schema
        try:
            jsonschema.validate(self.packet, PACKET_SCHEME)
        except jsonschema.exceptions.ValidationError as e:
            raise ValueError("Json validation failed: {}".format(e))
                
        # Verify outer header
        self.validate_outer_header()

        # Verify inner header
        self.validate_inner_header()
        
        if self.debug:
            print('Packet:')
            pprint.pprint(self.packet)
               
def main():
    rc = 0
    try:
        parser = argparse.ArgumentParser(description="Calculate egress interface for the given packet being routed over ECMP",
                                         usage="/usr/bin/ecmp_calc.py -i <ingress interface> -p <path to json>")
        parser.add_argument("-i", "--interface", required=True, help="ingress interface")
        parser.add_argument("-p", "--packet", required=True, help="json file describing a packet")
        parser.add_argument("-v", "--vrf", help="VRF name")
        parser.add_argument("-d", "--debug", default=False, action="store_true", help="when used, debug messages will be printed to stdout")
        args = parser.parse_args()
        
        ecmp_calc = EcmpCalc()
        ecmp_calc.validate_args(args.interface, args.packet, args.vrf, args.debug)
        ecmp_calc.get_ecmp_id()
        ecmp_calc.get_next_hops()
        ecmp_calc.calculate_egress_port()
        ecmp_calc.print_egress_port()
        
    except EcmpCalcExit as s:
        print(s)   
    except ValueError as s:
        print("Value error: {}".format(s))
        rc = 1
    except Exception as s:
        print("Error: {}".format(s))
        rc = 2
    return rc

if __name__ == "__main__":
    sys.exit(main())
