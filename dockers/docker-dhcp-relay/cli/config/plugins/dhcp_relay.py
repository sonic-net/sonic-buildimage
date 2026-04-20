import click
import ipaddress
import utilities_common.cli as clicommon

DHCP_RELAY_TABLE = "DHCP_RELAY"
DHCPV6_SERVERS = "dhcpv6_servers"
IPV6 = 6

VLAN_TABLE = "VLAN"
PORT_TABLE = "PORT"
DHCPV4_SERVERS = "dhcp_servers"
IPV4 = 4
DHCPV4_RELAY_TBL_SERVERS = "dhcpv4_servers"
DHCPV4_RELAY_TABLE = "DHCPV4_RELAY"


def check_sonic_dhcpv4_relay_flag(db):
    table = db.cfgdb.get_entry("DEVICE_METADATA", "localhost")
    if('has_sonic_dhcpv4_relay' in table and table['has_sonic_dhcpv4_relay'] == 'True'):
        return True
    return False


def get_interface_table_name(interface_name):
    """
    Determine the ConfigDB table name based on interface type for DHCP relay configuration.
    For Ethernet ports, returns the PORT table.
    For VLANs, returns the VLAN table.
    PortChannel is not supported for DHCP relay configuration.
    Returns the table name for the interface, or None if unsupported.
    """
    if interface_name.startswith("Vlan"):
        return VLAN_TABLE
    elif interface_name.startswith("Ethernet"):
        return PORT_TABLE
    else:
        return None

def validate_ips(ctx, ips, ip_version):
    for ip in ips:
        try:
            ip_address = ipaddress.ip_address(ip)
        except Exception:
            ctx.fail("{} is invalid IP address".format(ip))

        if ip_address.version != ip_version:
            ctx.fail("{} is not IPv{} address".format(ip, ip_version))


def get_dhcp_servers(db, interface_name, ctx, table_name, dhcp_servers_str, check_is_exist=True):
    """
    Get DHCP servers for an interface (VLAN or Ethernet).

    Args:
        db: Database object
        interface_name: Full interface name (e.g., "Vlan100", "Ethernet0")
        ctx: Click context
        table_name: ConfigDB table name to query
        dhcp_servers_str: Field name for DHCP servers
        check_is_exist: Whether to verify interface exists

    Returns:
        Tuple of (dhcp_servers list, table entry dict)
    """
    if check_is_exist:
        # Determine the base table for the interface
        base_table = get_interface_table_name(interface_name)
        if base_table is None:
            ctx.fail("{} is not a valid interface type".format(interface_name))

        # Check if interface exists in its base table
        keys = db.cfgdb.get_keys(base_table)
        if interface_name not in keys:
            ctx.fail("{} doesn't exist".format(interface_name))

        # Check if dhcp relay configs exist for the interface
        if base_table != table_name:
           keys = db.cfgdb.get_keys(table_name)
           if interface_name not in keys:
               return [],{}

    table = db.cfgdb.get_entry(table_name, interface_name)
    dhcp_servers = table.get(dhcp_servers_str, [])

    return dhcp_servers, table


def restart_dhcp_relay_service(db, ip_version):
    """
    Restart dhcp_relay service
    """
    if(ip_version == IPV4 and check_sonic_dhcpv4_relay_flag(db)):
        # if 'has_sonic_dhcpv4_relay' flag is present in DEVICE_METADATA['localhost'] and is 'true'
        return
    click.echo("Restarting DHCP relay service...")
    clicommon.run_command(['systemctl', 'stop', 'dhcp_relay'], display_cmd=False)
    clicommon.run_command(['systemctl', 'reset-failed', 'dhcp_relay'], display_cmd=False)
    clicommon.run_command(['systemctl', 'start', 'dhcp_relay'], display_cmd=False)


