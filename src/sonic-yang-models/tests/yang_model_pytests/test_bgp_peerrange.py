import pytest


def peer_range_config(qualified, peer_name="BGPSLBPassive", name="BGPSLBPassive",
                      routing_name="VnetA", routing_type="vnet", include_name=True):
    entry = {"peer_range_name": peer_name, "peer_asn": "65200",
             "src_address": "10.1.0.1", "ip_range": ["10.1.0.0/24"]}
    if include_name:
        entry["name"] = name
    list_name = "BGP_PEER_RANGE_TEMPLATE_LIST"
    data = {}
    if qualified:
        list_name = "BGP_PEER_RANGE_LIST"
        entry["vrf_name"] = routing_name
        if routing_type == "vrf":
            data["sonic-vrf:sonic-vrf"] = {
                "VRF": {"VRF_LIST": [{"name": routing_name}]}}
        else:
            data["sonic-vxlan:sonic-vxlan"] = {
                "VXLAN_TUNNEL": {"VXLAN_TUNNEL_LIST": [
                    {"name": "tunnel1", "src_ip": "10.1.0.1"}]}}
            data["sonic-vnet:sonic-vnet"] = {
                "VNET": {"VNET_LIST": [
                    {"name": routing_name, "vxlan_tunnel": "tunnel1", "vni": "1000"}]}}
    data["sonic-bgp-peerrange:sonic-bgp-peerrange"] = {
        "BGP_PEER_RANGE": {list_name: [entry]}}
    return data


@pytest.mark.parametrize("qualified", [False, True], ids=["template", "qualified"])
@pytest.mark.parametrize("newline", ["\n", "\r", "\r\n"], ids=["LF", "CR", "CRLF"])
@pytest.mark.parametrize("suffix", ["", "SecondLine"], ids=["trailing", "embedded"])
@pytest.mark.parametrize("field", ["peer_range_name", "name", "both"])
def test_reject_line_breaks(yang_model, qualified, newline, suffix, field):
    invalid = "BGPSLBPassive" + newline + suffix
    peer_name = invalid if field != "name" else "BGPSLBPassive"
    name = invalid if field != "peer_range_name" else "BGPSLBPassive"
    yang_model.load_data(
        peer_range_config(qualified, peer_name, name),
        "BGP peer-range name must not contain line breaks")


@pytest.mark.parametrize("newline", ["\n", "\r", "\r\n"], ids=["LF", "CR", "CRLF"])
@pytest.mark.parametrize("suffix", ["", "SecondLine"], ids=["trailing", "embedded"])
def test_reject_line_breaks_in_referenced_vnet(yang_model, newline, suffix):
    # Define the referenced VNET too, so rejection cannot be a missing leafref.
    yang_model.load_data(
        peer_range_config(True, routing_name="VnetA" + newline + suffix),
        "BGP peer-range VRF or VNET name must not contain line breaks")


@pytest.mark.parametrize("qualified", [False, True], ids=["template", "qualified"])
@pytest.mark.parametrize("name", ["BGPSLBPassive", "Peer-Range_1", "", "Peer Range"])
@pytest.mark.parametrize("include_name", [False, True], ids=["missing-name", "with-name"])
def test_single_line_names_unchanged(yang_model, qualified, name, include_name):
    yang_model.load_data(peer_range_config(
        qualified, name, name, include_name=include_name))


@pytest.mark.parametrize("routing_name,routing_type", [
    ("Vrf-blue_1", "vrf"), ("VnetA", "vnet"), ("Vnet-" + "x" * 250, "vnet")])
def test_valid_qualified_reference(yang_model, routing_name, routing_type):
    yang_model.load_data(peer_range_config(
        True, routing_name=routing_name, routing_type=routing_type))


@pytest.mark.parametrize("qualified", [False, True], ids=["template", "qualified"])
def test_name_must_still_match_key(yang_model, qualified):
    yang_model.load_data(
        peer_range_config(qualified, name="OtherPeer"),
        "Invalid name, name must match peer_range_name")


@pytest.mark.parametrize("routing_type", ["vrf", "vnet"])
def test_reference_must_still_exist(yang_model, routing_type):
    name = "Vrf-blue_1" if routing_type == "vrf" else "VnetA"
    data = peer_range_config(True, routing_name=name, routing_type=routing_type)
    del data["sonic-{}:sonic-{}".format(routing_type, routing_type)]
    yang_model.load_data(data, "Invalid union value")
