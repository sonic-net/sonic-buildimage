"""Regressions for existing SRv6 ConfigDB translation and applied-cache state."""
import copy
from unittest.mock import MagicMock, patch

import pytest

from .test_config import mockmapping


SOURCE = ('SRV6_MY_SOURCE', 'tunnel-source', {'source-address': '2001:db8::1'})
LOCATOR = ('SRV6_MY_LOCATORS', 'LOC1', {
    'prefix': '2001:db8:100::', 'block_len': '32', 'node_len': '16',
    'func_len': '16', 'arg_len': '0', 'vrf': 'default'})
SID = ('SRV6_MY_SIDS', 'LOC1|2001:db8:100:a::/128', {'action': 'uN'})


@pytest.fixture
def srv6_daemon():
    with patch.dict('sys.modules', **mockmapping):
        from frrcfgd.frrcfgd import BGPConfigDaemon, ExtConfigDBConnector
        daemon = BGPConfigDaemon()
        daemon.table_data_cache = {}
        daemon.config_db.get_table.return_value = {}
        daemon.config_db.serialize_key.side_effect = lambda key: (
            '|'.join(key) if isinstance(key, tuple) else key)
        run = MagicMock(return_value=True)
        with patch('frrcfgd.frrcfgd.g_run_command', run):
            yield daemon, run, ExtConfigDBConnector.get_table_key


@pytest.mark.parametrize('table,key,row', [SOURCE, LOCATOR, SID])
def test_srv6_successful_set_updates_applied_cache(srv6_daemon, table, key, row):
    daemon, run, cache_key = srv6_daemon
    daemon.bgp_table_handler_common(table, key, copy.deepcopy(row))
    assert run.call_count == 1
    assert daemon.table_data_cache[cache_key(table, key)] == row


@pytest.mark.parametrize('table,key,row,delete_command', [
    (*SOURCE, 'no source-address'),
    (*LOCATOR, 'no locator LOC1'),
    (*SID, 'no sid 2001:db8:100:a::/128'),
])
def test_srv6_delete_with_applied_state_is_scoped(
        srv6_daemon, table, key, row, delete_command):
    daemon, run, cache_key = srv6_daemon
    daemon.table_data_cache[cache_key(table, key)] = copy.deepcopy(row)
    daemon.bgp_table_handler_common(table, key, None)
    run.assert_called_once()
    commands = run.call_args[0][1][2::2]
    assert delete_command in commands
    assert 'no srv6' not in commands and 'no segment-routing' not in commands
    assert cache_key(table, key) not in daemon.table_data_cache


@pytest.mark.parametrize('table,key,row', [SOURCE, LOCATOR, SID])
def test_srv6_failed_delete_preserves_applied_cache(srv6_daemon, table, key, row):
    daemon, run, cache_key = srv6_daemon
    daemon.table_data_cache[cache_key(table, key)] = copy.deepcopy(row)
    run.return_value = False
    daemon.bgp_table_handler_common(table, key, None)
    run.assert_called_once()
    assert daemon.table_data_cache[cache_key(table, key)] == row


@pytest.mark.parametrize('table,key,row,update', [
    (*SOURCE, {'source-address': '2001:db8::2'}),
    (*LOCATOR, {**LOCATOR[2], 'func_len': '20'}),
    (*SID, {'action': 'uDT46', 'decap_vrf': 'default'}),
])
def test_srv6_failed_update_preserves_applied_cache(
        srv6_daemon, table, key, row, update):
    daemon, run, cache_key = srv6_daemon
    daemon.table_data_cache[cache_key(table, key)] = copy.deepcopy(row)
    run.return_value = False
    daemon.bgp_table_handler_common(table, key, copy.deepcopy(update))
    run.assert_called_once()
    assert daemon.table_data_cache[cache_key(table, key)] == row


def test_srv6_source_delete_without_cached_value_does_not_raise(srv6_daemon):
    daemon, run, _ = srv6_daemon
    daemon.bgp_table_handler_common('SRV6_MY_SOURCE', 'tunnel-source', None)
    run.assert_called_once()
    assert 'no source-address' in run.call_args[0][1]


def test_srv6_locator_applies_omitted_geometry_defaults(srv6_daemon):
    daemon, run, cache_key = srv6_daemon
    row = {'prefix': '2001:db8:100::'}
    daemon.bgp_table_handler_common('SRV6_MY_LOCATORS', 'LOC1', row.copy())
    assert 'prefix 2001:db8:100::/48 block-len 32 node-len 16 func-bits 16' in run.call_args[0][1]
    assert daemon.table_data_cache[cache_key('SRV6_MY_LOCATORS', 'LOC1')] == row


