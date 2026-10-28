#!/usr/bin/env python

import hashlib
import ipaddress
import os
import syslog

from jinja2 import Environment, FileSystemLoader
from dhcp_utilities.common.utils import merge_intervals, validate_str_type, is_smart_switch

UNICODE_TYPE = str
DHCP_SERVER_IPV4 = "DHCP_SERVER_IPV4"
DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS = "DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS"
DHCP_SERVER_IPV4_RANGE = "DHCP_SERVER_IPV4_RANGE"
DHCP_SERVER_IPV4_PORT = "DHCP_SERVER_IPV4_PORT"
DHCP_SERVER_IPV4_MATCH = "DHCP_SERVER_IPV4_MATCH"
DHCP_SERVER_IPV4_BINDING = "DHCP_SERVER_IPV4_BINDING"
VLAN_INTERFACE = "VLAN_INTERFACE"
VLAN_MEMBER = "VLAN_MEMBER"
DPUS = "DPUS"
MID_PLANE_BRIDGE = "MID_PLANE_BRIDGE"
MID_PLANE_BRIDGE_SUBNET_ID = 10000
PORT_MODE_CHECKER = ["DhcpServerTableCfgChangeEventChecker", "DhcpPortTableEventChecker", "DhcpRangeTableEventChecker",
                     "DhcpOptionTableEventChecker", "VlanTableEventChecker", "VlanIntfTableEventChecker",
                     "VlanMemberTableEventChecker"]
MATCH_MODE_CHECKER = ["DhcpServerTableCfgChangeEventChecker", "DhcpMatchTableEventChecker",
                      "DhcpBindingTableEventChecker", "DhcpRangeTableEventChecker", "DhcpOptionTableEventChecker",
                      "VlanTableEventChecker", "VlanIntfTableEventChecker", "VlanMemberTableEventChecker"]
SMART_SWITCH_CHECKER = ["DpusTableEventChecker", "MidPlaneTableEventChecker"]
LEASE_UPDATE_SCRIPT_PATH = "/etc/kea/lease_update.sh"
DEFAULT_LEASE_TIME = 900
DEFAULT_LEASE_PATH = "/var/lib/kea/kea-lease.csv"
KEA_DHCP4_CONF_TEMPLATE_PATH = "/usr/share/sonic/templates/kea-dhcp4.conf.j2"
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
DHCP_OPTION_FILE = f"{SCRIPT_DIR}/dhcp_option.csv"
SUPPORT_DHCP_OPTION_TYPE = ["binary", "boolean", "ipv4-address", "string", "uint8", "uint16", "uint32"]
OPTION_DHCP_SERVER_ID = "54"
SUPPORTED_MATCH_TYPES = ["circuit_id", "option60"]
MAX_DHCP_OPTION_BYTES = 255


