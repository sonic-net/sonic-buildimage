import click
import utilities_common.cli as clicommon

import ipaddress


@click.group(cls=clicommon.AbbreviationGroup, name="dhcp_server")
def dhcp_server():
    """config DHCP Server information"""
    pass


@dhcp_server.group(cls=clicommon.AliasedGroup, name="ipv4")
def dhcp_server_ipv4():
    """Show ipv4 related dhcp_server info"""
    pass


def validate_str_type(type, value):
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
    if type not in SUPPORT_TYPE:
        return False
    if type == "string":
        return True
    if type == "binary":
        if len(value) == 0 or len(value) % 2 != 0:
            return False
        return all(c in set(string.hexdigits) for c in value)
    if type == "boolean":
        return value in ["true", "false"]
    if type == "ipv4-address":
        try:
            if len(value.split(".")) != 4:
                return False
            return ipaddress.ip_address(value).version == 4
        except ValueError:
            return False
    if type.startswith("uint"):
        if not value.isdigit():
            return False
        length = int("".join([c for c in type if c.isdigit()]))
        return 0 <= int(value) <= int(pow(2, length)) - 1
    return False


def sanity_check_add(mode, lease_time, infer_gw_nm, gateway, netmask, dhcp_interface):
    if mode != "PORT":
        raise Exception("Only mode PORT is supported")
    if not lease_time.isdigit():
        raise Exception("lease_time is required and must be nonnegative integer")
    if not inter_gw_nm:
        ipaddress.ip_address(gateway)
        ipaddress.ip_address(netmask)


def infer_param_for_add(dhcp_interface):
    pass


@ipv4.command(name="add")
@click.argument("dhcp_interface", required=True)
@click.option("--mode", required=True)
@click.option("--lease_time", required=False, default="900")
@click.option("--infer_gw_nm", required=False, default=False, is_flag=True)
@click.option("--gateway", required=False)
@click.option("--netmask", required=False)
@clicommon.pass_db
def dhcp_server_ipv4_add(db, mode, lease_time, infer_gw_nm, gateway, netmask, dhcp_interface):
    sanity_check_add(mode, lease_time, infer_gw_nm, gateway, netmask, dhcp_interface):
    if infer_gw_nm:
        gateway, netmask = infer_param_for_add(dhcp_interface)
    dbconn = db.db
    key = "DHCP_SERVER_IPV4|" + dhcp_interface
    if dbconn.exists("CONFIG_DB", key):
        print("Dhcp_interface %s already exist".format(dhcp_interface))
    else:
        dbconn.hmset("CONFIG_DB", key, {
            "mode": mode,
            "lease_time": lease_time,
            "gateway": gateway,
            "netmask": netmask,
            "state": "disabled",
            })


@ipv4.command(name="del")
@click.argument("dhcp_interface", required=True)
@clicommon.pass_db
def dhcp_server_ipv4_del(db, dhcp_interface):
    dbconn = db.db
    key = "DHCP_SERVER_IPV4|" + dhcp_interface
    if dbconn.exists("CONFIG_DB", key):
        print("Dhcp interface %s exists in config db, proceed to delete".format(dhcp_interface))
        dbconn.delete("CONFIG_DB", key)
    else:
        print("Dhcp interface %s does not exist in config db".format(dhcp_interface))


@ipv4.command(name="enable")
@click.argument("dhcp_interface", required=True)
@clicommon.pass_db
def dhcp_server_ipv4_enable(db, dhcp_interface):
    dbconn = db.db
    key = "DHCP_SERVER_IPV4|" + dhcp_interface
    if dbconn.exists("CONFIG_DB", key):
        dbconn.set("CONFIG_DB", key, "state", "enabled")
    else:
        print("Failed to enable, dhcp interface %s does not exist".format(dhcp_interface))


@ipv4.command(name="disable")
@click.argument("dhcp_interface", required=True)
@clicommon.pass_db
def dhcp_server_ipv4_disable(db, dhcp_interface):
    dbconn = db.db
    key = "DHCP_SERVER_IPV4|" + dhcp_interface
    if dbconn.exists("CONFIG_DB", key):
        dbconn.set("CONFIG_DB", key, "state", "disabled")
    else:
        print("Failed to disable, dhcp interface %s does not exist".format(dhcp_interface))


@ipv4.command(name="update")
@clicommon.pass_db
def dhcp_server_ipv4_update(db):
    sanity_check_add(mode, lease_time, infer_gw_nm, gateway, netmask, dhcp_interface):
    if infer_gw_nm:
        gateway, netmask = infer_param_for_add(dhcp_interface)
    dbconn = db.db
    key = "DHCP_SERVER_IPV4|" + dhcp_interface
    if dbconn.exists("CONFIG_DB", key):
        dbconn.hmset("CONFIG_DB", key, {
            "mode": mode,
            "lease_time": lease_time,
            "gateway": gateway,
            "netmask": netmask,
            "customized_options": "",
            "state": "disabled",
            })
    else:
        print("Dhcp_interface %s does not exist, cannot update".format(dhcp_interface))


@ipv4.group(cls=clicommon.AliasedGroup, name="range")
def dhcp_server_ipv4_range():
    pass


