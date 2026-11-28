"""IS-IS lifecycle regressions and startup/runtime configuration parity."""
import copy
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest
from jinja2 import Environment, FileSystemLoader

from .test_config import mockmapping


@patch.dict('sys.modules', **mockmapping)
def test_isis_lifecycle():
    from frrcfgd.frrcfgd import BGPConfigDaemon, CachedDataWithOp, ExtConfigDBConnector
    daemon = object.__new__(BGPConfigDaemon)
    daemon.isis_interface_vrfs = {}
    daemon.isis_interface_bindings = {}
    daemon.isis_pending_bindings = {}
    daemon.config_db = MagicMock()
    daemon.config_db.get_entry.side_effect = lambda table, key: (
        {'enabled': 'true'} if table == 'ISIS_GLOBALS' else {})
    daemon.table_data_cache = {}
    run = MagicMock(return_value=True)
    daemon._BGPConfigDaemon__run_command = run

    def apply(table, key, row, old=None):
        old = old or {}
        daemon.table_data_cache = {ExtConfigDBConnector.get_table_key(table, key): old}
        data = {}
        for field in old.keys() | (row or {}).keys():
            op = CachedDataWithOp.OP_DELETE if field not in (row or {}) else CachedDataWithOp.OP_UPDATE
            data[field] = CachedDataWithOp((row or {}).get(field, old.get(field)), op)
        assert daemon._apply_isis(table, key, row is None, data)
        assert all(item.status == CachedDataWithOp.STAT_SUCC for item in data.values())
        return run.call_args[0][1][2::2]

    assert 'router isis default' in apply('ISIS_GLOBALS', 'default', {'enabled': 'true'})
    commands = apply('ISIS_GLOBALS', 'Vrf2', {'enabled': 'true', 'dynamic_hostname': 'false'})
    assert 'router isis Vrf2 vrf Vrf2' in commands
    assert 'no hostname dynamic' in commands
    assert 'no hostname' not in commands
    assert 'no router isis Vrf2 vrf Vrf2' in apply('ISIS_GLOBALS', 'Vrf2', None, {'enabled': 'true'})
    assert 'no router isis default' in apply('ISIS_GLOBALS', 'default', {'enabled': 'false'})
    old_net, new_net = '49.0001.0000.0000.0001.00', '49.0001.0000.0000.0002.00'
    commands = apply('ISIS_GLOBALS', 'default', {'enabled': 'true', 'net_title': new_net},
                     {'enabled': 'true', 'net_title': old_net})
    assert commands.index('no net ' + old_net) < commands.index('net ' + new_net)
    commands = apply('ISIS_INTERFACE', 'Ethernet0', {'enabled': 'true', 'passive': 'false'})
    assert 'ip router isis default' in commands
    assert 'no isis passive' in commands
    assert 'no isis network point-to-point' in commands
    assert 'no ipv6 router isis default' in apply('ISIS_INTERFACE', 'Ethernet0',
                                                  {'enabled': 'true', 'ipv6_enabled': 'false'})
    assert 'no ip router isis default' in apply('ISIS_INTERFACE', 'Ethernet0', {'enabled': 'false'})
    assert 'no topology ipv6-unicast' in apply('ISIS_GLOBALS', 'default',
                                               {'enabled': 'true', 'ipv6_multi_topology': 'false'})
    commands = apply('ISIS_GLOBALS_TIMERS', 'default', None,
                     {'lsp_mtu': '1400', 'lsp_refresh_interval': '800', 'lsp_lifetime': '1500'})
    assert all(command in commands for command in
               ('no lsp-mtu', 'no lsp-refresh-interval', 'no max-lsp-lifetime'))
    data = {'enabled': CachedDataWithOp('true', CachedDataWithOp.OP_ADD)}
    run.return_value = False
    assert not daemon._apply_isis('ISIS_GLOBALS', 'default', False, data)
    assert data['enabled'].status == CachedDataWithOp.STAT_FAIL


