import fcntl
import ipaddress
import json
import math
import os
import subprocess
import time
import uuid
from contextlib import contextmanager

import click
import utilities_common.cli as clicommon
from swsscommon import swsscommon


KEA_CONTROL_SOCKET = "/run/kea/kea4-ctrl-socket"
LEASE_CLEAR_LOCK = "/run/lock/sonic-dhcp-server-lease-clear.lock"
STATE_DB_TIMEOUT = 20
LEASE_SYNC_TABLE = "DHCP_SERVER_IPV4_LEASE_SYNC"
LEASE_SYNC_KEY = LEASE_SYNC_TABLE + "|GLOBAL"
KEA_SOCKET_CLIENT = """
import json
import socket
import sys

with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
    connection.settimeout(5)
    connection.connect(sys.argv[1])
    connection.sendall(sys.argv[2].encode("utf-8"))
    response = b""
    while True:
        chunk = connection.recv(65536)
        if not chunk:
            raise RuntimeError("Kea closed the connection without a complete JSON response")
        response += chunk
        try:
            result = json.loads(response)
        except (json.JSONDecodeError, UnicodeDecodeError):
            continue
        print(json.dumps(result))
        break
"""


def _container_command(*command):
    try:
        result = subprocess.run(
            ["docker", "exec", "dhcp_server", *command],
            capture_output=True, text=True, timeout=15, check=True)
    except subprocess.CalledProcessError as error:
        raise click.ClickException(
            "DHCP server command failed: {}".format(
                (error.stderr or error.stdout or str(error)).strip())) from error
    except (OSError, subprocess.TimeoutExpired) as error:
        raise click.ClickException("Cannot contact DHCP server: {}".format(error)) from error
    return result.stdout


def _kea_command(command, arguments=None):
    request = {"command": command}
    if arguments is not None:
        request["arguments"] = arguments
    output = _container_command(
        "python3", "-c", KEA_SOCKET_CLIENT, KEA_CONTROL_SOCKET, json.dumps(request))
    try:
        response = json.loads(output)
    except ValueError as error:
        raise click.ClickException("Invalid JSON response from Kea") from error
    if not isinstance(response, dict) or type(response.get("result")) is not int:
        raise click.ClickException("Invalid command response from Kea")
    if response["result"] not in (0, 3):
        raise click.ClickException(
            "Kea {} failed: {}".format(command, response.get("text", response)))
    return response


