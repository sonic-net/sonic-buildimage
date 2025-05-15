import click
import importlib
dhcprelay = importlib.import_module('show.plugins.dhcp-relay')

import utilities_common.cli as clicommon


def clear_dhcp_relay_ipv6_counter(interface):
    counter = dhcprelay.DHCPv6_Counter()
    counter_intf = counter.get_interface()

    if interface:
        counter.clear_table(interface)
    else:
        for intf in counter_intf:
            counter.clear_table(intf)

def clear_dhcp_relay_ipv4_counter(direction, pkt_type, interface):
    counter = dhcprelay.DHCPv4_Counter()
    counter.clear_table(direction, pkt_type, interface)

# sonic-clear dhcp6relay_counters
@click.group(cls=clicommon.AliasedGroup)
def dhcp4relay_clear():
    pass

@click.group(cls=clicommon.AliasedGroup)
def dhcp6relay_clear():
    pass


@dhcp6relay_clear.command('dhcp6relay_counters')
@click.option('-i', '--interface', required=False)
def dhcp6relay_clear_counters(interface):
    """ Clear dhcp6relay message counts """
    clear_dhcp_relay_ipv6_counter(interface)


@click.group(cls=clicommon.AliasedGroup, name="dhcp_relay")
def dhcp_relay():
    pass


@dhcp_relay.group(cls=clicommon.AliasedGroup, name="ipv6")
def dhcp_relay_ipv6():
    pass




@dhcp_relay_ipv6.command('counters')
@click.option('-i', '--interface', required=False)
def clear_dhcp_relay_ipv6_counters(interface):
    """ Clear dhcp_relay ipv6 message counts """
    clear_dhcp_relay_ipv6_counter(interface)

@dhcp4relay_clear.command('dhcp4relay_counters')
@click.option('-d', '--direction', required=False, type=click.Choice(['TX', 'RX']), help="Specify TX(egress) or RX(ingress)")
@click.option('-t', '--type', required=False, type=click.Choice(dhcprelay.dhcpv4_messages), help="Specify DHCP packet counter type")
@click.argument("vlan_interface", required=False)
def dhcp4relay_clear_counters(direction, type, vlan_interface):
    """ Clear dhcp_relay ipv4 message counts """
    clear_dhcp_relay_ipv4_counter(direction, type, vlan_interface)

def register(cli):
    cli.add_command(dhcp6relay_clear_counters)
    cli.add_command(dhcp4relay_clear_counters)
    cli.add_command(dhcp_relay)


if __name__ == '__main__':
    dhcp6relay_clear_counters()
    dhcp4relay_clear_counters()
    dhcp_relay()
