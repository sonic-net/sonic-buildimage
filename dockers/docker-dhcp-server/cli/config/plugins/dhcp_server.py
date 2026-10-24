import click
import utilities_common.cli as clicommon

import ipaddress
import string
import unicodedata


SUPPORTED_TYPE = ["binary", "boolean", "ipv4-address", "string", "uint8", "uint16", "uint32"]
SUPPORTED_MATCH_TYPE = ["circuit_id", "option60"]


def validate_str_type(type_, value):
    """
    To validate whether type is consistent with string value
    Args:
        type: string, value type
        value: checked value
    Returns:
        True, type consistent with value
        False, type not consistent with value
    """
    if not isinstance(value, str):
        return False
    if type_ not in SUPPORTED_TYPE:
        return False
    if type_ == "string":
        return True
    if type_ == "binary":
        if len(value) == 0 or len(value) % 2 != 0:
            return False
        return all(c in set(string.hexdigits) for c in value)
    if type_ == "boolean":
        return value in ["true", "false"]
    if type_ == "ipv4-address":
        try:
            if len(value.split(".")) != 4:
                return False
            return ipaddress.ip_address(value).version == 4
        except ValueError:
            return False
    if type_.startswith("uint"):
        if not value.isdigit():
            return False
        length = int("".join([c for c in type_ if c.isdigit()]))
        return 0 <= int(value) <= int(pow(2, length)) - 1
    return False


def validate_name(value, name, forbidden=""):
    """Validate a user-supplied name.

    `forbidden` lists characters that cannot round-trip through Config DB and so
    must be rejected for this particular name, mirroring the YANG model:
    '|' is the key separator, and ',' is the leaf-list separator used to join
    DHCP_SERVER_IPV4_BINDING/matches into a single 'matches@' string. It is
    deliberately empty for values that are neither a key nor a leaf-list member.
    """
    if not value or len(value) > 255:
        click.get_current_context().fail("{} must be between 1 and 255 characters".format(name))
    if any(char in value for char in forbidden):
        click.get_current_context().fail("{} must not contain {}".format(
            name, " or ".join("'{}'".format(char) for char in forbidden)))


def validate_key_name(value, name, forbidden):
    """Validate Config DB keys without normalizing their identity."""
    validate_name(value, name, forbidden)
    if any(unicodedata.category(char) in ("Cc", "Zs", "Zl", "Zp") for char in (value[0], value[-1])):
        click.get_current_context().fail(
            "{} must not have leading or trailing whitespace or control characters".format(name))


def split_csv(value, name):
    if not value:
        return []
    values = value.split(",")
    if any(not item for item in values):
        click.get_current_context().fail("{} cannot contain empty values".format(name))
    if len(values) != len(set(values)):
        click.get_current_context().fail("{} cannot contain duplicate values".format(name))
    return values


def get_vlan_subnets(dbconn, dhcp_interface):
    prefix = "VLAN_INTERFACE|{}|".format(dhcp_interface)
    subnets = []
    for key in dbconn.keys("CONFIG_DB", prefix + "*"):
        try:
            intf = ipaddress.ip_interface(key[len(prefix):])
        except ValueError:
            click.get_current_context().fail("Invalid subnet configured on vlan {}".format(dhcp_interface))
        if intf.version == 4:
            subnets.append(intf.network)
    if not subnets:
        click.get_current_context().fail("Vlan {} has no IPv4 subnet".format(dhcp_interface))
    return subnets


def validate_ip_pool(ips, subnets, dhcp_interface):
    for ip in ips:
        if not validate_str_type("ipv4-address", ip):
            click.get_current_context().fail("Illegal IP address {}".format(ip))
        if not any(ipaddress.ip_address(ip) in subnet for subnet in subnets):
            click.get_current_context().fail("IP {} is not in any subnet of vlan {}".format(ip, dhcp_interface))


def validate_range_pool(dbconn, ranges, subnets, dhcp_interface, range_overrides=None):
    for range_name in ranges:
        key = "DHCP_SERVER_IPV4_RANGE|" + range_name
        if range_overrides and range_name in range_overrides:
            range_value = range_overrides[range_name]
        else:
            if not dbconn.exists("CONFIG_DB", key):
                click.get_current_context().fail("Range {} does not exist".format(range_name))
            range_value = dbconn.get("CONFIG_DB", key, "range@")
        range_ips = range_value.split(",") if range_value else []
        if len(range_ips) not in [1, 2]:
            click.get_current_context().fail("Range {} has an invalid value".format(range_name))
        ip_start = range_ips[0]
        ip_end = range_ips[-1]
        if not validate_str_type("ipv4-address", ip_start) or not validate_str_type("ipv4-address", ip_end):
            click.get_current_context().fail("Range {} has an invalid IPv4 address".format(range_name))
        if count_ipv4(ip_start, ip_end) < 1:
            click.get_current_context().fail("Range {} has an invalid address order".format(range_name))
        if not any(ipaddress.ip_address(ip_start) in subnet and ipaddress.ip_address(ip_end) in subnet
                   for subnet in subnets):
            click.get_current_context().fail(
                "Range {} is not in any subnet of vlan {}".format(range_name, dhcp_interface))


