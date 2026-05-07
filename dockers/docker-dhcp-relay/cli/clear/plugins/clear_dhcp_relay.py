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

# ============================================================
# Constants
# ============================================================

SUPPORTED_DIR = ["TX", "RX"]
COUNTERS_DB_SEPRATOR = ":"
STATE_DB_SEPRATOR = "|"
WAITING_DB_WRITING_RETRY_TIMES = 2

# DHCPv4 dhcpmon counter constants (COUNTERS_DB)
# shared lock for both v4 and v6 — dhcpmon is a single process handling both,
# SIGUSR1/SIGUSR2 pauses all writes globally, so clears must not overlap
DHCPMON_CLEAR_COUNTER_LOCK_FILE = "/tmp/clear_dhcpmon_counter.lock"
DHCPV4_COUNTER_TABLE_PREFIX = "DHCPV4_COUNTER_TABLE"
DHCPV4_COUNTER_UPDATE_STATE_TABLE = "DHCPV4_COUNTER_UPDATE"
SUPPORTED_DHCP_TYPE = [
    "Unknown", "Discover", "Offer", "Request", "Decline", "Ack", "Nak",
    "Release", "Inform", "Bootp", "Malformed", "Ignored"
]

# DHCPv6 dhcpmon counter constants (COUNTERS_DB)

DHCPV6_COUNTER_TABLE_PREFIX = "DHCPV6_COUNTER_TABLE"
DHCPV6_COUNTER_UPDATE_STATE_TABLE = "DHCPV6_COUNTER_UPDATE"
SUPPORTED_DHCPV6_TYPE = [
    "Unknown", "Solicit", "Advertise", "Request", "Confirm",
    "Renew", "Rebind", "Reply", "Release", "Decline",
    "Reconfigure", "Information-Request",
    "Relay-Forward", "Relay-Reply", "Malformed", "Ignored"
]


# ============================================================
# Helper functions for dhcp6relay / dhcp4relay (STATE_DB)
# These clear the relay process counters in STATE_DB,
# NOT the dhcpmon counters in COUNTERS_DB.
# ============================================================

def clear_dhcp_relay_ipv6_counter(interface):
    """Clear dhcp6relay STATE_DB counters."""
    counter = dhcprelay.DHCPv6_Counter()
    counter_intf = counter.get_interface()
    if interface:
        counter.clear_table(interface)
    else:
        for intf in counter_intf:
            counter.clear_table(intf)


def clear_dhcp_relay_ipv4_vlan_counter(direction, pkt_type, interface):
    """Clear dhcp4relay STATE_DB per-vlan counters."""
    counter = dhcprelay.DHCPv4_Counter()
    counter.clear_table(direction, pkt_type, interface)


# ============================================================
# Helper functions for dhcpmon counters (COUNTERS_DB)
# Shared by both DHCPv4 and DHCPv6 clear workflows.
#
# The dhcpmon clear workflow:
#   1. CLI sends SIGUSR1 to dhcpmon -> dhcpmon pauses DB writes
#   2. CLI zeroes counter entries in COUNTERS_DB
#   3. CLI sends SIGUSR2 to dhcpmon -> dhcpmon reads DB back
#      into cache (picking up the zeroed values), then resumes
# ============================================================

def is_vlan_interface_valid(vlan_interface, db):
    if vlan_interface == "":
        return True
    if not db.exists(db.CONFIG_DB, "VLAN|{}".format(vlan_interface)):
        return False
    return True


def notify_dhcpmon_processes(vlan_interface, sig):
    """Send a signal to all dhcpmon processes (or a specific vlan)."""
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
            match = re.search(
                r'-id\s+(Vlan\d+)', " ".join(proc.cmdline())
            )
            if not match:
                continue
            if vlan_interface != "" and vlan_interface != match.group(1):
                continue
            pid = proc.pid
            os.kill(int(pid), sig)
            syslog.syslog(
                syslog.LOG_INFO,
                "Clear DHCP counter: Sent signal {} to pid {}".format(
                    sig, pid
                )
            )
            notified_vlans.add(match.group(1))
        except (psutil.NoSuchProcess, psutil.ZombieProcess):
            pass

    return notified_vlans


def is_write_db_paused(db, table_name, vlan_interface):
    return db.get(
        db.STATE_DB,
        table_name + STATE_DB_SEPRATOR + vlan_interface,
        "pause_write_to_db"
    ) == "done"


def get_paused_vlans(db, table_name, vlans):
    """Wait for dhcpmon to confirm it has paused DB writes."""
    paused_vlan = set()
    for _ in range(WAITING_DB_WRITING_RETRY_TIMES + 1):
        for vlan in vlans:
            if is_write_db_paused(db, table_name, vlan):
                paused_vlan.add(vlan)
        if len(paused_vlan) == len(vlans):
            break
        time.sleep(1)
    else:
        click.echo(
            "Skip clearing counter for {} due to writing "
            "COUNTERS_DB hasn't been stopped.".format(
                vlans - paused_vlan
            )
        )
    return paused_vlan


