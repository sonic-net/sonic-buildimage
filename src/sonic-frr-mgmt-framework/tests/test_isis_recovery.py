"""Source-only counterexamples for the bounded recovery kernel, not native FRR."""

import copy
from dataclasses import replace
import json
from pathlib import Path

import pytest

from frrcfgd.isis_recovery import (
    AtomicJSONStore,
    DesiredState,
    Identity,
    Mutation,
    NativeDefaults,
    NativeSnapshot,
    RecoveryError,
    RecoveryKernel,
    RecoveryRecord,
    normalize_net,
    parse_running_config,
    prepare_startup_record,
)


PROCESS = Identity('isis-process', 'default')
NET_A = Identity('isis-net', 'default|49000100000000000100')
NET_B = Identity('isis-net', 'default|49000100000000000200')
BINDING = Identity('isis-binding', 'Ethernet0|ipv6|default')
LOCATOR = Identity('srv6-locator', 'LOC1')
FOREIGN = Identity('srv6-locator', 'OTHER')
SID = Identity('srv6-sid', '2001:db8:100:a::/128')


def desired(objects):
    return DesiredState(NativeSnapshot(objects))


class MemoryStore:
    def __init__(self, record=None, events=None):
        self.record = record
        self.events = events if events is not None else []
        self.saves = 0
        self.fail_save = None
        self.fail_load = False

    def load(self):
        if self.fail_load:
            raise OSError('injected metadata read failure')
        return self.record

    def save(self, record):
        self.saves += 1
        if self.saves == self.fail_save:
            raise OSError('injected metadata write failure')
        self.record = record
        self.events.append(('save', record.status, record.owned, record.possible))


def registered(state, identities=None, **kwargs):
    return RecoveryRecord(
        'default',
        frozenset(state.snapshot.objects if identities is None else identities),
        frozenset(),
        state.digest,
        'applied',
        None,
        100.0,
        **kwargs
    )


class Native:
    """Stateful command fake: partial apply and lost responses alter real fake state."""

    def __init__(self, objects, store):
        self.objects = copy.deepcopy(objects)
        self.store = store
        self.reads = 0
        self.applies = 0
        self.fail_read_at = set()
        self.fail_after = None
        self.crash_after = None
        self.lose_response = False
        self.after_apply = None
        self.operations = []
        self.unsupported = ()
        self.unconfirmed = set()

    def readback(self):
        self.reads += 1
        self.store.events.append(('read', self.reads))
        if self.reads in self.fail_read_at:
            raise RecoveryError('injected native read failure')
        return NativeSnapshot(self.objects, self.unsupported, frozenset(self.unconfirmed))

    def apply(self, payload):
        self.applies += 1
        self.store.events.append(('apply', payload))
        for index, (identity, value) in enumerate(payload, 1):
            assert identity in self.store.record.possible, 'mutation without write-ahead identity'
            if value is None:
                self.objects.pop(identity, None)
            else:
                self.objects[identity] = copy.deepcopy(value)
            self.operations.append((identity, value))
            if self.crash_after == index:
                raise SystemExit('injected process crash')
            if self.fail_after == index:
                return False
        if self.after_apply:
            self.after_apply()
        if self.lose_response:
            raise OSError('injected lost response after native success')
        return True


def plan(native, target, authorized):
    operations = [
        (identity, target.objects.get(identity))
        for identity in sorted(authorized)
        if native.objects.get(identity) != target.objects.get(identity)
    ]
    return Mutation(tuple(operations), frozenset(identity for identity, _ in operations))


def step(store, native, state, clock=lambda: 100.0, **kwargs):
    supplier = state if callable(state) else lambda: state
    return RecoveryKernel(store, clock=clock).step(supplier, native.readback, plan, native.apply, timeout=10, **kwargs)


def test_empty_native_bootstrap_records_possible_before_mutation():
    target = desired({PROCESS: {'vrf': 'default'}, NET_A: {'process': 'default', 'net': 'a'}})
    store = MemoryStore()
    native = Native({}, store)
    result = step(store, native, target)
    assert result.status == 'applied' and result.mutation_attempted
    assert [event[0] for event in store.events] == ['save', 'read', 'save', 'apply', 'read', 'save']
    assert store.record.owned == frozenset(target.snapshot.objects)
    assert not store.record.possible and store.record.deadline is None


def test_runtime_attempt_records_boot_id_without_rearming_same_digest_on_restart():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore()
    native = Native({}, store)
    native.fail_read_at = {1}
    boot_id = '00000000-0000-0000-0000-000000000001'

    kernel = RecoveryKernel(store, clock=lambda: 100, boot_id_supplier=lambda: boot_id)
    assert kernel.step(lambda: target, native.readback, plan, native.apply, 10).status == 'readback_error'
    assert store.record.boot_id == boot_id and store.record.deadline == 110

    kernel = RecoveryKernel(store, clock=lambda: 111, boot_id_supplier=lambda: boot_id)
    assert kernel.step(lambda: target, native.readback, plan, native.apply, 10).status == 'failed'
    assert store.record.boot_id == boot_id and store.record.deadline == 110
    assert native.reads == 1 and native.applies == 0