def get_match_entry(dbconn, match_name, match_overrides=None):
    if match_overrides and match_name in match_overrides:
        entry = match_overrides[match_name]
    else:
        key = "DHCP_SERVER_IPV4_MATCH|" + match_name
        if not dbconn.exists("CONFIG_DB", key):
            click.get_current_context().fail("Match {} does not exist".format(match_name))
        entry = dbconn.get_all("CONFIG_DB", key)
    if entry.get("type") not in SUPPORTED_MATCH_TYPE:
        click.get_current_context().fail("Match {} has unsupported type {}".format(match_name, entry.get("type", "")))
    if not entry.get("value") or len(entry["value"]) > 255:
        click.get_current_context().fail("Match {} has an invalid value".format(match_name))
    return entry


def resolve_port_alias(dbconn, alias):
    """Resolve a circuit_id value to the single port that dhcp_relay would stamp.

    dhcp_relay uses the port alias, falls back to the port name when the port has
    no alias, and stamps PortChannels with their name, so all three are accepted
    here. This mirrors _parse_port_map_alias() in dhcp_cfggen.
    """
    ports = []
    for key in dbconn.keys("CONFIG_DB", "PORT|*"):
        port = key.split("|", 1)[1]
        if (dbconn.get("CONFIG_DB", key, "alias") or port) == alias:
            ports.append(port)
    for key in dbconn.keys("CONFIG_DB", "PORTCHANNEL|*"):
        portchannel = key.split("|", 1)[1]
        if portchannel == alias:
            ports.append(portchannel)
    if not ports:
        click.get_current_context().fail("Circuit ID alias {} does not exist".format(alias))
    if len(ports) > 1:
        click.get_current_context().fail("Circuit ID alias {} is ambiguous".format(alias))
    return ports[0]


def validate_binding_entry(dbconn, dhcp_interface, binding_name, entry, match_overrides=None,
                           range_overrides=None):
    validate_key_name(binding_name, "Binding name", "|")
    if not dbconn.exists("CONFIG_DB", "DHCP_SERVER_IPV4|" + dhcp_interface):
        click.get_current_context().fail("Interface {} is not a valid dhcp interface".format(dhcp_interface))

    match_names = split_csv(entry.get("matches@"), "Match list")
    if not match_names:
        click.get_current_context().fail("Binding {} must reference at least one match".format(binding_name))

    conditions = {}
    for match_name in match_names:
        validate_key_name(match_name, "Match name", "|,")
        match = get_match_entry(dbconn, match_name, match_overrides)
        match_type = match["type"]
        if match_type in conditions:
            click.get_current_context().fail(
                "Binding {} cannot reference multiple matches of type {}".format(binding_name, match_type)
            )
        conditions[match_type] = match["value"]
        if match_type == "circuit_id":
            port = resolve_port_alias(dbconn, match["value"])
            if not dbconn.exists("CONFIG_DB", "VLAN_MEMBER|{}|{}".format(dhcp_interface, port)):
                click.get_current_context().fail(
                    "Circuit ID alias {} is not a member of vlan {}".format(match["value"], dhcp_interface)
                )

    has_ips = bool(entry.get("ips@"))
    has_ranges = bool(entry.get("ranges@"))
    if has_ips == has_ranges:
        click.get_current_context().fail("Exactly one of IP list or range list must be provided")

    subnets = get_vlan_subnets(dbconn, dhcp_interface)
    if has_ips:
        validate_ip_pool(split_csv(entry["ips@"], "IP list"), subnets, dhcp_interface)
    else:
        validate_range_pool(dbconn, split_csv(entry["ranges@"], "Range list"), subnets, dhcp_interface,
                            range_overrides)
    return conditions


def predicates_overlap(first, second):
    return all(match_type not in second or second[match_type] == value for match_type, value in first.items())


def validate_vlan_bindings(dbconn, dhcp_interface, candidate=None, match_overrides=None,
                           range_overrides=None):
    bindings = {}
    prefix = "DHCP_SERVER_IPV4_BINDING|{}|".format(dhcp_interface)
    for key in dbconn.keys("CONFIG_DB", prefix + "*"):
        bindings[key[len(prefix):]] = dbconn.get_all("CONFIG_DB", key)
    if candidate:
        bindings[candidate[0]] = candidate[1]
    predicates = {}
    for binding_name in sorted(bindings):
        predicates[binding_name] = validate_binding_entry(
            dbconn, dhcp_interface, binding_name, bindings[binding_name], match_overrides,
            range_overrides
        )

    binding_names = sorted(predicates)
    for index, first_name in enumerate(binding_names):
        for second_name in binding_names[index + 1:]:
            first = predicates[first_name]
            second = predicates[second_name]
            if len(first) == len(second) and predicates_overlap(first, second):
                click.get_current_context().fail(
                    "Bindings {} and {} overlap with equal specificity on {}".format(
                        first_name, second_name, dhcp_interface
                    )
                )


