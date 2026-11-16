"""Scoped adapter contracts with stateful/source fakes, never native FRR proof."""

import copy
import json
from pathlib import Path
import threading
from types import SimpleNamespace
from unittest.mock import MagicMock, Mock, patch

import pytest
from jinja2 import Environment, FileSystemLoader

from frrcfgd.isis_config import ConfigError
from frrcfgd.isis_desired import parse_canonical_config, verified_defaults
from frrcfgd.isis_manager import ConfigRenderer, DeltaPlanner, NativeReader, RecoveryWorker
from frrcfgd.isis_native import NativeError
from frrcfgd.isis_recovery import (
    AtomicJSONStore,
    DesiredState,
    Identity,
    NativeSnapshot,
    RecoveryError,
    RecoveryKernel,
    RecoveryRecord,
)
from .test_config import mockmapping


PROCESS = Identity('isis-process', 'default')
LOCATOR = Identity('srv6-locator', 'LOC1')
ISIS_SRV6 = Identity('isis-srv6', 'default')
SID = Identity('srv6-sid', '2001:db8:100:fe01::/64')
FOREIGN = Identity('srv6-locator', 'OTHER')
VERSION = (10, 5, 4)


@pytest.fixture
def defaults():
    return verified_defaults(VERSION)


def writer(body=''):
    return 'frr version 10.5.4\nfrr defaults traditional\n!\n' + body + '\n!\n'


def projected(body, defaults):
    return parse_canonical_config(body, defaults, '10.5.4')


def commands(mutation):
    return [(operation.daemon, command) for operation in mutation.payload for command in operation.commands]


def profile(defaults, *, mtu=1497, refresh=900, lifetime=1200, interface=True):
    binding = 'interface Ethernet0\n ipv6 router isis default\n isis network point-to-point\n!\n' if interface else ''
    return projected(
        binding
        + '''router isis default
 is-type level-2-only
 net 49.0001.0000.0000.0001.00
 topology ipv6-unicast
 lsp-mtu {}
 lsp-refresh-interval {}
 max-lsp-lifetime {}
'''.format(
            mtu, refresh, lifetime
        ),
        defaults,
    )


def locator(prefix='2001:db8:100::/48'):
    return dict(prefix=prefix, block_len=32, node_len=16, func_len=16, arg_len=0, behavior='usid', format='usid-f3216')


def srv6_binding(interface='Loopback0'):
    return dict(locator='LOC1', interface=interface)


def ua(locator_name='LOC1', adj='fe80::2'):
    return dict(locator=locator_name, behavior='uA', vrf=None, interface='Ethernet0', adj=adj)


@pytest.mark.parametrize(
    'old,new',
    [
        (dict(mtu=1400, refresh=100, lifetime=500), dict(mtu=1497, refresh=900, lifetime=1200)),
        (dict(mtu=1400, refresh=64000, lifetime=65000), dict(mtu=1497, refresh=100, lifetime=500)),
    ],
)
def test_planner_default_reset_and_timer_order_remain_valid_from_actual_old_state(defaults, old, new):
    native, target = profile(defaults, **old), profile(defaults, **new)
    mutation = DeltaPlanner(defaults)(native, target, frozenset(native.objects) | frozenset(target.objects))
    lines = [command for _, command in commands(mutation)]
    assert 'lsp-mtu 1497' in lines
    refresh_position = lines.index('lsp-refresh-interval ' + str(new['refresh']))
    lifetime_positions = [
        (index, int(command.split()[1]))
        for index, command in enumerate(lines)
        if command.startswith('max-lsp-lifetime ')
    ]
    assert lifetime_positions[-1][1] == new['lifetime']
    if new['refresh'] > old['lifetime'] - 300:
        assert any(
            index < refresh_position and lifetime >= new['refresh'] + 300 for index, lifetime in lifetime_positions
        )
    if new['lifetime'] < old['refresh'] + 300:
        assert refresh_position < lifetime_positions[-1][0]


