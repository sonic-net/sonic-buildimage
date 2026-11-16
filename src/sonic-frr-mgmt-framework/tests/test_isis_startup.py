"""Startup registration counterexamples; run on the selected Linux test worker."""

from contextlib import contextmanager
from dataclasses import asdict, replace
import json
import stat

import pytest

from frrcfgd.isis_desired import (
    normalize_version, parse_canonical_config, project_managed_snapshot, verified_defaults,
)
from frrcfgd.isis_recovery import (
    AtomicJSONStore, Identity, NativeSnapshot, RecoveryError, parse_running_config,
)
from frrcfgd.isis_startup import main, prepare_startup


FRAGMENT = '''interface Ethernet0
 ip router isis default
 ipv6 router isis default
 isis metric 10
!
router isis default
 net 49.0001.0000.0000.0001.00
 is-type level-2-only
 topology ipv6-unicast
!
segment-routing
 srv6
  locators
   locator LOC1
    prefix 2001:db8:100::/48 block-len 32 node-len 16 func-bits 16
    behavior usid
    format usid-f3216
   exit
  exit
  encapsulation
   source-address 2001:db8::1
  exit
  static-sids
   sid 2001:db8:100:a::/128 locator LOC1 behavior uA interface Ethernet0 nexthop fe80::2
  exit
 exit
exit
!
router isis default
 segment-routing srv6
  locator LOC1
  interface Loopback0
 exit
exit
!
router bgp 65000
 neighbor 192.0.2.1 password startup-secret
 address-family ipv4 unicast
  neighbor 192.0.2.1 activate
'''


class MemoryStore:
    def __init__(self, record=None, failure=None):
        self.record, self.failure = record, failure
        self.events = []

    @contextmanager
    def guard(self):
        self.events.append('guard')
        if self.failure == 'guard':
            raise OSError('injected lock failure')
        try:
            yield
        finally:
            self.events.append('unguard')

    def load(self):
        self.events.append('load')
        if self.failure == 'load':
            raise ValueError('injected invalid journal')
        return self.record

    def save(self, record):
        self.events.append(('save', record.status))
        if self.failure == 'save':
            raise OSError('injected persistence failure')
        self.record = record


@pytest.fixture
def startup_inputs(tmp_path):
    config = {
        'PORT': {'Ethernet0': {}},
        'LOOPBACK_INTERFACE': {'Loopback0': {}},
        'ISIS_GLOBALS': {'default': {
            'enabled': 'true', 'net_title': '49.0001.0000.0000.0001.00',
            'level': 'level-2', 'srv6_locator': 'LOC1', 'srv6_interface': 'Loopback0'}},
        'ISIS_INTERFACE': {'Ethernet0': {'enabled': 'true'}},
        'SRV6_MY_LOCATORS': {'LOC1': {
            'prefix': '2001:db8:100::/48', 'behavior': 'usid', 'format': 'usid-f3216'}},
        'SRV6_MY_SOURCE': {'tunnel-source': {'source-address': '2001:db8::1'}},
        'SRV6_MY_SIDS': {'LOC1|2001:db8:100:a::/128': {
            'action': 'uA', 'interface': 'Ethernet0', 'adj': 'fe80::2'}},
        'BGP_NEIGHBOR': {'192.0.2.1': {'password': 'startup-secret'}},
    }
    snapshot, rendered, defaults = [tmp_path / name for name in
                                    ('snapshot.json', 'candidate.conf', 'defaults.json')]
    snapshot.write_text(json.dumps(config))
    snapshot.chmod(0o600)
    rendered.write_text(FRAGMENT)
    defaults.write_text(json.dumps({'frr_version': '10.5.4',
                                   'defaults': asdict(verified_defaults((10, 5, 4)))}))
    return snapshot, rendered, defaults


def test_startup_registers_only_pending_under_guard_and_keeps_inputs(startup_inputs):
    before = [path.read_bytes() for path in startup_inputs]
    store = MemoryStore()
    result = prepare_startup(*startup_inputs, store=store, clock=lambda: 100)
    assert result.status == 'prepared' and len(result.digest) == 64
    assert store.events == ['guard', 'load', ('save', 'pending'), 'unguard']
    assert store.record.status == 'pending' and not store.record.owned
    assert Identity('isis-process', 'default') in store.record.possible
    assert Identity('srv6-sid', '2001:db8:100:a::/128') in store.record.possible
    assert store.record.deadline == 130
    assert [path.read_bytes() for path in startup_inputs] == before
    assert 'startup-secret' not in json.dumps(result.as_json())