def test_matching_native_with_registration_has_zero_mutations_and_no_planner():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore(registered(target))
    native = Native(target.snapshot.objects, store)

    def forbidden(*args):
        pytest.fail('matching configuration must not invoke planner or mutation')

    result = RecoveryKernel(store, clock=lambda: 101).step(
        lambda: target, native.readback, forbidden, forbidden, timeout=10
    )
    assert result.status == 'applied' and not result.mutation_attempted
    assert native.reads == 1 and native.applies == 0
    assert store.saves == 0 and store.record.deadline is None


def test_equal_configuration_with_unconfirmed_owned_allocation_cannot_publish_applied():
    target = desired({LOCATOR: {'prefix': '2001:db8:100::/48'}})
    store = MemoryStore(registered(target))
    native = Native(target.snapshot.objects, store)
    native.unconfirmed = {LOCATOR}
    result = step(store, native, target)
    assert result.status == 'pending' and store.record.status == 'pending'
    assert native.applies == 0 and LOCATOR in native.objects


def test_unconfirmed_owned_identity_repair_then_readback_can_confirm_without_false_absence():
    target = desired({LOCATOR: {'prefix': '2001:db8:100::/48'}})
    store = MemoryStore(registered(target))
    native = Native(target.snapshot.objects, store)
    native.unconfirmed = {LOCATOR}
    native.after_apply = native.unconfirmed.clear

    def repair(snapshot, wanted, authorized):
        assert LOCATOR in snapshot.objects and LOCATOR in snapshot.unconfirmed
        return Mutation(((LOCATOR, wanted.objects[LOCATOR]),), frozenset([LOCATOR]))

    result = RecoveryKernel(store, clock=lambda: 101).step(
        lambda: target, native.readback, repair, native.apply, timeout=10
    )
    assert result.status == 'applied' and result.mutation_attempted
    assert native.applies == 1 and native.reads == 2 and LOCATOR in result.observed.objects


def test_unrelated_foreign_unconfirmed_identity_does_not_block_matching_owned_profile():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore(registered(target))
    native = Native(dict(target.snapshot.objects), store)
    native.objects[FOREIGN] = {'prefix': '2001:db8:200::/48'}
    native.unconfirmed = {FOREIGN}
    assert step(store, native, target).status == 'applied'
    assert native.applies == 0 and FOREIGN in native.objects


def test_readback_failure_after_applied_starts_one_bounded_attempt():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore(registered(target))
    native = Native(target.snapshot.objects, store)
    native.fail_read_at = {1, 2}
    assert step(store, native, target, clock=lambda: 101).status == 'readback_error'
    assert store.record.deadline == 111
    assert step(store, native, target, clock=lambda: 105).status == 'readback_error'
    assert store.record.deadline == 111
    assert step(store, native, target, clock=lambda: 112).status == 'failed'
    assert native.applies == 0


def test_live_matching_identity_without_metadata_is_not_adopted():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore()
    native = Native(target.snapshot.objects, store)
    assert step(store, native, target).status == 'conflict'
    assert native.applies == 0 and not store.record.owned and not store.record.possible


def test_foreign_srv6_objects_are_preserved_without_requiring_ownership():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore()
    native = Native({FOREIGN: {'prefix': '2001:db8:200::/48'}}, store)
    assert step(store, native, target).status == 'applied'
    assert FOREIGN in native.objects and FOREIGN not in store.record.owned


def test_foreign_isis_process_blocks_creation_of_second_instance():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore()
    native = Native({Identity('isis-process', '1'): {'vrf': 'default'}}, store)
    assert step(store, native, target).status == 'conflict'
    assert native.applies == 0


def test_foreign_isis_binding_without_process_blocks_second_instance():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore()
    native = Native(
        {Identity('isis-binding', 'Ethernet0|ipv6|1'): {'interface': 'Ethernet0', 'af': 'ipv6', 'process': '1'}}, store
    )
    assert step(store, native, target).status == 'conflict'
    assert native.applies == 0


def test_unowned_binding_blocks_owned_process_deletion():
    old = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore(registered(old))
    native = Native(dict(old.snapshot.objects), store)
    native.objects[BINDING] = {'interface': 'Ethernet0', 'af': 'ipv6', 'process': 'default'}
    assert step(store, native, desired({})).status == 'conflict'
    assert native.applies == 0


@pytest.mark.parametrize('process_value', [None, {'vrf': 'default', 'level': 'level-2'}])
def test_unowned_isis_srv6_child_blocks_owned_process_delete_or_change(process_value):
    old = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore(registered(old))
    native = Native(old.snapshot.objects, store)
    native.objects[Identity('isis-srv6', 'default')] = {'locator': 'OTHER', 'interface': 'Loopback0', 'node_msd': {}}
    native.objects[FOREIGN] = {'prefix': '2001:db8:200::/48'}
    target = desired({PROCESS: process_value} if process_value else {})
    result = step(store, native, target)
    assert result.status == 'conflict' and 'SRv6 child' in result.reason
    assert native.applies == 0 and FOREIGN in native.objects


