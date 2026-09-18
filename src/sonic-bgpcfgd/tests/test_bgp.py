from contextlib import contextmanager
from copy import deepcopy
from unittest.mock import MagicMock, call, patch

import os
import pytest
from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from . import swsscommon_test
from .util import CONSTANTS_PATH, load_constants, render_constants
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

def test_peer_key_validation():
    for constant in load_constant_files():
        m = constructor(constant)
        assert m.parse_key("Vrf-RED_1|FC00:10::1") == ("Vrf-RED_1", "fc00:10::1")
        assert m.parse_key("default|Ethernet0") == ("default", "Ethernet0")

        for key in (None, "vrf name|10.10.10.1", "default|not;an-address",
                    "default|Ethernet-Future0",
                    "default|10.10.10.1" + chr(10)):
            assert m.parse_key(key) is None

        dynamic = constructor(constant, peer_type="dynamic")
        assert dynamic.parse_key("default|BGPSLB-Passive_1") == (
            "default", "BGPSLB-Passive_1")
        assert dynamic.parse_key("vnet1|BGPWithVnet") == (
            "vnet1", "BGPWithVnet")
        long_vnet = "vnet-" + "x" * 250
        assert dynamic.parse_key(long_vnet + "|BGPWithVnet") == (
            long_vnet, "BGPWithVnet")
        assert m.parse_key(long_vnet + "|10.10.10.1") is None

        for key in ("default|", "default|peer group", "default|peer" + chr(10),
                    long_vnet + "x|BGPPeer",
                    "-vnet|BGPPeer", "vnet;show|BGPPeer",
                    "vnet1|peer;show"):
            assert dynamic.parse_key(key) is None

def test_invalid_peer_keys_are_ignored():
    for constant in load_constant_files():
        m = constructor(constant)
        m.cfg_mgr.push.reset_mock()

        assert m.set_handler("vrf name|10.10.10.1", {"admin_status": "up"})
        m.del_handler("default|10.10.10.1" + chr(10))

        m.cfg_mgr.push.assert_not_called()

def test_add_peer():
    for constant in load_constant_files():
        m = constructor(constant)
        res = m.set_handler("30.30.30.1", {'asn': '65200', 'holdtime': '180', 'keepalive': '60', 'local_addr': '30.30.30.30', 'name': 'TOR', 'nhopself': '0', 'rrclient': '0'})
        assert res, "Expect True return value"


@pytest.fixture(scope="module", params=load_constant_files(), ids=lambda path: os.path.basename(path))
def peer_name_constants(request):
    return request.param


@pytest.fixture(params=('general', 'internal', 'monitors', 'voq_chassis'))
def peer_name_manager(request, peer_name_constants, peer_name_state_table):
    return constructor(peer_name_constants, peer_type=request.param, with_lo4096_ipv4=True)


@pytest.fixture
def peer_name_state_table():
    # Other test modules replace swsscommon during collection; keep BGP and
    # the shared Manager's operation constants consistent in the full suite.
    with patch('bgpcfgd.managers_bgp.swsscommon.SET_COMMAND', 'SET'), \
            patch('bgpcfgd.managers_bgp.swsscommon.DEL_COMMAND', 'DEL'), \
            patch('bgpcfgd.managers_bgp.swsscommon.DBConnector'), \
            patch('bgpcfgd.managers_bgp.swsscommon.Table') as table:
        table.return_value.get.return_value = (True, [])
        yield table.return_value


