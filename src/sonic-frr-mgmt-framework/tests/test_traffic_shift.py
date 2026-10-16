import copy
import ipaddress
import subprocess
from pathlib import Path
from unittest.mock import MagicMock, patch

import jinja2
import pytest

from frrcfgd.traffic_shift import TrafficShift
from tests.test_config import mockmapping


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


@pytest.fixture
def daemon_class():
    with patch.dict('sys.modules', **mockmapping):
        from frrcfgd.frrcfgd import BGPConfigDaemon
        yield BGPConfigDaemon


def sent(run):
    """Return the commands of the last TSA transaction."""
    return run.call_args.args[0]


def logged(log, message):
    return any(message in str(call) for call in log.call_args_list)


def test_enable_disable_and_inheritance(tsa):
    shift, tables, run = tsa
    assert shift.apply(True)
    commands = sent(run)
    assert 'ip prefix-list FRRCFGD_TSA_default_ip permit 10.0.0.1/32' in commands
    assert 'ipv6 prefix-list FRRCFGD_TSA_default_ipv6 permit 2001:db8::1/128' in commands
    assert 'call OUT4' in commands and 'call OUT6' in commands
    assert 'neighbor PG route-map FRRCFGD_TSA_default_ipv4_OUT4 out' in commands
    assert not any(c.startswith('neighbor Ethernet0 route-map') for c in commands)
    assert shift.apply(False)
    commands = sent(run)
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
    commands = sent(run)
    assert 'call NEW_OUT' in commands
    assert 'no route-map FRRCFGD_TSA_default_ipv4_OUT4' in commands
    assert 'no ip prefix-list FRRCFGD_TSA_default_ip permit 10.0.0.1/32' in commands
    assert shift.apply(False)
    assert 'neighbor PG route-map NEW_OUT out' in sent(run)


def test_neighbor_without_policy_and_af_deletion(tsa):
    shift, tables, run = tsa
    key = ('default', '10.0.0.2', 'ipv4_unicast')
    tables['BGP_NEIGHBOR'][key[:2]] = {'asn': '65001'}
    tables['BGP_NEIGHBOR_AF'][key] = {'admin_status': 'true'}
    assert shift.apply(True)
    assert 'neighbor 10.0.0.2 route-map FRRCFGD_TSA_default_ipv4 out' in sent(run)
    del tables['BGP_NEIGHBOR_AF'][key]
    assert shift.apply(True)
    assert 'no neighbor 10.0.0.2 route-map FRRCFGD_TSA_default_ipv4 out' in sent(run)
    assert 'no route-map FRRCFGD_TSA_default_ipv4' in sent(run)


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
    commands = sent(run)
    assert 'neighbor PG route-map OUT4 out' in commands
    assert 'no route-map FRRCFGD_TSA_default_ipv4_OLD_OUT' in commands
    assert 'no ip prefix-list FRRCFGD_TSA_default_ip' in commands
    assert not shift.enabled


@pytest.mark.parametrize('enabled', [True, False])
def test_restart_cleans_binding_for_removed_af(tsa, enabled):
    shift, tables, run = tsa
    del tables['BGP_PEER_GROUP_AF'][('default', 'PG', 'ipv4_unicast')]
    shift.recover('''router bgp 65000
 neighbor PG peer-group
 address-family ipv4 unicast
  neighbor PG route-map FRRCFGD_TSA_default_ipv4_OLD_OUT out
 exit-address-family
exit
route-map FRRCFGD_TSA_default_ipv4_OLD_OUT permit 10
''')
    assert shift.bindings == {
        ('BGP_PEER_GROUP_AF', ('default', 'PG', 'ipv4_unicast')):
            ('65000', 'FRRCFGD_TSA_default_ipv4_OLD_OUT')}
    assert shift.apply(enabled)
    commands = sent(run)
    detach = 'no neighbor PG route-map FRRCFGD_TSA_default_ipv4_OLD_OUT out'
    cleanup = 'no route-map FRRCFGD_TSA_default_ipv4_OLD_OUT'
    assert commands.index(detach) < commands.index(cleanup)
    policy = 'FRRCFGD_TSA_default_ipv6_OUT6' if enabled else 'OUT6'
    assert 'neighbor PG route-map {} out'.format(policy) in commands