def test_unowned_extra_net_in_managed_process_is_a_conflict():
    target = desired({PROCESS: {'vrf': 'default'}, NET_B: {'process': 'default', 'net': 'b'}})
    store = MemoryStore(registered(target, [PROCESS]))
    native = Native({PROCESS: {'vrf': 'default'}, NET_A: {'process': 'default', 'net': 'a'}}, store)
    assert step(store, native, target).status == 'conflict'
    assert native.applies == 0


@pytest.mark.parametrize('target_locator', [None, {'prefix': '2001:db8:101::/48'}])
def test_foreign_sid_reference_blocks_owned_locator_delete_or_change(target_locator):
    old = desired({LOCATOR: {'prefix': '2001:db8:100::/48'}})
    target = desired({LOCATOR: target_locator} if target_locator else {})
    store = MemoryStore(registered(old))
    native = Native(dict(old.snapshot.objects, **{}), store)
    native.objects[SID] = {'locator': 'LOC1', 'behavior': 'uN', 'vrf': None}
    assert step(store, native, target).status == 'conflict'
    assert native.applies == 0


def test_unrelated_foreign_sid_does_not_block_owned_locator_change():
    old = desired({LOCATOR: {'prefix': '2001:db8:100::/48'}})
    target = desired({LOCATOR: {'prefix': '2001:db8:101::/48'}})
    store = MemoryStore(registered(old))
    native = Native(old.snapshot.objects, store)
    native.objects[SID] = {'locator': 'OTHER', 'behavior': 'uN', 'vrf': None}
    assert step(store, native, target).status == 'applied'
    assert SID in native.objects and SID not in store.record.owned


def test_downtime_delete_removes_old_owned_objects_and_preserves_foreign():
    old = desired(
        {
            PROCESS: {'vrf': 'default'},
            NET_A: {'process': 'default', 'net': 'a'},
            BINDING: {'interface': 'Ethernet0', 'af': 'ipv6', 'process': 'default'},
        }
    )
    target = desired({})
    store = MemoryStore(registered(old))
    native = Native(old.snapshot.objects, store)
    native.objects[FOREIGN] = {'prefix': '2001:db8:200::/48'}
    assert step(store, native, target).status == 'applied'
    assert native.objects == {FOREIGN: {'prefix': '2001:db8:200::/48'}}
    assert not store.record.owned and not store.record.possible


def test_partial_failure_retains_pending_union_and_next_step_converges_without_event():
    old = desired({PROCESS: {'vrf': 'default'}, NET_A: {'process': 'default', 'net': 'a'}})
    target = desired({PROCESS: {'vrf': 'default'}, NET_B: {'process': 'default', 'net': 'b'}})
    store = MemoryStore(registered(old))
    native = Native(old.snapshot.objects, store)
    native.fail_after = 1
    result = step(store, native, target)
    assert result.status == 'pending'
    assert store.record.possible == frozenset(old.snapshot.objects) | frozenset(target.snapshot.objects)
    assert len(native.operations) == 1
    native.fail_after = None
    assert step(store, native, target, clock=lambda: 101).status == 'applied'
    assert native.objects == target.snapshot.objects
    assert [event[0] for event in store.events].count('read') == 4


def test_crash_after_partial_native_success_preserves_old_and_possible_identities():
    old = desired({NET_A: {'process': 'default', 'net': 'a'}})
    target = desired({NET_B: {'process': 'default', 'net': 'b'}})
    store = MemoryStore(registered(old))
    native = Native(old.snapshot.objects, store)
    native.crash_after = 1
    with pytest.raises(SystemExit):
        step(store, native, target)
    assert store.record.status == 'pending'
    assert store.record.possible == frozenset([NET_A, NET_B])
    native.crash_after = None
    assert step(store, native, target, clock=lambda: 101).status == 'applied'
    assert native.objects == target.snapshot.objects


def test_lost_response_is_confirmed_by_native_readback_without_repeated_mutation():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore()
    native = Native({}, store)
    native.lose_response = True
    assert step(store, native, target).status == 'applied'
    assert native.applies == 1 and native.reads == 2
    assert step(store, native, target, clock=lambda: 101).status == 'applied'
    assert native.applies == 1


def test_missing_post_mutation_readback_never_confirms_and_retry_reads_before_mutation():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore()
    native = Native({}, store)
    native.fail_read_at = {2}
    result = step(store, native, target)
    assert result.status == 'readback_error' and result.mutation_attempted
    assert store.record.status == 'pending' and store.record.possible == frozenset([PROCESS])
    assert step(store, native, target, clock=lambda: 101).status == 'applied'
    assert native.applies == 1 and native.reads == 3


@pytest.mark.parametrize('failure', ['load', 'first_save', 'pre_mutation_save'])
def test_metadata_failure_before_mutation_sends_no_command(failure):
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore()
    store.fail_load = failure == 'load'
    store.fail_save = {'first_save': 1, 'pre_mutation_save': 2}.get(failure)
    native = Native({}, store)
    assert step(store, native, target).status == 'metadata_error'
    assert native.applies == 0