def make_peer_dependencies_ready(m):
    metadata = dict(m.directory.get("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost"))
    metadata.update(type="ToRRouter", deployment_id="1")
    m.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost", metadata)
    m.directory.put("CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME, "tsa_enabled", "false")
    m.directory.put("CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME, "idf_isolation_state", "unisolated")
    m.directory.put("CONFIG_DB", swsscommon.CFG_PORT_TABLE_NAME, "Ethernet4", {})
    m.directory.put("CONFIG_DB", swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME, "Loopback0", {})
    m.directory.put("CONFIG_DB", swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME, "Loopback4096", {})
    assert m.directory.available_deps(m.deps)


@contextmanager
def assert_peer_event_discarded(m, key, data, expected_error=None):
    peers = m.peers.copy()
    directory = {slot: deepcopy(values) for slot, values in m.directory.data.items()}
    original_data = deepcopy(data)
    initialized = m.post_dependencies_init_complete
    loopbacks = m.loopbacks[:]
    m.cfg_mgr.reset_mock()
    with patch.object(m, 'add_peer', wraps=m.add_peer) as add_peer, \
            patch.object(m, 'update_peer', wraps=m.update_peer) as update_peer, \
            patch.object(m, 'update_state_db', wraps=m.update_state_db) as update_state_db, \
            patch.object(m.directory, 'put', wraps=m.directory.put) as directory_put, \
            patch('bgpcfgd.managers_bgp.swsscommon.DBConnector') as db_connector, \
            patch('bgpcfgd.managers_bgp.log_err') as log_err:
        yield
        add_peer.assert_not_called()
        update_peer.assert_not_called()
        update_state_db.assert_not_called()
        directory_put.assert_not_called()
        db_connector.assert_not_called()
        m.cfg_mgr.push.assert_not_called()
        if expected_error is not None:
            log_err.assert_called_once_with(expected_error)
        elif isinstance(key, str):
            log_err.assert_called_once_with(
                "Peer '(%s|%s)' name must not contain newline characters" % m.split_key(key)
            )
        else:
            log_err.assert_called_once_with("Invalid BGP peer table key: {!r}".format(key))
    assert m.peers == peers
    assert m.directory.data == directory
    assert data == original_data
    assert m.post_dependencies_init_complete == initialized
    assert m.loopbacks == loopbacks
    assert m.set_queue == []


@pytest.mark.parametrize('newline', ['\n', '\r', '\r\n'], ids=['LF', 'CR', 'CRLF'])
@pytest.mark.parametrize('key', ['30.30.30.1', '10.10.10.1', 'Vrf-10|30.30.30.1'],
                         ids=['new', 'existing', 'vrf'])
@pytest.mark.parametrize('admin_status', [None, 'up', 'down'])
@pytest.mark.parametrize('entry', ['direct', 'handler-ready', 'handler-missing', 'replay'])
def test_set_peer_rejects_multiline_name(peer_name_manager, newline, key, admin_status, entry):
    m = peer_name_manager
    data = {'asn': '65200', 'local_addr': '30.30.30.30/24', 'name': 'TOR' + newline + 'SECOND LINE'}
    if admin_status is not None:
        data['admin_status'] = admin_status
    if entry in ('handler-ready', 'replay'):
        make_peer_dependencies_ready(m)
    else:
        assert not m.directory.available_deps(m.deps)
    if entry == 'replay':
        m.set_queue.append((key, data))

    with assert_peer_event_discarded(m, key, data):
        if entry == 'direct':
            assert m.set_handler(key, data) is True, "Invalid SET must be consumed, not retried"
        elif entry == 'replay':
            m.on_deps_change()
            m.on_deps_change()
        else:
            m.handler(key, swsscommon.SET_COMMAND, data)
            m.on_deps_change()


@pytest.mark.parametrize('key', [None, 42, [], {}], ids=['none', 'number', 'list', 'dict'])
@pytest.mark.parametrize('entry', ['direct', 'handler-ready', 'handler-missing', 'replay'])
def test_invalid_key_with_multiline_name_is_consumed(peer_name_manager, key, entry):
    m = peer_name_manager
    data = {'name': 'TOR\r\nSECOND LINE', 'admin_status': 'down'}
    if entry in ('handler-ready', 'replay'):
        make_peer_dependencies_ready(m)
    else:
        assert not m.directory.available_deps(m.deps)
    if entry == 'replay':
        m.set_queue.append((key, data))

    with assert_peer_event_discarded(m, key, data):
        if entry == 'direct':
            assert m.set_handler(key, data) is True
        elif entry == 'replay':
            m.on_deps_change()
            m.on_deps_change()
        else:
            m.handler(key, swsscommon.SET_COMMAND, data)
            m.on_deps_change()


@pytest.mark.parametrize('newline', ['\n', '\r', '\r\n'], ids=['LF', 'CR', 'CRLF'])
def test_queued_invalid_add_after_valid_peer_creation(peer_name_manager, newline):
    m = peer_name_manager
    key = '30.30.30.1'
    make_peer_dependencies_ready(m)
    invalid = {'asn': '65200', 'local_addr': '30.30.30.30', 'name': 'TOR' + newline, 'admin_status': 'down'}
    # Simulate a stale event queued before validation was introduced.
    m.set_queue.append((key, invalid))
    valid = {'asn': '65200', 'local_addr': '30.30.30.30', 'name': 'TOR'}
    assert m.set_handler(key, valid)
    assert ('default', key) in m.peers
    assert m.set_queue == [(key, invalid)]
    with assert_peer_event_discarded(m, key, invalid):
        m.on_deps_change()
        m.on_deps_change()
    assert m.directory.get(m.db_name, m.table_name, 'default|' + key) == valid


def test_invalid_replay_does_not_block_retryable_peer(peer_name_manager):
    m = peer_name_manager
    make_peer_dependencies_ready(m)
    key = '30.30.30.1'
    data = {'asn': '65200', 'local_addr': '40.40.40.40', 'name': 'TOR'}
    m.handler(key, swsscommon.SET_COMMAND, data)
    assert m.set_queue == [(key, data)]
    m.cfg_mgr.push.assert_not_called()
    m.set_queue.insert(0, (key, {'name': 'TOR\n', 'admin_status': 'down'}))
    with patch('bgpcfgd.managers_bgp.log_err') as log_err:
        m.on_deps_change()
        assert m.set_queue == [(key, data)]
        m.cfg_mgr.push.assert_not_called()
        m.directory.put("LOCAL", "local_addresses", "Ethernet4|40.40.40.40",
                        {"interface": "Ethernet4", "prefixlen": "24"})
        log_err.assert_called_once_with(
            "Peer '(default|30.30.30.1)' name must not contain newline characters"
        )
    assert m.set_queue == []
    assert ('default', key) in m.peers
    assert m.directory.get(m.db_name, m.table_name, 'default|' + key) == data
    m.cfg_mgr.push.assert_called()


@pytest.mark.parametrize('name', ['TOR', '', None], ids=['valid', 'empty', 'missing'])
@pytest.mark.parametrize('key', ['30.30.30.1', '10.10.10.1'], ids=['new', 'existing'])
@pytest.mark.parametrize('entry', ['direct', 'handler-ready', 'handler-missing'])
def test_set_peer_valid_name_unchanged(peer_name_manager, peer_name_state_table, name, key, entry):
    m = peer_name_manager
    data = {'asn': '65200', 'local_addr': '30.30.30.30', 'admin_status': 'up'}
    if name is not None:
        data['name'] = name
    if m.check_neig_meta and name == '':
        m.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_NEIGHBOR_METADATA_TABLE_NAME, '', {})
    if entry == 'handler-ready':
        make_peer_dependencies_ready(m)
    with patch('bgpcfgd.managers_bgp.log_err') as log_err:
        if entry == 'direct':
            assert m.set_handler(key, data) is True
        else:
            m.handler(key, swsscommon.SET_COMMAND, data)
            if entry == 'handler-missing':
                assert m.set_queue == [(key, data)]
                m.cfg_mgr.push.assert_not_called()
                make_peer_dependencies_ready(m)
        log_err.assert_not_called()
    assert m.set_queue == []
    assert ('default', key) in m.peers
    assert m.directory.get(m.db_name, m.table_name, 'default|' + key) == data
    m.cfg_mgr.push.assert_called()
    peer_name_state_table.set.assert_called_once_with(key, sorted(data.items()))


@pytest.fixture
def sentinel_manager(peer_name_state_table):
    with patch.object(bgpcfgd.managers_bgp.BGPPeerMgrBase, 'load_peers',
                      return_value={('default', 'BGPSentinelExisting')}):
        m = constructor(CONSTANTS_PATH, peer_type='sentinels')
    m.table_name = 'BGP_SENTINELS'
    m.directory.put(m.db_name, m.table_name, 'default|BGPSentinelExisting',
                    {'name': 'BGPSentinelExisting', 'src_address': '10.1.0.32',
                     'ip_range': '10.1.0.0/24'})
    if m.check_neig_meta:
        for name in ('BGPSentinel', 'BGPSentinelExisting'):
            m.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_NEIGHBOR_METADATA_TABLE_NAME, name, {})
    return m