def test_recover_scopes_bindings_to_bgp_vrf_and_unicast_af(tsa):
    shift, tables, run = tsa
    shift.recover('''router bgp 65000
 neighbor PG peer-group
 address-family ipv4 unicast
  neighbor PG route-map FRRCFGD_TSA_default_ipv4_OUT4 out
 exit-address-family
 address-family l2vpn evpn
  neighbor PG route-map FRRCFGD_TSA_IGNORED out
 exit-address-family
exit
router bgp 65002 vrf Vrf_red
 address-family ipv6 unicast
  neighbor 2001:db8::2 route-map FRRCFGD_TSA_Vrf_red_ipv6_OUT6 out
  neighbor 2001:db8::3 route-map USER_OUT out
 exit-address-family
exit
router ospf
 address-family ipv4 unicast
  neighbor PG route-map FRRCFGD_TSA_IGNORED out
exit
''')
    assert shift.bindings == {
        ('BGP_PEER_GROUP_AF', ('default', 'PG', 'ipv4_unicast')):
            ('65000', 'FRRCFGD_TSA_default_ipv4_OUT4'),
        ('BGP_NEIGHBOR_AF', ('Vrf_red', '2001:db8::2', 'ipv6_unicast')):
            ('65002', 'FRRCFGD_TSA_Vrf_red_ipv6_OUT6')}
    assert shift.enabled  # A binding alone still needs reconciliation.
    shift.recover('')
    assert not shift.bindings and not shift.enabled


@pytest.mark.parametrize('nested', [
    ' segment-routing srv6\n  locator MAIN\n exit\n',
    ' bmp targets COLLECTOR\n  bmp connect 192.0.2.1 port 5000 min-retry 1000 max-retry 2000\n exit\n'])
def test_recover_keeps_router_context_after_nested_exit(tsa, nested):
    shift, tables, run = tsa
    shift.recover('router bgp 65000\n neighbor PG peer-group\n' + nested +
                  ' !\n address-family ipv4 unicast\n'
                  '  neighbor PG route-map FRRCFGD_TSA_default_ipv4_OUT4 out\n'
                  ' exit-address-family\nexit\n')
    assert shift.bindings == {
        ('BGP_PEER_GROUP_AF', ('default', 'PG', 'ipv4_unicast')):
            ('65000', 'FRRCFGD_TSA_default_ipv4_OUT4')}


@pytest.mark.parametrize('peer_exists', [True, False])
def test_recovered_binding_cleanup_does_not_recreate_deleted_peer(tsa, peer_exists):
    shift, tables, run = tsa
    tables['BGP_GLOBALS']['Vrf_red'] = {'local_asn': '65002'}
    if peer_exists:
        tables['BGP_NEIGHBOR'][('Vrf_red', '2001:db8::2')] = {'asn': '65003'}
    shift.recover('''router bgp 65002 vrf Vrf_red
 address-family ipv6 unicast
  neighbor 2001:db8::2 route-map FRRCFGD_TSA_Vrf_red_ipv6_OUT6 out
 exit-address-family
exit
route-map FRRCFGD_TSA_Vrf_red_ipv6_OUT6 permit 10
''')
    assert shift.apply(False)
    commands = sent(run)
    detach = 'no neighbor 2001:db8::2 route-map FRRCFGD_TSA_Vrf_red_ipv6_OUT6 out'
    assert (detach in commands) == peer_exists
    if peer_exists:
        index = commands.index(detach)
        assert commands[index - 2:index] == ['router bgp 65002 vrf Vrf_red',
                                          'address-family ipv6 unicast']
    assert 'no route-map FRRCFGD_TSA_Vrf_red_ipv6_OUT6' in commands