def test_metadata_completion_failure_keeps_recoverable_union_then_converges():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore()
    store.fail_save = 3
    native = Native({}, store)
    result = step(store, native, target)
    assert result.status == 'metadata_error' and result.mutation_attempted
    assert store.record.status == 'pending' and store.record.possible == frozenset([PROCESS])
    assert step(store, native, target, clock=lambda: 101).status == 'applied'
    assert native.applies == 1


def test_deadline_survives_restart_and_explicit_retry_reads_before_resuming():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore()
    native = Native({}, store)
    native.fail_read_at = {1}
    assert step(store, native, target).status == 'readback_error'
    assert store.record.deadline == 110
    assert step(store, native, target, clock=lambda: 111).status == 'failed'
    assert store.record.deadline == 110 and native.applies == 0
    assert step(store, native, target, clock=lambda: 112).status == 'failed'
    assert store.record.deadline == 110
    assert step(store, native, target, clock=lambda: 113, retry=True).status == 'applied'
    assert native.reads == 3


@pytest.mark.parametrize('initial,failed_at', [(111, 111), (105, 111)])
def test_unavailable_desired_cannot_hide_pending_expiry_and_new_digest_can_recover(initial, failed_at):
    old = desired({NET_A: {'process': 'default', 'net': 'a'}})
    target = desired({NET_B: {'process': 'default', 'net': 'b'}})
    record = replace(registered(old), status='pending', deadline=110)
    store = MemoryStore(record)
    native = Native(old.snapshot.objects, store)
    clock, before_supplier = [initial], []

    def unavailable():
        before_supplier.append(store.record.status)
        clock[0] = failed_at
        raise RecoveryError('desired producer unavailable')

    result = step(store, native, unavailable, clock=lambda: clock[0])
    assert result.status == 'failed' and not result.desired_available
    assert before_supplier == ['failed' if initial >= 110 else 'pending']
    assert store.record.status == 'failed' and store.record.deadline == 110
    assert native.reads == 0 and native.applies == 0
    assert step(store, native, target, clock=lambda: 112).status == 'applied'
    assert native.objects == target.snapshot.objects


def test_unavailable_desired_preserves_pending_deadline_and_explicit_retry_request():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore(replace(registered(target), status='pending', deadline=110))
    native = Native({}, store)

    def unavailable():
        raise RecoveryError('desired producer unavailable')

    result = step(store, native, unavailable, clock=lambda: 105, retry=True)
    assert result.status == 'desired_error' and not result.desired_available
    assert store.record.status == 'pending' and store.record.deadline == 110
    assert store.record.last_observed == 105
    assert native.reads == 0 and native.applies == 0


def test_clock_rollback_does_not_extend_existing_deadline():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore()
    native = Native({}, store)
    native.fail_read_at = {1}
    assert step(store, native, target).status == 'readback_error'
    result = step(store, native, target, clock=lambda: 90)
    assert result.status == 'failed' and 'backwards' in result.reason
    assert store.record.deadline == 110 and store.record.last_observed == 100
    assert store.record.status == 'failed' and native.reads == 1 and native.applies == 0
    assert step(store, native, target, clock=lambda: 111).status == 'failed'


def test_deadline_expiring_during_mutation_cannot_publish_success():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore()
    native = Native({}, store)
    clock = [100]
    native.after_apply = lambda: clock.__setitem__(0, 111)
    result = step(store, native, target, clock=lambda: clock[0])
    assert result.status == 'failed' and result.mutation_attempted
    assert store.record.status == 'failed' and store.record.deadline == 110
    assert store.record.possible == frozenset([PROCESS])
    native.after_apply = None
    assert step(store, native, target, clock=lambda: 112, retry=True).status == 'applied'
    assert native.applies == 1


def test_invalid_clock_before_mutation_cannot_authorize_commands():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore()
    native = Native({}, store)
    ticks = iter([100, float('nan')])
    assert step(store, native, target, clock=lambda: next(ticks)).status == 'metadata_error'
    assert native.applies == 0


def test_new_desired_starts_new_attempt_but_retains_old_possible_identities():
    first = desired({NET_A: {'process': 'default', 'net': 'a'}})
    second = desired({NET_B: {'process': 'default', 'net': 'b'}})
    store = MemoryStore()
    native = Native({}, store)
    native.fail_read_at = {2}
    assert step(store, native, first).status == 'readback_error'
    assert step(store, native, first, clock=lambda: 111).status == 'failed'
    assert step(store, native, second, clock=lambda: 112).status == 'applied'
    assert native.objects == second.snapshot.objects