def test_disable_or_both_af_false_removes_bindings_without_reapplying_circuit_settings(defaults):
    native, target = profile(defaults), profile(defaults, interface=False)
    mutation = DeltaPlanner(defaults)(native, target, frozenset(native.objects))
    lines = [line for _, line in commands(mutation)]
    assert 'no ipv6 router isis default' in lines
    assert not any(line.startswith(('isis metric ', 'isis network ', 'isis passive')) for line in lines)
    removed = DeltaPlanner(defaults)(native, NativeSnapshot({}), frozenset(native.objects))
    lines = [line for _, line in commands(removed)]
    assert lines.index('no ipv6 router isis default') < lines.index('no router isis default')


def test_unrelated_foreign_native_objects_and_health_do_not_enter_owned_delta(defaults):
    native = profile(defaults, mtu=1400)
    objects = copy.deepcopy(native.objects)
    objects[FOREIGN] = locator('2001:db8:200::/48')
    native = NativeSnapshot(objects, unconfirmed=frozenset([FOREIGN]))
    target = profile(defaults)
    mutation = DeltaPlanner(defaults)(native, target, frozenset(target.objects))
    assert FOREIGN not in mutation.identities
    assert all('OTHER' not in line for _, line in commands(mutation))
    objects = {
        PROCESS: target.objects[PROCESS],
        ISIS_SRV6: dict(srv6_binding(), locator='OTHER'),
        FOREIGN: locator('2001:db8:200::/48'),
        LOCATOR: locator(),
    }
    only_locator = NativeSnapshot({LOCATOR: locator('2001:db8:300::/48')})
    mutation = DeltaPlanner(defaults)(NativeSnapshot(objects), only_locator, frozenset([LOCATOR]))
    assert mutation.identities == frozenset([LOCATOR])
    assert all(daemon != 'isisd' and 'OTHER' not in line for daemon, line in commands(mutation))


def test_locator_change_rejects_unowned_native_reference(defaults):
    native = NativeSnapshot({LOCATOR: locator(), SID: ua()})
    target = NativeSnapshot({LOCATOR: locator('2001:db8:200::/48')})
    with pytest.raises(RecoveryError, match='foreign reference'):
        DeltaPlanner(defaults)(native, target, frozenset([LOCATOR]))


def test_locator_dependency_order_detach_then_replace_then_reattach(defaults):
    new_sid = Identity('srv6-sid', '2001:db8:200:fe01::/64')
    native = NativeSnapshot({LOCATOR: locator(), ISIS_SRV6: srv6_binding(), SID: ua()})
    target = NativeSnapshot({LOCATOR: locator('2001:db8:200::/48'), ISIS_SRV6: srv6_binding(), new_sid: ua()})
    mutation = DeltaPlanner(defaults)(native, target, frozenset(native.objects) | frozenset(target.objects))
    lines = commands(mutation)
    assert all('arg-bits' not in line for _, line in lines)
    removal = lines.index(('zebra', 'no locator LOC1'))
    creation = next(
        index
        for index, item in enumerate(lines)
        if item[0] == 'zebra' and item[1].startswith('prefix 2001:db8:200::/48')
    )
    assert lines.index(('mgmtd', 'no sid ' + SID.key)) < removal
    assert lines.index(('isisd', 'no locator LOC1')) < removal < creation
    assert (
        next(
            index for index, item in enumerate(lines) if item[0] == 'mgmtd' and item[1].startswith('sid ' + new_sid.key)
        )
        > creation
    )
    assert max(index for index, item in enumerate(lines) if item == ('isisd', 'locator LOC1')) > creation


def test_unmanaged_node_msd_blocks_parent_removal_but_is_preserved_on_binding_update(defaults):
    base = profile(defaults)
    objects = copy.deepcopy(base.objects)
    objects[LOCATOR], objects[ISIS_SRV6] = locator(), srv6_binding()
    native = NativeSnapshot(objects)
    raw_objects = copy.deepcopy(objects)
    raw_objects[ISIS_SRV6]['node_msd'] = dict(defaults.node_msd, **{'max-segs-left': 6})
    raw = NativeSnapshot(raw_objects)
    planner = DeltaPlanner(defaults, lambda: raw)
    with pytest.raises(RecoveryError, match='unmanaged node MSD'):
        planner(native, NativeSnapshot({}), frozenset(native.objects))
    target_objects = copy.deepcopy(objects)
    target_objects[ISIS_SRV6]['interface'] = 'Loopback4'
    mutation = planner(native, NativeSnapshot(target_objects), frozenset(objects))
    lines = [line for _, line in commands(mutation)]
    assert 'no locator LOC1' in lines and 'interface Loopback4' in lines
    assert 'no segment-routing srv6' not in lines and not any('node-msd' in line for line in lines)


