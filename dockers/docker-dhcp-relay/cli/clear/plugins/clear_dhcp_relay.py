import click
import fcntl
import importlib
import json
import signal
import syslog
import os
import psutil
import time
import re
dhcprelay = importlib.import_module('show.plugins.dhcp-relay')

import utilities_common.cli as clicommon

DHCPV4_CLEAR_COUNTER_LOCK_FILE = "/tmp/clear_dhcpv4_counter.lock"
SUPPORTED_DHCP_TYPE = [
    "Unknown", "Discover", "Offer", "Request", "Decline", "Ack", "Nak", "Release", "Inform", "Bootp"
]
SUPPORTED_DIR = ["TX", "RX"]
DHCPV4_COUNTER_TABLE_PREFIX = "DHCPV4_COUNTER_TABLE"
COUNTERS_DB_SEPRATOR = ":"
DHCPV4_COUNTER_UPDATE_STATE_TABLE = "DHCPV4_COUNTER_UPDATE"
STATE_DB_SEPRATOR = "|"
WAITING_DB_WRITING_RETRY_TIMES = 2


def clear_dhcp_relay_ipv6_counter(interface):
    counter = dhcprelay.DHCPv6_Counter()
    counter_intf = counter.get_interface()
    if interface:
        counter.clear_table(interface)
    else:
        for intf in counter_intf:
            counter.clear_table(intf)

def clear_dhcp_relay_ipv4_vlan_counter(direction, pkt_type, interface):
    counter = dhcprelay.DHCPv4_Counter()
    counter.clear_table(direction, pkt_type, interface)

def is_vlan_interface_valid(vlan_interface, db):
    # If not vlan interface specified, treat it as valid
    if vlan_interface == "":
        return True
    # If vlan interface is not in config_db VLAN table, treat it as invalid
    if not db.exists(db.CONFIG_DB, "VLAN|{}".format(vlan_interface)):
        return False
    return True


def clear_dhcpv4_db_counters(db, direction, type, paused_vlans):
    types = SUPPORTED_DHCP_TYPE if type is None else [type]
    directions = SUPPORTED_DIR if direction is None else [direction]
    for vlan in paused_vlans:
        counters_key = DHCPV4_COUNTER_TABLE_PREFIX + COUNTERS_DB_SEPRATOR + vlan + "*"
        for key in db.keys(db.COUNTERS_DB, counters_key):
            # Make sure we clear the correct interface
            # Sample: If we want to clear Vlan100, but there is a Vlan1000 exists too, we need to exclude Vlan1000
            if not (":{}:".format(vlan) in key or key.endswith(vlan)):
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
    notified_vlans = set()
    try:
        signal.Signals(sig)
    except ValueError:
        click.echo("Invalid signal, skip it")
        return

    for proc in psutil.process_iter():
        try:
            if proc.name() != "dhcpmon":
                continue
            match = re.search(r'-id\s+(Vlan\d+)', " ".join(proc.cmdline()))
            if not match:
                continue
            if vlan_interface != "" and vlan_interface != match.group(1):
                continue
            pid = proc.pid
            os.kill(int(pid), sig)
            syslog.syslog(syslog.LOG_INFO, "Clear DHCPv4 counter: Sent signal {} to pid {}".format(sig, pid))
            notified_vlans.add(match.group(1))
        except (psutil.NoSuchProcess, psutil.ZombieProcess):
            pass

    return notified_vlans


def is_write_db_paused(db, table_name, vlan_interface):
    return db.get(db.STATE_DB, table_name + STATE_DB_SEPRATOR + vlan_interface, "pause_write_to_db") == "done"


def get_paused_vlans(db, table_name, vlans):
    paused_vlan = set()
    for _ in range(WAITING_DB_WRITING_RETRY_TIMES + 1):
        for vlan in vlans:
            if is_write_db_paused(db, table_name, vlan):
                paused_vlan.add(vlan)
        if len(paused_vlan) == len(vlans):
            break
        time.sleep(1)
    else:
        click.echo("Skip clearing counter for {} due to writing COUNTERS_DB hasn't been stopped."
                   .format(vlans - paused_vlan))
    return paused_vlan


def clear_writing_state_db_flag(db):
    for key in db.keys(db.STATE_DB, DHCPV4_COUNTER_UPDATE_STATE_TABLE + STATE_DB_SEPRATOR + "*"):
        db.delete(db.STATE_DB, key)


def get_failed_vlans(db, vlans):
    failed_vlans = {vlan: set(["RX", "TX"]) for vlan in vlans}
    for _ in range(WAITING_DB_WRITING_RETRY_TIMES + 1):
        for vlan in vlans:
            if db.get(db.STATE_DB, DHCPV4_COUNTER_UPDATE_STATE_TABLE + STATE_DB_SEPRATOR + vlan,
                      "rx_cache_update") == "done":
                failed_vlans[vlan].discard("RX")
            if db.get(db.STATE_DB, DHCPV4_COUNTER_UPDATE_STATE_TABLE + STATE_DB_SEPRATOR + vlan,
                      "tx_cache_update") == "done":
                failed_vlans[vlan].discard("TX")
            if not failed_vlans[vlan]:
                del failed_vlans[vlan]
        if len(failed_vlans) == 0:
            return failed_vlans
        time.sleep(1)
    return failed_vlans


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

# sonic-clear dhcp6relay_counters
@click.group(cls=clicommon.AliasedGroup)
def dhcp4relay_clear():
    pass

@dhcp4relay_clear.command('dhcp4relay-vlan-counters')
@click.option('-d', '--direction', required=False, type=click.Choice(['TX', 'RX']), help="Specify TX(egress) or RX(ingress)")
@click.option('-t', '--type', required=False, type=click.Choice(dhcprelay.dhcpv4_messages), help="Specify DHCP packet counter type")
@click.argument("vlan_interface", required=False)
def dhcp4relay_clear_vlan_counters(direction, type, vlan_interface):
    """ Clear dhcp_relay ipv4 message counts """
    clear_dhcp_relay_ipv4_vlan_counter(direction, type, vlan_interface)

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
            clear_writing_state_db_flag(db.db)
            notified_vlans = notify_dhcpmon_processes(interface, signal.SIGUSR1)
            # Fetch the vlans that has been stopped to writing COUNTERS_DB
            paused_vlans = get_paused_vlans(db.db, DHCPV4_COUNTER_UPDATE_STATE_TABLE, notified_vlans)
            clear_dhcpv4_db_counters(db.db, dir, type, paused_vlans)
            notify_dhcpmon_processes(interface, signal.SIGUSR2)
            failed_vlans = get_failed_vlans(db.db, paused_vlans)
            if failed_vlans:
                failed_msg = ", ".join(["{}({})".format(vlan, ",".join(dirs)) for vlan, dirs in failed_vlans.items()])
                ctx.fail("Failed to clear counter for {}".format(failed_msg))
                return
            click.echo("Clear DHCPv4 relay counter done")
        except BlockingIOError:
            ctx.fail("Cannot lock {}, seems another user is clearing DHCPv4 relay counter simultaneous"
                     .format(DHCPV4_CLEAR_COUNTER_LOCK_FILE))
        finally:
            clear_writing_state_db_flag(db.db)
            fcntl.flock(lock_file, fcntl.LOCK_UN)


def register(cli):
    cli.add_command(dhcp6relay_clear_counters)
    cli.add_command(dhcp4relay_clear_vlan_counters)
    cli.add_command(dhcp_relay)


if __name__ == '__main__':
    dhcp6relay_clear_counters()
    dhcp4relay_clear_vlan_counters()
    dhcp_relay()
