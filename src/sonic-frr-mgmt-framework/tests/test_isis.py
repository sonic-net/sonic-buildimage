"""IS-IS composite-key events, restart parity, and failed application handling."""
import copy
import queue
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest
from jinja2 import Environment, FileSystemLoader

from .test_config import mockmapping


class ConfigDB:
    def __init__(self):
        self.tables = {}

    def get_entry(self, table, key):
        key = '|'.join(key) if isinstance(key, tuple) else key
        return copy.deepcopy(self.tables.get(table, {}).get(key, {}))

    def get_table(self, table):
        return copy.deepcopy(self.tables.get(table, {}))

    def set(self, table, key, row):
        rows = self.tables.setdefault(table, {})
        if row is None:
            rows.pop(key, None)
        else:
            rows[key] = copy.deepcopy(row)


@pytest.fixture
def daemon():
    with patch.dict('sys.modules', **mockmapping):
        from frrcfgd.frrcfgd import BGPConfigDaemon
        instance = BGPConfigDaemon()
        instance.config_db = ConfigDB()
        instance.table_data_cache = {}
        instance.bgp_message = queue.Queue()
        instance.isis_interface_vrfs = {}
        instance.isis_interface_bindings = {}
        instance._BGPConfigDaemon__run_command = MagicMock(return_value=True)
        yield instance


def event(daemon, table, key, row):
    daemon.config_db.set(table, key, row)
    daemon.isis_handler(table, key, copy.deepcopy(row))
    call = daemon._BGPConfigDaemon__run_command.call_args
    return call[0][1][2::2] if call else []


def test_instance_row_defaults_and_net_replacement(daemon):
    commands = event(daemon, 'ISIS_GLOBALS', 'VrfBlue|CORE', {'NULL': 'NULL'})
    assert 'router isis CORE vrf VrfBlue' in commands
    assert 'metric-style wide' in commands
    assert 'lsp-mtu 1497' in commands
    assert 'no topology ipv6-unicast' in commands
    assert 'hostname dynamic' in commands
    assert 'lsp-timers level-1 gen-interval 30 refresh-interval 900 max-lifetime 1200' in commands
    assert daemon.table_data_cache['ISIS_GLOBALS&&VrfBlue|CORE'] == {'NULL': 'NULL'}
    old = '49.0001.0000.0000.0001.00'
    new = '49.0002.0000.0000.0001.00'
    event(daemon, 'ISIS_GLOBALS', 'VrfBlue|CORE', {'net_title': old})
    commands = event(daemon, 'ISIS_GLOBALS', 'VrfBlue|CORE', {'net_title': new})
    assert commands.index('no net ' + old) < commands.index('net ' + new)
    commands = event(daemon, 'ISIS_GLOBALS', 'VrfBlue|CORE', {'NULL': 'NULL'})
    assert 'no net ' + new in commands
    assert 'no router isis CORE vrf VrfBlue' in event(daemon, 'ISIS_GLOBALS', 'VrfBlue|CORE', None)


def test_full_lsp_timers_preserve_other_values_and_backoff(daemon):
    row = {'lsp_generation_interval_level_1': '40', 'lsp_refresh_interval_level_1': '1000',
           'lsp_lifetime_level_1': '1500', 'spf_minimum_interval_level_2': '5',
           'spf_init_delay': '0', 'spf_short_delay': '50', 'spf_long_delay': '100',
           'spf_hold_down': '1000', 'spf_time_to_learn': '500'}
    commands = event(daemon, 'ISIS_GLOBALS', 'default|A', row)
    assert 'lsp-timers level-1 gen-interval 40 refresh-interval 1000 max-lifetime 1500' in commands
    assert 'spf-interval level-2 5' in commands
    assert 'spf-delay-ietf init-delay 0 short-delay 50 long-delay 100 holddown 1000 time-to-learn 500' in commands
    commands = event(daemon, 'ISIS_GLOBALS', 'default|A', {'NULL': 'NULL'})
    assert 'no spf-delay-ietf' in commands