def add_dhcp_relay(interface_name, dhcp_relay_ips, db, ip_version):
    """
    Add DHCP relay servers to an interface (VLAN or Ethernet).

    Args:
        interface_name: Full interface name (e.g., "Vlan100", "Ethernet0")
        dhcp_relay_ips: List of DHCP server IP addresses to add
        db: Database object
        ip_version: IP version (4 or 6)
    """
    # Determine the appropriate table based on interface type
    base_table = get_interface_table_name(interface_name)
    ctx = click.get_current_context()

    if base_table is None:
        ctx.fail("{} is not a valid interface type (must be Vlan or Ethernet)".format(interface_name))

    # For VLANs with IPv6, use DHCP_RELAY_TABLE; otherwise use the base table
    if ip_version == 6 and base_table == VLAN_TABLE:
        table_name = DHCP_RELAY_TABLE
    else:
        table_name = base_table

    dhcp_servers_str = DHCPV6_SERVERS if ip_version == 6 else DHCPV4_SERVERS

    # Check if using new DHCPv4 relay (only for VLANs)
    use_dhcpv4_relay_table = (ip_version == IPV4 and
                               base_table == VLAN_TABLE and
                               check_sonic_dhcpv4_relay_flag(db))

    if use_dhcpv4_relay_table:
       dhcp_table_name = DHCPV4_RELAY_TABLE
       dhcpv4_servers_str = DHCPV4_RELAY_TBL_SERVERS

    # Verify ip addresses are valid
    validate_ips(ctx, dhcp_relay_ips, ip_version)

    # It's unnecessary for DHCPv6 Relay to verify entry exist
    check_config_exist = True if ip_version == 4 else False

    if use_dhcpv4_relay_table:
       dhcpv4_servers, v4_table = get_dhcp_servers(db, interface_name, ctx, dhcp_table_name, dhcpv4_servers_str, check_config_exist)
    else:
       dhcp_servers, table = get_dhcp_servers(db, interface_name, ctx, table_name, dhcp_servers_str, check_config_exist)

    added_ips = []

    for dhcp_relay_ip in dhcp_relay_ips:
        # Verify ip addresses not duplicate in add list
        if dhcp_relay_ip in added_ips:
            ctx.fail("Find duplicate DHCP relay ip {} in add list".format(dhcp_relay_ip))

        if use_dhcpv4_relay_table:
            if dhcp_relay_ip in dhcpv4_servers:
                ctx.fail("{} is already a DHCPv4 relay for {}".format(dhcp_relay_ip, interface_name))
            dhcpv4_servers.append(dhcp_relay_ip)
        else:
            if dhcp_relay_ip in dhcp_servers:
                ctx.fail("{} is already a DHCP relay for {}".format(dhcp_relay_ip, interface_name))
            dhcp_servers.append(dhcp_relay_ip)
        added_ips.append(dhcp_relay_ip)

    # for IPv4 VLANs with new relay, add entry to DHCPV4_RELAY table
    if use_dhcpv4_relay_table:
        v4_table[dhcpv4_servers_str] = dhcpv4_servers
        db.cfgdb.set_entry(dhcp_table_name, interface_name, v4_table)
    else:
        table[dhcp_servers_str] = dhcp_servers
        db.cfgdb.set_entry(table_name, interface_name, table)

    click.echo("Added DHCP relay address [{}] to {}".format(",".join(dhcp_relay_ips), interface_name))
    try:
        restart_dhcp_relay_service(db, ip_version)
    except SystemExit as e:
        ctx.fail("Restart service dhcp_relay failed with error {}".format(e))


def del_dhcp_relay(interface_name, dhcp_relay_ips, db, ip_version):
    """
    Remove DHCP relay servers from an interface (VLAN or Ethernet).

    Args:
        interface_name: Full interface name (e.g., "Vlan100", "Ethernet0")
        dhcp_relay_ips: List of DHCP server IP addresses to remove
        db: Database object
        ip_version: IP version (4 or 6)
    """
    # Determine the appropriate table based on interface type
    base_table = get_interface_table_name(interface_name)
    ctx = click.get_current_context()

    if base_table is None:
        ctx.fail("{} is not a valid interface type (must be Vlan or Ethernet)".format(interface_name))

    # For VLANs with IPv6, use DHCP_RELAY_TABLE; otherwise use the base table
    if ip_version == 6 and base_table == VLAN_TABLE:
        table_name = DHCP_RELAY_TABLE
    else:
        table_name = base_table

    dhcp_servers_str = DHCPV6_SERVERS if ip_version == 6 else DHCPV4_SERVERS

    # Check if using new DHCPv4 relay (only for VLANs)
    use_dhcpv4_relay_table = (ip_version == IPV4 and
                               base_table == VLAN_TABLE and
                               check_sonic_dhcpv4_relay_flag(db))

    if use_dhcpv4_relay_table:
       dhcp_table_name = DHCPV4_RELAY_TABLE
       dhcpv4_servers_str = DHCPV4_RELAY_TBL_SERVERS

    # Verify ip addresses are valid
    validate_ips(ctx, dhcp_relay_ips, ip_version)

    if use_dhcpv4_relay_table:
       dhcpv4_servers, v4_table = get_dhcp_servers(db, interface_name, ctx, dhcp_table_name, dhcpv4_servers_str)
    else:
       dhcp_servers, table = get_dhcp_servers(db, interface_name, ctx, table_name, dhcp_servers_str)

    removed_ips = []

    for dhcp_relay_ip in dhcp_relay_ips:
        # Verify ip addresses not duplicate in del list
        if dhcp_relay_ip in removed_ips:
            ctx.fail("Find duplicate DHCP relay ip {} in del list".format(dhcp_relay_ip))

        if use_dhcpv4_relay_table:
            if dhcp_relay_ip in dhcpv4_servers:
                dhcpv4_servers.remove(dhcp_relay_ip)
                removed_ips.append(dhcp_relay_ip)
            else:
                ctx.fail("{} is not a DHCPv4 relay for {}".format(dhcp_relay_ip, interface_name))
        else:
           if dhcp_relay_ip in dhcp_servers:
              dhcp_servers.remove(dhcp_relay_ip)
              removed_ips.append(dhcp_relay_ip)
           else:
              ctx.fail("{} is not a DHCP relay for {}".format(dhcp_relay_ip, interface_name))

    # Update the database
    if use_dhcpv4_relay_table:
       if len(dhcpv4_servers) == 0:
           db.cfgdb.set_entry(dhcp_table_name, interface_name, None)
       else:
           v4_table[dhcpv4_servers_str] = dhcpv4_servers
           db.cfgdb.set_entry(dhcp_table_name, interface_name, v4_table)
    else:
        if len(dhcp_servers) == 0:
            del table[dhcp_servers_str]
        else:
            table[dhcp_servers_str] = dhcp_servers
        db.cfgdb.set_entry(table_name, interface_name, table)

    # For IPv6 VLANs, remove entry if empty
    if ip_version == 6 and base_table == VLAN_TABLE and len(table.keys()) == 0:
        db.cfgdb.set_entry(table_name, interface_name, None)

    click.echo("Removed DHCP relay address [{}] from {}".format(",".join(dhcp_relay_ips), interface_name))
    try:
        restart_dhcp_relay_service(db, ip_version)
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