@pytest.mark.parametrize('identity', [ISIS_SRV6, SID])
def test_unconfirmed_allocation_detaches_and_reapplies_existing_configuration(defaults, identity):
    objects = {LOCATOR: locator(), ISIS_SRV6: srv6_binding(), SID: ua()}
    native = NativeSnapshot(objects, unconfirmed=frozenset([identity]))
    mutation = DeltaPlanner(defaults)(native, NativeSnapshot(objects), frozenset(objects))
    lines = commands(mutation)
    if identity == SID:
        assert lines.index(('mgmtd', 'no sid ' + SID.key)) < next(
            index for index, item in enumerate(lines) if item[0] == 'mgmtd' and item[1].startswith('sid ' + SID.key)
        )
    else:
        assert lines.index(('isisd', 'no locator LOC1')) < lines.index(('isisd', 'locator LOC1'))
    assert identity in mutation.identities


def test_policy_only_row_emits_zero_static_sid_objects(defaults):
    templates = Path(__file__).resolve().parents[1] / 'templates/srv6'
    template = Environment(loader=FileSystemLoader(str(templates)), trim_blocks=True).get_template('srv6.conf.j2')
    output = template.render(
        SRV6_MY_LOCATORS={'LOC1': {'prefix': '2001:db8:100::/48'}},
        SRV6_MY_SIDS={'LOC1|2001:db8:100:fe01::/64': {'decap_dscp_mode': 'pipe'}},
    )
    snapshot = projected(output, defaults)
    assert LOCATOR in snapshot.objects
    assert not any(identity.kind == 'srv6-sid' for identity in snapshot.objects)


class ReaderClient:
    def __init__(self):
        self.calls = []
        self.isis = writer(
            '''router isis default
 is-type level-2-only
 net 49.0001.0000.0000.0001.00
 segment-routing srv6
  locator LOC1
  interface Loopback0
 exit
exit'''
        )
        self.zebra = writer(
            '''segment-routing
 srv6
  locators
   locator LOC1
    prefix 2001:db8:100::/48 block-len 32 node-len 16
    format usid-f3216
   exit
  exit
 exit
exit'''
        )
        self.mgmtd = writer(
            '''segment-routing
 srv6
  static-sids
   sid 2001:db8:100:fe01::/64 locator LOC1 behavior uA interface Ethernet0 nexthop fe80::2
  exit
 exit
exit'''
        )
        self.clients = 'isis[1] 1\nstatic[2] 1\n'
        self.backends = 'Client: zebra\nClient: staticd\n'
        self.detail = dict(
            name='LOC1',
            prefix='2001:db8:100::/48',
            behavior='usid',
            statusUp=True,
            blockBitsLength=32,
            nodeBitsLength=16,
            functionBitsLength=16,
            argumentBitsLength=0,
        )
        self.allocations = {
            '2001:db8:100::': dict(
                sid='2001:db8:100::',
                locator='LOC1',
                behavior='uN',
                allocationMode='dynamic',
                clients=[dict(protocol='isis', instance=0)],
            ),
            '2001:db8:100:fe01::': dict(
                sid='2001:db8:100:fe01::',
                locator='LOC1',
                behavior='uA',
                allocationMode='explicit',
                clients=[dict(protocol='static', instance=0)],
                context=dict(interfaceName='Ethernet0', nexthopIpv6Address='fe80::2'),
            ),
        }
        self.apply = Mock(return_value=True)

    def query(self, daemon, command='do write terminal', *, budget):
        assert 0 < budget <= 5
        self.calls.append((daemon, command, budget))
        if command == 'show zebra client summary':
            return self.clients
        if command == 'show mgmt backend-adapter all':
            return self.backends
        if command == 'do write terminal':
            return self.isis if daemon == 'isisd' else getattr(self, daemon)
        if command.endswith(' detail json'):
            return json.dumps(self.detail)
        if command.endswith(' sid json'):
            return json.dumps(self.allocations)
        raise AssertionError('Unexpected native query: ' + command)


