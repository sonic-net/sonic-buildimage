import click
import json
import re
import ast
from natsort import natsorted
from tabulate import tabulate
import show.vlan as show_vlan
import utilities_common.cli as clicommon
from typing import Dict, Optional

from swsscommon.swsscommon import ConfigDBConnector
from swsscommon.swsscommon import SonicV2Connector


# COUNTERS_DB Table
DHCPv4_COUNTER_TABLE = 'COUNTERS_DHCPV4'
# STATE_DB Table
DHCPv6_COUNTER_TABLE = 'DHCPv6_COUNTER_TABLE'

# DHCPv4 Counter Messages
dhcpv4_messages = [
     'Unknown', 'Discover', 'Offer', 'Request', 'Acknowledge', 'NegativeAcknowledge', 'Release',
     'Inform', 'Decline', 'Malformed', 'Dropped'
]

# DHCPv6 Counter Messages
messages = ["Unknown", "Solicit", "Advertise", "Request", "Confirm", "Renew", "Rebind", "Reply", "Release", "Decline",
            "Reconfigure", "Information-Request", "Relay-Forward", "Relay-Reply", "Malformed"]

# DHCP_RELAY Config Table
DHCP_RELAY = 'DHCP_RELAY'
VLAN = "VLAN"
DHCPV4_RELAY_TABLE = 'DHCPV4_RELAY'
DHCPV6_SERVERS = "dhcpv6_servers"
DHCPV4_SERVERS = "dhcp_servers"
DHCPV4_TABLE_PARAMS = ["dhcpv4_servers", "server_vrf", "source_interface", "link_selection",
                       "vrf_selection", "server_id_override", "agent_relay_mode", "max_hop_count"]
SUPPORTED_DHCPV4_TYPE = [
    "Unknown", "Discover", "Offer", "Request", "Decline", "Ack", "Nak", "Release", "Inform", "Bootp"
]
SUPPORTED_DIR = ["TX", "RX"]
DHCPV4_COUNTER_TABLE_PREFIX = "DHCPV4_COUNTER_TABLE"
VLAN_MEMBER_TABLE_PREFIX = "VLAN_MEMBER"
COUNTERS_DB_SEPRATOR = ":"
CONFIG_DB_SEPRATOR = "|"
MGMT_PORT_TABLE = "MGMT_PORT"
config_db = ConfigDBConnector()


def check_sonic_dhcpv4_relay_flag():
    if config_db is None:
        return

    config_db.connect()
    table = config_db.get_entry("DEVICE_METADATA", "localhost")
    if('has_sonic_dhcpv4_relay' in table and table['has_sonic_dhcpv4_relay'] == 'True'):
        return True
    return False

def get_dhcp_helper_address(ctx, vlan):
    cfg, db = ctx
    vlan_dhcp_helper_data, _, _ = cfg
    vlan_config = vlan_dhcp_helper_data.get(vlan)
    if not vlan_config:
        return ""

    feature_data = db.cfgdb.get_table("FEATURE")
    dhcp_server_enabled = is_dhcp_server_enabled(feature_data)
    dhcp_helpers = ["N/A"] if dhcp_server_enabled else vlan_config.get('dhcp_servers', [])

    return '\n'.join(natsorted(dhcp_helpers))


show_vlan.VlanBrief.register_column('DHCP Helper Address', get_dhcp_helper_address)

