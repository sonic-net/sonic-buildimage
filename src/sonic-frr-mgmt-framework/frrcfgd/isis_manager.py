"""Scoped single-instance recovery adapter, independent of the BGP VTY lock.

The durable record authorizes identities, never desired values. ConfigDB and
fresh native readback remain the two inputs to every reconciliation step.
"""

import copy
import ipaddress
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import threading
import time

from .isis_config import (
    MANAGED_TABLES,
    ROUTED_TABLES,
    normalize_managed_config,
    validate_isis_config,
    validate_srv6_config,
)
from .isis_desired import parse_canonical_config, project_managed_snapshot
from .isis_native import NativeClient, NativeError, Operation, complete_running_config
from .isis_recovery import (
    AtomicJSONStore,
    DesiredState,
    Identity,
    Mutation,
    NativeSnapshot,
    RecoveryError,
    RecoveryKernel,
    read_boot_id,
)


def _operation(daemon, *commands):
    return Operation(daemon, ('configure terminal', *commands, 'end'))


def _net(value):
    return '.'.join([value[:2]] + [value[index:index + 4] for index in range(2, len(value) - 2, 4)] + [value[-2:]])


class DeltaPlanner:
    """Order only owned deltas; detach references before replacing a locator."""

    def __init__(self, defaults, raw_supplier=lambda: None):
        self.defaults, self.raw_supplier = defaults, raw_supplier

    def __call__(self, native, desired, authorized):
        old, new = native.objects, desired.objects
        changed = {key for key in set(authorized) | set(new) if old.get(key) != new.get(key)}
        changed.update(native.unconfirmed & set(authorized))
        if not changed:
            return None
        if not changed <= set(authorized):
            raise RecoveryError('Delta includes an unauthorized native identity')
        locators = {key.key for key in changed if key.kind == 'srv6-locator'}
        process = Identity('isis-process', 'default')
        removed_process = process in changed and process in old and process not in new
        detached = {
            key
            for key, value in old.items()
            if key.kind in ('isis-srv6', 'srv6-sid')
            and (key in changed or value.get('locator') in locators or (key.kind == 'isis-srv6' and removed_process))
        }
        if not detached <= set(authorized):
            raise RecoveryError('Locator delta would detach a foreign reference')
        raw = self.raw_supplier()
        if raw is not None:
            for key, value in raw.objects.items():
                if key.kind == 'isis-srv6' and ((key in detached and key not in new) or removed_process):
                    if value.get('node_msd') != self.defaults.node_msd:
                        raise RecoveryError('Parent removal would erase unmanaged node MSD configuration')
        touched = changed | detached
        operations = []
        # Staticd owns the backend; its configuration is written through MGMtd.
        for key in sorted(detached):
            if key.kind == 'srv6-sid':
                operations.append(_operation('mgmtd', 'segment-routing', 'srv6', 'static-sids', 'no sid ' + key.key))
            else:
                if key in new and old[key].get('locator'):
                    operations.append(
                        _operation(
                            'isisd', 'router isis default', 'segment-routing srv6', 'no locator ' + old[key]['locator']
                        )
                    )
                elif key not in new:
                    operations.append(_operation('isisd', 'router isis default', 'no segment-routing srv6'))
        for key in sorted(changed):
            value = old.get(key)
            if value is None:
                continue
            if key.kind == 'isis-binding' and key not in new:
                af = 'ip' if value['af'] == 'ipv4' else 'ipv6'
                operations.append(
                    _operation(
                        'isisd', 'interface ' + value['interface'], 'no ' + af + ' router isis ' + value['process']
                    )
                )
            elif key.kind == 'isis-net' and key not in new:
                operations.append(_operation('isisd', 'router isis default', 'no net ' + _net(value['net'])))
            elif key.kind == 'isis-process' and key not in new:
                operations.append(_operation('isisd', 'no router isis default'))
            elif key.kind == 'srv6-locator':
                # Remove the entire owned locator after detaching consumers.
                # This also clears a previous format before geometry changes.
                operations.append(_operation('zebra', 'segment-routing', 'srv6', 'locators', 'no locator ' + key.key))
            elif key.kind == 'srv6-source' and key not in new:
                operations.append(_operation('zebra', 'segment-routing', 'srv6', 'encapsulation', 'no source-address'))
        for key in sorted(changed):
            value = new.get(key)
            if value is None:
                continue
            if key.kind == 'srv6-source':
                operations.append(
                    _operation(
                        'zebra', 'segment-routing', 'srv6', 'encapsulation', 'source-address ' + value['address']
                    )
                )
            elif key.kind == 'srv6-locator':
                commands = [
                    'segment-routing',
                    'srv6',
                    'locators',
                    'locator ' + key.key,
                    'prefix {} block-len {} node-len {} func-bits {}'.format(
                        value['prefix'], value['block_len'], value['node_len'], value['func_len']
                    ),
                ]
                if value.get('behavior') == 'usid':
                    commands.append('behavior usid')
                if value.get('format'):
                    commands.append('format ' + value['format'])
                operations.append(_operation('zebra', *commands))
        process = Identity('isis-process', 'default')
        if process in changed and process in new:
            value = new[process]
            level = 'level-2-only' if value['level'] == 'level-2' else value['level']
            operations.append(
                _operation(
                    'isisd',
                    'router isis default',
                    'is-type ' + level,
                    ('' if value['dynamic_hostname'] else 'no ') + 'hostname dynamic',
                    ('' if value['ipv6_multi_topology'] else 'no ') + 'topology ipv6-unicast',
                    'lsp-mtu ' + str(value['lsp_mtu']),
                    'max-lsp-lifetime 65535',
                    'lsp-refresh-interval ' + str(value['lsp_refresh_interval']),
                    'max-lsp-lifetime ' + str(value['lsp_lifetime']),
                )
            )
        for key in sorted(changed):
            value = new.get(key)
            if value is None:
                continue
            if key.kind == 'isis-net':
                operations.append(_operation('isisd', 'router isis default', 'net ' + _net(value['net'])))
            elif key.kind == 'isis-binding':
                af = 'ip' if value['af'] == 'ipv4' else 'ipv6'
                operations.append(
                    _operation('isisd', 'interface ' + value['interface'], af + ' router isis ' + value['process'])
                )
        # Set circuit values only after an active AF binding exists. The desired
        # projection excludes disabled and both-AF-false circuit intentions.
        for key in sorted(changed):
            if key.kind == 'isis-circuit' and key in new:
                value = new[key]
                operations.append(
                    _operation(
                        'isisd',
                        'interface ' + key.key,
                        ('' if value['passive'] else 'no ') + 'isis passive',
                        'isis metric ' + str(value['metric']),
                        ('' if value['circuit_type'] == 'p2p' else 'no ') + 'isis network point-to-point',
                    )
                )
        for key in sorted(changed | detached):
            value = new.get(key)
            if value is None:
                continue
            if key.kind == 'srv6-sid':
                command = 'sid {} locator {} behavior {}'.format(key.key, value['locator'], value['behavior'])
                if value['behavior'] == 'uA':
                    command += ' interface ' + value['interface']
                    if value.get('adj'):
                        command += ' nexthop ' + value['adj']
                elif value['behavior'] in ('uDT4', 'uDT6', 'uDT46'):
                    command += ' vrf ' + value['vrf']
                operations.append(_operation('mgmtd', 'segment-routing', 'srv6', 'static-sids', command))
            elif key.kind == 'isis-srv6':
                operations.append(
                    _operation(
                        'isisd',
                        'router isis default',
                        'segment-routing srv6',
                        'locator ' + value['locator'],
                        'interface ' + value['interface'],
                    )
                )
        if not operations:
            raise RecoveryError('No representable delta for changed native objects')
        return Mutation(tuple(operations), frozenset(touched))


