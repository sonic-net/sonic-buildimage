import click
import ipaddress
import utilities_common.cli as clicommon

DHCP_RELAY_TABLE = "DHCP_RELAY"
DHCPV6_SERVERS = "dhcpv6_servers"
IPV6 = 6

VLAN_TABLE = "VLAN"
DHCPV4_SERVERS = "dhcp_servers"
IPV4 = 4
DHCPV4_RELAY_TBL_SERVERS = "dhcpv4_servers"
DHCPV4_RELAY_TABLE = "DHCPV4_RELAY"


def check_sonic_dhcpv4_relay_flag(db):
    table = db.cfgdb.get_entry("FEATURE", "dhcp_relay")
    if('has_sonic_dhcpv4_relay' in table and table['has_sonic_dhcpv4_relay'] == 'True'):
        return True
    return False

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
        # Check if vlan is created in VLAN_TABLE
        keys = db.cfgdb.get_keys(VLAN_TABLE)
        if vlan_name not in keys:
            ctx.fail("{} doesn't exist".format(vlan_name))
        # Check if dhcp relay configs exist for a vlan
        if VLAN_TABLE != table_name:
           keys = db.cfgdb.get_keys(table_name)
           if vlan_name not in keys:
               return [],{}

    table = db.cfgdb.get_entry(table_name, vlan_name)
    dhcp_servers = table.get(dhcp_servers_str, [])

    return dhcp_servers, table


def restart_dhcp_relay_service(db):
    """
    Restart dhcp_relay service
    """
    if(check_sonic_dhcpv4_relay_flag(db)):
        # if 'has_sonic_dhcpv4_relay' flag is present in FEATURE['dhcp_relay] and is 'true'
        return
    click.echo("Restarting DHCP relay service...")
    clicommon.run_command(['systemctl', 'stop', 'dhcp_relay'], display_cmd=False)
    clicommon.run_command(['systemctl', 'reset-failed', 'dhcp_relay'], display_cmd=False)
    clicommon.run_command(['systemctl', 'start', 'dhcp_relay'], display_cmd=False)

def add_dhcp_relay(vid, dhcp_relay_ips, db, ip_version):
    table_name = DHCP_RELAY_TABLE if ip_version == 6 else VLAN_TABLE
    dhcp_servers_str = DHCPV6_SERVERS if ip_version == 6 else DHCPV4_SERVERS
    vlan_name = "Vlan{}".format(vid)
    if ip_version == IPV4:
       dhcp_table_name = DHCPV4_RELAY_TABLE
       dhcpv4_servers_str = DHCPV4_RELAY_TBL_SERVERS
    ctx = click.get_current_context()
    # Verify ip addresses are valid
    validate_ips(ctx, dhcp_relay_ips, ip_version)

    # It's unnecessary for DHCPv6 Relay to verify entry exist
    check_config_exist = True if ip_version == 4 else False
    dhcp_servers, table = get_dhcp_servers(db, vlan_name, ctx, table_name, dhcp_servers_str, check_config_exist)
    if ip_version == IPV4:
       dhcpv4_servers, v4_table = get_dhcp_servers(db, vlan_name, ctx, dhcp_table_name, dhcpv4_servers_str, check_config_exist)
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

        if ip_version == IPV4:
           if dhcp_relay_ip in dhcpv4_servers:
              #Already configured as DHCP relay 
              return
           dhcpv4_servers.append(dhcp_relay_ip)

    table[dhcp_servers_str] = dhcp_servers

    db.cfgdb.set_entry(table_name, vlan_name, table)
    click.echo("Added DHCP relay address [{}] to {}".format(",".join(dhcp_relay_ips), vlan_name))

    # for IPv4, we will add same entry to DHCPV4_RELAY table also
    if ip_version == IPV4:
       v4_table[dhcpv4_servers_str] = dhcpv4_servers
       db.cfgdb.set_entry(dhcp_table_name, vlan_name, v4_table)

    try:
        restart_dhcp_relay_service(db)
    except SystemExit as e:
        ctx.fail("Restart service dhcp_relay failed with error {}".format(e))


