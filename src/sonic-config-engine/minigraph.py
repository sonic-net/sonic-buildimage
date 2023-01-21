from __future__ import print_function

import ipaddress
import math
import os
import sys
import json
from collections import defaultdict

from lxml import etree as ET
from lxml.etree import QName

from natsort import natsorted, ns as natsortns

from portconfig import get_port_config
from sonic_py_common.interface import backplane_prefix

# TODO: Remove this once we no longer support Python 2
if sys.version_info.major == 3:
    UNICODE_TYPE = str
else:
    UNICODE_TYPE = unicode

"""minigraph.py
version_added: "1.9"
author: Guohan Lu (gulv@microsoft.com)
short_description: Parse minigraph xml file and device description xml file
"""

ns = "Microsoft.Search.Autopilot.Evolution"
ns1 = "http://schemas.datacontract.org/2004/07/Microsoft.Search.Autopilot.Evolution"
ns2 = "Microsoft.Search.Autopilot.NetMux"
ns3 = "http://www.w3.org/2001/XMLSchema-instance"

# Device types
spine_chassis_frontend_role = 'SpineChassisFrontendRouter'
chassis_backend_role = 'ChassisBackendRouter'

backend_device_types = ['BackEndToRRouter', 'BackEndLeafRouter']
console_device_types = ['MgmtTsToR']
backend_hwskus = ['Arista-7050QX32S-Q32', 'Arista-7050-QX-32S', 'Arista-7050-QX32']
dhcp_server_enabled_device_types = ['BmcMgmtToRRouter']
VLAN_SUB_INTERFACE_SEPARATOR = '.'
VLAN_SUB_INTERFACE_VLAN_ID = '10'

FRONTEND_ASIC_SUB_ROLE = 'FrontEnd'
BACKEND_ASIC_SUB_ROLE = 'BackEnd'

dualtor_cable_types = ["active-active", "active-standby"]

# Default Virtual Network Index (VNI) 
vni_default = 8000

###############################################################################
#
# Minigraph parsing functions
#
###############################################################################

class minigraph_encoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, (
            ipaddress.IPv4Network, ipaddress.IPv6Network,
            ipaddress.IPv4Address, ipaddress.IPv6Address
            )):
            return str(obj)
        return json.JSONEncoder.default(self, obj)

def get_peer_switch_info(link_metadata, devices):
    peer_switch_table = {}
    peer_switch_ip = None
    mux_tunnel_name = None
    for port, data in link_metadata.items():
        if "PeerSwitch" in data:
            peer_hostname = data["PeerSwitch"]
            peer_lo_addr_str = devices[peer_hostname]["lo_addr"]
            peer_lo_addr = ipaddress.ip_network(UNICODE_TYPE(peer_lo_addr_str)) if peer_lo_addr_str else None

            peer_switch_table[peer_hostname] = {
                'address_ipv4': str(peer_lo_addr.network_address) if peer_lo_addr else peer_lo_addr_str
            }
            mux_tunnel_name = port
            peer_switch_ip = peer_switch_table[peer_hostname]['address_ipv4']

    return peer_switch_table, mux_tunnel_name, peer_switch_ip


def parse_device(device):
    lo_prefix = None
    lo_prefix_v6 = None
    mgmt_prefix = None
    mgmt_prefix_v6 = None
    d_type = None   # don't shadow type()
    hwsku = None
    name = None
    deployment_id = None
    cluster = None
    d_subtype = None

    for node in device:
        if node.tag == str(QName(ns, "Address")):
            lo_prefix = node.find(str(QName(ns2, "IPPrefix"))).text
        elif node.tag == str(QName(ns, "AddressV6")):
            lo_prefix_v6 = node.find(str(QName(ns2, "IPPrefix"))).text
        elif node.tag == str(QName(ns, "ManagementAddress")):
            mgmt_prefix = node.find(str(QName(ns2, "IPPrefix"))).text
        elif node.tag == str(QName(ns, "ManagementAddressV6")):
            mgmt_prefix_v6 = node.find(str(QName(ns2, "IPPrefix"))).text
        elif node.tag == str(QName(ns, "Hostname")):
            name = node.text
        elif node.tag == str(QName(ns, "HwSku")):
            hwsku = node.text
        elif node.tag == str(QName(ns, "DeploymentId")):
            deployment_id = node.text
        elif node.tag == str(QName(ns, "ElementType")):
            d_type = node.text
        elif node.tag == str(QName(ns, "ClusterName")):
            cluster = node.text
        elif node.tag == str(QName(ns, "SubType")):
            d_subtype = node.text

    if d_type is None and str(QName(ns3, "type")) in device.attrib:
        d_type = device.attrib[str(QName(ns3, "type"))]

    return (lo_prefix, lo_prefix_v6, mgmt_prefix, mgmt_prefix_v6, name, hwsku, d_type, deployment_id, cluster, d_subtype)


def calculate_lcm_for_ecmp (nhdevices_bank_map, nhip_bank_map):
    banks_enumerated = {}
    lcm_array = []
    for value in nhdevices_bank_map.values():
        for key in nhip_bank_map.keys():
            if nhip_bank_map[key] == value:
                if value not in banks_enumerated:
                    banks_enumerated[value] = 1
                else:
                    banks_enumerated[value] = banks_enumerated[value] + 1
    for bank_enumeration in banks_enumerated.values():
        lcm_list = range(1, bank_enumeration+1)
        lcm_comp = lcm_list[0]
        for i in lcm_list[1:]:
            lcm_comp = lcm_comp * i / calculate_gcd(lcm_comp, i)
        lcm_array.append(lcm_comp)

    LCM = sum(lcm_array)
    return int(LCM)

def calculate_gcd(x, y):
    while y != 0:
        (x, y) = (y, x % y)
    return int(x)

def formulate_fine_grained_ecmp(version, dpg_ecmp_content, port_device_map, port_alias_map):
    family = ""
    tag = ""
    neigh_key = []
    if version == "ipv4":
        family = "IPV4"
        tag = "fgnhg_v4"
    elif version == "ipv6":
        family = "IPV6"
        tag = "fgnhg_v6"

    port_nhip_map = dpg_ecmp_content['port_nhip_map']
    nhg_int = dpg_ecmp_content['nhg_int']

    nhip_device_map = {port_nhip_map[x]: port_device_map[x] for x in port_device_map
                         if x in port_nhip_map}
    nhip_devices = sorted(list(set(nhip_device_map.values())))
    nhdevices_ip_bank_map = {device: bank for bank, device in enumerate(nhip_devices)}
    nhip_bank_map = {ip: nhdevices_ip_bank_map[device] for ip, device in nhip_device_map.items()}
    LCM = calculate_lcm_for_ecmp(nhdevices_ip_bank_map, nhip_bank_map)

    FG_NHG_MEMBER = {ip: {"FG_NHG": tag, "bank": bank} for ip, bank in nhip_bank_map.items()}
    nhip_port_map = dict(zip(port_nhip_map.values(), port_nhip_map.keys()))

    for nhip, memberinfo in FG_NHG_MEMBER.items():
        if nhip in nhip_port_map:
            memberinfo["link"] = port_alias_map[nhip_port_map[nhip]]
            FG_NHG_MEMBER[nhip] = memberinfo

    FG_NHG = {tag: {"bucket_size": LCM, "match_mode": "nexthop-based"}}
    for ip in nhip_bank_map:
        neigh_key.append(str(nhg_int + "|" + ip))
    NEIGH = {neigh_key: {"family": family} for neigh_key in neigh_key}

    fine_grained_content = {"FG_NHG_MEMBER": FG_NHG_MEMBER, "FG_NHG": FG_NHG, "NEIGH": NEIGH}
    return fine_grained_content


def parse_png(png, hname, dpg_ecmp_content = None):
    neighbors = {}
    devices = {}
    console_dev = ''
    console_port = ''
    mgmt_dev = ''
    mgmt_port = ''
    port_speeds = {}
    console_ports = {}
    mux_cable_ports = {}
    port_device_map = {}
    png_ecmp_content = {}
    FG_NHG_MEMBER = {}
    FG_NHG = {}
    NEIGH = {}

    for child in png:
        if child.tag == str(QName(ns, "DeviceInterfaceLinks")):
            for link in child.findall(str(QName(ns, "DeviceLinkBase"))):
                linktype = link.find(str(QName(ns, "ElementType"))).text
                if linktype == "DeviceSerialLink":
                    enddevice = link.find(str(QName(ns, "EndDevice"))).text
                    endport = link.find(str(QName(ns, "EndPort"))).text
                    startdevice = link.find(str(QName(ns, "StartDevice"))).text
                    startport = link.find(str(QName(ns, "StartPort"))).text
                    baudrate = link.find(str(QName(ns, "Bandwidth"))).text
                    flowcontrol = 1 if link.find(str(QName(ns, "FlowControl"))) is not None and link.find(str(QName(ns, "FlowControl"))).text == 'true' else 0
                    if enddevice.lower() == hname.lower() and endport.isdigit():
                        console_ports[endport] = {
                            'remote_device': startdevice,
                            'baud_rate': baudrate,
                            'flow_control': flowcontrol
                        }
                    elif startport.isdigit():
                        console_ports[startport] = {
                            'remote_device': enddevice,
                            'baud_rate': baudrate,
                            'flow_control': flowcontrol
                        }
                    continue

                if linktype == "DeviceInterfaceLink":
                    endport = link.find(str(QName(ns, "EndPort"))).text
                    startdevice = link.find(str(QName(ns, "StartDevice"))).text
                    port_device_map[endport] = startdevice

                if linktype != "DeviceInterfaceLink" and linktype != "UnderlayInterfaceLink" and linktype != "DeviceMgmtLink":
                    continue

                enddevice = link.find(str(QName(ns, "EndDevice"))).text
                endport = link.find(str(QName(ns, "EndPort"))).text
                startdevice = link.find(str(QName(ns, "StartDevice"))).text
                startport = link.find(str(QName(ns, "StartPort"))).text
                bandwidth_node = link.find(str(QName(ns, "Bandwidth")))
                bandwidth = bandwidth_node.text if bandwidth_node is not None else None
                if enddevice.lower() == hname.lower():
                    if endport in port_alias_map:
                        endport = port_alias_map[endport]
                    if linktype != "DeviceMgmtLink":
                        neighbors[endport] = {'name': startdevice, 'port': startport}
                    if bandwidth:
                        port_speeds[endport] = bandwidth
                elif startdevice.lower() == hname.lower():
                    if startport in port_alias_map:
                        startport = port_alias_map[startport]
                    if linktype != "DeviceMgmtLink":
                        neighbors[startport] = {'name': enddevice, 'port': endport}
                    if bandwidth:
                        port_speeds[startport] = bandwidth

        if child.tag == str(QName(ns, "Devices")):
            for device in child.findall(str(QName(ns, "Device"))):
                (lo_prefix, lo_prefix_v6, mgmt_prefix, mgmt_prefix_v6, name, hwsku, d_type, deployment_id, cluster, d_subtype) = parse_device(device)
                device_data = {}
                if hwsku != None:
                    device_data['hwsku'] = hwsku
                if cluster != None:
                    device_data['cluster'] = cluster
                if deployment_id != None:
                    device_data['deployment_id'] = deployment_id
                if lo_prefix != None:
                    device_data['lo_addr'] = lo_prefix
                if lo_prefix_v6 != None:
                    device_data['lo_addr_v6'] = lo_prefix_v6
                if mgmt_prefix != None:
                    device_data['mgmt_addr'] = mgmt_prefix
                if mgmt_prefix_v6 != None:
                    device_data['mgmt_addr_v6'] = mgmt_prefix_v6
                if d_type != None:
                    device_data['type'] = d_type
                if d_subtype != None:
                    device_data['subtype'] = d_subtype
                devices[name] = device_data

        if child.tag == str(QName(ns, "DeviceInterfaceLinks")):
            for if_link in child.findall(str(QName(ns, 'DeviceLinkBase'))):
                if str(QName(ns3, "type")) in if_link.attrib:
                    link_type = if_link.attrib[str(QName(ns3, "type"))]
                    if link_type == 'DeviceSerialLink':
                        for node in if_link:
                            if node.tag == str(QName(ns, "EndPort")):
                                console_port = node.text.split()[-1]
                            elif node.tag == str(QName(ns, "EndDevice")):
                                console_dev = node.text
                    elif link_type == 'DeviceMgmtLink':
                        for node in if_link:
                            if node.tag == str(QName(ns, "EndPort")):
                                mgmt_port = node.text.split()[-1]
                            elif node.tag == str(QName(ns, "EndDevice")):
                                mgmt_dev = node.text


        if child.tag == str(QName(ns, "DeviceInterfaceLinks")):
            for link in child.findall(str(QName(ns, 'DeviceLinkBase'))):
                if link.find(str(QName(ns, "ElementType"))).text == "LogicalLink":
                    intf_name = link.find(str(QName(ns, "EndPort"))).text
                    start_device = link.find(str(QName(ns, "StartDevice"))).text
                    if intf_name in port_alias_map:
                        intf_name = port_alias_map[intf_name]

                    mux_cable_ports[intf_name] = start_device

        if dpg_ecmp_content and (len(dpg_ecmp_content)):
            for version, content in dpg_ecmp_content.items():  # version is ipv4 or ipv6
                fine_grained_content = formulate_fine_grained_ecmp(version, content, port_device_map, port_alias_map)  # port_alias_map
                FG_NHG_MEMBER.update(fine_grained_content['FG_NHG_MEMBER'])
                FG_NHG.update(fine_grained_content['FG_NHG'])
                NEIGH.update(fine_grained_content['NEIGH'])

            png_ecmp_content = {"FG_NHG_MEMBER": FG_NHG_MEMBER, "FG_NHG": FG_NHG, "NEIGH": NEIGH}

    return (neighbors, devices, console_dev, console_port, mgmt_dev, mgmt_port, port_speeds, console_ports, mux_cable_ports, png_ecmp_content)