def test_af_detach_clears_attributes_and_reenable_uses_defaults(daemon):
    event(daemon, 'ISIS_GLOBALS', 'default|A', {'level': 'level-2'})
    row = {'instance_name': 'A', 'ipv4_enabled': 'true', 'metric_level_1': '0',
           'hello_interval_level_2': '10', 'network_type': 'p2p'}
    commands = event(daemon, 'ISIS_INTERFACE', 'default|Ethernet0', row)
    assert 'isis circuit-type level-2-only' in commands
    assert 'ip router isis A' in commands
    assert 'no ipv6 router isis A' in commands
    assert 'isis hello-interval level-2 10' in commands
    commands = event(daemon, 'ISIS_INTERFACE', 'default|Ethernet0', None)
    assert 'no ip router isis A' in commands
    assert 'no isis metric' in commands
    assert 'no isis hello-interval' in commands
    assert not daemon.isis_interface_bindings
    commands = event(daemon, 'ISIS_INTERFACE', 'default|Ethernet0',
                     {'instance_name': 'A', 'ipv6_enabled': 'true'})
    assert 'no isis metric level-1' in commands
    assert 'no isis hello-interval level-2' in commands


def test_parent_level_update_recalculates_interface_without_rewriting_row(daemon):
    event(daemon, 'ISIS_GLOBALS', 'default|A', {'NULL': 'NULL'})
    row = {'instance_name': 'A', 'ipv4_enabled': 'true', 'level': 'level-1-2'}
    event(daemon, 'ISIS_INTERFACE', 'default|Ethernet0', row)
    event(daemon, 'ISIS_REDISTRIBUTE', 'default|A|ipv4|table|level-1|10', {'metric': '20'})
    run = daemon._BGPConfigDaemon__run_command
    run.reset_mock()
    commands = event(daemon, 'ISIS_GLOBALS', 'default|A', {'level': 'level-1'})
    assert commands == ['configure terminal', 'interface Ethernet0', 'isis circuit-type level-1', 'end']
    assert len(run.call_args_list) == 2
    assert all(call[0][0] != 'ISIS_REDISTRIBUTE' for call in run.call_args_list)
    assert daemon.config_db.get_entry('ISIS_INTERFACE', 'default|Ethernet0') == row


def test_mtu_update_leaves_interfaces_and_table_policies_untouched(daemon):
    event(daemon, 'ISIS_GLOBALS', 'default|A', {'lsp_mtu': '1300'})
    event(daemon, 'ISIS_INTERFACE', 'default|Ethernet0',
          {'instance_name': 'A', 'ipv4_enabled': 'true', 'network_type': 'p2p'})
    event(daemon, 'ISIS_REDISTRIBUTE', 'default|A|ipv4|table|level-2|10', {'metric': '20'})
    run = daemon._BGPConfigDaemon__run_command
    run.reset_mock()
    commands = event(daemon, 'ISIS_GLOBALS', 'default|A', {'lsp_mtu': '1400'})
    assert commands == ['configure terminal', 'router isis A', 'lsp-mtu 1400', 'end']
    assert run.call_count == 1


@pytest.mark.parametrize('table,key,row', [
    ('ISIS_GLOBALS', 'default|A', {'NULL': 'NULL'}),
    ('ISIS_INTERFACE', 'default|Ethernet0', {'instance_name': 'A', 'ipv4_enabled': 'true'}),
    ('ISIS_REDISTRIBUTE', 'default|A|ipv4|table|level-2|10', {'metric': '20'}),
])
def test_unchanged_event_does_not_send_frr_commands(daemon, table, key, row):
    event(daemon, 'ISIS_GLOBALS', 'default|A', {'NULL': 'NULL'})
    event(daemon, table, key, row)
    run = daemon._BGPConfigDaemon__run_command
    run.reset_mock()
    event(daemon, table, key, row)
    run.assert_not_called()
    assert daemon.table_data_cache[table + '&&' + key] == row


