from unittest.mock import MagicMock, patch

import os
from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd import manager as manager_mod
from . import swsscommon_test
from .util import load_constants, render_constants
from swsscommon import swsscommon
import bgpcfgd.managers_bgp

TEMPLATE_PATH = os.path.abspath('../../dockers/docker-fpm-frr/frr')

def load_constant_files():
    # Production constants come from the shared build template
    # (files/build_templates/constants.yml.j2), rendered to a temp file, plus
    # the extra test-only constants fixtures under tests/data/constants.
    constant_files = [render_constants()]
    path = "tests/data/constants"
    constant_files += [os.path.abspath(os.path.join(path, name)) for name in os.listdir(path)
               if os.path.isfile(os.path.join(path, name)) and name.startswith("constants")]

    return constant_files


def constructor(constants_path, bgp_router_id="", peer_type="general", with_lo0_ipv4=True, with_lo4096_ipv4=False, vrf=None):
    cfg_mgr = MagicMock()
    constants = load_constants(constants_path)['constants']
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(TEMPLATE_PATH),
        'constants': constants
    }

    return_value_map = {
        "['vtysh', '-H', '/dev/null', '-c', 'show bgp vrfs json']": (0, "{\"vrfs\": {\"default\": {}}}", ""),
        "['vtysh', '-c', 'show bgp vrf default neighbors json']": (0, "{\"10.10.10.1\": {}, \"20.20.20.1\": {}, \"fc00:10::1\": {}, \"DynNbr1\": {}, \"DynNbr2\": {}}", ""),
        "['vtysh', '-c', 'show bgp peer-group DynNbr1 json']": (0, "{\"DynNbr1\":{\"dynamicRanges\":{\"IPv4\":{\"count\":1,\"ranges\":[\"10.255.0.0/24\"]}}}}", ""),
        "['vtysh', '-c', 'show bgp peer-group DynNbr2 json']": (0, "{\"DynNbr2\":{\"dynamicRanges\":{\"IPv4\":{\"count\":1,\"ranges\":[\"192.168.0.0/24\",\"192.168.1.0/24\"]}}}}", "")
    }

    bgpcfgd.managers_bgp.run_command = lambda cmd: return_value_map[str(cmd)]
    m = bgpcfgd.managers_bgp.BGPPeerMgrBase(common_objs, "CONFIG_DB", swsscommon.CFG_BGP_NEIGHBOR_TABLE_NAME, peer_type, True)
    assert m.peer_type == peer_type
    assert m.check_neig_meta == ('bgp' in constants and 'use_neighbors_meta' in constants['bgp'] and constants['bgp']['use_neighbors_meta'])

    localhost_obj = {"bgp_asn": "65100"}
    if len(bgp_router_id) != 0:
        localhost_obj["bgp_router_id"] = bgp_router_id
    m.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost", localhost_obj)
    if with_lo4096_ipv4:
        m.directory.put("CONFIG_DB", swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME, "Loopback4096|11.11.11.11/32", {})
    if with_lo0_ipv4:
        m.directory.put("CONFIG_DB", swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME, "Loopback0|11.11.11.11/32", {})
    m.directory.put("CONFIG_DB", swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME, "Loopback0|FC00:1::32/128", {})
    # For VRF-aware tests, populate the appropriate VRF binding attribute
    intf_meta_v4 = {"admin_status": "up"}
    intf_meta_v6 = {"admin_status": "up"}
    if vrf:
        field = "vnet_name" if vrf.startswith("Vnet") else "vrf_name"
        intf_meta_v4[field] = vrf
        intf_meta_v6[field] = vrf
    m.directory.put("LOCAL", "local_addresses", "Ethernet4|30.30.30.30", {"interface": "Ethernet4", "prefixlen": "24"})
    m.directory.put("LOCAL", "local_addresses", "Ethernet8|fc00:20::20", {"interface": "Ethernet8", "prefixlen": "96"})
    m.directory.put("LOCAL", "interfaces", "Ethernet4", intf_meta_v4)
    m.directory.put("LOCAL", "interfaces", "Ethernet8", intf_meta_v6)
    m.directory.put("CONFIG_DB", swsscommon.CFG_BGP_NEIGHBOR_TABLE_NAME, "default|10.10.10.1", {"ip_range": None})

    if m.check_neig_meta:
        m.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_NEIGHBOR_METADATA_TABLE_NAME, "TOR", {})

    return m


