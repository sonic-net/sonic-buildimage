import click
import ipaddress
import utilities_common.cli as clicommon

DHCP_RELAY_TABLE = "DHCP_RELAY"
DHCPV6_SERVERS = "dhcpv6_servers"
IPV6 = 6

VLAN_TABLE = "VLAN"
DHCPV4_SERVERS = "dhcp_servers"
IPV4 = 4


def validate_ips(ctx, ips, ip_version):
    for ip in ips:
        try:
            ip_address = ipaddress.ip_address(ip)
        except Exception:
            ctx.fail("{} is invalid IP address".format(ip))

        if ip_address.version != ip_version:
            ctx.fail("{} is not IPv{} address".format(ip, ip_version))


def get_dhcp_servers(db, vlan_name, ctx, table_name, dhcp_servers_str, check_is_exist=True):
    if check_is_exist:
        keys = db.cfgdb.get_keys(table_name)
        if vlan_name not in keys:
            ctx.fail("{} doesn't exist".format(vlan_name))

    table = db.cfgdb.get_entry(table_name, vlan_name)
    dhcp_servers = table.get(dhcp_servers_str, [])

    return dhcp_servers, table


def restart_dhcp_relay_service():
    """
    Restart dhcp_relay service
    """
    click.echo("Restarting DHCP relay service...")
    clicommon.run_command(['systemctl', 'stop', 'dhcp_relay'], display_cmd=False)
    clicommon.run_command(['systemctl', 'reset-failed', 'dhcp_relay'], display_cmd=False)
    clicommon.run_command(['systemctl', 'start', 'dhcp_relay'], display_cmd=False)


def add_dhcp_relay(vid, dhcp_relay_ips, db, ip_version):
    table_name = DHCP_RELAY_TABLE if ip_version == 6 else VLAN_TABLE
    dhcp_servers_str = DHCPV6_SERVERS if ip_version == 6 else DHCPV4_SERVERS
    vlan_name = "Vlan{}".format(vid)
    ctx = click.get_current_context()
    # Verify ip addresses are valid
    validate_ips(ctx, dhcp_relay_ips, ip_version)

    # It's unnecessary for DHCPv6 Relay to verify entry exist
    check_config_exist = True if ip_version == 4 else False
    dhcp_servers, table = get_dhcp_servers(db, vlan_name, ctx, table_name, dhcp_servers_str, check_config_exist)
    added_ips = []

    for dhcp_relay_ip in dhcp_relay_ips:
        # Verify ip addresses not duplicate in add list
        if dhcp_relay_ip in added_ips:
            ctx.fail("Error: Find duplicate DHCP relay ip {} in add list".format(dhcp_relay_ip))
        # Verify ip addresses not exist in DB
        if dhcp_relay_ip in dhcp_servers:
            click.echo("{} is already a DHCP relay for {}".format(dhcp_relay_ip, vlan_name))
            return

        dhcp_servers.append(dhcp_relay_ip)
        added_ips.append(dhcp_relay_ip)

    table[dhcp_servers_str] = dhcp_servers

    db.cfgdb.set_entry(table_name, vlan_name, table)
    click.echo("Added DHCP relay address [{}] to {}".format(",".join(dhcp_relay_ips), vlan_name))
    try:
        restart_dhcp_relay_service()
    except SystemExit as e:
        ctx.fail("Restart service dhcp_relay failed with error {}".format(e))


def del_dhcp_relay(vid, dhcp_relay_ips, db, ip_version):
    table_name = DHCP_RELAY_TABLE if ip_version == 6 else VLAN_TABLE
    dhcp_servers_str = DHCPV6_SERVERS if ip_version == 6 else DHCPV4_SERVERS
    vlan_name = "Vlan{}".format(vid)
    ctx = click.get_current_context()
    # Verify ip addresses are valid
    validate_ips(ctx, dhcp_relay_ips, ip_version)
    dhcp_servers, table = get_dhcp_servers(db, vlan_name, ctx, table_name, dhcp_servers_str)
    removed_ips = []

    for dhcp_relay_ip in dhcp_relay_ips:
        # Verify ip addresses not duplicate in del list
        if dhcp_relay_ip in removed_ips:
            ctx.fail("Error: Find duplicate DHCP relay ip {} in del list".format(dhcp_relay_ip))
        # Remove dhcp servers if they exist in the DB
        if dhcp_relay_ip not in dhcp_servers:
            ctx.fail("{} is not a DHCP relay for {}".format(dhcp_relay_ip, vlan_name))

        dhcp_servers.remove(dhcp_relay_ip)
        removed_ips.append(dhcp_relay_ip)

    if len(dhcp_servers) == 0:
        del table[dhcp_servers_str]
    else:
        table[dhcp_servers_str] = dhcp_servers

    if ip_version == 6 and len(table.keys()) == 0:
        table = None

    db.cfgdb.set_entry(table_name, vlan_name, table)
    click.echo("Removed DHCP relay address [{}] from {}".format(",".join(dhcp_relay_ips), vlan_name))
    try:
        restart_dhcp_relay_service()
    except SystemExit as e:
        ctx.fail("Restart service dhcp_relay failed with error {}".format(e))


