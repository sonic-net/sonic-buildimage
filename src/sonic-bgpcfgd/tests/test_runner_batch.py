from collections import deque
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


def make_peers(monkeypatch):
    # Scheduling tests use the repository's validator mock independently of the installed binding version.
    for name in ('isVrfNameValid', 'isInterfaceNameValid'):
        monkeypatch.setattr(swsscommon, name, getattr(swsscommon_test.swsscommon, name), raising=False)
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
        monkeypatch.setattr(swsscommon, name, value)
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

    monkeypatch.setattr(swsscommon, 'DBConnector', Db)
    monkeypatch.setattr(swsscommon, 'SubscriberStateTable', Subscriber)
    monkeypatch.setattr(swsscommon, 'Select', Selector)
    monkeypatch.setattr(swsscommon.SonicDBConfig, 'getDbId', lambda name: 4)
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


def make_frr_backed(monkeypatch, switch_type):
    from bgpcfgd.config import ConfigMgr
    from bgpcfgd.managers_device_global import DeviceGlobalCfgMgr
    common, peer, interface = make_peers(monkeypatch)
    frr = AccumulatingFrr()
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
    assert commands.rindex('neighbor 10.1.0.1 shutdown') > commands.rindex('neighbor 10.1.0.1 remote-as')
    assert not peer.set_queue
