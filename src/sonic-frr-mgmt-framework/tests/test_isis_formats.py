"""CONFIG_DB format persistence through the startup and runtime adapters."""

import copy
import json
from pathlib import Path
from types import SimpleNamespace

import pytest
from jinja2 import Environment, FileSystemLoader

from frrcfgd.isis_config import ConfigError, MANAGED_TABLES, validate_srv6_config
from frrcfgd.isis_desired import parse_canonical_config, verified_defaults
from frrcfgd.isis_manager import ConfigRenderer, DeltaPlanner
from frrcfgd.isis_recovery import (
    AtomicJSONStore, DesiredState, Identity, NativeSnapshot, RecoveryError,
    RecoveryKernel, parse_running_config,
)
from frrcfgd.isis_snapshot import TABLES, capture
from frrcfgd.isis_startup import prepare_startup
from .test_isis_recovery import MemoryStore, Native, plan, registered


FORMAT = Identity('srv6-format', 'usid-f3216')
LOCATOR = Identity('srv6-locator', 'LOC1')
VERSION = (10, 5, 4)
DEFAULTS = verified_defaults(VERSION)
TEMPLATES = Path(__file__).parents[1] / 'templates' / 'srv6'


class JournalNative(Native):
    def __init__(self, objects, journal):
        super().__init__(objects, MemoryStore())
        self.journal = journal

    def apply(self, payload):
        self.store.record = self.journal.load()
        return super().apply(payload)


def config(start='53248'):
    return {
        'SRV6_MY_FORMATS': {'usid-f3216': {'lib_start': start}},
        'SRV6_MY_LOCATORS': {'LOC1': {
            'prefix': '2001:db8:100::/48', 'behavior': 'usid', 'format': 'usid-f3216'}},
    }


def render(data):
    validate_srv6_config(data, VERSION)
    return Environment(loader=FileSystemLoader(str(TEMPLATES))).get_template('srv6.conf.j2').render(**data)


def desired(data):
    return DesiredState(parse_canonical_config(render(data), DEFAULTS, VERSION))


def writer(value, compressed=False):
    nesting = '    compressed usid\n ' if compressed else ''
    return ('frr version 10.5.4\nsegment-routing\n srv6\n  formats\n'
            '   format usid-f3216\n' + nesting + '    local-id-block start ' + str(value) + '\nend\n')


@pytest.mark.parametrize('start', ['1', '53248', '57344', '65023'])
def test_format_schema_validation_and_render(start):
    data = config(start)
    before = copy.deepcopy(data)
    state = desired(data)
    assert state.snapshot.objects[FORMAT] == {'lib_start': int(start)}
    assert data == before


@pytest.mark.parametrize('start', ['0', '65024', '-1', '65536', '0xD000', True, '1.5', None])
def test_invalid_format_boundary_fails_before_render(start):
    with pytest.raises(ConfigError):
        desired(config(start))


@pytest.mark.parametrize('change', ['format-name', 'unknown-field', 'missing-start', 'unused-format'])
def test_format_contract_rejects_unrepresentable_rows(change):
    data = config()
    if change == 'format-name':
        data['SRV6_MY_FORMATS']['usid-f4816'] = data['SRV6_MY_FORMATS'].pop('usid-f3216')
    elif change == 'unknown-field':
        data['SRV6_MY_FORMATS']['usid-f3216']['elib_start'] = '60000'
    elif change == 'missing-start':
        data['SRV6_MY_FORMATS']['usid-f3216'].clear()
    else:
        data['SRV6_MY_LOCATORS']['LOC1'].pop('format')
    with pytest.raises(ConfigError):
        desired(data)


@pytest.mark.parametrize('compressed', [False, True])
@pytest.mark.parametrize('value', ['53248', '0xD000'])
def test_native_format_writer_and_canonical_nesting_have_equal_semantics(value, compressed):
    observed = parse_running_config(writer(value, compressed), DEFAULTS)
    assert not observed.unsupported
    assert observed.objects[FORMAT] == {'lib_start': 53248}


def test_native_default_omission_and_explicit_default_compare_equal():
    target = desired(config('57344'))
    native = NativeSnapshot({LOCATOR: target.snapshot.objects[LOCATOR]})
    assert FORMAT not in parse_running_config(writer(57344), DEFAULTS).objects
    state = Native(native.objects, MemoryStore(registered(target)))
    store = state.store
    result = RecoveryKernel(store, clock=lambda: 101).step(
        lambda: target, state.readback, plan, state.apply, 10)
    assert result.status == 'applied' and not result.mutation_attempted


def test_legacy_configs_without_format_table_do_not_acquire_format_ownership():
    data = config()
    del data['SRV6_MY_FORMATS']
    assert FORMAT not in desired(data).snapshot.objects


def test_unknown_native_format_leaf_prevents_mutation():
    body = writer(53248).replace(
        '    local-id-block start 53248',
        '    local-id-block start 53248\n    wide-local-id-block start 65520 end 65527')
    native = parse_running_config(body, DEFAULTS)
    target = desired(config('49152'))
    store = MemoryStore(registered(desired(config())))
    result = RecoveryKernel(store, clock=lambda: 101).step(
        lambda: target, lambda: native, plan, lambda _: pytest.fail('unsafe mutation'), 10)
    assert result.status == 'conflict'