def test_srv6_locator_geometry_reset_uses_native_defaults(srv6_daemon):
    daemon, run, cache_key = srv6_daemon
    daemon.table_data_cache[cache_key('SRV6_MY_LOCATORS', 'LOC1')] = {
        **LOCATOR[2], 'func_len': '20'}
    row = {'prefix': '2001:db8:100::'}
    daemon.bgp_table_handler_common('SRV6_MY_LOCATORS', 'LOC1', row.copy())
    assert 'prefix 2001:db8:100::/48 block-len 32 node-len 16 func-bits 16' in run.call_args[0][1]
    assert daemon.table_data_cache[cache_key('SRV6_MY_LOCATORS', 'LOC1')] == row


@pytest.mark.parametrize('unsupported', [{'arg_len': '16'}, {'vrf': 'Vrf2'}])
def test_srv6_locator_cannot_ack_unsupported_native_geometry(srv6_daemon, unsupported):
    daemon, run, cache_key = srv6_daemon
    table, key, old = LOCATOR
    daemon.table_data_cache[cache_key(table, key)] = old.copy()
    daemon.bgp_table_handler_common(table, key, {**old, **unsupported})
    run.assert_not_called()
    assert daemon.table_data_cache[cache_key(table, key)] == old


def test_srv6_policy_only_create_and_delete_do_not_mutate_static_sid(srv6_daemon):
    daemon, run, cache_key = srv6_daemon
    table, key = SID[:2]
    row = {'decap_dscp_mode': 'pipe'}
    daemon.bgp_table_handler_common(table, key, row.copy())
    assert daemon.table_data_cache[cache_key(table, key)] == row
    daemon.bgp_table_handler_common(table, key, None)
    run.assert_not_called()
    assert cache_key(table, key) not in daemon.table_data_cache


def test_srv6_action_to_policy_withdraws_static_sid_and_retains_policy(srv6_daemon):
    daemon, run, cache_key = srv6_daemon
    table, key = SID[:2]
    daemon.table_data_cache[cache_key(table, key)] = {
        'action': 'uN', 'decap_dscp_mode': 'pipe'}
    row = {'decap_dscp_mode': 'pipe'}
    daemon.bgp_table_handler_common(table, key, row.copy())
    assert 'no sid 2001:db8:100:a::/128' in run.call_args[0][1]
    assert daemon.table_data_cache[cache_key(table, key)] == row
    run.reset_mock()
    restored = {**row, 'action': 'uN'}
    daemon.bgp_table_handler_common(table, key, restored.copy())
    assert 'sid 2001:db8:100:a::/128 locator LOC1 behavior uN' in run.call_args[0][1]
    assert daemon.table_data_cache[cache_key(table, key)] == restored


def test_srv6_udt_uses_default_decap_vrf(srv6_daemon):
    daemon, run, cache_key = srv6_daemon
    table, key = SID[:2]
    row = {'action': 'uDT46'}
    daemon.bgp_table_handler_common(table, key, row.copy())
    assert 'sid 2001:db8:100:a::/128 locator LOC1 behavior uDT46 vrf default' in run.call_args[0][1]
    assert daemon.table_data_cache[cache_key(table, key)] == row


@pytest.mark.parametrize('action', ['', 'unsupported', 'None'])
def test_srv6_invalid_action_does_not_withdraw_applied_sid(srv6_daemon, action):
    daemon, run, cache_key = srv6_daemon
    table, key, old = SID
    daemon.table_data_cache[cache_key(table, key)] = old.copy()
    daemon.bgp_table_handler_common(table, key, {'action': action})
    run.assert_not_called()
    assert daemon.table_data_cache[cache_key(table, key)] == old


@pytest.mark.parametrize('old_action,owned', [
    ('uN', True), ('uDT46', True), ('uDT4', True), ('uDT6', True),
    ('invalid', False), ('None', False), ('', False), ('uA', False),
])
@pytest.mark.parametrize('deleted', [True, False])
def test_srv6_only_supported_old_action_authorizes_withdrawal(
        srv6_daemon, old_action, owned, deleted):
    daemon, run, cache_key = srv6_daemon
    table, key = SID[:2]
    daemon.table_data_cache[cache_key(table, key)] = {
        'action': old_action, 'decap_dscp_mode': 'pipe'}
    policy = {'decap_dscp_mode': 'pipe'}
    daemon.bgp_table_handler_common(table, key, None if deleted else policy.copy())
    if owned:
        run.assert_called_once()
        assert 'no sid 2001:db8:100:a::/128' in run.call_args[0][1]
    else:
        run.assert_not_called()
    if deleted:
        assert cache_key(table, key) not in daemon.table_data_cache
    else:
        assert daemon.table_data_cache[cache_key(table, key)] == policy


