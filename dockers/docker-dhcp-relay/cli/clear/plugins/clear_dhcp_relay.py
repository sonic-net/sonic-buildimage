import click
import fcntl
import importlib
import json
import signal
import syslog
import os
dhcp6_relay = importlib.import_module('show.plugins.dhcp-relay')

import utilities_common.cli as clicommon

DHCPV4_CLEAR_COUNTER_LOCK_FILE = "/tmp/clear_dhcpv4_counter.lock"
SUPPORTED_DHCP_TYPE = [
    "Unknown", "Discover", "Offer", "Request", "Decline", "Ack", "Nak", "Release", "Inform", "Bootp"
]
SUPPORTED_DIR = ["TX", "RX"]
DHCPV4_COUNTER_TABLE_PREFIX = "DHCPV4_COUNTER_TABLE"
COUNTERS_DB_SEPRATOR = ":"


def clear_dhcp_relay_ipv6_counter(interface):
    counter = dhcp6_relay.DHCPv6_Counter()
    counter_intf = counter.get_interface()
    if interface:
        counter.clear_table(interface)
    else:
        for intf in counter_intf:
            counter.clear_table(intf)


def is_vlan_interface_valid(vlan_interface, db):
    # If not vlan interface specified, treat it as valid
    if vlan_interface == "":
        return True
    # If vlan interface is not in config_db VLAN table, treat it as invalid
    if not db.exists(db.CONFIG_DB, "VLAN|{}".format(vlan_interface)):
        return False
    return True


def clear_dhcpv4_db_counters(db, vlan_interface, direction, type):
    types = SUPPORTED_DHCP_TYPE if type is None else [type]
    directions = SUPPORTED_DIR if direction is None else [direction]
    counters_key = DHCPV4_COUNTER_TABLE_PREFIX + COUNTERS_DB_SEPRATOR + vlan_interface + "*"
    for key in db.keys(db.COUNTERS_DB, counters_key):
        # Make sure we clear the correct interface
        # Sample: If we want to clear Vlan100, but there is a Vlan1000 exists too, we need to exclude Vlan1000
        if not (":{}:".format(vlan_interface) in key or key.endswith(vlan_interface)):
            continue
        for dir in directions:
            count_str = db.get(db.COUNTERS_DB, key, dir)
            if count_str is None:
                # Would add new entry if corresponding dir doesn't exist in COUNTERS_DB
                count_str = "{}"
            count_str = count_str.replace("\'", "\"")
            count_obj = json.loads(count_str)
            for current_type in types:
                # Would set count for types to 0 no matter whether this type exists in COUNTERS_DB
                count_obj[current_type] = "0"
            count_str = json.dumps(count_obj).replace("\"", "\'")
            db.set(db.COUNTERS_DB, key, dir, count_str)


def notify_dhcpmon_processes(vlan_interface, sig):
    try:
        signal.Signals(sig)
    except ValueError:
        click.echo("Invalid signal, skip it")
        return

    cmd = "ps aux | grep dhcpmon"
    if vlan_interface:
        cmd += " | grep '\-id {} '".format(vlan_interface)
    cmd += " | grep \-v grep | awk \'{print $2}\'"
    out, _ = clicommon.run_command(cmd, shell=True, return_cmd=True)
    out = out.strip()
    if len(out) == 0:
        return
    for pid in out.split("\n"):
        os.kill(int(pid), sig)
        syslog.syslog(syslog.LOG_INFO, "Clear DHCPv4 counter: Sent signal {} to pid {}".format(sig, pid))


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


@dhcp_relay.group(cls=clicommon.AliasedGroup, name="ipv4")
def dhcp_relay_ipv4():
    pass


@dhcp_relay_ipv4.command('counters')
@click.option('-i', '--interface', required=False, default="")
@click.option('--dir', type=click.Choice(SUPPORTED_DIR), required=False)
@click.option('--type', type=click.Choice(SUPPORTED_DHCP_TYPE), required=False)
@clicommon.pass_db
def clear_dhcp_relay_ipv4_counters(db, interface, dir, type):
    """ Clear dhcp_relay ipv4 counts """
    ctx = click.get_current_context()
    if os.geteuid() != 0:
        ctx.fail("Clear DHCPv4 counter need to be run with sudo permission")
        return
    if not is_vlan_interface_valid(interface, db.db):
        ctx.fail("{} doesn't exist".format(interface))
        return
    with open(DHCPV4_CLEAR_COUNTER_LOCK_FILE, "w") as lock_file:
        try:
            fcntl.flock(lock_file, fcntl.LOCK_EX | fcntl.LOCK_NB)
            notify_dhcpmon_processes(interface, signal.SIGUSR1)
            clear_dhcpv4_db_counters(db.db, interface, dir, type)
            notify_dhcpmon_processes(interface, signal.SIGUSR2)
            click.echo("Clear DHCPv4 relay counter done")
        except BlockingIOError:
            ctx.fail("Cannot lock {}, seems another user is clearing DHCPv4 relay counter simultaneous"
                     .format(DHCPV4_CLEAR_COUNTER_LOCK_FILE))
        finally:
            fcntl.flock(lock_file, fcntl.LOCK_UN)


def register(cli):
    cli.add_command(dhcp6relay_clear_counters)
    cli.add_command(dhcp_relay)


if __name__ == '__main__':
    dhcp6relay_clear_counters()
    dhcp_relay()