def del_dhcp_relay(vid, dhcp_relay_ips, db, ip_version):
    table_name = DHCP_RELAY_TABLE if ip_version == 6 else VLAN_TABLE
    dhcp_servers_str = DHCPV6_SERVERS if ip_version == 6 else DHCPV4_SERVERS
    vlan_name = "Vlan{}".format(vid)
    if ip_version == IPV4:
       dhcp_table_name = DHCPV4_RELAY_TABLE
       dhcpv4_servers_str = DHCPV4_RELAY_TBL_SERVERS
    ctx = click.get_current_context()
    # Verify ip addresses are valid
    validate_ips(ctx, dhcp_relay_ips, ip_version)
    dhcp_servers, table = get_dhcp_servers(db, vlan_name, ctx, table_name, dhcp_servers_str)
    if ip_version == IPV4:
       dhcpv4_servers, v4_table = get_dhcp_servers(db, vlan_name, ctx, dhcp_table_name, dhcpv4_servers_str)
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

        if ip_version == IPV4:
           if dhcp_relay_ip in dhcpv4_servers:
              dhcpv4_servers.remove(dhcp_relay_ip)

    if len(dhcp_servers) == 0:
        del table[dhcp_servers_str]
    else:
        table[dhcp_servers_str] = dhcp_servers

    if ip_version == 6 and len(table.keys()) == 0:
        table = None

    db.cfgdb.set_entry(table_name, vlan_name, table)
    click.echo("Removed DHCP relay address [{}] from {}".format(",".join(dhcp_relay_ips), vlan_name))

    # for IPv4, we will remove same entry from DHCPV4_RELAY table also
    if ip_version == IPV4:
       if len(dhcpv4_servers) == 0:
           del v4_table[dhcpv4_servers_str]
       else:
           v4_table[dhcpv4_servers_str] = dhcpv4_servers

       if len(v4_table.keys()) == 0:
           v4_table = None

       db.cfgdb.set_entry(dhcp_table_name, vlan_name, v4_table)

    try:
        restart_dhcp_relay_service(db)
    except SystemExit as e:
        ctx.fail("Restart service dhcp_relay failed with error {}".format(e))


def is_dhcp_server_enabled(db):
    dhcp_server_feature_entry = db.cfgdb.get_entry("FEATURE", "dhcp_server")
    return "state" in dhcp_server_feature_entry and dhcp_server_feature_entry["state"] == "enabled"

@click.group(cls=clicommon.AbbreviationGroup, name="dhcpv4_relay")
def dhcpv4_relay():
    pass

def validate_vrf_exists(db, vrf_name):
    """Check if the given VRF exists in the ConfigDB"""
    keys = db.cfgdb.get_keys("VRF")
    if vrf_name in keys:
        return True
    return False

def validate_vlan_exists(db, vlan_name):
    """Check if the given Vlan exists in the ConfigDB"""
    keys = db.cfgdb.get_keys("VLAN")
    if vlan_name in keys:
        return True
    return False

def add_dhcpv4_source_interface(vlan_name, source_interface, db):
    config_db = db.cfgdb
    ctx = click.get_current_context()

    interface_mapping = {
            "Ethernet" : "INTERFACE",
            "PortChannel" : "PORTCHANNEL_INTERFACE",
            "Loopback" : "LOOPBACK_INTERFACE"
    }
    interface_table = next((table for prefix, table in interface_mapping.items() if source_interface.startswith(prefix)), None)
    if not interface_table:
        ctx.fail(f"{source_interface} is not a valid Ethernet, PortChannel, or Loopback interface.")
        return False
    dhcp_entry = config_db.get_entry(DHCPV4_RELAY_TABLE, vlan_name) or {}

    # Check if the same source interface is already configured
    if dhcp_entry.get("source_interface") == source_interface:
        click.echo(f"Source interface {source_interface} is already set for {vlan_name}")
    else:
        # Update the source interface in DHCP relay entry
        updated_entry = dict(dhcp_entry)
        updated_entry["source_interface"] = source_interface
        config_db.set_entry(DHCPV4_RELAY_TABLE, vlan_name, updated_entry)
    return True

