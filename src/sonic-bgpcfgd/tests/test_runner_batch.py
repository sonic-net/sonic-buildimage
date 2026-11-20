from collections import deque
from itertools import permutations
import os
from unittest.mock import MagicMock

import pytest

from . import swsscommon_test
from .util import load_constants, render_constants
from bgpcfgd.directory import Directory
from bgpcfgd.managers_bgp import BGPPeerMgrBase
from bgpcfgd.managers_db import BGPDataBaseMgr
from bgpcfgd.managers_intf import InterfaceMgr
from bgpcfgd.template import TemplateFabric
import bgpcfgd.runner as runner_module
from swsscommon import swsscommon


def swsscommon_bindings():
    # Collection can bind the modules under test to different swsscommon mocks, so patch each one they use.
    import bgpcfgd.manager
    import bgpcfgd.managers_bbr
    import bgpcfgd.managers_bgp
    import bgpcfgd.managers_db
    import bgpcfgd.managers_device_global
    bindings = {id(swsscommon): swsscommon}
    for module in (bgpcfgd.manager, bgpcfgd.managers_bbr, bgpcfgd.managers_bgp, bgpcfgd.managers_db,
                   bgpcfgd.managers_device_global, runner_module):
        bindings.setdefault(id(module.swsscommon), module.swsscommon)
    return list(bindings.values())


def make_peers(monkeypatch):
    for binding in swsscommon_bindings():
        # Scheduling tests use the repository's validator mock independently of the installed binding version.
        for name in ('isVrfNameValid', 'isInterfaceNameValid'):
            monkeypatch.setattr(binding, name, getattr(swsscommon_test.swsscommon, name), raising=False)
        for name, value in {
            'SET_COMMAND': 'SET',
            'DEL_COMMAND': 'DEL',
            'CFG_DEVICE_METADATA_TABLE_NAME': 'DEVICE_METADATA',
            'CFG_DEVICE_NEIGHBOR_METADATA_TABLE_NAME': 'DEVICE_NEIGHBOR_METADATA',
            'CFG_PORT_TABLE_NAME': 'PORT',
            'CFG_BGP_DEVICE_GLOBAL_TABLE_NAME': 'BGP_DEVICE_GLOBAL',
            'CFG_LOOPBACK_INTERFACE_TABLE_NAME': 'LOOPBACK_INTERFACE',
            'CFG_BGP_NEIGHBOR_TABLE_NAME': 'BGP_NEIGHBOR',
        }.items():
            monkeypatch.setattr(binding, name, value)
    monkeypatch.setattr(BGPPeerMgrBase, 'load_peers', staticmethod(lambda: set()))
    monkeypatch.setattr('bgpcfgd.managers_bgp.log_debug', lambda msg: None)
    monkeypatch.setattr('bgpcfgd.managers_bgp.log_info', lambda msg: None)
    monkeypatch.setattr('bgpcfgd.manager.log_debug', lambda msg: None)
    monkeypatch.setattr(runner_module, 'log_debug', lambda msg: None)
    common = {'directory': Directory(), 'cfg_mgr': MagicMock(),
              'tf': TemplateFabric(os.path.abspath('../../dockers/docker-fpm-frr/frr')),
              'constants': load_constants(render_constants())['constants']}
    directory = common['directory']
    directory.put('CONFIG_DB', 'DEVICE_METADATA', 'localhost',
                  {'bgp_asn': '65100', 'type': 'ToRRouter', 'deployment_id': '1'})
    directory.put('CONFIG_DB', 'BGP_DEVICE_GLOBAL', 'tsa_enabled', 'false')
    directory.put('CONFIG_DB', 'BGP_DEVICE_GLOBAL', 'idf_isolation_state', 'unisolated')
    directory.put('CONFIG_DB', 'PORT', 'Ethernet0', {})
    loopback = InterfaceMgr(common, 'CONFIG_DB', 'LOOPBACK_INTERFACE')
    loopback.set_handler('Loopback0', {'NULL': 'NULL'})
    loopback.set_handler('Loopback0|192.0.2.1/32', {'NULL': 'NULL'})
    peer = BGPPeerMgrBase(common, 'CONFIG_DB', 'BGP_NEIGHBOR', 'general', False)
    interface = InterfaceMgr(common, 'CONFIG_DB', 'INTERFACE')
    return common, peer, interface