def clear_dhcpmon_db_counters(
    db, direction, type, paused_vlans,
    counter_table_prefix, supported_types
):
    """Zero out dhcpmon counters in COUNTERS_DB."""
    types = supported_types if type is None else [type]
    directions = SUPPORTED_DIR if direction is None else [direction]
    for vlan in paused_vlans:
        counters_key = (
            counter_table_prefix
            + COUNTERS_DB_SEPRATOR + vlan + "*"
        )
        for key in (db.keys(db.COUNTERS_DB, counters_key) or []):
            if not (":{}:".format(vlan) in key
                    or key.endswith(vlan)):
                continue
            for dir in directions:
                count_str = db.get(db.COUNTERS_DB, key, dir)
                if count_str is None:
                    count_str = "{}"
                count_str = count_str.replace("\'", '"')
                count_obj = json.loads(count_str)
                for current_type in types:
                    count_obj[current_type] = "0"
                count_str = json.dumps(count_obj).replace('"', "\'")
                db.set(db.COUNTERS_DB, key, dir, count_str)


def clear_state_db_flags(db, state_table):
    """Clear counter update flags in STATE_DB."""
    for key in (db.keys(
        db.STATE_DB,
        state_table + STATE_DB_SEPRATOR + "*"
    ) or []):
        db.delete(db.STATE_DB, key)


def get_failed_vlans_for_table(db, state_table, vlans):
    """Check which vlans failed to sync cache counters."""
    failed_vlans = {vlan: set(["RX", "TX"]) for vlan in vlans}
    for _ in range(WAITING_DB_WRITING_RETRY_TIMES + 1):
        for vlan in vlans:
            if vlan not in failed_vlans:
                continue
            prefix = state_table + STATE_DB_SEPRATOR + vlan
            if db.get(db.STATE_DB, prefix,
                      "rx_cache_update") == "done":
                failed_vlans[vlan].discard("RX")
            if db.get(db.STATE_DB, prefix,
                      "tx_cache_update") == "done":
                failed_vlans[vlan].discard("TX")
            if not failed_vlans[vlan]:
                del failed_vlans[vlan]
        if len(failed_vlans) == 0:
            return failed_vlans
        time.sleep(1)
    return failed_vlans


def clear_dhcpmon_counters(
    db, interface, dir, type,
    lock_file_path, state_table, counter_table_prefix,
    supported_types, version_label
):
    """
    Clear dhcpmon counters in COUNTERS_DB. Shared by v4 and v6.

    Workflow:
      1. Acquire file lock (prevent concurrent clears)
      2. Clear stale STATE_DB flags
      3. SIGUSR1 -> dhcpmon pauses DB writes
      4. Wait for dhcpmon to confirm pause
      5. Zero out counter entries in COUNTERS_DB
      6. SIGUSR2 -> dhcpmon syncs DB back to cache
      7. Verify dhcpmon completed the sync
    """
    ctx = click.get_current_context()
    if os.geteuid() != 0:
        ctx.fail(
            "Clearing {} counters requires sudo privileges"
            .format(version_label)
        )
        return
    if not is_vlan_interface_valid(interface, db.db):
        ctx.fail("{} doesn't exist".format(interface))
        return
    with open(lock_file_path, "w") as lock_file:
        locked = False
        try:
            fcntl.flock(lock_file, fcntl.LOCK_EX | fcntl.LOCK_NB)
            locked = True
            clear_state_db_flags(db.db, state_table)
            notified_vlans = notify_dhcpmon_processes(
                interface, signal.SIGUSR1
            )
            paused_vlans = get_paused_vlans(
                db.db, state_table, notified_vlans
            )
            clear_dhcpmon_db_counters(
                db.db, dir, type, paused_vlans,
                counter_table_prefix, supported_types
            )
            notify_dhcpmon_processes(interface, signal.SIGUSR2)
            failed_vlans = get_failed_vlans_for_table(
                db.db, state_table, paused_vlans
            )
            if failed_vlans:
                failed_msg = ", ".join(
                    ["{}({})".format(vlan, ",".join(dirs))
                     for vlan, dirs in failed_vlans.items()]
                )
                ctx.fail(
                    "Failed to clear counter for {}".format(
                        failed_msg
                    )
                )
                return
            click.echo(
                "Clear {} relay counter done".format(version_label)
            )
        except BlockingIOError:
            ctx.fail(
                "Cannot lock {}; another user is already clearing "
                "{} relay counters simultaneously".format(
                    lock_file_path, version_label
                )
            )
        finally:
            if locked:
                clear_state_db_flags(db.db, state_table)
            fcntl.flock(lock_file, fcntl.LOCK_UN)


