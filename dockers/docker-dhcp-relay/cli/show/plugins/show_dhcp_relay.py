import click
import ast
from natsort import natsorted
from tabulate import tabulate
import show.vlan as show_vlan
import utilities_common.cli as clicommon

from swsscommon.swsscommon import ConfigDBConnector
from swsscommon.swsscommon import SonicV2Connector

# STATE_DB Table
DHCPv4_COUNTER_TABLE = 'DHCP_COUNTER_TABLE'
DHCPv6_COUNTER_TABLE = 'DHCPv6_COUNTER_TABLE'

# DHCPv4 Counter Messages
dhcpv4_messages = [
    "Unknown", "Discover", "Offer", "Request", "Decline", "Ack", "Nack", "Release", "Inform"
]

# DHCPv6 Counter Messages
dhcpv6_messages = [
    "Unknown", "Solicit", "Advertise", "Request", "Confirm", "Renew", "Rebind", "Reply", "Release", 
    "Decline", "Reconfigure", "Information-Request", "Relay-Forward", "Relay-Reply", "Malformed"
]

# DHCP_RELAY Config Table
DHCP_RELAY = 'DHCP_RELAY'
VLAN = "VLAN"
DHCPV6_SERVERS = "dhcpv6_servers"
DHCPV4_SERVERS = "dhcp_servers"
config_db = ConfigDBConnector()


def get_dhcp_helper_address(ctx, vlan):
    cfg, _ = ctx
    vlan_dhcp_helper_data, _, _ = cfg
    vlan_config = vlan_dhcp_helper_data.get(vlan)
    if not vlan_config:
        return ""

    dhcp_helpers = vlan_config.get('dhcp_servers', [])

    return '\n'.join(natsorted(dhcp_helpers))


show_vlan.VlanBrief.register_column('DHCP Helper Address', get_dhcp_helper_address)

class DHCPv4_Counter(object):
    def __init__(self):
        self.db = SonicV2Connector(use_unix_socket_path=False)
        self.db.connect(self.db.STATE_DB)
        self.table_name = DHCPv4_COUNTER_TABLE + self.db.get_db_separator(self.db.STATE_DB)

    def get_interface(self):
        """ Get all names of all interfaces in DHCPv4_COUNTER_TABLE """
        interfaces = []
        for key in self.db.keys(self.db.STATE_DB):
            if DHCPv4_COUNTER_TABLE in key:
                interfaces.append(key[21:])
        return interfaces

    def get_dhcp4relay_msg_count(self, interface, dir):
        """ Get count of a dhcprelay message """
        value = self.db.get(self.db.STATE_DB, self.table_name + str(interface), str(dir))
        cnts = ast.literal_eval(str(value))
        data = []
        if cnts is not None:
            for k, v in cnts.items():
                data.append([k, v])
        return data

    def clear_table(self, interface):
        """ Reset all message counts to 0 """
        v4_cnts = {}
        for msg in dhcpv4_messages:
            v4_cnts[msg] = '0'
        self.db.set(self.db.STATE_DB, self.table_name + str(interface), str("RX"), str(v4_cnts))
        self.db.set(self.db.STATE_DB, self.table_name + str(interface), str("TX"), str(v4_cnts))

def print_dhcpv4_count(counter, intf):
    """Print count of each message"""
    rx_data = counter.get_dhcp4relay_msg_count(intf, "RX")
    print(tabulate(rx_data, headers=["Message Type", intf+"(RX)"], tablefmt='simple', stralign='right') + "\n")
    tx_data = counter.get_dhcp4relay_msg_count(intf, "TX")
    print(tabulate(tx_data, headers=["Message Type", intf+"(TX)"], tablefmt='simple', stralign='right') + "\n")

#
# 'dhcp4relay_counters' group ###
#


@click.group(cls=clicommon.AliasedGroup, name="dhcp4relay_counters")
def dhcp4relay_counters():
    """Show DHCPv4 counter"""
    pass


def ipv4_counters(interface):
    counter = DHCPv4_Counter()
    counter_intf = counter.get_interface()

    if interface:
        print_dhcpv4_count(counter, interface)
    else:
        for intf in counter_intf:
            print_dhcpv4_count(counter, intf)


# 'counts' subcommand ("show dhcp4relay_counters counts")
@dhcp4relay_counters.command('counts')
@click.option('-i', '--interface', required=False)
@click.option('--verbose', is_flag=True, help="Enable verbose output")
def counts(interface, verbose):
    """Show dhcp4relay message counts"""
    ipv4_counters(interface)