class ConfigRenderer:
    """Render exactly one validated snapshot through the installed cfggen."""

    def __init__(self, defaults, version, directory='/usr/local/sonic/frrcfgd', executable='sonic-cfggen'):
        self.defaults, self.version = defaults, version
        self.directory, self.executable = Path(directory), executable

    def __call__(self, config):
        config = normalize_managed_config(config)
        validate_isis_config(config)
        validate_srv6_config(config, self.version)
        # ConfigDB compound keys outside this component are not needed by its
        # canonical templates; preserve only dependency tables here.
        selected = {
            name: copy.deepcopy(config.get(name, {}))
            for name in MANAGED_TABLES + ROUTED_TABLES + ('PORT', 'PORTCHANNEL', 'VLAN', 'VRF')
        }
        selected = {
            name: {'|'.join(key) if isinstance(key, tuple) else key: value for key, value in rows.items()}
            for name, rows in selected.items()
        }
        with tempfile.TemporaryDirectory(prefix='frrcfgd-isis-') as temporary:
            snapshot = Path(temporary) / 'snapshot.json'
            snapshot.write_text(json.dumps(selected))
            snapshot.chmod(0o600)
            fragments = []
            for template in ('isisd.conf.j2', 'srv6.conf.j2'):
                result = subprocess.run(
                    [
                        self.executable,
                        '-j',
                        str(snapshot),
                        '-T',
                        str(self.directory),
                        '-t',
                        str(self.directory / template),
                    ],
                    capture_output=True,
                    text=True,
                    timeout=5,
                    check=True,
                )
                fragments.append(result.stdout)
        return DesiredState(parse_canonical_config('\n'.join(fragments), self.defaults, self.version))


