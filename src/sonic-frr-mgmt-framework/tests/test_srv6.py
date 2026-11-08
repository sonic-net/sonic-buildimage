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


@pytest.mark.parametrize('table,key,row', [SOURCE, LOCATOR, SID])
def test_srv6_failed_set_does_not_ack_applied_cache(srv6_daemon, table, key, row):
    daemon, run, cache_key = srv6_daemon
    run.return_value = False
    daemon.bgp_table_handler_common(table, key, copy.deepcopy(row))
    run.assert_called_once()
    assert cache_key(table, key) not in daemon.table_data_cache