def run_passes(monkeypatch, common, managers, passes):
    subscribers = {}

    class Db:
        def __init__(self, *args):
            pass

        def getDbId(self):
            return 4

    class Subscriber:
        def __init__(self, db, table):
            self.db, self.table = db, table
            self.events = deque()
            subscribers[table] = self

        def getDbConnector(self):
            return self.db

        def getTableName(self):
            return self.table

        def pop(self):
            return self.events.popleft() if self.events else ('', '', ())

    class Selector:
        TIMEOUT, ERROR = 0, 1

        def addSelectable(self, subscriber):
            pass

        def select(self, timeout):
            current = next(events)
            runner.subscribers = [subscribers[table] for table in current]
            for table, messages in current.items():
                subscribers[table].events.extend(messages)
            return 2, None

    for binding in swsscommon_bindings():
        monkeypatch.setattr(binding, 'DBConnector', Db)
        monkeypatch.setattr(binding, 'SubscriberStateTable', Subscriber)
        monkeypatch.setattr(binding, 'Select', Selector)
        monkeypatch.setattr(binding.SonicDBConfig, 'getDbId', lambda name: 4)
    monkeypatch.setattr(runner_module, 'g_run', True)
    runner = runner_module.Runner(common['cfg_mgr'])
    for manager in managers:
        runner.add_manager(manager)
    events = iter(passes)
    committed = []

    def commit():
        committed.append({manager.table_name: len(getattr(manager, 'peers', ())) for manager in managers})
        if len(committed) == len(passes):
            runner_module.g_run = False
        return True

    cfg_mgr = common['cfg_mgr']
    if isinstance(cfg_mgr, MagicMock):
        cfg_mgr.commit.side_effect = commit
        runner.run()
        assert cfg_mgr.commit.call_count == len(passes)
    else:
        real_commit = cfg_mgr.commit
        runner.cfg_manager = MagicMock(before_flush=cfg_mgr.before_flush)
        runner.cfg_manager.commit.side_effect = lambda: real_commit() and commit()
        runner.run()
    return committed


def event(key, data=None, op='SET'):
    return key, op, tuple((data or {}).items())


def peer_data(local):
    return {'local_addr': local, 'asn': '65200', 'name': 'TOR', 'rrclient': '0', 'nhopself': '0'}