class DHCPv4_Counter(object):
    def __init__(self):
        self.db = SonicV2Connector(use_unix_socket_path=False)
        self.db.connect(self.db.COUNTERS_DB)
        self.table_name = DHCPv4_COUNTER_TABLE+ self.db.get_db_separator(self.db.COUNTERS_DB)
        self.packet_abbr = ['Un', 'Dis', 'Off', 'Req', 'Ack', 'Nack', 'Rel', 'Inf', 'Dec', 'Mal', 'Drp']

    def _fetch_db_data(self, vlan: str) -> Dict:
        """Fetch DHCP counter data from Redis COUNTERS_DB."""
        dhcp_data = {}

        for key in self.db.keys(self.db.COUNTERS_DB):
            if DHCPv4_COUNTER_TABLE in key:
               table_data = self.db.get_all(self.db.COUNTERS_DB, key)

               intf_parts = key.split(self.db.get_db_separator(self.db.COUNTERS_DB))
               if len(intf_parts) > 1:
                   _intf = intf_parts[1]  # Get VLAN name

                   if _intf not in dhcp_data:
                       dhcp_data[_intf] = {}

                   # Get TX and RX counters for this interface
                   if "TX" in key:
                       dhcp_data[_intf]['TX'] = table_data
                   if "RX" in key:
                       dhcp_data[_intf]['RX'] = table_data

        return {DHCPv4_COUNTER_TABLE: dhcp_data}

    def _get_interface_counters(self, vlan: str, direction: Optional[str] = None, pkt_type: Optional[str] = None) -> Dict:
        """Extract counter data for interfaces from Redis data."""
        interface_counters = {}
        redis_data = self._fetch_db_data(vlan)

        for key, value in redis_data[DHCPv4_COUNTER_TABLE].items():
            if vlan in key:
                if direction:
                    counter_data = value[direction]
                    interface_counters[vlan] = counter_data
                else:
                    rx_data = value['RX']
                    tx_data = value['TX']
                    interface_counters[vlan] = {'RX': rx_data, 'TX': tx_data}

        return interface_counters

    def show_direction_counters(self, vlan: str, direction: str):
        """Generate table showing all packet types for a specific direction."""
        # Header with packet type abbreviations
        abbr_header = [
            "Packet type Abbr: Un - Unknown, Dis - Discover, Off - Offer, Req - Request,",
            "                  Ack - Acknowledge, Nack - NegativeAcknowledge, Rel - Release,",
            "                  Inf - Inform, Dec - Decline, Mal - Malformed, Drp - Dropped",
            ""
        ]

        try:
            interface_data = self._get_interface_counters(vlan, direction)

            # Prepare table headers and data
            headers = [f"Vlan ({direction})"] + self.packet_abbr
            table_data = []

            for interface, counters in sorted(interface_data.items()):
                row = [interface]
                row.extend(str(counters.get(ptype, '0')) for ptype in dhcpv4_messages)
                table_data.append(row)

            # Generate table using tabulate
            table = tabulate(table_data, headers=headers, tablefmt='grid')
            print("\n".join(abbr_header + [table]))

        except Exception as e:
            print(f"Error fetching data from Redis: {str(e)}")

    def show_packet_type_counters(self, vlan: str, pkt_type: str, direction: Optional[str] = None):
        """Generate table showing counters for a specific packet type."""
        try:
            # Determine columns based on direction
            columns = ['TX', 'RX'] if not direction else [direction]

            # Prepare table headers and data
            headers = [f"Vlan ({pkt_type})"] + columns
            table_data = []

            interface_data = self._get_interface_counters(vlan)
            for interface, counters in sorted(interface_data.items()):
                row = [interface]
                for col in columns:
                    count = counters[col][pkt_type]
                    row.append(count)
                table_data.append(row)

            # Generate table using tabulate
            print(tabulate(table_data, headers=headers, tablefmt='grid'))

        except Exception as e:
            print(f"Error fetching data from Redis: {str(e)}")

    def clear_table(self, direction, pkt_type, vlan_intf):
        """ Reset message counts to 0 """
        v4_cnts = {}
        for msg in dhcpv4_messages:
            v4_cnts[msg] = '0'

        for key in self.db.keys(self.db.COUNTERS_DB):
            if DHCPv4_COUNTER_TABLE in key:
                if vlan_intf and vlan_intf not in key:
                    continue

                self.db.hmset(self.db.COUNTERS_DB, key, (v4_cnts))


def ipv4_counters(dir, pkt_type, vlan):
    config_db.connect()
    feature_tbl = config_db.get_table("FEATURE")
    if is_dhcp_server_enabled(feature_tbl):
        click.echo("Unsupport to check dhcp_relay ipv4 counter when dhcp_server feature is enabled")
        return
    counter = DHCPv4_Counter()

    if dir and pkt_type:
        counter.show_packet_type_counters(vlan, pkt_type, dir)
    elif pkt_type:
        counter.show_packet_type_counters(vlan, pkt_type)
    elif dir:
        counter.show_direction_counters(vlan, dir)
    else:
        #when direction and message type both are not selected, for interface
        #sending direction as "TX" by default.
        counter.show_direction_counters(vlan, "TX")