@pytest.mark.parametrize('change_phase', ['snapshot', 'pre_mutation', 'confirmation'])
def test_concurrent_desired_cannot_confirm_old_digest(change_phase):
    first = desired({PROCESS: {'vrf': 'default', 'value': 'first'}})
    second = desired({PROCESS: {'vrf': 'default', 'value': 'second'}})
    current = [first]
    store = MemoryStore(registered(first) if change_phase == 'confirmation' else None)
    native = Native(first.snapshot.objects if change_phase == 'confirmation' else {}, store)
    calls = [0]
    change_call = 2 if change_phase == 'snapshot' else 3

    def supplier():
        calls[0] += 1
        if calls[0] == change_call:
            current[0] = second
        return current[0]

    result = step(store, native, supplier)
    assert result.status == 'stale' and store.record.status == 'pending'
    assert native.applies == 0
    assert step(store, native, lambda: second, clock=lambda: 101).status == 'applied'
    assert native.objects == second.snapshot.objects


def test_desired_changed_during_mutation_retains_union_and_converges_latest():
    first = desired({NET_A: {'process': 'default', 'net': 'a'}})
    second = desired({NET_B: {'process': 'default', 'net': 'b'}})
    current = [first]
    store = MemoryStore()
    native = Native({}, store)
    native.after_apply = lambda: current.__setitem__(0, second)
    result = step(store, native, lambda: current[0])
    assert result.status == 'stale' and result.mutation_attempted
    assert store.record.digest == first.digest and store.record.possible == frozenset([NET_A])
    native.after_apply = None
    assert step(store, native, lambda: current[0], clock=lambda: 101).status == 'applied'
    assert native.objects == second.snapshot.objects


def test_unsupported_readback_and_unowned_plan_fail_closed():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore()
    native = Native({}, store)
    native.unsupported = ('unknown managed knob',)
    assert step(store, native, target).status == 'conflict'
    native.unsupported = ()
    result = RecoveryKernel(store, clock=lambda: 101).step(
        lambda: target, native.readback, lambda *args: Mutation('bad', frozenset([FOREIGN])), native.apply, timeout=10
    )
    assert result.status == 'conflict' and native.applies == 0


def test_untyped_native_result_is_a_failed_read_not_empty_state():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore()
    result = RecoveryKernel(store, clock=lambda: 100).step(
        lambda: target,
        lambda: {},
        lambda *args: pytest.fail('must not plan'),
        lambda *args: pytest.fail('must not mutate'),
        timeout=10,
    )
    assert result.status == 'readback_error' and store.record.status == 'pending'


def test_metadata_owner_mismatch_never_reads_or_mutates_native():
    target = desired({PROCESS: {'vrf': 'default'}})
    store = MemoryStore(replace(registered(target), owner='other-namespace'))
    native = Native({}, store)
    assert step(store, native, target).status == 'conflict'
    assert native.reads == 0 and native.applies == 0


def test_digest_is_canonical_and_reader_snapshot_is_copied():
    objects = {NET_A: {'net': 'a', 'process': 'default'}, PROCESS: {'vrf': 'default'}}
    first = desired(objects)
    second = desired({PROCESS: {'vrf': 'default'}, NET_A: {'process': 'default', 'net': 'a'}})
    objects[PROCESS]['vrf'] = 'changed'
    assert first.digest == second.digest


def test_native_snapshot_is_immutable_detached_and_validates_unconfirmed_subset():
    original = {PROCESS: {'vrf': 'default', 'nested': {'value': 1}}}
    flags = {PROCESS}
    snapshot = NativeSnapshot(original, unconfirmed=flags)
    original[PROCESS]['nested']['value'] = 2
    flags.clear()
    assert snapshot.objects[PROCESS]['nested']['value'] == 1
    assert snapshot.unconfirmed == frozenset([PROCESS])
    with pytest.raises(RecoveryError, match='observational confirmation'):
        _ = DesiredState(snapshot).digest
    with pytest.raises(TypeError):
        snapshot.objects[FOREIGN] = {}
    with pytest.raises(TypeError):
        snapshot.objects[PROCESS]['nested']['value'] = 3
    detached = copy.deepcopy(snapshot.objects)
    detached[PROCESS]['nested']['value'] = 4
    assert snapshot.objects[PROCESS]['nested']['value'] == 1
    with pytest.raises(RecoveryError, match='must be present'):
        NativeSnapshot(original, unconfirmed=frozenset([FOREIGN]))


def startup(previous, target, now=100, **kwargs):
    return prepare_startup_record(previous, target, 'default', now, 10, trusted_preapply=True, **kwargs)


def test_startup_registration_requires_explicit_trusted_preapply_context():
    target = desired({PROCESS: {'vrf': 'default'}})
    with pytest.raises(RecoveryError, match='trusted pre-application'):
        prepare_startup_record(None, target, 'default', 100, 10)


def test_startup_registration_is_pending_union_not_success_or_lost_ownership():
    old = desired({PROCESS: {'vrf': 'default'}})
    previous = replace(registered(old), possible=frozenset([NET_A]), status='pending', deadline=110)
    target = desired({NET_B: {'process': 'default', 'net': 'b'}})
    record = startup(previous, target, now=101)
    assert record.status == 'pending' and record.owned == frozenset([PROCESS])
    assert record.possible == frozenset([PROCESS, NET_A, NET_B])
    assert record.digest == target.digest and record.deadline == 111