def test_interface_metric_af_and_reset_only_apply_related_commands(daemon):
    event(daemon, 'ISIS_GLOBALS', 'default|A', {'NULL': 'NULL'})
    row = {'instance_name': 'A', 'ipv4_enabled': 'true', 'metric_level_1': '10'}
    event(daemon, 'ISIS_INTERFACE', 'default|Ethernet0', row)
    row['metric_level_1'] = '20'
    assert event(daemon, 'ISIS_INTERFACE', 'default|Ethernet0', row) == [
        'configure terminal', 'interface Ethernet0', 'isis metric level-1 20', 'end']
    row['ipv6_enabled'] = 'true'
    assert event(daemon, 'ISIS_INTERFACE', 'default|Ethernet0', row) == [
        'configure terminal', 'interface Ethernet0', 'ipv6 router isis A', 'end']
    row.pop('metric_level_1')
    assert event(daemon, 'ISIS_INTERFACE', 'default|Ethernet0', row) == [
        'configure terminal', 'interface Ethernet0', 'no isis metric level-1', 'end']


def test_changed_lsp_and_backoff_groups_preserve_unchanged_values(daemon):
    row = {'lsp_generation_interval_level_1': '40', 'lsp_refresh_interval_level_1': '1000',
           'lsp_lifetime_level_1': '1500', 'spf_init_delay': '0', 'spf_short_delay': '50',
           'spf_long_delay': '100', 'spf_hold_down': '1000', 'spf_time_to_learn': '500'}
    event(daemon, 'ISIS_GLOBALS', 'default|A', row)
    row['lsp_generation_interval_level_1'] = '50'
    assert event(daemon, 'ISIS_GLOBALS', 'default|A', row) == [
        'configure terminal', 'router isis A',
        'lsp-timers level-1 gen-interval 50 refresh-interval 1000 max-lifetime 1500', 'end']
    row.pop('lsp_generation_interval_level_1')
    assert event(daemon, 'ISIS_GLOBALS', 'default|A', row) == [
        'configure terminal', 'router isis A',
        'lsp-timers level-1 gen-interval 30 refresh-interval 1000 max-lifetime 1500', 'end']
    row['spf_short_delay'] = '60'
    assert event(daemon, 'ISIS_GLOBALS', 'default|A', row) == [
        'configure terminal', 'router isis A',
        'spf-delay-ietf init-delay 0 short-delay 60 long-delay 100 holddown 1000 time-to-learn 500', 'end']


def test_startup_child_replay_does_not_rebuild_already_restored_table_policy(daemon):
    snapshot = [
        ('ISIS_GLOBALS', 'default|A', {'NULL': 'NULL'}),
        ('ISIS_INTERFACE', 'default|Ethernet0', {'instance_name': 'A', 'ipv4_enabled': 'true'}),
        ('ISIS_REDISTRIBUTE', 'default|A|ipv4|table|level-2|10', {'metric': '20'}),
    ]
    for table, key, row in snapshot:
        daemon.config_db.set(table, key, row)
    with patch.object(daemon.config_db, 'serialize_key', side_effect=lambda key: key, create=True):
        for table, key, row in snapshot:
            daemon._BGPConfigDaemon__replay_table_entry(table, key, copy.deepcopy(row))
    calls = daemon._BGPConfigDaemon__run_command.call_args_list
    assert [call[0][0] for call in calls] == ['ISIS_GLOBALS', 'ISIS_INTERFACE', 'ISIS_REDISTRIBUTE']
    assert 'no redistribute ipv4 table 10 level-2' not in calls[-1][0][1]
    for table, key, row in snapshot:
        assert daemon.table_data_cache[table + '&&' + key] == row


def test_parent_applies_child_cache_before_child_event(daemon):
    key = 'default|A|ipv4|table|level-2|10'
    daemon.config_db.set('ISIS_REDISTRIBUTE', key, {'metric': '20'})
    event(daemon, 'ISIS_GLOBALS', 'default|A', {'NULL': 'NULL'})
    assert daemon.table_data_cache['ISIS_REDISTRIBUTE&&' + key] == {'metric': '20'}
    commands = event(daemon, 'ISIS_REDISTRIBUTE', key, {'metric': '30'})
    assert 'no redistribute ipv4 table 10 level-2' in commands


