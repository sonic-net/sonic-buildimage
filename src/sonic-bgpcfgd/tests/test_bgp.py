from unittest.mock import MagicMock, patch

import os
from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from . import swsscommon_test
from .util import load_constants
from swsscommon import swsscommon
import bgpcfgd.managers_bgp

TEMPLATE_PATH = os.path.abspath('../../dockers/docker-fpm-frr/frr')

def load_constant_files():
    paths = ["tests/data/constants", "../../files/image_config/constants"]
    constant_files = []

    for path in paths:
        constant_files += [os.path.abspath(os.path.join(path, name)) for name in os.listdir(path)
                   if os.path.isfile(os.path.join(path, name)) and name.startswith("constants")]

    return constant_files


def constructor(constants_path, bgp_router_id="", peer_type="general", with_lo0_ipv4=True, with_lo4096_ipv4=False):
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
    m.directory.put("LOCAL", "local_addresses", "30.30.30.30", {"interface": "Ethernet4|30.30.30.30/24"})
    m.directory.put("LOCAL", "local_addresses", "fc00:20::20", {"interface": "Ethernet8|fc00:20::20/96"})
    m.directory.put("LOCAL", "interfaces", "Ethernet4|30.30.30.30/24", {"anything": "anything"})
    m.directory.put("LOCAL", "interfaces", "Ethernet8|fc00:20::20/96", {"anything": "anything"})
    m.directory.put("CONFIG_DB", swsscommon.CFG_BGP_NEIGHBOR_TABLE_NAME, "default|10.10.10.1", {"ip_range": None})

    if m.check_neig_meta:
        m.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_NEIGHBOR_METADATA_TABLE_NAME, "TOR", {})

    return m

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

def test_add_peer_internal_no_router_id_no_lo4096():
    for constant in load_constant_files():
        m = constructor(constant, peer_type="internal")
        res = m.set_handler("30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert not res, "Expect False return value"

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

def test_add_peer_without_lo_ipv4():
    for constant in load_constant_files():
        m = constructor(constant, with_lo0_ipv4=False)
        res = m.set_handler("30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert not res, "Expect False return value"

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
        m = constructor(constant)
        res = m.set_handler("Vnet-10|30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert res, "Expect True return value"

def test_add_peer_ipv6_in_vnet():
    for constant in load_constant_files():
        m = constructor(constant)
        res = m.set_handler("Vnet-10|fc00:20::1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': 'fc00:20::20', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})

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