def test_startup_reentry_same_digest_preserves_deadline_across_restarts():
    target = desired({PROCESS: {'vrf': 'default'}})
    first = startup(None, target)
    second = startup(first, target, now=105)
    third = startup(second, target, now=109)
    assert first.deadline == second.deadline == third.deadline == 110
    assert second.last_observed == 105 and third.last_observed == 109
    assert third.possible == frozenset([PROCESS]) and not third.owned


def test_expired_startup_registration_forbids_emission_and_does_not_expand_authority():
    target = desired({PROCESS: {'vrf': 'default'}})
    previous = RecoveryRecord('default', frozenset(), frozenset(), target.digest, 'pending', 110, 100)
    record = startup(previous, target, now=111)
    assert record.status == 'failed' and record.deadline == 110
    assert not record.owned and not record.possible
    assert startup(record, target, now=112) == record
    retried = startup(record, target, now=113, retry=True)
    assert retried.status == 'pending' and retried.deadline == 123
    assert retried.possible == frozenset([PROCESS])


def test_startup_pending_clock_rollback_is_failed_with_original_deadline():
    target = desired({PROCESS: {'vrf': 'default'}})
    record = startup(None, target)
    failed = startup(record, target, now=99)
    assert failed.status == 'failed' and 'backwards' in failed.reason
    assert failed.deadline == 110 and failed.last_observed == 100


def test_startup_cannot_reassign_another_owner_record():
    target = desired({PROCESS: {'vrf': 'default'}})
    with pytest.raises(RecoveryError, match='scope mismatch'):
        startup(replace(registered(target), owner='other-namespace'), target)


def test_atomic_json_roundtrip_schema_and_replace_failure(tmp_path, monkeypatch):
    target = desired({PROCESS: {'vrf': 'default'}})
    old = registered(target)
    store = AtomicJSONStore(tmp_path / 'ownership.json')
    with store.guard():
        store.save(old)
        assert store.load() == old
    import frrcfgd.isis_recovery as module

    monkeypatch.setattr(module.os, 'replace', lambda *args: (_ for _ in ()).throw(OSError('replace failed')))
    with pytest.raises(OSError):
        store.save(replace(old, status='pending', deadline=110))
    assert store.load() == old
    assert sorted(p.name for p in tmp_path.iterdir()) == ['ownership.json', 'ownership.json.lock']
    corrupt = old.as_json()
    corrupt['version'] = 2
    store.path.write_text(json.dumps(corrupt))
    with pytest.raises(RecoveryError):
        store.load()


def test_atomic_metadata_corruption_never_authorizes_live_mutation(tmp_path):
    store = AtomicJSONStore(tmp_path / 'ownership.json')
    store.path.write_text('{broken json')
    target = desired({PROCESS: {'vrf': 'default'}})
    native = Native({}, store)
    assert step(store, native, target).status == 'metadata_error'
    assert native.applies == 0 and native.reads == 0


def test_atomic_store_guard_does_not_block_behind_another_worker(tmp_path):
    first = AtomicJSONStore(tmp_path / 'ownership.json')
    second = AtomicJSONStore(first.path)
    with first.guard():
        with pytest.raises(BlockingIOError):
            with second.guard():
                pytest.fail('concurrent recovery writer acquired an occupied guard')
    with second.guard():
        assert second.load() is None


@pytest.mark.parametrize('fail_call', [1, 2])
def test_atomic_store_file_and_directory_fsync_failures_propagate(tmp_path, monkeypatch, fail_call):
    store = AtomicJSONStore(tmp_path / 'ownership.json')
    target = desired({PROCESS: {'vrf': 'default'}})
    import frrcfgd.isis_recovery as module

    original = module.os.fsync
    calls = [0]

    def failing_fsync(descriptor):
        calls[0] += 1
        if calls[0] == fail_call:
            raise OSError('injected fsync failure')
        return original(descriptor)

    monkeypatch.setattr(module.os, 'fsync', failing_fsync)
    with pytest.raises(OSError, match='fsync'):
        store.save(registered(target))
    if fail_call == 1:
        assert store.load() is None
    else:
        # Rename may be visible but durability was not acknowledged.
        assert store.load() == registered(target)


@pytest.fixture
def defaults():
    # Explicit source-test inputs; native release validation is a separate gate.
    return NativeDefaults(
        'level-1-2',
        False,
        True,
        1497,
        1200,
        900,
        False,
        10,
        'lan',
        32,
        16,
        16,
        0,
        'sr0',
        {'max-segs-left': 3, 'max-end-pop': 3, 'max-h-encaps': 2, 'max-end-d': 5},
    )


def running(body):
    return 'Building configuration...\nCurrent configuration:\n!\nfrr version 10.5.4\n' + body + '\n!\nend\n'


