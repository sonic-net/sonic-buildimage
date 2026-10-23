import json

import libyang as ly
import pytest


@pytest.fixture
def vnet_config():
    vnet = {
        "name": "Vnet1",
        "vxlan_tunnel_list": ["vtep1", "vtep2"],
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
    ["vtep1"], ["vtep2"], ["vtep1", "vtep2"], ["vtep2", "vtep1"],
])
def test_vxlan_tunnel_list_leafrefs(yang_model, vnet_config, tunnel_list):
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


@pytest.mark.parametrize("tunnel_list", [None, []], ids=["absent", "empty"])
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


@pytest.mark.parametrize("tunnel_list", ["", "vtep1", "vtep1,vtep2"])
@pytest.mark.parametrize("fallback", [False, True])
def test_yang_tunnel_list_requires_array(yang_model, vnet_config, tunnel_list, fallback):
    data, vnet = vnet_config
    vnet["vxlan_tunnel_list"] = tunnel_list
    if fallback:
        vnet["vxlan_tunnel"] = "vtep1"
    with pytest.raises(ly.LibyangError, match="vxlan_tunnel_list"):
        yang_model._load_data(data)


@pytest.mark.parametrize("tunnel_list", [
    ["missing"], ["vtep1", "missing"], ["missing", "vtep2"],
    ["missing1", "missing2"], ["vtep"], ["vtep1", "vtep"], [""],
])
def test_vxlan_tunnel_list_references(yang_model, vnet_config, tunnel_list):
    data, vnet = vnet_config
    vnet["vxlan_tunnel_list"] = tunnel_list
    yang_model.load_data(data, "Invalid leafref")


@pytest.mark.parametrize("tunnel_list", [["vtep1", "vtep1"], ["vtep2", "vtep2"]])
def test_vxlan_tunnel_list_duplicates(yang_model, vnet_config, tunnel_list):
    data, vnet = vnet_config
    vnet["vxlan_tunnel_list"] = tunnel_list
    yang_model.load_data(data, "Duplicat")


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
    vnet["vxlan_tunnel_list"] = ["vtep10"]
    yang_model.load_data(data)
    vnet["vxlan_tunnel_list"] = ["vtep1"]
    yang_model.load_data(data)
    vnet["vxlan_tunnel_list"] = ["vtep1", "vtep10"]
    yang_model.load_data(
        data, "vxlan_tunnel_list must not contain more than one {} tunnel.".format(family))


@pytest.mark.parametrize("tunnel_list", [["vtep1"], ["vtep2"], ["vtep1", "vtep2"]])
def test_vxlan_tunnel_list_with_fallback(yang_model, vnet_config, tunnel_list):
    data, vnet = vnet_config
    vnet["vxlan_tunnel"] = "vtep1"
    vnet["vxlan_tunnel_list"] = tunnel_list
    yang_model.load_data(data)


@pytest.mark.parametrize("relax_list_limit", [False, True], ids=["limit-enforced", "limit-relaxed"])
def test_vxlan_tunnel_list_max_elements(yang_model, vnet_config, relax_list_limit):
    # Isolate this limit from the global tunnel limit and address-family constraints.
    deviations = """
        deviation "/svxlan:sonic-vxlan/svxlan:VXLAN_TUNNEL/svxlan:VXLAN_TUNNEL_LIST" {
            deviate replace {
                max-elements 3;
            }
        }
        deviation "/svnet:sonic-vnet/svnet:VNET/svnet:VNET_LIST" {
            deviate delete {
                must "count(/svxlan:sonic-vxlan/svxlan:VXLAN_TUNNEL/svxlan:VXLAN_TUNNEL_LIST" +
                     "[svxlan:name = current()/vxlan_tunnel_list]" +
                     "[not(contains(svxlan:src_ip, ':'))]) <= 1";
                must "count(/svxlan:sonic-vxlan/svxlan:VXLAN_TUNNEL/svxlan:VXLAN_TUNNEL_LIST" +
                     "[svxlan:name = current()/vxlan_tunnel_list]" +
                     "[contains(svxlan:src_ip, ':')]) <= 1";
            }
        }
    """
    if relax_list_limit:
        deviations += """
            deviation "/svnet:sonic-vnet/svnet:VNET/svnet:VNET_LIST/svnet:vxlan_tunnel_list" {
                deviate replace {
                    max-elements 3;
                }
            }
        """

    yang_model.ctx.parse_module_str("""
        module vnet-cardinality-test {
            yang-version 1.1;
            namespace "urn:sonic:vnet-cardinality-test";
            prefix vct;
            import sonic-vnet {
                prefix svnet;
            }
            import sonic-vxlan {
                prefix svxlan;
            }
    """ + deviations + "}", "yang")

    data, vnet = vnet_config
    data["sonic-vxlan:sonic-vxlan"]["VXLAN_TUNNEL"]["VXLAN_TUNNEL_LIST"].append(
        {"name": "vtep3", "src_ip": "192.0.2.2"})
    yang_model.load_data(data)
    vnet["vxlan_tunnel_list"].append("vtep3")
    if relax_list_limit:
        yang_model.load_data(data)
    else:
        with pytest.raises(ly.LibyangError, match="Too many") as exc_info:
            yang_model._load_data(data)
        assert "vxlan_tunnel_list" in str(exc_info.value)