class SnapshotReader:
    """Bound the native SDK, including a server that accepts but never replies."""

    def __init__(self, timeout=5):
        self.timeout = timeout

    def __call__(self):
        from .isis_snapshot import TABLES

        result = subprocess.run(
            [sys.executable, '-m', 'frrcfgd.isis_snapshot'],
            capture_output=True,
            text=True,
            timeout=self.timeout,
            check=True,
        )
        if len(result.stdout) > 16 * 1024 * 1024:
            raise RecoveryError('Scoped ConfigDB snapshot exceeds limit')
        config = json.loads(result.stdout)
        if (
            not isinstance(config, dict)
            or set(config) != set(TABLES)
            or any(not isinstance(rows, dict) for rows in config.values())
        ):
            raise RecoveryError('Scoped ConfigDB snapshot is incomplete')
        return config


class NativeReader:
    """Read configuration only when the component's daemon connections are ready.

    Applied is configuration convergence, with SRv6 allocation checked in Zebra;
    it is not a Linux/ASIC or forwarding acceptance claim.
    """

    def __init__(self, defaults, version, client=None, budget=10, clock=time.monotonic):
        self.defaults, self.version = defaults, version
        self.client = client or NativeClient()
        self.raw = None
        self.budget, self.clock, self.deadline = budget, clock, None

    def query(self, daemon, command='do write terminal'):
        remaining = self.budget if self.deadline is None else self.deadline - self.clock()
        if remaining <= 0:
            raise NativeError('Component readback deadline expired')
        return self.client.query(daemon, command, budget=min(5, remaining))

    def ready(self):
        clients = self.query('zebra', 'show zebra client summary')
        names = {line.split()[0].split('[', 1)[0] for line in clients.splitlines() if line.split()}
        backends = self.query('mgmtd', 'show mgmt backend-adapter all')
        if not {'isis', 'static'} <= names or not {'zebra', 'staticd'} <= set(re.findall(r'Client:\s+(\w+)', backends)):
            raise NativeError('IS-IS/staticd Zebra API or MGMtd backend is not ready')

    def __call__(self):
        from .isis_recovery import parse_running_config

        self.deadline = self.clock() + self.budget
        self.ready()
        fragments = [(daemon, self.query(daemon)) for daemon in ('isisd', 'zebra', 'mgmtd')]
        text = complete_running_config(fragments)
        versions = re.findall(r'^frr version (\d+)\.(\d+)', text, re.MULTILINE)
        if not versions or any(tuple(map(int, item)) != tuple(self.version[:2]) for item in versions):
            raise NativeError('Native FRR version does not match selected defaults')
        raw = parse_running_config(text, self.defaults)
        # A committed NB tree alone does not prove a successful locator/SID
        # request. Keep the configuration objects and separately mark failures;
        # never pretend a failed allocation means the config object is absent.
        unconfirmed, allocations = set(), {}
        for key, value in raw.objects.items():
            if key.kind == 'srv6-locator':
                locator = json.loads(
                    self.query('zebra', 'show segment-routing srv6 locator ' + key.key + ' detail json')
                )
                same = (
                    locator.get('name') == key.key
                    and locator.get('prefix') == value['prefix']
                    and locator.get('behavior') == value.get('behavior')
                    and locator.get('statusUp') is True
                )
                for field, native_field in (
                    ('block_len', 'blockBitsLength'),
                    ('node_len', 'nodeBitsLength'),
                    ('func_len', 'functionBitsLength'),
                    ('arg_len', 'argumentBitsLength'),
                ):
                    same = same and locator.get(native_field) == value[field]
                if not same:
                    unconfirmed.add(key)
            if key.kind in ('isis-srv6', 'srv6-sid') and value.get('locator'):
                name = value['locator']
                if name not in allocations:
                    if Identity('srv6-locator', name) not in raw.objects:
                        allocations[name] = {}
                    else:
                        allocations[name] = json.loads(
                            self.query('zebra', 'show segment-routing srv6 locator ' + name + ' sid json')
                        )
            if key.kind == 'isis-srv6' and value.get('locator'):
                # FRR 10.5 uses GetLocator/GetSID, not the older GetChunk path.
                # With a NET, require the allocated node SID and its ISIS client.
                has_net = any(
                    item.kind == 'isis-net' and row['process'] == key.key for item, row in raw.objects.items()
                )
                expected = raw.objects.get(Identity('srv6-locator', value['locator']), {})
                action = 'uN' if expected.get('behavior') == 'usid' else 'End'
                if has_net and not any(
                    sid.get('locator') == value['locator']
                    and sid.get('behavior') == action
                    and ipaddress.IPv6Address(sid['sid']) in ipaddress.IPv6Network(expected['prefix'])
                    and any(
                        client.get('protocol') == 'isis' and client.get('instance') == 0
                        for client in sid.get('clients', [])
                    )
                    for sid in allocations[value['locator']].values()
                ):
                    unconfirmed.add(key)
            elif key.kind == 'srv6-sid':
                name = value['locator']
                address = str(ipaddress.IPv6Network(key.key).network_address)
                sid = allocations[name].get(address, {})
                locator = raw.objects.get(Identity('srv6-locator', name), {})
                action = value['behavior']
                if locator.get('behavior') != 'usid':
                    action = {'uN': 'End', 'uA': 'End.X', 'uDT4': 'End.DT4', 'uDT6': 'End.DT6', 'uDT46': 'End.DT46'}[
                        action
                    ]
                context = sid.get('context', {})
                valid = (
                    sid.get('locator') == name
                    and sid.get('behavior') == action
                    and sid.get('allocationMode') == 'explicit'
                    and any(
                        client.get('protocol') == 'static' and client.get('instance') == 0
                        for client in sid.get('clients', [])
                    )
                )
                if value['behavior'] == 'uA':
                    valid = valid and context.get('interfaceName') == value['interface']
                    adjacency = ipaddress.IPv6Address(context.get('nexthopIpv6Address') or '::')
                    valid = valid and (None if adjacency.is_unspecified else str(adjacency)) == value.get('adj')
                elif value['behavior'] in ('uDT4', 'uDT6', 'uDT46'):
                    valid = valid and context.get('vrfName', 'default') == value['vrf']
                if not valid:
                    unconfirmed.add(key)
        self.raw = raw
        return project_managed_snapshot(NativeSnapshot(raw.objects, raw.unsupported, frozenset(unconfirmed)))


