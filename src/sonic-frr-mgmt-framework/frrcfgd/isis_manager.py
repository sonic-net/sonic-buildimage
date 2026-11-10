"""Multi-instance IS-IS adapter for gs1571's bounded recovery kernel.

Reuse the ownership journal, fail-fast native transport, periodic/event worker
and readback-before-confirmation algorithm. Only the IS-IS projection/planner
changes; no SRv6 or southbound FRR modification is pulled into this feature.
"""
import json
import subprocess
import sys
import threading
from .isis_config import desired, router, interface, process_commands, binding_commands
from .isis_native import NativeClient, Operation, complete_running_config
from .isis_readback import parse
from .isis_recovery import (AtomicJSONStore, Identity, Mutation, RecoveryKernel,
                            RecoveryError, read_boot_id)


def conflict(native, wanted, authorized):
    if native.unsupported:
        return 'Unsupported native IS-IS context: ' + native.unsupported[0]
    targets = set(wanted.objects) | set(authorized)
    for identity, value in native.objects.items():
        if identity in targets and identity not in authorized:
            return 'Live identity lacks ownership: ' + str(identity)
        if identity.kind == 'isis-binding' and identity not in authorized:
            parent = Identity('isis-process', value['process'])
            if parent in targets and native.objects.get(parent) != wanted.objects.get(parent):
                return 'Unowned circuit prevents parent modification'
    return None


def operation(commands):
    return Operation('isisd', ('configure terminal', *commands, 'end'))


def _system_id(value):
    addresses = value.get('net_titles', [])
    return addresses[0].replace('.', '')[-14:-2] if addresses else None


def _reset_vrfs(old, new):
    """FRR owns one system ID per VRF, shared by all area instances."""
    previous = {key.key.split('|')[0]: _system_id(value) for key, value in old.items()
                if key.kind == 'isis-process' and _system_id(value)}
    following = {key.key.split('|')[0]: _system_id(value) for key, value in new.items()
                 if key.kind == 'isis-process' and _system_id(value)}
    return {vrf for vrf in previous.keys() & following.keys() if previous[vrf] != following[vrf]}


def plan(native, wanted, authorized):
    old, new = native.objects, wanted.objects
    changed = {key for key in set(authorized) | set(new) if old.get(key) != new.get(key)}
    reset_vrfs = _reset_vrfs(old, new)
    reset_parents = {key for key in set(old) | set(new)
                     if key.kind == 'isis-process' and key.key.split('|')[0] in reset_vrfs}
    # Changing a shared system ID requires removing every old area first. Never
    # acquire permission over a foreign area as a side effect of that operation.
    for key, value in old.items():
        if ((key in reset_parents) or
                (key.kind == 'isis-binding' and value['vrf'] in reset_vrfs)) and key not in authorized:
            raise RecoveryError('Unowned IS-IS object prevents shared system-ID change')
    changed |= reset_parents
    changed |= {key for key, value in {**old, **new}.items()
                if key.kind == 'isis-binding' and value['vrf'] in reset_vrfs}
    if not changed:
        return None
    operations = []
    for key in sorted(changed):
        value, target = old.get(key), new.get(key)
        if key.kind == 'isis-binding' and value and (
                not target or value['process'] != target['process'] or value['vrf'] in reset_vrfs):
            operations.append(operation([interface(key.key, value['vrf']),
                                         'no ip router isis ' + value['instance'],
                                         'no ipv6 router isis ' + value['instance'], 'exit']))
    # Complete all deletions before creating any replacement parent.
    for key in sorted(changed):
        if key.kind == 'isis-process' and key in old and (key not in new or key in reset_parents):
            operations.append(operation(['no ' + router(key.key)]))
    for key in sorted(changed):
        if key.kind == 'isis-process' and key in new:
            operations.append(operation(process_commands(key.key, new[key],
                                                         None if key in reset_parents else old.get(key))))
    for key in sorted(changed):
        if key.kind == 'isis-binding' and key in new:
            operations.append(operation(binding_commands(key.key, new[key])))
    return Mutation(tuple(operations), frozenset(changed))


class SnapshotReader:
    """The bounded SDK subprocess approach is retained from gs1571's adapter."""
    def __call__(self):
        from .isis_snapshot import TABLES
        result = subprocess.run([sys.executable, '-m', 'frrcfgd.isis_snapshot'],
                                capture_output=True, text=True, timeout=5, check=True)
        if len(result.stdout) > 16 * 1024 * 1024:
            raise RecoveryError('Scoped ConfigDB snapshot exceeds limit')
        config = json.loads(result.stdout)
        if not isinstance(config, dict) or set(config) != set(TABLES):
            raise RecoveryError('Incomplete ConfigDB snapshot')
        return config


class RecoveryWorker(threading.Thread):
    """gs1571 event/periodic reconciliation with independent native sessions."""
    def __init__(self, snapshot_supplier=None, client=None, journal='/etc/frr/isis-owned.json',
                 timeout=30, interval=2, report=lambda result: None, boot_id_supplier=read_boot_id):
        super().__init__(name='IS-IS recovery', daemon=True)
        self.snapshot_supplier = snapshot_supplier or SnapshotReader()
        self.client = client or NativeClient()
        self.kernel = RecoveryKernel(AtomicJSONStore(journal), owner='isis-northbound-v1',
                                     boot_id_supplier=boot_id_supplier, conflict_check=conflict)
        self.timeout, self.interval, self.report = timeout, interval, report
        self.wakeup, self.stopping = threading.Event(), threading.Event()
        self.retry_requested = threading.Event()

    def desired(self):
        return desired(self.snapshot_supplier())

    def readback(self):
        text = self.client.query('isisd', 'write terminal', budget=5)
        return parse(complete_running_config([('isisd', text)]))

    def apply(self, operations):
        record = self.kernel.store.load()
        if record is None or record.status != 'pending':
            raise RecoveryError('No durable pending authorization for native mutation')
        remaining = record.deadline - self.kernel.clock()
        if remaining <= 0:
            raise RecoveryError('Recovery deadline expired before native transport')
        return self.client.apply(operations, budget=min(10, remaining))

    def notify(self, *unused):
        self.wakeup.set()

    def request_retry(self):
        self.retry_requested.set()
        self.notify()

    def run(self):
        last_report = None
        while not self.stopping.is_set():
            self.wakeup.clear()
            retry = self.retry_requested.is_set()
            self.retry_requested.clear()
            try:
                result = self.kernel.step(self.desired, self.readback, plan, self.apply, self.timeout, retry=retry)
                if retry and not result.desired_available:
                    self.retry_requested.set()
            except Exception as error:
                result = error
                if retry:
                    self.retry_requested.set()
            report_key = (getattr(result, 'status', 'error'), getattr(result, 'reason', str(result)))
            if report_key != last_report:
                self.report(result)
                last_report = report_key
            self.wakeup.wait(self.interval)

    def stop(self):
        self.stopping.set()
        self.wakeup.set()
        if self.is_alive():
            self.join(timeout=40)