def validate_range_update(dbconn, range_name, updated_value):
    """Re-validate every binding that references a range before its endpoints move.

    A binding's pool is checked against the VLAN subnet when the binding itself is
    written, but the range it names can be edited afterwards. dhcp_cfggen aborts
    generation of the entire Kea config - not just the offending binding - when a
    pool falls outside the VLAN, so an unchecked range update can take DHCP down
    for every interface on the box, not only this one.

    DHCP_SERVER_IPV4_PORT entries can also reference a range and are deliberately
    not covered here: they are validated nowhere today, so guarding them would
    change behaviour that predates this series and belongs in its own change.
    """
    affected_vlans = set()
    for key in dbconn.keys("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|*"):
        entry = dbconn.get_all("CONFIG_DB", key)
        if range_name in entry.get("ranges@", "").split(","):
            affected_vlans.add(key.split("|", 2)[1])
    for dhcp_interface in sorted(affected_vlans):
        validate_vlan_bindings(
            dbconn,
            dhcp_interface,
            range_overrides={range_name: updated_value},
        )


def validate_match_update(dbconn, match_name, updated_entry):
    affected_vlans = set()
    for key in dbconn.keys("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|*"):
        entry = dbconn.get_all("CONFIG_DB", key)
        if match_name in entry.get("matches@", "").split(","):
            affected_vlans.add(key.split("|", 2)[1])
    for dhcp_interface in sorted(affected_vlans):
        validate_vlan_bindings(
            dbconn,
            dhcp_interface,
            match_overrides={match_name: updated_entry},
        )


@click.group(cls=clicommon.AbbreviationGroup, name="dhcp_server", invoke_without_command=True)
@clicommon.pass_db
def dhcp_server(db):
    """config DHCP Server information"""
    ctx = click.get_current_context()
    dbconn = db.db
    if dbconn.get("CONFIG_DB", "FEATURE|dhcp_server", "state") != "enabled":
        ctx.fail("Feature dhcp_server is not enabled")
    if ctx.invoked_subcommand is None:
        click.echo(ctx.get_help())
        ctx.exit()


@dhcp_server.group(cls=clicommon.AliasedGroup, name="ipv4")
def dhcp_server_ipv4():
    """Show ipv4 related dhcp_server info"""
    pass


@dhcp_server_ipv4.command(name="add")
@click.argument("dhcp_interface", required=True)
@click.option("--mode", required=True)
@click.option("--lease_time", required=False, default="900")
@click.option("--dup_gw_nm", required=False, default=False, is_flag=True)
@click.option("--gateway", required=False)
@click.option("--netmask", required=False)
@clicommon.pass_db
def dhcp_server_ipv4_add(db, mode, lease_time, dup_gw_nm, gateway, netmask, dhcp_interface):
    ctx = click.get_current_context()
    if mode != "PORT":
        ctx.fail("Only mode PORT is supported")
    if not validate_str_type("uint32", lease_time):
        ctx.fail("lease_time is required and must be nonnegative integer")
    dbconn = db.db
    if not dbconn.exists("CONFIG_DB", "VLAN_INTERFACE|" + dhcp_interface):
        ctx.fail("dhcp_interface {} does not exist".format(dhcp_interface))
    if dup_gw_nm:
        dup_success = False
        for key in dbconn.keys("CONFIG_DB", "VLAN_INTERFACE|" + dhcp_interface + "|*"):
            intf = ipaddress.ip_interface(key.split("|")[2])
            if intf.version != 4:
                continue
            dup_success = True
            gateway, netmask = str(intf.ip), str(intf.netmask)
        if not dup_success:
            ctx.fail("failed to found gateway and netmask for Vlan interface {}".format(dhcp_interface))
    elif not validate_str_type("ipv4-address", gateway) or not validate_str_type("ipv4-address", netmask):
        ctx.fail("gateway and netmask must be valid ipv4 string")
    key = "DHCP_SERVER_IPV4|" + dhcp_interface
    if dbconn.exists("CONFIG_DB", key):
        ctx.fail("Dhcp_interface {} already exist".format(dhcp_interface))
    else:
        dbconn.hmset("CONFIG_DB", key, {
            "mode": mode,
            "lease_time": lease_time,
            "gateway": gateway,
            "netmask": netmask,
            "state": "disabled",
            })