def test_format_table_is_captured_from_configdb_including_tuple_keys():
    class Connector:
        def get_table(self, name):
            return {('usid-f3216',): {'lib_start': '53248'}} if name == 'SRV6_MY_FORMATS' else {}
    assert 'SRV6_MY_FORMATS' in MANAGED_TABLES and 'SRV6_MY_FORMATS' in TABLES
    assert capture(Connector())['SRV6_MY_FORMATS'] == {'usid-f3216': {'lib_start': '53248'}}


def test_runtime_renderer_passes_format_table_to_both_cfggen_renders(monkeypatch):
    snapshots = []

    def cfggen(command, **kwargs):
        snapshot = json.loads(Path(command[command.index('-j') + 1]).read_text())
        snapshots.append(snapshot)
        fragment = render(snapshot) if command[-1].endswith('srv6.conf.j2') else '! IS-IS disabled\n'
        return SimpleNamespace(stdout=fragment)

    monkeypatch.setattr('frrcfgd.isis_manager.subprocess.run', cfggen)
    state = ConfigRenderer(DEFAULTS, VERSION)(config())
    assert len(snapshots) == 2
    assert all(snapshot['SRV6_MY_FORMATS'] == config()['SRV6_MY_FORMATS'] for snapshot in snapshots)
    assert state.snapshot.objects[FORMAT] == {'lib_start': 53248}


@pytest.mark.parametrize('start', ['49152', '57344', None])
def test_format_update_and_delete_reset_owned_override_before_recreating_locators(start):
    native = desired(config()).snapshot
    data = config(start)
    if start is None:
        del data['SRV6_MY_FORMATS']
    target = desired(data).snapshot
    mutation = DeltaPlanner(DEFAULTS)(native, target, frozenset(native.objects | target.objects))
    commands = [command for operation in mutation.payload for command in operation.commands]
    wanted = 'no local-id-block' if start is None else 'local-id-block start ' + start
    assert commands.index('no locator LOC1') < commands.index(wanted) < commands.index('locator LOC1')
    assert 'no format usid-f3216' not in commands
    assert mutation.identities == frozenset(native.objects | target.objects)


def test_changing_shared_format_preserves_ownership_boundary():
    native = desired(config()).snapshot
    objects = copy.deepcopy(native.objects)
    objects[Identity('srv6-locator', 'FOREIGN')] = dict(objects[LOCATOR], prefix='2001:db8:200::/48')
    with pytest.raises(RecoveryError, match='foreign locator'):
        DeltaPlanner(DEFAULTS)(NativeSnapshot(objects), desired(config('49152')).snapshot, frozenset(native.objects))


def test_changing_shared_format_cannot_detach_foreign_sid():
    native = desired(config()).snapshot
    objects = copy.deepcopy(native.objects)
    objects[Identity('srv6-sid', '2001:db8:100:fe01::/64')] = {
        'locator': 'LOC1', 'behavior': 'uA', 'interface': 'Ethernet0', 'adj': 'fe80::2', 'vrf': None}
    with pytest.raises(RecoveryError, match='foreign reference'):
        DeltaPlanner(DEFAULTS)(NativeSnapshot(objects), desired(config('49152')).snapshot, frozenset(native.objects))


@pytest.mark.parametrize('event', ['reload', 'cold-boot'])
def test_saved_json_restores_boundary_with_startup_registration(tmp_path, monkeypatch, event):
    saved = tmp_path / 'config_db.json'
    saved.write_text(json.dumps(config()))
    rendered = tmp_path / 'candidate.conf'
    snapshot = tmp_path / 'snapshot.json'
    snapshot.write_bytes(saved.read_bytes())
    snapshot.chmod(0o600)
    restored_config = json.loads(snapshot.read_text())
    rendered.write_text(render(restored_config))
    target = desired(restored_config)
    prior = desired(config('49152'))
    old_record = registered(prior, boot_id='00000000-0000-0000-0000-000000000001')
    current_boot = (old_record.boot_id if event == 'reload'
                    else '00000000-0000-0000-0000-000000000002')
    monkeypatch.setattr('frrcfgd.isis_startup.read_boot_id', lambda: current_boot)
    store = AtomicJSONStore(tmp_path / 'journal.json')
    store.save(old_record)
    outcome = prepare_startup(snapshot, rendered, frr_version=VERSION, store=store, clock=lambda: 100)
    assert outcome.status == 'prepared' and outcome.digest == target.digest
    assert store.load().boot_id == current_boot
    assert FORMAT in store.load().possible | store.load().owned
    native = JournalNative(prior.snapshot.objects if event == 'reload' else {}, store)
    result = RecoveryKernel(store, clock=lambda: 101).step(
        lambda: target, native.readback, plan, native.apply, 10)
    assert result.status == 'applied'
    assert native.objects[FORMAT] == {'lib_start': 53248}
    assert json.loads(saved.read_text()) == config()


@pytest.mark.parametrize('start', ['49152', None])
def test_offline_update_delete_survive_journal_reload(tmp_path, start):
    original = desired(config())
    store = AtomicJSONStore(tmp_path / 'journal.json')
    store.save(registered(original))
    data = config(start)
    if start is None:
        del data['SRV6_MY_FORMATS']
    target = desired(data)
    native = JournalNative(original.snapshot.objects, store)
    # A new adapter instance loads ownership after an update while it was stopped.
    restarted_store = AtomicJSONStore(store.path)
    result = RecoveryKernel(restarted_store, clock=lambda: 101).step(
        lambda: target, native.readback, plan, native.apply, 10)
    assert result.status == 'applied'
    assert native.objects.get(FORMAT) == target.snapshot.objects.get(FORMAT)