def parse_asic_external_link(link, asic_name, hostname):
    neighbors = {}
    port_speeds = {}
    enddevice = link.find(str(QName(ns, "EndDevice"))).text
    endport = link.find(str(QName(ns, "EndPort"))).text
    startdevice = link.find(str(QName(ns, "StartDevice"))).text
    startport = link.find(str(QName(ns, "StartPort"))).text
    bandwidth_node = link.find(str(QName(ns, "Bandwidth")))
    bandwidth = bandwidth_node.text if bandwidth_node is not None else None
    # if chassis internal is false, the interface name will be
    # interface alias which should be converted to asic port name
    if (enddevice.lower() == hostname.lower()):
        if ((endport in port_alias_asic_map) and
                (asic_name.lower() in port_alias_asic_map[endport].lower())):
            endport = port_alias_asic_map[endport]
            neighbors[port_alias_map[endport]] = {'name': startdevice, 'port': startport}
            if bandwidth:
                port_speeds[port_alias_map[endport]] = bandwidth
    elif (startdevice.lower() == hostname.lower()):
        if ((startport in port_alias_asic_map) and
                (asic_name.lower() in port_alias_asic_map[startport].lower())):
            startport = port_alias_asic_map[startport]
            neighbors[port_alias_map[startport]] = {'name': enddevice, 'port': endport}
            if bandwidth:
                port_speeds[port_alias_map[startport]] = bandwidth

    return neighbors, port_speeds

def parse_asic_internal_link(link, asic_name, hostname):
    neighbors = {}
    port_speeds = {}
    enddevice = link.find(str(QName(ns, "EndDevice"))).text
    endport = link.find(str(QName(ns, "EndPort"))).text
    startdevice = link.find(str(QName(ns, "StartDevice"))).text
    startport = link.find(str(QName(ns, "StartPort"))).text
    bandwidth_node = link.find(str(QName(ns, "Bandwidth")))
    bandwidth = bandwidth_node.text if bandwidth_node is not None else None
    if ((enddevice.lower() == asic_name.lower()) and
            (startdevice.lower() != hostname.lower())):
        if endport in port_alias_map:
            endport = port_alias_map[endport]
            neighbors[endport] = {'name': startdevice, 'port': startport}
            if bandwidth:
                port_speeds[endport] = bandwidth
    elif ((startdevice.lower() == asic_name.lower()) and
            (enddevice.lower() != hostname.lower())):
        if startport in port_alias_map:
            startport = port_alias_map[startport]
            neighbors[startport] = {'name': enddevice, 'port': endport}
            if bandwidth:
                port_speeds[startport] = bandwidth

    return neighbors, port_speeds

def parse_asic_png(png, asic_name, hostname):
    neighbors = {}
    devices = {}
    port_speeds = {}
    for child in png:
        if child.tag == str(QName(ns, "DeviceInterfaceLinks")):
            for link in child.findall(str(QName(ns, "DeviceLinkBase"))):
                # Chassis internal node is used in multi-asic device or chassis minigraph
                # where the minigraph will contain the internal asic connectivity and
                # external neighbor information. The ChassisInternal node will be used to
                # determine if the link is internal to the device or chassis.
                chassis_internal_node = link.find(str(QName(ns, "ChassisInternal")))
                chassis_internal = chassis_internal_node.text if chassis_internal_node is not None else "false"

                # If the link is an external link include the external neighbor
                # information in ASIC ports table
                if chassis_internal.lower() == "false":
                    ext_neighbors, ext_port_speeds = parse_asic_external_link(link, asic_name, hostname)
                    neighbors.update(ext_neighbors)
                    port_speeds.update(ext_port_speeds)
                else:
                    int_neighbors, int_port_speeds = parse_asic_internal_link(link, asic_name, hostname)
                    neighbors.update(int_neighbors)
                    port_speeds.update(int_port_speeds)

        if child.tag == str(QName(ns, "Devices")):
            for device in child.findall(str(QName(ns, "Device"))):
                (lo_prefix, lo_prefix_v6, mgmt_prefix, mgmt_prefix_v6, name, hwsku, d_type, deployment_id, cluster, _) = parse_device(device)
                device_data = {}
                if hwsku != None:
                    device_data['hwsku'] = hwsku
                if cluster != None:
                    device_data['cluster'] = cluster
                if deployment_id != None:
                    device_data['deployment_id'] = deployment_id
                if lo_prefix != None:
                    device_data['lo_addr'] = lo_prefix
                if lo_prefix_v6 != None:
                    device_data['lo_addr_v6'] = lo_prefix_v6
                if mgmt_prefix != None:
                    device_data['mgmt_addr'] = mgmt_prefix
                if mgmt_prefix_v6 != None:
                    device_data['mgmt_addr_v6'] = mgmt_prefix_v6
                if d_type != None:
                    device_data['type'] = d_type
                devices[name] = device_data

    return (neighbors, devices, port_speeds)


def parse_loopback_intf(child):
    lointfs = child.find(str(QName(ns, "LoopbackIPInterfaces")))
    lo_intfs = {}
    for lointf in lointfs.findall(str(QName(ns1, "LoopbackIPInterface"))):
        intfname = lointf.find(str(QName(ns, "AttachTo"))).text
        ipprefix = lointf.find(str(QName(ns1, "PrefixStr"))).text
        lo_intfs[(intfname, ipprefix)] = {}
    return lo_intfs