class DHCPv6_Counter(object):
    def __init__(self):
        self.db = SonicV2Connector(use_unix_socket_path=False)
        self.db.connect(self.db.STATE_DB)
        self.table_name = DHCPv6_COUNTER_TABLE + self.db.get_db_separator(self.db.STATE_DB)
        self.table_prefix_len = len(self.table_name)

    def get_interface(self):
        """ Get all names of all interfaces in DHCPv6_COUNTER_TABLE """
        vlans = []
        for key in self.db.keys(self.db.STATE_DB, self.table_name + "*"):
            vlans.append(key[self.table_prefix_len:])
        return vlans

    def get_dhcp6relay_msg_count(self, interface, msg):
        """ Get count of a dhcp6relay message """
        count = self.db.get(self.db.STATE_DB, self.table_name + str(interface), str(msg))
        data = [str(msg), count]
        return data

    def clear_table(self, interface):
        """ Reset all message counts to 0 """
        for msg in messages:
            self.db.set(self.db.STATE_DB, self.table_name + str(interface), str(msg), '0')


def print_count(counter, intf):
    """Print count of each message"""
    data = []
    for i in messages:
        data.append(counter.get_dhcp6relay_msg_count(intf, i))
    print(tabulate(data, headers=["Message Type", intf], tablefmt='simple', stralign='right') + "\n")

#
# 'dhcp6relay_counters' group ###
#


@click.group(cls=clicommon.AliasedGroup, name="dhcp6relay_counters")
def dhcp6relay_counters():
    """Show DHCPv6 counter"""
    pass


def ipv6_counters(interface):
    counter = DHCPv6_Counter()
    counter_intf = counter.get_interface()

    if interface:
        print_count(counter, interface)
    else:
        for intf in counter_intf:
            print_count(counter, intf)


# 'counts' subcommand ("show dhcp6relay_counters counts")
@dhcp6relay_counters.command('counts')
@click.option('-i', '--interface', required=False)
@click.option('--verbose', is_flag=True, help="Enable verbose output")
def counts(interface, verbose):
    """Show dhcp6relay message counts"""

    ipv6_counters(interface)


@click.group(cls=clicommon.AliasedGroup, name="dhcprelay_helper")
def dhcp_relay_helper():
    """Show DHCP_Relay helper information"""
    pass

def get_dhcpv4_relay_data_with_header(table_data, entry_names, dhcp_server_enabled=False):
    vlan_relay = []
    vlans = table_data.keys()
    for vlan in vlans:
        vlan_data = table_data.get(vlan)
        row = [vlan]

        if dhcp_server_enabled:
            continue;

        for entry in entry_names:
            entry_data = vlan_data.get(entry)
            if entry_data is None or len(entry_data) == 0:
                row.append("N/A")
            else:
                if isinstance(entry_data, list):
                    row.append("\n".join(entry_data))
                else:
                    row.append(entry_data)
        vlan_relay.append(row)

    headers = ["Interface", "DHCP Relay Address", "Server Vrf", "Source Interface", "Link Selection", "VRF Selection", "Server ID Override", "Agent Relay Mode", "Max Hop Count"]

    return tabulate(vlan_relay, tablefmt='grid', stralign='right', headers=headers) + '\n'


def get_dhcp_relay_data_with_header(table_data, entry_name, dhcp_server_enabled=False):
    vlan_relay = {}
    vlans = table_data.keys()
    for vlan in vlans:
        vlan_data = table_data.get(vlan)
        dhcp_relay_data = vlan_data.get(entry_name)
        if dhcp_relay_data is None or len(dhcp_relay_data) == 0:
            continue

        vlan_relay[vlan] = []
        if dhcp_server_enabled:
            vlan_relay[vlan].append("N/A")
        else:
            for address in dhcp_relay_data:
                vlan_relay[vlan].append(address)

    dhcp_relay_vlan_keys = vlan_relay.keys()
    relay_address_list = ["\n".join(vlan_relay[key]) for key in dhcp_relay_vlan_keys]
    data = {"Interface": dhcp_relay_vlan_keys, "DHCP Relay Address": relay_address_list}
    return tabulate(data, tablefmt='grid', stralign='right', headers='keys') + '\n'