def satisfy_non_port_dependencies(manager):
    """Populate the manager's metadata, loopback, global, and LOCAL dependencies."""
    values = {
        "localhost/type": "LeafRouter",
        "localhost/deployment_id": "0",
        "Loopback0": {},
        "Loopback4096": {},
        "tsa_enabled": "false",
        "idf_isolation_state": "unisolated",
    }
    for db_name, table_name, path in manager.deps:
        if table_name == swsscommon.CFG_PORT_TABLE_NAME or manager.directory.path_exist(db_name, table_name, path):
            continue
        if path:
            path_parts = path.split("/")
            root = path_parts.pop(0)
            root_value = dict(manager.directory.get_slot(db_name, table_name).get(root, {}))
            destination = root_value
            for part in path_parts[:-1]:
                destination = destination.setdefault(part, {})
            if path_parts:
                destination[path_parts[-1]] = values.get(path, {})
            else:
                root_value = values.get(path, {})
            manager.directory.put(db_name, table_name, root, root_value)
        else:
            manager.directory.put(db_name, table_name, "__ready__", {})


def deferred_test_manager():
    """Build a minimal manager for verifying the shared pending-event contract."""
    common_objs = {
        'directory': Directory(),
        'cfg_mgr': MagicMock(),
        'constants': {},
    }
    manager = manager_mod.Manager(
        common_objs, [], "CONFIG_DB", "TEST", wait_for_all_deps=False
    )
    del_handler = MagicMock()
    manager.set_handler = MagicMock(return_value=False)
    manager.del_handler = del_handler
    return manager, del_handler


def test_manager_latest_pending_set_replaces_same_key_only():
    manager, _ = deferred_test_manager()
    manager.handler("peer-a", manager_mod.swsscommon.SET_COMMAND, {'asn': '65100'})
    manager.handler("peer-b", manager_mod.swsscommon.SET_COMMAND, {'asn': '65200'})
    manager.handler("peer-a", manager_mod.swsscommon.SET_COMMAND, {'asn': '65300'})

    assert manager.set_queue == [
        ("peer-b", {'asn': '65200'}),
        ("peer-a", {'asn': '65300'}),
    ]


def test_manager_delete_cancels_only_matching_pending_set():
    manager, del_handler = deferred_test_manager()
    manager.handler("peer-a", manager_mod.swsscommon.SET_COMMAND, {'asn': '65100'})
    manager.handler("peer-b", manager_mod.swsscommon.SET_COMMAND, {'asn': '65200'})

    manager.handler("peer-a", manager_mod.swsscommon.DEL_COMMAND, {})

    assert manager.set_queue == [("peer-b", {'asn': '65200'})]
    del_handler.assert_called_once_with("peer-a")


@patch('bgpcfgd.managers_bgp.log_info')
def test_update_peer_up(mocked_log_info):
    for constant in load_constant_files():
        m = constructor(constant)
        res = m.set_handler("10.10.10.1", {"admin_status": "up"})
        assert res, "Expect True return value for peer update"
        mocked_log_info.assert_called_with("Peer 'default|10.10.10.1' admin state is set to 'up'")

@patch('bgpcfgd.managers_bgp.log_info')
def test_update_peer_up_ipv6(mocked_log_info):
    for constant in load_constant_files():
        m = constructor(constant)
        res = m.set_handler("fc00:10::1", {"admin_status": "up"})
        assert res, "Expect True return value for peer update"
        mocked_log_info.assert_called_with("Peer 'default|fc00:10::1' admin state is set to 'up'")