def parse_dpg(dpg, hname):
    aclintfs = None
    mgmtintfs = None
    subintfs = None
    tunnelintfs = defaultdict(dict)
    tunnelintfs_qos_remap_config = defaultdict(dict)

    for child in dpg:
        """
            In Multi-NPU platforms the acl intfs are defined only for the host not for individual asic.
            There is just one aclintf node in the minigraph
            Get the aclintfs node first.
        """
        if aclintfs is None and child.find(str(QName(ns, "AclInterfaces"))) is not None:
            aclintfs = child.find(str(QName(ns, "AclInterfaces")))
        """
            In Multi-NPU platforms the mgmt intfs are defined only for the host not for individual asic
            There is just one mgmtintf node in the minigraph
            Get the mgmtintfs node first. We need mgmt intf to get mgmt ip in per asic dockers.
        """
        if mgmtintfs is None and child.find(str(QName(ns, "ManagementIPInterfaces"))) is not None:
            mgmtintfs = child.find(str(QName(ns, "ManagementIPInterfaces")))
        hostname = child.find(str(QName(ns, "Hostname")))
        if hostname.text.lower() != hname.lower():
            continue

        vni = vni_default
        vni_element = child.find(str(QName(ns, "VNI")))
        if vni_element != None:
            if vni_element.text.isdigit():
                vni = int(vni_element.text)
            else:
                print("VNI must be an integer (use default VNI %d instead)" % vni_default, file=sys.stderr)

        ipintfs = child.find(str(QName(ns, "IPInterfaces")))
        intfs = {}
        ip_intfs_map = {}
        for ipintf in ipintfs.findall(str(QName(ns, "IPInterface"))):
            intfalias = ipintf.find(str(QName(ns, "AttachTo"))).text
            intfname = port_alias_map.get(intfalias, intfalias)
            ipprefix = ipintf.find(str(QName(ns, "Prefix"))).text
            intfs[(intfname, ipprefix)] = {}
            ip_intfs_map[ipprefix] = intfalias
        lo_intfs = parse_loopback_intf(child)

        subintfs = child.find(str(QName(ns, "SubInterfaces")))
        if subintfs is not None:
            for subintf in subintfs.findall(str(QName(ns, "SubInterface"))):
                intfalias = subintf.find(str(QName(ns, "AttachTo"))).text
                intfname = port_alias_map.get(intfalias, intfalias)
                ipprefix = subintf.find(str(QName(ns, "Prefix"))).text
                subintfvlan = subintf.find(str(QName(ns, "Vlan"))).text
                subintfname = intfname + VLAN_SUB_INTERFACE_SEPARATOR + subintfvlan
                intfs[(subintfname, ipprefix)] = {}

        mvrfConfigs = child.find(str(QName(ns, "MgmtVrfConfigs")))
        mvrf = {}
        if mvrfConfigs != None:
            mv = mvrfConfigs.find(str(QName(ns1, "MgmtVrfGlobal")))
            if mv != None:
                mvrf_en_flag = mv.find(str(QName(ns, "mgmtVrfEnabled"))).text
                mvrf["vrf_global"] = {"mgmtVrfEnabled": mvrf_en_flag}

        mgmt_intf = {}
        for mgmtintf in mgmtintfs.findall(str(QName(ns1, "ManagementIPInterface"))):
            intfname = mgmtintf.find(str(QName(ns, "AttachTo"))).text
            ipprefix = mgmtintf.find(str(QName(ns1, "PrefixStr"))).text
            mgmtipn = ipaddress.ip_network(UNICODE_TYPE(ipprefix), False)
            gwaddr = ipaddress.ip_address(next(mgmtipn.hosts()))
            mgmt_intf[(intfname, ipprefix)] = {'gwaddr': gwaddr}

        voqinbandintfs = child.find(str(QName(ns, "VoqInbandInterfaces")))
        voq_inband_intfs = {}
        if voqinbandintfs:
            for voqintf in voqinbandintfs.findall(str(QName(ns1, "VoqInbandInterface"))):
                intfname = voqintf.find(str(QName(ns, "Name"))).text
                intftype = voqintf.find(str(QName(ns, "Type"))).text
                ipprefix = voqintf.find(str(QName(ns1, "PrefixStr"))).text
                if intfname not in voq_inband_intfs:
                   voq_inband_intfs[intfname] = {'inband_type': intftype}
                voq_inband_intfs["%s|%s" % (intfname, ipprefix)] = {}

        pcintfs = child.find(str(QName(ns, "PortChannelInterfaces")))
        pc_intfs = []
        pcs = {}
        pc_members = {}
        intfs_inpc = [] # List to hold all the LAG member interfaces
        for pcintf in pcintfs.findall(str(QName(ns, "PortChannel"))):
            pcintfname = pcintf.find(str(QName(ns, "Name"))).text
            pcintfmbr = pcintf.find(str(QName(ns, "AttachTo"))).text
            pcmbr_list = pcintfmbr.split(';')
            pc_intfs.append(pcintfname)
            for i, member in enumerate(pcmbr_list):
                pcmbr_list[i] = port_alias_map.get(member, member)
                intfs_inpc.append(pcmbr_list[i])
                pc_members[(pcintfname, pcmbr_list[i])] = {}
            if pcintf.find(str(QName(ns, "Fallback"))) != None:
                pcs[pcintfname] = {'members': pcmbr_list, 'fallback': pcintf.find(str(QName(ns, "Fallback"))).text, 'min_links': str(int(math.ceil(len() * 0.75))), 'lacp_key': 'auto'}
            else:
                pcs[pcintfname] = {'members': pcmbr_list, 'min_links': str(int(math.ceil(len(pcmbr_list) * 0.75))), 'lacp_key': 'auto' }
        port_nhipv4_map = {}
        port_nhipv6_map = {}
        nhg_int = ""
        nhportlist = []
        dpg_ecmp_content = {}
        static_routes = {}
        ipnhs = child.find(str(QName(ns, "IPNextHops")))
        if ipnhs is not None:
            for ipnh in ipnhs.findall(str(QName(ns, "IPNextHop"))):
                if ipnh.find(str(QName(ns, "Type"))).text == 'FineGrainedECMPGroupMember':
                    ipnhfmbr = ipnh.find(str(QName(ns, "AttachTo"))).text
                    ipnhaddr = ipnh.find(str(QName(ns, "Address"))).text
                    nhportlist.append(ipnhfmbr)
                    if "." in ipnhaddr:
                        port_nhipv4_map[ipnhfmbr] = ipnhaddr
                    elif ":" in ipnhaddr:
                        port_nhipv6_map[ipnhfmbr] = ipnhaddr
                elif ipnh.find(str(QName(ns, "Type"))).text == 'StaticRoute':
                    prefix = ipnh.find(str(QName(ns, "AssociatedTo"))).text
                    ifname = ipnh.find(str(QName(ns, "AttachTo"))).text
                    nexthop = ipnh.find(str(QName(ns, "Address"))).text
                    advertise = ipnh.find(str(QName(ns, "Advertise"))).text
                    static_routes[prefix] = {'nexthop': nexthop, 'ifname': ifname, 'advertise': advertise}

            if port_nhipv4_map and port_nhipv6_map:
                subnet_check_ip = list(port_nhipv4_map.values())[0]
                for subnet_range in ip_intfs_map:
                    if ("." in subnet_range):
                        a = ipaddress.ip_address(UNICODE_TYPE(subnet_check_ip))
                        n = list(ipaddress.ip_network(UNICODE_TYPE(subnet_range), False).hosts())
                        if a in n:
                            nhg_int = ip_intfs_map[subnet_range]

                ipv4_content = {"port_nhip_map": port_nhipv4_map, "nhg_int": nhg_int}
                ipv6_content = {"port_nhip_map": port_nhipv6_map, "nhg_int": nhg_int}
                dpg_ecmp_content['ipv4'] = ipv4_content
                dpg_ecmp_content['ipv6'] = ipv6_content

        vlanintfs = child.find(str(QName(ns, "VlanInterfaces")))
        vlans = {}
        vlan_members = {}
        vlan_member_list = {}
        dhcp_relay_table = {}
        # Dict: vlan member (port/PortChannel) -> set of VlanID, in which the member if an untagged vlan member
        untagged_vlan_mbr = defaultdict(set)
        for vintf in vlanintfs.findall(str(QName(ns, "VlanInterface"))):
            vlanid = vintf.find(str(QName(ns, "VlanID"))).text
            vlantype = vintf.find(str(QName(ns, "Type")))
            if vlantype is None:
                vlantype_name = ""
            else:
                vlantype_name = vlantype.text
            vintfmbr = vintf.find(str(QName(ns, "AttachTo"))).text
            vmbr_list = vintfmbr.split(';')
            if vlantype_name != "Tagged":
                for member in vmbr_list:
                    untagged_vlan_mbr[member].add(vlanid)
        for vintf in vlanintfs.findall(str(QName(ns, "VlanInterface"))):
            vintfname = vintf.find(str(QName(ns, "Name"))).text
            vlanid = vintf.find(str(QName(ns, "VlanID"))).text
            vintfmbr = vintf.find(str(QName(ns, "AttachTo"))).text
            vlantype = vintf.find(str(QName(ns, "Type")))
            if vlantype is None:
                vlantype_name = ""
            else:
                vlantype_name = vlantype.text
            vmbr_list = vintfmbr.split(';')
            for i, member in enumerate(vmbr_list):
                vmbr_list[i] = port_alias_map.get(member, member)
                sonic_vlan_member_name = "Vlan%s" % (vlanid)
                if vlantype_name == "Tagged":
                    vlan_members[(sonic_vlan_member_name, vmbr_list[i])] = {'tagging_mode': 'tagged'}
                elif len(untagged_vlan_mbr[member]) > 1:
                    vlan_members[(sonic_vlan_member_name, vmbr_list[i])] = {'tagging_mode': 'tagged'}
                else:
                    vlan_members[(sonic_vlan_member_name, vmbr_list[i])] = {'tagging_mode': 'untagged'}

            vlan_attributes = {'vlanid': vlanid}
            dhcp_attributes = {}

            # If this VLAN requires a DHCP relay agent, it will contain a <DhcpRelays> element
            # containing a list of DHCP server IPs
            vintf_node = vintf.find(str(QName(ns, "DhcpRelays")))
            if vintf_node is not None and vintf_node.text is not None:
                vintfdhcpservers = vintf_node.text
                vdhcpserver_list = vintfdhcpservers.split(';')
                vlan_attributes['dhcp_servers'] = vdhcpserver_list

            vintf_node = vintf.find(str(QName(ns, "Dhcpv6Relays")))
            if vintf_node is not None and vintf_node.text is not None:
                vintfdhcpservers = vintf_node.text
                vdhcpserver_list = vintfdhcpservers.split(';')
                vlan_attributes['dhcpv6_servers'] = vdhcpserver_list
                dhcp_attributes['dhcpv6_servers'] = vdhcpserver_list
            sonic_vlan_member_name = "Vlan%s" % (vlanid)
            dhcp_relay_table[sonic_vlan_member_name] = dhcp_attributes

            vlanmac = vintf.find(str(QName(ns, "MacAddress")))
            if vlanmac is not None and vlanmac.text is not None:
                vlan_attributes['mac'] = vlanmac.text

            sonic_vlan_name = "Vlan%s" % vlanid
            if sonic_vlan_name != vintfname:
                vlan_attributes['alias'] = vintfname
            vlans[sonic_vlan_name] = vlan_attributes
            vlan_member_list[sonic_vlan_name] = vmbr_list

        acls = {}
        for aclintf in aclintfs.findall(str(QName(ns, "AclInterface"))):
            if aclintf.find(str(QName(ns, "InAcl"))) is not None:
                aclname = aclintf.find(str(QName(ns, "InAcl"))).text.upper().replace(" ", "_").replace("-", "_")
                stage = "ingress"
            elif aclintf.find(str(QName(ns, "OutAcl"))) is not None:
                aclname = aclintf.find(str(QName(ns, "OutAcl"))).text.upper().replace(" ", "_").replace("-", "_")
                stage = "egress"
            else:
                sys.exit("Error: 'AclInterface' must contain either an 'InAcl' or 'OutAcl' subelement.")
            aclattach = aclintf.find(str(QName(ns, "AttachTo"))).text.split(';')
            acl_intfs = []
            is_mirror = False
            is_mirror_v6 = False
            is_mirror_dscp = False
            use_port_alias = True

            # Walk through all interface names/alias to determine whether the input string is
            # port name or alias.We need this logic because there can be duplicaitons in port alias
            # and port names
            # The input port name/alias can be either port_name or port_alias. A mix of name and alias is not accepted
            port_name_count = 0
            port_alias_count = 0
            total_count = 0
            for member in aclattach:
                member = member.strip()
                if member in pcs or \
                    member in vlans or \
                    member.lower().startswith('erspan') or \
                    member.lower().startswith('egress_erspan') or \
                    member.lower().startswith('erspan_dscp'):
                    continue
                total_count += 1
                if member in port_alias_map:
                    port_alias_count += 1
                if member in port_names_map:
                    port_name_count += 1
            # All inputs are port alias
            if port_alias_count == total_count:
                use_port_alias = True
            # All inputs are port name
            elif port_name_count == total_count:
                use_port_alias = False
            # There are both port alias and port name, then port alias is preferred to keep the behavior not changed
            else:
                use_port_alias = True
                # For CTRLPLANE ACL, both counters are 0
                if (port_alias_count != 0) and (port_name_count != 0):
                    print("Warning: The given port name for ACL " + aclname + " is inconsistent. It must be either port name or alias ", file=sys.stderr)

            # TODO: Ensure that acl_intfs will only ever contain front-panel interfaces (e.g.,
            # maybe we should explicity ignore management and loopback interfaces?) because we
            # decide an ACL is a Control Plane ACL if acl_intfs is empty below.
            for member in aclattach:
                member = member.strip()
                if member in pcs:
                    # If try to attach ACL to a LAG interface then we shall add the LAG to
                    # to acl_intfs directly instead of break it into member ports, ACL attach
                    # to LAG will be applied to all the LAG members internally by SAI/SDK
                    acl_intfs.append(member)
                elif member in vlans:
                    # For egress ACL attaching to vlan, we break them into vlan members
                    if stage == "egress":
                        acl_intfs.extend(vlan_member_list[member])
                    else:
                        acl_intfs.append(member)
                elif use_port_alias and (member in port_alias_map):
                    acl_intfs.append(port_alias_map[member])
                    # Give a warning if trying to attach ACL to a LAG member interface, correct way is to attach ACL to the LAG interface
                    if port_alias_map[member] in intfs_inpc:
                        print("Warning: ACL " + aclname + " is attached to a LAG member interface " + port_alias_map[member] + ", instead of LAG interface", file=sys.stderr)
                elif (not use_port_alias) and (member in port_names_map):
                    acl_intfs.append(member)
                    if member in intfs_inpc:
                        print("Warning: ACL " + aclname + " is attached to a LAG member interface " + member + ", instead of LAG interface", file=sys.stderr)
                elif member.lower().startswith('erspan') or member.lower().startswith('egress_erspan') or member.lower().startswith('erspan_dscp'):
                    if 'dscp' in member.lower():
                        is_mirror_dscp = True
                    elif member.lower().startswith('erspanv6') or member.lower().startswith('egress_erspanv6'):
                        is_mirror_v6 = True
                    else:
                        is_mirror = True
                    # Erspan session will be attached to all front panel ports
                    # initially. If panel ports is a member port of LAG, then
                    # the LAG will be added to acl table instead of the panel
                    # ports. Non-active ports will be removed from this list
                    # later after the rest of the minigraph has been parsed.
                    acl_intfs = pc_intfs[:]
                    for panel_port in port_alias_map.values():
                        # because of port_alias_asic_map we can have duplicate in port_alias_map
                        # so check if already present do not add
                        if panel_port not in intfs_inpc and panel_port not in acl_intfs:
                            acl_intfs.append(panel_port)
                    break
            # if acl is classified as mirror (erpsan) or acl interface 
            # are binded then do not classify as Control plane.
            # For multi-asic platforms it's possible there is no
            # interface are binded to everflow in host namespace.
            if acl_intfs or is_mirror_v6 or is_mirror or is_mirror_dscp:
                # Remove duplications
                dedup_intfs = []
                for intf in acl_intfs:
                    if intf not in dedup_intfs:
                        dedup_intfs.append(intf)

                acls[aclname] = {'policy_desc': aclname,
                                 'stage': stage,
                                 'ports': dedup_intfs}
                if is_mirror:
                    acls[aclname]['type'] = 'MIRROR'
                elif is_mirror_v6:
                    acls[aclname]['type'] = 'MIRRORV6'
                elif is_mirror_dscp:
                    acls[aclname]['type'] = 'MIRROR_DSCP'
                else:
                    acls[aclname]['type'] = 'L3V6' if  'v6' in aclname.lower() else 'L3'
            else:
                # This ACL has no interfaces to attach to -- consider this a control plane ACL
                try:
                    aclservice = aclintf.find(str(QName(ns, "Type"))).text

                    # If we already have an ACL with this name and this ACL is bound to a different service,
                    # append the service to our list of services
                    if aclname in acls:
                        if acls[aclname]['type'] != 'CTRLPLANE':
                            print("Warning: ACL '%s' type mismatch. Not updating ACL." % aclname, file=sys.stderr)
                        elif acls[aclname]['services'] == aclservice:
                            print("Warning: ACL '%s' already contains service '%s'. Not updating ACL." % (aclname, aclservice), file=sys.stderr)
                        else:
                            acls[aclname]['services'].append(aclservice)
                    else:
                        acls[aclname] = {'policy_desc': aclname,
                                         'type': 'CTRLPLANE',
                                         'stage': stage,
                                         'services': [aclservice]}
                except:
                    print("Warning: Ignoring Control Plane ACL %s without type" % aclname, file=sys.stderr)


        mg_tunnels = child.find(str(QName(ns, "TunnelInterfaces")))
        if mg_tunnels is not None:
            table_key_to_mg_key_map = {"encap_ecn_mode": "EcnEncapsulationMode",
                                       "ecn_mode": "EcnDecapsulationMode",
                                       "dscp_mode": "DifferentiatedServicesCodePointMode",
                                       "ttl_mode": "TtlMode"}

            tunnel_qos_remap_table_key_to_mg_key_map = {
                                       "decap_dscp_to_tc_map": "DecapDscpToTcMap",
                                       "decap_tc_to_pg_map": "DecapTcToPgMap",
                                       "encap_tc_to_queue_map": "EncapTcToQueueMap",
                                       "encap_tc_to_dscp_map": "EncapTcToDscpMap"}

            for mg_tunnel in mg_tunnels.findall(str(QName(ns, "TunnelInterface"))):
                tunnel_type = mg_tunnel.attrib["Type"]
                tunnel_name = mg_tunnel.attrib["Name"]
                tunnelintfs[tunnel_type][tunnel_name] = {
                    "tunnel_type": mg_tunnel.attrib["Type"].upper(),
                }

                for table_key, mg_key in table_key_to_mg_key_map.items():
                    # If the minigraph has the key, add the corresponding config DB key to the table
                    if mg_key in mg_tunnel.attrib:
                        tunnelintfs[tunnel_type][tunnel_name][table_key] = mg_tunnel.attrib[mg_key]
                
                tunnelintfs_qos_remap_config[tunnel_type][tunnel_name] = {
                    "tunnel_type": mg_tunnel.attrib["Type"].upper(),
                }

                for table_key, mg_key in tunnel_qos_remap_table_key_to_mg_key_map.items():
                    if mg_key in mg_tunnel.attrib:
                        tunnelintfs_qos_remap_config[tunnel_type][tunnel_name][table_key] = mg_tunnel.attrib[mg_key]

        return intfs, lo_intfs, mvrf, mgmt_intf, voq_inband_intfs, vlans, vlan_members, dhcp_relay_table, pcs, pc_members, acls, vni, tunnelintfs, dpg_ecmp_content, static_routes, tunnelintfs_qos_remap_config
    return None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None


