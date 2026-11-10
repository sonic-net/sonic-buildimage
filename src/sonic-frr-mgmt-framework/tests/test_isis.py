"""Named-instance, per-level, AF and lifecycle regressions.

The four AF combinations and retained settings extend gs1571's original tests.
Native command execution and DUT forwarding are separate acceptance gates.
"""
import copy
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest
from jinja2 import Environment, FileSystemLoader
from sonic_isis import effective
from frrcfgd.isis_config import desired, render
from frrcfgd.isis_readback import parse
from frrcfgd.isis_manager import plan, conflict
from frrcfgd.isis_recovery import Identity, NativeSnapshot, RecoveryKernel, AtomicJSONStore
from frrcfgd.isis_startup import prepare
from .test_config import mockmapping

NET = '49.0001.0000.0000.0001.00'


def config(vrf='default', instance='CORE'):
    return {
        'VRF': {vrf: {}},
        'INTERFACE': {'Ethernet0': {'vrf_name': vrf}},
        'ISIS_INSTANCE': {(vrf, instance): {'enabled': 'true', 'net_titles': [NET]}},
        'ISIS_INTERFACE': {'Ethernet0': {'vrf': vrf, 'instance': instance, 'enabled': 'true',
                                         'ipv4_enabled': 'true', 'ipv6_enabled': 'true', 'passive': 'true',
                                         'network_type': 'point-to-point'}},
        'ISIS_INTERFACE_LEVEL': {('Ethernet0', 'level-1'): {'metric': '30'}},
    }


def native_text(tables):
    return 'frr version 10.7.1\n' + '\n'.join(render(tables)) + '\nend\n'


@pytest.mark.parametrize('ipv4,ipv6', [(True, True), (False, True), (True, False), (False, False)])
@pytest.mark.parametrize('vrf', ['default', 'VrfA'])
def test_independent_af_startup_runtime_and_readback(ipv4, ipv6, vrf):
    tables = config(vrf)
    tables['ISIS_INTERFACE']['Ethernet0'].update(ipv4_enabled=str(ipv4).lower(), ipv6_enabled=str(ipv6).lower())
    snapshot = desired(tables).snapshot
    output = render(tables)
    commands = [line.strip() for line in output]
    if ipv4 or ipv6:
        assert ('ip router isis CORE' in commands) == ipv4
        assert ('ipv6 router isis CORE' in commands) == ipv6
        assert 'isis metric level-1 30' in commands
        assert 'isis passive' in commands
    else:
        assert not any(' router isis CORE' in line for line in output)
        assert not any('isis metric' in line for line in output)
    assert parse(native_text(tables)).objects == snapshot.objects
    loader = FileSystemLoader(str(Path(__file__).parents[1] / 'templates/isisd'))
    template = Environment(loader=loader).get_template('isisd.conf.j2')
    assert [line for line in template.render(ISIS_CONFIG_LINES=output).splitlines()
            if line and not line.startswith('!')] == [line for line in output if line != '!']


def test_same_vrf_names_and_same_name_other_vrf():
    tables = config()
    tables['VRF']['VrfA'] = {}
    tables['ISIS_INSTANCE'][('default', 'EDGE')] = {'enabled': 'true', 'net_titles': [NET]}
    tables['ISIS_INSTANCE'][('VrfA', 'CORE')] = {'enabled': 'true', 'net_titles': ['49.0001.0000.0000.0002.00']}
    commands = render(tables)
    assert {'router isis CORE', 'router isis EDGE', 'router isis CORE vrf VrfA'} <= set(commands)
    assert parse(native_text(tables)).objects == desired(tables).snapshot.objects
    tables['ISIS_INSTANCE'][('default', 'EDGE')]['net_titles'] = ['49.0001.0000.0000.0002.00']
    with pytest.raises(ValueError, match='system ID'):
        desired(tables)


def test_explicit_binding_and_vrf_move_never_selects_another_parent():
    tables = config('VrfA')
    before = desired(tables).snapshot
    tables['VRF']['VrfB'] = {}
    tables['ISIS_INSTANCE'][('VrfB', 'CORE')] = {'enabled': 'true', 'net_titles': [NET]}
    tables['INTERFACE']['Ethernet0']['vrf_name'] = 'VrfB'
    after = desired(tables).snapshot
    binding = Identity('isis-binding', 'Ethernet0')
    assert binding not in after.objects
    mutation = plan(before, after, set(before.objects) | set(after.objects))
    assert 'no ip router isis CORE' in mutation.payload[0].commands
    assert 'interface Ethernet0 vrf VrfA' in mutation.payload[0].commands
    tables['ISIS_INTERFACE']['Ethernet0']['vrf'] = 'VrfB'
    assert desired(tables).snapshot.objects[binding]['process'] == 'VrfB|CORE'
    del tables['ISIS_INTERFACE']['Ethernet0']['instance']
    with pytest.raises(ValueError, match='explicit valid parent'):
        desired(tables)