def test_native_readback_retains_failed_allocations_as_unconfirmed_not_absent(defaults, tmp_path):
    client = ReaderClient()
    client.allocations = {}
    reader = NativeReader(defaults, VERSION, client=client)
    snapshot = reader()
    assert {ISIS_SRV6, SID} <= snapshot.objects.keys()
    assert {ISIS_SRV6, SID} <= snapshot.unconfirmed
    target = DesiredState(NativeSnapshot(snapshot.objects))
    store = AtomicJSONStore(tmp_path / 'ownership.json')
    store.save(RecoveryRecord('default', frozenset(snapshot.objects), frozenset(), target.digest, 'applied', None, 100))
    result = RecoveryKernel(store, clock=lambda: 101).step(
        lambda: target, reader, DeltaPlanner(defaults, lambda: reader.raw), client.apply, timeout=10
    )
    assert result.status == 'pending' and result.mutation_attempted
    assert store.load().status == 'pending' and SID in result.observed.objects


@pytest.mark.parametrize('fault', ['protocol', 'adj'])
def test_static_sid_requires_correct_client_mode_and_context(defaults, fault):
    client = ReaderClient()
    baseline = NativeReader(defaults, VERSION, client=client)()
    assert not baseline.unconfirmed and baseline.objects[SID]['adj'] == 'fe80::2'
    allocation = client.allocations['2001:db8:100:fe01::']
    if fault == 'protocol':
        allocation['clients'][0]['protocol'] = 'bgp'
    else:
        allocation['context']['nexthopIpv6Address'] = 'fe80::3'
    snapshot = NativeReader(defaults, VERSION, client=client)()
    assert SID in snapshot.unconfirmed and SID in snapshot.objects


def test_ua_omitted_adj_does_not_confirm_a_stale_native_next_hop(defaults):
    client = ReaderClient()
    client.mgmtd = client.mgmtd.replace(' nexthop fe80::2', '')
    assert SID in NativeReader(defaults, VERSION, client=client)().unconfirmed
    del client.allocations['2001:db8:100:fe01::']['context']['nexthopIpv6Address']
    assert SID not in NativeReader(defaults, VERSION, client=client)().unconfirmed
    client.allocations['2001:db8:100:fe01::']['context']['nexthopIpv6Address'] = '::'
    assert SID not in NativeReader(defaults, VERSION, client=client)().unconfirmed


def test_locator_running_tree_does_not_replace_operational_confirmation(defaults):
    client = ReaderClient()
    client.detail['functionBitsLength'] = 20
    snapshot = NativeReader(defaults, VERSION, client=client)()
    assert LOCATOR in snapshot.unconfirmed and LOCATOR in snapshot.objects


def test_isis_node_sid_requires_correct_client_and_presence_only_when_net_configured(defaults):
    client = ReaderClient()
    client.allocations['2001:db8:100::']['clients'] = [dict(protocol='static', instance=0)]
    assert ISIS_SRV6 in NativeReader(defaults, VERSION, client=client)().unconfirmed
    client.isis = client.isis.replace(' net 49.0001.0000.0000.0001.00\n', '')
    assert ISIS_SRV6 not in NativeReader(defaults, VERSION, client=client)().unconfirmed


def test_readiness_failure_blocks_readback_and_worker_never_reports_success(defaults, tmp_path):
    client = ReaderClient()
    client.clients = 'isis[1] 1\n'
    reader = NativeReader(defaults, VERSION, client=client)
    reports = []
    worker = RecoveryWorker(
        lambda: {},
        lambda config: DesiredState(NativeSnapshot({})),
        reader,
        journal=tmp_path / 'ownership.json',
        report=reports.append,
    )
    worker.wakeup.wait = lambda timeout: worker.stopping.set()
    worker.run()
    assert len(reports) == 1 and reports[0].status == 'readback_error'
    assert all(command != 'do write terminal' for _, command, _ in client.calls)
    client.apply.assert_not_called()


