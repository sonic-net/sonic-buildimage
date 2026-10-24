"""Common activation, earlier-format compatibility and generated process inventory."""
import copy
import json
import os

import pytest

import redis_auth_startup as startup


@pytest.fixture
def boot(tmp_path, monkeypatch):
    if os.geteuid() != 0:
        pytest.skip('requires root in an isolated container')
    intent = tmp_path / 'intent.json'
    monkeypatch.setattr(startup, 'build_enabled', lambda: True)
    monkeypatch.setattr(startup, 'socket_groups', lambda: [100, 1001])
    return intent, tmp_path / 'runtime'


@pytest.mark.parametrize('setting,mode', [({}, 'legacy'), ({'mode': 'legacy'}, 'legacy'),
    ({'mode': 'enforce'}, 'enforce'), ({'redis': 'legacy'}, 'legacy'),
    ({'redis_bmp': 'legacy'}, 'legacy'), ({'redis': 'legacy', 'redis_bmp': 'legacy'}, 'legacy'),
    ({'redis': 'enforce', 'redis_bmp': 'enforce'}, 'enforce')])
def test_normalization_preserves_stored_file(boot, setting, mode):
    intent, _ = boot
    content = json.dumps(setting)
    intent.write_text(content)
    assert startup.read_intent(intent) == {'mode': mode, 'tcp_exceptions': []}
    assert intent.read_text() == content


@pytest.mark.parametrize('setting', [
    {'redis': 'enforce'}, {'redis_bmp': 'enforce'}, {'redis': 'enforce', 'redis_bmp': 'legacy'},
    {'redis': 'enforce', 'mode': 'enforce'}, {'redis_bmp': 'legacy', 'tcp_exceptions': []},
    {'mode': 'auto'}, {'mode': True}, {'unknown': 'legacy'}, {'tcp_exceptions': 'redis_bmp'},
    {'tcp_exceptions': [None]}, {'tcp_exceptions': ['redis_bmp', 'redis_bmp']}, []])
def test_invalid_or_ambiguous_activation_is_rejected(boot, setting):
    intent, runtime = boot
    intent.write_text(json.dumps(setting))
    with pytest.raises(startup.CredentialStateError):
        startup.prepare_selected(intent_path=intent, runtime=runtime)
    assert not runtime.exists()


def test_common_enforcement_and_tcp_exception(boot):
    intent, runtime = boot
    intent.write_text('{"mode":"enforce","tcp_exceptions":["redis_bmp"]}')
    assert startup.prepare_selected(intent_path=intent, runtime=runtime) == ['redis']
    policy = json.loads((runtime / 'local.json').read_text())
    inventory = json.loads((runtime / 'server/instances.json').read_text())
    assert [item['unix_socket'] for item in policy] == ['/var/run/redis/redis.sock']
    assert inventory['redis']['protected'] is True
    assert inventory['redis_bmp']['protected'] is False
    assert inventory['redis_bmp']['port'] == 6400
    assert startup.runtime_required(intent_path=intent, runtime=runtime)
    original = (runtime / 'local-system/writer.secret').read_bytes()
    startup.prepare_selected(intent_path=intent, runtime=runtime)
    assert (runtime / 'local-system/writer.secret').read_bytes() == original
    intent.write_text('{"mode":"enforce"}')
    with pytest.raises(startup.CredentialStateError, match='activation changed'):
        startup.prepare_selected(intent_path=intent, runtime=runtime)


@pytest.mark.parametrize('exceptions', [['unknown'], ['redis', 'redis_bmp']])
def test_unknown_exception_or_empty_protected_set_is_rejected(boot, exceptions):
    intent, runtime = boot
    intent.write_text(json.dumps({'mode': 'enforce', 'tcp_exceptions': exceptions}))
    with pytest.raises(startup.CredentialStateError):
        startup.prepare_selected(intent_path=intent, runtime=runtime)
    assert not runtime.exists()


def test_configured_name_and_socket_need_no_activation_profile(boot):
    intent, runtime = boot
    instance = {'hostname': '127.0.0.1', 'port': 6381, 'unix_socket_path': '/var/run/redis/custom.sock'}
    (startup.CONFIG / 'database_config.json').write_text(json.dumps({
        'INSTANCES': {'local_custom': instance},
        'DATABASES': {'A': {'instance': 'local_custom'}, 'B': {'instance': 'local_custom'}}}))
    intent.write_text('{"mode":"enforce"}')
    assert startup.prepare_selected(intent_path=intent, runtime=runtime) == ['local_custom']
    policy = json.loads((runtime / 'local.json').read_text())
    assert len(policy) == 1 and policy[0]['unix_socket'] == instance['unix_socket_path']


def test_inventory_change_or_tampering_requires_reboot(boot):
    intent, runtime = boot
    intent.write_text('{"mode":"enforce"}')
    startup.prepare_selected(intent_path=intent, runtime=runtime)
    inventory = runtime / 'server/instances.json'
    source = json.loads(inventory.read_text())
    changed = copy.deepcopy(source)
    changed['redis_bmp']['port'] += 1
    inventory.write_text(json.dumps(changed))
    with pytest.raises(startup.CredentialStateError, match='inventory'):
        startup.runtime_required(intent_path=intent, runtime=runtime)


@pytest.mark.parametrize('field,value', [('hostname', '192.0.2.1'),
    ('unix_socket_path', '/tmp/redis.sock'), ('database_type', 'dpudb')])
def test_unqualified_endpoints_are_not_exempted_automatically(boot, field, value):
    intent, runtime = boot
    instances = startup.configured_instances()
    instances['redis'][field] = value
    (startup.CONFIG / 'database_config.json').write_text(json.dumps({'INSTANCES': instances}))
    intent.write_text('{"mode":"enforce"}')
    with pytest.raises(startup.CredentialStateError):
        startup.prepare_selected(intent_path=intent, runtime=runtime)
    assert not runtime.exists()


def test_policy_capacity_is_checked_before_password_creation(boot):
    intent, runtime = boot
    instances = {f'redis_{number}': {'hostname': '127.0.0.1', 'port': 6379 + number,
                                  'unix_socket_path': f'/var/run/redis/redis_{number}.sock'}
                 for number in range(40)}
    (startup.CONFIG / 'database_config.json').write_text(json.dumps({'INSTANCES': instances}))
    intent.write_text('{"mode":"enforce"}')
    with pytest.raises(startup.CredentialStateError, match='reader capacity'):
        startup.prepare_selected(intent_path=intent, runtime=runtime)
    assert not runtime.exists()