def is_dhcp_server_enabled(feature_tbl):
    if feature_tbl is not None and "dhcp_server" in feature_tbl and "state" in feature_tbl["dhcp_server"] and \
       feature_tbl["dhcp_server"]["state"] == "enabled":
        return True
    return False


def get_dhcp_relay(table_name, entry_name, with_header):
    if config_db is None:
        return

    config_db.connect()
    table_data = config_db.get_table(table_name)
    if table_data is None:
        return

    dhcp_server_enabled = False
    if table_name in {VLAN, DHCPV4_RELAY_TABLE}:
        feature_tbl = config_db.get_table("FEATURE")
        dhcp_server_enabled = is_dhcp_server_enabled(feature_tbl)

    if with_header:
        if table_name == DHCPV4_RELAY_TABLE:
            output = get_dhcpv4_relay_data_with_header(table_data, entry_name, dhcp_server_enabled)
        else:
            output = get_dhcp_relay_data_with_header(table_data, entry_name, dhcp_server_enabled)
        print(output)
    else:
        vlans = config_db.get_keys(table_name)
        for vlan in vlans:
            output = get_data(table_data, vlan)
            print(output)


def is_vlan_interface_valid(vlan_interface, db):
    # If not vlan interface specified, treat it as valid
    if vlan_interface == "":
        return True
    if not db.exists(db.COUNTERS_DB, DHCPV4_COUNTER_TABLE_PREFIX + COUNTERS_DB_SEPRATOR + vlan_interface):
        return False
    return True


def get_vlan_members_from_config_db(db, vlan_interface):
    vlan_members = {}
    for key in db.keys(db.CONFIG_DB, VLAN_MEMBER_TABLE_PREFIX + CONFIG_DB_SEPRATOR + vlan_interface + "*"):
        splits = key.split(CONFIG_DB_SEPRATOR)
        if not splits[1].endswith(vlan_interface):
            continue
        if splits[1] not in vlan_members:
            vlan_members[splits[1]] = set()
        vlan_members[splits[1]].add(splits[2])
    return vlan_members


def get_count_from_db(db, key, dir, type):
    count_str = db.get(db.COUNTERS_DB, key, dir)
    count = 0
    if count_str is not None:
        try:
            count = json.loads(count_str.replace("\'", "\"")).get(type, "0")
        except json.JSONDecodeError:
            pass
    return count


def append_vlan_count_with_type_specified(data, db, dirs, vlan_interface, current_type, summary):
    key = DHCPV4_COUNTER_TABLE_PREFIX + COUNTERS_DB_SEPRATOR + vlan_interface
    data[summary].append(vlan_interface)
    data["Intf Type"].append("VLAN")
    for current_dir in dirs:
        count = get_count_from_db(db, key, current_dir, current_type)
        data[current_dir].append(count)


def get_interfaces_type(interface_name, mgmt_intfs, vlan_interface, vlan_members):
    if interface_name in mgmt_intfs:
        interface_type = "MGMT"
    elif vlan_interface not in vlan_members:
        interface_type = "Unknown"
    elif interface_name in vlan_members[vlan_interface]:
        interface_type = "Downlink"
    else:
        interface_type = "Uplink"
    return interface_type


def append_interfaces_count_with_type_specified(data, db, dirs, vlan_interface, current_type, summary, vlan_members,
                                                mgmt_intfs):
    key_pattern = DHCPV4_COUNTER_TABLE_PREFIX + COUNTERS_DB_SEPRATOR + vlan_interface + COUNTERS_DB_SEPRATOR + "*"
    keys = sorted(db.keys(db.COUNTERS_DB, key_pattern), key=natural_sort_key)
    for key in keys:
        interface_name = key.split(COUNTERS_DB_SEPRATOR)[2]
        data[summary].append(interface_name)
        interface_type = get_interfaces_type(interface_name, mgmt_intfs, vlan_interface, vlan_members)
        data["Intf Type"].append(interface_type)
        for current_dir in dirs:
            count = get_count_from_db(db, key, current_dir, current_type)
            data[current_dir].append(count)