@pytest.mark.parametrize('failure', ['guard', 'load', 'save'])
def test_startup_metadata_failure_requires_fallback(startup_inputs, failure):
    store = MemoryStore(failure=failure)
    result = prepare_startup(*startup_inputs, store=store)
    assert result.status == 'fallback' and not result.digest
    assert store.record is None
    assert all(event != ('save', 'applied') for event in store.events)


@pytest.mark.parametrize('domain', ['isis', 'srv6'])
def test_startup_validates_both_domains_before_parser(startup_inputs, monkeypatch, domain):
    snapshot = startup_inputs[0]
    config = json.loads(snapshot.read_text())
    if domain == 'isis':
        config['ISIS_INTERFACE']['Ethernet0']['metric'] = '0'
    else:
        config['SRV6_MY_LOCATORS']['LOC1']['arg_len'] = '16'
    snapshot.write_text(json.dumps(config))
    calls = []
    monkeypatch.setattr('frrcfgd.isis_startup.parse_canonical_config',
                        lambda *args: calls.append(args))
    store = MemoryStore()
    result = prepare_startup(*startup_inputs, store=store)
    assert result.status == 'fallback'
    assert not calls and not store.events


@pytest.mark.parametrize('problem', ['snapshot-json', 'snapshot-mode', 'render-empty',
                                     'defaults-schema', 'defaults-type', 'defaults-version'])
def test_startup_bad_input_does_not_touch_journal(startup_inputs, problem):
    snapshot, rendered, defaults = startup_inputs
    if problem == 'snapshot-json':
        snapshot.write_text('{')
    elif problem == 'snapshot-mode':
        snapshot.chmod(0o644)
    elif problem == 'render-empty':
        rendered.write_text('')
    else:
        value = json.loads(defaults.read_text())
        if problem == 'defaults-schema':
            value['defaults'].pop('lsp_mtu')
        elif problem == 'defaults-type':
            value['defaults']['metric'] = True
        else:
            value['frr_version'] = 'unknown'
        defaults.write_text(json.dumps(value))
    store = MemoryStore()
    result = prepare_startup(*startup_inputs, store=store)
    assert result.status == 'fallback' and not store.events


def test_startup_reentry_preserves_deadline_and_expiry_requires_fallback(startup_inputs):
    store = MemoryStore()
    assert prepare_startup(*startup_inputs, store=store, timeout=10, clock=lambda: 100).status == 'prepared'
    first = store.record
    assert prepare_startup(*startup_inputs, store=store, timeout=10, clock=lambda: 105).status == 'prepared'
    assert store.record.deadline == first.deadline == 110
    expired = prepare_startup(*startup_inputs, store=store, timeout=10, clock=lambda: 111)
    assert expired.status == 'fallback' and store.record.status == 'failed'
    assert store.record.possible == first.possible
    assert prepare_startup(*startup_inputs, store=store, timeout=10, clock=lambda: 112).status == 'fallback'