@patch('bgpcfgd.managers_bgp.log_info')
def test_update_peer_down(mocked_log_info):
    for constant in load_constant_files():
        m = constructor(constant)
        res = m.set_handler("10.10.10.1", {"admin_status": "down"})
        assert res, "Expect True return value for peer update"
        mocked_log_info.assert_called_with("Peer 'default|10.10.10.1' admin state is set to 'down'")

@patch('bgpcfgd.managers_bgp.log_err')
def test_update_peer_no_admin_status(mocked_log_err):
    for constant in load_constant_files():
        m = constructor(constant)
        res = m.set_handler("10.10.10.1", {"anything": "anything"})
        assert res, "Expect True return value for peer update"
        mocked_log_err.assert_called_with("Peer '(default|10.10.10.1)': Can't update the peer. Only 'admin_status' attribute is supported")

@patch('bgpcfgd.managers_bgp.log_err')
def test_update_peer_invalid_admin_status(mocked_log_err):
    for constant in load_constant_files():
        m = constructor(constant)
        res = m.set_handler("10.10.10.1", {"admin_status": "invalid"})
        assert res, "Expect True return value for peer update"
        mocked_log_err.assert_called_with("Peer 'default|10.10.10.1': Can't update the peer. It has wrong attribute value attr['admin_status'] = 'invalid'")