def generate_output_with_type_specified(db, vlan_members, types, dirs, vlan_interface, mgmt_intfs):
    # If type specified, then there would be only 1 type
    current_type = types[0]
    summary = "{} ({})".format(vlan_interface, current_type)

    # Data need to be printed
    data = {
        summary: [],
        "Intf Type": []
    }
    for dir in dirs:
        data.setdefault(dir, [])

    # Get Vlan interface count
    append_vlan_count_with_type_specified(data, db, dirs, vlan_interface, current_type, summary)
    # Get physical interfaces count
    append_interfaces_count_with_type_specified(data, db, dirs, vlan_interface, current_type, summary,
                                                vlan_members, mgmt_intfs)
    return tabulate(data, tablefmt="pretty", stralign="left", headers="keys") + "\n"


def append_vlan_count_without_type_specified(data, db, dir, vlan_interface, types, summary):
    key = DHCPV4_COUNTER_TABLE_PREFIX + COUNTERS_DB_SEPRATOR + vlan_interface
    data[summary].append(vlan_interface)
    data["Intf Type"].append("VLAN")
    for current_type in types:
        count = get_count_from_db(db, key, dir, current_type)
        data[current_type].append(count)


def natural_sort_key(string):
    return [int(text) if text.isdigit() else text.lower() for text in re.split(r'(\d+)', string)]


def append_interfaces_count_without_type_specified(data, db, dir, vlan_interface, types, summary, vlan_members,
                                                   mgmt_intfs):
    key_pattern = DHCPV4_COUNTER_TABLE_PREFIX + COUNTERS_DB_SEPRATOR + vlan_interface + COUNTERS_DB_SEPRATOR + "*"
    keys = sorted(db.keys(db.COUNTERS_DB, key_pattern), key=natural_sort_key)
    for key in keys:
        interface_name = key.split(COUNTERS_DB_SEPRATOR)[2]
        data[summary].append(interface_name)
        interface_type = get_interfaces_type(interface_name, mgmt_intfs, vlan_interface, vlan_members)
        data["Intf Type"].append(interface_type)
        for current_type in types:
            count = get_count_from_db(db, key, dir, current_type)
            data[current_type].append(count)


def generate_output_without_type_specified(db, vlan_members, types, dir, vlan_interface, mgmt_intfs):
    summary = "{} ({})".format(vlan_interface, dir)
    data = {
        summary: [],
        "Intf Type": []
    }
    for type in types:
        data.setdefault(type, [])

    # Get vlan interface count
    append_vlan_count_without_type_specified(data, db, dir, vlan_interface, types, summary)
    # Get physical interfaces count
    append_interfaces_count_without_type_specified(data, db, dir, vlan_interface, types, summary, vlan_members,
                                                   mgmt_intfs)
    return tabulate(data, tablefmt="pretty", stralign="left", headers="keys") + "\n"


def get_mgmt_intfs_from_config_db(db):
    mgmt_intfs = [
        key.split(CONFIG_DB_SEPRATOR)[1] for key in db.keys(db.CONFIG_DB, MGMT_PORT_TABLE + CONFIG_DB_SEPRATOR + "*")
        if len(key.split(CONFIG_DB_SEPRATOR)) > 1
    ]
    return mgmt_intfs


def get_vlans_from_counters_db(vlan_interface, db):
    # Get vlan set from COUNTERS_DB
    counters_key = DHCPV4_COUNTER_TABLE_PREFIX + COUNTERS_DB_SEPRATOR + vlan_interface + "*"
    vlans = set()
    for key in db.keys(db.COUNTERS_DB, counters_key):
        splits = key.split(COUNTERS_DB_SEPRATOR)
        if vlan_interface == "" or vlan_interface == splits[1]:
            vlans.add(splits[1])
    return vlans


def print_dhcpv4_relay_counter_data(vlan_interface, dirs, types, db):
    # Get vlan set from COUNTERS_DB
    vlans = get_vlans_from_counters_db(vlan_interface, db)

    # Get vlan members
    vlan_members = get_vlan_members_from_config_db(db, vlan_interface)

    # Get mgmt interfaces
    mgmt_intfs = get_mgmt_intfs_from_config_db(db)

    # If user specify type
    if len(types) == 1:
        for vlan in vlans:
            output = generate_output_with_type_specified(db, vlan_members, types, dirs, vlan, mgmt_intfs)
            print(output)
    # type not specified
    else:
        for vlan in vlans:
            for dir in dirs:
                output = generate_output_without_type_specified(db, vlan_members, types, dir, vlan, mgmt_intfs)
                print(output)


