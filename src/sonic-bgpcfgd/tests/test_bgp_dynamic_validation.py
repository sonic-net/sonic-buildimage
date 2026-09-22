from copy import deepcopy
from unittest.mock import patch

import pytest

import bgpcfgd.manager
import bgpcfgd.managers_bgp
from .test_bgp import constructor, load_constant_files


@pytest.fixture(params=load_constant_files())
def dynamic_manager(request):
    # Collection can give these modules different swsscommon mock instances.
    # Use the real event strings in both dispatch layers, and restore afterward.
    with patch.object(bgpcfgd.manager.swsscommon, "SET_COMMAND", "SET"), \
            patch.object(bgpcfgd.manager.swsscommon, "DEL_COMMAND", "DEL"), \
            patch.object(bgpcfgd.managers_bgp.swsscommon, "SET_COMMAND", "SET"), \
            patch.object(bgpcfgd.managers_bgp.swsscommon, "DEL_COMMAND", "DEL"), \
            patch.object(bgpcfgd.managers_bgp.swsscommon, "DBConnector"), \
            patch.object(bgpcfgd.managers_bgp.swsscommon, "Table") as table:
        table.return_value.get.return_value = (True, [])
        m = constructor(request.param, peer_type="dynamic")
        m.table_name = "BGP_PEER_RANGE"
        m.check_neig_meta = False
        yield m, table.return_value


def make_dependencies_ready(m):
    common = bgpcfgd.managers_bgp.swsscommon
    metadata = dict(m.directory.get("CONFIG_DB", common.CFG_DEVICE_METADATA_TABLE_NAME, "localhost"))
    metadata.update(type="ToRRouter", deployment_id="1")
    m.directory.put("CONFIG_DB", common.CFG_DEVICE_METADATA_TABLE_NAME, "localhost", metadata)
    m.directory.put("CONFIG_DB", common.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME, "tsa_enabled", "false")
    m.directory.put("CONFIG_DB", common.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME, "idf_isolation_state", "unisolated")
    m.directory.put("CONFIG_DB", common.CFG_LOOPBACK_INTERFACE_TABLE_NAME, "Loopback0", {})
    assert m.directory.available_deps(m.deps)


def peer_data(name="BGPSLBPassive"):
    return {"name": name, "peer_asn": "65200", "src_address": "10.250.0.1",
            "ip_range": "10.250.0.0/27"}


@pytest.mark.parametrize("key", [
    None, "default|", "default|peer group", "VnetA|peer;show", "vnet;show|Peer",
])
@pytest.mark.parametrize("op", ["SET", "DEL"])
def test_upstream_dynamic_key_validation_is_preserved(dynamic_manager, key, op):
    m, state_table = dynamic_manager
    make_dependencies_ready(m)
    peers = m.peers.copy()
    with patch("bgpcfgd.managers_bgp.log_err") as log_err:
        m.handler(key, op, peer_data())
        log_err.assert_called_once()
    assert m.peers == peers
    assert m.set_queue == []
    m.cfg_mgr.push.assert_not_called()
    assert not state_table.mock_calls


