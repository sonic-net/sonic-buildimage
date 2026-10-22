import json

import libyang as ly
import pytest


@pytest.fixture
def vnet_config():
    vnet = {
        "name": "Vnet1",
        "vxlan_tunnel_list": "vtep1,vtep2",
        "vni": "10011",
    }
    data = {
        "sonic-vxlan:sonic-vxlan": {
            "VXLAN_TUNNEL": {"VXLAN_TUNNEL_LIST": [
                {"name": "vtep1", "src_ip": "192.0.2.1"},
                {"name": "vtep2", "src_ip": "2001:db8::1"},
            ]},
        },
        "sonic-vnet:sonic-vnet": {
            "VNET": {"VNET_LIST": [vnet]},
        },
    }
    return data, vnet


@pytest.mark.parametrize("tunnel_list", [
    "vtep1", "vtep2", "vtep1,vtep2", "vtep2,vtep1",
])
def test_vxlan_tunnel_list_string(yang_model, vnet_config, tunnel_list):
    data, vnet = vnet_config
    vnet["vxlan_tunnel_list"] = tunnel_list
    node = yang_model.ctx.parse_data_mem(
        json.dumps(data), "json", strict=True, no_state=True,
        json_string_datatypes=True)
    try:
        result = json.loads(node.print_mem("json", with_siblings=True))
        entry = result["sonic-vnet:sonic-vnet"]["VNET"]["VNET_LIST"][0]
        assert entry["vxlan_tunnel_list"] == tunnel_list
    finally:
        node.free()


@pytest.mark.parametrize("tunnel_list", [None, ""], ids=["absent", "empty"])
@pytest.mark.parametrize("fallback", [False, True])
def test_empty_vxlan_tunnel_list(yang_model, vnet_config, tunnel_list, fallback):
    data, vnet = vnet_config
    if tunnel_list is None:
        del vnet["vxlan_tunnel_list"]
    else:
        vnet["vxlan_tunnel_list"] = tunnel_list
    if fallback:
        vnet["vxlan_tunnel"] = "vtep1"
    error = None if fallback else (
        "At least one of vxlan_tunnel or vxlan_tunnel_list must be configured.")
    yang_model.load_data(data, error)


@pytest.mark.parametrize("tunnel_list", [
    ",", ",vtep1", "vtep1,", "vtep1,,vtep2",
    "vtep1,vtep2,vtep3", "vtep1,vtep2,vtep1",
])
def test_vxlan_tunnel_list_format(yang_model, vnet_config, tunnel_list):
    data, vnet = vnet_config
    vnet["vxlan_tunnel_list"] = tunnel_list
    yang_model.load_data(
        data, "vxlan_tunnel_list must be empty or contain one or two comma-separated tunnel names.")


@pytest.mark.parametrize("tunnel_list", [[], ["vtep1"], ["vtep1", "vtep2"]])
@pytest.mark.parametrize("fallback", [False, True])
def test_vxlan_tunnel_list_rejects_arrays(yang_model, vnet_config, tunnel_list, fallback):
    data, vnet = vnet_config
    vnet["vxlan_tunnel_list"] = tunnel_list
    if fallback:
        vnet["vxlan_tunnel"] = "vtep1"
    with pytest.raises(ly.LibyangError, match="vxlan_tunnel_list"):
        yang_model._load_data(data)


@pytest.mark.parametrize("tunnel_list", [
    "missing", "vtep1,missing", "missing,vtep2", "missing1,missing2",
    "vtep", "vtep1,vtep", "vtep1, vtep2", "vtep1, ",
])
def test_vxlan_tunnel_list_references(yang_model, vnet_config, tunnel_list):
    data, vnet = vnet_config
    vnet["vxlan_tunnel_list"] = tunnel_list
    yang_model.load_data(
        data, "vxlan_tunnel_list must reference existing VXLAN tunnels.")


@pytest.mark.parametrize("tunnel_list", ["vtep1,vtep1", "vtep2,vtep2"])
def test_vxlan_tunnel_list_duplicates(yang_model, vnet_config, tunnel_list):
    data, vnet = vnet_config
    vnet["vxlan_tunnel_list"] = tunnel_list
    yang_model.load_data(
        data, "vxlan_tunnel_list must not contain duplicate tunnels.")


@pytest.mark.parametrize("family,addresses", [
    ("IPv4", ["192.0.2.1", "192.0.2.2"]),
    ("IPv6", ["2001:db8::1", "2001:db8::2"]),
])
def test_vxlan_tunnel_list_address_family(yang_model, vnet_config, family, addresses):
    data, vnet = vnet_config
    tunnels = data["sonic-vxlan:sonic-vxlan"]["VXLAN_TUNNEL"]["VXLAN_TUNNEL_LIST"]
    for tunnel, address in zip(tunnels, addresses):
        tunnel["src_ip"] = address
    tunnels[1]["name"] = "vtep10"

    # A selected name must not also match another tunnel's name prefix.
    vnet["vxlan_tunnel_list"] = "vtep10"
    yang_model.load_data(data)
    vnet["vxlan_tunnel_list"] = "vtep1"
    yang_model.load_data(data)
    vnet["vxlan_tunnel_list"] = "vtep1,vtep10"
    yang_model.load_data(
        data, "vxlan_tunnel_list must not contain more than one {} tunnel.".format(family))


@pytest.mark.parametrize("tunnel_list", ["vtep1", "vtep2", "vtep1,vtep2"])
def test_vxlan_tunnel_list_with_fallback(yang_model, vnet_config, tunnel_list):
    data, vnet = vnet_config
    vnet["vxlan_tunnel"] = "vtep1"
    vnet["vxlan_tunnel_list"] = tunnel_list
    yang_model.load_data(data)