def test_lazy_prepare_recovers_after_two_failures_without_new_event_or_bgp_restart(defaults, tmp_path):
    target = DesiredState(profile(defaults, interface=False))
    attempts, reports, state = [0], [], [{}]

    class Reader:
        def __init__(self):
            self.defaults, self.raw = defaults, None
            self.client = SimpleNamespace(apply=Mock(side_effect=self.apply))

        def __call__(self):
            self.raw = NativeSnapshot(state[0])
            return self.raw

        def apply(self, operations, *, budget):
            assert attempts[0] == 3 and 0 < budget <= 10
            assert operations and all(operation.daemon == 'isisd' for operation in operations)
            assert any('router isis default' in operation.commands for operation in operations)
            state[0] = copy.deepcopy(target.snapshot.objects)
            return True

    reader = Reader()

    def prepare():
        attempts[0] += 1
        if attempts[0] <= 2:
            raise NativeError('preparation temporarily unavailable')
        return lambda: {}, lambda config: target, reader

    worker = RecoveryWorker(
        prepare=prepare, journal=tmp_path / 'ownership.json', timeout=30, interval=0, report=reports.append
    )
    worker.kernel.clock = lambda: 100
    worker.kernel.store.save(
        RecoveryRecord(
            'default',
            frozenset(target.snapshot.objects),
            frozenset(),
            target.digest,
            'failed',
            90,
            80,
            'Previous attempt expired',
        )
    )
    worker.request_retry()
    worker.notify = Mock(wraps=worker.notify)

    def report(result):
        reports.append(result)
        if getattr(result, 'status', None) == 'applied' or len(reports) >= 5:
            worker.stopping.set()

    worker.report = report
    with patch.dict('sys.modules', **mockmapping):
        from frrcfgd.frrcfgd import BGPConfigDaemon

        daemon = object.__new__(BGPConfigDaemon)
        daemon.isis_recovery_mode = True
        daemon.subscribe_all = Mock()
        daemon.config_db = Mock()
        daemon._start_isis_recovery = Mock(side_effect=lambda: setattr(daemon, 'isis_recovery_worker', worker))
        daemon.start()
        daemon.config_db.listen.assert_called_once()
        worker.run()
        daemon.config_db.stop_listen.assert_not_called()
    assert attempts[0] == 3 and len(reports) == 3
    assert all(not result.desired_available for result in reports[:2])
    assert reports[-1].status == 'applied' and state[0] == target.snapshot.objects
    worker.notify.assert_not_called()
    reader.client.apply.assert_called_once()


def test_renderer_uses_validated_same_snapshot_and_dotted_version_contract(defaults):
    config = {
        'PORT': {'Ethernet0': {}},
        'INTERFACE': {'Ethernet0': {}},
        'ISIS_GLOBALS': {'default': {'enabled': 'true', 'net_title': '49.0001.0000.0000.0001.00'}},
    }
    seen = []

    def render(command, **kwargs):
        data = json.loads(Path(command[command.index('-j') + 1]).read_text())
        seen.append(data)
        name = Path(command[command.index('-t') + 1]).name
        fragment = 'router isis default\n net 49.0001.0000.0000.0001.00\n' if name == 'isisd.conf.j2' else '! No SRv6\n'
        return SimpleNamespace(stdout=fragment)

    with patch('frrcfgd.isis_manager.subprocess.run', side_effect=render) as run:
        target = ConfigRenderer(defaults, VERSION)(config)
    assert PROCESS in target.snapshot.objects and run.call_count == 2
    assert seen[0] == seen[1] and 'BGP_GLOBALS' not in seen[0]
    config['ISIS_GLOBALS']['default']['net_title'] = '49..0001.0000.0000.0001.00'
    with patch('frrcfgd.isis_manager.subprocess.run') as run:
        with pytest.raises(ConfigError):
            ConfigRenderer(defaults, VERSION)(config)
        run.assert_not_called()