@pytest.mark.parametrize("newline", ["\n", "\r", "\r\n"], ids=["LF", "CR", "CRLF"])
@pytest.mark.parametrize("field", ["name", "key", "qualified-key", "vrf"])
@pytest.mark.parametrize("entry,existing,admin_status", [
    ("direct", False, None), ("direct", True, None),
    ("direct", True, "up"), ("direct", True, "down"),
    ("ready", False, None), ("ready", True, "down"),
    ("missing", False, None), ("missing", True, "up"),
    ("replay", False, None), ("replay", True, "down"),
])
def test_invalid_dynamic_event_has_no_side_effects(
        dynamic_manager, newline, field, entry, existing, admin_status):
    m, state_table = dynamic_manager
    key = "BGPSLBPassive"
    data = peer_data()
    if field == "name":
        data["name"] += newline + "SECOND LINE"
    elif field == "key":
        key += newline
    elif field == "qualified-key":
        key = "VnetA|" + key + newline + "SECOND LINE"
    else:
        key = "VnetA" + newline + "|" + key
    if admin_status is not None:
        data["admin_status"] = admin_status
    if entry in ("ready", "replay"):
        make_dependencies_ready(m)
    else:
        assert not m.directory.available_deps(m.deps)
    if existing:
        vrf, nbr = m.split_key(key)
        m.peers.add((vrf, nbr))
        m.directory.put(m.db_name, m.table_name, vrf + "|" + nbr, peer_data())
    if entry == "replay":
        m.set_queue.append((key, data))
    peers = m.peers.copy()
    directory = {slot: deepcopy(values) for slot, values in m.directory.data.items()}
    original = deepcopy(data)
    initialized = m.post_dependencies_init_complete
    loopbacks = m.loopbacks[:]
    m.cfg_mgr.reset_mock()
    state_table.reset_mock()
    with patch.object(m, "add_peer", wraps=m.add_peer) as add, \
            patch.object(m, "update_peer", wraps=m.update_peer) as update, \
            patch.object(m, "update_state_db", wraps=m.update_state_db) as state, \
            patch.object(m.directory, "put", wraps=m.directory.put) as put, \
            patch("bgpcfgd.managers_bgp.run_command") as command, \
            patch("bgpcfgd.managers_bgp.log_err") as log_err:
        if entry == "direct":
            assert m.set_handler(key, data) is True
        elif entry == "replay":
            m.on_deps_change()
            m.on_deps_change()
        else:
            m.handler(key, "SET", data)
            m.on_deps_change()
        log_err.assert_called_once_with(
            "BGP_PEER_RANGE {} must not contain line breaks".format(
                "name" if field == "name" else "key"))
        for mocked in (add, update, state, put, command, m.cfg_mgr.push):
            mocked.assert_not_called()
    assert not state_table.mock_calls
    assert m.peers == peers
    assert m.directory.data == directory
    assert data == original
    assert m.post_dependencies_init_complete == initialized
    assert m.loopbacks == loopbacks
    assert m.set_queue == []


@pytest.mark.parametrize("newline", ["\n", "\r", "\r\n"], ids=["LF", "CR", "CRLF"])
@pytest.mark.parametrize("key_format", ["Peer{}", "VnetA|Peer{}", "Vnet{}|Peer"])
@pytest.mark.parametrize("entry", ["direct", "handler"])
def test_invalid_dynamic_delete_preserves_existing_peer(dynamic_manager, newline, key_format, entry):
    m, state_table = dynamic_manager
    key = key_format.format(newline)
    vrf, nbr = m.split_key(key)
    m.peers.add((vrf, nbr))
    data = peer_data(nbr)
    m.directory.put(m.db_name, m.table_name, vrf + "|" + nbr, data)
    m.cfg_mgr.reset_mock()
    with patch("bgpcfgd.managers_bgp.log_err") as log_err:
        if entry == "direct":
            m.del_handler(key)
        else:
            m.handler(key, "DEL", {})
        log_err.assert_called_once_with("BGP_PEER_RANGE key must not contain line breaks")
    m.cfg_mgr.push.assert_not_called()
    assert not state_table.mock_calls
    assert (vrf, nbr) in m.peers
    assert m.directory.get(m.db_name, m.table_name, vrf + "|" + nbr) == data
    assert m.set_queue == []