def validate_interface_exists(db, interface_name):
    """Check if the given interface (VLAN or Ethernet) exists in the ConfigDB"""
    # Check if it's a VLAN
    if interface_name.startswith("Vlan"):
        return validate_vlan_exists(db, interface_name)
    # Check if it's an Ethernet port (either in PORT table or INTERFACE table for routed ports)
    elif interface_name.startswith("Ethernet"):
        # Check PORT table (for switch ports)
        port_keys = db.cfgdb.get_keys("PORT")
        if interface_name in port_keys:
            return True
        # Check INTERFACE table (for routed ports)
        interface_keys = db.cfgdb.get_keys("INTERFACE")
        return interface_name in interface_keys
    return False

def validate_source_interface(vlan_name, source_interface, db):
    config_db = db.cfgdb
    ctx = click.get_current_context()

    interface_mapping = {
            "Ethernet" : "INTERFACE",
            "PortChannel" : "PORTCHANNEL_INTERFACE",
            "Loopback" : "LOOPBACK_INTERFACE"
    }
    interface_table = None
    for prefix, table in interface_mapping.items():
        if source_interface.startswith(prefix):
            interface_table = table
            break

    if not interface_table:
        ctx.fail(f"{source_interface} is not a valid Ethernet, PortChannel, or Loopback interface.")
        return False

    # Check if the interface exists in the corresponding table
    interface_entries = config_db.get_table(interface_table)
    if source_interface not in interface_entries:
        ctx.fail(f"Interface {source_interface} not found in {interface_table} table. Please configure valid interface.")
        return False
    return True