# ============================================================
# Standalone commands
# These are older entry points kept for backward compatibility.
# ============================================================

# sonic-clear dhcp6relay_counters
# Clears dhcp6relay STATE_DB counters (NOT dhcpmon COUNTERS_DB)
@click.group(cls=clicommon.AliasedGroup)
def dhcp6relay_clear():
    pass


@dhcp6relay_clear.command('dhcp6relay_counters')
@click.option('-i', '--interface', required=False)
def dhcp6relay_clear_counters(interface):
    """ Clear dhcp6relay message counts """
    clear_dhcp_relay_ipv6_counter(interface)


# sonic-clear dhcp4relay-vlan-counters
# Clears dhcp4relay STATE_DB per-vlan counters (NOT dhcpmon)
@click.group(cls=clicommon.AliasedGroup)
def dhcp4relay_clear():
    pass


@dhcp4relay_clear.command('dhcp4relay-vlan-counters')
@click.option(
    '-d', '--direction', required=False,
    type=click.Choice(['TX', 'RX']),
    help="Specify TX(egress) or RX(ingress)"
)
@click.option(
    '-t', '--type', required=False,
    type=click.Choice(dhcprelay.dhcpv4_messages),
    help="Specify DHCP packet counter type"
)
@click.argument("vlan_interface", required=False)
def dhcp4relay_clear_vlan_counters(direction, type, vlan_interface):
    """ Clear dhcp_relay ipv4 message counts """
    clear_dhcp_relay_ipv4_vlan_counter(direction, type, vlan_interface)


# ============================================================
# Unified sonic-clear dhcp_relay commands
# ============================================================

@click.group(cls=clicommon.AliasedGroup, name="dhcp_relay")
def dhcp_relay():
    pass


# ------------------------------------------------------------
# sonic-clear dhcp_relay ipv6 counters
# Clears dhcpmon DHCPv6 counters in COUNTERS_DB
# ------------------------------------------------------------

@dhcp_relay.group(cls=clicommon.AliasedGroup, name="ipv6")
def dhcp_relay_ipv6():
    pass


@dhcp_relay_ipv6.command('counters')
@click.option('-i', '--interface', required=False, default="")
@click.option(
    '--dir', type=click.Choice(SUPPORTED_DIR), required=False
)
@click.option(
    '--type', type=click.Choice(SUPPORTED_DHCPV6_TYPE),
    required=False
)
@clicommon.pass_db
def clear_dhcp_relay_ipv6_counters(db, interface, dir, type):
    """ Clear dhcpmon ipv6 counters (COUNTERS_DB) """
    clear_dhcpmon_counters(
        db, interface, dir, type,
        lock_file_path=DHCPMON_CLEAR_COUNTER_LOCK_FILE,
        state_table=DHCPV6_COUNTER_UPDATE_STATE_TABLE,
        counter_table_prefix=DHCPV6_COUNTER_TABLE_PREFIX,
        supported_types=SUPPORTED_DHCPV6_TYPE,
        version_label="DHCPv6"
    )


# ------------------------------------------------------------
# sonic-clear dhcp_relay ipv4 counters
# Clears dhcpmon DHCPv4 counters in COUNTERS_DB
# ------------------------------------------------------------

@dhcp_relay.group(cls=clicommon.AliasedGroup, name="ipv4")
def dhcp_relay_ipv4():
    pass


@dhcp_relay_ipv4.command('counters')
@click.option('-i', '--interface', required=False, default="")
@click.option(
    '--dir', type=click.Choice(SUPPORTED_DIR), required=False
)
@click.option(
    '--type', type=click.Choice(SUPPORTED_DHCP_TYPE),
    required=False
)
@clicommon.pass_db
def clear_dhcp_relay_ipv4_counters(db, interface, dir, type):
    """ Clear dhcpmon ipv4 counters (COUNTERS_DB) """
    clear_dhcpmon_counters(
        db, interface, dir, type,
        lock_file_path=DHCPMON_CLEAR_COUNTER_LOCK_FILE,
        state_table=DHCPV4_COUNTER_UPDATE_STATE_TABLE,
        counter_table_prefix=DHCPV4_COUNTER_TABLE_PREFIX,
        supported_types=SUPPORTED_DHCP_TYPE,
        version_label="DHCPv4"
    )


# ============================================================
# Plugin registration
# ============================================================

def register(cli):
    cli.add_command(dhcp6relay_clear_counters)
    cli.add_command(dhcp4relay_clear_vlan_counters)
    cli.add_command(dhcp_relay)


if __name__ == '__main__':
    dhcp6relay_clear_counters()
    dhcp4relay_clear_vlan_counters()
    dhcp_relay()