def parse_host_loopback(dpg, hname):
    for child in dpg:
        hostname = child.find(str(QName(ns, "Hostname")))
        if hostname.text.lower() != hname.lower():
            continue
        lo_intfs = parse_loopback_intf(child)
        return lo_intfs


def parse_cpg(cpg, hname, local_devices=[]):
    bgp_sessions = {}
    bgp_internal_sessions = {}
    bgp_voq_chassis_sessions = {}
    myasn = None
    bgp_peers_with_range = {}
    for child in cpg:
        tag = child.tag
        if tag == str(QName(ns, "PeeringSessions")):
            for session in child.findall(str(QName(ns, "BGPSession"))):
                start_router = session.find(str(QName(ns, "StartRouter"))).text
                start_peer = session.find(str(QName(ns, "StartPeer"))).text
                end_router = session.find(str(QName(ns, "EndRouter"))).text
                end_peer = session.find(str(QName(ns, "EndPeer"))).text
                rrclient = 1 if session.find(str(QName(ns, "RRClient"))) is not None else 0
                if session.find(str(QName(ns, "HoldTime"))) is not None:
                    holdtime = session.find(str(QName(ns, "HoldTime"))).text
                else:
                    holdtime = 180
                if session.find(str(QName(ns, "KeepAliveTime"))) is not None:
                    keepalive = session.find(str(QName(ns, "KeepAliveTime"))).text
                else:
                    keepalive = 60
                nhopself = 1 if session.find(str(QName(ns, "NextHopSelf"))) is not None else 0

                # choose the right table and admin_status for the peer
                chassis_internal_ibgp = session.find(str(QName(ns, "ChassisInternal")))
                if chassis_internal_ibgp is not None and chassis_internal_ibgp.text == "voq":
                    table = bgp_voq_chassis_sessions
                    admin_status = 'up'
                elif chassis_internal_ibgp is not None and chassis_internal_ibgp.text == "chassis-packet":
                    table = bgp_internal_sessions
                    admin_status = 'up'
                elif end_router.lower() in local_devices and start_router.lower() in local_devices:
                    table = bgp_internal_sessions
                    admin_status = 'up'
                else:
                    table = bgp_sessions
                    admin_status = None

                if end_router.lower() == hname.lower():
                    table[start_peer.lower()] = {
                        'name': start_router,
                        'local_addr': end_peer.lower(),
                        'rrclient': rrclient,
                        'holdtime': holdtime,
                        'keepalive': keepalive,
                        'nhopself': nhopself
                    }
                    if admin_status:
                        table[start_peer.lower()]['admin_status'] = admin_status
                elif start_router.lower() == hname.lower():
                    table[end_peer.lower()] = {
                        'name': end_router,
                        'local_addr': start_peer.lower(),
                        'rrclient': rrclient,
                        'holdtime': holdtime,
                        'keepalive': keepalive,
                        'nhopself': nhopself
                    }
                    if admin_status:
                        table[end_peer.lower()]['admin_status'] = admin_status
        elif child.tag == str(QName(ns, "Routers")):
            for router in child.findall(str(QName(ns1, "BGPRouterDeclaration"))):
                asn = router.find(str(QName(ns1, "ASN"))).text
                hostname = router.find(str(QName(ns1, "Hostname"))).text
                if hostname.lower() == hname.lower():
                    myasn = asn
                    peers = router.find(str(QName(ns1, "Peers")))
                    for bgpPeer in peers.findall(str(QName(ns, "BGPPeer"))):
                        addr = bgpPeer.find(str(QName(ns, "Address"))).text
                        if bgpPeer.find(str(QName(ns1, "PeersRange"))) is not None: # FIXME: is better to check for type BGPPeerPassive
                            name = bgpPeer.find(str(QName(ns1, "Name"))).text
                            ip_range = bgpPeer.find(str(QName(ns1, "PeersRange"))).text
                            ip_range_group = ip_range.split(';') if ip_range and ip_range != "" else []
                            bgp_peers_with_range[name] = {
                                'name': name,
                                'ip_range': ip_range_group
                            }
                            if bgpPeer.find(str(QName(ns, "Address"))) is not None:
                                bgp_peers_with_range[name]['src_address'] = bgpPeer.find(str(QName(ns, "Address"))).text
                            if bgpPeer.find(str(QName(ns1, "PeerAsn"))) is not None:
                                bgp_peers_with_range[name]['peer_asn'] = bgpPeer.find(str(QName(ns1, "PeerAsn"))).text
                else:
                    for peer in bgp_sessions:
                        bgp_session = bgp_sessions[peer]
                        if hostname.lower() == bgp_session['name'].lower():
                            bgp_session['asn'] = asn
                    for peer in bgp_internal_sessions:
                        bgp_internal_session = bgp_internal_sessions[peer]
                        if hostname.lower() == bgp_internal_session['name'].lower():
                            bgp_internal_session['asn'] = asn
                    for peer in bgp_voq_chassis_sessions:
                        bgp_session = bgp_voq_chassis_sessions[peer]
                        if hostname.lower() == bgp_session['name'].lower():
                            bgp_session['asn'] = asn

    bgp_monitors = { key: bgp_sessions[key] for key in bgp_sessions if 'asn' in bgp_sessions[key] and bgp_sessions[key]['name'] == 'BGPMonitor' }
    def filter_bad_asn(table):
        return { key: table[key] for key in table if 'asn' in table[key] and int(table[key]['asn']) != 0 }
    bgp_sessions = filter_bad_asn(bgp_sessions)
    bgp_internal_sessions = filter_bad_asn(bgp_internal_sessions)
    bgp_voq_chassis_sessions = filter_bad_asn(bgp_voq_chassis_sessions)

    return bgp_sessions, bgp_internal_sessions, bgp_voq_chassis_sessions, myasn, bgp_peers_with_range, bgp_monitors


def parse_meta(meta, hname):
    syslog_servers = []
    dhcp_servers = []
    dhcpv6_servers = []
    ntp_servers = []
    tacacs_servers = []
    mgmt_routes = []
    erspan_dst = []
    deployment_id = None
    region = None
    cloudtype = None
    resource_type = None
    downstream_subrole = None
    switch_id = None
    switch_type = None
    max_cores = None
    kube_data = {}
    macsec_profile = {}
    redundancy_type = None
    downstream_redundancy_types = None
    qos_profile = None

    device_metas = meta.find(str(QName(ns, "Devices")))
    for device in device_metas.findall(str(QName(ns1, "DeviceMetadata"))):
        if device.find(str(QName(ns1, "Name"))).text.lower() == hname.lower():
            properties = device.find(str(QName(ns1, "Properties")))
            for device_property in properties.findall(str(QName(ns1, "DeviceProperty"))):
                name = device_property.find(str(QName(ns1, "Name"))).text
                value = device_property.find(str(QName(ns1, "Value"))).text
                value_group = value.strip().split(';') if value and value != "" else []
                if name == "DhcpResources":
                    dhcp_servers = value_group
                elif name == "NtpResources":
                    ntp_servers = value_group
                elif name == "SyslogResources":
                    syslog_servers = value_group
                elif name == "TacacsServer":
                    tacacs_servers = value_group
                    mgmt_routes.extend(value_group)
                elif name == "ForcedMgmtRoutes":
                    mgmt_routes.extend(value_group)
                elif name == "ErspanDestinationIpv4":
                    erspan_dst = value_group
                elif name == "DeploymentId":
                    deployment_id = value
                elif name == "Region":
                    region = value
                elif name == "CloudType":
                    cloudtype = value
                elif name == "ResourceType":
                    resource_type = value
                elif name == "DownStreamSubRole":
                    downstream_subrole = value
                elif name == "SwitchId":
                    switch_id = value
                elif name == "SwitchType":
                    switch_type = value
                elif name == "MaxCores":
                    max_cores = value
                elif name == "KubernetesEnabled":
                    kube_data["enable"] = value
                elif name == "KubernetesServerIp":
                    kube_data["ip"] = value
                elif name == 'MacSecProfile':
                    macsec_profile = parse_macsec_profile(value)
                elif name == "RedundancyType":
                    redundancy_type = value
                elif name == "DownstreamRedundancyTypes":
                    downstream_redundancy_types = value
                elif name == "SonicQosProfile":
                    qos_profile = value
    return syslog_servers, dhcp_servers, dhcpv6_servers, ntp_servers, tacacs_servers, mgmt_routes, erspan_dst, deployment_id, region, cloudtype, resource_type, downstream_subrole, switch_id, switch_type, max_cores, kube_data, macsec_profile, downstream_redundancy_types, redundancy_type, qos_profile