def test_failed_child_does_not_replay_successful_parent_net(daemon):
    key = 'default|A|ipv4|table|level-2|10'
    daemon.config_db.set('ISIS_REDISTRIBUTE', key, {'metric': '20'})
    run = daemon._BGPConfigDaemon__run_command
    run.side_effect = [True, False]
    row = {'net_title': '49.0001.0000.0000.0001.00'}
    event(daemon, 'ISIS_GLOBALS', 'default|A', row)
    assert daemon.table_data_cache['ISIS_GLOBALS&&default|A'] == row
    assert 'ISIS_REDISTRIBUTE&&' + key not in daemon.table_data_cache
    run.side_effect = None
    run.reset_mock()
    commands = event(daemon, 'ISIS_REDISTRIBUTE', key, {'metric': '30'})
    assert len(run.call_args_list) == 1
    assert 'net ' + row['net_title'] not in run.call_args[0][1]
    assert 'redistribute ipv4 table 10 level-2 metric 30' in commands
    assert daemon.table_data_cache['ISIS_REDISTRIBUTE&&' + key] == {'metric': '30'}


def test_table_policy_replacement_and_late_delete(daemon):
    event(daemon, 'ISIS_GLOBALS', 'default|A', {'NULL': 'NULL'})
    key = 'default|A|ipv6|table|level-2|10'
    commands = event(daemon, 'ISIS_REDISTRIBUTE', key, {'metric': '20', 'route_map': ['FILTER']})
    assert 'redistribute ipv6 table 10 level-2 metric 20 route-map FILTER' in commands
    assert 'no redistribute ipv6 table 10 level-2' not in commands
    commands = event(daemon, 'ISIS_REDISTRIBUTE', key, {'metric': '0'})
    assert commands.index('no redistribute ipv6 table 10 level-2') < commands.index(
        'redistribute ipv6 table 10 level-2 metric 0')
    event(daemon, 'ISIS_GLOBALS', 'default|A', None)
    daemon._BGPConfigDaemon__run_command.reset_mock()
    event(daemon, 'ISIS_REDISTRIBUTE', key, None)
    daemon._BGPConfigDaemon__run_command.assert_not_called()
    assert 'ISIS_REDISTRIBUTE&&' + key not in daemon.table_data_cache


@pytest.mark.parametrize('table,key,old,new,next_row,expected_commands,next_command', [
    ('ISIS_GLOBALS', 'default|A', {'lsp_mtu': '1300'}, {'lsp_mtu': '1400'},
     {'lsp_mtu': '1500'},
     ['configure terminal', 'router isis A', 'lsp-mtu 1400', 'end'], 'lsp-mtu 1500'),
    ('ISIS_REDISTRIBUTE', 'default|A|ipv4|table|level-2|10',
     {'metric': '20', 'route_map': ['FILTER']}, {'metric': '30'}, {'metric': '40'},
     ['configure terminal', 'router isis A', 'no redistribute ipv4 table 10 level-2',
      'redistribute ipv4 table 10 level-2 metric 30', 'end'],
     'redistribute ipv4 table 10 level-2 metric 40'),
])
def test_failed_event_keeps_cache_without_compensation_and_next_event_applies(
        daemon, table, key, old, new, next_row, expected_commands, next_command):
    event(daemon, 'ISIS_GLOBALS', 'default|A', {'NULL': 'NULL'})
    event(daemon, table, key, old)
    run = daemon._BGPConfigDaemon__run_command
    run.reset_mock()
    run.return_value = False
    with patch('frrcfgd.frrcfgd.syslog.syslog') as log:
        commands = event(daemon, table, key, new)
    assert commands == expected_commands
    assert run.call_count == 1
    assert daemon.config_db.get_entry(table, key) == new
    assert daemon.table_data_cache[table + '&&' + key] == old
    assert any('failed running {} config command'.format(table) in str(call) for call in log.call_args_list)
    run.return_value = True
    run.reset_mock()
    assert next_command in event(daemon, table, key, next_row)
    assert run.call_count == 1
    assert daemon.table_data_cache[table + '&&' + key] == next_row