def test_disabled_parent_retains_desired_values_and_reenable_restores():
    tables = config()
    tables['ISIS_INSTANCE_LEVEL'] = {('default', 'CORE', 'level-1'): {
        'lsp_refresh_interval': '100', 'lsp_maximum_lifetime': '500'}}
    original = copy.deepcopy(tables)
    before = desired(tables).snapshot
    tables['ISIS_INSTANCE'][('default', 'CORE')]['enabled'] = 'false'
    after = desired(tables).snapshot
    assert not after.objects
    mutation = plan(before, after, set(before.objects))
    assert 'no ipv6 router isis CORE' in mutation.payload[0].commands
    assert 'no router isis CORE' in mutation.payload[1].commands
    tables['ISIS_INSTANCE'][('default', 'CORE')]['enabled'] = 'true'
    assert desired(tables).snapshot.objects == before.objects
    assert tables == original


def test_levels_defaults_resets_and_atomic_lsp_command():
    tables = config()
    tables['ISIS_INSTANCE_LEVEL'] = {
        ('default', 'CORE', 'level-1'): {'lsp_refresh_interval': '100', 'lsp_maximum_lifetime': '500',
                                         'spf_minimum_interval': '7'},
        ('default', 'CORE', 'level-2'): {'lsp_generation_interval': '50'},
    }
    commands = [line.strip() for line in render(tables)]
    assert 'lsp-timers level-1 gen-interval 30 refresh-interval 100 max-lifetime 500' in commands
    assert 'lsp-timers level-2 gen-interval 50 refresh-interval 900 max-lifetime 1200' in commands
    assert 'spf-interval level-1 7' in commands
    assert 'lsp-mtu 1497' in commands
    tables['ISIS_INSTANCE'][('default', 'CORE')]['lsp_mtu'] = '1492'
    assert ' lsp-mtu 1492' in render(tables)
    del tables['ISIS_INTERFACE_LEVEL'][('Ethernet0', 'level-1')]['metric']
    assert effective('ISIS_INTERFACE_LEVEL', {})['metric'] == '10'
    assert ' isis metric level-1 10' in render(tables)
    tables['ISIS_INSTANCE_LEVEL'][('default', 'CORE', 'level-1')]['lsp_refresh_interval'] = '900'
    with pytest.raises(ValueError, match='refresh'):
        desired(tables)


def test_overload_spf_and_redistribution_roundtrip():
    tables = config()
    tables['ROUTE_MAP_SET'] = {'EXPORT': {}}
    tables['ROUTE_MAP'] = {('EXPORT', '10'): {}}
    tables['ISIS_INSTANCE'][('default', 'CORE')].update(
        overload='true', overload_on_startup='30', spf_init_delay='50', spf_short_delay='100',
        spf_long_delay='1000', spf_hold_down='10000', spf_time_to_learn='5000')
    tables['ISIS_REDISTRIBUTE'] = {('default', 'CORE', 'ipv4', 'connected', 'level-1'): {
        'metric': '20', 'route_map': 'EXPORT'}}
    before = desired(tables).snapshot
    assert parse(native_text(tables)).objects == before.objects
    assert ' redistribute ipv4 connected level-1 metric 20 route-map EXPORT' in render(tables)
    tables['ISIS_REDISTRIBUTE'].clear()
    mutation = plan(before, desired(tables).snapshot, set(before.objects))
    assert 'no redistribute ipv4 connected level-1' in mutation.payload[0].commands


@pytest.mark.parametrize('change', [
    lambda c: c['ISIS_INSTANCE'][('default', 'CORE')].update(spf_init_delay='1'),
    lambda c: c['ISIS_INSTANCE'][('default', 'CORE')].update(overload_on_startup='86401'),
    lambda c: c['ISIS_INTERFACE']['Ethernet0'].update(instance='missing'),
    lambda c: c.update(ISIS_INTERFACE_LEVEL={('missing', 'level-1'): {'metric': '10'}}),
    lambda c: c.update(ISIS_REDISTRIBUTE={('default', 'CORE', 'ipv6', 'ospf', 'level-1'): {}}),
    lambda c: c['ISIS_INSTANCE'][('default', 'CORE')].update(net_titles=['49.0001.00']),
    lambda c: c['ISIS_INSTANCE'][('default', 'CORE')].update(lsp_mtu='1497\nend'),
])
def test_invalid_graph_never_renders(change):
    tables = config()
    change(tables)
    with pytest.raises(ValueError):
        render(tables)