def parse_linkmeta(meta, hname):
    link = meta.find(str(QName(ns, "Link")))
    linkmetas = {}
    for linkmeta in link.findall(str(QName(ns1, "LinkMetadata"))):
        port = None
        fec_disabled = None

        # Sample: ARISTA05T1:Ethernet1/33;switch-t0:fortyGigE0/4
        key = linkmeta.find(str(QName(ns1, "Key"))).text
        endpoints = key.split(';')
        for endpoint in endpoints:
            t = endpoint.split(':')
            if len(t) == 2 and t[0].lower() == hname.lower():
                port = t[1]
                break
        else:
            # Cannot find a matching hname, something went wrong
            continue

        has_peer_switch = False
        upper_tor_hostname = ''
        lower_tor_hostname = ''
        auto_negotiation = None
        macsec_enabled = False
        tx_power = None
        laser_freq = None
        properties = linkmeta.find(str(QName(ns1, "Properties")))
        for device_property in properties.findall(str(QName(ns1, "DeviceProperty"))):
            name = device_property.find(str(QName(ns1, "Name"))).text
            value = device_property.find(str(QName(ns1, "Value"))).text
            if name == "FECDisabled":
                fec_disabled = value
            elif name in [ "GeminiPeeringLink", "LibraPeeringLink" ]:
                has_peer_switch = True
            elif name == "UpperTOR":
                upper_tor_hostname = value
            elif name == "LowerTOR":
                lower_tor_hostname = value
            elif name == "AutoNegotiation":
                auto_negotiation = value
            elif name == "MacSecEnabled":
                macsec_enabled = value
            elif name == "TxPower":
                tx_power = value
            elif name == "Frequency":
                laser_freq = value

        linkmetas[port] = {}
        if fec_disabled:
            linkmetas[port]["FECDisabled"] = fec_disabled
        if has_peer_switch:
            if upper_tor_hostname == hname:
                linkmetas[port]["PeerSwitch"] = lower_tor_hostname
            else:
                linkmetas[port]["PeerSwitch"] = upper_tor_hostname
        if auto_negotiation:
            linkmetas[port]["AutoNegotiation"] = auto_negotiation
        if macsec_enabled:
            linkmetas[port]["MacSecEnabled"] = macsec_enabled
        if tx_power:
            linkmetas[port]["tx_power"] = tx_power
        # Convert the freq in GHz
        if laser_freq:
            linkmetas[port]["laser_freq"] = int(float(laser_freq)*1000)
    return linkmetas

def parse_macsec_profile(val_string):
    macsec_profile = {}
    values = val_string.strip().split()
    for val in values:
        keys = val.strip().split('=')
        if keys[0] == 'PrimaryKey':
            macsec_profile['PrimaryKey'] = keys[1].strip('\"')
        elif keys[0] == 'FallbackKey':
            macsec_profile['FallbackKey'] = keys[1].strip('\"')

    return macsec_profile

def parse_asic_meta(meta, hname):
    sub_role = None
    switch_id = None
    switch_type = None
    max_cores = None
    deployment_id = None
    macsec_profile = {}
    device_metas = meta.find(str(QName(ns, "Devices")))
    for device in device_metas.findall(str(QName(ns1, "DeviceMetadata"))):
        if device.find(str(QName(ns1, "Name"))).text.lower() == hname.lower():
            properties = device.find(str(QName(ns1, "Properties")))
            for device_property in properties.findall(str(QName(ns1, "DeviceProperty"))):
                name = device_property.find(str(QName(ns1, "Name"))).text
                value = device_property.find(str(QName(ns1, "Value"))).text
                if name == "SubRole":
                    sub_role = value
                elif name == "SwitchId":
                    switch_id = value
                elif name == "SwitchType":
                    switch_type = value
                elif name == "MaxCores":
                    max_cores = value
                elif name == "DeploymentId":
                    deployment_id = value
                elif name == 'MacSecProfile':
                    macsec_profile = parse_macsec_profile(value)

    return sub_role, switch_id, switch_type, max_cores, deployment_id, macsec_profile

def parse_deviceinfo(meta, hwsku):
    port_speeds = {}
    port_descriptions = {}
    sys_ports = {}
    for device_info in meta.findall(str(QName(ns, "DeviceInfo"))):
        dev_sku = device_info.find(str(QName(ns, "HwSku"))).text
        if dev_sku == hwsku:
            interfaces = device_info.find(str(QName(ns, "EthernetInterfaces"))).findall(str(QName(ns1, "EthernetInterface")))
            interfaces = interfaces + device_info.find(str(QName(ns, "ManagementInterfaces"))).findall(str(QName(ns1, "ManagementInterface")))
            for interface in interfaces:
                alias = interface.find(str(QName(ns, "InterfaceName"))).text
                speed = interface.find(str(QName(ns, "Speed"))).text
                desc  = interface.find(str(QName(ns, "Description")))
                if desc != None:
                    port_descriptions[port_alias_map.get(alias, alias)] = desc.text
                port_speeds[port_alias_map.get(alias, alias)] = speed

            sysports = device_info.find(str(QName(ns, "SystemPorts")))
            if sysports is not None:
                for sysport in sysports.findall(str(QName(ns, "SystemPort"))):
                    portname = sysport.find(str(QName(ns, "Name"))).text
                    hostname = sysport.find(str(QName(ns, "Hostname")))
                    asic_name = sysport.find(str(QName(ns, "AsicName")))
                    system_port_id = sysport.find(str(QName(ns, "SystemPortId"))).text
                    switch_id = sysport.find(str(QName(ns, "SwitchId"))).text
                    core_id = sysport.find(str(QName(ns, "CoreId"))).text
                    core_port_id = sysport.find(str(QName(ns, "CorePortId"))).text
                    speed = sysport.find(str(QName(ns, "Speed"))).text
                    num_voq = sysport.find(str(QName(ns, "NumVoq"))).text
                    key = portname
                    if asic_name is not None:
                       key = "%s|%s" % (asic_name.text, key)
                    if hostname is not None:
                       key = "%s|%s" % (hostname.text, key)
                    sys_ports[key] = {"system_port_id": system_port_id, "switch_id": switch_id, "core_index": core_id, "core_port_index": core_port_id, "speed": speed, "num_voq": num_voq}

    return port_speeds, port_descriptions, sys_ports

# Function to check if IP address is present in the key.
# If it is present, then the key would be a tuple.
def is_ip_prefix_in_key(key):
    return (isinstance(key, tuple))

# Special parsing for spine chassis frontend
def parse_spine_chassis_fe(results, vni, lo_intfs, phyport_intfs, pc_intfs, pc_members, devices):
    chassis_vnet ='VnetFE'
    chassis_vxlan_tunnel = 'TunnelInt'
    chassis_vni = vni

    # Vxlan tunnel information
    lo_addr = '0.0.0.0'
    for lo in lo_intfs:
        lo_network = ipaddress.ip_network(UNICODE_TYPE(lo[1]), False)
        if lo_network.version == 4:
            lo_addr = str(lo_network.network_address)
            break
    results['VXLAN_TUNNEL'] = {chassis_vxlan_tunnel: {
        'src_ip': lo_addr
    }}

    # Vnet information
    results['VNET'] = {chassis_vnet: {
        'vxlan_tunnel': chassis_vxlan_tunnel,
        'vni': chassis_vni
    }}

    # For each IP interface
    for intf in phyport_intfs:
        # A IP interface may have multiple entries.
        # For example, "Ethernet0": {}", "Ethernet0|192.168.1.1": {}"
        # We only care about the one without IP information
        if is_ip_prefix_in_key(intf) == True:
            continue

        neighbor_router = results['DEVICE_NEIGHBOR'][intf]['name']

        # If the neighbor router is an external router
        if devices[neighbor_router]['type'] != chassis_backend_role:
            # Enslave the interface to a Vnet
            phyport_intfs[intf] = {'vnet_name': chassis_vnet}

    # For each port channel IP interface
    for pc_intf in pc_intfs:
        # A port channel IP interface may have multiple entries.
        # For example, "Portchannel0": {}", "Portchannel0|192.168.1.1": {}"
        # We only care about the one without IP information
        if is_ip_prefix_in_key(pc_intf) == True:
            continue

        intf_name = None
        # Get a physical interface that belongs to this port channel
        for pc_member in pc_members:
            if pc_member[0] == pc_intf:
                intf_name = pc_member[1]
                break

        if intf_name is None:
            print('Warning: cannot find any interfaces that belong to %s' % (pc_intf), file=sys.stderr)
            continue

        # Get the neighbor router of this port channel interface
        neighbor_router = results['DEVICE_NEIGHBOR'][intf_name]['name']

        # If the neighbor router is an external router
        if devices[neighbor_router]['type'] != chassis_backend_role:
            # Enslave the port channel interface to a Vnet
            pc_intfs[pc_intf] = {'vnet_name': chassis_vnet}

###############################################################################
#
# Post-processing functions
#
###############################################################################

def filter_acl_table_for_backend(acls, vlan_members):
    filter_acls = {}
    for acl_name, value in acls.items():
        if 'everflow' not in acl_name.lower():
            filter_acls[acl_name] = value

    ports = set()
    for vlan, member in vlan_members:
        ports.add(member)
    filter_acls['DATAACL'] = { 'policy_desc': 'DATAACL',
                               'stage': 'ingress',
                               'type': 'L3',
                               'ports': list(ports)
                             }
    return filter_acls

def filter_acl_table_bindings(acls, neighbors, port_channels, sub_role, device_type, is_storage_device, vlan_members):
    if device_type == 'BackEndToRRouter' and is_storage_device:
        return filter_acl_table_for_backend(acls, vlan_members)

    filter_acls = {}

    # If the asic role is BackEnd no ACL Table (Ctrl/Data/Everflow) is binded.
    # This will be applicable in Multi-NPU Platforms.

    if sub_role == BACKEND_ASIC_SUB_ROLE:
        return filter_acls

    front_port_channel_intf = []

    # List of Backplane ports
    backplane_port_list = [v for k,v in port_alias_map.items() if v.startswith(backplane_prefix())]

    # Get the front panel port channel.
    for port_channel_intf in port_channels:
        backend_port_channel = any(lag_member in backplane_port_list \
                                   for lag_member in port_channels[port_channel_intf]['members'])
        if not backend_port_channel:
            front_port_channel_intf.append(port_channel_intf)

    for acl_table, group_params in acls.items():
        group_type = group_params.get('type', None)
        filter_acls[acl_table] = acls[acl_table]

        # For Control Plane and Data ACL no filtering is needed
        # Control Plane ACL has no Interface associated and
        # Data Plane ACL Interface are attached via minigraph
        # AclInterface.
        if group_type != 'MIRROR' and group_type != 'MIRRORV6' and group_type != 'MIRROR_DSCP':
            continue

        # Filters out back-panel ports from the binding list for Everflow (Mirror)
        # ACL tables. We define an "back-panel" port as one that is a member of a
        # port channel connected to back asic or directly connected to back asic.
        # This will be applicable in Multi-NPU Platforms.
        front_panel_ports = []
        for port in group_params.get('ports', []):
            # Filter out backplane ports
            if port in backplane_port_list:
                continue
            # Filter out backplane port channels
            if port in port_channels and port not in front_port_channel_intf:
                continue
            front_panel_ports.append(port)

        # Filters out inactive front-panel ports from the binding list for mirror
        # ACL tables. We define an "active" port as one that is a member of a
        # front pannel port channel or one that is connected to a neighboring device via front panel port.
        active_ports = [port for port in front_panel_ports if port in neighbors.keys() or port in front_port_channel_intf]

        if not active_ports:
            print('Warning: mirror table {} in ACL_TABLE does not have any ports bound to it'.format(acl_table), file=sys.stderr)

        filter_acls[acl_table]['ports'] = active_ports

    return filter_acls

def enable_internal_bgp_session(bgp_sessions, filename, asic_name):
    '''
    In Multi-NPU session the internal sessions will always be up.
    So adding the admin-status 'up' configuration to bgp sessions
    BGP session between FrontEnd and BackEnd Asics are internal bgp sessions
    '''
    local_sub_role = parse_asic_sub_role(filename, asic_name)

    for peer_ip in bgp_sessions.keys():
        peer_name = bgp_sessions[peer_ip]['name']
        peer_sub_role = parse_asic_sub_role(filename, peer_name)
        if ((local_sub_role == FRONTEND_ASIC_SUB_ROLE and peer_sub_role == BACKEND_ASIC_SUB_ROLE) or
            (local_sub_role == BACKEND_ASIC_SUB_ROLE and peer_sub_role == FRONTEND_ASIC_SUB_ROLE)):
            bgp_sessions[peer_ip].update({'admin_status': 'up'})