def test_parser_normalizes_multiple_nets_bindings_defaults_and_ignores_bgp(defaults):
    text = running(
        '''interface Ethernet0
 description unrelated
 ipv6 router isis default
 isis metric 42
 isis network point-to-point
!
router isis default
 net 49.0001.0000.0000.0001.00
 net 49.0001.0000.0000.0002.00
 is-type level-2-only
 topology ipv6-unicast
!
router bgp 65000
 address-family ipv4 unicast
  neighbor 192.0.2.1 activate'''
    )
    value = parse_running_config(text, defaults)
    assert not value.unsupported
    assert {NET_A, NET_B} <= set(value.objects)
    assert value.objects[PROCESS]['level'] == 'level-2'
    assert value.objects[PROCESS]['lsp_mtu'] == 1497
    assert value.objects[PROCESS]['lsp_lifetime'] == 1200
    assert value.objects[BINDING]['af'] == 'ipv6'
    assert Identity('isis-binding', 'Ethernet0|ipv4|default') not in value.objects
    assert value.objects[Identity('isis-circuit', 'Ethernet0')] == dict(passive=False, metric=42, circuit_type='p2p')


def test_parser_unrelated_interface_in_foreign_vrf_is_preserved_outside_scope(defaults):
    snapshot = parse_running_config(
        running(
            '''interface Ethernet4 vrf Blue
 description foreign routed interface
 ip address 192.0.2.1/24
 ipv6 address 2001:db8:4::1/64
exit
!
router bgp 65000 vrf Blue
 address-family ipv4 unicast
  redistribute connected'''
        ),
        defaults,
    )
    assert not snapshot.unsupported and not snapshot.objects


def test_parser_isis_binding_in_foreign_interface_vrf_cannot_be_adopted(defaults):
    snapshot = parse_running_config(running('interface Ethernet4 vrf Blue\n ipv6 router isis default'), defaults)
    assert snapshot.unsupported
    assert not any(identity.kind == 'isis-binding' for identity in snapshot.objects)


def test_parser_normalizes_srv6_and_isis_locator_references(defaults):
    text = running(
        '''router isis default
 segment-routing srv6
  locator LOC1
  node-msd
   max-segs-left 3
   max-end-pop 4
!
segment-routing
 srv6
  encapsulation
   source-address 2001:0db8:0000::1
  locators
   locator LOC1
    prefix 2001:db8:100::/48 block-len 32 node-len 16 func-bits 16
    behavior usid
    format usid-f3216
   exit
   locator OTHER
    prefix 2001:db8:200::/48
  static-sids
   sid 2001:0db8:0100:000a::/128 locator LOC1 behavior uN
   sid 2001:db8:100:b::/128 locator LOC1 behavior uDT46 vrf default'''
    )
    value = parse_running_config(text, defaults)
    assert not value.unsupported
    assert value.objects[Identity('srv6-source', 'default')] == {'address': '2001:db8::1'}
    assert value.objects[LOCATOR] == dict(
        prefix='2001:db8:100::/48',
        block_len=32,
        node_len=16,
        func_len=16,
        arg_len=0,
        behavior='usid',
        format='usid-f3216',
    )
    assert value.objects[SID] == dict(locator='LOC1', behavior='uN', vrf=None, interface=None, adj=None)
    assert value.objects[Identity('isis-srv6', 'default')] == dict(
        locator='LOC1',
        interface='sr0',
        node_msd={'max-segs-left': 3, 'max-end-pop': 4, 'max-h-encaps': 2, 'max-end-d': 5},
    )
    assert FOREIGN in value.objects


@pytest.mark.parametrize(
    'interface_command,expected', [('interface Loopback0', 'Loopback0'), ('no interface Loopback0', 'sr0')]
)
def test_parser_isis_srv6_interface_and_native_reset_default(defaults, interface_command, expected):
    value = parse_running_config(
        running('router isis default\n segment-routing srv6\n  ' + interface_command), defaults
    )
    assert value.objects[Identity('isis-srv6', 'default')]['interface'] == expected


@pytest.mark.parametrize('optional,adj', [('', None), (' nexthop 2001:0db8:0000::1', '2001:db8::1')])
def test_parser_ua_interface_optional_adjacency_uses_existing_adj_field(defaults, optional, adj):
    value = parse_running_config(
        running(
            'segment-routing\n srv6\n  static-sids\n'
            '   sid 2001:db8:100:a::/128 locator LOC1 behavior uA interface Ethernet0' + optional
        ),
        defaults,
    )
    assert value.objects[SID] == dict(locator='LOC1', behavior='uA', vrf=None, interface='Ethernet0', adj=adj)
    unspecified = parse_running_config(
        running(
            'segment-routing\n srv6\n  static-sids\n'
            '   sid 2001:db8:100:a::/128 locator LOC1 behavior uA interface Ethernet0 nexthop ::'
        ),
        defaults,
    )
    assert unspecified.objects[SID]['adj'] is None


