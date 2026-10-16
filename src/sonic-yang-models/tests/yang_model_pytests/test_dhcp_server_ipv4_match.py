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