@dhcp_server_ipv4.command(name="del")
@click.argument("dhcp_interface", required=True)
@clicommon.pass_db
def dhcp_server_ipv4_del(db, dhcp_interface):
    ctx = click.get_current_context()
    dbconn = db.db
    key = "DHCP_SERVER_IPV4|" + dhcp_interface
    if dbconn.exists("CONFIG_DB", key):
        binding_keys = dbconn.keys("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|{}|*".format(dhcp_interface))
        if binding_keys:
            ctx.fail("Dhcp interface {} has match bindings and cannot be deleted".format(dhcp_interface))
        click.echo("Dhcp interface {} exists in config db, proceed to delete".format(dhcp_interface))
        dbconn.delete("CONFIG_DB", key)
    else:
        ctx.fail("Dhcp interface {} does not exist in config db".format(dhcp_interface))


@dhcp_server_ipv4.command(name="update")
@click.argument("dhcp_interface", required=True)
@click.option("--mode", required=False)
@click.option("--lease_time", required=False)
@click.option("--dup_gw_nm", required=False, default=False, is_flag=True)
@click.option("--gateway", required=False)
@click.option("--netmask", required=False)
@clicommon.pass_db
def dhcp_server_ipv4_update(db, mode, lease_time, dup_gw_nm, gateway, netmask, dhcp_interface):
    ctx = click.get_current_context()
    dbconn = db.db
    interface_key = "DHCP_SERVER_IPV4|" + dhcp_interface
    if not dbconn.exists("CONFIG_DB", interface_key):
        ctx.fail("Dhcp interface {} does not exist in config db".format(dhcp_interface))
    updates = {}
    if mode:
        if mode != "PORT":
            ctx.fail("Only mode PORT is supported")
        updates["mode"] = mode
    if lease_time:
        if not validate_str_type("uint32", lease_time):
            ctx.fail("lease_time is required and must be nonnegative integer")
        updates["lease_time"] = lease_time
    if dup_gw_nm:
        dup_success = False
        for key in dbconn.keys("CONFIG_DB", "VLAN_INTERFACE|" + dhcp_interface + "|*"):
            intf = ipaddress.ip_interface(key.split("|")[2])
            if intf.version != 4:
                continue
            dup_success = True
            gateway, netmask = str(intf.ip), str(intf.netmask)
        if not dup_success:
            ctx.fail("failed to found gateway and netmask for Vlan interface {}".format(dhcp_interface))
    elif gateway and not validate_str_type("ipv4-address", gateway):
        ctx.fail("gateway must be valid ipv4 string")
    elif netmask and not validate_str_type("ipv4-address", netmask):
        ctx.fail("netmask must be valid ipv4 string")
    if gateway:
        updates["gateway"] = gateway
    if netmask:
        updates["netmask"] = netmask
    for field, value in updates.items():
        dbconn.set("CONFIG_DB", interface_key, field, value)


@dhcp_server_ipv4.command(name="enable")
@click.argument("dhcp_interface", required=True)
@clicommon.pass_db
def dhcp_server_ipv4_enable(db, dhcp_interface):
    ctx = click.get_current_context()
    dbconn = db.db
    key = "DHCP_SERVER_IPV4|" + dhcp_interface
    if dbconn.exists("CONFIG_DB", key):
        dbconn.set("CONFIG_DB", key, "state", "enabled")
    else:
        ctx.fail("Failed to enable, dhcp interface {} does not exist".format(dhcp_interface))


@dhcp_server_ipv4.command(name="disable")
@click.argument("dhcp_interface", required=True)
@clicommon.pass_db
def dhcp_server_ipv4_disable(db, dhcp_interface):
    ctx = click.get_current_context()
    dbconn = db.db
    key = "DHCP_SERVER_IPV4|" + dhcp_interface
    if dbconn.exists("CONFIG_DB", key):
        dbconn.set("CONFIG_DB", key, "state", "disabled")
    else:
        ctx.fail("Failed to disable, dhcp interface {} does not exist".format(dhcp_interface))


@dhcp_server_ipv4.group(cls=clicommon.AliasedGroup, name="range")
def dhcp_server_ipv4_range():
    pass


def count_ipv4(start, end):
    ip1 = int(ipaddress.IPv4Address(start))
    ip2 = int(ipaddress.IPv4Address(end))
    return ip2 - ip1 + 1


@dhcp_server_ipv4_range.command(name="add")
@click.argument("range_name", required=True)
@click.argument("ip_start", required=True)
@click.argument("ip_end", required=False)
@clicommon.pass_db
def dhcp_server_ipv4_range_add(db, range_name, ip_start, ip_end):
    ctx = click.get_current_context()
    if not ip_end:
        ip_end = ip_start
    if not validate_str_type("ipv4-address", ip_start) or not validate_str_type("ipv4-address", ip_end):
        ctx.fail("ip_start or ip_end is not valid ipv4 address")
    if count_ipv4(ip_start, ip_end) < 1:
        ctx.fail("range value is illegal")
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_RANGE|" + range_name
    if dbconn.exists("CONFIG_DB", key):
        ctx.fail("Range {} already exist".format(range_name))
    else:
        dbconn.hmset("CONFIG_DB", key, {"range@": ip_start + "," + ip_end})


