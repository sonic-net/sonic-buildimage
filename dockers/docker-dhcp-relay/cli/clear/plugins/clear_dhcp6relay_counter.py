import click
import importlib
dhcp6_relay = importlib.import_module('show.plugins.dhcp-relay')

import utilities_common.cli as clicommon


def clear_dhcp_relay_ipv6_counter(interface):
    counter = dhcp6_relay.DHCPv6_Counter()
    counter_intf = counter.get_interface()

    if interface:
        counter.clear_table(interface)
    else:
        for intf in counter_intf:
            counter.clear_table(intf)


# sonic-clear dhcp6relay_counters
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


def register(cli):
    cli.add_command(dhcp6relay_clear_counters)
    cli.add_command(dhcp_relay)


if __name__ == '__main__':
    dhcp6relay_clear_counters()
    dhcp_relay()