class RecoveryWorker(threading.Thread):
    """Periodic drift checks and event wakeups, without blocking BGP callbacks."""

    def __init__(
        self,
        snapshot_supplier=None,
        renderer=None,
        reader=None,
        *,
        prepare=None,
        journal='/etc/frr/frrcfgd-owned.json',
        timeout=30,
        interval=2,
        report=lambda result: None
    ):
        super().__init__(name='IS-IS recovery', daemon=True)
        if prepare is None and any(value is None for value in (snapshot_supplier, renderer, reader)):
            raise ValueError('Recovery requires dependencies or a bounded preparation factory')
        self.snapshot_supplier, self.renderer, self.reader = snapshot_supplier, renderer, reader
        self.prepare = prepare
        self.timeout, self.interval, self.report = timeout, interval, report
        self.kernel = RecoveryKernel(AtomicJSONStore(journal), boot_id_supplier=read_boot_id)
        self.planner = DeltaPlanner(reader.defaults, lambda: reader.raw) if reader is not None else None
        self.wakeup, self.stopping, self.retry_requested = threading.Event(), threading.Event(), threading.Event()

    def desired(self):
        if self.reader is None:
            supplier, renderer, reader = self.prepare()
            if not callable(supplier) or not callable(renderer) or not callable(reader):
                raise ValueError('Invalid prepared recovery dependencies')
            planner = DeltaPlanner(reader.defaults, lambda: reader.raw)
            # A failed preparation never publishes a half-initialized client.
            self.snapshot_supplier, self.renderer, self.reader, self.planner = supplier, renderer, reader, planner
        return self.renderer(self.snapshot_supplier())

    def notify(self, *unused):
        self.wakeup.set()

    def request_retry(self):
        """An explicit operator request starts a new bounded attempt."""
        self.retry_requested.set()
        self.wakeup.set()

    def apply(self, operations):
        # The transport must not outlive the persisted component deadline.
        # The kernel still holds the journal lock and has durably authorized
        # this mutation; a missing/unreadable record forbids sending commands.
        record = self.kernel.store.load()
        if record is None or record.status != 'pending':
            raise RecoveryError('No pending authorization for native mutation')
        remaining = record.deadline - self.kernel.clock()
        if remaining <= 0:
            raise RecoveryError('Recovery deadline expired before native transport')
        return self.reader.client.apply(operations, budget=min(10, remaining))

    def run(self):
        while not self.stopping.is_set():
            self.wakeup.clear()
            retry = self.retry_requested.is_set()
            self.retry_requested.clear()
            try:
                result = self.kernel.step(
                    self.desired,
                    lambda: self.reader(),
                    lambda *args: self.planner(*args),
                    self.apply,
                    self.timeout,
                    retry=retry,
                )
                if retry and not result.desired_available:
                    self.retry_requested.set()
                self.report(result)
            except Exception as error:
                # Invalid/incomplete desired graphs must never reach native.
                if retry:
                    self.retry_requested.set()
                self.report(error)
            self.wakeup.wait(self.interval)

    def stop(self):
        self.stopping.set()
        self.wakeup.set()
        self.join(timeout=15)