def test_isis_template():
    templates = Path(__file__).resolve().parents[1] / 'templates'
    template = Environment(loader=FileSystemLoader(str(templates / 'isisd'))).get_template('isisd.conf.j2')
    config = {'ISIS_GLOBALS': {'Vrf2': {'enabled': 'true', 'dynamic_hostname': 'false',
                                        'ipv6_multi_topology': 'false'}},
              'ISIS_INTERFACE': {'Ethernet0': {'enabled': 'true', 'ipv6_enabled': 'false'}},
              'INTERFACE': {'Ethernet0': {'vrf_name': 'Vrf2'}}}
    output = template.render(**config)
    assert 'router isis Vrf2 vrf Vrf2' in output
    assert 'no hostname dynamic' in output
    assert 'no topology ipv6-unicast' in output
    assert 'ip router isis Vrf2' in output
    assert 'ipv6 router isis' not in output
    assert 'isis network point-to-point' not in output
    for globals_config in ({}, {'Vrf2': {'enabled': 'false'}}):
        disabled = copy.deepcopy(config)
        disabled['ISIS_GLOBALS'] = globals_config
        output = template.render(**disabled)
        assert 'router isis' not in output
    config['ISIS_INTERFACE']['Ethernet0']['enabled'] = 'false'
    assert 'ip router isis' not in template.render(**config)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command', return_value=True)
def test_isis_event_dispatch_and_cache(run):
    from frrcfgd.frrcfgd import BGPConfigDaemon, ExtConfigDBConnector
    daemon = BGPConfigDaemon()
    daemon.bgp_table_handler_common('ISIS_GLOBALS', 'default', {'enabled': 'true'})
    run.assert_called_once()
    assert 'router isis default' in run.call_args[0][1]
    key = ExtConfigDBConnector.get_table_key('ISIS_GLOBALS', 'default')
    assert daemon.table_data_cache[key]['enabled'] == 'true'
    daemon.bgp_table_handler_common('ISIS_GLOBALS', 'default', None)
    assert 'no router isis default' in run.call_args[0][1]
    assert key not in daemon.table_data_cache


@patch.dict('sys.modules', **mockmapping)
def test_isis_timer_order_and_deleted_parent():
    from frrcfgd.frrcfgd import BGPConfigDaemon, CachedDataWithOp, ExtConfigDBConnector
    daemon = object.__new__(BGPConfigDaemon)
    daemon.isis_interface_vrfs = {}
    daemon.isis_interface_bindings = {}
    daemon.isis_pending_bindings = {}
    daemon.config_db = MagicMock()
    daemon.config_db.get_entry.return_value = {'enabled': 'true'}
    daemon._BGPConfigDaemon__run_command = MagicMock(return_value=True)
    key = ExtConfigDBConnector.get_table_key('ISIS_GLOBALS_TIMERS', 'default')
    daemon.table_data_cache = {key: {'lsp_refresh_interval': '100', 'lsp_lifetime': '500'}}
    data = {name: CachedDataWithOp(value, CachedDataWithOp.OP_DELETE)
            for name, value in daemon.table_data_cache[key].items()}
    assert daemon._apply_isis('ISIS_GLOBALS_TIMERS', 'default', True, data)
    commands = daemon._BGPConfigDaemon__run_command.call_args[0][1]
    assert commands.index('no max-lsp-lifetime') < commands.index('no lsp-refresh-interval')
    daemon._BGPConfigDaemon__run_command.reset_mock()
    daemon.config_db.get_entry.return_value = {}
    assert daemon._apply_isis('ISIS_GLOBALS_TIMERS', 'default', True, data)
    daemon._BGPConfigDaemon__run_command.assert_not_called()