@dhcpv4_relay.command("update")
@click.option("--dhcpv4-servers", required=False, multiple=True, help="List of DHCPv4 relay servers to update")
@click.option("--source-interface", required=False, help="Source interface for DHCPv4 relay")
@click.option("--link-selection", required=False, type=click.Choice(["enable", "disable"]), help="Link selection flag for DHCPv4 Relay")
@click.option("--vrf-selection", required=False, type=click.Choice(["enable", "disable"]), help="VRF selection flag for DHCPv4 relay")
@click.option("--server-id-override", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable server ID override for DHCPv4 relay")
@click.option("--server-vrf", required=False, help="Server VRF name for DHCPv4 relay")
@click.option("--agent-relay-mode", required=False, type=click.Choice(["discard", "forward_and_append", "forward_and_replace", "forward_untouched"]),
              help="Set agent relay mode for DHCPv4 relay")
@click.option("--max-hop-count", required=False, help="Maximum hop count for DHCPv4 relay")
@click.argument("vlan_name", metavar="<VLAN_NAME>", required=True)
@clicommon.pass_db
def update_dhcpv4_relay(db, vlan_name, dhcpv4_servers, source_interface, link_selection, vrf_selection, server_id_override, server_vrf, agent_relay_mode, max_hop_count):
    """Update an existing DHCPv4 relay entry for a VLAN"""
    config_db = db.cfgdb
    ctx = click.get_current_context()

    # Check if VLAN exists
    if not validate_vlan_exists(db, vlan_name):
        ctx.fail(f"Error: Vlan {vlan_name} does not exist in the configDB".format(vlan_name))
    vlan_entry = config_db.get_entry(DHCPV4_RELAY_TABLE, vlan_name)

    updated_entry = vlan_entry.copy()

    if dhcpv4_servers:
        existing_servers = set(vlan_entry.get("dhcpv4_servers@", "").split(","))
        updated_entry["dhcpv4_servers@"] = ",".join(existing_servers.union(set(dhcpv4_servers)))

    if source_interface:
        add_dhcpv4_source_interface(vlan_name, source_interface, db)

    if link_selection:
        updated_entry["link_selection"] = link_selection

    if vrf_selection:
        updated_entry["vrf_selection"] = vrf_selection

    if server_id_override:
        updated_entry["server_id_override"] = server_id_override

    if server_vrf:
        if not validate_vrf_exists(db, server_vrf):
            ctx.fail(f"VRF {server_vrf} does not exist in the VRF table.")

        updated_entry["server_vrf"] = server_vrf

        config_db.set_entry('VRF', server_vrf, {'VRF': server_vrf})

    if agent_relay_mode:
        updated_entry["agent_relay_mode"] = agent_relay_mode

    if max_hop_count is not None:
        updated_entry["max_hop_count"] = max_hop_count

    # Apply updated entry to the database
    config_db.set_entry(DHCPV4_RELAY_TABLE, vlan_name, updated_entry)
    if source_interface:
        click.echo(f"Updated Source Interface Configuration to {vlan_name}")
    elif server_vrf:
        click.echo(f"Updated Server VRF {server_vrf} to {vlan_name}")
    elif agent_relay_mode:
        click.echo(f"Updated Agent Relay Mode to {vlan_name}")
    elif max_hop_count:
        click.echo(f"Updated Max Hop Count to {vlan_name}")
    elif link_selection:
        click.echo(f"Updated link-selection flag configuration to disable to {vlan_name}")
    elif vrf_selection:
        click.echo(f"Updated vrf-selection flag configuration to disable to {vlan_name}")
    elif server_id_override:
        click.echo(f"Updated server-id-override flag configuration to disable to {vlan_name}")
    else:
        click.echo(f"Updated DHCPv4 Relay Configuration to {vlan_name}")

@dhcpv4_relay.command("add")
@click.option("--dhcpv4-servers", required=True, multiple=True, help="List of DHCPv4 relay servers")
@click.option("--source-interface", required=False, help="Source interface for DHCPv4 relay")
@click.option("--link-selection", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable link selection for DHCPv4 relay")
@click.option("--vrf-selection", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable VRF selection for DHCPv4 relay")
@click.option("--server-id-override", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable server ID override for DHCPv4 relay")
@click.option("--server-vrf", required=False, help="Server VRF name for DHCPv4 relay")
@click.option("--agent-relay-mode", required=False, type=click.Choice(["discard", "forward_and_append", "forward_and_replace", "forward_untouched"]),
              help="Set agent relay mode for DHCPv4 relay")
@click.option("--max-hop-count", required=False, type=int, help="Maximum hop count for DHCPv4 relay")
@click.argument("vlan_name", metavar="<VLAN_NAME>", required=True)
@clicommon.pass_db
def add_dhcpv4_relay(db, dhcpv4_servers, vlan_name, source_interface, link_selection, vrf_selection, server_id_override, server_vrf, agent_relay_mode, max_hop_count):
    """Add DHCPv4 relay configuration for a VLAN"""
    config_db = db.cfgdb
    ctx = click.get_current_context()

    existing_entry = config_db.get_entry(DHCPV4_RELAY_TABLE, vlan_name) or {}
    relay_entry = existing_entry.copy()
    new_servers = set(existing_entry.get("dhcpv4_servers@", "").split(","))
    new_servers.update(dhcpv4_servers)

    relay_entry["dhcpv4_servers@"] = ",".join(new_servers)

    if source_interface:
        add_dhcpv4_source_interface(vlan_name, source_interface, db)

    if server_vrf:
        if not all([link_selection == "enable", vrf_selection == "enable", server_id_override == "enable"]):
            ctx.fail("server-vrf requires link-selection, vrf-selection and server-id-override flags to be enabled.")

        if not validate_vrf_exists(db, server_vrf):
            ctx.fail(f"VRF {server_vrf} does not exist in the VRF table.")

    if agent_relay_mode:
        relay_entry["agent_relay_mode"] = agent_relay_mode

    if max_hop_count is not None:
        relay_entry["max_hop_count"] = max_hop_count

    config_db.set_entry(DHCPV4_RELAY_TABLE, vlan_name, relay_entry)
    if source_interface and server_vrf and agent_relay_mode and max_hop_count:
        click.echo(f"Added Source Interface, Server VRF {server_vrf}, agent_relay_mode and max_hop_count to {vlan_name}")
    elif source_interface:
        click.echo(f"Added Source Interface to {vlan_name}")
    elif server_vrf:
        click.echo(f"Added Server VRF {server_vrf} to {vlan_name}")
    elif agent_relay_mode:
        click.echo(f"Added Agent Relay Mode to {vlan_name}")
    elif max_hop_count:
        click.echo(f"Added Max Hop Count as {max_hop_count} to {vlan_name}")
    elif link_selection:
        click.echo(f"Added link-selection flag configuration as enable to {vlan_name}")
    elif vrf_selection:
        click.echo(f"Added vrf-selection flag configuration as enable to {vlan_name}")
    elif server_id_override:
        click.echo(f"Added server-id-override flag configuration as enable to {vlan_name}")
    else:
        click.echo(f"Added DHCPv4 relay configuration for {vlan_name}")

@dhcpv4_relay.command("del")
@click.argument("vlan_name", metavar="<VLAN_NAME>", required=True)
@clicommon.pass_db
def del_dhcpv4_relay(db, vlan_name):
    """Delete entire DHCPv4 relay from a VLAN"""
    ctx = click.get_current_context()

    if not validate_vlan_exists(db, vlan_name):
        ctx.fail(f"Vlan {vlan_name} does not exist in the configDB".format(vlan_name))

    db.cfgdb.set_entry(DHCPV4_RELAY_TABLE, vlan_name, None)
    click.echo(f"Removed DHCPv4 relay configuration from {vlan_name}")

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
@click.option("--source-interface", required=False, help="Source interface for DHCPv4 relay")
@click.option("--link-selection", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable link selection for DHCPv4 relay")
@click.option("--vrf-selection", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable VRF selection for DHCPv4 relay")
@click.option("--server-id-override", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable server ID override for DHCPv4 relay")
@click.option("--server-vrf", required=False, help="Server VRF name for DHCPv4 relay")
@click.option("--agent-relay-mode", required=False, type=click.Choice(["discard", "forward_and_append", "forward_and_replace", "forward_untouched"]),
              help="Set agent relay mode for DHCPv4 relay")
@click.option("--max-hop-count", required=False, type=int, help="Maximum hop count for DHCPv4 relay")
@clicommon.pass_db
def add_dhcp_relay_ipv4_helper(db, vid, dhcp_relay_helpers, source_interface, link_selection, vrf_selection, server_id_override, server_vrf, agent_relay_mode, max_hop_count):
    if is_dhcp_server_enabled(db):
        click.echo("Cannot change ipv4 dhcp_relay configuration when dhcp_server feature is enabled")
        return

    if check_sonic_dhcpv4_relay_flag(db):
        add_dhcpv4_relay.callback(dhcp_relay_helpers, "Vlan"+str(vid), source_interface, link_selection, vrf_selection, server_id_override, server_vrf, agent_relay_mode, max_hop_count)
        return
    else:
        if source_interface or link_selection or vrf_selection or server_id_override or server_vrf or agent_relay_mode or max_hop_count :
            click.echo(f"These parameters are applicable for new DHCPv4 Relay feature")
            return

    add_dhcp_relay(vid, dhcp_relay_helpers, db, IPV4)

@dhcp_relay_ipv4_helper.command("update")
@click.argument("vid", metavar="<vid>", required=True, type=int)
@click.argument("dhcp_relay_helpers", nargs=-1, required=True)
@click.option("--source-interface", required=False, help="Source interface for DHCPv4 relay")
@click.option("--link-selection", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable link selection for DHCPv4 relay")
@click.option("--vrf-selection", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable VRF selection for DHCPv4 relay")
@click.option("--server-id-override", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable server ID override for DHCPv4 relay")
@click.option("--server-vrf", required=False, help="Server VRF name for DHCPv4 relay")
@click.option("--agent-relay-mode", required=False, type=click.Choice(["discard", "forward_and_append", "forward_and_replace", "forward_untouched"]),
              help="Set agent relay mode for DHCPv4 relay")
@click.option("--max-hop-count", required=False, type=int, help="Maximum hop count for DHCPv4 relay")
@clicommon.pass_db
def update_dhcp_relay_ipv4_helper(db, vid, dhcp_relay_helpers, source_interface, link_selection, vrf_selection, server_id_override, server_vrf, agent_relay_mode, max_hop_count):
    if is_dhcp_server_enabled(db):
        click.echo("Cannot change ipv4 dhcp_relay configuration when dhcp_server feature is enabled")
        return

    if check_sonic_dhcpv4_relay_flag(db):
        update_dhcpv4_relay.callback("Vlan"+str(vid), dhcp_relay_helpers, source_interface, link_selection, vrf_selection, server_id_override, server_vrf, agent_relay_mode, max_hop_count)
    else:
        click.echo(f"This command is applicable for new DHCPv4 Relay feature")


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
            restart_dhcp_relay_service(db)
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
        restart_dhcp_relay_service(db)
    except SystemExit as e:
        ctx.fail("Restart service dhcp_relay failed with error {}".format(e))


def register(cli):
    cli.add_command(dhcp_relay)
    cli.add_command(dhcpv4_relay)
    cli.commands['vlan'].add_command(vlan_dhcp_relay)


if __name__ == '__main__':
    dhcp_relay()
    dhcpv4_relay()
    vlan_dhcp_relay()