@pytest.fixture
def canonical_daemon():
    with patch.dict('sys.modules', **mockmapping):
        from frrcfgd.frrcfgd import BGPConfigDaemon, ExtConfigDBConnector

        db = MagicMock()
        tables = {
            'ISIS_GLOBALS': {'default': {'enabled': 'true'}},
            'ISIS_INTERFACE': {'Ethernet0': {'enabled': 'true'}},
            'INTERFACE': {'Ethernet0': {}},
            'BGP_GLOBALS': {'default': {'local_asn': '65000'}},
        }
        db.get_entry.side_effect = lambda table, key: (
            {
                'docker_routing_config_mode': 'unified',
                'frr_mgmt_framework_config': 'true',
                'use_template_render_for_restore': 'true',
            }
            if table == 'DEVICE_METADATA'
            else copy.deepcopy(tables.get(table, {}).get(key, {}))
        )
        db.get_table.side_effect = lambda table: copy.deepcopy(tables.get(table, {}))
        db.get_table_data.return_value = {
            ExtConfigDBConnector.get_table_key(table, key): copy.deepcopy(row)
            for table in ('ISIS_GLOBALS', 'ISIS_INTERFACE', 'BGP_GLOBALS')
            for key, row in tables[table].items()
        }
        factory = Mock(return_value=db)
        factory.get_table_key = ExtConfigDBConnector.get_table_key
        with patch('frrcfgd.frrcfgd.ExtConfigDBConnector', factory), patch('frrcfgd.frrcfgd.g_run_command') as run:
            daemon = BGPConfigDaemon()
        worker = SimpleNamespace(notify=Mock(), stop=Mock())
        daemon.isis_recovery_worker = worker
        yield daemon, worker, run, ExtConfigDBConnector


def test_canonical_isis_events_only_wake_worker_and_bgp_keeps_its_existing_queue(canonical_daemon):
    daemon, worker, _, connector = canonical_daemon
    assert daemon.isis_recovery_mode and not daemon.isis_interface_bindings
    assert connector.get_table_key('ISIS_GLOBALS', 'default') not in daemon.table_data_cache
    assert connector.get_table_key('ISIS_INTERFACE', 'Ethernet0') not in daemon.table_data_cache
    assert connector.get_table_key('BGP_GLOBALS', 'default') in daemon.table_data_cache
    with patch('frrcfgd.frrcfgd.g_run_command') as run:
        daemon.bgp_table_handler_common('ISIS_GLOBALS', 'default', {'enabled': 'true'})
        daemon.bgp_table_handler_common('SRV6_MY_LOCATORS', 'LOC1', {'prefix': '2001:db8:100::/48'})
        daemon.isis_routed_interface_handler('INTERFACE', 'Ethernet0', {})
        run.assert_not_called()
    assert worker.notify.call_count == 3 and daemon.bgp_message.empty()
    assert connector.get_table_key('ISIS_GLOBALS', 'default') not in daemon.table_data_cache
    worker.notify.reset_mock()
    daemon._BGPConfigDaemon__update_bgp = Mock()
    daemon.bgp_table_handler_common('BGP_GLOBALS', 'default', {'local_asn': '65001'})
    key, deleted, table, row = daemon.bgp_message.get_nowait()
    assert (key, deleted, table) == ('default', False, 'BGP_GLOBALS')
    assert row['local_asn'].data == '65001'
    daemon._BGPConfigDaemon__update_bgp.assert_called_once()
    worker.notify.assert_not_called()


def test_recovery_initialization_failure_keeps_independent_bgp_listener_alive():
    with patch.dict('sys.modules', **mockmapping):
        from frrcfgd.frrcfgd import BGPConfigDaemon, BgpdClientMgr

        daemon = object.__new__(BGPConfigDaemon)
        daemon.isis_recovery_mode = True
        daemon.subscribe_all = Mock()
        daemon._start_isis_recovery = Mock(side_effect=NativeError('ISIS unavailable'))
        daemon.config_db = Mock()
        daemon.start()
        daemon.subscribe_all.assert_called_once()
        daemon.config_db.listen.assert_called_once()
        assert 'isisd' not in BgpdClientMgr.ALL_DAEMONS