@contextmanager
def _lease_clear_lock():
    try:
        descriptor = os.open(LEASE_CLEAR_LOCK, os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW, 0o600)
    except OSError as error:
        raise click.ClickException("Cannot lock DHCP lease clearing: {}".format(error)) from error
    with os.fdopen(descriptor, "a") as lock_file:
        try:
            fcntl.flock(lock_file, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as error:
            raise click.ClickException("Another DHCP lease clear is in progress") from error
        yield


def _subnet_id(dbconn, dhcp_interface):
    if not dbconn.exists("CONFIG_DB", "DHCP_SERVER_IPV4|" + dhcp_interface):
        raise click.ClickException("DHCP interface {} is not configured".format(dhcp_interface))
    config = _kea_command("config-get").get("arguments", {}).get("Dhcp4", {})
    subnets = config.get("subnet4", [])
    vlan_id = dbconn.get("CONFIG_DB", "VLAN|" + dhcp_interface, "vlanid")
    if vlan_id is not None and dhcp_interface == "Vlan" + str(vlan_id):
        try:
            vlan_id = int(vlan_id)
        except ValueError as error:
            raise click.ClickException("Invalid VLAN ID for {}".format(dhcp_interface)) from error
        subnets = [subnet for subnet in subnets if subnet.get("id") == vlan_id]
    else:
        midplane = dbconn.get_all("CONFIG_DB", "MID_PLANE_BRIDGE|GLOBAL")
        if midplane.get("bridge") != dhcp_interface:
            raise click.ClickException("Cannot resolve subnet for {}".format(dhcp_interface))
        network = str(ipaddress.IPv4Network(midplane["ip_prefix"], strict=False))
        subnets = [subnet for subnet in subnets if subnet.get("subnet") == network]
    if len(subnets) != 1 or type(subnets[0].get("id")) is not int or subnets[0]["id"] <= 0:
        raise click.ClickException("No unique active Kea subnet for {}".format(dhcp_interface))
    return subnets[0]["id"]


def _server_leases(subnet_id, ip_address):
    response = _kea_command("lease4-get-all", {"subnets": [subnet_id]})
    if response["result"] == 3:
        return []
    leases = response.get("arguments", {}).get("leases")
    if not isinstance(leases, list) or any(
            not isinstance(lease, dict) or
            not {"subnet-id", "ip-address"} <= lease.keys()
            for lease in leases):
        raise click.ClickException("Invalid lease list from Kea")
    return [
        lease for lease in leases
        if lease["subnet-id"] == subnet_id
        and (ip_address is None or lease["ip-address"] == ip_address)
    ]


def _state_leases(dbconn, dhcp_interface, ip_address):
    leases = []
    prefix = "DHCP_SERVER_IPV4_LEASE|" + dhcp_interface + "|"
    for key in dbconn.keys("STATE_DB", "DHCP_SERVER_IPV4_LEASE|*"):
        if not key.startswith(prefix):
            continue
        entry = dbconn.get_all("STATE_DB", key)
        if ip_address is None or entry.get("ip") == ip_address:
            leases.append(key)
    return leases


@contextmanager
def _lease_state_events():
    try:
        state_db = swsscommon.DBConnector("STATE_DB", 1000)
        subscriber = swsscommon.SubscriberStateTable(state_db, LEASE_SYNC_TABLE)
        selector = swsscommon.Select()
        selector.addSelectable(subscriber)
        try:
            yield subscriber, selector
        finally:
            selector.removeSelectable(subscriber)
    except RuntimeError as error:
        raise click.ClickException("Cannot monitor DHCP lease state: {}".format(error)) from error


def _sync_state_db(dbconn, dhcp_interface, ip_address, subscriber, selector):
    """Wait until dhcpservd completes a lease refresh requested after the Kea change."""
    # dhcpservd acknowledges this ID only after a refresh that read it before the
    # lease file, so stale in-flight refreshes cannot satisfy the wait.
    request_id = uuid.uuid4().hex
    dbconn.set("STATE_DB", LEASE_SYNC_KEY, "request_id", request_id)
    deadline = time.monotonic() + STATE_DB_TIMEOUT
    next_signal = 0
    # The daemon is the sole State DB writer. Ask it to reread Kea's persisted
    # deletion records rather than deleting Redis keys that it could resurrect.
    while True:
        now = time.monotonic()
        if now >= next_signal:
            _container_command(
                "supervisorctl", "signal", "USR1", "dhcp-server-ipv4:dhcpservd")
            # SWSS may restart its five-second epoll wait on a signal before
            # Python can run the handler, which can also throttle for two seconds.
            # Faster retries can keep postponing the pending handler indefinitely.
            next_signal = time.monotonic() + 8
        if dbconn.get("STATE_DB", LEASE_SYNC_KEY, "completed_id") == request_id:
            if _state_leases(dbconn, dhcp_interface, ip_address):
                raise click.ClickException(
                    "Matching leases still exist in State DB; check for clients reacquiring addresses")
            return
        now = time.monotonic()
        if now >= deadline:
            raise click.ClickException(
                "Kea leases were cleared, but State DB synchronization timed out; "
                "check dhcpservd before retrying")
        timeout_ms = max(1, math.ceil((min(next_signal, deadline) - now) * 1000))
        state, _ = selector.select(timeout_ms)
        if state == swsscommon.Select.OBJECT:
            subscriber.pops()
        elif state != swsscommon.Select.TIMEOUT:
            raise click.ClickException("Cannot monitor DHCP lease state: select() failed")


@click.group(cls=clicommon.AliasedGroup, name="dhcp_server", invoke_without_command=True)
@clicommon.pass_db
def dhcp_server(db):
    """Clear DHCP server state."""
    ctx = click.get_current_context()
    if db.db.get("CONFIG_DB", "FEATURE|dhcp_server", "state") != "enabled":
        ctx.fail("Feature dhcp_server is not enabled")
    if ctx.invoked_subcommand is None:
        click.echo(ctx.get_help())


@dhcp_server.group(cls=clicommon.AliasedGroup)
def ipv4():
    """Clear IPv4 DHCP server state."""
    pass


@ipv4.command()
@click.argument("dhcp_interface")
@click.argument("ip_address", required=False)
@click.option("--all", "clear_all", is_flag=True, help="Clear all leases on the interface.")
@clicommon.pass_db
def lease(db, dhcp_interface, ip_address, clear_all):
    """Delete leases from Kea and State DB without restarting the server.

    Supply exactly one of IP_ADDRESS or --all. Root privileges are
    required. Disconnect the old device first: clearing a lease does not revoke
    its address, and a connected client can immediately acquire another lease.
    """
    if sum((ip_address is not None, clear_all)) != 1:
        raise click.UsageError("Supply exactly one of IP_ADDRESS or --all")
    if ip_address is not None:
        try:
            ip_address = str(ipaddress.IPv4Address(ip_address))
        except ipaddress.AddressValueError as error:
            raise click.BadParameter("Must be an IPv4 address", param_hint="IP_ADDRESS") from error
    if os.geteuid() != 0:
        raise click.ClickException("Root privileges are required; run with sudo")
    with _lease_clear_lock():
        subnet_id = _subnet_id(db.db, dhcp_interface)
        # Establish the subscription before deletion so fast updates are not missed.
        with _lease_state_events() as (subscriber, selector):
            if not _server_leases(subnet_id, ip_address):
                if _state_leases(db.db, dhcp_interface, ip_address):
                    _sync_state_db(db.db, dhcp_interface, ip_address, subscriber, selector)
                raise click.ClickException("No matching Kea lease on {}".format(dhcp_interface))
            if clear_all:
                response = _kea_command("lease4-wipe", {"subnet-id": subnet_id})
            else:
                response = _kea_command("lease4-del", {"ip-address": ip_address})
            _sync_state_db(db.db, dhcp_interface, ip_address, subscriber, selector)
            if response["result"] != 0:
                raise click.ClickException("No matching Kea lease on {}".format(dhcp_interface))
            if _server_leases(subnet_id, ip_address):
                raise click.ClickException(
                    "Matching leases still exist in Kea; check for clients reacquiring addresses")
    click.echo("Cleared matching lease(s) on {} from Kea and State DB.".format(dhcp_interface))


def register(cli):
    cli.add_command(dhcp_server)


if __name__ == '__main__':
    dhcp_server()