def is_dhcp_server_enabled(db):
    dhcp_server_feature_entry = db.cfgdb.get_entry("FEATURE", "dhcp_server")
    return "state" in dhcp_server_feature_entry and dhcp_server_feature_entry["state"] == "enabled"


@click.group(cls=clicommon.AbbreviationGroup, name="dhcp_relay")
def dhcp_relay():
    """config DHCP_Relay information"""
    pass


@dhcp_relay.group(cls=clicommon.AbbreviationGroup, name="ipv6")
def dhcp_relay_ipv6():
    pass


@dhcp_relay_ipv6.group(cls=clicommon.AbbreviationGroup, name="destination")
def dhcp_relay_ipv6_destination():
    pass


@dhcp_relay_ipv6_destination.command("add")
@click.argument("vid", metavar="<vid>", required=True, type=int)
@click.argument("dhcp_relay_destinations", nargs=-1, required=True)
@clicommon.pass_db
def add_dhcp_relay_ipv6_destination(db, vid, dhcp_relay_destinations):
    add_dhcp_relay(vid, dhcp_relay_destinations, db, IPV6)


@dhcp_relay_ipv6_destination.command("del")
@click.argument("vid", metavar="<vid>", required=True, type=int)
@click.argument("dhcp_relay_destinations", nargs=-1, required=True)
@clicommon.pass_db
def del_dhcp_relay_ipv6_destination(db, vid, dhcp_relay_destinations):
    del_dhcp_relay(vid, dhcp_relay_destinations, db, IPV6)


@dhcp_relay.group(cls=clicommon.AbbreviationGroup, name="ipv4")
def dhcp_relay_ipv4():
    pass


@dhcp_relay_ipv4.group(cls=clicommon.AbbreviationGroup, name="helper")
def dhcp_relay_ipv4_helper():
    pass


@dhcp_relay_ipv4_helper.command("add")
@click.argument("vid", metavar="<vid>", required=True, type=int)
@click.argument("dhcp_relay_helpers", nargs=-1, required=True)
@clicommon.pass_db
def add_dhcp_relay_ipv4_helper(db, vid, dhcp_relay_helpers):
    if is_dhcp_server_enabled(db):
        click.echo("Cannot change ipv4 dhcp_relay configuration when dhcp_server feature is enabled")
        return
    add_dhcp_relay(vid, dhcp_relay_helpers, db, IPV4)


@dhcp_relay_ipv4_helper.command("del")
@click.argument("vid", metavar="<vid>", required=True, type=int)
@click.argument("dhcp_relay_helpers", nargs=-1, required=True)
@clicommon.pass_db
def del_dhcp_relay_ipv4_helper(db, vid, dhcp_relay_helpers):
    if is_dhcp_server_enabled(db):
        click.echo("Cannot change ipv4 dhcp_relay configuration when dhcp_server feature is enabled")
        return
    del_dhcp_relay(vid, dhcp_relay_helpers, db, IPV4)


# subcommand of vlan
@click.group(cls=clicommon.AbbreviationGroup, name='dhcp_relay')
def vlan_dhcp_relay():
    pass