@dhcp_server_ipv4_range.command(name="update")
@click.argument("range_name", required=True)
@click.argument("ip_start", required=True)
@click.argument("ip_end", required=False)
@clicommon.pass_db
def dhcp_server_ipv4_range_update(db, range_name, ip_start, ip_end):
    ctx = click.get_current_context()
    if not ip_end:
        ip_end = ip_start
    if not validate_str_type("ipv4-address", ip_start) or not validate_str_type("ipv4-address", ip_end):
        ctx.fail("ip_start or ip_end is not valid ipv4 address")
    if count_ipv4(ip_start, ip_end) < 1:
        ctx.fail("range value is illegal")
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_RANGE|" + range_name
    if dbconn.exists("CONFIG_DB", key):
        validate_range_update(dbconn, range_name, ip_start + "," + ip_end)
        dbconn.set("CONFIG_DB", key, "range@", ip_start + "," + ip_end)
    else:
        ctx.fail("Range {} does not exist, cannot update".format(range_name))


@dhcp_server_ipv4_range.command(name="del")
@click.argument("range_name", required=True)
@click.option("--force", required=False, default=False, is_flag=True)
@clicommon.pass_db
def dhcp_sever_ipv4_range_del(db, range_name, force):
    ctx = click.get_current_context()
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_RANGE|" + range_name
    if dbconn.exists("CONFIG_DB", key):
        if not force:
            for port in dbconn.keys("CONFIG_DB", "DHCP_SERVER_IPV4_PORT*"):
                ranges = dbconn.get("CONFIG_DB", port, "ranges@")
                if ranges and range_name in ranges.split(","):
                    ctx.fail("Range {} is referenced in {}, cannot delete, add --force to bypass or range unbind to unbind range first".format(range_name, port))
            for binding in dbconn.keys("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|*"):
                ranges = dbconn.get("CONFIG_DB", binding, "ranges@")
                if ranges and range_name in ranges.split(","):
                    ctx.fail(
                        "Range {} is referenced in {}, cannot delete, add --force to "
                        "bypass or delete the binding first".format(
                            range_name, binding
                        )
                    )
        dbconn.delete("CONFIG_DB", key)
    else:
        ctx.fail("Range {} does not exist, cannot delete".format(range_name))


@dhcp_server_ipv4.group(cls=clicommon.AliasedGroup, name="match")
def dhcp_server_ipv4_match():
    pass


@dhcp_server_ipv4_match.command(name="add")
@click.argument("match_name", required=True)
@click.option("--type", "match_type", required=True)
@click.option("--value", required=True)
@clicommon.pass_db
def dhcp_server_ipv4_match_add(db, match_name, match_type, value):
    ctx = click.get_current_context()
    validate_key_name(match_name, "Match name", "|,")
    validate_name(value, "Match value")
    if match_type not in SUPPORTED_MATCH_TYPE:
        ctx.fail("Only match types {} are supported".format(", ".join(SUPPORTED_MATCH_TYPE)))
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_MATCH|" + match_name
    if dbconn.exists("CONFIG_DB", key):
        ctx.fail("Match {} already exists".format(match_name))
    # The model requires a circuit_id to name a real port even when no binding
    # references the match yet, so check here rather than only at binding time.
    if match_type == "circuit_id":
        resolve_port_alias(dbconn, value)
    dbconn.hmset("CONFIG_DB", key, {"type": match_type, "value": value})


@dhcp_server_ipv4_match.command(name="update")
@click.argument("match_name", required=True)
@click.option("--type", "match_type", required=False)
@click.option("--value", required=False)
@clicommon.pass_db
def dhcp_server_ipv4_match_update(db, match_name, match_type, value):
    ctx = click.get_current_context()
    validate_key_name(match_name, "Match name", "|,")
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_MATCH|" + match_name
    if not dbconn.exists("CONFIG_DB", key):
        ctx.fail("Match {} does not exist".format(match_name))
    if match_type and match_type not in SUPPORTED_MATCH_TYPE:
        ctx.fail("Only match types {} are supported".format(", ".join(SUPPORTED_MATCH_TYPE)))
    if value is not None:
        validate_name(value, "Match value")

    updated_entry = dict(dbconn.get_all("CONFIG_DB", key))
    if match_type:
        updated_entry["type"] = match_type
    if value is not None:
        updated_entry["value"] = value
    if updated_entry.get("type") == "circuit_id":
        resolve_port_alias(dbconn, updated_entry.get("value"))
    validate_match_update(dbconn, match_name, updated_entry)
    dbconn.hmset("CONFIG_DB", key, updated_entry)