@patch.dict('sys.modules', **mockmapping)
def test_isis_vrf_rebinding_and_deletion():
    """A->B->delete must remove the applied instance, never a reread VRF."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    for table, name in (('INTERFACE', 'Ethernet0'), ('PORTCHANNEL_INTERFACE', 'PortChannel1'),
                        ('VLAN_INTERFACE', 'Vlan100'), ('LOOPBACK_INTERFACE', 'Loopback0'),
                        ('VLAN_SUB_INTERFACE', 'Ethernet0.100')):
        daemon = BGPConfigDaemon()
        daemon.table_data_cache = {}
        row = {'enabled': 'true', 'passive': 'true', 'metric': '30',
               'circuit_type': 'p2p', 'ipv6_enabled': 'false'}
        globals_rows = {'VrfA': {'enabled': 'true'}, 'VrfB': {'enabled': 'true'}}
        daemon.config_db.get_entry.side_effect = lambda tbl, key: (
            row.copy() if tbl == 'ISIS_INTERFACE' else globals_rows.get(key, {}) if tbl == 'ISIS_GLOBALS' else {})
        run = MagicMock(return_value=True)
        daemon._BGPConfigDaemon__run_command = run
        daemon.isis_routed_interface_handler(table, name, {'vrf_name': 'VrfA'})
        assert daemon.isis_interface_bindings[name] == 'VrfA'
        daemon.isis_routed_interface_handler(table, name, {'vrf_name': 'VrfB'})
        commands = run.call_args[0][1]
        assert commands.index('no ip router isis VrfA') < commands.index('ip router isis VrfB')
        assert 'no ipv6 router isis VrfA' in commands
        assert 'no ipv6 router isis VrfB' in commands
        assert 'isis passive' in commands and 'isis metric 30' in commands
        assert 'isis network point-to-point' in commands
        assert daemon.isis_interface_bindings[name] == 'VrfB'
        # Address add/remove events must not erase the base interface's VRF.
        run.reset_mock()
        daemon.isis_routed_interface_handler(table, (name, '10.0.0.1/24'), None)
        daemon.isis_routed_interface_handler(table, name + '|10.0.0.1/24', None)
        run.assert_not_called()
        daemon.isis_routed_interface_handler(table, name, None)
        assert 'no ip router isis VrfB' in run.call_args[0][1]
        assert 'ip router isis default' not in run.call_args[0][1]
        assert name not in daemon.isis_interface_bindings
        # ISIS row deletion arriving later must not recreate an interface/binding.
        run.reset_mock()
        daemon.bgp_table_handler_common('ISIS_INTERFACE', name, None)
        run.assert_not_called()
        # Reverse order: remove ISIS first, then the routed-interface row.
        daemon.isis_routed_interface_handler(table, name, {'vrf_name': 'VrfB'})
        assert daemon.isis_interface_bindings[name] == 'VrfB'
        daemon.bgp_table_handler_common('ISIS_INTERFACE', name, None)
        assert 'no ip router isis VrfB' in run.call_args[0][1]
        run.reset_mock()
        row.clear()
        daemon.isis_routed_interface_handler(table, name, None)
        run.assert_not_called()


@patch.dict('sys.modules', **mockmapping)
def test_isis_move_to_missing_process_then_enable_and_retry():
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    daemon.table_data_cache = {}
    row = {'enabled': 'true'}
    globals_rows = {'default': {'enabled': 'true'}}
    daemon.config_db.get_entry.side_effect = lambda table, key: (
        row.copy() if table == 'ISIS_INTERFACE' else globals_rows.get(key, {}) if table == 'ISIS_GLOBALS' else {})
    daemon.config_db.get_table.side_effect = lambda table: (
        {'Ethernet0': row.copy()} if table == 'ISIS_INTERFACE' else {})
    run = MagicMock(return_value=True)
    daemon._BGPConfigDaemon__run_command = run
    daemon.isis_routed_interface_handler('INTERFACE', 'Ethernet0', {})
    daemon.isis_routed_interface_handler('INTERFACE', 'Ethernet0', {'vrf_name': 'VrfB'})
    assert 'no ip router isis default' in run.call_args[0][1]
    assert 'ip router isis VrfB' not in run.call_args[0][1]
    assert 'Ethernet0' not in daemon.isis_interface_bindings
    globals_rows['VrfB'] = {'enabled': 'true'}
    daemon.bgp_table_handler_common('ISIS_GLOBALS', 'VrfB', globals_rows['VrfB'].copy())
    assert daemon.isis_interface_bindings['Ethernet0'] == 'VrfB'
    assert 'ipv6 router isis VrfB' in run.call_args[0][1]
    # Failed command batches retain the last successful binding for retry/cleanup.
    run.return_value = False
    daemon.isis_routed_interface_handler('INTERFACE', 'Ethernet0', {})
    assert daemon.isis_interface_bindings['Ethernet0'] == 'VrfB'
    run.return_value = True
    daemon.isis_routed_interface_handler('INTERFACE', 'Ethernet0', {})
    assert 'no ip router isis VrfB' in run.call_args[0][1]
    assert daemon.isis_interface_bindings['Ethernet0'] == 'default'
    # Disabling the parent removes bindings without deleting the desired rows.
    globals_rows['default'] = {'enabled': 'false'}
    daemon.bgp_table_handler_common('ISIS_GLOBALS', 'default', globals_rows['default'].copy())
    assert 'Ethernet0' not in daemon.isis_interface_bindings
    assert row == {'enabled': 'true'}


@patch.dict('sys.modules', **mockmapping)
def test_isis_partial_move_failure_then_delete():
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    daemon.table_data_cache = {}
    daemon.isis_interface_bindings['Ethernet0'] = 'VrfA'
    daemon.config_db.get_entry.side_effect = lambda table, key: (
        {'enabled': 'true'} if table in ('ISIS_GLOBALS', 'ISIS_INTERFACE') else {})
    run = MagicMock(return_value=False)
    daemon._BGPConfigDaemon__run_command = run
    daemon.isis_routed_interface_handler('INTERFACE', 'Ethernet0', {'vrf_name': 'VrfB'})
    assert daemon.isis_pending_bindings['Ethernet0'] == {'VrfA', 'VrfB'}
    assert daemon.isis_interface_bindings['Ethernet0'] == 'VrfA'
    run.return_value = True
    daemon.isis_routed_interface_handler('INTERFACE', 'Ethernet0', None)
    commands = run.call_args[0][1]
    for vrf in ('VrfA', 'VrfB'):
        assert 'no ip router isis ' + vrf in commands
        assert 'no ipv6 router isis ' + vrf in commands
    assert 'Ethernet0' not in daemon.isis_interface_bindings
    assert 'Ethernet0' not in daemon.isis_pending_bindings


@patch.dict('sys.modules', **mockmapping)
def test_isis_startup_bindings_and_subscriptions():
    from frrcfgd.frrcfgd import BGPConfigDaemon
    db = MagicMock()
    tables = {'INTERFACE': {'Ethernet0': {'vrf_name': 'VrfA'},
                            ('Ethernet0', '10.0.0.1/24'): {}},
              'ISIS_INTERFACE': {'Ethernet0': {'enabled': 'true'},
                                 'Ethernet4': {'enabled': 'false'}},
              'ISIS_GLOBALS': {'VrfA': {'enabled': 'true'}}}
    db.get_table.side_effect = lambda table: copy.deepcopy(tables.get(table, {}))
    db.get_entry.side_effect = lambda table, key: copy.deepcopy(tables.get(table, {}).get(key, {}))
    db.get_table_data.return_value = {}
    with patch('frrcfgd.frrcfgd.ExtConfigDBConnector', return_value=db):
        daemon = BGPConfigDaemon()
    assert daemon.isis_interface_bindings == {'Ethernet0': 'VrfA'}
    assert daemon.isis_interface_vrfs == {'Ethernet0': 'VrfA'}
    daemon.subscribe_all()
    for table in daemon.ISIS_ROUTED_INTERFACE_TABLES:
        db.subscribe.assert_any_call(table, daemon.isis_routed_interface_handler)


@patch.dict('sys.modules', **mockmapping)
def test_isis_parent_reenable_restores_retained_timers():
    from frrcfgd.frrcfgd import BGPConfigDaemon, ExtConfigDBConnector
    daemon = BGPConfigDaemon()
    timers = {'lsp_refresh_interval': '100', 'lsp_lifetime': '500', 'lsp_mtu': '1400'}
    rows = {'ISIS_GLOBALS': {'enabled': 'true'}, 'ISIS_GLOBALS_TIMERS': timers}
    daemon.config_db.get_entry.side_effect = lambda table, key: rows.get(table, {}).copy()
    daemon.config_db.get_table.return_value = {}
    timer_key = ExtConfigDBConnector.get_table_key('ISIS_GLOBALS_TIMERS', 'default')
    daemon.table_data_cache = {timer_key: timers.copy()}
    run = MagicMock(return_value=True)
    daemon._BGPConfigDaemon__run_command = run
    daemon.bgp_table_handler_common('ISIS_GLOBALS', 'default', {'enabled': 'false'})
    run.reset_mock()
    daemon.bgp_table_handler_common('ISIS_GLOBALS', 'default', {'enabled': 'true'})
    commands = run.call_args[0][1]
    assert commands.index('lsp-refresh-interval 100') < commands.index('max-lsp-lifetime 500')
    assert 'lsp-mtu 1400' in commands


@pytest.mark.parametrize('ipv4,ipv6', [(True, True), (False, True), (True, False), (False, False)])
@patch.dict('sys.modules', **mockmapping)
def test_isis_independent_address_family_bindings(ipv4, ipv6):
    """Changing either AF must remove its old binding and preserve the other."""
    from frrcfgd.frrcfgd import BGPConfigDaemon, CachedDataWithOp
    daemon = object.__new__(BGPConfigDaemon)
    daemon.isis_interface_vrfs = {'Ethernet0': 'default'}
    daemon.isis_interface_bindings = {'Ethernet0': 'default'}
    daemon.isis_pending_bindings = {}
    daemon.table_data_cache = {}
    daemon.config_db = MagicMock()
    daemon.config_db.get_entry.return_value = {'enabled': 'true'}
    run = MagicMock(return_value=True)
    daemon._BGPConfigDaemon__run_command = run
    row = {'enabled': 'true', 'ipv4_enabled': str(ipv4).lower(),
           'ipv6_enabled': str(ipv6).lower(), 'metric': '30',
           'passive': 'true', 'circuit_type': 'p2p'}
    data = {name: CachedDataWithOp(value, CachedDataWithOp.OP_UPDATE)
            for name, value in row.items()}
    assert daemon._apply_isis('ISIS_INTERFACE', 'Ethernet0', False, data)
    commands = run.call_args[0][1][2::2]
    for enabled, command in ((ipv4, 'ip router isis default'),
                             (ipv6, 'ipv6 router isis default')):
        assert (command if enabled else 'no ' + command) in commands
        assert ('no ' + command if enabled else command) not in commands
    if ipv4 or ipv6:
        assert daemon.isis_interface_bindings == {'Ethernet0': 'default'}
        assert 'isis metric 30' in commands
    else:
        assert not daemon.isis_interface_bindings
        assert not any(command.startswith(('isis ', 'no isis ')) for command in commands)
    assert all(item.status == CachedDataWithOp.STAT_SUCC for item in data.values())


@pytest.mark.parametrize('ipv4,ipv6', [(True, True), (False, True), (True, False), (False, False)])
def test_isis_template_independent_address_families(ipv4, ipv6):
    templates = Path(__file__).resolve().parents[1] / 'templates' / 'isisd'
    template = Environment(loader=FileSystemLoader(str(templates))).get_template('isisd.conf.j2')
    row = {'enabled': 'true', 'ipv4_enabled': str(ipv4).lower(),
           'ipv6_enabled': str(ipv6).lower(), 'passive': 'true',
           'metric': '30', 'circuit_type': 'p2p'}
    output = template.render(ISIS_GLOBALS={'default': {'enabled': 'true'}},
                             ISIS_INTERFACE={'Ethernet0': row})
    commands = [line.strip() for line in output.splitlines()]
    assert ('ip router isis default' in commands) == ipv4
    assert ('ipv6 router isis default' in commands) == ipv6
    assert ('isis metric 30' in commands) == (ipv4 or ipv6)
    assert ('isis passive' in commands) == (ipv4 or ipv6)
    assert ('isis network point-to-point' in commands) == (ipv4 or ipv6)


@patch.dict('sys.modules', **mockmapping)
def test_isis_address_family_reset_restores_legacy_defaults():
    from frrcfgd.frrcfgd import BGPConfigDaemon, CachedDataWithOp
    daemon = object.__new__(BGPConfigDaemon)
    daemon.isis_interface_vrfs = {}
    daemon.isis_interface_bindings = {}
    daemon.isis_pending_bindings = {}
    daemon.table_data_cache = {}
    daemon.config_db = MagicMock()
    daemon.config_db.get_entry.return_value = {'enabled': 'true'}
    run = MagicMock(return_value=True)
    daemon._BGPConfigDaemon__run_command = run
    data = {'enabled': CachedDataWithOp('true', CachedDataWithOp.OP_UPDATE),
            'ipv4_enabled': CachedDataWithOp('false', CachedDataWithOp.OP_DELETE),
            'ipv6_enabled': CachedDataWithOp('false', CachedDataWithOp.OP_DELETE)}
    assert daemon._apply_isis('ISIS_INTERFACE', 'Ethernet0', False, data)
    commands = run.call_args[0][1][2::2]
    assert 'ip router isis default' in commands
    assert 'ipv6 router isis default' in commands
    assert daemon.isis_interface_bindings == {'Ethernet0': 'default'}


@patch.dict('sys.modules', **mockmapping)
def test_isis_startup_does_not_record_disabled_address_families():
    from frrcfgd.frrcfgd import BGPConfigDaemon
    tables = {'ISIS_GLOBALS': {'default': {'enabled': 'true'}},
              'ISIS_INTERFACE': {
                  'Ethernet0': {'enabled': 'true', 'ipv4_enabled': 'false', 'ipv6_enabled': 'false'},
                  'Ethernet4': {'enabled': 'true', 'ipv4_enabled': 'false', 'ipv6_enabled': 'true'},
                  'Ethernet8': {'enabled': 'true'}}}
    db = MagicMock()
    db.get_table.side_effect = lambda table: copy.deepcopy(tables.get(table, {}))
    db.get_entry.side_effect = lambda table, key: copy.deepcopy(tables.get(table, {}).get(key, {}))
    db.get_table_data.return_value = {}
    with patch('frrcfgd.frrcfgd.ExtConfigDBConnector', return_value=db):
        daemon = BGPConfigDaemon()
    assert daemon.isis_interface_bindings == {'Ethernet4': 'default', 'Ethernet8': 'default'}