@pytest.mark.parametrize("newline", ["\n", "\r", "\r\n"], ids=["LF", "CR", "CRLF"])
def test_stale_dynamic_name_after_valid_creation(dynamic_manager, newline):
    m, state_table = dynamic_manager
    make_dependencies_ready(m)
    data = peer_data()
    key = data["name"]
    stale = dict(data, name=key + newline, admin_status="down")
    m.set_queue.append((key, stale))
    assert m.set_handler(key, data) is True
    assert ("default", key) in m.peers
    m.cfg_mgr.reset_mock()
    state_table.reset_mock()
    with patch("bgpcfgd.managers_bgp.log_err") as log_err:
        m.on_deps_change()
        m.on_deps_change()
        log_err.assert_called_once_with("BGP_PEER_RANGE name must not contain line breaks")
    assert m.set_queue == []
    assert m.directory.get(m.db_name, m.table_name, "default|" + key) == data
    m.cfg_mgr.push.assert_not_called()
    assert not state_table.mock_calls


def test_invalid_dynamic_replay_does_not_block_retryable_peer(dynamic_manager):
    m, _ = dynamic_manager
    make_dependencies_ready(m)
    data = dict(peer_data(), local_addr="40.40.40.40")
    key = data["name"]
    m.handler(key, "SET", data)
    assert m.set_queue == [(key, data)]
    m.set_queue.insert(0, (key, dict(data, name=key + "\n")))
    with patch("bgpcfgd.managers_bgp.log_err") as log_err:
        m.on_deps_change()
        assert m.set_queue == [(key, data)]
        m.cfg_mgr.push.assert_not_called()
        m.directory.put("LOCAL", "local_addresses", "Ethernet4|40.40.40.40",
                        {"interface": "Ethernet4", "prefixlen": "24"})
        log_err.assert_called_once_with("BGP_PEER_RANGE name must not contain line breaks")
    assert m.set_queue == []
    assert ("default", key) in m.peers
    m.cfg_mgr.push.assert_called()


@pytest.mark.parametrize("key", ["BGPSLBPassive", "VnetA|BGPSLBPassive", "Vrf-blue|BGPSLBPassive"])
@pytest.mark.parametrize("entry", ["direct", "ready", "missing"])
@pytest.mark.parametrize("address,prefix", [
    ("10.250.0.1", "10.250.0.0/27"), ("fc00:20::1", "fc00:20::/64")])
def test_valid_dynamic_add_update_delete(dynamic_manager, key, entry, address, prefix):
    m, state_table = dynamic_manager
    vrf, nbr = m.split_key(key)
    data = dict(peer_data(nbr), src_address=address, ip_range=prefix)
    if entry == "ready":
        make_dependencies_ready(m)
    with patch("bgpcfgd.managers_bgp.log_err") as log_err:
        if entry == "direct":
            assert m.set_handler(key, data) is True
        else:
            m.handler(key, "SET", data)
            if entry == "missing":
                assert m.set_queue == [(key, data)]
                m.cfg_mgr.push.assert_not_called()
                make_dependencies_ready(m)
        log_err.assert_not_called()
    assert m.set_queue == []
    assert (vrf, nbr) in m.peers
    state_key = nbr if vrf == "default" else key
    state_table.set.assert_called_once_with(state_key, sorted(data.items()))
    commands = "\n".join(call.args[0] for call in m.cfg_mgr.push.call_args_list)
    assert "neighbor " + nbr + " peer-group" in commands
    assert "bgp listen range " + prefix + " peer-group " + nbr in commands
    update = dict(data, admin_status="down")
    m.cfg_mgr.reset_mock()
    assert m.set_handler(key, update) is True
    commands = "\n".join(call.args[0] for call in m.cfg_mgr.push.call_args_list)
    assert "neighbor " + nbr + " shutdown" in commands
    assert m.directory.get(m.db_name, m.table_name, vrf + "|" + nbr) == update
    m.cfg_mgr.reset_mock()
    m.handler(key, "DEL", {"name": nbr + "\r\n"})
    commands = "\n".join(call.args[0] for call in m.cfg_mgr.push.call_args_list)
    assert "no bgp listen range " + prefix + " peer-group " + nbr in commands
    assert "no neighbor " + nbr in commands
    assert (vrf, nbr) not in m.peers
    state_table.delete.assert_called_once_with(state_key)
    assert m.set_queue == []