@range.command(name="add")
@click.argument("range_name", required=True)
@click.argument("ip_start", required=True)
@click.argument("ip_end", required=False)
@clicommon.pass_db
def dhcp_server_ipv4_range_add(db, range_name, ip_start, ip_end):
    if not ip_end:
        ip_end = ip_start
    ipaddress.ip_address(ip_start)
    ipaddress.ip_address(ip_end)
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_RANGE|" + range_name
    if dbconn.exists("CONFIG_DB", key):
        print("Range %s already exist".format(range_name))
    else:
        db.hmset("CONFIG_DB", key, {"ranges": ip_start + "," + ip_end})


@range.command(name="update")
@click.argument("range_name", required=True)
@click.argument("ip_start", required=True)
@click.argument("ip_end", required=False)
@clicommon.pass_db
def dhcp_server_ipv4_range_update(db, range_name, ip_start, ip_end):
    if not ip_end:
        ip_end = ip_start
    ipaddress.ip_address(ip_start)
    ipaddress.ip_address(ip_end)
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_RANGE|" + range_name
    if dbconn.exists("CONFIG_DB", key):
        db.hmset("CONFIG_DB", key, {"ranges": ip_start + "," + ip_end})
    else:
        print("Range %s does not exist, cannot update".format(range_name))


@range.command(name="del")
@click.argument("range_name", required=True)
@clicommon.pass_db
def dhcp_sever_ipv4_range_del(db, range_name)
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_RANGE|" + range_name
    if dbconn.exists("CONFIG_DB", key):
        print("Range %s exists in config db, proceed to delete".format(range_name))
        dbconn.delete("CONFIG_DB", key)
    else:
        print("Dhcp interface %s does not exist in config db".format(range_name))


@ipv4.group(cls=clicommon.AliasedGroup, name="ip")
def dhcp_server_ipv4_ip():
    pass


@ip.command(name="bind")
@click.argument("vlan_interface", required=True)
@click.argument("interface", required=True)
@click.options("--range", required=False, nargs=-1)
@click.argument("ip_list", required=False, nargs=-1)
@clicommon.pass_db
def dhcp_server_ipv4_ip_bind(db, vlan_interface, interface, range_, ip_list):
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_PORT|" + vlan_interface + "|" + interface
    if range_:
        dbconn.hmset("CONFIG_DB", key, {"ranges": ",".join(range_)})
    elif ip_list:
        dbconn.hmset("CONFIG_DB", key, {"ips": ",".join(ip_list)})
    else:
        raise Exception("One of range and ip_list should be provided")


@ip.command(name="unbind")
@click.argument("vlan_interface", required=True)
@click.argument("interface", required=True)
@click.options("--range", required=False, nargs=-1)
@click.argument("ip_list", required=False, nargs=-1)
@clicommon.pass_db
def dhcp_server_ipv4_ip_unbind(db, vlan_interface, interface, range_, ip_list):
    pass


@ipv4.group(cls=clicommon.AliasedGroup, name="option")
def dhcp_server_ipv4_option():
    pass


@option.command(name="add")
@click.argument("option_name", required=True)
@click.argument("option_id", required=True)
@click.argument("type", required=True)
@click.argument("value", required=True)
@clicommon.pass_db
def dhcp_server_ipv4_option_add(db, option_name, option_id, type_, value):
    assert option_id.isdigit()
    assert type_ == "string"
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS|" + option_name
    dbconn.hmset("CONFIG_DB", key, {
        "option_id": option_id,
        "type": type_,
        "value": value,
        })


@option.command(name="del")
@click.argument("option_name", required=True)
@clicommon.pass_db
def dhcp_server_ipv4_option_del(db, option_name):
    dbconn = db.db
    key = "DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS|" + option_name
    dbconn.delete("CONFIG_DB", key)


@option.command(name="bind")
@click.argument("dhcp_interface", required=True)
@click.argument("option_list", nargs=-1)
@clicommon.pass_db
def dhcp_server_ipv4_option_bind(db, dhcp_interface, option_list):
    dbconn = db.db
    key = "DHCP_SERVER_IPV4|" + dhcp_interface
    if dbconn.exists("CONFIG_DB", key) and option_list:
        old_value = dbconn.get("CONFIG_DB", key, "customized_options")
        if old_value:
            option_list = [old_value] + option_list
        dbconn.set("CONFIG_DB", key, "customized_options", ",".join(option_list))


@option.command(name="unbind")
@click.argument("dhcp_interface", required=True)
@click.argument("option_list", nargs=-1)
@click.option("--all", required=False, default=False, is_flag=True)
@clicommon.pass_db
def dhcp_server_ipv4_option_unbind(db, dhcp_interface, option_list, all_):
    dbconn = db.db
    key = "DHCP_SERVER_IPV4|" + dhcp_interface
    if all_:
        dbconn.set("CONFIG_DB", key, "customized_options", "")
    else:
        old_options = dbconn.get("CONFIG_DB", key, "customized_options")
        option_set = set(old_options.split(","))
        new_option_set = option_set - set(option_list)
        new_options = ",".join(new_option_set)
        dbconn.set("CONFIG_DB", key, "customized_options", new_options)


def register(cli):
    cli.add_command(dhcp_server)


if __name__ == '__main__':
    dhcp_server()
