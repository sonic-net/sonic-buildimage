import copy
import ipaddress
from pathlib import Path
from unittest.mock import MagicMock, patch

import jinja2
import pytest

from frrcfgd.traffic_shift import TrafficShift


@pytest.fixture
def tsa():
    tables = {
        'BGP_GLOBALS': {'default': {'local_asn': '65000'}},
        'BGP_PEER_GROUP': {('default', 'PG'): {'asn': '65001'}},
        'BGP_NEIGHBOR': {('default', 'Ethernet0'): {'peer_group_name': 'PG'}},
        'BGP_PEER_GROUP_AF': {
            ('default', 'PG', 'ipv4_unicast'): {'route_map_out': ['OUT4']},
            ('default', 'PG', 'ipv6_unicast'): {'route_map_out': ['OUT6']}},
        'BGP_NEIGHBOR_AF': {('default', 'Ethernet0', 'ipv4_unicast'): {'admin_status': 'true'}},
        'LOOPBACK_INTERFACE': {
            'Loopback0': {},
            ('Loopback0', '10.0.0.1/32'): {},
            ('Loopback0', '2001:db8::1/128'): {}}
    }
    db = MagicMock()
    db.get_table.side_effect = lambda table: copy.deepcopy(tables.get(table, {}))
    db.get_entry.return_value = {}
    run = MagicMock(return_value=True)
    shift = TrafficShift(db, run, lambda table, key, entry: key)
    return shift, tables, run


def test_enable_disable_and_inheritance(tsa):
    shift, tables, run = tsa
    assert shift.apply(True)
    commands = run.call_args.args[0]
    assert 'ip prefix-list FRRCFGD_TSA_default_ip permit 10.0.0.1/32' in commands
    assert 'ipv6 prefix-list FRRCFGD_TSA_default_ipv6 permit 2001:db8::1/128' in commands
    assert 'call OUT4' in commands and 'call OUT6' in commands
    assert 'neighbor PG route-map FRRCFGD_TSA_default_ipv4_OUT4 out' in commands
    assert not any(c.startswith('neighbor Ethernet0 route-map') for c in commands)
    assert shift.apply(False)
    commands = run.call_args.args[0]
    assert 'neighbor PG route-map OUT4 out' in commands
    assert 'neighbor PG route-map OUT6 out' in commands
    assert 'no route-map FRRCFGD_TSA_default_ipv4_OUT4' in commands
    assert 'no ip prefix-list FRRCFGD_TSA_default_ip' in commands
    assert not shift.enabled and not shift.route_maps and not shift.prefixes


def test_policy_and_loopback_updates_while_isolated(tsa):
    shift, tables, run = tsa
    assert shift.apply(True)
    tables['BGP_PEER_GROUP_AF'][('default', 'PG', 'ipv4_unicast')]['route_map_out'] = ['NEW_OUT']
    del tables['LOOPBACK_INTERFACE'][('Loopback0', '10.0.0.1/32')]
    assert shift.apply(True)
    commands = run.call_args.args[0]
    assert 'call NEW_OUT' in commands
    assert 'no route-map FRRCFGD_TSA_default_ipv4_OUT4' in commands
    assert 'no ip prefix-list FRRCFGD_TSA_default_ip permit 10.0.0.1/32' in commands
    assert shift.apply(False)
    assert 'neighbor PG route-map NEW_OUT out' in run.call_args.args[0]


def test_neighbor_without_policy_and_af_deletion(tsa):
    shift, tables, run = tsa
    key = ('default', '10.0.0.2', 'ipv4_unicast')
    tables['BGP_NEIGHBOR'][key[:2]] = {'asn': '65001'}
    tables['BGP_NEIGHBOR_AF'][key] = {'admin_status': 'true'}
    assert shift.apply(True)
    assert 'neighbor 10.0.0.2 route-map FRRCFGD_TSA_default_ipv4 out' in run.call_args.args[0]
    del tables['BGP_NEIGHBOR_AF'][key]
    assert shift.apply(True)
    assert 'no neighbor 10.0.0.2 route-map FRRCFGD_TSA_default_ipv4 out' in run.call_args.args[0]
    assert 'no route-map FRRCFGD_TSA_default_ipv4' in run.call_args.args[0]