@dhcp_server_ipv4_match.command(name="del")
@click.argument("match_name", required=True)
@clicommon.pass_db
def dhcp_server_ipv4_match_del(db, match_name):
    ctx = click.get_current_context()
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_MATCH|" + match_name
    if not dbconn.exists("CONFIG_DB", key):
        ctx.fail("Match {} does not exist".format(match_name))
    for binding in dbconn.keys("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|*"):
        matches = dbconn.get("CONFIG_DB", binding, "matches@")
        if matches and match_name in matches.split(","):
            ctx.fail("Match {} is referenced in {} and cannot be deleted".format(match_name, binding))
    dbconn.delete("CONFIG_DB", key)


def build_match_field(match_list, binding_name):
    match_names = split_csv(match_list, "Match list")
    if not match_names:
        click.get_current_context().fail("Binding {} must reference at least one match".format(binding_name))
    return ",".join(sorted(match_names))


def build_pool_fields(range_, ip_list):
    ctx = click.get_current_context()
    if bool(range_) == bool(ip_list):
        ctx.fail("Exactly one of IP list or range list must be provided")
    if ip_list:
        ips = split_csv(ip_list, "IP list")
        for ip in ips:
            if not validate_str_type("ipv4-address", ip):
                ctx.fail("Illegal IP address {}".format(ip))
        return {"ips@": ",".join(sorted(ips, key=ipaddress.ip_address))}
    ranges = split_csv(range_, "Range list")
    return {"ranges@": ",".join(sorted(ranges))}


@dhcp_server_ipv4.group(cls=clicommon.AliasedGroup, name="binding")
def dhcp_server_ipv4_binding():
    pass


@dhcp_server_ipv4_binding.command(name="add")
@click.argument("dhcp_interface", required=True)
@click.argument("binding_name", required=True)
@click.option("--match", "match_list", required=True)
@click.option("--range", "range_", required=False)
@click.argument("ip_list", required=False)
@clicommon.pass_db
def dhcp_server_ipv4_binding_add(db, dhcp_interface, binding_name, match_list, range_, ip_list):
    ctx = click.get_current_context()
    dbconn = db.db
    validate_key_name(binding_name, "Binding name", "|")

    entry = {"matches@": build_match_field(match_list, binding_name)}
    entry.update(build_pool_fields(range_, ip_list))

    key = "DHCP_SERVER_IPV4_BINDING|{}|{}".format(dhcp_interface, binding_name)
    if dbconn.exists("CONFIG_DB", key):
        ctx.fail("Binding {} already exists on {}".format(binding_name, dhcp_interface))
    validate_vlan_bindings(dbconn, dhcp_interface, candidate=(binding_name, entry))
    dbconn.hmset("CONFIG_DB", key, entry)


@dhcp_server_ipv4_binding.command(name="update")
@click.argument("dhcp_interface", required=True)
@click.argument("binding_name", required=True)
@click.option("--match", "match_list", required=False)
@click.option("--range", "range_", required=False)
@click.argument("ip_list", required=False)
@clicommon.pass_db
def dhcp_server_ipv4_binding_update(db, dhcp_interface, binding_name, match_list, range_, ip_list):
    ctx = click.get_current_context()
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_BINDING|{}|{}".format(dhcp_interface, binding_name)
    if not dbconn.exists("CONFIG_DB", key):
        ctx.fail("Binding {} does not exist on {}".format(binding_name, dhcp_interface))
    if not match_list and not range_ and not ip_list:
        ctx.fail("Nothing to update, provide at least one of --match, --range or an IP list")

    existing = dict(dbconn.get_all("CONFIG_DB", key))
    entry = {"matches@": existing.get("matches@", "")}
    if match_list:
        entry["matches@"] = build_match_field(match_list, binding_name)
    if range_ or ip_list:
        entry.update(build_pool_fields(range_, ip_list))
    elif existing.get("ips@"):
        entry["ips@"] = existing["ips@"]
    else:
        entry["ranges@"] = existing.get("ranges@", "")

    validate_vlan_bindings(dbconn, dhcp_interface, candidate=(binding_name, entry))
    dbconn.delete("CONFIG_DB", key)
    dbconn.hmset("CONFIG_DB", key, entry)


@dhcp_server_ipv4_binding.command(name="del")
@click.argument("dhcp_interface", required=True)
@click.argument("binding_name", required=True)
@clicommon.pass_db
def dhcp_server_ipv4_binding_del(db, dhcp_interface, binding_name):
    ctx = click.get_current_context()
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_BINDING|{}|{}".format(dhcp_interface, binding_name)
    if not dbconn.exists("CONFIG_DB", key):
        ctx.fail("Binding {} does not exist on {}".format(binding_name, dhcp_interface))
    dbconn.delete("CONFIG_DB", key)