@pytest.fixture(params=[
    ('10.1.0.32', '10.1.0.0/24'),
    ('fc00:1::32', '2603:10a0:321:82f9::/64,2603:10a1:30a:8000::/59'),
], ids=['ipv4', 'ipv6'])
def sentinel_data(request):
    address, ranges = request.param
    return {'name': 'BGPSentinel', 'src_address': address, 'ip_range': ranges}


@pytest.mark.parametrize('newline', ['\n', '\r', '\r\n'], ids=['LF', 'CR', 'CRLF'])
@pytest.mark.parametrize('entry,existing,admin_status', [
    ('direct', False, None),
    ('direct', True, None),
    ('direct', True, 'up'),
    ('direct', True, 'down'),
    ('handler-ready', False, None),
    ('handler-ready', True, None),
    ('handler-missing', False, None),
    ('handler-missing', True, 'down'),
    ('replay', False, None),
    ('replay', True, 'up'),
])
def test_sentinel_rejects_multiline_name(sentinel_manager, newline, entry, existing, admin_status):
    m = sentinel_manager
    key = 'BGPSentinelExisting' if existing else 'BGPSentinel'
    data = {'name': key + newline + 'SECOND LINE', 'src_address': '10.1.0.32',
            'ip_range': '10.1.0.0/24'}
    if admin_status is not None:
        data['admin_status'] = admin_status
    if entry in ('handler-ready', 'replay'):
        make_peer_dependencies_ready(m)
    else:
        assert not m.directory.available_deps(m.deps)
    if entry == 'replay':
        m.set_queue.append((key, data))
    with assert_peer_event_discarded(m, key, data):
        if entry == 'direct':
            assert m.set_handler(key, data) is True
        elif entry == 'replay':
            m.on_deps_change()
            m.on_deps_change()
        else:
            m.handler(key, swsscommon.SET_COMMAND, data)
            m.on_deps_change()


