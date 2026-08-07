"""
Tests for the general/router.j2 dispatcher: verifies that the thin wrappers
in dockers/docker-fpm-frr/frr/bgpd/templates/general/{instance,peer-group,
policies}.conf.j2 route to numbered/* sub-templates when neighbor_addr is an
IP address, and to unnumbered/* sub-templates when neighbor_addr is an
interface name (non-IP string).

These tests exercise the templates directly via TemplateFabric, decoupled
from any bgpcfgd Manager state, so they cleanly validate the routing logic.
"""
import os

import pytest

from bgpcfgd.template import TemplateFabric

TEMPLATE_PATH = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "..", "..", "..",
                 "dockers", "docker-fpm-frr", "frr")
)


@pytest.fixture(scope="module")
def tf():
    return TemplateFabric(TEMPLATE_PATH)


def _common_kwargs(neighbor_addr):
    """Minimal kwargs that satisfy both numbered and unnumbered templates."""
    return {
        "neighbor_addr": neighbor_addr,
        "bgp_asn": 65100,
        "vrf": "default",
        "bgp_session": {
            "asn": 64600,
            "name": "ARISTA01T1",
            "admin_status": "up",
            "local_addr": "10.0.0.0",
            "holdtime": "180",
            "keepalive": "60",
        },
        "constants": {"bgp": {}},
        "CONFIG_DB__DEVICE_METADATA": {
            "localhost": {
                "type": "ToRRouter",
                "subtype": "",
                "default_bgp_status": "up",
                "bgp_asn": "65100",
            }
        },
        "CONFIG_DB__BGP_BBR": {"status": "disabled"},
        "CONFIG_DB__LOOPBACK_INTERFACE": {},
        "CONFIG_DB__DEVICE_NEIGHBOR_METADATA": {},
        "loopback0_ipv4": "10.1.0.32",
    }


# ---------------------------------------------------------------------------
# IP neighbor -> dispatcher must route to general/numbered/*.conf.j2
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("ip", ["10.0.0.1", "fc00::1"])
def test_dispatcher_instance_ip_routes_to_numbered(tf, ip):
    tmpl = tf.from_file("bgpd/templates/general/instance.conf.j2")
    out = tmpl.render(**_common_kwargs(ip))
    # Numbered template renders `peer-group PEER_V4`/`PEER_V6` and uses the
    # numbered marker comment.
    assert "bgpd/templates/general/numbered/instance.conf.j2" not in out, \
        "Marker comment is in the sub-template; not expected to leak"
    assert ("PEER_V4" in out) or ("PEER_V6" in out), \
        "Numbered instance template should set peer-group PEER_V4/PEER_V6"
    # And must not have the unnumbered-only artifact
    assert "PEER_UNNUMBERED" not in out
    assert "interface peer-group" not in out


@pytest.mark.parametrize("ip", ["10.0.0.1", "fc00::1"])
def test_dispatcher_peer_group_ip_routes_to_numbered(tf, ip):
    tmpl = tf.from_file("bgpd/templates/general/peer-group.conf.j2")
    out = tmpl.render(**_common_kwargs(ip))
    assert "neighbor PEER_V4 peer-group" in out
    assert "neighbor PEER_V6 peer-group" in out
    assert "PEER_UNNUMBERED" not in out


@pytest.mark.parametrize("ip", ["10.0.0.1", "fc00::1"])
def test_dispatcher_policies_ip_routes_to_numbered(tf, ip):
    tmpl = tf.from_file("bgpd/templates/general/policies.conf.j2")
    out = tmpl.render(**_common_kwargs(ip))
    # Numbered policies sets the default IPv4/IPv6 prefix-lists.
    assert "ip prefix-list DEFAULT_IPV4 permit 0.0.0.0/0" in out


# ---------------------------------------------------------------------------
# Interface-name neighbor -> dispatcher must route to general/unnumbered/*.conf.j2
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("iface", ["PortChannel101", "Ethernet0", "Po1031.101"])
def test_dispatcher_instance_iface_routes_to_unnumbered(tf, iface):
    tmpl = tf.from_file("bgpd/templates/general/instance.conf.j2")
    out = tmpl.render(**_common_kwargs(iface))
    # Unnumbered template uses `neighbor <iface> interface peer-group PEER_UNNUMBERED`
    assert ("neighbor %s interface peer-group PEER_UNNUMBERED" % iface) in out, \
        "Unnumbered instance template should bind iface to PEER_UNNUMBERED"
    # Numbered-only artifact must not appear
    assert "peer-group PEER_V4" not in out
    assert "peer-group PEER_V6" not in out


@pytest.mark.parametrize("iface", ["PortChannel101", "Po1031.101"])
def test_dispatcher_peer_group_iface_routes_to_unnumbered(tf, iface):
    tmpl = tf.from_file("bgpd/templates/general/peer-group.conf.j2")
    out = tmpl.render(**_common_kwargs(iface))
    assert "neighbor PEER_UNNUMBERED peer-group" in out
    assert "PEER_V4 peer-group" not in out


@pytest.mark.parametrize("iface", ["PortChannel101", "Po1031.101"])
def test_dispatcher_policies_iface_routes_to_unnumbered(tf, iface):
    tmpl = tf.from_file("bgpd/templates/general/policies.conf.j2")
    out = tmpl.render(**_common_kwargs(iface))
    # Numbered DEFAULT_IPV4/V6 prefix-list should NOT appear in unnumbered policies.
    assert "ip prefix-list DEFAULT_IPV4 permit 0.0.0.0/0" not in out


# ---------------------------------------------------------------------------
# router.j2 macro itself: import and call directly
# ---------------------------------------------------------------------------

def test_router_macro_directly_dispatches(tf):
    """Render an inline template that imports the macro and calls it -
    proves the macro contract works regardless of the wrapper *.conf.j2."""
    src = (
        '{% from "bgpd/templates/general/router.j2" import general_routing '
        'with context %}{{ general_routing("instance") }}'
    )
    t = tf.env.from_string(src)
    # IP neighbor -> numbered branch
    out_ip = t.render(**_common_kwargs("10.0.0.1"))
    assert "PEER_V4" in out_ip or "PEER_V6" in out_ip
    # Interface neighbor -> unnumbered branch
    out_if = t.render(**_common_kwargs("PortChannel101"))
    assert "PEER_UNNUMBERED" in out_if