def test_command_failure_does_not_acknowledge_state(tsa):
    shift, tables, run = tsa
    run.return_value = False
    assert not shift.apply(True)
    assert not shift.enabled


def test_restart_reconciles_flag_changed_while_daemon_was_down(tsa):
    shift, tables, run = tsa
    shift.recover('''route-map FRRCFGD_TSA_default_ipv4_OLD_OUT permit 10
ip prefix-list FRRCFGD_TSA_default_ip seq 5 permit 10.0.0.1/32
''')
    assert shift.enabled
    assert shift.apply(False)
    commands = run.call_args.args[0]
    assert 'neighbor PG route-map OUT4 out' in commands
    assert 'no route-map FRRCFGD_TSA_default_ipv4_OLD_OUT' in commands
    assert 'no ip prefix-list FRRCFGD_TSA_default_ip' in commands
    assert not shift.enabled


def test_loopback_exemptions_are_vrf_scoped(tsa):
    shift, tables, run = tsa
    tables['LOOPBACK_INTERFACE']['Loopback1'] = {'vrf_name': 'Vrf_red'}
    tables['LOOPBACK_INTERFACE'][('Loopback1', '10.1.0.1/32')] = {}
    tables['BGP_GLOBALS']['Vrf_red'] = {'local_asn': '65002'}
    tables['BGP_PEER_GROUP_AF'][('Vrf_red', 'PG', 'ipv4_unicast')] = {'route_map_out': ['OUT4']}
    assert shift.apply(True)
    commands = run.call_args.args[0]
    assert 'ip prefix-list FRRCFGD_TSA_Vrf_red_ip permit 10.1.0.1/32' in commands
    assert 'ip prefix-list FRRCFGD_TSA_default_ip permit 10.1.0.1/32' not in commands
    assert 'route-map FRRCFGD_TSA_Vrf_red_ipv4_OUT4 permit 10' in commands
    assert 'match ip address prefix-list FRRCFGD_TSA_Vrf_red_ip' in commands


def test_no_loopbacks_still_installs_a_denying_policy(tsa):
    shift, tables, run = tsa
    tables['LOOPBACK_INTERFACE'] = {}
    assert shift.apply(True)
    commands = run.call_args.args[0]
    assert 'match ip address prefix-list FRRCFGD_TSA_default_ip' in commands
    assert 'neighbor PG route-map FRRCFGD_TSA_default_ipv4_OUT4 out' in commands


def test_reserved_names_and_invalid_inputs(tsa):
    shift, tables, run = tsa
    tables['ROUTE_MAP'] = {('FRRCFGD_TSA_default_ipv4_OUT4', '10'): {}}
    assert not shift.apply(True)
    run.assert_not_called()
    del tables['ROUTE_MAP']
    shift.validate_input = lambda *args: None
    assert not shift.apply(True)
    run.assert_not_called()