def select_mmu_profiles(profile, platform, hwsku):
    """
        Select MMU files based on the device metadata attribute - SonicQosProfile
        if no QosProfile exists in the minigraph, then no action is needed.
        if a profile exists in the minigraph,
            - create a dir path to search 1 level down from the base path.
            - if no such dir path exists, no action is needed.
            - if a dir path exists, check for the presence of each file from
              the copy list in the dir path and copy it over to the base path.
    """
    if not profile:
        return

    files_to_copy = ['pg_profile_lookup.ini', 'qos.json.j2', 'buffers_defaults_t0.j2', 'buffers_defaults_t1.j2']

    path = os.path.join('/usr/share/sonic/device', platform, hwsku)

    dir_path = os.path.join(path, profile)
    if os.path.exists(dir_path):
        for file_item in files_to_copy:
            file_in_dir = os.path.join(dir_path, file_item)
            if os.path.isfile(file_in_dir):
                base_file = os.path.join(path, file_item)
                exec_cmd("sudo cp {} {}".format(file_in_dir, base_file))

###############################################################################
#
# Main functions
#
###############################################################################
def parse_xml(filename, platform=None, port_config_file=None, asic_name=None, hwsku_config_file=None):
    """ Parse minigraph xml file.

    Keyword arguments:
    filename -- minigraph file name
    platform -- device platform
    port_config_file -- port config file name
    asic_name -- asic name; to parse multi-asic device minigraph to
    generate asic specific configuration.
     """

    root = ET.parse(filename).getroot()

    u_neighbors = None
    u_devices = None
    hwsku = None
    bgp_sessions = None
    bgp_monitors = []
    bgp_asn = None
    intfs = None
    dpg_ecmp_content = {}
    png_ecmp_content = {}
    vlan_intfs = None
    pc_intfs = None
    tunnel_intfs = None
    tunnel_intfs_qos_remap_config = None
    vlans = None
    vlan_members = None
    dhcp_relay_table = None
    pcs = None
    mgmt_intf = None
    voq_inband_intfs = None
    lo_intfs = None
    neighbors = None
    devices = None
    sub_role = None
    resource_type = None
    downstream_subrole = None
    docker_routing_config_mode = "separated"
    port_speeds_default = {}
    port_speed_png = {}
    port_descriptions = {}
    sys_ports = {}
    console_ports = {}
    mux_cable_ports = {}
    syslog_servers = []
    dhcp_servers = []
    dhcpv6_servers = []
    ntp_servers = []
    tacacs_servers = []
    mgmt_routes = []
    erspan_dst = []
    bgp_peers_with_range = None
    deployment_id = None
    region = None
    cloudtype = None
    switch_id = None
    switch_type = None
    max_cores = None
    hostname = None
    linkmetas = {}
    host_lo_intfs = None
    is_storage_device = False
    local_devices = []
    kube_data = {}
    static_routes = {}
    system_defaults = {}
    macsec_profile = {}
    downstream_redundancy_types = None
    redundancy_type = None
    qos_profile = None

    hwsku_qn = QName(ns, "HwSku")
    hostname_qn = QName(ns, "Hostname")
    docker_routing_config_mode_qn = QName(ns, "DockerRoutingConfigMode")
    for child in root:
        if child.tag == str(hwsku_qn):
            hwsku = child.text
        if child.tag == str(hostname_qn):
            hostname = child.text
        if child.tag == str(docker_routing_config_mode_qn):
            docker_routing_config_mode = child.text

    (ports, alias_map, alias_asic_map) = get_port_config(hwsku=hwsku, platform=platform, port_config_file=port_config_file, asic_name=asic_name, hwsku_config_file=hwsku_config_file)
    
    port_names_map.update(ports)
    port_alias_map.update(alias_map)
    port_alias_asic_map.update(alias_asic_map)

    # Get the local device node from DeviceMetadata
    local_devices = parse_asic_meta_get_devices(root)

    for child in root:
        if asic_name is None:
            if child.tag == str(QName(ns, "DpgDec")):
                (intfs, lo_intfs, mvrf, mgmt_intf, voq_inband_intfs, vlans, vlan_members, dhcp_relay_table, pcs, pc_members, acls, vni, tunnel_intfs, dpg_ecmp_content, static_routes, tunnel_intfs_qos_remap_config) = parse_dpg(child, hostname)
            elif child.tag == str(QName(ns, "CpgDec")):
                (bgp_sessions, bgp_internal_sessions, bgp_voq_chassis_sessions, bgp_asn, bgp_peers_with_range, bgp_monitors) = parse_cpg(child, hostname)
            elif child.tag == str(QName(ns, "PngDec")):
                (neighbors, devices, console_dev, console_port, mgmt_dev, mgmt_port, port_speed_png, console_ports, mux_cable_ports, png_ecmp_content) = parse_png(child, hostname, dpg_ecmp_content)
            elif child.tag == str(QName(ns, "UngDec")):
                (u_neighbors, u_devices, _, _, _, _, _, _) = parse_png(child, hostname, None)
            elif child.tag == str(QName(ns, "MetadataDeclaration")):
                (syslog_servers, dhcp_servers, dhcpv6_servers, ntp_servers, tacacs_servers, mgmt_routes, erspan_dst, deployment_id, region, cloudtype, resource_type, downstream_subrole, switch_id, switch_type, max_cores, kube_data, macsec_profile, downstream_redundancy_types, redundancy_type, qos_profile) = parse_meta(child, hostname)
            elif child.tag == str(QName(ns, "LinkMetadataDeclaration")):
                linkmetas = parse_linkmeta(child, hostname)
            elif child.tag == str(QName(ns, "DeviceInfos")):
                (port_speeds_default, port_descriptions, sys_ports) = parse_deviceinfo(child, hwsku)
        else:
            if child.tag == str(QName(ns, "DpgDec")):
                (intfs, lo_intfs, mvrf, mgmt_intf, voq_inband_intfs, vlans, vlan_members, dhcp_relay_table, pcs, pc_members, acls, vni, tunnel_intfs, dpg_ecmp_content, static_routes, tunnel_intfs_qos_remap_config) = parse_dpg(child, asic_name)
                host_lo_intfs = parse_host_loopback(child, hostname)
            elif child.tag == str(QName(ns, "CpgDec")):
                (bgp_sessions, bgp_internal_sessions, bgp_voq_chassis_sessions, bgp_asn, bgp_peers_with_range, bgp_monitors) = parse_cpg(child, asic_name, local_devices)
            elif child.tag == str(QName(ns, "PngDec")):
                (neighbors, devices, port_speed_png) = parse_asic_png(child, asic_name, hostname)
            elif child.tag == str(QName(ns, "MetadataDeclaration")):
                (sub_role, switch_id, switch_type, max_cores, deployment_id, macsec_profile) = parse_asic_meta(child, asic_name)
            elif child.tag == str(QName(ns, "LinkMetadataDeclaration")):
                linkmetas = parse_linkmeta(child, hostname)
            elif child.tag == str(QName(ns, "DeviceInfos")):
                (port_speeds_default, port_descriptions, sys_ports) = parse_deviceinfo(child, hwsku)

    select_mmu_profiles(qos_profile, platform, hwsku)
    # set the host device type in asic metadata also
    device_type = [devices[key]['type'] for key in devices if key.lower() == hostname.lower()][0]
    if asic_name is None:
        current_device = [devices[key] for key in devices if key.lower() == hostname.lower()][0]
    else:
        current_device = [devices[key] for key in devices if key.lower() == asic_name.lower()][0]

    results = {}
    results['DEVICE_METADATA'] = {'localhost': {
        'bgp_asn': bgp_asn,
        'region': region,
        'cloudtype': cloudtype,
        'docker_routing_config_mode': docker_routing_config_mode,
        'hostname': hostname,
        'hwsku': hwsku,
        'type': device_type,
        'synchronous_mode': 'enable',
        'yang_config_validation': 'disable'
        }
    }

    if deployment_id is not None:
        results['DEVICE_METADATA']['localhost']['deployment_id'] = deployment_id

    cluster = [devices[key] for key in devices if key.lower() == hostname.lower()][0].get('cluster', "")
    if cluster:
        results['DEVICE_METADATA']['localhost']['cluster'] = cluster

    if kube_data:
        results['KUBERNETES_MASTER'] = {
            'SERVER': {
                'disable': str(kube_data.get('enable', '0') == '0'),
                'ip': kube_data.get('ip', '')
            }
        }

    results['PEER_SWITCH'], mux_tunnel_name, peer_switch_ip = get_peer_switch_info(linkmetas, devices)

    if bool(results['PEER_SWITCH']):
        results['DEVICE_METADATA']['localhost']['subtype'] = 'DualToR'
        if len(results['PEER_SWITCH'].keys()) > 1:
            print("Warning: more than one peer switch was found. Only the first will be parsed: {}".format(results['PEER_SWITCH'].keys()[0]))

        results['DEVICE_METADATA']['localhost']['peer_switch'] = list(results['PEER_SWITCH'].keys())[0]
    
    # Enable tunnel_qos_remap if downstream_redundancy_types(T1) or redundancy_type(T0) = Gemini/Libra
    enable_tunnel_qos_map = False
    if platform and 'kvm' in platform:
        enable_tunnel_qos_map = False
    elif results['DEVICE_METADATA']['localhost']['type'].lower() == 'leafrouter' and ('gemini' in str(downstream_redundancy_types).lower() or 'libra' in str(downstream_redundancy_types).lower()):
        enable_tunnel_qos_map = True
    elif results['DEVICE_METADATA']['localhost']['type'].lower() == 'torrouter' and ('gemini' in str(redundancy_type).lower() or 'libra' in str(redundancy_type).lower()):
        enable_tunnel_qos_map = True

    if enable_tunnel_qos_map:
        system_defaults['tunnel_qos_remap'] = {"status": "enabled"}

    if len(system_defaults) > 0:
        results['SYSTEM_DEFAULTS'] = system_defaults
    
    # for this hostname, if sub_role is defined, add sub_role in 
    # device_metadata
    if sub_role is not None:
        current_device['sub_role'] = sub_role
        results['DEVICE_METADATA']['localhost']['sub_role'] =  sub_role
        results['DEVICE_METADATA']['localhost']['asic_name'] =  asic_name
    elif switch_type == "voq":
       # On Voq switches asic_name is mandatory even on single-asic devices
       results['DEVICE_METADATA']['localhost']['asic_name'] = 'Asic0'

    # on Voq system each asic has a switch_id
    if switch_id is not None:
        results['DEVICE_METADATA']['localhost']['switch_id'] = switch_id
    # on Voq system each asic has a switch_type
    if switch_type is not None:
        results['DEVICE_METADATA']['localhost']['switch_type'] = switch_type
    # on Voq system each asic has a max_cores
    if max_cores is not None:
        results['DEVICE_METADATA']['localhost']['max_cores'] = max_cores

    # Voq systems have an inband interface
    if voq_inband_intfs is not None:
        results['VOQ_INBAND_INTERFACE'] = {}
        for key in voq_inband_intfs:
           results['VOQ_INBAND_INTERFACE'][key] = voq_inband_intfs[key]


    if resource_type is not None:
        results['DEVICE_METADATA']['localhost']['resource_type'] = resource_type

    if downstream_subrole is not None:
        results['DEVICE_METADATA']['localhost']['downstream_subrole'] = downstream_subrole

    results['BGP_NEIGHBOR'] = bgp_sessions
    results['BGP_MONITORS'] = bgp_monitors
    results['BGP_PEER_RANGE'] = bgp_peers_with_range
    results['BGP_INTERNAL_NEIGHBOR'] = bgp_internal_sessions
    results['BGP_VOQ_CHASSIS_NEIGHBOR'] = bgp_voq_chassis_sessions
    if mgmt_routes and mgmt_intf and len(mgmt_intf):
        # TODO: differentiate v4 and v6
        next(iter(mgmt_intf.values()))['forced_mgmt_routes'] = mgmt_routes
    results['MGMT_PORT'] = {}
    results['MGMT_INTERFACE'] = {}
    mgmt_intf_count = 0
    mgmt_alias_reverse_mapping = {}
    sorted_keys = natsorted(mgmt_intf.keys(), alg=natsortns.IGNORECASE, key=lambda x : "|".join(x))
    for key in sorted_keys:
        alias = key[0]
        if alias in mgmt_alias_reverse_mapping:
            name = mgmt_alias_reverse_mapping[alias]
        else:
            name = 'eth' + str(mgmt_intf_count)
            mgmt_intf_count += 1
            mgmt_alias_reverse_mapping[alias] = name
        results['MGMT_PORT'][name] = {'alias': alias, 'admin_status': 'up'}
        if alias in port_speeds_default:
            results['MGMT_PORT'][name]['speed'] = port_speeds_default[alias]
        results['MGMT_INTERFACE'][(name, key[1])] = mgmt_intf[key]
    results['LOOPBACK_INTERFACE'] = {}
    for lo_intf in lo_intfs:
        results['LOOPBACK_INTERFACE'][lo_intf] = lo_intfs[lo_intf]
        results['LOOPBACK_INTERFACE'][lo_intf[0]] = {}

    if host_lo_intfs is not None:
        for host_lo_intf in host_lo_intfs:
            results['LOOPBACK_INTERFACE'][host_lo_intf] = host_lo_intfs[host_lo_intf]
            results['LOOPBACK_INTERFACE'][host_lo_intf[0]] = {}

    results['MGMT_VRF_CONFIG'] = mvrf

    phyport_intfs = {}
    vlan_intfs = {}
    pc_intfs = {}
    vlan_invert_mapping = { v['alias']:k for k,v in vlans.items() if 'alias' in v }
    vlan_sub_intfs = {}

    for intf in intfs:
        if intf[0][0:4] == 'Vlan':
            vlan_intfs[intf] = {}

            if bool(results['PEER_SWITCH']):
                vlan_intfs[intf[0]] = {
                    'proxy_arp': 'enabled',
                    'grat_arp': 'enabled'
                }
            else:
                vlan_intfs[intf[0]] = {}
        elif intf[0] in vlan_invert_mapping:
            vlan_intfs[(vlan_invert_mapping[intf[0]], intf[1])] = {}

            if bool(results['PEER_SWITCH']):
                vlan_intfs[vlan_invert_mapping[intf[0]]] = {
                    'proxy_arp': 'enabled',
                    'grat_arp': 'enabled'
                }
            else:
                vlan_intfs[vlan_invert_mapping[intf[0]]] = {}
        elif VLAN_SUB_INTERFACE_SEPARATOR in intf[0]:
            vlan_sub_intfs[intf] = {}
            vlan_sub_intfs[intf[0]] = {'admin_status': 'up'}
        elif intf[0][0:11] == 'PortChannel':
            pc_intfs[intf] = {}
            pc_intfs[intf[0]] = {}
        else:
            phyport_intfs[intf] = {}
            phyport_intfs[intf[0]] = {}

    results['INTERFACE'] = phyport_intfs
    results['VLAN_INTERFACE'] = vlan_intfs

    if sys_ports:
       results['SYSTEM_PORT'] = sys_ports

    for port_name in port_speeds_default:
        # ignore port not in port_config.ini
        if port_name not in ports:
            continue

        ports.setdefault(port_name, {})['speed'] = port_speeds_default[port_name]

    for port_name in port_speed_png:
        # not consider port not in port_config.ini
        # If no port_config_file is found ports is empty so ignore this error
        if port_config_file is not None:
            if port_name not in ports:
                print("Warning: ignore interface '%s' as it is not in the port_config.ini" % port_name, file=sys.stderr)
                continue

        # skip management ports
        if port_name in mgmt_alias_reverse_mapping.keys():
            continue

        ports.setdefault(port_name, {})['speed'] = port_speed_png[port_name]

    for port_name, port in list(ports.items()):
        # get port alias from port_config.ini
        alias = port.get('alias', port_name)
        # generate default 100G FEC only if FECDisabled is not true and 'fec' is not defined in port_config.ini
        # Note: FECDisabled only be effective on 100G port right now
        if linkmetas.get(alias, {}).get('FECDisabled', '').lower() == 'true':
            port['fec'] = 'none'
        elif not port.get('fec') and port.get('speed') == '100000':
            port['fec'] = 'rs'

        # If AutoNegotiation is available in the minigraph, we override any value we may have received from port_config.ini
        autoneg = linkmetas.get(alias, {}).get('AutoNegotiation')
        if autoneg:
            port['autoneg'] = 'on' if autoneg.lower() == 'true' else 'off'

        # If macsec is enabled on interface, and profile is valid, add the profile to port
        macsec_enabled = linkmetas.get(alias, {}).get('MacSecEnabled')
        if macsec_enabled and 'PrimaryKey' in macsec_profile:
            port['macsec'] = macsec_profile['PrimaryKey']

        tx_power = linkmetas.get(alias, {}).get('tx_power')
        if tx_power:
            port['tx_power'] = tx_power

        laser_freq = linkmetas.get(alias, {}).get('laser_freq')
        if laser_freq:
            port['laser_freq'] = laser_freq

    # set port description if parsed from deviceinfo
    for port_name in port_descriptions:
        # ignore port not in port_config.ini
        if port_name not in ports:
            continue

        ports.setdefault(port_name, {})['description'] = port_descriptions[port_name]

    for port_name, port in ports.items():
        if not port.get('description'):
            if port_name in neighbors:
                # for the ports w/o description set it to neighbor name:port
                port['description'] = "%s:%s" % (neighbors[port_name]['name'], neighbors[port_name]['port'])
            else:
                # for the ports w/o neighbor info, set it to port alias
                port['description'] = port.get('alias', port_name)

    # set default port MTU as 9100 and default TPID 0x8100
    for port in ports.values():
        port['mtu'] = '9100'
        port['tpid'] = '0x8100'

    # asymmetric PFC is disabled by default
    for port in ports.values():
        port['pfc_asym'] = 'off'

    # set physical port default admin status up
    for port in phyport_intfs:
        if port[0] in ports:
            ports.get(port[0])['admin_status'] = 'up'

    if len(vlan_sub_intfs):
        for subintf in vlan_sub_intfs:
            if not isinstance(subintf, tuple):
                parent_port = subintf.split(".")[0]
                if parent_port in ports:
                     ports.get(parent_port)['admin_status'] = 'up'

    for member in list(pc_members.keys()) + list(vlan_members.keys()):
        port = ports.get(member[1])
        if port:
            port['admin_status'] = 'up'

    for port in neighbors.keys():
        if port in ports.keys():
            # make all neighbors connected ports to 'admin_up'
            ports[port]['admin_status'] = 'up'

    # bring up the inband voq interfaces
    for inband_port in voq_inband_intfs:
        if inband_port in ports.keys():
            ports[inband_port]['admin_status'] = 'up'

    # bring up the recirc port for voq chassis
    for port, port_attributes in ports.items():
        port_role = port_attributes.get('role', None)
        if port_role == 'Rec':
            ports[port]['admin_status'] = 'up'

    results['PORT'] = ports
    results['CONSOLE_PORT'] = console_ports

    if port_config_file:
        port_set = set(ports.keys())
        for (pc_name, mbr_map) in list(pcs.items()):
            # remove portchannels that contain ports not existing in port_config.ini
            # when port_config.ini exists
            if not set(mbr_map['members']).issubset(port_set):
                print("Warning: ignore '%s' as part of its member interfaces is not in the port_config.ini" % pc_name, file=sys.stderr)
                del pcs[pc_name]

    # set default port channel MTU as 9100 and admin status up and default TPID 0x8100
    for pc in pcs.values():
        pc['mtu'] = '9100'
        pc['tpid'] = '0x8100'
        pc['admin_status'] = 'up'

    results['PORTCHANNEL'] = pcs
    results['PORTCHANNEL_MEMBER'] = pc_members

    for pc_intf in list(pc_intfs.keys()):
        # remove portchannels not in PORTCHANNEL dictionary
        if isinstance(pc_intf, tuple) and pc_intf[0] not in pcs:
            print("Warning: ignore '%s' interface '%s' as '%s' is not in the valid PortChannel list" % (pc_intf[0], pc_intf[1], pc_intf[0]), file=sys.stderr)
            del pc_intfs[pc_intf]
            pc_intfs.pop(pc_intf[0], None)

    results['PORTCHANNEL_INTERFACE'] = pc_intfs

    # for storage backend subinterface info present in minigraph takes precedence over hwsku
    if current_device['type'] in backend_device_types and bool(vlan_sub_intfs):
        del results['INTERFACE']
        del results['PORTCHANNEL_INTERFACE']
        is_storage_device = True
        results['VLAN_SUB_INTERFACE'] = vlan_sub_intfs
        # storage backend T0 have all vlan members tagged
        for vlan in vlan_members:
            vlan_members[vlan]["tagging_mode"] = "tagged"
    elif current_device['type'] in backend_device_types and hwsku in backend_hwskus:
        del results['INTERFACE']
        del results['PORTCHANNEL_INTERFACE']
        is_storage_device = True

        for intf in phyport_intfs.keys():
            if isinstance(intf, tuple):
                intf_info = list(intf)
                intf_info[0] = intf_info[0] + VLAN_SUB_INTERFACE_SEPARATOR + VLAN_SUB_INTERFACE_VLAN_ID
                sub_intf = tuple(intf_info)
                vlan_sub_intfs[sub_intf] = {}
            else:
                sub_intf = intf + VLAN_SUB_INTERFACE_SEPARATOR + VLAN_SUB_INTERFACE_VLAN_ID
                vlan_sub_intfs[sub_intf] = {"admin_status" : "up"}

        for pc_intf in pc_intfs.keys():
            if isinstance(pc_intf, tuple):
                pc_intf_info = list(pc_intf)
                pc_intf_info[0] = pc_intf_info[0] + VLAN_SUB_INTERFACE_SEPARATOR + VLAN_SUB_INTERFACE_VLAN_ID
                sub_intf = tuple(pc_intf_info)
                vlan_sub_intfs[sub_intf] = {}
            else:
                sub_intf = pc_intf + VLAN_SUB_INTERFACE_SEPARATOR + VLAN_SUB_INTERFACE_VLAN_ID
                vlan_sub_intfs[sub_intf] = {"admin_status" : "up"}
        results['VLAN_SUB_INTERFACE'] = vlan_sub_intfs
        # storage backend T0 have all vlan members tagged
        for vlan in vlan_members:
            vlan_members[vlan]["tagging_mode"] = "tagged"
    elif resource_type is not None and 'Storage' in resource_type:
        is_storage_device = True
    elif bool(vlan_sub_intfs):
        results['VLAN_SUB_INTERFACE'] = vlan_sub_intfs


    if is_storage_device:
        results['DEVICE_METADATA']['localhost']['storage_device'] = "true"

    # remove bgp monitor and slb peers for storage backend
    if is_storage_device and 'BackEnd' in current_device['type']:
        results['BGP_MONITORS'] = {}
        results['BGP_PEER_RANGE'] = {}

    results['VLAN'] = vlans
    results['VLAN_MEMBER'] = vlan_members

    # Add src_ip and qos remapping config into TUNNEL table if tunnel_qos_remap is enabled
    results['TUNNEL'] = get_tunnel_entries(tunnel_intfs, tunnel_intfs_qos_remap_config, lo_intfs, system_defaults.get('tunnel_qos_remap', {}), mux_tunnel_name, peer_switch_ip)

    active_active_ports = get_ports_in_active_active(root, devices, neighbors)
    results['MUX_CABLE'] = get_mux_cable_entries(ports, mux_cable_ports, active_active_ports, neighbors, devices, redundancy_type)

    # If connected to a smart cable, get the connection position
    for port_name, port in results['PORT'].items():
        if port_name in results['MUX_CABLE']:
            port['mux_cable'] = "true"

    if static_routes:
        results['STATIC_ROUTE'] = static_routes

    for nghbr in list(neighbors.keys()):
        # remove port not in port_config.ini
        if nghbr not in ports:
            if port_config_file is not None:
                print("Warning: ignore interface '%s' in DEVICE_NEIGHBOR as it is not in the port_config.ini" % nghbr, file=sys.stderr)
            del neighbors[nghbr]
    results['DEVICE_NEIGHBOR'] = neighbors
    if asic_name is None:
        results['DEVICE_NEIGHBOR_METADATA'] = { key:devices[key] for key in devices if key.lower() != hostname.lower() }
    else:
        results['DEVICE_NEIGHBOR_METADATA'] = { key:devices[key] for key in devices if key in {device['name'] for device in neighbors.values()} }
    results['SYSLOG_SERVER'] = dict((item, {}) for item in syslog_servers)
    results['DHCP_SERVER'] = dict((item, {}) for item in dhcp_servers)
    results['DHCP_RELAY'] = dhcp_relay_table
    results['NTP_SERVER'] = dict((item, {}) for item in ntp_servers)
    results['TACPLUS_SERVER'] = dict((item, {'priority': '1', 'tcp_port': '49'}) for item in tacacs_servers)
    results['ACL_TABLE'] = filter_acl_table_bindings(acls, neighbors, pcs, sub_role, current_device['type'], is_storage_device, vlan_members)
    results['FEATURE'] = {
        'telemetry': {
            'state': 'enabled'
        }
    }
    results['TELEMETRY'] = {
        'gnmi': {
            'client_auth': 'true',
            'port': '50051',
            'log_level': '2'
        },
        'certs': {
            'server_crt': '/etc/sonic/telemetry/streamingtelemetryserver.cer',
            'server_key': '/etc/sonic/telemetry/streamingtelemetryserver.key',
            'ca_crt': '/etc/sonic/telemetry/dsmsroot.cer'
        }
    }
    results['FEATURE'] = {
        'gnmi': {
            'state': 'enabled'
        }
    }
    results['GNMI'] = {
        'gnmi': {
            'client_auth': 'true',
            'port': '50052',
            'log_level': '2'
        },
        'certs': {
            'server_crt': '/etc/sonic/telemetry/streamingtelemetryserver.cer',
            'server_key': '/etc/sonic/telemetry/streamingtelemetryserver.key',
            'ca_crt': '/etc/sonic/telemetry/dsmsroot.cer'
        }
    }
    results['RESTAPI'] = {
        'config': {
            'client_auth': 'true',
            'allow_insecure': 'false',
            'log_level': 'info'
        },
        'certs': {
            'server_crt': '/etc/sonic/credentials/restapiserver.crt',
            'server_key': '/etc/sonic/credentials/restapiserver.key',
            'ca_crt': '/etc/sonic/credentials/AME_ROOT_CERTIFICATE.pem',
            'client_crt_cname': 'client.restapi.sonic.gbl,vnetds.prod.int.azure-int.net'
        }
    }

    if len(png_ecmp_content):
        results['FG_NHG_MEMBER'] = png_ecmp_content['FG_NHG_MEMBER']
        results['FG_NHG'] = png_ecmp_content['FG_NHG']
        results['NEIGH'] = png_ecmp_content['NEIGH']

    # Do not configure the minigraph's mirror session, which is currently unused
    # mirror_sessions = {}
    # if erspan_dst:
    #     lo_addr = '0.0.0.0'
    #     for lo in lo_intfs:
    #         lo_network = ipaddress.ip_network(UNICODE_TYPE(lo[1]), False)
    #         if lo_network.version == 4:
    #             lo_addr = str(lo_network.network_address)
    #             break
    #     count = 0
    #     for dst in erspan_dst:
    #         mirror_sessions['everflow{}'.format(count)] = {"dst_ip": dst, "src_ip": lo_addr}
    #         count += 1
    #     results['MIRROR_SESSION'] = mirror_sessions

    # Special parsing for spine chassis frontend routers
    if current_device['type'] == spine_chassis_frontend_role:
        parse_spine_chassis_fe(results, vni, lo_intfs, phyport_intfs, pc_intfs, pc_members, devices)

    # Enable console management feature for console swtich
    results['CONSOLE_SWITCH'] = {
        'console_mgmt' : {
            'enabled' : 'yes' if current_device['type'] in console_device_types else 'no'
        }
    }

    # Enable DHCP Server feature for specific device type
    if current_device['type'] in dhcp_server_enabled_device_types:
        results['DEVICE_METADATA']['localhost']['dhcp_server'] = 'enabled'

    return results

