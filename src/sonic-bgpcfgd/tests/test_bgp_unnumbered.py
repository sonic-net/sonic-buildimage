"""
Unit tests for BGP unnumbered (interface-based) neighbor support.
Tests template rendering and is_interface_neighbor detection logic.
"""
import os
import sys
import json
from unittest.mock import MagicMock

# Mock SONiC-specific modules for standalone testing
sys.modules.setdefault('swsscommon', MagicMock())
sys.modules.setdefault('sonic_py_common', MagicMock())
sys.modules.setdefault('sonic_py_common.device_info', MagicMock())

from bgpcfgd.template import TemplateFabric
from bgpcfgd.config import ConfigMgr
from bgpcfgd.managers_bgp import is_interface_neighbor

TEMPLATE_PATH = os.path.abspath('../../dockers/docker-fpm-frr/frr')


# --- is_interface_neighbor tests ---

def test_is_interface_neighbor_portchannel():
    assert is_interface_neighbor('PortChannel101') is True

def test_is_interface_neighbor_ethernet():
    assert is_interface_neighbor('Ethernet0') is True

def test_is_interface_neighbor_vlan():
    assert is_interface_neighbor('Vlan100') is True

def test_is_interface_neighbor_ipv4():
    assert is_interface_neighbor('10.10.10.1') is False

def test_is_interface_neighbor_ipv6():
    assert is_interface_neighbor('fc00::1') is False

def test_is_interface_neighbor_dynamic_name():
    assert is_interface_neighbor('BGPSLBPassive') is False

def test_is_interface_neighbor_portchannel_high():
    assert is_interface_neighbor('PortChannel4096') is True

def test_is_interface_neighbor_ethernet_typical():
    assert is_interface_neighbor('Ethernet48') is True

def test_is_interface_neighbor_vlan_high():
    assert is_interface_neighbor('Vlan1000') is True

def test_is_interface_neighbor_empty_string():
    assert is_interface_neighbor('') is False

def test_is_interface_neighbor_none():
    assert is_interface_neighbor(None) is False

def test_is_interface_neighbor_trailing_junk():
    assert is_interface_neighbor('Ethernet0foo') is False


# --- Template rendering tests ---

def render_general_instance(neighbor_addr, bgp_session, bgp_asn='65100'):
    tf = TemplateFabric(TEMPLATE_PATH)
    template = tf.from_file('bgpd/templates/general/instance.conf.j2')
    return template.render(
        CONFIG_DB__DEVICE_METADATA={'localhost': {}},
        neighbor_addr=neighbor_addr,
        bgp_session=bgp_session,
        bgp_asn=bgp_asn,
        constants={'deployment_id_asn_map': {'5': '51111'}}
    )


def test_unnumbered_dual_stack_rendering():
    """Interface neighbor renders with PEER_UNNUMBERED and both address families."""
    result = render_general_instance('PortChannel101', {'asn': '65200', 'name': 'spine1'})
    assert 'neighbor PortChannel101 interface peer-group PEER_UNNUMBERED' in result
    assert 'neighbor PortChannel101 activate' in result


def test_unnumbered_v6only_rendering():
    """Interface neighbor with v6only=true renders only IPv6 address family."""
    result = render_general_instance('Ethernet0', {'asn': '65200', 'name': 'spine2', 'v6only': 'true'})
    assert 'neighbor Ethernet0 interface peer-group PEER_UNNUMBERED' in result
    # IPv6 should be present
    assert 'address-family ipv6' in result
    assert 'neighbor Ethernet0 activate' in result
    # IPv4 should NOT be present
    assert 'address-family ipv4' not in result


def test_ipv4_neighbor_unchanged():
    """IPv4 neighbor still renders with PEER_V4 (no regression)."""
    result = render_general_instance('10.10.10.10', {'asn': '555', 'name': 'remote_peer'})
    assert 'neighbor 10.10.10.10 peer-group PEER_V4' in result
    assert 'PEER_UNNUMBERED' not in result


def test_ipv6_neighbor_unchanged():
    """IPv6 neighbor still renders with PEER_V6 (no regression)."""
    result = render_general_instance('fc00::1', {'asn': '555', 'name': 'remote_peer'})
    assert 'neighbor fc00::1 peer-group PEER_V6' in result
    assert 'PEER_UNNUMBERED' not in result


def test_peer_group_has_unnumbered():
    """Peer-group template defines PEER_UNNUMBERED with extended-nexthop."""
    tf = TemplateFabric(TEMPLATE_PATH)
    template = tf.from_file('bgpd/templates/general/peer-group.conf.j2')
    result = template.render(
        CONFIG_DB__DEVICE_METADATA={'localhost': {'type': 'LeafRouter'}},
        CONFIG_DB__BGP_BBR={'status': 'disabled'}
    )
    assert 'neighbor PEER_UNNUMBERED peer-group' in result
    assert 'neighbor PEER_UNNUMBERED capability extended-nexthop' in result


def test_unnumbered_remote_as():
    """Interface neighbor renders remote-as correctly."""
    result = render_general_instance('PortChannel101', {'asn': '65200', 'name': 'spine1'})
    assert 'neighbor PortChannel101 remote-as 65200' in result


def test_unnumbered_shutdown():
    """Interface neighbor renders shutdown when admin_status is down."""
    result = render_general_instance('PortChannel101', {'asn': '65200', 'name': 'spine1', 'admin_status': 'down'})
    assert 'neighbor PortChannel101 shutdown' in result