def test_startup_template_matches_runtime(tsa):
    shift, tables, run = tsa
    tables['BGP_DEVICE_GLOBAL'] = {'STATE': {'tsa_enabled': 'true'}}
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(
        str(Path(__file__).parents[1] / 'templates' / 'bgpd')))
    env.filters['ip_network'] = lambda value: str(ipaddress.ip_network(value, strict=False).network_address)
    env.filters['prefixlen'] = lambda value: ipaddress.ip_network(value, strict=False).prefixlen
    rendered = env.get_template('bgpd.conf.db.tsa.j2').render(**tables)
    assert 'route-map FRRCFGD_TSA_default_ipv4_OUT4 permit 10' in rendered
    assert 'ip prefix-list FRRCFGD_TSA_default_ip permit 10.0.0.1/32' in rendered
    assert 'ipv6 prefix-list FRRCFGD_TSA_default_ipv6 permit 2001:db8::1/128' in rendered
    assert 'call OUT4' in rendered
    assert 'route-map FRRCFGD_TSA_default_ipv4 permit 10' not in rendered
    rendered = env.get_template('bgpd.conf.db.nbr_af.j2').render(
        **tables, vrf='default', nbr_name='PG', af='ipv4_unicast', af_str='ipv4',
        n_af_val=tables['BGP_PEER_GROUP_AF'][('default', 'PG', 'ipv4_unicast')])
    assert 'neighbor PG route-map FRRCFGD_TSA_default_ipv4_OUT4 out' in rendered
    tables['BGP_DEVICE_GLOBAL']['STATE']['tsa_enabled'] = 'false'
    rendered = env.get_template('bgpd.conf.db.nbr_af.j2').render(
        **tables, vrf='default', nbr_name='PG', af='ipv4_unicast', af_str='ipv4',
        n_af_val=tables['BGP_PEER_GROUP_AF'][('default', 'PG', 'ipv4_unicast')])
    assert 'neighbor PG route-map OUT4 out' in rendered
    assert 'FRRCFGD_TSA' not in rendered


def test_device_global_handler_and_live_af_update():
    from tests.test_config import mockmapping
    with patch.dict('sys.modules', **mockmapping), patch('frrcfgd.frrcfgd.g_run_command') as run:
        from frrcfgd.frrcfgd import BGPConfigDaemon
        daemon = BGPConfigDaemon()
        daemon.traffic_shift = MagicMock(enabled=False)
        daemon.device_global_handler('BGP_DEVICE_GLOBAL', 'CONFED', {'tsa_enabled': 'true'})
        daemon.device_global_handler('BGP_DEVICE_GLOBAL', 'STATE', {'tsa_enabled': 'invalid'})
        daemon.traffic_shift.apply.assert_not_called()
        daemon.device_global_handler('BGP_DEVICE_GLOBAL', 'STATE', {'tsa_enabled': 'true'})
        daemon.traffic_shift.apply.assert_called_with(True)
        daemon.traffic_shift.enabled = True
        daemon.device_global_handler('BGP_DEVICE_GLOBAL', 'STATE', None)
        daemon.traffic_shift.apply.assert_called_with(False)
        daemon.bgp_asn['default'] = '65000'
        daemon.traffic_shift.apply.reset_mock()
        daemon.bgp_table_handler_common('BGP_PEER_GROUP_AF', 'default|PG|ipv4_unicast',
                                       {'route_map_out': ['NEW_OUT'], 'admin_status': 'true'})
        daemon.traffic_shift.apply.assert_called_with(True)
        assert not any('route-map NEW_OUT out' in str(c) for c in run.call_args_list)
        assert daemon.table_data_cache['BGP_PEER_GROUP_AF&&default|PG|ipv4_unicast']['route_map_out'] == ['NEW_OUT']


def test_startup_replay_never_temporarily_restores_transit_policy():
    from tests.test_config import mockmapping
    with patch.dict('sys.modules', **mockmapping):
        from frrcfgd.frrcfgd import BGPConfigDaemon
        with patch('frrcfgd.frrcfgd.ExtConfigDBConnector') as connector, \
                patch('frrcfgd.frrcfgd.g_run_command', return_value=True), \
                patch('frrcfgd.frrcfgd.subprocess.check_output', return_value=''):
            db = connector.return_value
            db.get_entry.side_effect = lambda table, key: (
                {'docker_routing_config_mode': 'unified', 'use_template_render_for_restore': 'false'}
                if table == 'DEVICE_METADATA' else {'tsa_enabled': 'true'})
            db.get_table.side_effect = lambda table: (
                {'default': {'local_asn': '65000'}} if table == 'BGP_GLOBALS' else {})
            db.get_table_data.return_value = {}
            replay_states = []
            def replay(daemon, table, key, data):
                replay_states.append(daemon.traffic_shift.enabled)
            with patch.object(BGPConfigDaemon, '_BGPConfigDaemon__replay_table_entry', replay):
                BGPConfigDaemon()
            assert replay_states and all(replay_states)