@pytest.mark.parametrize('entry', ['direct', 'handler-ready', 'handler-missing'])
def test_sentinel_valid_add_and_delete(sentinel_manager, sentinel_data, peer_name_state_table, entry):
    m = sentinel_manager
    key = sentinel_data['name']
    if entry == 'handler-ready':
        make_peer_dependencies_ready(m)
    with patch('bgpcfgd.managers_bgp.log_err') as log_err:
        if entry == 'direct':
            assert m.set_handler(key, sentinel_data) is True
        else:
            m.handler(key, swsscommon.SET_COMMAND, sentinel_data)
            if entry == 'handler-missing':
                assert m.set_queue == [(key, sentinel_data)]
                m.cfg_mgr.push.assert_not_called()
                make_peer_dependencies_ready(m)
        log_err.assert_not_called()
    assert m.set_queue == []
    assert ('default', key) in m.peers
    assert m.directory.get(m.db_name, m.table_name, 'default|' + key) == sentinel_data
    peer_name_state_table.set.assert_called_once_with(key, sorted(sentinel_data.items()))
    commands = '\n'.join(call.args[0] for call in m.cfg_mgr.push.call_args_list)
    assert 'template: bgpd/templates/sentinels/instance.conf.j2' in commands
    assert 'neighbor BGPSentinel peer-group' in commands
    assert 'neighbor BGPSentinel update-source ' + sentinel_data['src_address'] in commands
    for prefix in sentinel_data['ip_range'].split(','):
        assert 'bgp listen range ' + prefix + ' peer-group BGPSentinel' in commands
    m.cfg_mgr.reset_mock()
    with patch('bgpcfgd.managers_bgp.log_err') as log_err:
        m.handler(key, swsscommon.DEL_COMMAND, {'name': key + '\r\n'})
        log_err.assert_not_called()
    commands = '\n'.join(call.args[0] for call in m.cfg_mgr.push.call_args_list)
    for prefix in sentinel_data['ip_range'].split(','):
        assert 'no bgp listen range ' + prefix + ' peer-group BGPSentinel' in commands
    assert 'no neighbor BGPSentinel' in commands
    assert ('default', key) not in m.peers
    assert 'default|' + key not in m.directory.get_slot(m.db_name, m.table_name)
    peer_name_state_table.delete.assert_called_once_with(key)
    assert m.set_queue == []