@dhcp_server_ipv4.command(name="bind")
@click.argument("dhcp_interface", required=True)
@click.argument("member_interface", required=True)
@click.option("--range", "range_", required=False)
@click.argument("ip_list", required=False)
@clicommon.pass_db
def dhcp_server_ipv4_ip_bind(db, dhcp_interface, member_interface, range_, ip_list):
    ctx = click.get_current_context()
    dbconn = db.db
    if not dbconn.exists("CONFIG_DB", "VLAN_MEMBER|" + dhcp_interface + "|" + member_interface):
        ctx.fail("Cannot confirm member interface {} is really in dhcp interface {}".format(member_interface, dhcp_interface))
    vlan_prefix = "VLAN_INTERFACE|" + dhcp_interface + "|"
    subnets = [ipaddress.ip_network(key[len(vlan_prefix):], strict=False) for key in dbconn.keys("CONFIG_DB", vlan_prefix + "*")]
    if range_:
        range_ = set(range_.split(","))
        for r in range_:
            if not dbconn.exists("CONFIG_DB", "DHCP_SERVER_IPV4_RANGE|" + r):
                ctx.fail("Cannot bind nonexistent range {} to interface".format(r))
            ip_range = dbconn.get("CONFIG_DB", "DHCP_SERVER_IPV4_RANGE|" + r, "range@").split(",")
            if len(ip_range) == 1:
                ip_start = ip_range[0]
                ip_end = ip_range[0]
            if len(ip_range) == 2:
                ip_start = ip_range[0]
                ip_end = ip_range[1]
            if not any([ipaddress.ip_address(ip_start) in subnet and ipaddress.ip_address(ip_end) in subnet for subnet in subnets]):
                ctx.fail("Range {} is not in any subnet of vlan {}".format(r, dhcp_interface))
    if ip_list:
        ip_list = set(ip_list.split(","))
        for ip in ip_list:
            if not validate_str_type("ipv4-address", ip):
                ctx.fail("Illegal IP address {}".format(ip))
            if not any([ipaddress.ip_address(ip) in subnet for subnet in subnets]):
                ctx.fail("IP {} is not in any subnet of vlan {}".format(ip, dhcp_interface))
    if range_ and ip_list or not range_ and not ip_list:
        ctx.fail("Only one of range and ip list need to be provided")
    key = "DHCP_SERVER_IPV4_PORT|" + dhcp_interface + "|" + member_interface
    key_exist = dbconn.exists("CONFIG_DB", key)
    for bind_value_name, bind_value in [["ips@", ip_list], ["ranges@", range_]]:
        if key_exist:
            existing_value = dbconn.get("CONFIG_DB", key, bind_value_name)
            if (not not existing_value) == (not bind_value):
                ctx.fail("IP bind cannot have ip range and ip list configured at the same time")
            if bind_value:
                value_set = set(existing_value.split(",")) if existing_value else set()
                new_value_set = value_set.union(bind_value)
                dbconn.set("CONFIG_DB", key, bind_value_name, ",".join(new_value_set))
        elif bind_value:
            dbconn.hmset("CONFIG_DB", key, {bind_value_name: ",".join(bind_value)})


@dhcp_server_ipv4.command(name="unbind")
@click.argument("dhcp_interface", required=True)
@click.argument("member_interface", required=True)
@click.option("--range", "range_", required=False)
@click.argument("ip_list", required=False)
@clicommon.pass_db
def dhcp_server_ipv4_ip_unbind(db, dhcp_interface, member_interface, range_, ip_list):
    ctx = click.get_current_context()
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_PORT|" + dhcp_interface + "|" + member_interface
    if ip_list == "all":
        dbconn.delete("CONFIG_DB", key)
        return
    if range_ and ip_list or not range_ and not ip_list:
        ctx.fail("Only one of range and ip list need to be provided")
    if not dbconn.exists("CONFIG_DB", key):
        ctx.fail("The specified dhcp_interface and member interface is not bind to ip or range")
    for unbind_value_name, unbind_value in [["ips@", ip_list], ["ranges@", range_]]:
        if unbind_value:
            unbind_value = set(unbind_value.split(","))
            existing_value = dbconn.get("CONFIG_DB", key, unbind_value_name)
            value_set = set(existing_value.split(",")) if existing_value else set()
            if value_set.issuperset(unbind_value):
                new_value_set = value_set.difference(unbind_value)
                if new_value_set:
                    dbconn.set("CONFIG_DB", key, unbind_value_name, ",".join(new_value_set))
                else:
                    dbconn.delete("CONFIG_DB", key)
            else:
                ctx.fail("Attempting to unbind range or ip that is not binded")


@dhcp_server_ipv4.group(cls=clicommon.AliasedGroup, name="option")
def dhcp_server_ipv4_option():
    pass