def test_startup_registration_then_offline_delete_uses_durable_ownership(tmp_path):
    tables = config()
    store = AtomicJSONStore(tmp_path / "owned.json")
    prepared = prepare(tables, store, clock=lambda: 1,
                       boot_id_supplier=lambda: '00000000-0000-0000-0000-000000000001')
    assert prepared['ISIS_CONFIG_LINES'] == render(tables)
    observed = desired(tables).snapshot
    kernel = RecoveryKernel(store, owner='isis-northbound-v1', clock=lambda: 2, conflict_check=conflict)
    assert kernel.step(lambda: desired(tables), lambda: observed, plan, lambda _: False, 30).status == 'applied'
    # Daemon stopped: remove all desired IS-IS rows, keep native state and journal.
    empty = copy.deepcopy(tables)
    empty['ISIS_INSTANCE'] = {}
    empty['ISIS_INTERFACE'] = {}
    empty['ISIS_INTERFACE_LEVEL'] = {}
    applied = []

    def apply(operations):
        nonlocal observed
        applied.extend(operations)
        observed = NativeSnapshot({})
        return True

    restarted = RecoveryKernel(store, owner='isis-northbound-v1', clock=lambda: 3, conflict_check=conflict)
    result = restarted.step(lambda: desired(empty), lambda: observed, plan, apply, 30)
    assert result.status == 'applied'
    assert applied[0].commands[1] == 'interface Ethernet0'
    assert 'no router isis CORE' in applied[1].commands


def test_unowned_native_objects_and_incomplete_readback_are_not_adopted():
    tables = config()
    wanted = desired(tables)
    assert conflict(wanted.snapshot, wanted.snapshot, set())
    for text in ('', 'router isis CORE\nend', 'frr version 10.7.1\nrouter isis CORE'):
        with pytest.raises(ValueError):
            parse(text)
    text = native_text(tables).replace(' metric-style wide', ' segment-routing srv6')
    assert parse(text).unsupported


@patch.dict('sys.modules', **mockmapping)
def test_native_worker_is_independent_of_bgp_callbacks():
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    daemon.isis_worker = MagicMock()
    daemon.isis_table_handler('ISIS_INSTANCE', 'default|CORE', {})
    daemon.isis_worker.notify.assert_called_once()
    daemon.isis_worker.reset_mock()
    daemon.isis_routed_interface_handler('INTERFACE', 'Ethernet0|10.0.0.1/31', {})
    daemon.isis_worker.notify.assert_not_called()
    daemon.isis_routed_interface_handler('INTERFACE', 'Ethernet0', {'vrf_name': 'VrfA'})
    daemon.isis_worker.notify.assert_called_once()
    assert all(handler == daemon.isis_table_handler for table, handler in daemon.table_handler_list
               if table.startswith('ISIS_'))


def test_shared_system_id_change_removes_all_old_areas_before_recreation():
    tables = config()
    tables['ISIS_INSTANCE'][('default', 'EDGE')] = {'enabled': 'true', 'net_titles': [NET]}
    before = desired(tables).snapshot
    for row in tables['ISIS_INSTANCE'].values():
        row['net_titles'] = ['49.0001.0000.0000.0002.00']
    after = desired(tables).snapshot
    mutation = plan(before, after, set(before.objects))
    commands = [command for op in mutation.payload for command in op.commands]
    assert commands.index('no ipv6 router isis CORE') < commands.index('no router isis CORE')
    assert max(commands.index('no router isis ' + name) for name in ('CORE', 'EDGE')) < min(
        commands.index('router isis ' + name) for name in ('CORE', 'EDGE'))
    assert commands.index('router isis EDGE') < commands.index('ip router isis CORE')
    from frrcfgd.isis_recovery import RecoveryError
    with pytest.raises(RecoveryError, match='Unowned'):
        plan(before, after, set(before.objects) - {Identity('isis-process', 'default|EDGE')})


def test_level_reduction_withdraws_policy_before_native_level_change():
    tables = config()
    tables['ISIS_REDISTRIBUTE'] = {('default', 'CORE', 'ipv4', 'connected', 'level-1'): {}}
    before = desired(tables).snapshot
    tables['ISIS_REDISTRIBUTE'].clear()
    tables['ISIS_INSTANCE'][('default', 'CORE')]['level_type'] = 'level-2-only'
    commands = plan(before, desired(tables).snapshot, set(before.objects)).payload[0].commands
    assert commands.index('no redistribute ipv4 connected level-1') < commands.index('is-type level-2-only')