@pytest.mark.parametrize('table,key,row', [SOURCE, LOCATOR, SID])
def test_srv6_failed_set_does_not_ack_applied_cache(srv6_daemon, table, key, row):
    daemon, run, cache_key = srv6_daemon
    run.return_value = False
    daemon.bgp_table_handler_common(table, key, copy.deepcopy(row))
    run.assert_called_once()
    assert cache_key(table, key) not in daemon.table_data_cache


def apply_sid_operation(daemon, operation):
    rows = {'set': {'action': 'uN', 'decap_dscp_mode': 'pipe'},
            'delete': None, 'policy': {'decap_dscp_mode': 'pipe'}}
    daemon.bgp_table_handler_common(SID[0], SID[1], rows[operation])


@pytest.mark.parametrize('operation', ['set', 'delete', 'policy'])
@pytest.mark.parametrize('location', ['desired', 'cache'])
@pytest.mark.parametrize('other_action', ['uN', 'uA'])
def test_srv6_native_sid_admission_blocks_other_actions(
        srv6_daemon, operation, location, other_action):
    daemon, run, cache_key = srv6_daemon
    table, key = SID[:2]
    daemon.table_data_cache[cache_key(table, key)] = {
        'action': 'uN', 'decap_dscp_mode': 'uniform'}
    foreign_key = 'LOC2|2001:db8:100:a::/128'
    foreign = {'action': other_action}
    if location == 'desired':
        daemon.config_db.get_table.return_value = {foreign_key: foreign}
    else:
        daemon.table_data_cache[cache_key(table, foreign_key)] = foreign
    before = copy.deepcopy(daemon.table_data_cache)
    apply_sid_operation(daemon, operation)
    run.assert_not_called()
    assert daemon.table_data_cache == before


@pytest.mark.parametrize('operation', ['set', 'delete', 'policy'])
@pytest.mark.parametrize('location', ['desired', 'cache'])
def test_srv6_native_sid_admission_allows_policy_only_shared_prefix(
        srv6_daemon, operation, location):
    daemon, run, cache_key = srv6_daemon
    table, key = SID[:2]
    daemon.table_data_cache[cache_key(table, key)] = {
        'action': 'uN', 'decap_dscp_mode': 'uniform'}
    foreign_key = 'LOC2|2001:db8:100:a::/128'
    foreign = {'decap_dscp_mode': 'pipe'}
    if location == 'desired':
        daemon.config_db.get_table.return_value = {foreign_key: foreign.copy()}
    else:
        daemon.table_data_cache[cache_key(table, foreign_key)] = foreign.copy()
    apply_sid_operation(daemon, operation)
    run.assert_called_once()
    if operation == 'delete':
        assert cache_key(table, key) not in daemon.table_data_cache
    elif operation == 'policy':
        assert daemon.table_data_cache[cache_key(table, key)] == {'decap_dscp_mode': 'pipe'}
    else:
        assert daemon.table_data_cache[cache_key(table, key)] == {
            'action': 'uN', 'decap_dscp_mode': 'pipe'}
    if location == 'cache':
        assert daemon.table_data_cache[cache_key(table, foreign_key)] == foreign


@pytest.mark.parametrize('operation', ['set', 'delete', 'policy'])
@pytest.mark.parametrize('location', ['desired-tuple', 'cache'])
def test_srv6_native_sid_admission_normalizes_ipv6_prefix_aliases(
        srv6_daemon, operation, location):
    daemon, run, cache_key = srv6_daemon
    table, key = SID[:2]
    daemon.table_data_cache[cache_key(table, key)] = {'action': 'uN'}
    alias = '2001:0DB8:0100:000A:0:0:0:0/128'
    if location == 'desired-tuple':
        daemon.config_db.get_table.return_value = {('LOC2', alias): {'action': 'uN'}}
    else:
        daemon.table_data_cache[cache_key(table, 'LOC2|' + alias)] = {'action': 'uN'}
    before = copy.deepcopy(daemon.table_data_cache)
    apply_sid_operation(daemon, operation)
    run.assert_not_called()
    assert daemon.table_data_cache == before


@pytest.mark.parametrize('snapshot', [
    None, [], {'malformed-key': {'action': 'uN'}},
    {'LOC2|2001:db8:100:a::/128': None}, RuntimeError('snapshot unavailable'),
])
def test_srv6_native_sid_admission_snapshot_failure_is_closed(srv6_daemon, snapshot):
    daemon, run, cache_key = srv6_daemon
    table, key = SID[:2]
    daemon.table_data_cache[cache_key(table, key)] = {'action': 'uN'}
    if isinstance(snapshot, Exception):
        daemon.config_db.get_table.side_effect = snapshot
    else:
        daemon.config_db.get_table.return_value = snapshot
    before = copy.deepcopy(daemon.table_data_cache)
    apply_sid_operation(daemon, 'delete')
    run.assert_not_called()
    assert daemon.table_data_cache == before