def test_actual_bgp_client_constructor_never_connects_optional_isis_endpoint():
    class Endpoint:
        def __init__(self):
            self.connected, self.bound, self.sent, self.closed = None, None, [], False

        def connect(self, path):
            assert path != '/run/frr/isisd.vty', 'optional ISIS must not block BGP client construction'
            self.connected = path

        def settimeout(self, timeout):
            self.timeout = timeout

        def sendall(self, data):
            self.sent.append(data)

        def recv(self, size):
            return b'\0\0\0\0'

        def bind(self, path):
            self.bound = path

        def listen(self, backlog):
            self.backlog = backlog

        def close(self):
            self.closed = True

    endpoints = []

    def socket_factory(*args):
        endpoint = Endpoint()
        endpoints.append(endpoint)
        return endpoint

    try:
        with patch.dict('sys.modules', **mockmapping):
            from frrcfgd.frrcfgd import BgpdClientMgr

            with patch('frrcfgd.frrcfgd.socket.socket', side_effect=socket_factory), patch('frrcfgd.frrcfgd.os.unlink'):
                manager = BgpdClientMgr()
            assert not manager.is_alive()
            assert set(manager.client_socks) == {'bgpd', 'zebra', 'staticd', 'bfdd', 'ospfd', 'pimd', 'mgmtd'}
            assert all(endpoint.sent == [b'enable\0'] for endpoint in manager.client_socks.values())
            assert manager.proxy_sock.bound == BgpdClientMgr.PROXY_SERVER_ADDR
            assert manager.proxy_sock.backlog == 1
    finally:
        for endpoint in endpoints:
            endpoint.close()
    assert endpoints and all(endpoint.closed for endpoint in endpoints)


def test_native_subscription_precedes_first_worker_snapshot():
    with patch.dict('sys.modules', **mockmapping):
        from frrcfgd.frrcfgd import BGPConfigDaemon, ExtConfigDBConnector
        from frrcfgd.isis_snapshot import TABLES

        event_db = ExtConfigDBConnector()
        event_db.db_name = 'CONFIG_DB'
        event_db.subscription_ready = threading.Event()
        event_db.get_dbid = Mock(return_value=4)
        event_db.pubsub = Mock()
        producer = Mock(return_value={name: {} for name in TABLES})
        daemon = object.__new__(BGPConfigDaemon)
        daemon.config_db = event_db
        fake_worker = SimpleNamespace(start=Mock())
        version_result = SimpleNamespace(stdout='zebra version 10.5.4')
        with patch('frrcfgd.frrcfgd.subprocess.run', return_value=version_result) as version, patch(
            'frrcfgd.isis_manager.SnapshotReader', return_value=producer
        ), patch('frrcfgd.isis_manager.ConfigRenderer'), patch('frrcfgd.isis_manager.NativeReader'), patch(
            'frrcfgd.isis_manager.RecoveryWorker', return_value=fake_worker
        ) as factory:
            daemon._start_isis_recovery()
            version.assert_not_called()
            snapshot, _, _ = factory.call_args.kwargs['prepare']()
            version.assert_called_once()
        with pytest.raises(ValueError, match='not acknowledged'):
            snapshot()
        producer.assert_not_called()
        message = {'type': 'pmessage', 'channel': '__keyspace@4__:ISIS_GLOBALS|default'}

        def subscribe(pattern):
            assert pattern == '__keyspace@4__:*'
            assert not event_db.subscription_ready.is_set()
            with pytest.raises(ValueError, match='not acknowledged'):
                snapshot()

        def receive(timeout, interrupt_on_signal):
            assert timeout == 1 and interrupt_on_signal is True
            assert event_db.subscription_ready.is_set()
            event_db.stop_listen()
            return message

        def event(delivered):
            assert delivered == message
            snapshot()

        event_db.pubsub.psubscribe.side_effect = subscribe
        event_db.pubsub.get_message.side_effect = receive
        event_db.sub_msg_handler = event
        event_db.listen_thread(1)
        producer.assert_called_once()
        event_db.pubsub.psubscribe.assert_called_once_with('__keyspace@4__:*')
        event_db.pubsub.punsubscribe.assert_called_once_with('__keyspace@4__:*')
        assert not event_db.subscription_ready.is_set()
        fake_worker.start.assert_called_once()