def test_apply_reads_each_af_table_once(tsa):
    shift, tables, run = tsa
    assert shift.apply(True)
    shift.config_db.get_table.reset_mock()
    del tables['BGP_PEER_GROUP_AF'][('default', 'PG', 'ipv4_unicast')]
    assert shift.apply(True)
    calls = [call.args[0] for call in shift.config_db.get_table.call_args_list]
    assert calls.count('BGP_PEER_GROUP_AF') == 1
    assert calls.count('BGP_NEIGHBOR_AF') == 1


def test_loopback_exemptions_are_vrf_scoped(tsa):
    shift, tables, run = tsa
    tables['LOOPBACK_INTERFACE']['Loopback1'] = {'vrf_name': 'Vrf_red'}
    tables['LOOPBACK_INTERFACE'][('Loopback1', '10.1.0.1/32')] = {}
    tables['BGP_GLOBALS']['Vrf_red'] = {'local_asn': '65002'}
    tables['BGP_PEER_GROUP_AF'][('Vrf_red', 'PG', 'ipv4_unicast')] = {'route_map_out': ['OUT4']}
    assert shift.apply(True)
    commands = sent(run)
    assert 'ip prefix-list FRRCFGD_TSA_Vrf_red_ip permit 10.1.0.1/32' in commands
    assert 'ip prefix-list FRRCFGD_TSA_default_ip permit 10.1.0.1/32' not in commands
    assert 'route-map FRRCFGD_TSA_Vrf_red_ipv4_OUT4 permit 10' in commands
    assert 'match ip address prefix-list FRRCFGD_TSA_Vrf_red_ip' in commands


def test_no_loopbacks_still_installs_a_denying_policy(tsa):
    shift, tables, run = tsa
    tables['LOOPBACK_INTERFACE'] = {}
    assert shift.apply(True)
    commands = sent(run)
    assert 'match ip address prefix-list FRRCFGD_TSA_default_ip' in commands
    assert 'neighbor PG route-map FRRCFGD_TSA_default_ipv4_OUT4 out' in commands


def test_reserved_names_and_invalid_inputs(tsa):
    shift, tables, run = tsa
    tables['ROUTE_MAP'] = {('FRRCFGD_TSA_default_ipv4_OUT4', '10'): {}}
    assert not shift.apply(True)
    del tables['ROUTE_MAP']
    tables['LOOPBACK_INTERFACE'][('Loopback0', 'not-a-prefix')] = {}
    assert not shift.apply(True)
    del tables['LOOPBACK_INTERFACE'][('Loopback0', 'not-a-prefix')]
    shift.validate_input = lambda *args: None
    assert not shift.apply(True)
    run.assert_not_called()


def ip_version(value):
    try:
        return ipaddress.ip_network(value, strict=False).version
    except ValueError:
        return None


def render_startup_config(tables):
    """Render bgpd.conf.db.j2 with stand-ins for sonic-cfggen's address filters."""
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(
        str(Path(__file__).parents[1] / 'templates' / 'bgpd')))
    env.filters.update({
        'ipv4': lambda value: ip_version(value) == 4,
        'ipv6': lambda value: ip_version(value) == 6,
        'ip_network': lambda value: str(ipaddress.ip_network(value, strict=False).network_address),
        'prefixlen': lambda value: ipaddress.ip_network(value, strict=False).prefixlen})
    return env.get_template('bgpd.conf.db.j2').render(**tables).splitlines()