def test_becoming_aggregate_member_detaches_retained_binding():
    tables = config()
    before = desired(tables).snapshot
    tables['PORTCHANNEL_MEMBER'] = {('PortChannel1', 'Ethernet0'): {}}
    after = desired(tables).snapshot
    assert Identity('isis-binding', 'Ethernet0') not in after.objects
    assert tables['ISIS_INTERFACE']['Ethernet0']['instance'] == 'CORE'
    mutation = plan(before, after, set(before.objects))
    assert 'no ip router isis CORE' in mutation.payload[0].commands


def test_template_rejects_missing_startup_preparation():
    from jinja2 import UndefinedError
    loader = FileSystemLoader(str(Path(__file__).parents[1] / 'templates/isisd'))
    template = Environment(loader=loader).get_template('isisd.conf.j2')
    with pytest.raises(UndefinedError, match='ISIS_CONFIG_LINES'):
        template.render(ISIS_INSTANCE={'default|CORE': {'enabled': 'true'}})
    assert 'router isis' not in template.render()


def test_duplicate_native_interface_readback_is_not_silently_overwritten():
    from frrcfgd.isis_recovery import RecoveryError
    text = native_text(config()).replace('end\n', 'interface Ethernet0\n description duplicate\nend\n')
    with pytest.raises(RecoveryError, match='Duplicate native interface'):
        parse(text)


@patch.dict('sys.modules', **mockmapping)
def test_hup_requests_only_isis_retry():
    import signal
    from frrcfgd import frrcfgd
    with patch.object(frrcfgd, 'BGPConfigDaemon') as daemon, \
            patch.object(frrcfgd, 'BgpdClientMgr'), \
            patch.object(frrcfgd, 'main_loop', False), \
            patch.object(frrcfgd.signal, 'signal') as register:
        frrcfgd.main()
        handler = next(call.args[1] for call in register.call_args_list if call.args[0] == signal.SIGHUP)
        handler(signal.SIGHUP, None)
        daemon.return_value.isis_worker.request_retry.assert_called_once_with()
        daemon.return_value.start.assert_called_once_with()
        daemon.return_value.stop.assert_called_once_with()


AF_STATES = ((False, False), (False, True), (True, False), (True, True))


@pytest.mark.parametrize('before_af', AF_STATES)
@pytest.mark.parametrize('after_af', AF_STATES)
@pytest.mark.parametrize('vrf', ['default', 'VrfA'])
def test_every_af_transition_preserves_circuit_settings(before_af, after_af, vrf):
    """Exercise planner transitions, including destruction when the last AF leaves.

    This checks command ordering and retained policy, not native FRR execution.
    The latter remains a separate acceptance gate.
    """
    tables = config(vrf)
    row = tables['ISIS_INTERFACE']['Ethernet0']
    row.update(ipv4_enabled=str(before_af[0]).lower(), ipv6_enabled=str(before_af[1]).lower())
    before = desired(tables).snapshot
    row.update(ipv4_enabled=str(after_af[0]).lower(), ipv6_enabled=str(after_af[1]).lower())
    after = desired(tables).snapshot
    mutation = plan(before, after, set(before.objects))
    if before_af == after_af:
        assert mutation is None
        return
    binding = Identity('isis-binding', 'Ethernet0')
    assert mutation.identities == frozenset({binding})
    assert len(mutation.payload) == 1
    commands = mutation.payload[0].commands
    expected_header = 'interface Ethernet0' + (' vrf ' + vrf if vrf != 'default' else '')
    assert commands[1] == expected_header
    for enabled, prefix in zip(after_af, ('ip', 'ipv6')):
        assert (('' if enabled else 'no ') + prefix + ' router isis CORE') in commands
    if any(after_af):
        # FRR may recreate the circuit when switching away from the last active
        # family. All retained policy must follow the new family binding.
        last_bind = max(commands.index(prefix + ' router isis CORE')
                        for enabled, prefix in zip(after_af, ('ip', 'ipv6')) if enabled)
        for setting in ('isis passive', 'isis network point-to-point',
                        'isis metric level-1 30', 'isis metric level-2 10'):
            assert commands.index(setting) > last_bind
        assert after.objects[binding]['levels']['level-1']['metric'] == '30'
    else:
        assert binding not in after.objects
        assert not any(command.startswith('isis ') for command in commands)
        assert row['passive'] == 'true' and row['instance'] == 'CORE'