class DhcpServCfgGenerator(object):
    port_alias_map = {}
    lease_update_script_path = ""
    lease_path = ""
    hook_lib_path = ""

    def __init__(self, dhcp_db_connector, hook_lib_path, lease_path=DEFAULT_LEASE_PATH,
                 lease_update_script_path=LEASE_UPDATE_SCRIPT_PATH, dhcp_option_path=DHCP_OPTION_FILE,
                 kea_conf_template_path=KEA_DHCP4_CONF_TEMPLATE_PATH):
        self.db_connector = dhcp_db_connector
        self.lease_path = lease_path
        self.lease_update_script_path = lease_update_script_path
        self.hook_lib_path = hook_lib_path
        # Read port alias map file, this file is render after container start, so it would not change any more
        self._parse_port_map_alias()
        # Get kea config template
        self._get_render_template(kea_conf_template_path)
        self._read_dhcp_option(dhcp_option_path)

    def generate(self):
        """
        Generate dhcp server config
        Returns:
            config string
            set of ranges used
            set of enabled dhcp interface
            set of used options
            set of db table need to be monitored
            set of enabled PORT-mode interfaces
            set of enabled MATCH-mode interfaces
            set of match conditions used by enabled MATCH bindings
        """
        # Generate from running config_db
        # PORT and PORTCHANNEL are not monitored, so refresh circuit ID names on every generation
        self._parse_port_map_alias()
        # Get host name
        device_metadata = self.db_connector.get_config_db_table("DEVICE_METADATA")
        hostname = self._parse_hostname(device_metadata)
        smart_switch = is_smart_switch(device_metadata)
        # Get ip information of vlan
        vlan_interface = self.db_connector.get_config_db_table(VLAN_INTERFACE)
        vlan_member_table = self.db_connector.get_config_db_table(VLAN_MEMBER)
        vlan_interfaces, vlan_members = self._parse_vlan(vlan_interface, vlan_member_table)

        # Parse dpu
        dpus_table = self.db_connector.get_config_db_table(DPUS)
        mid_plane_table = self.db_connector.get_config_db_table(MID_PLANE_BRIDGE)
        mid_plane, dpus = self._parse_dpu(dpus_table, mid_plane_table) if smart_switch else ({}, {})

        dhcp_server_ipv4, customized_options_ipv4, range_ipv4, port_ipv4, match_ipv4, binding_ipv4 = \
            self._get_dhcp_ipv4_tables_from_db()
        # Parse range table
        ranges = self._parse_range(range_ipv4)

        # Parse port table
        dhcp_interfaces = vlan_interfaces
        if smart_switch and "bridge" in mid_plane and "ip_prefix" in mid_plane:
            mid_plane_name = mid_plane["bridge"]
            dhcp_interfaces[mid_plane_name] = [{
                "network": ipaddress.ip_network(mid_plane["ip_prefix"], strict=False),
                "ip": mid_plane["ip_prefix"]
            }]
            dpus = ["{}|{}".format(mid_plane_name, dpu) for dpu in dpus]
        dhcp_members = vlan_members | set(dpus)
        port_ips, port_used_ranges = self._parse_port(port_ipv4, dhcp_interfaces, dhcp_members, ranges)
        match_pools, match_client_classes, match_used_ranges, used_matches = self._parse_match_bindings(
            dhcp_server_ipv4,
            match_ipv4,
            binding_ipv4,
            dhcp_interfaces,
            vlan_members,
            ranges,
            hostname
        )
        customized_options = self._parse_customized_options(customized_options_ipv4)
        render_obj, enabled_dhcp_interfaces, used_options, subscribe_table = \
            self._construct_obj_for_template(
                dhcp_server_ipv4,
                port_ips,
                hostname,
                customized_options,
                smart_switch,
                match_pools,
                match_client_classes
            )
        enabled_port_interfaces = {
            name for name, config in dhcp_server_ipv4.items()
            if config.get("state") == "enabled" and config.get("mode") == "PORT"
        }
        enabled_match_interfaces = {
            name for name, config in dhcp_server_ipv4.items()
            if config.get("state") == "enabled" and config.get("mode") == "MATCH"
        }

        if smart_switch:
            subscribe_table |= set(SMART_SWITCH_CHECKER)

        return (
            self._render_config(render_obj),
            port_used_ranges | match_used_ranges,
            enabled_dhcp_interfaces,
            used_options,
            subscribe_table,
            enabled_port_interfaces,
            enabled_match_interfaces,
            used_matches
        )

    def _parse_dpu(self, dpus_table, mid_plane_table):
        """
        Parse dpu related tables
        Args:
            dpus_table: DPU table dict
            mid_plane_table: mid_plane table dict
        Returns:
            Parsed obj, sample:
                mid_plane = {
                    "bridge": "bridge-midplane",
                    "address": "169.254.200.254/24"
                }
                dpus = {
                    "dpu0"
                }
        """
        mid_plane = mid_plane_table.get("GLOBAL", {})
        dpus = set([dpu_value["midplane_interface"] for dpu_value in dpus_table.values()
                   if "midplane_interface" in dpu_value])
        return mid_plane, dpus

    def _parse_customized_options(self, customized_options_ipv4):
        customized_options = {}
        for option_name, config in customized_options_ipv4.items():
            is_standard_option = config["id"] in self.dhcp_option["standard_options"].keys()
            is_customized_option = config["id"] in self.dhcp_option["customized_options"].keys()
            if not is_standard_option and not is_customized_option:
                syslog.syslog(syslog.LOG_ERR, "Unsupported option: {}".format(config["id"]))
                continue
            if is_standard_option:
                option_type = self.dhcp_option["standard_options"][config["id"]][0]
                if "type" in config and config["type"] != option_type:
                    syslog.syslog(syslog.LOG_WARNING,
                                  ("Option type [{}] is not consistent with expected dhcp option type [{}], will " +
                                   "honor expected type")
                                  .format(config["type"], option_type))
            else:
                option_type = config["type"] if "type" in config else "string"
            if option_type not in SUPPORT_DHCP_OPTION_TYPE:
                syslog.syslog(syslog.LOG_ERR, "Unsupported type: {}, currently only support {}"
                              .format(option_type, SUPPORT_DHCP_OPTION_TYPE))
                continue
            if not validate_str_type(option_type, config["value"]):
                syslog.syslog(syslog.LOG_ERR, "Option type [{}] and value [{}] are not consistent"
                              .format(option_type, config["value"]))
                continue
            if option_type == "string" and len(config["value"]) > 253:
                syslog.syslog(syslog.LOG_ERR, "String option value too long: {}".format(option_name))
                continue
            always_send = config["always_send"] if "always_send" in config else "true"
            customized_options[option_name] = {
                "id": config["id"],
                "value": config["value"].replace(",", "\\\\,") if option_type == "string" else config["value"],
                "type": option_type,
                "always_send": always_send,
                "option_type": "standard" if is_standard_option else "customized"
            }
        return customized_options

    def _render_config(self, render_obj):
        output = self.kea_template.render(render_obj)
        return output

    def _parse_vlan(self, vlan_interface, vlan_member):
        vlan_interfaces = self._get_vlan_ipv4_interface(vlan_interface.keys())
        vlan_members = set(vlan_member.keys())
        return vlan_interfaces, vlan_members

    def _parse_hostname(self, device_metadata):
        localhost_entry = device_metadata.get("localhost", {})
        if localhost_entry is None or "hostname" not in localhost_entry:
            syslog.syslog(syslog.LOG_ERR, "Cannot get hostname")
            raise Exception("Cannot get hostname")
        return localhost_entry["hostname"]

    def _get_render_template(self, kea_conf_template_path):
        # Semgrep does not allow to use jinja2 directly, but we do need jinja2 for SONiC
        env = Environment(loader=FileSystemLoader(os.path.dirname(kea_conf_template_path)))  # nosemgrep
        self.kea_template = env.get_template(os.path.basename(kea_conf_template_path))

    def _parse_port_map_alias(self):
        self.port_alias_map = {}
        port_table = self.db_connector.get_config_db_table("PORT")
        pc_table = self.db_connector.get_config_db_table("PORTCHANNEL")
        for port_name, item in port_table.items():
            self.port_alias_map[port_name] = item.get("alias", port_name)
        for pc_name in pc_table.keys():
            self.port_alias_map[pc_name] = pc_name

    def _construct_obj_for_template(self, dhcp_server_ipv4, port_ips, hostname, customized_options, smart_switch=False,
                                    match_pools=None, match_client_classes=None):
        match_pools = match_pools or {}
        match_client_classes = match_client_classes or []
        subnets = []
        client_classes = []
        enabled_dhcp_interfaces = set()
        used_options = set()
        customized_option_keys = customized_options.keys()
        # Different mode would subscribe different table, always subscribe DHCP_SERVER_IPV4
        subscribe_table = set(["DhcpServerTableCfgChangeEventChecker"])
        for dhcp_interface_name, dhcp_config in dhcp_server_ipv4.items():
            if "state" not in dhcp_config or dhcp_config["state"] != "enabled":
                continue
            enabled_dhcp_interfaces.add(dhcp_interface_name)
            if dhcp_config["mode"] == "PORT":
                subscribe_table |= set(PORT_MODE_CHECKER)
                if dhcp_interface_name not in port_ips:
                    syslog.syslog(syslog.LOG_WARNING, "Cannot get DHCP port config for {}"
                                  .format(dhcp_interface_name))
                    continue
                curr_options = {}
                dhcp_server_id_option = {}
                if "customized_options" in dhcp_config:
                    for option in dhcp_config["customized_options"]:
                        used_options.add(option)
                        if option not in customized_option_keys:
                            syslog.syslog(syslog.LOG_WARNING, "Customized option {} configured for {} is not defined"
                                          .format(option, dhcp_interface_name))
                            continue
                        current_option = {
                                "always_send": customized_options[option]["always_send"],
                                "value": customized_options[option]["value"],
                                "option_type": customized_options[option]["option_type"],
                                "id": customized_options[option]["id"]
                        }
                        if customized_options[option]["id"] == OPTION_DHCP_SERVER_ID:
                            dhcp_server_id_option = current_option
                        else:
                            curr_options[option] = current_option
                for dhcp_interface_ip, port_config in port_ips[dhcp_interface_name].items():
                    pools = []
                    for port_name, ip_ranges in port_config.items():
                        ip_range = None
                        for ip_range in ip_ranges:
                            client_class = "{}:{}".format(hostname, port_name)
                            ip_range = {
                                "range": "{} - {}".format(ip_range[0], ip_range[1]),
                                "client_class": client_class
                            }
                            pools.append(ip_range)
                        if ip_range is not None:
                            class_len = len(client_class)
                            client_classes.append({
                                "name": client_class,
                                "condition": "substring(relay4[1].hex, -{}, {}) == '{}'".format(class_len, class_len,
                                                                                                client_class)
                            })
                    if not dhcp_server_id_option:
                        dhcp_server_id_option = {
                            "value": dhcp_interface_ip.split("/")[0],
                            "always_send": "true"
                        }
                    subnet_obj = {
                        "id": MID_PLANE_BRIDGE_SUBNET_ID if smart_switch else dhcp_interface_name.replace("Vlan", ""),
                        "subnet": str(ipaddress.ip_network(dhcp_interface_ip, strict=False)),
                        "pools": pools,
                        "dhcp_server_id_option": dhcp_server_id_option,
                        "lease_time": dhcp_config["lease_time"] if "lease_time" in dhcp_config else DEFAULT_LEASE_TIME,
                        "customized_options": curr_options
                    }
                    if "gateway" in dhcp_config:
                        subnet_obj["gateway"] = dhcp_config["gateway"]
                    subnets.append(subnet_obj)
            elif dhcp_config["mode"] == "MATCH":
                subscribe_table |= set(MATCH_MODE_CHECKER)
                if dhcp_interface_name not in match_pools:
                    syslog.syslog(syslog.LOG_WARNING, "Cannot get DHCP MATCH binding config for {}"
                                  .format(dhcp_interface_name))
                    continue
                curr_options = {}
                dhcp_server_id_option = {}
                if "customized_options" in dhcp_config:
                    for option in dhcp_config["customized_options"]:
                        used_options.add(option)
                        if option not in customized_option_keys:
                            syslog.syslog(syslog.LOG_WARNING, "Customized option {} configured for {} is not defined"
                                          .format(option, dhcp_interface_name))
                            continue
                        current_option = {
                            "always_send": customized_options[option]["always_send"],
                            "value": customized_options[option]["value"],
                            "option_type": customized_options[option]["option_type"],
                            "id": customized_options[option]["id"]
                        }
                        if customized_options[option]["id"] == OPTION_DHCP_SERVER_ID:
                            dhcp_server_id_option = current_option
                        else:
                            curr_options[option] = current_option
                if len(match_pools[dhcp_interface_name]) > 1:
                    raise ValueError(
                        "DHCP MATCH pools on {} cannot span multiple IPv4 prefixes".format(dhcp_interface_name)
                    )
                for dhcp_interface_ip, pools in match_pools[dhcp_interface_name].items():
                    current_server_id_option = dhcp_server_id_option
                    if not current_server_id_option:
                        current_server_id_option = {
                            "value": dhcp_interface_ip.split("/")[0],
                            "always_send": "true"
                        }
                    subnet_obj = {
                        "id": MID_PLANE_BRIDGE_SUBNET_ID if smart_switch else dhcp_interface_name.replace("Vlan", ""),
                        "subnet": str(ipaddress.ip_network(dhcp_interface_ip, strict=False)),
                        "pools": pools,
                        "dhcp_server_id_option": current_server_id_option,
                        "lease_time": dhcp_config["lease_time"] if "lease_time" in dhcp_config else DEFAULT_LEASE_TIME,
                        "customized_options": curr_options
                    }
                    if "gateway" in dhcp_config:
                        subnet_obj["gateway"] = dhcp_config["gateway"]
                    subnets.append(subnet_obj)
        client_classes.extend(match_client_classes)
        render_obj = {
            "subnets": subnets,
            "client_classes": client_classes,
            "lease_update_script_path": self.lease_update_script_path,
            "lease_path": self.lease_path,
            "customized_options": {key: value for key, value in customized_options.items()
                                   if value["option_type"] == "customized"},
            "hook_lib_path": self.hook_lib_path
        }
        return render_obj, enabled_dhcp_interfaces, used_options, subscribe_table

    def _get_dhcp_ipv4_tables_from_db(self):
        """
        Get DHCP Server IPv4 related table from config_db.
        Returns:
            Six table objects.
        """
        dhcp_server_ipv4 = self.db_connector.get_config_db_table(DHCP_SERVER_IPV4)
        customized_options_ipv4 = self.db_connector.get_config_db_table(DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS)
        range_ipv4 = self.db_connector.get_config_db_table(DHCP_SERVER_IPV4_RANGE)
        port_ipv4 = self.db_connector.get_config_db_table(DHCP_SERVER_IPV4_PORT)
        match_ipv4 = self.db_connector.get_config_db_table(DHCP_SERVER_IPV4_MATCH)
        binding_ipv4 = self.db_connector.get_config_db_table(DHCP_SERVER_IPV4_BINDING)
        return dhcp_server_ipv4, customized_options_ipv4, range_ipv4, port_ipv4, match_ipv4, binding_ipv4

    def _get_vlan_ipv4_interface(self, vlan_interface_keys):
        """
        Get ipv4 info of vlans
        Args:
            vlan_interface_keys: Keys of vlan_interfaces, sample:
                [
                    "Vlan1000|192.168.0.1/21",
                    "Vlan1000|fc02:1000::1/64"
                ]
        Returns:
            Vlans infomation, sample:
                {
                    'Vlan1000': [{
                        'network': IPv4Network('192.168.0.0/24'),
                        'ip': '192.168.0.1/24'
                    }]
                }
        """
        ret = {}
        for key in vlan_interface_keys:
            splits = key.split("|")
            # Skip with no ip address
            if len(splits) != 2:
                continue
            network = ipaddress.ip_network(UNICODE_TYPE(splits[1]), False)
            # Skip ipv6
            if network.version != 4:
                continue
            if splits[0] not in ret:
                ret[splits[0]] = []
            ret[splits[0]].append({"network": network, "ip": splits[1]})
        return ret

    def _parse_range(self, range_ipv4):
        """
        Parse content in DHCP_SERVER_IPV4_RANGE table to below format:
        {
            'range2': [IPv4Address('192.168.0.3'), IPv4Address('192.168.0.6')],
            'range1': [IPv4Address('192.168.0.2'), IPv4Address('192.168.0.5')],
            'range3': [IPv4Address('192.168.0.10'), IPv4Address('192.168.0.10')]
        }
        Args:
            range_ipv4: Table object or dict of range.
        """
        ranges = {}
        for range in list(range_ipv4.keys()):
            curr_range = range_ipv4.get(range, {}).get("range", {})
            list_length = len(curr_range)
            if list_length == 0 or list_length > 2:
                syslog.syslog(syslog.LOG_WARNING, f"Length of {curr_range} is {list_length}, which is invalid!")
                continue
            address_start = ipaddress.ip_address(curr_range[0])
            address_end = ipaddress.ip_address(curr_range[1] if list_length == 2 else curr_range[0])
            # To make sure order of range is correct
            if address_start > address_end:
                syslog.syslog(syslog.LOG_WARNING, f"Start of {curr_range} is greater than end, skip it")
                continue
            ranges[range] = [address_start, address_end]

        return ranges

    def _match_range_network(self, dhcp_interface, dhcp_interface_name, port, range, port_ips):
        """
        Loop the IP of the dhcp interface and find the network that target range is in this network. And to construct
        below data to record range - port map
        {
            'Vlan1000': {
                '192.168.0.1/24': {
                    'etp2': [
                        [IPv4Address('192.168.0.7'), IPv4Address('192.168.0.7')]
                    ]
                }
            }
        }
        Args:
            dhcp_interface: Ip and network information of current DHCP interface, sample:
                [{
                    'network': IPv4Network('192.168.0.0/24'),
                    'ip': '192.168.0.1/24'
                }]
            dhcp_interface_name: Name of DHCP interface.
            port: Name of DHCP member port.
            range: Ip Range, sample:
                [IPv4Address('192.168.0.2'), IPv4Address('192.168.0.5')]
        """
        for dhcp_interface_ip in dhcp_interface:
            if not range[0] in dhcp_interface_ip["network"] or \
               not range[1] in dhcp_interface_ip["network"]:
                continue
            dhcp_interface_ip_str = dhcp_interface_ip["ip"]
            if dhcp_interface_ip_str not in port_ips[dhcp_interface_name]:
                port_ips[dhcp_interface_name][dhcp_interface_ip_str] = {}
            if port not in port_ips[dhcp_interface_name][dhcp_interface_ip_str]:
                port_ips[dhcp_interface_name][dhcp_interface_ip_str][port] = []
            port_ips[dhcp_interface_name][dhcp_interface_ip_str][port].append([range[0], range[1]])
            break

    def _parse_port(self, port_ipv4, dhcp_interfaces, dhcp_members, ranges):
        """
        Parse content in DHCP_SERVER_IPV4_PORT table to below format, which indicate ip ranges assign to interface.
        Args:
            port_ipv4: Table object.
            dhcp_interfaces: DHCP interfaces information, sample:
                {
                    'Vlan1000': [{
                        'network': IPv4Network('192.168.0.0/24'),
                        'ip': '192.168.0.1/24'
                    }]
                }
            dhcp_members: List of DHCP members
            ranges: Dict of ranges
        Returns:
            Dict of dhcp conf, sample:
                {
                    'Vlan1000': {
                        '192.168.0.1/24': {
                            'etp2': [
                                ['192.168.0.7', '192.168.0.7']
                            ],
                            'etp3': [
                                ['192.168.0.2', '192.168.0.6'],
                                ['192.168.0.10', '192.168.0.10']
                            ]
                        }
                    }
                }
            Set of used ranges.
        """
        port_ips = {}
        ip_ports = {}
        used_ranges = set()
        for port_key in list(port_ipv4.keys()):
            port_config = port_ipv4.get(port_key, {})
            # Cannot specify both 'ips' and 'ranges'
            if "ips" in port_config and len(port_config["ips"]) != 0 and "ranges" in port_config \
               and len(port_config["ranges"]) != 0:
                syslog.syslog(syslog.LOG_WARNING, f"Port config for {port_key} contains both ips and ranges, skip")
                continue
            splits = port_key.split("|")
            # Skip port not in correct vlan
            if port_key not in dhcp_members:
                syslog.syslog(syslog.LOG_WARNING, f"Port {splits[1]} is not in {splits[0]}")
                continue
            # Get dhcp interface name like Vlan1000
            dhcp_interface_name = splits[0]
            # Get dhcp member interface name like etp1, be consistent with dhcp_relay, if alias doesn't exist,
            # use port name directly
            port = self.port_alias_map[splits[1]] if splits[1] in self.port_alias_map else splits[1]
            if dhcp_interface_name not in dhcp_interfaces:
                syslog.syslog(syslog.LOG_WARNING, f"Interface {dhcp_interface_name} doesn't have IPv4 address")
                continue
            if dhcp_interface_name not in port_ips:
                port_ips[dhcp_interface_name] = {}
            # Get ip information of Vlan
            dhcp_interface = dhcp_interfaces[dhcp_interface_name]

            for dhcp_interface_ip in dhcp_interface:
                ip_ports[str(dhcp_interface_ip["network"])] = dhcp_interface_name

            if "ips" in port_config and len(port_config["ips"]) != 0:
                for ip in set(port_config["ips"]):
                    ip_address = ipaddress.ip_address(ip)
                    # Loop the IP of the dhcp interface and find the network that target ip is in this network.
                    self._match_range_network(dhcp_interface, dhcp_interface_name, port, [ip_address, ip_address],
                                              port_ips)
            if "ranges" in port_config and len(port_config["ranges"]) != 0:
                for range_name in list(port_config["ranges"]):
                    used_ranges.add(range_name)
                    if range_name not in ranges:
                        syslog.syslog(syslog.LOG_WARNING, f"Range {range_name} is not in range table, skip")
                        continue
                    range = ranges[range_name]
                    # Loop the IP of the dhcp interface and find the network that target range is in this network.
                    self._match_range_network(dhcp_interface, dhcp_interface_name, port, range, port_ips)
        # Merge ranges to avoid overlap
        for dhcp_interface_name, value in port_ips.items():
            for dhcp_interface_ip, port_range in value.items():
                for port_name, ip_range in port_range.items():
                    ranges = merge_intervals(ip_range)
                    ranges = [[str(range[0]), str(range[1])] for range in ranges]
                    port_ips[dhcp_interface_name][dhcp_interface_ip][port_name] = ranges
        return port_ips, used_ranges

    @staticmethod
    def _normalize_list(value):
        if value is None:
            return []
        if isinstance(value, str):
            return value.split(",") if value else []
        return list(value)

    @staticmethod
    def _merge_match_intervals(intervals):
        merged = []
        for start, end in sorted(intervals, key=lambda interval: interval[0]):
            if not merged or int(start) > int(merged[-1][1]) + 1:
                merged.append([start, end])
            else:
                merged[-1][1] = max(merged[-1][1], end)
        return merged

    @staticmethod
    def _predicates_overlap(first, second):
        return all(match_type not in second or second[match_type]["value"] == condition["value"]
                   for match_type, condition in first.items())

    @staticmethod
    def _interval_sets_overlap(first, second):
        for first_start, first_end in first:
            for second_start, second_end in second:
                if first_start <= second_end and second_start <= first_end:
                    return True
        return False

    def _parse_match_condition(self, match_name, match_ipv4, hostname, condition_cache):
        if match_name in condition_cache:
            return condition_cache[match_name]
        if match_name not in match_ipv4:
            raise ValueError("Match {} does not exist".format(match_name))

        config = match_ipv4[match_name]
        match_type = config.get("type")
        value = config.get("value")
        if match_type not in SUPPORTED_MATCH_TYPES:
            raise ValueError("Match {} has unsupported type {}".format(match_name, match_type))
        if not isinstance(value, str) or not value:
            raise ValueError("Match {} has an invalid value".format(match_name))

        port_name = None
        resolved_value = value
        if match_type == "circuit_id":
            ports = sorted(port for port, alias in self.port_alias_map.items() if alias == value)
            if len(ports) > 1:
                raise ValueError("Circuit ID value {} is ambiguous".format(value))
            # Like _parse_port, fall back to the value itself when no port or PortChannel carries it
            port_name = ports[0] if ports else value
            resolved_value = "{}:{}".format(hostname, value)

        encoded_value = resolved_value.encode("utf-8")
        if len(encoded_value) > MAX_DHCP_OPTION_BYTES:
            raise ValueError("Match {} exceeds the DHCP option payload limit".format(match_name))
        if match_type == "circuit_id":
            atom = "relay4[1].exists and relay4[1].hex == 0x{}".format(encoded_value.hex())
        else:
            atom = "option[60].exists and option[60].hex == 0x{}".format(encoded_value.hex())

        condition = {
            "name": match_name,
            "type": match_type,
            "value": value,
            "port": port_name,
            "atom": atom
        }
        condition_cache[match_name] = condition
        return condition

    def _find_match_subnet(self, dhcp_interface_name, dhcp_interfaces, start, end):
        matching_subnets = [
            interface for interface in dhcp_interfaces.get(dhcp_interface_name, [])
            if start in interface["network"] and end in interface["network"]
        ]
        if not matching_subnets:
            syslog.syslog(syslog.LOG_WARNING, "Pool {}-{} of {} is not in an IPv4 subnet, skip".format(
                start, end, dhcp_interface_name))
            return None
        if len(matching_subnets) > 1:
            raise ValueError(
                "Pool {}-{} matches multiple IPv4 subnets of {}".format(start, end, dhcp_interface_name)
            )
        return matching_subnets[0]["ip"]

    def _parse_binding_pools(self, dhcp_interface_name, binding_name, binding_config, dhcp_interfaces, ranges):
        ips = self._normalize_list(binding_config.get("ips"))
        range_names = self._normalize_list(binding_config.get("ranges"))
        if bool(ips) == bool(range_names):
            raise ValueError(
                "Binding {}|{} must configure exactly one of ips or ranges".format(
                    dhcp_interface_name, binding_name
                )
            )
        values = ips if ips else range_names
        if len(values) != len(set(values)):
            raise ValueError("Binding {}|{} contains duplicate pool values".format(
                dhcp_interface_name, binding_name
            ))

        intervals_by_subnet = {}
        used_ranges = set()
        if ips:
            for ip in values:
                try:
                    address = ipaddress.ip_address(ip)
                except ValueError:
                    raise ValueError("Binding {}|{} contains invalid IP {}".format(
                        dhcp_interface_name, binding_name, ip
                    ))
                if address.version != 4:
                    raise ValueError("Binding {}|{} contains non-IPv4 address {}".format(
                        dhcp_interface_name, binding_name, ip
                    ))
                subnet = self._find_match_subnet(
                    dhcp_interface_name, dhcp_interfaces, address, address
                )
                if subnet is not None:
                    intervals_by_subnet.setdefault(subnet, []).append([address, address])
        else:
            for range_name in values:
                used_ranges.add(range_name)
                if range_name not in ranges:
                    syslog.syslog(syslog.LOG_WARNING, "Range {} referenced by {}|{} is not in range table, skip"
                                  .format(range_name, dhcp_interface_name, binding_name))
                    continue
                start, end = ranges[range_name]
                subnet = self._find_match_subnet(
                    dhcp_interface_name, dhcp_interfaces, start, end
                )
                if subnet is not None:
                    intervals_by_subnet.setdefault(subnet, []).append([start, end])

        for subnet, intervals in intervals_by_subnet.items():
            intervals_by_subnet[subnet] = self._merge_match_intervals(intervals)
        return intervals_by_subnet, used_ranges

    def _build_match_class_name(self, dhcp_interface_name, subnet, intervals, binding_keys):
        if dhcp_interface_name.startswith("Vlan") and dhcp_interface_name[4:].isdigit():
            interface_id = dhcp_interface_name[4:]
        else:
            interface_id = hashlib.sha256(dhcp_interface_name.encode("utf-8")).hexdigest()[:8]
        interval_text = ",".join("{}-{}".format(start, end) for start, end in intervals)
        hash_input = "|".join(
            [dhcp_interface_name, subnet, interval_text] + sorted(binding_keys)
        )
        suffix = hashlib.sha256(hash_input.encode("utf-8")).hexdigest()[:16]
        return "sonic_match_{}_{}".format(interface_id, suffix)

    def _parse_match_bindings(self, dhcp_server_ipv4, match_ipv4, binding_ipv4, dhcp_interfaces, vlan_members,
                              ranges, hostname):
        enabled_match_interfaces = {
            name for name, config in dhcp_server_ipv4.items()
            if config.get("state") == "enabled" and config.get("mode") == "MATCH"
        }
        if not enabled_match_interfaces:
            return {}, [], set(), set()

        bindings_by_interface = {name: [] for name in enabled_match_interfaces}
        for binding_key, binding_config in binding_ipv4.items():
            key_parts = binding_key.split("|", 1)
            if len(key_parts) != 2 or key_parts[0] not in enabled_match_interfaces:
                continue
            bindings_by_interface[key_parts[0]].append((key_parts[1], binding_config))

        condition_cache = {}
        used_matches = set()
        used_ranges = set()
        compiled_pools = {}
        client_classes = []

        for dhcp_interface_name in sorted(enabled_match_interfaces):
            interface_bindings = bindings_by_interface[dhcp_interface_name]
            if not interface_bindings:
                continue
            if dhcp_interface_name not in dhcp_interfaces:
                syslog.syslog(syslog.LOG_WARNING, f"Interface {dhcp_interface_name} doesn't have IPv4 address")
                continue

            normalized_bindings = []
            for binding_name, binding_config in sorted(interface_bindings):
                match_names = self._normalize_list(binding_config.get("matches"))
                if not match_names:
                    raise ValueError("Binding {}|{} must reference at least one match".format(
                        dhcp_interface_name, binding_name
                    ))
                if len(match_names) != len(set(match_names)):
                    raise ValueError("Binding {}|{} contains duplicate matches".format(
                        dhcp_interface_name, binding_name
                    ))

                binding_key = "{}|{}".format(dhcp_interface_name, binding_name)
                # Track references before skipping so restoring them triggers regeneration, as PORT mode does
                used_matches.update(match_names)
                used_ranges.update(self._normalize_list(binding_config.get("ranges")))
                conditions = {}
                skip_reason = None
                for match_name in match_names:
                    if match_name not in match_ipv4:
                        skip_reason = "match {} does not exist".format(match_name)
                        break
                    condition = self._parse_match_condition(
                        match_name, match_ipv4, hostname, condition_cache
                    )
                    if condition["type"] in conditions:
                        raise ValueError(
                            "Binding {}|{} contains multiple matches of type {}".format(
                                dhcp_interface_name, binding_name, condition["type"]
                            )
                        )
                    if condition["type"] == "circuit_id" and \
                       "{}|{}".format(dhcp_interface_name, condition["port"]) not in vlan_members:
                        skip_reason = "circuit ID value {} is not a member of {}".format(
                            condition["value"], dhcp_interface_name
                        )
                        break
                    conditions[condition["type"]] = condition
                if skip_reason:
                    syslog.syslog(syslog.LOG_WARNING, "Binding {}: {}, skip".format(binding_key, skip_reason))
                    continue

                intervals_by_subnet, _ = self._parse_binding_pools(
                    dhcp_interface_name,
                    binding_name,
                    binding_config,
                    dhcp_interfaces,
                    ranges
                )
                if not intervals_by_subnet:
                    syslog.syslog(syslog.LOG_WARNING, "Binding {} has no usable pool, skip".format(binding_key))
                    continue
                raw_predicate = " and ".join(
                    "({})".format(conditions[match_type]["atom"])
                    for match_type in SUPPORTED_MATCH_TYPES if match_type in conditions
                )
                normalized_bindings.append({
                    "name": binding_name,
                    "key": binding_key,
                    "conditions": conditions,
                    "specificity": len(conditions),
                    "raw_predicate": raw_predicate,
                    "intervals_by_subnet": intervals_by_subnet
                })

            for index, first in enumerate(normalized_bindings):
                for second in normalized_bindings[index + 1:]:
                    if first["specificity"] == second["specificity"] and \
                       self._predicates_overlap(first["conditions"], second["conditions"]):
                        raise ValueError(
                            "Bindings {} and {} overlap with equal specificity".format(
                                first["key"], second["key"]
                            )
                        )
                    common_subnets = set(first["intervals_by_subnet"]) & set(second["intervals_by_subnet"])
                    for subnet in common_subnets:
                        first_intervals = first["intervals_by_subnet"][subnet]
                        second_intervals = second["intervals_by_subnet"][subnet]
                        if first_intervals != second_intervals and \
                           self._interval_sets_overlap(first_intervals, second_intervals):
                            raise ValueError(
                                "Bindings {} and {} have partially overlapping pools".format(
                                    first["key"], second["key"]
                                )
                            )

            for binding in normalized_bindings:
                higher_predicates = sorted(
                    other["raw_predicate"] for other in normalized_bindings
                    if other["specificity"] > binding["specificity"] and
                    self._predicates_overlap(binding["conditions"], other["conditions"])
                )
                effective_predicate = binding["raw_predicate"]
                if higher_predicates:
                    exclusions = " or ".join("({})".format(predicate) for predicate in higher_predicates)
                    effective_predicate = "({}) and not ({})".format(
                        binding["raw_predicate"], exclusions
                    )
                binding["effective_predicate"] = effective_predicate

            pool_groups = {}
            for binding in normalized_bindings:
                for subnet, intervals in binding["intervals_by_subnet"].items():
                    interval_key = tuple((str(start), str(end)) for start, end in intervals)
                    pool_groups.setdefault((subnet, interval_key), []).append(binding)

            compiled_pools[dhcp_interface_name] = {}
            for (subnet, interval_key), grouped_bindings in sorted(pool_groups.items()):
                binding_keys = sorted(binding["key"] for binding in grouped_bindings)
                class_name = self._build_match_class_name(
                    dhcp_interface_name, subnet, interval_key, binding_keys
                )
                effective_predicates = sorted(
                    binding["effective_predicate"] for binding in grouped_bindings
                )
                class_condition = effective_predicates[0]
                if len(effective_predicates) > 1:
                    class_condition = " or ".join(
                        "({})".format(predicate) for predicate in effective_predicates
                    )
                client_classes.append({
                    "name": class_name,
                    "condition": class_condition
                })
                syslog.syslog(
                    syslog.LOG_INFO,
                    "DHCP MATCH class {} maps to bindings {}".format(class_name, ",".join(binding_keys))
                )
                pools = compiled_pools[dhcp_interface_name].setdefault(subnet, [])
                for start, end in interval_key:
                    pools.append({
                        "range": "{} - {}".format(start, end),
                        "client_class": class_name
                    })

            for pools in compiled_pools[dhcp_interface_name].values():
                pools.sort(
                    key=lambda pool: (
                        ipaddress.ip_address(pool["range"].split(" - ")[0]),
                        pool["client_class"]
                    )
                )

        client_classes.sort(key=lambda client_class: client_class["name"])
        return compiled_pools, client_classes, used_ranges, used_matches

    def _read_dhcp_option(self, file_path):
        # TODO current only support unassigned options, use dict in case support more options in the future
        # key: option code, value: option type list
        self.dhcp_option = {
            "customized_options": {},
            "standard_options": {}
        }
        with open(file_path, "r") as file:
            lines = file.readlines()
            for line in lines:
                if "Code,Type,Customized Type" in line:
                    continue
                splits = line.strip().split(",")
                if splits[-1] == "unassigned":
                    self.dhcp_option["customized_options"][splits[0]] = []
                elif splits[-1] == "defined_supported":
                    # TODO record and fqdn types are not supported currently
                    if splits[1] in SUPPORT_DHCP_OPTION_TYPE:
                        self.dhcp_option["standard_options"][splits[0]] = [splits[1]]