def get_tunnel_entries(tunnel_intfs, tunnel_intfs_qos_remap_config, lo_intfs, tunnel_qos_remap, mux_tunnel_name, peer_switch_ip):
    lo_addr = ''
    # Use the first IPv4 loopback as the tunnel destination IP
    for addr in lo_intfs.keys():
        ip_addr = ipaddress.ip_network(UNICODE_TYPE(addr[1]))
        if isinstance(ip_addr, ipaddress.IPv4Network):
            lo_addr = str(ip_addr.network_address)
            break

    tunnels = {}

    default_qos_map_for_mux_tunnel = {
        "decap_dscp_to_tc_map": "AZURE_TUNNEL",
        "decap_tc_to_pg_map": "AZURE_TUNNEL",
        "encap_tc_to_dscp_map": "AZURE_TUNNEL",
        "encap_tc_to_queue_map": "AZURE_TUNNEL"
    }

    for type, tunnel_dict in tunnel_intfs.items():
        for tunnel_key, tunnel_attr in tunnel_dict.items():
            tunnel_attr['dst_ip'] = lo_addr

            if (tunnel_qos_remap.get('status') == 'enabled') and (mux_tunnel_name == tunnel_key) and (peer_switch_ip is not None):
                tunnel_attr['src_ip'] = peer_switch_ip
                # The DSCP mode must be pipe if remap is enabled
                tunnel_attr['dscp_mode'] = "pipe"
                if tunnel_key in tunnel_intfs_qos_remap_config[type]:
                    tunnel_attr.update(tunnel_intfs_qos_remap_config[type][tunnel_key].items())
                # Use default value if qos remap attribute is missing
                for k, v in default_qos_map_for_mux_tunnel.items():
                    if k not in tunnel_attr:
                        tunnel_attr[k] = v

            tunnels[tunnel_key] = tunnel_attr

    return tunnels