def test_sentinel_valid_cached_update(sentinel_manager, sentinel_data, peer_name_state_table):
    m = sentinel_manager
    key = 'BGPSentinelExisting'
    data = dict(sentinel_data, name=key, admin_status='up')
    make_peer_dependencies_ready(m)
    with patch('bgpcfgd.managers_bgp.log_err') as log_err:
        m.handler(key, swsscommon.SET_COMMAND, data)
        log_err.assert_not_called()
    assert ('default', key) in m.peers
    assert m.directory.get(m.db_name, m.table_name, 'default|' + key) == data
    peer_name_state_table.set.assert_called_once_with(key, sorted(data.items()))
    m.cfg_mgr.push.assert_called_once()
    assert 'no neighbor BGPSentinelExisting shutdown' in m.cfg_mgr.push.call_args.args[0]
    assert m.set_queue == []


@pytest.mark.parametrize('newline', ['\n', '\r', '\r\n'], ids=['LF', 'CR', 'CRLF'])
def test_sentinel_stale_replay_after_valid_add(sentinel_manager, newline):
    m = sentinel_manager
    make_peer_dependencies_ready(m)
    key = 'BGPSentinel'
    valid = {'name': key, 'src_address': '10.1.0.32', 'ip_range': '10.1.0.0/24'}
    invalid = dict(valid, name=key + newline, admin_status='down')
    m.set_queue.append((key, invalid))
    assert m.set_handler(key, valid) is True
    assert ('default', key) in m.peers
    assert m.set_queue == [(key, invalid)]
    with assert_peer_event_discarded(m, key, invalid):
        m.on_deps_change()
        m.on_deps_change()
    assert m.directory.get(m.db_name, m.table_name, 'default|' + key) == valid


@pytest.mark.parametrize('key', [
    'BGPSentinel\n', 'BGPSentinel\r', 'BGPSentinel\r\n',
    'default|BGPSentinel\n', 'default|BGPSentinel\r', 'default|BGPSentinel\r\n',
    'Vrf\n|BGPSentinel', 'Vrf\r|BGPSentinel', 'Vrf\r\n|BGPSentinel',
    None, 42, [], {},
], ids=[
    'bare-LF', 'bare-CR', 'bare-CRLF',
    'qualified-LF', 'qualified-CR', 'qualified-CRLF',
    'vrf-LF', 'vrf-CR', 'vrf-CRLF',
    'none', 'number', 'list', 'dict',
])
@pytest.mark.parametrize('name', ['BGPSentinel', '', None], ids=['clean', 'empty', 'missing'])
@pytest.mark.parametrize('entry', ['direct', 'handler-ready', 'handler-missing', 'replay'])
def test_sentinel_rejects_invalid_key_independently_of_name(sentinel_manager, key, name, entry):
    m = sentinel_manager
    data = {'src_address': '10.1.0.32', 'ip_range': '10.1.0.0/24', 'admin_status': 'down'}
    if name is not None:
        data['name'] = name
    if entry in ('handler-ready', 'replay'):
        make_peer_dependencies_ready(m)
    else:
        assert not m.directory.available_deps(m.deps)
    if entry == 'replay':
        m.set_queue.append((key, data))
    with assert_peer_event_discarded(
            m, key, data, expected_error="Invalid BGP peer table key: {!r}".format(key)):
        if entry == 'direct':
            assert m.set_handler(key, data) is True
        elif entry == 'replay':
            m.on_deps_change()
            m.on_deps_change()
        else:
            m.handler(key, swsscommon.SET_COMMAND, data)
            assert m.set_queue == []
            m.on_deps_change()