def test_failed_interface_update_keeps_applied_binding_and_cache(daemon):
    event(daemon, 'ISIS_GLOBALS', 'default|A', {'NULL': 'NULL'})
    key = 'default|Ethernet0'
    old = {'instance_name': 'A', 'ipv4_enabled': 'true', 'metric_level_1': '10'}
    new = dict(old, metric_level_1='20')
    event(daemon, 'ISIS_INTERFACE', key, old)
    run = daemon._BGPConfigDaemon__run_command
    run.return_value = False
    event(daemon, 'ISIS_INTERFACE', key, new)
    assert daemon.table_data_cache['ISIS_INTERFACE&&' + key] == old
    assert daemon.isis_interface_bindings[key] == ('default', 'A')
    assert daemon.config_db.get_entry('ISIS_INTERFACE', key) == new
    run.return_value = True
    commands = event(daemon, 'ISIS_INTERFACE', key, None)
    assert 'no ip router isis A' in commands
    assert not daemon.isis_interface_bindings
    assert 'ISIS_INTERFACE&&' + key not in daemon.table_data_cache


def test_routed_vrf_change_detaches_old_binding(daemon):
    event(daemon, 'ISIS_GLOBALS', 'default|A', {'NULL': 'NULL'})
    event(daemon, 'ISIS_INTERFACE', 'default|Ethernet0', {'instance_name': 'A', 'ipv4_enabled': 'true'})
    daemon.isis_routed_interface_handler('INTERFACE', 'Ethernet0', {'vrf_name': 'VrfBlue'})
    assert 'no ip router isis A' in daemon._BGPConfigDaemon__run_command.call_args[0][1]
    assert not daemon.isis_interface_bindings


def test_full_key_dispatch_validation(daemon):
    run = daemon._BGPConfigDaemon__run_command
    daemon.isis_handler('ISIS_GLOBALS', ('default', 'A'), {'NULL': 'NULL'})
    assert 'router isis A' in run.call_args[0][1]
    run.reset_mock()
    for key in ('default', 'default|A|extra', 'default|A\nexit'):
        daemon.isis_handler('ISIS_GLOBALS', key, {'NULL': 'NULL'})
    run.assert_not_called()


@pytest.mark.parametrize('full_attributes', [False, True])
def test_template_runtime_parity(daemon, full_attributes):
    config = {
        'ISIS_GLOBALS': {'VrfBlue|A': {'net_title': '49.0001.0000.0000.0001.00', 'level': 'level-2',
                                     'lsp_lifetime_level_1': '1500', 'overload': 'true'}},
        'ISIS_INTERFACE': {'VrfBlue|Ethernet0': {'instance_name': 'A', 'ipv6_enabled': 'true',
                                               'hello_interval_level_2': '10', 'network_type': 'p2p'}},
        'ISIS_REDISTRIBUTE': {'VrfBlue|A|ipv6|table|level-2|10': {'metric': '20', 'route_map': ['FILTER']}},
        'INTERFACE': {'Ethernet0': {'vrf_name': 'VrfBlue'}},
    }
    if full_attributes:
        config['ISIS_GLOBALS']['VrfBlue|A'].update(
            dynamic_hostname='false', lsp_mtu='1400', ipv6_multi_topology='true',
            overload_on_startup='60', lsp_generation_interval_level_1='40',
            lsp_refresh_interval_level_1='1000', lsp_generation_interval_level_2='50',
            lsp_refresh_interval_level_2='600', lsp_lifetime_level_2='1400',
            spf_minimum_interval_level_1='5', spf_minimum_interval_level_2='6',
            spf_init_delay='0', spf_short_delay='50', spf_long_delay='100',
            spf_hold_down='1000', spf_time_to_learn='500')
        config['ISIS_INTERFACE']['VrfBlue|Ethernet0'].update(
            ipv4_enabled='true', passive='true', metric_level_1='20', metric_level_2='30',
            hello_interval_level_1='5', hello_multiplier_level_1='4', hello_multiplier_level_2='5')
    templates = Path(__file__).resolve().parents[1] / 'templates'
    template = Environment(loader=FileSystemLoader(str(templates / 'isisd'))).get_template('isisd.conf.j2')
    output = template.render(**config)
    daemon.isis_interface_vrfs['Ethernet0'] = 'VrfBlue'
    runtime = []
    for table in ('ISIS_GLOBALS', 'ISIS_INTERFACE', 'ISIS_REDISTRIBUTE'):
        key, row = next(iter(config[table].items()))
        runtime.extend(event(daemon, table, key, row))
    startup_lines = {line.strip() for line in output.splitlines() if line.strip() and not line.startswith('!')}
    # AF disable commands aren't needed in a fresh process; compare effective configuration.
    runtime_lines = set(runtime) - {'configure terminal', 'end', 'no ip router isis A'}
    assert startup_lines == runtime_lines
    assert 'isis hello-interval level-2 10' in startup_lines
    tuple_config = {}
    for table, rows in config.items():
        tuple_config[table] = {(tuple(key.split('|')) if '|' in key else key): row for key, row in rows.items()}
    assert template.render(**tuple_config) == output


