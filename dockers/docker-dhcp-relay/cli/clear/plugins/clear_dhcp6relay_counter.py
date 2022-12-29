import sys
import click
import importlib
dhcp6_relay = importlib.import_module('show.plugins.dhcp-relay')

import utilities_common.cli as clicommon


# sonic-clear dhcp6relay_counters
@click.group(cls=clicommon.AliasedGroup)
def dhcp6relay_clear():
    pass

@dhcp6relay_clear.command('dhcp6relay_counters')
@click.option('-i', '--interface', required=False)
def dhcp6relay_clear_counters(interface):
    """ Clear dhcp6relay message counts """

    counter = dhcp6_relay.DHCPv6_Counter()
    counter_intf = counter.get_interface()

    if interface:
        counter.clear_table(interface)
    else:
        for intf in counter_intf:
                counter.clear_table(intf)

def register(cli):
    cli.add_command(dhcp6relay_clear_counters)

if __name__ == '__main__':
    dhcp6relay_clear_counters()