def test_sentinel_invalid_key_does_not_discard_valid_queued_peer(sentinel_manager, sentinel_data):
    m = sentinel_manager
    key = sentinel_data['name']
    m.handler(key, swsscommon.SET_COMMAND, sentinel_data)
    assert m.set_queue == [(key, sentinel_data)]
    with patch('bgpcfgd.managers_bgp.log_err') as log_err:
        m.handler(key + '\n', swsscommon.SET_COMMAND, sentinel_data)
        assert m.set_queue == [(key, sentinel_data)]
        m.cfg_mgr.push.assert_not_called()
        # A stale invalid event must not prevent the valid event from draining.
        m.set_queue.insert(0, (key + '\r', sentinel_data))
        make_peer_dependencies_ready(m)
        assert log_err.call_args_list == [
            call("Invalid BGP peer table key: {!r}".format(key + '\n')),
            call("Invalid BGP peer table key: {!r}".format(key + '\r')),
        ]
    assert m.set_queue == []
    assert ('default', key) in m.peers
    assert m.directory.get(m.db_name, m.table_name, 'default|' + key) == sentinel_data
    m.cfg_mgr.push.assert_called()


@pytest.mark.parametrize('key', [None, 42, [], {}, 'default|bad name', 'default|bad\r\n'])
@pytest.mark.parametrize('entry', ['direct', 'handler-missing', 'replay'])
def test_sentinel_malformed_key_with_multiline_name(sentinel_manager, key, entry):
    m = sentinel_manager
    data = {'name': 'BGPSentinel\r\n', 'admin_status': 'down'}
    if entry == 'replay':
        make_peer_dependencies_ready(m)
        m.set_queue.append((key, data))
    expected_error = None
    if isinstance(key, str) and ('\r' in key or '\n' in key):
        expected_error = "Invalid BGP peer table key: {!r}".format(key)
    with assert_peer_event_discarded(m, key, data, expected_error=expected_error):
        if entry == 'direct':
            assert m.set_handler(key, data) is True
        elif entry == 'replay':
            m.on_deps_change()
        else:
            m.handler(key, swsscommon.SET_COMMAND, data)


@pytest.mark.parametrize('name', ['', None, 'Different Sentinel Name'])
def test_sentinel_name_guard_preserves_existing_update_behavior(sentinel_manager, name):
    m = sentinel_manager
    data = {'admin_status': 'up'}
    if name is not None:
        data['name'] = name
    with patch('bgpcfgd.managers_bgp.log_err') as log_err:
        assert m.set_handler('BGPSentinelExisting', data) is True
        log_err.assert_not_called()
    m.cfg_mgr.push.assert_called()
    assert m.directory.get(m.db_name, m.table_name, 'default|BGPSentinelExisting') == data


def test_sentinel_key_validation_unchanged(sentinel_manager):
    m = sentinel_manager
    assert m.parse_key('default|BGPSentinel_1') == ('default', 'BGPSentinel_1')
    for key in ('', 'default|', 'bad vrf|BGPSentinel', 'default|bad name',
                'default|BGPSentinel\n', 'fc00:10::1'):
        with patch('bgpcfgd.managers_bgp.log_err') as log_err, \
                patch.object(m, 'add_peer') as add_peer, \
                patch.object(m, 'update_peer') as update_peer:
            assert m.set_handler(key, {'name': 'BGPSentinel'}) is True
            log_err.assert_called_once()
            add_peer.assert_not_called()
            update_peer.assert_not_called()
    m.cfg_mgr.push.assert_not_called()