def test_add_peer():
    for constant in load_constant_files():
        m = constructor(constant)
        res = m.set_handler("30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert res, "Expect True return value"

def test_add_peer_internal():
    for constant in load_constant_files():
        m = constructor(constant, peer_type="internal", with_lo4096_ipv4=True)
        res = m.set_handler("30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert res, "Expect True return value"

@patch('bgpcfgd.managers_bgp.log_info')
def test_add_peer_internal_no_router_id_no_lo4096(mocked_log_info):
    for constant in load_constant_files():
        m = constructor(constant, peer_type="internal")
        res = m.set_handler("30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert not res, "Expect False return value"
        mocked_log_info.assert_called_with("Additional loopbacks acquired for peer internal, loopback list ['Loopback0', 'Loopback4096']")

def test_add_peer_internal_router_id():
    for constant in load_constant_files():
        m = constructor(constant,  bgp_router_id="8.8.8.8", peer_type="internal", with_lo4096_ipv4=True)
        res = m.set_handler("30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert res, "Expect True return value"

def test_add_peer_internal_router_id_no_lo4096():
    for constant in load_constant_files():
        m = constructor(constant, bgp_router_id="8.8.8.8", peer_type="internal")

def test_add_peer_router_id():
    for constant in load_constant_files():
        m = constructor(constant, bgp_router_id="8.8.8.8")
        res = m.set_handler("30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert res, "Expect True return value"

@patch('bgpcfgd.managers_bgp.log_info')
@patch('bgpcfgd.managers_bgp.log_warn')
def test_add_peer_without_lo_ipv4(mocked_log_warn, mocked_log_info):
    for constant in load_constant_files():
        m = constructor(constant, with_lo0_ipv4=False)
        res = m.set_handler("30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert not res, "Expect False return value"
        mocked_log_info.assert_called_with("No additional loopbacks acquired for peer general, loopback list ['Loopback0']")
        mocked_log_warn.assert_called_with("Loopback0 ipv4 address is not presented yet and bgp_router_id not configured")

def test_add_peer_without_lo_ipv4_router_id():
    for constant in load_constant_files():
        m = constructor(constant, bgp_router_id="8.8.8.8", with_lo0_ipv4=False)
        res = m.set_handler("30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert res, "Expect True return value"

def test_add_peer_ipv6():
    for constant in load_constant_files():
        m = constructor(constant)
        res = m.set_handler("fc00:20::1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': 'fc00:20::20', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert res, "Expect True return value"

def test_add_peer_in_vnet():
    for constant in load_constant_files():
        m = constructor(constant, vrf="Vnet-10")
        res = m.set_handler("Vnet-10|30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert res, "Expect True return value"

def test_add_peer_ipv6_in_vnet():
    for constant in load_constant_files():
        m = constructor(constant, vrf="Vnet-10")
        res = m.set_handler("Vnet-10|fc00:20::1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': 'fc00:20::20', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert res, "Expect True return value"

@patch('bgpcfgd.managers_bgp.log_debug')
def test_add_peer_vrf_mismatch(mocked_log_debug):
    """Test that a peer in Vrf_0003 cannot pass dependency check using an address only present in Vrf_0002"""
    for constant in load_constant_files():
        m = constructor(constant, vrf="Vrf_0002")
        # Peer is in Vrf_0003 but the local address 30.30.30.30 only exists on Ethernet4 in Vrf_0002
        res = m.set_handler("Vrf_0003|30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert not res, "Expect False: VRF mismatch should block peer addition"

@patch('bgpcfgd.managers_bgp.log_debug')
def test_add_peer_default_vrf_rejects_vrf_bound_interface(mocked_log_debug):
    """Test that a default VRF peer cannot match an interface bound to a non-default VRF"""
    for constant in load_constant_files():
        m = constructor(constant, vrf="Vrf_0002")
        # Peer is in default VRF but local address 30.30.30.30 is on Ethernet4 in Vrf_0002
        res = m.set_handler("30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert not res, "Expect False: default VRF peer should not match VRF-bound interface"

def test_overlapping_ip_different_vrfs():
    """Test that the same IP on two interfaces in different VRFs matches the correct one"""
    for constant in load_constant_files():
        m = constructor(constant, vrf="Vrf_0002")
        # Add a second interface with the SAME IP but in Vrf_0003
        m.directory.put("LOCAL", "local_addresses", "Ethernet12|30.30.30.30", {"interface": "Ethernet12", "prefixlen": "24"})
        m.directory.put("LOCAL", "interfaces", "Ethernet12", {"admin_status": "up", "vrf_name": "Vrf_0003"})
        # Peer in Vrf_0003 should match Ethernet12, not Ethernet4
        res = m.set_handler("Vrf_0003|30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert res, "Expect True: peer in Vrf_0003 should match Ethernet12 (same VRF)"

def test_overlapping_ip_different_vnets():
    """Test that the same IP on two VNET interfaces matches the correct one"""
    for constant in load_constant_files():
        m = constructor(constant, vrf="Vnet-10")
        # Add a second interface with the SAME IP but in Vnet-20
        m.directory.put("LOCAL", "local_addresses", "Ethernet12|30.30.30.30", {"interface": "Ethernet12", "prefixlen": "24"})
        m.directory.put("LOCAL", "interfaces", "Ethernet12", {"admin_status": "up", "vnet_name": "Vnet-20"})
        # Peer in Vnet-20 should match Ethernet12, not Ethernet4
        res = m.set_handler("Vnet-20|30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert res, "Expect True: peer in Vnet-20 should match Ethernet12 (same VNET)"

@patch('bgpcfgd.managers_bgp.log_debug')
def test_add_peer_vnet_mismatch(mocked_log_debug):
    """Test that a peer in Vnet-20 cannot pass dependency check using an address only present in Vnet-10"""
    for constant in load_constant_files():
        m = constructor(constant, vrf="Vnet-10")
        # Peer is in Vnet-20 but the local address 30.30.30.30 only exists on Ethernet4 in Vnet-10
        res = m.set_handler("Vnet-20|30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert not res, "Expect False: VNET mismatch should block peer addition"

@patch('bgpcfgd.managers_bgp.log_debug')
def test_add_peer_default_vrf_rejects_vnet_bound_interface(mocked_log_debug):
    """Test that a default VRF peer cannot match an interface bound to a VNET"""
    for constant in load_constant_files():
        m = constructor(constant, vrf="Vnet-10")
        # Peer is in default VRF but local address 30.30.30.30 is on Ethernet4 in Vnet-10
        res = m.set_handler("30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert not res, "Expect False: default VRF peer should not match VNET-bound interface"


def test_add_unnumbered_peer_in_vrf():
    for constant in load_constant_files():
        m = constructor(constant)
        m.directory.put("LOCAL", "interfaces", "PortChannel101", {})
        res = m.set_handler("Vrf-10|PortChannel101", {'asn': '65200', 'name': 'TOR'})
        assert res, "Expect True return value"
        assert any(
            'router bgp 65100 vrf Vrf-10' in call.args[0]
            and 'neighbor PEER_UNNUMBERED peer-group' in call.args[0]
            for call in m.cfg_mgr.push.call_args_list
        )
        assert any(
            'router bgp 65100 vrf Vrf-10' in call.args[0]
            and 'neighbor PortChannel101 interface peer-group PEER_UNNUMBERED' in call.args[0]
            for call in m.cfg_mgr.push.call_args_list
        )


def test_unnumbered_peer_manager_depends_on_port_table():
    for constant in load_constant_files():
        peers = load_constants(constant)['constants']['bgp']['peers']
        port_dependency = ("CONFIG_DB", swsscommon.CFG_PORT_TABLE_NAME, "")
        portchannel_dependency = ("CONFIG_DB", "PORTCHANNEL", "")
        for peer_type in ("general", "internal", "voq_chassis"):
            dependencies = constructor(constant, peer_type=peer_type).deps
            assert port_dependency in dependencies
            assert portchannel_dependency not in dependencies
        for peer_type in ("dynamic", "monitors", "sentinels"):
            if peer_type in peers:
                dependencies = constructor(constant, peer_type=peer_type).deps
                assert port_dependency not in dependencies
                assert portchannel_dependency not in dependencies


def test_add_unnumbered_peer_from_port_table():
    for constant in load_constant_files():
        for neighbor in ("Ethernet-Future0", "Ethernet-BP0", "Ethernet-Rec0", "Ethernet-IB0"):
            m = constructor(constant)
            m.directory.put("CONFIG_DB", swsscommon.CFG_PORT_TABLE_NAME, neighbor, {})
            res = m.set_handler(neighbor, {'asn': '65200', 'name': 'TOR'})
            assert res, "Expect True return value"
            assert any(
                'neighbor %s interface peer-group PEER_UNNUMBERED' % neighbor in call.args[0]
                for call in m.cfg_mgr.push.call_args_list
            )


def test_add_unnumbered_peer_from_interface_table():
    for constant in load_constant_files():
        for neighbor in ("PortChannel101", "Vlan1000", "Eth24.15"):
            m = constructor(constant)
            m.directory.put("LOCAL", "interfaces", neighbor, {})
            res = m.set_handler(neighbor, {'asn': '65200', 'name': 'TOR'})
            assert res, "Expect True return value"
            assert any(
                'neighbor %s interface peer-group PEER_UNNUMBERED' % neighbor in call.args[0]
                for call in m.cfg_mgr.push.call_args_list
            )


def test_late_portchannel_neighbor_converges():
    for constant in load_constant_files():
        for peer_type, peer_group in (
            ("general", "PEER_UNNUMBERED"),
            ("internal", "INTERNAL_PEER_UNNUMBERED"),
            ("voq_chassis", "VOQ_CHASSIS_PEER_UNNUMBERED"),
        ):
            m = constructor(
                constant,
                peer_type=peer_type,
                with_lo4096_ipv4=(peer_type == "internal")
            )
            satisfy_non_port_dependencies(m)
            m.directory.put("CONFIG_DB", swsscommon.CFG_PORT_TABLE_NAME, "Ethernet0", {})

            m.handler(
                "PortChannel101",
                manager_mod.swsscommon.SET_COMMAND,
                {'asn': '65200', 'name': 'TOR'}
            )
            assert len(m.set_queue) == 1

            m.directory.put("CONFIG_DB", "PORTCHANNEL", "PortChannel101", {})

            assert m.set_queue == []
            assert any(
                'neighbor PortChannel101 interface peer-group %s' % peer_group in call.args[0]
                for call in m.cfg_mgr.push.call_args_list
            )


def test_latest_pending_set_replaces_earlier_data():
    for constant in load_constant_files():
        m = constructor(constant)
        satisfy_non_port_dependencies(m)

        m.handler(
            "Ethernet-Future0",
            manager_mod.swsscommon.SET_COMMAND,
            {'asn': '65200', 'name': 'TOR'}
        )
        m.handler(
            "Ethernet-Future0",
            manager_mod.swsscommon.SET_COMMAND,
            {'asn': '65300', 'name': 'TOR'}
        )

        assert m.set_queue == [
            ("Ethernet-Future0", {'asn': '65300', 'name': 'TOR'})
        ]

        m.directory.put("CONFIG_DB", swsscommon.CFG_PORT_TABLE_NAME, "Ethernet-Future0", {})

        assert m.set_queue == []
        assert any(
            'neighbor Ethernet-Future0 remote-as 65300' in call.args[0]
            for call in m.cfg_mgr.push.call_args_list
        )
        assert not any(
            'neighbor Ethernet-Future0 remote-as 65200' in call.args[0]
            for call in m.cfg_mgr.push.call_args_list
        )


@patch('bgpcfgd.managers_bgp.log_debug')
def test_defer_non_ip_neighbor_missing_from_interface_tables(mocked_log_debug):
    for constant in load_constant_files():
        m = constructor(constant)
        res = m.set_handler("Ethernet-Future0", {'asn': '65200', 'name': 'TOR'})
        assert not res, "Expect False return value"
        mocked_log_debug.assert_called_with(
            "Peer 'Ethernet-Future0' is not yet present in the PORT, PORTCHANNEL, or interface tables"
        )


@patch('bgpcfgd.managers_bgp.log_err')
@patch('bgpcfgd.managers_bgp.log_warn')
def test_late_port_neighbor_converges_without_errors(mocked_log_warn, mocked_log_err):
    for constant in load_constant_files():
        m = constructor(constant)
        satisfy_non_port_dependencies(m)

        m.handler(
            "Ethernet-Future0",
            manager_mod.swsscommon.SET_COMMAND,
            {'asn': '65200', 'name': 'TOR'}
        )
        assert len(m.set_queue) == 1

        for port in ("Ethernet0", "Ethernet4", "Ethernet8"):
            m.directory.put("CONFIG_DB", swsscommon.CFG_PORT_TABLE_NAME, port, {})

        assert len(m.set_queue) == 1
        m.directory.put("CONFIG_DB", swsscommon.CFG_PORT_TABLE_NAME, "Ethernet-Future0", {})

        assert m.set_queue == []
        mocked_log_err.assert_not_called()
        mocked_log_warn.assert_not_called()
        assert any(
            'neighbor Ethernet-Future0 interface peer-group PEER_UNNUMBERED' in call.args[0]
            for call in m.cfg_mgr.push.call_args_list
        )


def test_delete_removes_pending_peer():
    for constant in load_constant_files():
        for key in ("Ethernet-Future0", "Vrf-10|Ethernet-Future0"):
            m = constructor(constant)
            satisfy_non_port_dependencies(m)

            m.handler(
                key,
                manager_mod.swsscommon.SET_COMMAND,
                {'asn': '65200', 'name': 'TOR'}
            )
            assert len(m.set_queue) == 1

            m.handler(key, manager_mod.swsscommon.DEL_COMMAND, {})
            assert m.set_queue == []

            m.directory.put("CONFIG_DB", swsscommon.CFG_PORT_TABLE_NAME, "Ethernet-Future0", {})
            assert not any(
                'neighbor Ethernet-Future0 interface peer-group' in call.args[0]
                for call in m.cfg_mgr.push.call_args_list
            )


@patch('bgpcfgd.managers_bgp.log_info')
def test_add_dynamic_peer(mocked_log_info):
    for constant in load_constant_files():
        m = constructor(constant, peer_type="dynamic")
        m.check_neig_meta = False
        res = m.set_handler("BGPSLBPassive", {"peer_asn": "65200", "ip_range": "10.250.0.0/27", "name": "BGPSLBPassive", "src_address": "10.250.0.1"})
        mocked_log_info.assert_called_with("Peer '(default|BGPSLBPassive)' has been scheduled to be added with attributes '{'peer_asn': '65200', 'ip_range': '10.250.0.0/27', 'name': 'BGPSLBPassive', 'src_address': '10.250.0.1'}'")
        assert res, "Expect True return value"

@patch('bgpcfgd.managers_bgp.log_info')
def test_add_dynamic_peer_ipv6(mocked_log_info):
    for constant in load_constant_files():
        m = constructor(constant, peer_type="dynamic")
        m.check_neig_meta = False
        res = m.set_handler("BGPSLBPassive", {"peer_asn": "65200", "ip_range": "fc00:20::/64", "name": "BGPSLBPassive", "src_address": "fc00:20::1"})
        mocked_log_info.assert_called_with("Peer '(default|BGPSLBPassive)' has been scheduled to be added with attributes '{'peer_asn': '65200', 'ip_range': 'fc00:20::/64', 'name': 'BGPSLBPassive', 'src_address': 'fc00:20::1'}'")
        assert res, "Expect True return value"

@patch('bgpcfgd.managers_bgp.log_info')
@patch('bgpcfgd.managers_bgp.swsscommon.Table')
@patch('bgpcfgd.managers_bgp.swsscommon.DBConnector')
def modify_dynamic_peer_common(mock_db_conn, mock_table, mocked_log_info, peer, data, update_log, final_log):
    for constant in load_constant_files():
        m = constructor(constant, peer_type="dynamic")
        m.cfg_mgr.push = MagicMock(return_value = None)
        m.check_neig_meta = False
        swsscommon.STATE_BGP_PEER_CONFIGURED_TABLE_NAME = "BGP_PEER_CONFIGURED_TABLE"
        mock_state_db_table = MagicMock()
        mock_table.return_value = mock_state_db_table
        res = m.set_handler(peer, data)
        assert res, "Expect True return value"
        if "update" in m.templates:
            mock_state_db_table.set.assert_called_once_with(peer, list(sorted(data.items())))
            mocked_log_info.assert_any_call(update_log)
            mocked_log_info.assert_called_with(final_log)

def test_add_dynamic_peer_range():
    data = {"peer_asn": "65200", "ip_range": "10.255.0.0/24,10.255.1.0/24", "name": "DynNbr1"}
    peer = "DynNbr1"
    update_log = "Peer '(default|DynNbr1)' ip range is going to be updated. Ranges to delete: [] Ranges to add: ['10.255.1.0/24']"
    final_log = "Peer '(default|DynNbr1)' ip range has been scheduled to be updated with range '10.255.0.0/24,10.255.1.0/24'"
    modify_dynamic_peer_common(peer=peer, data=data, update_log=update_log, final_log=final_log)

def test_modify_dynamic_peer_range():
    data = {"peer_asn": "65200", "ip_range": "10.255.0.0/26", "name": "DynNbr1"}
    peer = "DynNbr1"
    update_log = "Peer '(default|DynNbr1)' ip range is going to be updated. Ranges to delete: ['10.255.0.0/24'] Ranges to add: ['10.255.0.0/26']"
    final_log = "Peer '(default|DynNbr1)' ip range has been scheduled to be updated with range '10.255.0.0/26'"
    modify_dynamic_peer_common(peer=peer, data=data, update_log=update_log, final_log=final_log)

def test_delete_dynamic_peer_range():
    data = {"peer_asn": "65200", "ip_range": "192.168.0.0/24", "name": "DynNbr2"}
    peer = "DynNbr2"
    update_log = "Peer '(default|DynNbr2)' ip range is going to be updated. Ranges to delete: ['192.168.1.0/24'] Ranges to add: []"
    final_log = "Peer '(default|DynNbr2)' ip range has been scheduled to be updated with range '192.168.0.0/24'"
    modify_dynamic_peer_common(peer=peer, data=data, update_log=update_log, final_log=final_log)

@patch('bgpcfgd.managers_bgp.log_warn')
def test_add_peer_no_local_addr(mocked_log_warn):
    for constant in load_constant_files():
        m = constructor(constant)
        res = m.set_handler("30.30.30.1", {"admin_status": "up"})
        assert res, "Expect True return value"
        mocked_log_warn.assert_called_with("Peer 30.30.30.1. Missing attribute 'local_addr'")

@patch('bgpcfgd.managers_bgp.log_debug')
def test_add_peer_invalid_local_addr(mocked_log_debug):
    for constant in load_constant_files():
        m = constructor(constant)
        res = m.set_handler("30.30.30.1", {"local_addr": "40.40.40.40", "admin_status": "up"})
        assert not res, "Expect False return value"
        mocked_log_debug.assert_called_with("Peer '30.30.30.1' with local address '40.40.40.40' wait for the corresponding interface to be set")

@patch('bgpcfgd.managers_bgp.log_info')
def test_del_handler(mocked_log_info):
    for constant in load_constant_files():
        m = constructor(constant)
        m.del_handler("10.10.10.1")
        mocked_log_info.assert_called_with("Peer '(default|10.10.10.1)' has been removed")
    
@patch('bgpcfgd.managers_bgp.log_info')
def test_del_handler_dynamic_template_exists(mocked_log_info):
    for constant in load_constant_files():
        m = constructor(constant, peer_type="dynamic")
        base_template = "bgpd/templates/" + m.constants["bgp"]["peers"]["dynamic"]["template_dir"] + "/delete.conf.j2"
        if os.path.exists(TEMPLATE_PATH + "/" + base_template):
            mocked_log_info.assert_called_with("Using delete template found at %s" % base_template)
        m.del_handler("10.10.10.1")
        mocked_log_info.assert_called_with("Peer '(default|10.10.10.1)' has been removed")

@patch('bgpcfgd.managers_bgp.log_warn')
def test_del_handler_nonexist_peer(mocked_log_warn):
    for constant in load_constant_files():
        m = constructor(constant)
        m.del_handler("40.40.40.1")
        mocked_log_warn.assert_called_with("Peer '(default|40.40.40.1)' has not been found")

@patch('bgpcfgd.managers_bgp.log_info')
@patch('bgpcfgd.managers_bgp.log_warn')
def test_del_handler_dynamic_nonexist_peer_template_exists(mocked_log_warn, mocked_log_info):
    for constant in load_constant_files():
        m = constructor(constant, peer_type="dynamic")
        base_template = "bgpd/templates/" + m.constants["bgp"]["peers"]["dynamic"]["template_dir"] + "/delete.conf.j2"
        if os.path.exists(TEMPLATE_PATH + "/" + base_template):
            mocked_log_info.assert_called_with("Using delete template found at %s" % base_template)
        m.del_handler("40.40.40.1")
        mocked_log_warn.assert_called_with("Peer '(default|40.40.40.1)' has not been found")