@pytest.mark.parametrize('interface,field,value,command,reset_command', [
    (False, 'level', 'level-1', 'is-type level-1', 'is-type level-1-2'),
    (False, 'dynamic_hostname', 'false', 'no hostname dynamic', 'hostname dynamic'),
    (False, 'lsp_mtu', '1400', 'lsp-mtu 1400', 'lsp-mtu 1497'),
    (False, 'ipv6_multi_topology', 'true', 'topology ipv6-unicast', 'no topology ipv6-unicast'),
    (False, 'overload', 'true', 'set-overload-bit', 'no set-overload-bit'),
    (False, 'overload_on_startup', '60', 'set-overload-bit on-startup 60', 'no set-overload-bit on-startup'),
    (True, 'level', 'level-1', 'isis circuit-type level-1', 'isis circuit-type level-1-2'),
    (True, 'network_type', 'p2p', 'isis network point-to-point', 'no isis network point-to-point'),
    (True, 'passive', 'true', 'isis passive', 'no isis passive'),
])
def test_scalar_field_delta_and_reset_mapping(daemon, interface, field, value, command, reset_command):
    event(daemon, 'ISIS_GLOBALS', 'default|A', {'net_title': '49.0001.0000.0000.0001.00'})
    table, key = ('ISIS_INTERFACE', 'default|Ethernet0') if interface else ('ISIS_GLOBALS', 'default|A')
    baseline = ({'instance_name': 'A', 'ipv4_enabled': 'true'} if interface
                else {'net_title': '49.0001.0000.0000.0001.00'})
    event(daemon, table, key, baseline)
    run = daemon._BGPConfigDaemon__run_command
    context = 'interface Ethernet0' if interface else 'router isis A'
    for row, expected in ((dict(baseline, **{field: value}), command), (baseline, reset_command)):
        run.reset_mock()
        assert event(daemon, table, key, row) == ['configure terminal', context, expected, 'end']
        assert run.call_count == 1
        assert run.call_args[0][0] == table
        assert daemon.table_data_cache[table + '&&' + key] == row


@pytest.mark.parametrize('interface,field,value,command,reset_command,level', [
    values + (n,)
    for values in [
        (False, 'lsp_generation_interval', '40',
         'lsp-timers level-{} gen-interval 40 refresh-interval 900 max-lifetime 1200',
         'lsp-timers level-{} gen-interval 30 refresh-interval 900 max-lifetime 1200'),
        (False, 'lsp_refresh_interval', '600',
         'lsp-timers level-{} gen-interval 30 refresh-interval 600 max-lifetime 1200',
         'lsp-timers level-{} gen-interval 30 refresh-interval 900 max-lifetime 1200'),
        (False, 'lsp_lifetime', '1500',
         'lsp-timers level-{} gen-interval 30 refresh-interval 900 max-lifetime 1500',
         'lsp-timers level-{} gen-interval 30 refresh-interval 900 max-lifetime 1200'),
        (False, 'spf_minimum_interval', '5', 'spf-interval level-{} 5', 'spf-interval level-{} 1'),
        (True, 'metric', '20', 'isis metric level-{} 20', 'no isis metric level-{}'),
        (True, 'hello_interval', '5', 'isis hello-interval level-{} 5', 'no isis hello-interval level-{}'),
        (True, 'hello_multiplier', '4', 'isis hello-multiplier level-{} 4', 'no isis hello-multiplier level-{}'),
    ]
    for n in (1, 2)
])
def test_per_level_field_delta_and_reset_mapping(
        daemon, interface, field, value, command, reset_command, level):
    event(daemon, 'ISIS_GLOBALS', 'default|A', {'net_title': '49.0001.0000.0000.0001.00'})
    table, key = ('ISIS_INTERFACE', 'default|Ethernet0') if interface else ('ISIS_GLOBALS', 'default|A')
    baseline = ({'instance_name': 'A', 'ipv4_enabled': 'true'} if interface
                else {'net_title': '49.0001.0000.0000.0001.00'})
    event(daemon, table, key, baseline)
    run = daemon._BGPConfigDaemon__run_command
    context = 'interface Ethernet0' if interface else 'router isis A'
    for row, expected in ((dict(baseline, **{field + '_level_' + str(level): value}), command),
                          (baseline, reset_command)):
        run.reset_mock()
        assert event(daemon, table, key, row) == [
            'configure terminal', context, expected.format(level), 'end']
        assert run.call_count == 1
        assert daemon.table_data_cache[table + '&&' + key] == row