def tsa_config(lines):
    """Return the TSA prefix-lists, wrapper calls and outbound bindings in FRR config lines."""
    config, wrapper, vrf, af = {}, None, None, None
    for words in (line.split() for line in lines):
        if words[:2] == ['router', 'bgp']:
            vrf = words[4] if len(words) > 4 else 'default'
        elif words[:1] == ['address-family']:
            af = words[1]
        elif words[1:2] == ['prefix-list'] and words[2].startswith(TrafficShift.PREFIX):
            config[tuple(words)] = True
        elif words[:1] == ['route-map']:
            wrapper = words[1] if words[1].startswith(TrafficShift.PREFIX) else None
            if wrapper:
                config[('route-map', wrapper)] = None
        elif words[:1] == ['call'] and wrapper:
            config[('route-map', wrapper)] = words[1]
        elif len(words) == 5 and words[0] == 'neighbor' and words[2::2] == ['route-map', 'out']:
            config[('neighbor', vrf, af, words[1])] = words[3]
    return config


@pytest.mark.parametrize('enabled', [True, False])
def test_startup_template_matches_runtime(tsa, enabled):
    shift, tables, run = tsa
    tables['BGP_DEVICE_GLOBAL'] = {'STATE': {'tsa_enabled': 'true' if enabled else 'false'}}
    tables['BGP_GLOBALS']['Vrf_red'] = {'local_asn': '65000'}
    tables['BGP_NEIGHBOR'].update({('default', 'Ethernet4'): {'peer_group_name': 'PG'},
                                   ('default', '10.0.0.2'): {'asn': '65002'},
                                   ('Vrf_red', '10.1.0.2'): {'asn': '65002'}})
    tables['BGP_NEIGHBOR_AF'].update({
        ('default', 'Ethernet4', 'ipv4_unicast'): {'route_map_out': ['OUT4']},
        ('default', '10.0.0.2', 'ipv6_unicast'): {},
        ('Vrf_red', '10.1.0.2', 'ipv4_unicast'): {'route_map_out': ['RED_OUT']}})
    tables['LOOPBACK_INTERFACE'].update({'Loopback1': {'vrf_name': 'Vrf_red'},
                                         ('Loopback1', '10.1.0.1/32'): {}})
    assert shift.apply(enabled)
    runtime = tsa_config(sent(run))
    assert tsa_config(render_startup_config(tables)) == runtime
    if enabled:
        assert runtime[('neighbor', 'default', 'ipv4', 'Ethernet4')] == 'FRRCFGD_TSA_default_ipv4_OUT4'
        assert ('neighbor', 'default', 'ipv4', 'Ethernet0') not in runtime  # Inherits PG.
        assert sent(run).count('route-map FRRCFGD_TSA_default_ipv4_OUT4 permit 10') == 1


def test_device_global_handler_and_live_af_update(daemon_class):
    with patch('frrcfgd.frrcfgd.g_run_command') as run:
        daemon = daemon_class()
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


@pytest.mark.parametrize('error', [FileNotFoundError('vtysh unavailable'),
                                 subprocess.CalledProcessError(1, ['vtysh'])])
def test_recovery_failure_preserves_last_known_state(tsa, daemon_class, error):
    shift, tables, run = tsa
    assert shift.apply(True)
    state = copy.deepcopy((shift.enabled, shift.route_maps, shift.prefixes, shift.bindings))
    daemon = daemon_class.__new__(daemon_class)
    daemon.traffic_shift = shift
    with patch('frrcfgd.frrcfgd.subprocess.check_output', side_effect=error), \
            patch('frrcfgd.frrcfgd.syslog.syslog') as log:
        assert not daemon._BGPConfigDaemon__recover_tsa()
    assert (shift.enabled, shift.route_maps, shift.prefixes, shift.bindings) == state
    assert logged(log, 'Failed to recover TSA state')


@pytest.mark.parametrize('handler', ['enable', 'disable', 'loopback', 'global',
                                   'BGP_NEIGHBOR_AF', 'BGP_PEER_GROUP_AF'])