def test_parser_recorded_native_writer_forms_use_effective_f3216_behavior(defaults):
    # Source conformance to a synthetic fixture derived from recorded VTY
    # writers; this test does not execute or make a new native FRR claim.
    text = (Path(__file__).parent / 'fixtures/isis_recovery/frr_10_5_native.conf').read_text()
    snapshot = parse_running_config(text, defaults)
    assert not snapshot.unsupported
    assert snapshot.objects[LOCATOR] == dict(
        prefix='2001:db8:1::/48',
        block_len=32,
        node_len=16,
        func_len=16,
        arg_len=0,
        behavior='usid',
        format='usid-f3216',
    )
    assert snapshot.objects[Identity('srv6-sid', '2001:db8:1:8001::/64')] == dict(
        locator='LOC1', behavior='uA', vrf=None, interface='Ethernet0', adj='fe80::2'
    )
    assert snapshot.objects[PROCESS]['lsp_mtu'] == 1497
    assert snapshot.objects[Identity('isis-circuit', 'Ethernet0')]['metric'] == 10


@pytest.mark.parametrize('prefix,block', [('2001:db8:100::/48', 24), ('2001:db8:100::/64', 40)])
def test_parser_omitted_native_geometry_uses_native_node_default_not_model_defaults(defaults, prefix, block):
    native_defaults = replace(defaults, block_len=40, node_len=24)
    snapshot = parse_running_config(
        running('segment-routing\n srv6\n  locators\n   locator LOC1\n' '    prefix ' + prefix), native_defaults
    )
    assert snapshot.objects[LOCATOR]['block_len'] == block
    assert snapshot.objects[LOCATOR]['node_len'] == 24
    explicit = parse_running_config(
        running(
            'segment-routing\n srv6\n  locators\n   locator LOC1\n'
            '    prefix 2001:db8:100::/48 block-len 32 node-len 16'
        ),
        native_defaults,
    )
    assert explicit.objects[LOCATOR]['block_len'] == 32 and explicit.objects[LOCATOR]['node_len'] == 16
    with pytest.raises(RecoveryError, match='locator geometry'):
        parse_running_config(
            running('segment-routing\n srv6\n  locators\n   locator LOC1\n' '    prefix 2001:db8::/32'), native_defaults
        )


def test_parser_implicit_and_explicit_default_node_msd_are_equivalent(defaults):
    implicit = parse_running_config(running('router isis default\n segment-routing srv6\n  locator LOC1'), defaults)
    explicit = parse_running_config(
        running(
            '''router isis default
 segment-routing srv6
  locator LOC1
  node-msd
   max-segs-left 3
   max-end-pop 3
   max-h-encaps 2
   max-end-d 5
  exit
 exit
exit'''
        ),
        defaults,
    )
    assert implicit == explicit
    assert DesiredState(implicit).digest == DesiredState(explicit).digest


def test_parser_native_nested_node_msd_writer_exit_forms(defaults):
    snapshot = parse_running_config(
        running(
            '''router isis default
 segment-routing srv6
  interface Ethernet0
  locator LOC1
  node-msd
   max-segs-left 6
   max-end-pop 7
   max-h-encaps 8
   max-end-d 9
  exit
 exit
exit'''
        ),
        defaults,
    )
    assert not snapshot.unsupported
    assert snapshot.objects[Identity('isis-srv6', 'default')] == dict(
        locator='LOC1',
        interface='Ethernet0',
        node_msd={'max-segs-left': 6, 'max-end-pop': 7, 'max-h-encaps': 8, 'max-end-d': 9},
    )


@pytest.mark.parametrize(
    'parameters',
    ['behavior uA', 'behavior uA nexthop 2001:db8::1', 'behavior uN interface Ethernet0', 'behavior uDT46'],
)
def test_parser_invalid_static_sid_argument_combinations_fail_closed(defaults, parameters):
    with pytest.raises(RecoveryError):
        parse_running_config(
            running('segment-routing\n srv6\n  static-sids\n' '   sid 2001:db8:100:a::/128 locator LOC1 ' + parameters),
            defaults,
        )


@pytest.mark.parametrize(
    'text',
    [
        '',
        'end',
        'frr version 10.5.4\nrouter isis default',
        'frr version 10.5.4\n% failed read\nend',
        'frr version 10.5.4\n orphan\nend',
    ],
)
def test_parser_unavailable_or_incomplete_is_not_empty(defaults, text):
    with pytest.raises(RecoveryError):
        parse_running_config(text, defaults)


@pytest.mark.parametrize(
    'body',
    [
        'router isis default\n authentication mode md5',
        'interface Ethernet0\n isis unknown-knob 1',
        'segment-routing\n srv6\n  unknown-context',
    ],
)
def test_parser_unknown_relevant_context_is_explicitly_unsupported(defaults, body):
    assert parse_running_config(running(body), defaults).unsupported


def test_parser_duplicate_native_sid_identity_is_ambiguous(defaults):
    with pytest.raises(RecoveryError, match='Duplicate native identity'):
        parse_running_config(
            running(
                '''segment-routing
 srv6
  static-sids
   sid 2001:db8:100:a::/128 locator LOC1 behavior uN
   sid 2001:0db8:0100:000a::/128 locator OTHER behavior uN'''
            ),
            defaults,
        )


def test_net_canonical_identity_and_invalid_nsel():
    assert normalize_net('49.0001.0000.0000.0001.00') == '49000100000000000100'
    with pytest.raises(RecoveryError):
        normalize_net('49.0001.0000.0000.0001.01')