def get_ports_in_active_active(root, devices, neighbors):
    """Parse out ports in active-active cable type."""
    servers = {hostname.lower(): device_data for hostname, device_data in devices.items() if device_data["type"] == "Server"}
    ports_in_active_active = {}
    dpg_section = root.find(str(QName(ns, "DpgDec")))
    neighbor_to_port_mapping = {neighbor["name"].lower(): port for port, neighbor in neighbors.items()}
    if dpg_section is not None:
        for child in dpg_section:
            hostname = child.find(str(QName(ns, "Hostname")))
            if hostname is None:
                continue
            hostname = hostname.text.lower()
            if hostname not in servers:
                continue
            lo_intfs = parse_loopback_intf(child)
            soc_intfs = {}
            for intfname, ipprefix in lo_intfs.keys():
                intfname_lower = intfname.lower()
                if hostname + "soc" == intfname_lower:
                    ipprefix = str(ipaddress.ip_network(UNICODE_TYPE(ipprefix.split("/")[0])))
                    if "." in ipprefix:
                        soc_intfs["soc_ipv4"] = ipprefix
                    elif ":" in ipprefix:
                        soc_intfs["soc_ipv6"] = ipprefix
            if hostname in neighbor_to_port_mapping and soc_intfs:
                ports_in_active_active[neighbor_to_port_mapping[hostname]] = soc_intfs
    return ports_in_active_active


def get_mux_cable_entries(ports, mux_cable_ports, active_active_ports, neighbors, devices, redundancy_type):
    mux_cable_table = {}
    if redundancy_type:
        redundancy_type = redundancy_type.lower()

    for port in ports:
        is_active_active = redundancy_type in ("libra", "mixed") and port in active_active_ports
        is_active_standby = port in mux_cable_ports
        if is_active_active and is_active_standby:
            print("Warning: skip %s as it is defined as active-standby and actie-active" % port, file=sys.stderr)
            continue
        if not (is_active_active or is_active_standby):
            continue

        entry = {}
        neighbor = neighbors[port]['name']
        entry['state'] = 'auto'

        if devices[neighbor]['lo_addr'] is not None:
            # Always force a /32 prefix for server IPv4 loopbacks
            server_ipv4_lo_addr = devices[neighbor]['lo_addr'].split("/")[0]
            server_ipv4_lo_prefix = ipaddress.ip_network(UNICODE_TYPE(server_ipv4_lo_addr))
            entry['server_ipv4'] = str(server_ipv4_lo_prefix)

            if 'lo_addr_v6' in devices[neighbor] and devices[neighbor]['lo_addr_v6'] is not None:
                server_ipv6_lo_addr = devices[neighbor]['lo_addr_v6'].split('/')[0]
                server_ipv6_lo_prefix = ipaddress.ip_network(UNICODE_TYPE(server_ipv6_lo_addr))
                entry['server_ipv6'] = str(server_ipv6_lo_prefix)

            if is_active_active:
                entry['cable_type'] = 'active-active'
                entry.update(active_active_ports[port])

            mux_cable_table[port] = entry
        else:
            print("Warning: no server IPv4 loopback found for {}, skipping mux cable table entry".format(neighbor), file=sys.stderr)

    return mux_cable_table


def parse_device_desc_xml(filename):
    root = ET.parse(filename).getroot()
    (lo_prefix, lo_prefix_v6, mgmt_prefix, mgmt_prefix_v6, hostname, hwsku, d_type, _, _, _) = parse_device(root)

    results = {}
    results['DEVICE_METADATA'] = {'localhost': {
        'hostname': hostname,
        'hwsku': hwsku,
        }}

    results['LOOPBACK_INTERFACE'] = {('lo', lo_prefix): {}}
    if lo_prefix_v6:
        results['LOOPBACK_INTERFACE'] = {('lo_v6', lo_prefix_v6): {}}

    results['MGMT_INTERFACE'] = {}
    if mgmt_prefix:
        mgmtipn = ipaddress.ip_network(UNICODE_TYPE(mgmt_prefix), False)
        if mgmtipn != ipaddress.ip_network(u'0.0.0.0/0', False):
            gwaddr = ipaddress.ip_address((next(mgmtipn.hosts())))
            results['MGMT_INTERFACE'].update({('eth0', mgmt_prefix): {'gwaddr': gwaddr}})

    if mgmt_prefix_v6:
        mgmtipn_v6 = ipaddress.ip_network(UNICODE_TYPE(mgmt_prefix_v6), False)
        if mgmtipn != ipaddress.ip_network(u'::/0', False):
            gwaddr_v6 = ipaddress.ip_address((next(mgmtipn_v6.hosts())))
            results['MGMT_INTERFACE'].update({('eth0', mgmt_prefix_v6): {'gwaddr': gwaddr_v6}})

    return results

def parse_asic_sub_role(filename, asic_name):
    if not os.path.isfile(filename):
        return None
    root = ET.parse(filename).getroot()
    for child in root:
        if child.tag == str(QName(ns, "MetadataDeclaration")):
            sub_role, _, _, _, _, _= parse_asic_meta(child, asic_name)
            return sub_role

def parse_asic_switch_type(filename, asic_name):
    if os.path.isfile(filename):
        root = ET.parse(filename).getroot()
        for child in root:
            if child.tag == str(QName(ns, "MetadataDeclaration")):
                _, _, switch_type, _, _, _ = parse_asic_meta(child, asic_name)
                return switch_type
    return None

def parse_asic_meta_get_devices(root):
    local_devices = []

    for child in root:
        if child.tag == str(QName(ns, "MetadataDeclaration")):
            device_metas = child.find(str(QName(ns, "Devices")))
            for device in device_metas.findall(str(QName(ns1, "DeviceMetadata"))):
                name = device.find(str(QName(ns1, "Name"))).text.lower()
                local_devices.append(name)

    return local_devices

port_names_map = {}
port_alias_map = {}
port_alias_asic_map = {}


def print_parse_xml(filename):
    results = parse_xml(filename)
    print((json.dumps(results, indent=3, cls=minigraph_encoder)))