@pytest.mark.parametrize('af,source', [('ipv4', 'ospf'), ('ipv6', 'ospf6')])
def test_ordinary_policy_replacement_and_delete(daemon, af, source):
    event(daemon, 'ISIS_GLOBALS', 'VrfBlue|A', {'NULL': 'NULL'})
    key = 'VrfBlue|A|{}|{}|level-1|0'.format(af, source)
    policy = 'redistribute {} {} level-1'.format(af, source)
    for row, expected in (({'metric': '20', 'route_map': ['FILTER']}, policy + ' metric 20 route-map FILTER'),
                          ({'metric': '0'}, policy + ' metric 0'), (None, 'no ' + policy)):
        daemon._BGPConfigDaemon__run_command.reset_mock()
        assert event(daemon, 'ISIS_REDISTRIBUTE', key, row) == [
            'configure terminal', 'router isis A vrf VrfBlue', expected, 'end']
        assert daemon._BGPConfigDaemon__run_command.call_count == 1


@pytest.mark.parametrize('changed_row', [{'vrf_name': 'VrfBlue'}, None])
def test_routed_owner_change_and_restore_binding(daemon, changed_row):
    event(daemon, 'ISIS_GLOBALS', 'default|A', {'NULL': 'NULL'})
    row = {'instance_name': 'A', 'ipv6_enabled': 'true', 'hello_multiplier_level_2': '4'}
    event(daemon, 'ISIS_INTERFACE', 'default|Ethernet0', row)
    daemon.isis_routed_interface_handler('INTERFACE', 'Ethernet0', changed_row)
    assert 'no ipv6 router isis A' in daemon._BGPConfigDaemon__run_command.call_args[0][1]
    assert not daemon.isis_interface_bindings
    assert daemon.config_db.get_entry('ISIS_INTERFACE', 'default|Ethernet0') == row
    daemon.isis_routed_interface_handler('INTERFACE', 'Ethernet0', {})
    assert daemon.isis_interface_bindings == {'default|Ethernet0': ('default', 'A')}
    assert 'ipv6 router isis A' in daemon._BGPConfigDaemon__run_command.call_args[0][1]
    assert 'isis hello-multiplier level-2 4' in daemon._BGPConfigDaemon__run_command.call_args[0][1]


def test_prefix_only_routed_event_does_not_move_binding(daemon):
    event(daemon, 'ISIS_GLOBALS', 'default|A', {'NULL': 'NULL'})
    event(daemon, 'ISIS_INTERFACE', 'default|Ethernet0', {'instance_name': 'A', 'ipv4_enabled': 'true'})
    run = daemon._BGPConfigDaemon__run_command
    run.reset_mock()
    for key in ('Ethernet0|192.0.2.1/31', ('Ethernet0', '2001:db8::1/64')):
        daemon.isis_routed_interface_handler('INTERFACE', key, None)
    run.assert_not_called()
    assert daemon.isis_interface_bindings == {'default|Ethernet0': ('default', 'A')}