SUPPORTED_OPTION_ID = ["147", "148", "149", "163", "164", "165", "166", "167", "168", "169", "170", "171", "172", "173", "174", "178", "179", "180", "181", "182", "183", "184", "185", "186", "187", "188", "189", "190", "191", "192", "193", "194", "195", "196", "197", "198", "199", "200", "201", "202", "203", "204", "205", "206", "207", "214", "215", "216", "217", "218", "219", "222", "223"]


@dhcp_server_ipv4_option.command(name="add")
@click.argument("option_name", required=True)
@click.argument("option_id", required=True)
@click.argument("type_", required=True)
@click.argument("value", required=True)
@clicommon.pass_db
def dhcp_server_ipv4_option_add(db, option_name, option_id, type_, value):
    ctx = click.get_current_context()
    if option_id not in SUPPORTED_OPTION_ID:
        ctx.fail("Option id {} is not supported".format(option_id))
    if type_ not in SUPPORTED_TYPE:
        ctx.fail("Input type is not supported")
    if not validate_str_type(type_, value):
        ctx.fail("Value {} is not of type {}".format(value, type_))
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS|" + option_name
    if dbconn.exists("CONFIG_DB", key):
        ctx.fail("Option {} already exist".format(option_name))
    dbconn.hmset("CONFIG_DB", key, {
        "id": option_id,
        "type": type_,
        "value": value,
        })


@dhcp_server_ipv4_option.command(name="del")
@click.argument("option_name", required=True)
@clicommon.pass_db
def dhcp_server_ipv4_option_del(db, option_name):
    ctx = click.get_current_context()
    dbconn = db.db
    option_key = "DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS|" + option_name
    if not dbconn.exists("CONFIG_DB", option_key):
        ctx.fail("Option {} does not exist, cannot delete".format(option_name))
    for key in dbconn.keys("CONFIG_DB", "DHCP_SERVER_IPV4|*"):
        existing_options = dbconn.get("CONFIG_DB", key, "customized_options@")
        if existing_options and option_name in existing_options.split(","):
            ctx.fail("Option {} is referenced in {}, cannot delete".format(option_name, key[len("DHCP_SERVER_IPV4|"):]))
    dbconn.delete("CONFIG_DB", option_key)


@dhcp_server_ipv4_option.command(name="bind")
@click.argument("dhcp_interface", required=True)
@click.argument("option_list", required=True)
@clicommon.pass_db
def dhcp_server_ipv4_option_bind(db, dhcp_interface, option_list):
    ctx = click.get_current_context()
    dbconn = db.db
    key = "DHCP_SERVER_IPV4|" + dhcp_interface
    if not dbconn.exists("CONFIG_DB", key):
        ctx.fail("Interface {} is not valid dhcp interface".format(dhcp_interface))
    option_list = option_list.split(",")
    for option_name in option_list:
        option_key = "DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS|" + option_name
        if not dbconn.exists("CONFIG_DB", option_key):
            ctx.fail("Option {} does not exist, cannot bind".format(option_name))
    existing_value = dbconn.get("CONFIG_DB", key, "customized_options@")
    value_set = set(existing_value.split(",")) if existing_value else set()
    new_value_set = value_set.union(option_list)
    dbconn.set("CONFIG_DB", key, "customized_options@", ",".join(new_value_set))


@dhcp_server_ipv4_option.command(name="unbind")
@click.argument("dhcp_interface", required=True)
@click.argument("option_list", required=False)
@click.option("--all", "all_", required=False, default=False, is_flag=True)
@clicommon.pass_db
def dhcp_server_ipv4_option_unbind(db, dhcp_interface, option_list, all_):
    ctx = click.get_current_context()
    dbconn = db.db
    key = "DHCP_SERVER_IPV4|" + dhcp_interface
    if not dbconn.exists("CONFIG_DB", key):
        ctx.fail("Interface {} is not valid dhcp interface".format(dhcp_interface))
    if all_:
        redis_client = dbconn.get_redis_client("CONFIG_DB")
        redis_client.hdel(key, "customized_options@")
    else:
        unbind_value = set(option_list.split(","))
        existing_value = dbconn.get("CONFIG_DB", key, "customized_options@")
        value_set = set(existing_value.split(",")) if existing_value else set()
        if value_set.issuperset(unbind_value):
            new_value_set = value_set.difference(unbind_value)
            if new_value_set:
                dbconn.set("CONFIG_DB", key, "customized_options@", ",".join(new_value_set))
            else:
                redis_client = dbconn.get_redis_client("CONFIG_DB")
                redis_client.hdel(key, "customized_options@")
        else:
            ctx.fail("Attempting to unbind option that is not binded")


def register(cli):
    cli.add_command(dhcp_server)


if __name__ == '__main__':
    dhcp_server()