@pytest.mark.parametrize('peers_first', [True, False])
def test_scale_retries_are_bounded_independently_of_table_order(monkeypatch, peers_first):
    common, peer, interface = make_peers(monkeypatch)
    peers, interfaces = [], []
    for number in range(926):
        if number < 924:
            local = '10.{}.{}.0'.format(number // 256, number % 256)
            peers.append(event(local[:-1] + '1', peer_data(local)))
            interface_name = 'Ethernet{}'.format((number // 2) * 4)
            if number % 2 == 0:
                interfaces.append(event(interface_name, {'NULL': 'NULL'}))
            interfaces.append(event('{}|{}/31'.format(interface_name, local), {'NULL': 'NULL'}))
        else:
            peers.append(event('203.0.113.{}'.format(number - 923), peer_data('192.0.2.1')))
    assert len(interfaces) == 1386
    original_set = peer.set_handler
    calls = []

    def counted(key, data):
        calls.append(key)
        assert len(calls) <= 2 * len(peers), 'dependency retries amplified the first pass'
        return original_set(key, data)

    peer.set_handler = counted
    order = [('BGP_NEIGHBOR', peers), ('INTERFACE', interfaces)]
    if not peers_first:
        order.reverse()
    committed = run_passes(monkeypatch, common, [peer, interface], [dict(order)])
    assert committed[-1]['BGP_NEIGHBOR'] == 926
    assert len(calls) <= 1852
    assert not peer.set_queue


def test_later_interface_resolves_peer_in_the_same_commit(monkeypatch):
    common, peer, interface = make_peers(monkeypatch)
    committed = run_passes(monkeypatch, common, [peer, interface], [
        {'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))]},
        {'INTERFACE': [event('Ethernet0', {'NULL': 'NULL'}),
                       event('Ethernet0|10.1.0.0/31', {'NULL': 'NULL'})]},
    ])
    assert [state['BGP_NEIGHBOR'] for state in committed] == [0, 1]
    assert not peer.set_queue


def test_deleted_interface_is_not_used_for_new_peer(monkeypatch):
    common, peer, interface = make_peers(monkeypatch)
    interface.set_handler('Ethernet0', {'NULL': 'NULL'})
    interface.set_handler('Ethernet0|10.1.0.0/31', {'NULL': 'NULL'})
    committed = run_passes(monkeypatch, common, [peer, interface], [{
        'INTERFACE': [event('Ethernet0|10.1.0.0/31', op='DEL'), event('Ethernet0', op='DEL')],
        'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))],
    }])
    assert committed[-1]['BGP_NEIGHBOR'] == 0
    assert any(key == '10.1.0.1' for key, _ in peer.set_queue)


def test_queue_cancellation_and_unnumbered_peers(monkeypatch):
    common, peer, interface = make_peers(monkeypatch)
    committed = run_passes(monkeypatch, common, [peer, interface], [{
        'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0')),
                         event('10.1.0.1', op='DEL'),
                         event('Ethernet0', {'asn': '65200', 'name': 'TOR'})],
        'INTERFACE': [event('Ethernet0', {'NULL': 'NULL'}),
                       event('Ethernet0|10.1.0.0/31', {'NULL': 'NULL'})],
    }])
    assert committed[-1]['BGP_NEIGHBOR'] == 1
    assert peer.peers == {('default', 'Ethernet0')}
    assert not peer.set_queue


def test_non_peer_dependency_callbacks_run_during_the_drain(monkeypatch):
    common, peer, interface = make_peers(monkeypatch)
    database = BGPDataBaseMgr(common, 'CONFIG_DB', 'OTHER')
    observed = []
    common['directory'].subscribe([('CONFIG_DB', 'OTHER', '')],
                                  lambda: observed.append(common['cfg_mgr'].commit.call_count))
    run_passes(monkeypatch, common, [database, peer, interface], [
        {'OTHER': [event('first', {'value': 1}), event('second', {'value': 2})]},
    ])
    assert observed == [0, 0]


def test_deferred_peer_uses_latest_global_state_and_initializes_once(monkeypatch):
    common, peer, interface = make_peers(monkeypatch)
    initialized = MagicMock(wraps=peer.post_dependencies_init)
    peer.post_dependencies_init = initialized
    rendered_states = []
    original_update = peer.peer_group_mgr.update

    def group_update(tag, **kwargs):
        rendered_states.append(kwargs['CONFIG_DB__BGP_DEVICE_GLOBAL']['tsa_enabled'])
        return original_update(tag, **kwargs)

    peer.peer_group_mgr.update = group_update
    common['directory'].subscribe([('LOCAL', 'interfaces', '')],
                                  lambda: common['directory'].put('CONFIG_DB', 'BGP_DEVICE_GLOBAL',
                                                                  'tsa_enabled', 'true'))
    run_passes(monkeypatch, common, [peer, interface], [{
        'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))],
        'INTERFACE': [event('Ethernet0', {'NULL': 'NULL'}),
                       event('Ethernet0|10.1.0.0/31', {'NULL': 'NULL'})],
    }])
    assert rendered_states == ['true']
    initialized.assert_called_once()
    commands = '\n'.join(call.args[0] for call in common['cfg_mgr'].push.call_args_list)
    assert commands.index('route-map FROM_BGP_PEER_V4') < commands.index('neighbor 10.1.0.1 peer-group')


class AccumulatingFrr:
    """FRR stand-in whose running config is everything written so far."""

    def __init__(self):
        self.config = ''

    def write(self, text):
        self.config += text + '\n'
        return True

    def get_config(self):
        return self.config

    def restart_peer_groups(self, peer_groups):
        return True


class RouteMapFrr(AccumulatingFrr):
    """FRR stand-in that keeps one entry per route-map sequence, with the last clause of each kind,
    drops an entry on its `no` line and records the peer groups each commit restarts; everything else
    accumulates as written (a `no` line of a prefix list or a community list is kept, not applied)."""

    def __init__(self):
        super().__init__()
        self.other = []
        self.route_maps = {}  # (name, action, seq) -> {clause kind: line}
        self.restarts = []  # the peer groups restarted at each commit
        self.refuse_writes = 0  # the next writes to refuse, as vtysh -f refusing a batch

    def restart_peer_groups(self, peer_groups):
        self.restarts.append(sorted(peer_groups))
        return True

    def write(self, text):
        if self.refuse_writes:
            self.refuse_writes -= 1
            return False
        entry = None
        for line in text.split('\n'):
            words = line.split()
            if not words:
                continue
            if words[0] == 'route-map' and len(words) == 4:
                entry = self.route_maps.setdefault(tuple(words[1:]), {})
            elif words[:2] == ['no', 'route-map'] and len(words) == 5:
                self.route_maps.pop(tuple(words[2:]), None)
                entry = None
            elif entry is not None and line.startswith(' '):
                entry[' '.join(words[:2])] = line
            else:
                entry = None
                self.other.append(line)
        self.config = '\n'.join(self.other + [
            '\n'.join(['route-map %s %s %s' % key] + list(clauses.values()))
            for key, clauses in self.route_maps.items()]) + '\n'
        return True


def make_frr_backed(monkeypatch, switch_type, frr=None):
    from bgpcfgd.config import ConfigMgr
    from bgpcfgd.managers_device_global import DeviceGlobalCfgMgr
    common, peer, interface = make_peers(monkeypatch)
    frr = frr or AccumulatingFrr()
    common['cfg_mgr'] = ConfigMgr(frr)
    common['directory'].put('CONFIG_DB', 'DEVICE_METADATA', 'localhost',
                            {'bgp_asn': '65100', 'type': switch_type, 'deployment_id': '1'})
    monkeypatch.setattr('bgpcfgd.managers_device_global.DeviceGlobalCfgMgr.prime_confed_from_config_db',
                        lambda self: None)
    monkeypatch.setattr('bgpcfgd.managers_device_global.DeviceGlobalCfgMgr.get_chassis_tsa_status',
                        lambda self: 'false', raising=False)
    device_global = DeviceGlobalCfgMgr(common, 'CONFIG_DB', 'BGP_DEVICE_GLOBAL')
    return common, frr, device_global, interface


def test_peer_deferred_earlier_in_a_pass_is_in_frr_when_tsa_reads_it(monkeypatch):
    from bgpcfgd.managers_bgp import BGPPeerMgrBase
    common, frr, device_global, interface = make_frr_backed(monkeypatch, 'ToRRouter')
    peer = BGPPeerMgrBase(common, 'CONFIG_DB', 'BGP_NEIGHBOR', 'general', False)
    seen_by_tsa = []
    isolate = device_global.isolate_unisolate_device

    def recording_isolate(state):
        seen_by_tsa.append(frr.config)
        return isolate(state)

    device_global.isolate_unisolate_device = recording_isolate
    run_passes(monkeypatch, common, [peer, interface, device_global], [{
        'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))],
        'INTERFACE': [event('Ethernet0', {'NULL': 'NULL'}),
                       event('Ethernet0|10.1.0.0/31', {'NULL': 'NULL'})],
        'BGP_DEVICE_GLOBAL': [event('STATE', {'tsa_enabled': 'true'})],
    }])
    assert len(seen_by_tsa) == 1
    # TSA builds its isolation from the neighbors it reads back from FRR
    assert 'neighbor 10.1.0.1 peer-group PEER_V4' in seen_by_tsa[0]
    assert 'route-map TO_BGP_PEER_V4 deny 40' in frr.config


def test_bbr_sees_peer_groups_of_peers_deferred_in_the_same_pass(monkeypatch):
    from bgpcfgd.managers_bbr import BBRMgr
    common, frr, device_global, interface = make_frr_backed(monkeypatch, 'SpineRouter')
    monkeypatch.setattr('bgpcfgd.managers_bbr.BBRMgr.get_bbr_status_from_config_db', lambda self: None)
    common['constants'].setdefault('bgp', {}).setdefault('bbr', {})['enabled'] = True
    bbr = BBRMgr(common, 'CONFIG_DB', 'BGP_BBR')
    from bgpcfgd.managers_bgp import BGPPeerMgrBase
    peer = BGPPeerMgrBase(common, 'CONFIG_DB', 'BGP_NEIGHBOR', 'general', False)
    run_passes(monkeypatch, common, [peer, interface, device_global, bbr], [{
        'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))],
        'INTERFACE': [event('Ethernet0', {'NULL': 'NULL'}),
                       event('Ethernet0|10.1.0.0/31', {'NULL': 'NULL'})],
        'BGP_DEVICE_GLOBAL': [event('STATE', {'tsa_enabled': 'true'})],
        'BGP_BBR': [event('all', {'status': 'enabled'})],
    }])
    assert 'neighbor 10.1.0.1 peer-group PEER_V4' in frr.config
    assert 'neighbor PEER_V4 allowas-in 1' in frr.config



@pytest.mark.parametrize('interfaces_first', [True, False])
def test_newer_set_in_the_unblocking_pass_is_applied_last(monkeypatch, interfaces_first):
    common, peer, interface = make_peers(monkeypatch)
    older = dict(peer_data('10.1.0.0'), admin_status='up')
    newer = dict(peer_data('10.1.0.0'), admin_status='down')
    unblocking = {'INTERFACE': [event('Ethernet0', {'NULL': 'NULL'}),
                                event('Ethernet0|10.1.0.0/31', {'NULL': 'NULL'})],
                  'BGP_NEIGHBOR': [event('10.1.0.1', newer)]}
    if not interfaces_first:
        unblocking = dict(reversed(list(unblocking.items())))
    run_passes(monkeypatch, common, [peer, interface], [
        {'BGP_NEIGHBOR': [event('10.1.0.1', older)]},
        unblocking,
    ])
    assert ('default', '10.1.0.1') in peer.peers
    assert common['directory'].get('CONFIG_DB', 'BGP_NEIGHBOR', 'default|10.1.0.1')['admin_status'] == 'down'
    commands = '\n'.join(call.args[0] for call in common['cfg_mgr'].push.call_args_list)
    admin_lines = [line.strip() for line in commands.splitlines() if line.strip() in (
        'neighbor 10.1.0.1 shutdown', 'no neighbor 10.1.0.1 shutdown')]
    assert admin_lines[-1] == 'neighbor 10.1.0.1 shutdown'
    assert not peer.set_queue


@pytest.mark.parametrize('order', list(permutations(('new_local', 'replacement', 'old_local'))))
def test_deferred_replacement_asn_is_used_for_creation(monkeypatch, order):
    common, peer, interface = make_peers(monkeypatch)
    vlan = InterfaceMgr(common, 'CONFIG_DB', 'VLAN_INTERFACE')
    key = '198.51.100.253'
    old = peer_data('192.0.2.10')
    new = dict(peer_data('192.0.2.20'), asn='65300')
    events = {
        'new_local': ('VLAN_INTERFACE', [event('Vlan3999', {'NULL': 'NULL'}),
                      event('Vlan3999|192.0.2.20/31', {'NULL': 'NULL'})]),
        'replacement': ('BGP_NEIGHBOR', [event(key, new)]),
        'old_local': ('INTERFACE', [event('Ethernet0', {'NULL': 'NULL'}),
                      event('Ethernet0|192.0.2.10/31', {'NULL': 'NULL'})]),
    }
    run_passes(monkeypatch, common, [peer, interface, vlan], [
        {'BGP_NEIGHBOR': [event(key, old)]},
        dict(events[item] for item in order),
    ])
    commands = '\n'.join(call.args[0] for call in common['cfg_mgr'].push.call_args_list)
    assert 'neighbor {} remote-as 65300'.format(key) in commands
    assert 'neighbor {} remote-as 65200'.format(key) not in commands
    assert common['directory'].get('CONFIG_DB', 'BGP_NEIGHBOR', 'default|' + key) == new
    assert not peer.set_queue


def test_replacement_waits_for_its_own_local_address(monkeypatch):
    common, peer, interface = make_peers(monkeypatch)
    key = '198.51.100.253'
    new = dict(peer_data('192.0.2.20'), asn='65300')
    states = run_passes(monkeypatch, common, [peer, interface], [
        {'BGP_NEIGHBOR': [event(key, peer_data('192.0.2.10'))]},
        {'BGP_NEIGHBOR': [event(key, new)],
         'INTERFACE': [event('Ethernet0', {'NULL': 'NULL'}),
                       event('Ethernet0|192.0.2.10/31', {'NULL': 'NULL'})]},
        {'INTERFACE': [event('Ethernet0|192.0.2.20/31', {'NULL': 'NULL'})]},
    ])
    assert [state['BGP_NEIGHBOR'] for state in states] == [0, 0, 1]
    commands = '\n'.join(call.args[0] for call in common['cfg_mgr'].push.call_args_list)
    assert 'neighbor {} remote-as 65300'.format(key) in commands
    assert 'neighbor {} remote-as 65200'.format(key) not in commands
    assert not peer.set_queue


@pytest.mark.parametrize('across_passes', [False, True])
def test_delete_then_set_recreates_an_existing_peer(monkeypatch, across_passes):
    common, peer, interface = make_peers(monkeypatch)
    key = '198.51.100.253'
    old = peer_data('192.0.2.1')
    new = dict(old, asn='65300')
    changes = [event(key, op='DEL'), event(key, new)]
    passes = [{'BGP_NEIGHBOR': [event(key, old)]}]
    passes += ([{'BGP_NEIGHBOR': [change]} for change in changes]
               if across_passes else [{'BGP_NEIGHBOR': changes}])
    run_passes(monkeypatch, common, [peer, interface], passes)
    commands = '\n'.join(call.args[0] for call in common['cfg_mgr'].push.call_args_list)
    assert commands.index('neighbor {} remote-as 65200'.format(key)) < commands.index('no neighbor ' + key)
    assert commands.index('no neighbor ' + key) < commands.index('neighbor {} remote-as 65300'.format(key))
    assert not peer.set_queue


def test_mid_pass_commit_replays_the_replacement_before_tsa_reads(monkeypatch):
    common, frr, device_global, interface = make_frr_backed(monkeypatch, 'ToRRouter')
    peer = BGPPeerMgrBase(common, 'CONFIG_DB', 'BGP_NEIGHBOR', 'general', False)
    key = '198.51.100.253'
    new = dict(peer_data('192.0.2.20'), asn='65300')
    seen = []
    isolate = device_global.isolate_unisolate_device

    def record(state):
        seen.append(frr.config)
        return isolate(state)

    device_global.isolate_unisolate_device = record
    run_passes(monkeypatch, common, [peer, interface, device_global], [
        {'BGP_NEIGHBOR': [event(key, peer_data('192.0.2.10'))]},
        {'BGP_NEIGHBOR': [event(key, new)],
         'INTERFACE': [event('Ethernet0', {'NULL': 'NULL'}),
                       event('Ethernet0|192.0.2.10/31', {'NULL': 'NULL'}),
                       event('Ethernet0|192.0.2.20/31', {'NULL': 'NULL'})],
         'BGP_DEVICE_GLOBAL': [event('STATE', {'tsa_enabled': 'true'})]},
    ])
    assert len(seen) == 1
    assert 'neighbor {} remote-as 65300'.format(key) in seen[0]
    assert 'neighbor {} remote-as 65200'.format(key) not in frr.config
    assert not peer.set_queue


@pytest.mark.parametrize('in_batch', [False, True])
@pytest.mark.parametrize('bad_name', ['TOR\nSECOND', 'TOR\r\nSECOND', 42])
def test_rejected_replacement_cancels_pending_peer(monkeypatch, in_batch, bad_name):
    common, peer, interface = make_peers(monkeypatch)
    key, other = '198.51.100.253', '198.51.100.254'
    peer.handler(key, 'SET', peer_data('192.0.2.10'))
    peer.handler(other, 'SET', peer_data('192.0.2.10'))
    assert len(peer.set_queue) == 2
    if in_batch:
        peer.begin_batch()
    peer.handler(key, 'SET', dict(peer_data('192.0.2.20'), asn='65300', name=bad_name))
    interface.handler('Ethernet0', 'SET', {'NULL': 'NULL'})
    interface.handler('Ethernet0|192.0.2.10/31', 'SET', {'NULL': 'NULL'})
    peer.end_batch()
    assert peer.peers == {('default', other)}
    assert not peer.set_queue
    commands = '\n'.join(call.args[0] for call in common['cfg_mgr'].push.call_args_list)
    assert 'neighbor {} remote-as'.format(key) not in commands
    peer.handler(key, 'SET', dict(peer_data('192.0.2.1'), asn='65300'))
    assert ('default', key) in peer.peers
    commands = '\n'.join(call.args[0] for call in common['cfg_mgr'].push.call_args_list)
    assert 'neighbor {} remote-as 65300'.format(key) in commands


@pytest.mark.parametrize('bbr_first', [True, False])
def test_bbr_status_from_the_unblocking_pass_reaches_deferred_leaf_peers(monkeypatch, bbr_first):
    from bgpcfgd.managers_bbr import BBRMgr
    common, frr, device_global, interface = make_frr_backed(monkeypatch, 'LeafRouter')
    monkeypatch.setattr('bgpcfgd.managers_bbr.BBRMgr.get_bbr_status_from_config_db', lambda self: None)
    common['constants'].setdefault('bgp', {}).setdefault('bbr', {})['enabled'] = True
    bbr = BBRMgr(common, 'CONFIG_DB', 'BGP_BBR')
    peer = BGPPeerMgrBase(common, 'CONFIG_DB', 'BGP_NEIGHBOR', 'general', False)
    unblocking = {'INTERFACE': [event('Ethernet0', {'NULL': 'NULL'}),
                                event('Ethernet0|10.1.0.0/31', {'NULL': 'NULL'})],
                  'BGP_BBR': [event('all', {'status': 'enabled'})]}
    if bbr_first:
        unblocking = dict(reversed(list(unblocking.items())))
    run_passes(monkeypatch, common, [peer, interface, device_global, bbr], [
        {'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))]},
        unblocking,
    ])
    assert 'neighbor 10.1.0.1 peer-group PEER_V4' in frr.config
    assert 'neighbor PEER_V4 allowas-in 1' in frr.config


def last_allow_list_default(config, af):
    """The community the last pushed default rule of deployment 0's allow-list route-map sets."""
    header = 'route-map ALLOW_LIST_DEPLOYMENT_ID_0_%s permit 65535' % af
    lines = config.split('\n')
    last = None
    for i, line in enumerate(lines):
        if line.strip() == header:
            for follow in lines[i + 1:i + 4]:
                if follow.strip().startswith('set community '):
                    last = follow.split()[2]
                    break
    return last


@pytest.mark.parametrize('order', ['interfaces_first', 'allow_list_first', 'allow_list_in_an_earlier_pass',
                                   'peer_first_same_pass', 'allow_list_first_same_pass'])
def test_peer_render_repeats_the_allow_list_default_action(monkeypatch, order):
    from bgpcfgd.managers_allow_list import BGPAllowListMgr
    common, frr, device_global, interface = make_frr_backed(monkeypatch, 'ToRRouter')
    common['constants']['bgp']['allow_list']['enabled'] = True
    allow_list = BGPAllowListMgr(common, 'CONFIG_DB', 'BGP_ALLOWED_PREFIXES')
    peer = BGPPeerMgrBase(common, 'CONFIG_DB', 'BGP_NEIGHBOR', 'general', False)
    interfaces = [event('Ethernet0', {'NULL': 'NULL'}), event('Ethernet0|10.1.0.0/31', {'NULL': 'NULL'})]
    deny = [event('DEPLOYMENT_ID|0', {'prefixes_v4': '10.0.0.0/24', 'default_action': 'deny'})]
    if order == 'interfaces_first':
        passes = [{'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))]},
                  {'INTERFACE': interfaces, 'BGP_ALLOWED_PREFIXES': deny}]
    elif order == 'allow_list_first':
        passes = [{'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))]},
                  {'BGP_ALLOWED_PREFIXES': deny, 'INTERFACE': interfaces}]
    elif order == 'allow_list_in_an_earlier_pass':
        passes = [{'INTERFACE': interfaces}, {'BGP_ALLOWED_PREFIXES': deny},
                  {'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))]}]
    elif order == 'peer_first_same_pass':
        passes = [{'INTERFACE': interfaces},
                  {'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))], 'BGP_ALLOWED_PREFIXES': deny}]
    else:
        passes = [{'INTERFACE': interfaces},
                  {'BGP_ALLOWED_PREFIXES': deny, 'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))]}]
    run_passes(monkeypatch, common, [peer, interface, device_global, allow_list], passes)
    assert 'neighbor 10.1.0.1 peer-group PEER_V4' in frr.config
    # The policy template renders deployment 0's default rule; a peer rendered after the allow-list
    # manager applied 'deny' repeats 'deny' instead of the template's initial value
    assert last_allow_list_default(frr.config, 'V4') == 'no-export'
    assert last_allow_list_default(frr.config, 'V6') == 'no-export'


@pytest.mark.parametrize('constants_default', ['deny', 'permit'])
def test_peer_render_before_any_row_keeps_the_template_initial_value(monkeypatch, constants_default):
    from bgpcfgd.managers_allow_list import BGPAllowListMgr
    common, frr, device_global, interface = make_frr_backed(monkeypatch, 'ToRRouter')
    common['constants']['bgp']['allow_list']['enabled'] = True
    common['constants']['bgp']['allow_list']['default_action'] = constants_default
    allow_list = BGPAllowListMgr(common, 'CONFIG_DB', 'BGP_ALLOWED_PREFIXES')
    peer = BGPPeerMgrBase(common, 'CONFIG_DB', 'BGP_NEIGHBOR', 'general', False)
    run_passes(monkeypatch, common, [peer, interface, device_global, allow_list], [{
        'INTERFACE': [event('Ethernet0', {'NULL': 'NULL'}), event('Ethernet0|10.1.0.0/31', {'NULL': 'NULL'})],
        'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))],
    }])
    # FRR held no rule and no row was applied: the render writes the template's initial value, the
    # drop community, whatever the constants' default action, as it did before the record existed
    drop = common['constants']['bgp']['allow_list']['drop_community']
    assert last_allow_list_default(frr.config, 'V4') == drop
    assert last_allow_list_default(frr.config, 'V6') == drop


def allow_list_runner(monkeypatch, constants_default='permit', running=None, frr=None):
    """Managers for the allow-list rule tests, on an FRR that may already hold deployment 0's rule."""
    from bgpcfgd.managers_allow_list import BGPAllowListMgr
    common, frr, device_global, interface = make_frr_backed(monkeypatch, 'ToRRouter', frr or RouteMapFrr())
    common['constants']['bgp']['allow_list']['enabled'] = True
    common['constants']['bgp']['allow_list']['default_action'] = constants_default
    if running:
        for af in ('V4', 'V6'):
            frr.write('route-map ALLOW_LIST_DEPLOYMENT_ID_0_%s permit 65535\n set community %s additive\n'
                      % (af, running))
    allow_list = BGPAllowListMgr(common, 'CONFIG_DB', 'BGP_ALLOWED_PREFIXES')
    peer = BGPPeerMgrBase(common, 'CONFIG_DB', 'BGP_NEIGHBOR', 'general', False)
    managers = [peer, interface, device_global, allow_list]
    return common, frr, managers


INTERFACES = [event('Ethernet0', {'NULL': 'NULL'}), event('Ethernet0|10.1.0.0/31', {'NULL': 'NULL'})]
DROP = '5060:12345'


def row(key='DEPLOYMENT_ID|0', **fields):
    return event(key, dict({'prefixes_v4': '10.0.0.0/24'}, **fields))


def rule(frr, af='V4'):
    return last_allow_list_default(frr.config, af)


@pytest.mark.parametrize('running, constants_default, action, expected', [
    ('no-export', 'permit', 'deny', 'no-export'),   # FRR kept the rule from an earlier bgpcfgd; the peer is new to FRR
    (DROP, 'deny', 'permit', DROP),
])
def test_allow_list_rule_is_written_after_a_peer_rendered_in_the_same_pass(monkeypatch, running,
                                                                            constants_default, action, expected):
    common, frr, managers = allow_list_runner(monkeypatch, constants_default, running)
    # a fresh process starts its record from the rule FRR holds: the peer renders it, then the row,
    # whose value FRR already holds, is drained
    run_passes(monkeypatch, common, managers, [
        {'INTERFACE': INTERFACES, 'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))],
         'BGP_ALLOWED_PREFIXES': [row(default_action=action)]},
    ])
    assert rule(frr) == expected and rule(frr, 'V6') == expected


@pytest.mark.parametrize('change', ['permit', 'delete'])
def test_allow_list_rule_follows_a_row_changed_after_a_peer_rendered_in_the_same_pass(monkeypatch, change):
    common, frr, managers = allow_list_runner(monkeypatch)
    second = [row(default_action='permit')] if change == 'permit' else [event('DEPLOYMENT_ID|0', op='DEL')]
    run_passes(monkeypatch, common, managers, [
        {'INTERFACE': INTERFACES, 'BGP_ALLOWED_PREFIXES': [row(default_action='deny')]},
        {'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))], 'BGP_ALLOWED_PREFIXES': second},
    ])
    # the peer repeated deny from the record; the row drained after it sets the constants' permit
    assert rule(frr) == DROP and rule(frr, 'V6') == DROP
    assert common['directory'].get('LOCAL', 'allow_list_default_action', 'ALLOW_LIST_DEPLOYMENT_ID_0_V4') == 'permit'


def test_two_policies_of_one_deployment_in_one_pass_end_with_the_last(monkeypatch):
    common, frr, managers = allow_list_runner(monkeypatch, running=DROP)
    run_passes(monkeypatch, common, managers, [{
        'BGP_ALLOWED_PREFIXES': [row(default_action='deny'), row('DEPLOYMENT_ID|0|1010:2020')],
    }])
    # the second policy applies the constants' default although FRR already held it when it was drained
    assert rule(frr) == DROP
    assert common['directory'].get('LOCAL', 'allow_list_default_action', 'ALLOW_LIST_DEPLOYMENT_ID_0_V4') == 'permit'


def test_delete_then_set_of_a_row_in_one_pass_leaves_the_default_rule_as_set(monkeypatch):
    common, frr, managers = allow_list_runner(monkeypatch, running='no-export')
    run_passes(monkeypatch, common, managers, [{
        'BGP_ALLOWED_PREFIXES': [event('DEPLOYMENT_ID|0', op='DEL'), row(default_action='deny')],
    }])
    # the default rule follows the SET; the policy's other entries are compared with the running
    # configuration alone, as before this series, and are not the subject of this test
    assert rule(frr) == 'no-export'
    assert common['directory'].get('LOCAL', 'allow_list_default_action', 'ALLOW_LIST_DEPLOYMENT_ID_0_V4') == 'deny'


def test_unchanged_policy_after_a_peer_render_with_its_action_writes_nothing(monkeypatch):
    common, frr, managers = allow_list_runner(monkeypatch)
    run_passes(monkeypatch, common, managers, [
        {'INTERFACE': INTERFACES, 'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))],
         'BGP_ALLOWED_PREFIXES': [row(default_action='deny')]},
        {'INTERFACE': [event('Ethernet4', {'NULL': 'NULL'}), event('Ethernet4|10.1.0.2/31', {'NULL': 'NULL'})],
         'BGP_NEIGHBOR': [event('10.1.0.3', peer_data('10.1.0.2'))],
         'BGP_ALLOWED_PREFIXES': [row(default_action='deny')]},
    ])
    assert rule(frr) == 'no-export'
    # the second peer rendered the policy's own action before the unchanged row was drained: the row
    # writes the rule again only when the pending value differs, so no peer group is restarted
    assert frr.restarts == [[], []]


@pytest.mark.parametrize('row_first', [True, False])
def test_allow_list_rule_with_a_tsa_commit_between_the_peer_render_and_the_row(monkeypatch, row_first):
    common, frr, managers = allow_list_runner(monkeypatch)
    tables = {'INTERFACE': INTERFACES,
              'BGP_ALLOWED_PREFIXES': [row(default_action='deny')],
              'BGP_DEVICE_GLOBAL': [event('STATE', {'tsa_enabled': 'true'})]}
    if not row_first:
        tables = {k: tables[k] for k in ('INTERFACE', 'BGP_DEVICE_GLOBAL', 'BGP_ALLOWED_PREFIXES')}
    run_passes(monkeypatch, common, managers, [
        {'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))]},
        tables,
    ])
    # the TSA commit flushes the deferred peer mid-pass; the rule ends with the row's action either way
    assert 'neighbor 10.1.0.1 peer-group PEER_V4' in frr.config
    assert rule(frr) == 'no-export' and rule(frr, 'V6') == 'no-export'


@pytest.mark.parametrize('second_row', ['unchanged', 'changed'])
def test_allow_list_rule_after_a_restart_of_bgpcfgd_alone(monkeypatch, second_row):
    # life 1: a peer and a deny policy are committed, so FRR holds the peer groups and the rule
    common, frr, managers = allow_list_runner(monkeypatch)
    run_passes(monkeypatch, common, managers, [
        {'INTERFACE': INTERFACES, 'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))],
         'BGP_ALLOWED_PREFIXES': [row(default_action='deny')]},
    ])
    assert rule(frr) == 'no-export'
    # life 2: fresh managers on the same FRR; a peer new to FRR is drained before the row
    action = 'deny' if second_row == 'unchanged' else 'permit'
    common, frr, managers = allow_list_runner(monkeypatch, frr=frr)
    run_passes(monkeypatch, common, managers, [
        {'INTERFACE': INTERFACES + [event('Ethernet4', {'NULL': 'NULL'}), event('Ethernet4|10.1.0.2/31', {'NULL': 'NULL'})],
         'BGP_NEIGHBOR': [event('10.1.0.3', peer_data('10.1.0.2'))],
         'BGP_ALLOWED_PREFIXES': [row(default_action=action)]},
    ])
    if second_row == 'unchanged':
        # the record started from the rule FRR holds, so the render repeated it and nothing is written
        assert rule(frr) == 'no-export'
        assert frr.restarts[-1] == []
    else:
        # the rule FRR holds changes: the row writes it after the render and the peer groups re-evaluate
        assert rule(frr) == DROP
        assert frr.restarts[-1] == ['PEER_V4', 'PEER_V6']


def test_a_policy_whose_write_frr_refused_is_restored_by_its_next_row(monkeypatch):
    common, frr, managers = allow_list_runner(monkeypatch)
    passes = [
        {'INTERFACE': INTERFACES, 'BGP_NEIGHBOR': [event('10.1.0.1', peer_data('10.1.0.0'))],
         'BGP_ALLOWED_PREFIXES': [row(default_action='deny')]},
        {'BGP_ALLOWED_PREFIXES': [row(default_action='permit')]},
        {'INTERFACE': [event('Ethernet4', {'NULL': 'NULL'}), event('Ethernet4|10.1.0.2/31', {'NULL': 'NULL'})],
         'BGP_NEIGHBOR': [event('10.1.0.3', peer_data('10.1.0.2'))],
         'BGP_ALLOWED_PREFIXES': [row(default_action='permit')]},
    ]
    committed = []
    original_commit = common['cfg_mgr'].commit

    def commit_refusing_the_second():
        if len(committed) == 1:
            frr.refuse_writes = 1
        committed.append(True)
        original_commit()  # returns False for the refused write; the Runner logs it and goes on
        return True

    common['cfg_mgr'].commit = commit_refusing_the_second
    run_passes(monkeypatch, common, managers, passes)
    # FRR refused the change to permit, so it kept deny while the record says permit; the new peer
    # rendered permit before the row was drained, and the row still writes the rule and refreshes
    # the peer groups, because the rule FRR holds differs from its action
    assert rule(frr) == DROP and rule(frr, 'V6') == DROP
    assert frr.restarts[-1] == ['PEER_V4', 'PEER_V6']