@dhcp_relay_helper.command('ipv6')
def get_dhcpv6_helper_address():
    """Parse through DHCP_RELAY table for each interface in config_db.json and print dhcpv6 helpers in table format"""
    get_dhcp_relay(DHCP_RELAY, DHCPV6_SERVERS, with_header=False)


def get_data(table_data, vlan):
    vlan_data = table_data.get(vlan, {})
    helpers_data = vlan_data.get('dhcpv6_servers')
    addr = {vlan: []}
    output = ''
    if helpers_data is not None:
        for ip in helpers_data:
            addr[vlan].append(ip)
        output = tabulate({'Interface': [vlan], vlan: addr.get(vlan)}, tablefmt='simple', stralign='right') + '\n'
    return output


@click.group(cls=clicommon.AliasedGroup, name="dhcp_relay")
def dhcp_relay():
    """show DHCP_Relay information"""
    pass


@dhcp_relay.group(cls=clicommon.AliasedGroup, name="ipv6")
def dhcp_relay_ipv6():
    pass


@dhcp_relay.group(cls=clicommon.AliasedGroup, name="ipv4")
def dhcp_relay_ipv4():
    pass

@dhcp_relay_ipv4.command("vlan-counters")
@click.option('-d', '--direction', required=False, type=click.Choice(['TX', 'RX']), help="Specify TX(egress) or RX(ingress)")
@click.option('-t', '--type', required=False, type=click.Choice(dhcpv4_messages), help="Specify DHCP packet counter type")
@click.argument("vlan_interface", required=True)
def dhcp_relay_ip4_vlan_counters(direction, type, vlan_interface):
    ipv4_counters(direction, type, vlan_interface)


@dhcp_relay_ipv4.command("helper")
def dhcp_relay_ipv4_destination():
    if check_sonic_dhcpv4_relay_flag():
        get_dhcp_relay(DHCPV4_RELAY_TABLE, DHCPV4_TABLE_PARAMS, with_header=True)
    else:
        get_dhcp_relay(VLAN, DHCPV4_SERVERS, with_header=True)


@dhcp_relay_ipv6.command("destination")
def dhcp_relay_ipv6_destination():
    get_dhcp_relay(DHCP_RELAY, DHCPV6_SERVERS, with_header=True)

@click.group(cls=clicommon.AliasedGroup, name="dhcp4relay-counters")
def dhcp4relay_counters():
    """Show DHCPv4 counter"""
    pass

@dhcp4relay_counters.command("vlan-counts")
@click.option('-d', '--direction', required=False, type=click.Choice(['TX', 'RX']), help="Specify TX(egress) or RX(ingress)")
@click.option('-t', '--type', required=False, type=click.Choice(dhcpv4_messages), help="Specify DHCP packet counter type")
@click.argument("vlan_interface", required=True)
def counts(direction, type, vlan_interface):
    ipv4_counters(direction, type, vlan_interface)

@dhcp_relay_ipv6.command("counters")
@click.option('-i', '--interface', required=False)
def dhcp_relay_ip6counters(interface):
    ipv6_counters(interface)


@dhcp_relay_ipv4.command("counters")
@clicommon.pass_db
@click.argument("vlan_interface", required=False, default="")
@click.option('--dir', type=click.Choice(SUPPORTED_DIR), required=False)
@click.option('--type', type=click.Choice(SUPPORTED_DHCPV4_TYPE), required=False)
def dhcp_relay_ip4counters(db, vlan_interface, dir, type):
    if not is_vlan_interface_valid(vlan_interface, db.db):
        click.echo("Vlan interface {} doesn't have any count data".format(vlan_interface))
        return
    dirs = [dir] if dir else SUPPORTED_DIR
    types = [type] if type else SUPPORTED_DHCPV4_TYPE
    print_dhcpv4_relay_counter_data(vlan_interface, dirs, types, db.db)


def register(cli):
    cli.add_command(dhcp6relay_counters)
    cli.add_command(dhcp4relay_counters)
    cli.add_command(dhcp_relay_helper)
    cli.add_command(dhcp_relay)