@pytest.mark.parametrize('peer_type', ['dynamic'])
@pytest.mark.parametrize('newline', ['\n', '\r', '\r\n'], ids=['LF', 'CR', 'CRLF'])
@pytest.mark.parametrize('existing', [False, True], ids=['new', 'existing'])
def test_multiline_name_excluded_peer_types(peer_name_state_table, peer_type, newline, existing):
    m = constructor(CONSTANTS_PATH, peer_type=peer_type)
    key = 'DynNbr1' if existing else 'BGPSLBPassive'
    data = {'name': 'TOR' + newline, 'admin_status': 'up', 'peer_asn': '65200',
            'ip_range': '10.250.0.0/27', 'src_address': '10.250.0.1'}
    # Neighbor metadata readiness is independent of the name validation scope.
    if m.check_neig_meta:
        m.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_NEIGHBOR_METADATA_TABLE_NAME, data['name'], {})
    with patch('bgpcfgd.managers_bgp.log_err') as log_err:
        m.handler(key, swsscommon.SET_COMMAND, data)
        assert m.set_queue == [(key, data)]
        make_peer_dependencies_ready(m)
        log_err.assert_not_called()
    assert m.set_queue == []
    assert ('default', key) in m.peers
    m.cfg_mgr.push.assert_called()
    assert m.directory.get(m.db_name, m.table_name, 'default|' + key) == data


@pytest.mark.parametrize('deps_ready', [False, True])
def test_peer_name_validation_does_not_affect_other_operations(peer_name_manager, deps_ready):
    m = peer_name_manager
    if deps_ready:
        make_peer_dependencies_ready(m)
    data = {'name': 'TOR\r\n', 'admin_status': 'down'}
    with patch('bgpcfgd.managers_bgp.log_err') as log_err:
        m.handler('10.10.10.1', swsscommon.DEL_COMMAND, data)
        log_err.assert_not_called()
    assert ('default', '10.10.10.1') not in m.peers
    m.cfg_mgr.push.assert_called()
    with patch('bgpcfgd.manager.log_err') as manager_log_err, \
            patch('bgpcfgd.managers_bgp.log_err') as log_err:
        m.handler('10.10.10.1', 'OTHER', data)
        manager_log_err.assert_called_once_with("Invalid operation 'OTHER' for key '10.10.10.1'")
        log_err.assert_not_called()
    assert m.set_queue == []


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
        port_dependency = ("CONFIG_DB", swsscommon.CFG_PORT_TABLE_NAME, "")
        assert port_dependency in constructor(constant).deps
        assert port_dependency not in constructor(constant, peer_type="dynamic").deps


def test_add_unnumbered_peer_from_port_table():
    for constant in load_constant_files():
        m = constructor(constant)
        m.directory.put("CONFIG_DB", swsscommon.CFG_PORT_TABLE_NAME, "EthernetFuture0", {})
        res = m.set_handler("EthernetFuture0", {'asn': '65200', 'name': 'TOR'})
        assert res, "Expect True return value"
        assert any(
            'neighbor EthernetFuture0 interface peer-group PEER_UNNUMBERED' in call.args[0]
            for call in m.cfg_mgr.push.call_args_list
        )


@patch('bgpcfgd.managers_bgp.log_err')
def test_reject_unknown_non_ip_neighbor(mocked_log_err):
    for constant in load_constant_files():
        m = constructor(constant)
        res = m.set_handler("EthernetFuture0", {'asn': '65200', 'name': 'TOR'})
        assert not res, "Expect False return value"
        mocked_log_err.assert_called_with(
            "Peer 'EthernetFuture0' is neither a valid IP address nor present in the PORT or interface tables"
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