@dhcpv4_relay.command("update")
@click.option("--dhcpv4-servers", required=False, help="List of DHCPv4 relay servers to update")
@click.option("--source-interface", required=False, help="Source interface for DHCPv4 relay")
@click.option("--link-selection", required=False, type=click.Choice(["enable", "disable"]), help="Link selection flag for DHCPv4 Relay")
@click.option("--vrf-selection", required=False, type=click.Choice(["enable", "disable"]), help="VRF selection flag for DHCPv4 relay")
@click.option("--server-id-override", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable server ID override for DHCPv4 relay")
@click.option("--server-vrf", required=False, help="Server VRF name for DHCPv4 relay")
@click.option("--agent-relay-mode", required=False, type=click.Choice(["discard", "append", "replace"]),
              help="Set agent relay mode for DHCPv4 relay")
@click.option("--max-hop-count", required=False, type=int, help="Maximum hop count for DHCPv4 relay")
@click.option("--circuit-id-format", required=False, type=click.Choice(["default", "interface_ip", "vlan_name", "custom"]),
              help="Circuit ID format for DHCPv4 relay")
@click.option("--circuit-id", required=False, help="Custom Circuit ID value (used when circuit-id-format is 'custom')")
@click.argument("interface_name", metavar="<INTERFACE_NAME>", required=True)
@clicommon.pass_db
def update_dhcpv4_relay(db, interface_name, dhcpv4_servers, source_interface, link_selection,
                        vrf_selection, server_id_override, server_vrf, agent_relay_mode, max_hop_count,
                        circuit_id_format, circuit_id):
    """Update an existing DHCPv4 relay entry for an interface (VLAN or Ethernet)"""
    config_db = db.cfgdb
    ctx = click.get_current_context()

    # Check if interface exists
    if not validate_interface_exists(db, interface_name):
        ctx.fail(f"Error: Interface {interface_name} does not exist in the configDB")
    existing_entry = config_db.get_entry(DHCPV4_RELAY_TABLE, interface_name)
    if not existing_entry:
        ctx.fail(f"Error: No existing DHCPv4 relay configuration found for {interface_name}")

    updated_entry = existing_entry.copy()
    updated_fields = []

    # validating dhcpv4_servers:
    if dhcpv4_servers:
        new_servers = [ip.strip() for ip in dhcpv4_servers.split(",") if ip.strip()]
        validate_ips(ctx, new_servers, ip_version=4)

        existing_servers = existing_entry.get("dhcpv4_servers", "")
        if isinstance(existing_servers, str):
            existing_servers = existing_servers.split(",")
        combined_servers = existing_servers[:]
        for ip in new_servers:
            if ip not in combined_servers:
                combined_servers.append(ip)
        updated_entry["dhcpv4_servers"] = combined_servers
        updated_fields.append(f"DHCPv4 Servers as {dhcpv4_servers}")

    if source_interface and existing_entry.get("source_interface") != source_interface:
        if not validate_source_interface(interface_name, source_interface, db):
            ctx.fail(f"Invalid source interface {source_interface}")
        updated_entry["source_interface"] = source_interface
        updated_fields.append(f"Source Interface as {source_interface}")

    flags_dict = {
        "link_selection": link_selection,
        "vrf_selection": vrf_selection,
        "server_id_override": server_id_override,
    }
    if existing_entry.get("server_vrf"):
        for flag in ["link_selection", "vrf_selection", "server_id_override"]:
            if flags_dict[flag] == 'disable':
                ctx.fail(f"{flag} cannot be disabled when server-vrf is configured.")

    for flag in ["link_selection", "vrf_selection", "server_id_override"]:
        value = flags_dict[flag]
        if value and existing_entry.get(flag) != value:
            updated_entry[flag] = value
            updated_fields.append(f"{flag} as {value}")

    if server_vrf and existing_entry.get("server_vrf") != server_vrf:
        if server_vrf != "default" and not validate_vrf_exists(db, server_vrf):
            ctx.fail(f"VRF {server_vrf} does not exist in the VRF table.")

        for flag in ["link_selection", "vrf_selection", "server_id_override"]:
            if updated_entry.get(flag, existing_entry.get(flag)) != "enable":
                ctx.fail("server-vrf requires link-selection, vrf-selection and server-id-override flags to be enabled.")

        updated_entry["server_vrf"] = server_vrf
        updated_fields.append(f"Server VRF as {server_vrf}")

    if agent_relay_mode and existing_entry.get("agent_relay_mode") != agent_relay_mode:
        updated_entry["agent_relay_mode"] = agent_relay_mode
        updated_fields.append(f"Agent Relay Mode as {agent_relay_mode}")

    if max_hop_count:
        if not (1 <= max_hop_count <= 16):
            ctx.fail("max-hop-count must be between 1 to 16")
        updated_entry["max_hop_count"] = max_hop_count
        updated_fields.append(f"Max Hop Count as {max_hop_count}")

    if circuit_id_format and existing_entry.get("circuit_id_format") != circuit_id_format:
        updated_entry["circuit_id_format"] = circuit_id_format
        updated_fields.append(f"Circuit ID Format as {circuit_id_format}")

    if circuit_id and existing_entry.get("circuit_id") != circuit_id:
        # Validate that circuit_id_format is set to 'custom'
        format_value = circuit_id_format if circuit_id_format else existing_entry.get("circuit_id_format")
        if format_value != "custom":
            ctx.fail("circuit-id can only be set when circuit-id-format is 'custom'")
        updated_entry["circuit_id"] = circuit_id
        updated_fields.append(f"Circuit ID as {circuit_id}")

    # Apply updated entry to the database
    config_db.set_entry(DHCPV4_RELAY_TABLE, interface_name, updated_entry)

    if updated_fields:
        click.echo(f"Updated {', '.join(updated_fields)} to {interface_name}")
    else:
        click.echo(f"No changes made to {interface_name}")


@dhcpv4_relay.command("add")
@click.option("--dhcpv4-servers", required=True, help="List of DHCPv4 relay servers")
@click.option("--source-interface", required=False, help="Source interface for DHCPv4 relay")
@click.option("--link-selection", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable link selection for DHCPv4 relay")
@click.option("--vrf-selection", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable VRF selection for DHCPv4 relay")
@click.option("--server-id-override", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable server ID override for DHCPv4 relay")
@click.option("--server-vrf", required=False, help="Server VRF name for DHCPv4 relay")
@click.option("--agent-relay-mode", required=False, type=click.Choice(["discard", "append", "replace"]),
              help="Set agent relay mode for DHCPv4 relay")
@click.option("--max-hop-count", required=False, type=int, help="Maximum hop count for DHCPv4 relay")
@click.option("--circuit-id-format", required=False, type=click.Choice(["default", "interface_ip", "vlan_name", "custom"]),
              help="Circuit ID format for DHCPv4 relay")
@click.option("--circuit-id", required=False, help="Custom Circuit ID value (used when circuit-id-format is 'custom')")
@click.argument("interface_name", metavar="<INTERFACE_NAME>", required=True)
@clicommon.pass_db
def add_dhcpv4_relay(db, dhcpv4_servers, interface_name, source_interface, link_selection, vrf_selection, server_id_override, server_vrf, agent_relay_mode, max_hop_count, circuit_id_format, circuit_id):
    """Add DHCPv4 relay configuration for an interface (VLAN or Ethernet)"""
    config_db = db.cfgdb
    ctx = click.get_current_context()

    # Check if interface exists
    if not validate_interface_exists(db, interface_name):
        ctx.fail(f"Error: Interface {interface_name} does not exist in the configDB")

    interface_entry = config_db.get_entry(DHCPV4_RELAY_TABLE, interface_name)
    if interface_entry:
        ctx.fail(f"DHCPv4 relay entry for {interface_name} already exists. Use 'update' instead.")

    relay_entry = {}
    added_fields = []

    # Adding dhcpv4_servers - 
    new_server_list = list({ip.strip() for ip in dhcpv4_servers.split(",") if ip.strip()})
    validate_ips(ctx, new_server_list, ip_version=4)
    relay_entry["dhcpv4_servers"] = new_server_list
    added_fields.append(f"DHCPv4 Servers as {','.join(sorted(new_server_list))}")

    if source_interface:
        if not validate_source_interface(interface_name, source_interface, db):
            ctx.fail(f"Invalid source interface {source_interface}")
        relay_entry["source_interface"] = source_interface
        added_fields.append(f"Source Interface as {source_interface}")

    if server_vrf:
        if not all([link_selection == "enable", vrf_selection == "enable", server_id_override == "enable"]):
            ctx.fail("server-vrf requires link-selection, vrf-selection and server-id-override flags to be enabled.")

        if server_vrf != "default" and not validate_vrf_exists(db, server_vrf):
            ctx.fail(f"VRF {server_vrf} does not exist in the VRF table.")
        relay_entry["server_vrf"] = server_vrf
        added_fields.append(f"Server VRF as {server_vrf}")

    flags_dict = {
        "link_selection": link_selection,
        "vrf_selection": vrf_selection,
        "server_id_override": server_id_override,
    }
    for flag in ["link_selection", "vrf_selection", "server_id_override"]:
        value = flags_dict[flag]
        if value:
            relay_entry[flag] = value
            added_fields.append(f"{flag} as {value}")

    if agent_relay_mode:
        relay_entry["agent_relay_mode"] = agent_relay_mode
        added_fields.append(f"Agent Relay Mode as {agent_relay_mode}")

    if max_hop_count:
        if not (1 <= max_hop_count <= 16):
            ctx.fail("max-hop-count must be between 1 to 16")
        relay_entry["max_hop_count"] = max_hop_count
        added_fields.append(f"Max Hop Count as {max_hop_count}")

    if circuit_id_format:
        relay_entry["circuit_id_format"] = circuit_id_format
        added_fields.append(f"Circuit ID Format as {circuit_id_format}")

    if circuit_id:
        # Validate that circuit_id_format is set to 'custom'
        if circuit_id_format != "custom":
            ctx.fail("circuit-id can only be set when circuit-id-format is 'custom'")
        relay_entry["circuit_id"] = circuit_id
        added_fields.append(f"Circuit ID as {circuit_id}")

    config_db.set_entry(DHCPV4_RELAY_TABLE, interface_name, relay_entry)
    click.echo(f"Added {', '.join(added_fields)} to {interface_name}")


@dhcpv4_relay.command("del")
@click.option("--dhcpv4-servers", required=False, help="Delete list of DHCPv4 servers from DHCPv4 relay")
@click.option("--source-interface", required=False, is_flag=True, help="Delete source interface from DHCPv4 relay")
@click.option("--link-selection", required=False, is_flag=True, help="Delete link selection flag from DHCPv4 relay")
@click.option("--vrf-selection", required=False, is_flag=True, help="Delete VRF selection flag from DHCPv4 relay")
@click.option("--server-id-override", required=False, is_flag=True, help="Delete server ID override flag from DHCPv4 relay")
@click.option("--server-vrf", required=False, is_flag=True, help="Delete server VRF from DHCPv4 relay")
@click.option("--agent-relay-mode", required=False, is_flag=True, help="Delete agent relay mode from DHCPv4 relay")
@click.option("--max-hop-count", required=False, is_flag=True, help="Delete max hop count from DHCPv4 relay")
@click.argument("interface_name", metavar="<INTERFACE_NAME>", required=True)
@clicommon.pass_db
def del_dhcpv4_relay(db, dhcpv4_servers,  source_interface, link_selection, vrf_selection, server_id_override, server_vrf, agent_relay_mode, max_hop_count, interface_name):
    """Delete DHCPv4 relay configuration from interface (VLAN or Ethernet)"""
    ctx = click.get_current_context()
    config_db = db.cfgdb

    relay_entry = config_db.get_entry(DHCPV4_RELAY_TABLE, interface_name)
    if not relay_entry:
        ctx.fail(f"DHCPv4 relay configuration not found for {interface_name}")

    # delete whole dhcpv4-relay configuration incase of interface deletion
    if len([v for k, v in ctx.params.items() if k != "interface_name" and v]) == 0:
        config_db.set_entry(DHCPV4_RELAY_TABLE, interface_name, None)
        click.echo(f"Removed DHCPv4 relay configuration for {interface_name}")
        return

    deleted_fields = []
    if dhcpv4_servers and "dhcpv4_servers" in relay_entry:
        existing_servers = relay_entry.get("dhcpv4_servers")
        servers_to_delete = [ip.strip() for ip in dhcpv4_servers.split(",") if ip.strip()]
        validate_ips(ctx, servers_to_delete, ip_version=4)

        for ip in servers_to_delete:
            if ip not in existing_servers:
                ctx.fail(f"{ip} is not a configured DHCPv4 server on {interface_name}")

        updated_servers = [ip for ip in existing_servers if ip not in servers_to_delete]
        if not updated_servers:
            config_db.set_entry(DHCPV4_RELAY_TABLE, interface_name, None)
            click.echo(f"Removed DHCPv4 relay configuration for {interface_name}")
            return
        else:
            relay_entry["dhcpv4_servers"] = updated_servers
            deleted_fields.append(f"Servers [{dhcpv4_servers}]")

    if source_interface and "source_interface" in relay_entry:
        del relay_entry["source_interface"]
        deleted_fields.append("Source Interface")

    if server_vrf and "server_vrf" in relay_entry:
        del relay_entry["server_vrf"]
        deleted_fields.append("Server VRF")

    if "server_vrf" in relay_entry:
        if link_selection or vrf_selection or server_id_override:
            ctx.fail("Cannot delete link-selection, vrf-selection, or server-id-override when server-vrf is configured")

    if link_selection and "link_selection" in relay_entry:
        del relay_entry["link_selection"]
        deleted_fields.append("link-selection")

    if vrf_selection and "vrf_selection" in relay_entry:
        del relay_entry["vrf_selection"]
        deleted_fields.append("vrf-selection")

    if server_id_override and "server_id_override" in relay_entry:
        del relay_entry["server_id_override"]
        deleted_fields.append("server-id-override")

    if agent_relay_mode and "agent_relay_mode" in relay_entry:
        del relay_entry["agent_relay_mode"]
        deleted_fields.append("Agent Relay Mode")

    if max_hop_count and "max_hop_count" in relay_entry:
        del relay_entry["max_hop_count"]
        deleted_fields.append("Max Hop Count")

    config_db.set_entry(DHCPV4_RELAY_TABLE, interface_name, relay_entry)

    if deleted_fields:
        click.echo(f"Removed DHCPv4 relay configuration from {interface_name} for {', '.join(deleted_fields)}")


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
    vlan_name = "Vlan{}".format(vid)
    add_dhcp_relay(vlan_name, dhcp_relay_destinations, db, IPV6)


@dhcp_relay_ipv6_destination.command("del")
@click.argument("vid", metavar="<vid>", required=True, type=int)
@click.argument("dhcp_relay_destinations", nargs=-1, required=True)
@clicommon.pass_db
def del_dhcp_relay_ipv6_destination(db, vid, dhcp_relay_destinations):
    vlan_name = "Vlan{}".format(vid)
    del_dhcp_relay(vlan_name, dhcp_relay_destinations, db, IPV6)


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
@click.option("--agent-relay-mode", required=False, type=click.Choice(["discard", "append", "replace"]),
              help="Set agent relay mode for DHCPv4 relay")
@click.option("--max-hop-count", required=False, type=int, help="Maximum hop count for DHCPv4 relay")
@clicommon.pass_db
def add_dhcp_relay_ipv4_helper(db, vid, dhcp_relay_helpers, source_interface, link_selection, vrf_selection, server_id_override, server_vrf, agent_relay_mode, max_hop_count):
    vlan_name = "Vlan{}".format(vid)
    if is_dhcp_server_enabled(db):
        click.echo("Cannot change ipv4 dhcp_relay configuration when dhcp_server feature is enabled")
        return

    if check_sonic_dhcpv4_relay_flag(db):
        add_dhcpv4_relay.callback(",".join(dhcp_relay_helpers), vlan_name, source_interface, link_selection, vrf_selection, server_id_override, server_vrf, agent_relay_mode, max_hop_count, None, None)
        return
    else:
        if source_interface or link_selection or vrf_selection or server_id_override or server_vrf or agent_relay_mode or max_hop_count:
            click.echo(f"These parameters are applicable for new DHCPv4 Relay feature")
            return

    add_dhcp_relay(vlan_name, dhcp_relay_helpers, db, IPV4)

@dhcp_relay_ipv4_helper.command("update")
@click.argument("vid", metavar="<vid>", required=True, type=int)
@click.argument("dhcp_relay_helpers", nargs=-1, required=True)
@click.option("--source-interface", required=False, help="Source interface for DHCPv4 relay")
@click.option("--link-selection", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable link selection for DHCPv4 relay")
@click.option("--vrf-selection", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable VRF selection for DHCPv4 relay")
@click.option("--server-id-override", required=False, type=click.Choice(["enable", "disable"]), help="Enable/Disable server ID override for DHCPv4 relay")
@click.option("--server-vrf", required=False, help="Server VRF name for DHCPv4 relay")
@click.option("--agent-relay-mode", required=False, type=click.Choice(["discard", "append", "replace"]),
              help="Set agent relay mode for DHCPv4 relay")
@click.option("--max-hop-count", required=False, type=int, help="Maximum hop count for DHCPv4 relay")
@clicommon.pass_db
def update_dhcp_relay_ipv4_helper(db, vid, dhcp_relay_helpers, source_interface, link_selection, vrf_selection, server_id_override, server_vrf, agent_relay_mode, max_hop_count):
    if is_dhcp_server_enabled(db):
        click.echo("Cannot change ipv4 dhcp_relay configuration when dhcp_server feature is enabled")
        return

    if check_sonic_dhcpv4_relay_flag(db):
        update_dhcpv4_relay.callback("Vlan"+str(vid), ",".join(dhcp_relay_helpers), source_interface, link_selection, vrf_selection, server_id_override, server_vrf, agent_relay_mode, max_hop_count, None, None)
    else:
        click.echo(f"This command is applicable for new DHCPv4 Relay feature")


@dhcp_relay_ipv4_helper.command("del")
@click.argument("vid", metavar="<vid>", required=True, type=int)
@click.argument("dhcp_relay_helpers", nargs=-1, required=True)
@clicommon.pass_db
def del_dhcp_relay_ipv4_helper(db, vid, dhcp_relay_helpers):
    vlan_name = "Vlan{}".format(vid)
    if is_dhcp_server_enabled(db):
        click.echo("Cannot change ipv4 dhcp_relay configuration when dhcp_server feature is enabled")
        return
    del_dhcp_relay(vlan_name, dhcp_relay_helpers, db, IPV4)


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
    relay_entry = {}

    # Verify vlan is valid
    vlan_name = 'Vlan{}'.format(vid)
    vlan = db.cfgdb.get_entry('VLAN', vlan_name)
    if len(vlan) == 0:
        ctx.fail("{} doesn't exist".format(vlan_name))

    # Verify all ip addresses are valid and not exist in DB
    dhcp_servers = vlan.get('dhcp_servers', [])
    dhcpv6_servers = vlan.get('dhcpv6_servers', [])
    # Track if we need to update DHCPV4_RELAY table
    relay_entry = db.cfgdb.get_entry('DHCPV4_RELAY', vlan_name)
    dhcpv4_servers = relay_entry.get('dhcpv4_servers', []) if relay_entry else []

    for ip_addr in dhcp_relay_destination_ips:
        try:
            ipaddress.ip_address(ip_addr)
            if (ip_addr in dhcp_servers) or (ip_addr in dhcpv6_servers) or (ip_addr in dhcpv4_servers):
                click.echo("{} is already a DHCP relay destination for {}".format(ip_addr, vlan_name))
                continue
            if clicommon.ipaddress_type(ip_addr) == 4:
                if is_dhcp_server_enabled(db):
                    click.echo("Cannot change dhcp_relay configuration when dhcp_server feature is enabled")
                    return
                if not check_sonic_dhcpv4_relay_flag(db):
                    dhcp_servers.append(ip_addr)
                else:
                    dhcpv4_servers.append(ip_addr)
            else:
                dhcpv6_servers.append(ip_addr)
            added_servers.append(ip_addr)
        except Exception:
            ctx.fail('{} is invalid IP address'.format(ip_addr))

    ip_version = IPV4 if clicommon.ipaddress_type(ip_addr) == 4 else IPV6
    if ip_version == IPV4 and check_sonic_dhcpv4_relay_flag(db):
        if len(dhcpv4_servers):
            relay_entry['dhcpv4_servers'] = dhcpv4_servers
        db.cfgdb.set_entry('DHCPV4_RELAY', vlan_name, relay_entry)
    else:
        # Append new dhcp servers to config DB
        if len(dhcp_servers):
            vlan['dhcp_servers'] = dhcp_servers
        if len(dhcpv6_servers):
            vlan['dhcpv6_servers'] = dhcpv6_servers
        db.cfgdb.set_entry('VLAN', vlan_name, vlan)

    if len(added_servers):
        click.echo("Added DHCP relay destination addresses {} to {}".format(added_servers, vlan_name))
        try:
            restart_dhcp_relay_service(db, ip_version)
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

    # Track if we need to update DHCPV4_RELAY table
    dhcpv4_relay_changed = False
    relay_entry = db.cfgdb.get_entry('DHCPV4_RELAY', vlan_name)
    dhcpv4_servers = relay_entry.get('dhcpv4_servers', []) if relay_entry else []

    for ip_addr in dhcp_relay_destination_ips:
        if (ip_addr not in dhcp_servers) and (ip_addr not in dhcpv6_servers) and (ip_addr not in dhcpv4_servers):
            ctx.fail("{} is not a DHCP relay destination for {}".format(ip_addr, vlan_name))
        if clicommon.ipaddress_type(ip_addr) == 4:
            if is_dhcp_server_enabled(db):
                click.echo("Cannot change dhcp_relay configuration when dhcp_server feature is enabled")
                return
            if not check_sonic_dhcpv4_relay_flag(db):
                dhcp_servers.remove(ip_addr)
            else:
                if ip_addr in dhcpv4_servers:
                    dhcpv4_servers.remove(ip_addr)
                    dhcpv4_relay_changed = True
        else:
            dhcpv6_servers.remove(ip_addr)

    ip_version = IPV4 if clicommon.ipaddress_type(ip_addr) == 4 else IPV6
    # Update DHCPV4_RELAY table if needed
    if ip_version == IPV4 and check_sonic_dhcpv4_relay_flag(db) :
        if dhcpv4_relay_changed:
            if len(dhcpv4_servers) == 0:
                db.cfgdb.set_entry('DHCPV4_RELAY', vlan_name, None)
            else:
                relay_entry['dhcpv4_servers'] = dhcpv4_servers
                db.cfgdb.set_entry('DHCPV4_RELAY', vlan_name, relay_entry)

    else:
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
        restart_dhcp_relay_service(db, ip_version)
    except SystemExit as e:
        ctx.fail("Restart service dhcp_relay failed with error {}".format(e))


# subcommand of interface
@click.group(cls=clicommon.AbbreviationGroup, name='dhcp_relay')
def interface_dhcp_relay():
    """Configure DHCP relay on physical interfaces"""
    pass


@interface_dhcp_relay.command('add')
@click.argument('interface_name', metavar='<interface_name>', required=True)
@click.argument('dhcp_relay_destination_ips', nargs=-1, required=True)
@clicommon.pass_db
def add_interface_dhcp_relay_destination(db, interface_name, dhcp_relay_destination_ips):
    """Add DHCP relay destination IP address(es) to an Ethernet interface"""

    ctx = click.get_current_context()
    added_ipv4 = []
    added_ipv6 = []

    # Validate interface type
    base_table = get_interface_table_name(interface_name)
    if base_table is None:
        ctx.fail("{} is not a valid interface type (must be Ethernet)".format(interface_name))

    # Only allow Ethernet, not VLAN
    if base_table == VLAN_TABLE:
        ctx.fail("Use 'config vlan dhcp_relay' command for VLAN interfaces")

    # Separate IPv4 and IPv6 addresses
    for ip_addr in dhcp_relay_destination_ips:
        try:
            ip_obj = ipaddress.ip_address(ip_addr)
            if ip_obj.version == 4:
                added_ipv4.append(ip_addr)
            else:
                added_ipv6.append(ip_addr)
        except Exception:
            ctx.fail('{} is invalid IP address'.format(ip_addr))

    # Add IPv4 servers
    if added_ipv4:
        add_dhcp_relay(interface_name, added_ipv4, db, IPV4)

    # Add IPv6 servers
    if added_ipv6:
        add_dhcp_relay(interface_name, added_ipv6, db, IPV6)


@interface_dhcp_relay.command('del')
@click.argument('interface_name', metavar='<interface_name>', required=True)
@click.argument('dhcp_relay_destination_ips', nargs=-1, required=True)
@clicommon.pass_db
def del_interface_dhcp_relay_destination(db, interface_name, dhcp_relay_destination_ips):
    """Remove DHCP relay destination IP address(es) from an Ethernet interface"""

    ctx = click.get_current_context()
    remove_ipv4 = []
    remove_ipv6 = []

    # Validate interface type
    base_table = get_interface_table_name(interface_name)
    if base_table is None:
        ctx.fail("{} is not a valid interface type (must be Ethernet)".format(interface_name))

    # Only allow Ethernet, not VLAN
    if base_table == VLAN_TABLE:
        ctx.fail("Use 'config vlan dhcp_relay' command for VLAN interfaces")

    # Separate IPv4 and IPv6 addresses
    for ip_addr in dhcp_relay_destination_ips:
        try:
            ip_obj = ipaddress.ip_address(ip_addr)
            if ip_obj.version == 4:
                remove_ipv4.append(ip_addr)
            else:
                remove_ipv6.append(ip_addr)
        except Exception:
            ctx.fail('{} is invalid IP address'.format(ip_addr))

    # Remove IPv4 servers
    if remove_ipv4:
        del_dhcp_relay(interface_name, remove_ipv4, db, IPV4)

    # Remove IPv6 servers
    if remove_ipv6:
        del_dhcp_relay(interface_name, remove_ipv6, db, IPV6)


def register(cli):
    cli.add_command(dhcp_relay)
    cli.add_command(dhcpv4_relay)
    cli.commands['vlan'].add_command(vlan_dhcp_relay)
    cli.commands['interface'].add_command(interface_dhcp_relay)


if __name__ == '__main__':
    dhcp_relay()
    dhcpv4_relay()
    vlan_dhcp_relay()
    interface_dhcp_relay()
