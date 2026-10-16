import pytest


def match_binding_data(match_name="vendor", binding_name="b1"):
    return {
        "sonic-dhcp-server-ipv4:sonic-dhcp-server-ipv4": {
            "DHCP_SERVER_IPV4": {
                "DHCP_SERVER_IPV4_LIST": [{
                    "name": "Vlan100",
                    "mode": "PORT",
                    "lease_time": 900,
                    "netmask": "255.255.255.0",
                    "state": "enabled",
                }],
            },
            "DHCP_SERVER_IPV4_MATCH": {
                "DHCP_SERVER_IPV4_MATCH_LIST": [{
                    "name": match_name,
                    "type": "option60",
                    "value": " MAIA A ",
                }],
            },
            "DHCP_SERVER_IPV4_BINDING": {
                "DHCP_SERVER_IPV4_BINDING_LIST": [{
                    "name": "Vlan100",
                    "binding": binding_name,
                    "matches": [match_name],
                    "ips": ["192.0.2.20"],
                }],
            },
        },
    }


@pytest.mark.parametrize("field,error", [
    ("match", "Match condition name must not have leading or trailing whitespace"),
    ("binding", "DHCP binding name must not have leading or trailing whitespace"),
])
@pytest.mark.parametrize("whitespace", [
    pytest.param(" ", id="space"),
    pytest.param("\t", id="tab"),
    pytest.param("\r", id="CR"),
    pytest.param("\n", id="LF"),
    pytest.param("\u0085", id="NEL"),
    pytest.param("\u00a0", id="NBSP"),
    pytest.param("\u2003", id="em-space"),
    pytest.param("\u2028", id="line-separator"),
])
@pytest.mark.parametrize("leading", [True, False], ids=["leading", "trailing"])
def test_reject_surrounding_whitespace(yang_model, field, error, whitespace, leading):
    name = whitespace + "vendor" if leading else "vendor" + whitespace
    data = match_binding_data(
        match_name=name if field == "match" else "vendor",
        binding_name=name if field == "binding" else "b1",
    )
    yang_model.load_data(data, error)


@pytest.mark.parametrize("match_name,binding_name", [
    ("v", "b"),
    ("vendor a", "binding a"),
    ("vendor  a", "binding  a"),
    ("rack-1.vendor_a:x", "rack-1.binding_a:x,1"),
])
def test_preserve_valid_names_and_match_values(yang_model, match_name, binding_name):
    yang_model.load_data(match_binding_data(match_name, binding_name))


@pytest.fixture(params=[
    pytest.param(
        ("etp1", [("Ethernet0", "etp1"), ("Ethernet4", "etp1")], []),
        id="duplicate-aliases",
    ),
    pytest.param(
        ("Ethernet4", [("Ethernet0", "Ethernet4"), ("Ethernet4", None)], []),
        id="alias-versus-aliasless-name",
    ),
    pytest.param(
        ("PortChannel0001", [("Ethernet0", "PortChannel0001")], ["PortChannel0001"]),
        id="alias-versus-portchannel-name",
    ),
])
def ambiguous_circuit_data(request):
    value, ports, portchannels = request.param
    data = match_binding_data()
    condition = data["sonic-dhcp-server-ipv4:sonic-dhcp-server-ipv4"][
        "DHCP_SERVER_IPV4_MATCH"]["DHCP_SERVER_IPV4_MATCH_LIST"][0]
    condition.update(type="circuit_id", value=value)
    port_entries = []
    for index, (name, alias) in enumerate(ports):
        entry = {"name": name, "lanes": str(index), "speed": 40000}
        if alias is not None:
            entry["alias"] = alias
        port_entries.append(entry)
    data["sonic-port:sonic-port"] = {"PORT": {"PORT_LIST": port_entries}}
    if portchannels:
        data["sonic-portchannel:sonic-portchannel"] = {
            "PORTCHANNEL": {
                "PORTCHANNEL_LIST": [{"name": name, "admin_status": "up"} for name in portchannels],
            },
        }
    return data


def test_reject_ambiguous_circuit_id(yang_model, ambiguous_circuit_data):
    # A standalone match must be unambiguous, not only one used by a binding.
    del ambiguous_circuit_data["sonic-dhcp-server-ipv4:sonic-dhcp-server-ipv4"]["DHCP_SERVER_IPV4_BINDING"]
    yang_model.load_data(
        ambiguous_circuit_data, "circuit_id value must not resolve to multiple interfaces")


def test_option60_does_not_require_unique_port(yang_model, ambiguous_circuit_data):
    condition = ambiguous_circuit_data["sonic-dhcp-server-ipv4:sonic-dhcp-server-ipv4"][
        "DHCP_SERVER_IPV4_MATCH"]["DHCP_SERVER_IPV4_MATCH_LIST"][0]
    condition["type"] = "option60"
    yang_model.load_data(ambiguous_circuit_data)


def test_port_mode_without_match_allows_alias_collisions(yang_model, ambiguous_circuit_data):
    dhcp = ambiguous_circuit_data["sonic-dhcp-server-ipv4:sonic-dhcp-server-ipv4"]
    del dhcp["DHCP_SERVER_IPV4_MATCH"]
    del dhcp["DHCP_SERVER_IPV4_BINDING"]
    yang_model.load_data(ambiguous_circuit_data)


def test_circuit_id_ignores_unrelated_alias_collisions(yang_model, ambiguous_circuit_data):
    ambiguous_circuit_data["sonic-port:sonic-port"]["PORT"]["PORT_LIST"].append({
        "name": "Ethernet8", "alias": "etp8", "lanes": "8", "speed": 40000,
    })
    condition = ambiguous_circuit_data["sonic-dhcp-server-ipv4:sonic-dhcp-server-ipv4"][
        "DHCP_SERVER_IPV4_MATCH"]["DHCP_SERVER_IPV4_MATCH_LIST"][0]
    condition["value"] = "etp8"
    yang_model.load_data(ambiguous_circuit_data)


def test_circuit_id_alias_equal_to_own_port_name(yang_model):
    data = match_binding_data()
    condition = data["sonic-dhcp-server-ipv4:sonic-dhcp-server-ipv4"][
        "DHCP_SERVER_IPV4_MATCH"]["DHCP_SERVER_IPV4_MATCH_LIST"][0]
    condition.update(type="circuit_id", value="Ethernet0")
    data["sonic-port:sonic-port"] = {
        "PORT": {"PORT_LIST": [{
            "name": "Ethernet0", "alias": "Ethernet0", "lanes": "0", "speed": 40000,
        }]},
    }
    yang_model.load_data(data)
