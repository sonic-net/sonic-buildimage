import click
import re
from tabulate import tabulate
import utilities_common.cli as clicommon


import ipaddress
from datetime import datetime
import fnmatch


def ts_to_str(ts):
    return datetime.fromtimestamp(int(ts)).strftime("%Y-%m-%d %H:%M:%S")


@click.group(cls=clicommon.AbbreviationGroup, name="dhcp_server", invoke_without_command=True)
@clicommon.pass_db
def dhcp_server(db):
    """Show dhcp_server related info"""
    ctx = click.get_current_context()
    dbconn = db.db
    if dbconn.get("CONFIG_DB", "FEATURE|dhcp_server", "state") != "enabled":
        ctx.fail("Feature dhcp_server is not enabled")
    if ctx.invoked_subcommand is None:
        click.echo(ctx.get_help())
        ctx.exit()


@dhcp_server.group(cls=clicommon.AliasedGroup)
def ipv4():
    """Show ipv4 related dhcp_server info"""
    pass


@ipv4.command()
@click.argument('dhcp_interface', required=False, default="*")
@clicommon.pass_db
def lease(db, dhcp_interface):
    headers = ["Interface", "MAC Address", "IP", "Lease Start", "Lease End"]
    table = []
    dbconn = db.db
    for key in dbconn.keys("STATE_DB", "DHCP_SERVER_IPV4_LEASE|" + dhcp_interface + "|*"):
        entry = dbconn.get_all("STATE_DB", key)
        interface, mac = key.split("|")[1:]
        port = dbconn.get("STATE_DB", "FDB_TABLE|" + interface + ":" + mac, "port")
        if not port:
            # Smart switch sample: aa:bb:cc:dd:ee:ff dev dpu0 master bridge-midplane
            (out, _) = clicommon.run_command("sudo bridge fdb show | grep {}".format(mac), return_cmd=True, shell=True)
            match = re.match(r'([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2} dev (.*) master (.*)', out)
            if match and match.group(3).strip() == interface:
                port = match.group(2).strip()
        if not port:
            port = "<Unknown>"
        table.append([interface + "|" + port, mac, entry["ip"], ts_to_str(entry["lease_start"]), ts_to_str(entry["lease_end"])])
    click.echo(tabulate(table, headers=headers, tablefmt="grid"))


def count_ipv4(start, end):
    ip1 = int(ipaddress.IPv4Address(start))
    ip2 = int(ipaddress.IPv4Address(end))
    return ip2 - ip1 + 1


@ipv4.command()
@click.argument('range_name', required=False, default="*")
@clicommon.pass_db
def range(db, range_name):
    headers = ["Range", "IP Start", "IP End", "IP Count"]
    table = []
    dbconn = db.db
    for key in dbconn.keys("CONFIG_DB", "DHCP_SERVER_IPV4_RANGE|" + range_name):
        name = key.split("|")[1]
        entry = dbconn.get_all("CONFIG_DB", key)
        range_ = entry["range@"].split(",")
        if len(range_) == 1:
            start, end = range_[0], range_[0]
        elif len(range_) == 2:
            start, end = range_
        else:
            table.append([name, "", "", "range value is illegal"])
            continue
        count = count_ipv4(start, end)
        if count < 1:
            count = "range value is illegal"
        table.append([name, start, end, count])
    click.echo(tabulate(table, headers=headers, tablefmt="grid"))


def dhcp_interface_is_match(input_, key):
    regex = fnmatch.translate(input_)
    regex = re.compile(regex)
    if regex.match(key):
        return True
    for item in key.split("|"):
        if regex.match(item):
            return True
    return False


@ipv4.command()
@click.argument('dhcp_interface', required=False, default="*")
@click.option('--with_customized_options', default=False, is_flag=True)
@clicommon.pass_db
def info(db, dhcp_interface, with_customized_options):
    headers = ["Interface", "Mode", "Gateway", "Netmask", "Lease Time(s)", "State"]
    if with_customized_options:
        headers.append("Customized Options")
    table = []
    dbconn = db.db
    for key in dbconn.keys("CONFIG_DB", "DHCP_SERVER_IPV4|" + dhcp_interface):
        entry = dbconn.get_all("CONFIG_DB", key)
        interface = key.split("|")[1]
        table.append([interface, entry["mode"], entry.get("gateway", ""), entry["netmask"], entry["lease_time"], entry["state"]])
        if with_customized_options:
            table[-1].append(entry["customized_options@"] if "customized_options@" in entry else "")
    click.echo(tabulate(table, headers=headers, tablefmt="grid"))


@ipv4.command()
@click.argument("option_name", required=False, default="*")
@clicommon.pass_db
def option(db, option_name):
    headers = ["Option Name", "Option ID", "Value", "Type"]
    table = []
    dbconn = db.db
    for key in dbconn.keys("CONFIG_DB", "DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS|" + option_name):
        entry = dbconn.get_all("CONFIG_DB", key)
        name = key.split("|")[1]
        table.append([name, entry["id"], entry["value"], entry["type"]])
    click.echo(tabulate(table, headers=headers, tablefmt="grid"))


@ipv4.command()
@click.argument('interface', required=False, default="*")
@clicommon.pass_db
def port(db, interface):
    headers = ["Interface", "Bind"]
    table = []
    dbconn = db.db
    for key in dbconn.keys("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|*"):
        intf = key[len("DHCP_SERVER_IPV4_PORT|"):]
        if dhcp_interface_is_match(interface, intf):
            entry = dbconn.get_all("CONFIG_DB", key)
            if "ranges@" in entry:
                table.append([intf, entry["ranges@"].replace(",", "\n")])
            if "ips@" in entry:
                table.append([intf, entry["ips@"].replace(",", "\n")])
    click.echo(tabulate(table, headers=headers, tablefmt="grid"))


def register(cli):
    cli.add_command(dhcp_server)