@pytest.mark.parametrize('previous_phase', ['pending', 'failed', 'legacy'])
def test_new_boot_rearms_f3216_after_previous_deadline(startup_inputs, monkeypatch, previous_phase):
    boot = '00000000-0000-0000-0000-000000000001'
    monkeypatch.setattr('frrcfgd.isis_startup.read_boot_id', lambda: boot, raising=False)
    snapshot, rendered, _ = startup_inputs
    config = json.loads(snapshot.read_text())
    config['ISIS_INTERFACE']['Ethernet0']['metric'] = '20'
    config['SRV6_MY_SIDS']['LOC1|2001:db8:100:fe01::/64'] = (
        config['SRV6_MY_SIDS'].pop('LOC1|2001:db8:100:a::/128')
    )
    snapshot.write_text(json.dumps(config))
    rendered.write_text(FRAGMENT.replace('isis metric 10', 'isis metric 20')
                        .replace('2001:db8:100:a::/128', '2001:db8:100:fe01::/64'))
    store = MemoryStore()
    assert prepare_startup(*startup_inputs, store=store, timeout=30, clock=lambda: 100).status == 'prepared'

    old_owned = Identity('srv6-locator', 'OLD')
    old_possible = Identity('srv6-sid', '2001:db8:200:fe01::/64')
    previous = replace(store.record, owned=frozenset({old_owned}),
                       possible=store.record.possible | {old_possible})
    if previous_phase == 'legacy':
        old_json = previous.as_json()
        old_json.pop('boot_id', None)
        previous = type(previous).from_json(old_json)
    if previous_phase == 'failed':
        previous = replace(previous, status='failed', reason='Recovery deadline expired')
    store.record = previous

    boot = '00000000-0000-0000-0000-000000000002'
    assert prepare_startup(*startup_inputs, store=store, timeout=30, clock=lambda: 200).status == 'prepared'
    record = store.record
    assert record.status == 'pending' and record.deadline == 230
    assert record.digest == previous.digest and record.boot_id == boot
    assert record.owned == previous.owned and record.possible == previous.possible | {old_owned}
    assert Identity('srv6-sid', '2001:db8:100:fe01::/64') in record.possible
    assert type(record).from_json(record.as_json()) == record

    assert prepare_startup(*startup_inputs, store=store, timeout=30, clock=lambda: 210).status == 'prepared'
    assert store.record.deadline == 230
    assert prepare_startup(*startup_inputs, store=store, timeout=30, clock=lambda: 231).status == 'fallback'
    assert prepare_startup(*startup_inputs, store=store, timeout=30, clock=lambda: 232).status == 'fallback'
    assert store.record.status == 'failed' and store.record.deadline == 230


def test_startup_retains_old_authority_when_desired_identity_changes(startup_inputs):
    store = MemoryStore()
    prepare_startup(*startup_inputs, store=store, clock=lambda: 100)
    old = Identity('isis-process', 'old-tag')
    store.record = replace(store.record, owned=frozenset({old}), possible=frozenset({old}))
    result = prepare_startup(*startup_inputs, store=store, clock=lambda: 101)
    assert result.status == 'prepared' and old in store.record.owned and old in store.record.possible


def test_startup_actual_atomic_store_is_mode_0600_and_pending(startup_inputs, tmp_path):
    store = AtomicJSONStore(tmp_path / 'state' / 'frrcfgd-owned.json')
    result = prepare_startup(*startup_inputs, store=store)
    assert result.status == 'prepared' and store.load().status == 'pending'
    assert stat.S_IMODE(store.path.stat().st_mode) == 0o600


def test_startup_cli_reports_fallback_without_input_contents(startup_inputs, tmp_path, capsys):
    snapshot, rendered, defaults = startup_inputs
    journal = tmp_path / 'state.json'
    arguments = ['--snapshot', str(snapshot), '--rendered-config', str(rendered),
                 '--native-defaults', str(defaults), '--journal', str(journal)]
    assert main(arguments) == 0
    assert json.loads(capsys.readouterr().out)['status'] == 'prepared'
    journal.write_text('{')
    assert main(arguments) == 1
    output = capsys.readouterr().out
    assert json.loads(output)['status'] == 'fallback'
    assert 'startup-secret' not in output


@pytest.mark.parametrize('version', ['10.5.4', '10.7.1'])
def test_startup_installed_version_selector_and_unknown_version_fallback(startup_inputs, version):
    snapshot, rendered, _ = startup_inputs
    store = MemoryStore()
    assert prepare_startup(snapshot, rendered, frr_version=version, store=store).status == 'prepared'
    other = MemoryStore()
    assert prepare_startup(snapshot, rendered, frr_version='10.8.1', store=other).status == 'fallback'
    assert not other.events


@pytest.mark.parametrize('mixed', [False, True])
def test_startup_named_profiles_keep_legacy_path_without_adoption(startup_inputs, mixed):
    snapshot = startup_inputs[0]
    config = json.loads(snapshot.read_text())
    config['ISIS_GLOBALS'] = {'Blue': {'enabled': 'true'}}
    if mixed:
        config['ISIS_GLOBALS']['default'] = {'enabled': 'true'}
    snapshot.write_text(json.dumps(config))
    store = MemoryStore()
    outcome = prepare_startup(*startup_inputs, store=store)
    assert outcome.status == 'legacy' and outcome.profile == 'legacy'
    assert not store.events and not outcome.digest