@pytest.mark.parametrize('recovery_fails', [False, True])
def test_all_apply_failures_are_logged_and_recovered(daemon_class, handler, recovery_fails):
    with patch('frrcfgd.frrcfgd.g_run_command', return_value=True) as run:
        daemon = daemon_class()
        daemon.traffic_shift = MagicMock(enabled=True)
        daemon.traffic_shift.apply.return_value = False
        daemon.bgp_asn['default'] = '65000'
        daemon.table_data_cache = {}
        run.reset_mock()
        error = subprocess.CalledProcessError(1, ['vtysh']) if recovery_fails else None
        with patch('frrcfgd.frrcfgd.subprocess.check_output', return_value='running config',
                   side_effect=error) as read, \
                patch('frrcfgd.frrcfgd.syslog.syslog') as log:
            if handler in ('enable', 'disable'):
                daemon.device_global_handler('BGP_DEVICE_GLOBAL', 'STATE',
                                             {'tsa_enabled': 'true' if handler == 'enable' else 'false'})
            elif handler == 'loopback':
                daemon.loopback_handler('LOOPBACK_INTERFACE', 'Loopback0|10.0.0.1/32', {})
            elif handler == 'global':
                daemon.bgp_global_handler('BGP_GLOBALS', 'default', {'local_asn': '65000'})
            else:
                peer = 'Ethernet0' if handler == 'BGP_NEIGHBOR_AF' else 'PG'
                key = 'default|{}|ipv4_unicast'.format(peer)
                daemon.bgp_table_handler_common(handler, key, {'route_map_out': ['NEW_OUT']})
                run.assert_not_called()
                assert not daemon.table_data_cache.get(handler + '&&' + key, {})
    daemon.traffic_shift.apply.assert_called_once_with(handler != 'disable')
    read.assert_called_once()
    if recovery_fails:
        daemon.traffic_shift.recover.assert_not_called()
        assert daemon.traffic_shift.enabled
        assert logged(log, 'Failed to recover TSA state')
    else:
        daemon.traffic_shift.recover.assert_called_once_with('running config')
    assert logged(log, 'Failed to apply TSA state')


def startup_config_db(connector, tsa_enabled):
    """CONFIG_DB for constructing the daemon in unified mode with BGP configured."""
    db = connector.return_value
    db.get_entry.side_effect = lambda table, key: (
        {'docker_routing_config_mode': 'unified', 'use_template_render_for_restore': 'false'}
        if table == 'DEVICE_METADATA' else {'tsa_enabled': tsa_enabled})
    db.get_table.side_effect = lambda table: (
        {'default': {'local_asn': '65000'}} if table == 'BGP_GLOBALS' else {})
    db.get_table_data.return_value = {}
    db.serialize_key.side_effect = lambda key: '|'.join(key) if isinstance(key, tuple) else key


@pytest.mark.parametrize('state', ['true', 'false'])
def test_startup_survives_unready_vtysh(daemon_class, state):
    with patch('frrcfgd.frrcfgd.ExtConfigDBConnector') as connector, \
            patch('frrcfgd.frrcfgd.g_run_command', return_value=True), \
            patch('frrcfgd.frrcfgd.subprocess.check_output',
                  side_effect=subprocess.CalledProcessError(1, ['vtysh'])), \
            patch('frrcfgd.frrcfgd.syslog.syslog') as log:
        startup_config_db(connector, state)
        daemon = daemon_class()
    assert daemon.traffic_shift.enabled == (state == 'true')
    assert logged(log, 'Failed to recover TSA state')


def test_startup_replay_never_temporarily_restores_transit_policy(daemon_class):
    replay_states = []
    def replay(daemon, table, key, data):
        replay_states.append(daemon.traffic_shift.enabled)
    with patch('frrcfgd.frrcfgd.ExtConfigDBConnector') as connector, \
            patch('frrcfgd.frrcfgd.g_run_command', return_value=True), \
            patch('frrcfgd.frrcfgd.subprocess.check_output', return_value=''), \
            patch.object(daemon_class, '_BGPConfigDaemon__replay_table_entry', replay):
        startup_config_db(connector, 'true')
        daemon_class()
    assert replay_states and all(replay_states)