class DHCPv6_Counter(object):
    def __init__(self):
        self.db = SonicV2Connector(use_unix_socket_path=False)
        self.db.connect(self.db.STATE_DB)
        self.table_name = DHCPv6_COUNTER_TABLE + self.db.get_db_separator(self.db.STATE_DB)

    def get_interface(self):
        """ Get all names of all interfaces in DHCPv6_COUNTER_TABLE """
        interfaces = []
        for key in self.db.keys(self.db.STATE_DB):
            if DHCPv6_COUNTER_TABLE in key:
                interfaces.append(key[21:])
        return interfaces

    def get_dhcp6relay_msg_count(self, interface, dir):
        """ Get count of a dhcp6relay message """
        value = self.db.get(self.db.STATE_DB, self.table_name + str(interface), str(dir))
        cnts = ast.literal_eval(str(value))
        data = []
        if cnts is not None:
            for k, v in cnts.items():
                data.append([k, v])
        return data

    def clear_table(self, interface):
        """ Reset all message counts to 0 """
        v6_cnts = {}
        for msg in dhcpv6_messages:
            v6_cnts[msg] = '0'
        self.db.set(self.db.STATE_DB, self.table_name + str(interface), str("RX"), str(v6_cnts))
        self.db.set(self.db.STATE_DB, self.table_name + str(interface), str("TX"), str(v6_cnts))


def print_dhcpv6_count(counter, intf):
    """Print count of each message"""
    rx_data = counter.get_dhcp6relay_msg_count(intf, "RX")
    print(tabulate(rx_data, headers=["Message Type", intf+"(RX)"], tablefmt='simple', stralign='right') + "\n")
    tx_data = counter.get_dhcp6relay_msg_count(intf, "TX")
    print(tabulate(tx_data, headers=["Message Type", intf+"(TX)"], tablefmt='simple', stralign='right') + "\n")


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
        print_dhcpv6_count(counter, interface)
    else:
        for intf in counter_intf:
            print_dhcpv6_count(counter, intf)


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


def get_dhcp_relay_data_with_header(table_data, entry_name):
    vlan_relay = {}
    vlans = table_data.keys()
    for vlan in vlans:
        vlan_data = table_data.get(vlan)
        dhcp_relay_data = vlan_data.get(entry_name)
        if dhcp_relay_data is None or len(dhcp_relay_data) == 0:
            continue

        vlan_relay[vlan] = []
        for address in dhcp_relay_data:
            vlan_relay[vlan].append(address)

    dhcp_relay_vlan_keys = vlan_relay.keys()
    relay_address_list = ["\n".join(vlan_relay[key]) for key in dhcp_relay_vlan_keys]
    data = {"Interface": dhcp_relay_vlan_keys, "DHCP Relay Address": relay_address_list}
    return tabulate(data, tablefmt='grid', stralign='right', headers='keys') + '\n'


def get_dhcp_relay(table_name, entry_name, with_header):
    if config_db is None:
        return

    config_db.connect()
    table_data = config_db.get_table(table_name)
    if table_data is None:
        return

    if with_header:
        output = get_dhcp_relay_data_with_header(table_data, entry_name)
        print(output)
    else:
        vlans = config_db.get_keys(table_name)
        for vlan in vlans:
            output = get_data(table_data, vlan)
            print(output)


@dhcp_relay_helper.command('ipv6')
def get_dhcpv6_helper_address():
    """Parse through DHCP_RELAY table for each interface in config_db.json and print dhcpv6 helpers in table format"""
    get_dhcp_relay(DHCP_RELAY, DHCPV6_SERVERS, with_header=False)


def get_data(table_data, vlan):
    vlan_data = table_data.get(vlan, {})
    helpers_data = vlan_data.get('dhcpv6_servers')
    addr = {vlan:[]}
    output = ''
    if helpers_data is not None:
        for ip in helpers_data:
            addr[vlan].append(ip)
        output = tabulate({'Interface':[vlan], vlan:addr.get(vlan)}, tablefmt='simple', stralign='right') + '\n'
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


@dhcp_relay_ipv4.command("helper")
def dhcp_relay_ipv4_destination():
    get_dhcp_relay(VLAN, DHCPV4_SERVERS, with_header=True)


@dhcp_relay_ipv6.command("destination")
def dhcp_relay_ipv6_destination():
    get_dhcp_relay(DHCP_RELAY, DHCPV6_SERVERS, with_header=True)


@dhcp_relay_ipv6.command("counters")
@click.option('-i', '--interface', required=False)
def dhcp_relay_ip6counters(interface):
    ipv6_counters(interface)


def register(cli):
    cli.add_command(dhcp4relay_counters)
    cli.add_command(dhcp6relay_counters)
    cli.add_command(dhcp_relay_helper)
    cli.add_command(dhcp_relay)