@pytest.mark.parametrize('field', ['srv6_locator', 'srv6_interface'])
def test_startup_named_profile_new_binding_requires_legacy_preserving_fallback(startup_inputs, field):
    snapshot = startup_inputs[0]
    config = json.loads(snapshot.read_text())
    config['ISIS_GLOBALS'] = {'Blue': {'enabled': 'true', field: 'LOC1'}}
    snapshot.write_text(json.dumps(config))
    store = MemoryStore()
    outcome = prepare_startup(*startup_inputs, store=store)
    assert outcome.status == 'fallback' and outcome.profile == 'legacy'
    assert not store.events


@pytest.mark.parametrize('namespace', ['0', '1'])
def test_startup_nondefault_namespace_preserves_legacy_isis_without_adoption(startup_inputs, namespace):
    store = MemoryStore()
    result = prepare_startup(*startup_inputs, namespace=namespace, store=store)
    assert result.status == 'fallback' and result.profile == 'legacy'
    assert not result.digest and not store.events


def test_explicit_version_normalization_is_shared_between_runtime_and_startup():
    assert normalize_version('10.5.4') == normalize_version((10, 5, 4)) == (10, 5, 4)
    defaults = verified_defaults('10.5.4')
    assert parse_canonical_config(FRAGMENT, defaults, (10, 5, 4)) == \
        parse_canonical_config(FRAGMENT, defaults, '10.5.4')
    assert defaults.block_len == 40 and defaults.node_len == 24
    assert verified_defaults((10, 7, 1)) == defaults


def test_canonical_parser_merges_only_default_process_and_preserves_native_strictness():
    defaults = verified_defaults((10, 5, 4))
    result = parse_canonical_config(FRAGMENT, defaults, '10.5.4')
    assert result.objects[Identity('isis-process', 'default')]['level'] == 'level-2'
    assert result.objects[Identity('isis-srv6', 'default')] == {'locator': 'LOC1', 'interface': 'Loopback0'}
    assert not any('startup-secret' in str(value) for value in result.objects.values())
    with pytest.raises(RecoveryError, match='Duplicate native identity'):
        parse_running_config('frr version 10.5.4\n' + FRAGMENT + '\nend\n', defaults)


@pytest.mark.parametrize('extra', [
    'router isis default\n is-type level-1\n!',
    'router isis default\n net 49.0001.0000.0000.0001.00\n!',
    'router isis default\n mystery setting\n!',
    'router isis foreign\n net 49.0001.0000.0000.0002.00\n!',
    'segment-routing\n srv6\n  locators\n   locator LOC1\n    prefix 2001:db8:200::/48\n!',
])
def test_canonical_parser_rejects_conflicts_and_other_duplicate_identities(extra):
    with pytest.raises(RecoveryError):
        parse_canonical_config(FRAGMENT + '\n' + extra, verified_defaults((10, 5, 4)), '10.5.4')


def test_canonical_parser_rejects_native_adoption_and_version_mismatch():
    defaults = verified_defaults((10, 5, 4))
    with pytest.raises(RecoveryError):
        parse_canonical_config('Building configuration...\n' + FRAGMENT, defaults, '10.5.4')
    with pytest.raises(RecoveryError):
        parse_canonical_config('frr version 10.7.1\n' + FRAGMENT, defaults, '10.5.4')


def test_projection_preserves_raw_msd_and_only_retained_unconfirmed_identities():
    active, orphan = Identity('isis-circuit', 'Ethernet0'), Identity('isis-circuit', 'Ethernet4')
    srv6 = Identity('isis-srv6', 'default')
    snapshot = NativeSnapshot({
        active: {'metric': 10}, orphan: {'metric': 20},
        Identity('isis-binding', 'Ethernet0|ipv4|default'): {'interface': 'Ethernet0'},
        srv6: {'locator': 'LOC1', 'node_msd': {'max-segs-left': 9}},
    }, unconfirmed=frozenset({active, orphan}))
    projected = project_managed_snapshot(snapshot)
    assert orphan not in projected.objects and active in projected.objects
    assert projected.unconfirmed == frozenset({active})
    assert 'node_msd' not in projected.objects[srv6]
    assert snapshot.objects[srv6]['node_msd'] == {'max-segs-left': 9}
    assert orphan in snapshot.objects and snapshot.unconfirmed == frozenset({active, orphan})