@vlan_dhcp_relay.command('add')
@click.argument('vid', metavar='<vid>', required=True, type=int)
@click.argument('dhcp_relay_destination_ips', nargs=-1, required=True)
@clicommon.pass_db
def add_vlan_dhcp_relay_destination(db, vid, dhcp_relay_destination_ips):
    """ Add a destination IP address to the VLAN's DHCP relay """

    ctx = click.get_current_context()
    added_servers = []

    # Verify vlan is valid
    vlan_name = 'Vlan{}'.format(vid)
    vlan = db.cfgdb.get_entry('VLAN', vlan_name)
    if len(vlan) == 0:
        ctx.fail("{} doesn't exist".format(vlan_name))

    # Verify all ip addresses are valid and not exist in DB
    dhcp_servers = vlan.get('dhcp_servers', [])
    dhcpv6_servers = vlan.get('dhcpv6_servers', [])

    for ip_addr in dhcp_relay_destination_ips:
        try:
            ipaddress.ip_address(ip_addr)
            if (ip_addr in dhcp_servers) or (ip_addr in dhcpv6_servers):
                click.echo("{} is already a DHCP relay destination for {}".format(ip_addr, vlan_name))
                continue
            if clicommon.ipaddress_type(ip_addr) == 4:
                if is_dhcp_server_enabled(db):
                    click.echo("Cannot change dhcp_relay configuration when dhcp_server feature is enabled")
                    return
                dhcp_servers.append(ip_addr)
            else:
                dhcpv6_servers.append(ip_addr)
            added_servers.append(ip_addr)
        except Exception:
            ctx.fail('{} is invalid IP address'.format(ip_addr))

    # Append new dhcp servers to config DB
    if len(dhcp_servers):
        vlan['dhcp_servers'] = dhcp_servers
    if len(dhcpv6_servers):
        vlan['dhcpv6_servers'] = dhcpv6_servers

    db.cfgdb.set_entry('VLAN', vlan_name, vlan)

    if len(added_servers):
        click.echo("Added DHCP relay destination addresses {} to {}".format(added_servers, vlan_name))
        try:
            restart_dhcp_relay_service()
        except SystemExit as e:
            ctx.fail("Restart service dhcp_relay failed with error {}".format(e))


@vlan_dhcp_relay.command('del')
@click.argument('vid', metavar='<vid>', required=True, type=int)
@click.argument('dhcp_relay_destination_ips', nargs=-1, required=True)
@clicommon.pass_db
def del_vlan_dhcp_relay_destination(db, vid, dhcp_relay_destination_ips):
    """ Remove a destination IP address from the VLAN's DHCP relay """

    ctx = click.get_current_context()

    # Verify vlan is valid
    vlan_name = 'Vlan{}'.format(vid)
    vlan = db.cfgdb.get_entry('VLAN', vlan_name)
    if len(vlan) == 0:
        ctx.fail("{} doesn't exist".format(vlan_name))

    # Remove dhcp servers if they exist in the DB
    dhcp_servers = vlan.get('dhcp_servers', [])
    dhcpv6_servers = vlan.get('dhcpv6_servers', [])

    for ip_addr in dhcp_relay_destination_ips:
        if (ip_addr not in dhcp_servers) and (ip_addr not in dhcpv6_servers):
            ctx.fail("{} is not a DHCP relay destination for {}".format(ip_addr, vlan_name))
        if clicommon.ipaddress_type(ip_addr) == 4:
            if is_dhcp_server_enabled(db):
                click.echo("Cannot change dhcp_relay configuration when dhcp_server feature is enabled")
                return
            dhcp_servers.remove(ip_addr)
        else:
            dhcpv6_servers.remove(ip_addr)

    # Update dhcp servers to config DB
    if len(dhcp_servers):
        vlan['dhcp_servers'] = dhcp_servers
    else:
        if 'dhcp_servers' in vlan.keys():
            del vlan['dhcp_servers']

    if len(dhcpv6_servers):
        vlan['dhcpv6_servers'] = dhcpv6_servers
    else:
        if 'dhcpv6_servers' in vlan.keys():
            del vlan['dhcpv6_servers']

    db.cfgdb.set_entry('VLAN', vlan_name, vlan)
    click.echo("Removed DHCP relay destination addresses {} from {}".format(dhcp_relay_destination_ips, vlan_name))
    try:
        restart_dhcp_relay_service()
    except SystemExit as e:
        ctx.fail("Restart service dhcp_relay failed with error {}".format(e))


def register(cli):
    cli.add_command(dhcp_relay)
    cli.commands['vlan'].add_command(vlan_dhcp_relay)


if __name__ == '__main__':
    dhcp_relay()
    vlan_dhcp_relay()
