"""Multi-ASIC discovery and recovery in disposable fixtures, without a SONiC image."""
import copy
import json
import os
from pathlib import Path

import pytest

import redis_auth_startup as startup
import test_redis_auth_startup as existing
from test_redis_auth_startup import Redis, configure


@pytest.fixture
def multi(tmp_path, monkeypatch):
    if os.geteuid() != 0:
        pytest.skip('requires root in a disposable container')
    platform = startup.platform_directory()
    (platform / 'asic.conf').write_text('NUM_ASIC=2\n')
    monkeypatch.setattr(startup, 'build_enabled', lambda: True)
    intent = tmp_path / 'intent.json'
    intent.write_text('{"mode":"enforce"}')
    return intent, tmp_path / 'runtime', platform


def prepared(multi):
    intent, runtime, _ = multi
    selected = startup.prepare_selected(intent_path=intent, runtime=runtime)
    return selected, json.loads((runtime / 'server/instances.json').read_text()), json.loads((runtime / 'local.json').read_text())


def container_map(inventory, number, address='172.17.0.2'):
    prefix = 'asic' + number + '/' if number else ''
    instances = {key[len(prefix):]: {field: value[field] for field in ('hostname', 'port', 'unix_socket_path')}
                 for key, value in inventory.items() if (key.startswith(prefix) if prefix else '/' not in key)}
    if number:
        for item in instances.values():
            item['hostname'] = address
    return {'INSTANCES': instances}, {'INSTANCES': {name: {'is_protected_mode': not number} for name in instances}}


@pytest.mark.parametrize('count', [2, 4, 6])
def test_inventory_is_available_before_any_database_or_netns_exists(multi, count):
    intent, runtime, platform = multi
    (platform / 'asic.conf').write_text(f'NUM_ASIC={count}\n')
    selected, inventory, policy = prepared(multi)
    assert len(selected) == 2 * (count + 1) == len(policy)
    assert len((runtime / 'local.json').read_bytes()) <= 4096
    assert all(item['tcp_host'] == '127.0.0.1' and item['tcp_port'] == 0 for item in policy)
    for number in [''] + list(map(str, range(count))):
        database, additional = container_map(inventory, number)
        assert configure.configure(database, additional, policy, inventory,
            environ={'NAMESPACE_ID': number, 'NAMESPACE_COUNT': str(count)},
            local_ip='172.17.0.2', chassis=False) == {'redis', 'redis_bmp'}
        assert all(item['port'] == 0 for item in database['INSTANCES'].values())
    assert startup.runtime_required(intent_path=intent, runtime=runtime)


def test_larger_inventory_fails_before_creating_password(multi):
    intent, runtime, platform = multi
    (platform / 'asic.conf').write_text('NUM_ASIC=16\n')
    with pytest.raises(startup.CredentialStateError, match='reader capacity'):
        startup.prepare_selected(intent_path=intent, runtime=runtime)
    assert not runtime.exists()


def test_namespace_recreation_does_not_pin_its_bridge_address(multi):
    intent, runtime, _ = multi
    _, inventory, policy = prepared(multi)
    secret = (runtime / 'local-system/writer.secret').read_bytes()
    for address in ('172.17.0.2', '172.17.0.9'):
        database, additional = container_map(inventory, '1', address)
        configure.configure(database, additional, policy, inventory,
            environ={'NAMESPACE_ID': '1', 'NAMESPACE_COUNT': '2'}, local_ip=address, chassis=False)
        assert all(item['port'] == 0 for item in database['INSTANCES'].values())
        startup.prepare_selected(intent_path=intent, runtime=runtime)
        assert (runtime / 'local-system/writer.secret').read_bytes() == secret
    database, additional = container_map(inventory, '1', '192.0.2.3')
    with pytest.raises(ValueError, match='endpoint'):
        configure.configure(database, additional, policy, inventory,
            environ={'NAMESPACE_ID': '1', 'NAMESPACE_COUNT': '2'}, local_ip='172.17.0.9', chassis=False)


@pytest.mark.parametrize('fault', ['missing-sibling', 'wrong-socket', 'duplicate-policy', 'foreign-policy', 'wrong-count', 'wrong-namespace'])
def test_namespace_inventory_and_policy_must_agree(multi, fault):
    _, inventory, policy = prepared(multi)
    database, additional = container_map(inventory, '0')
    environ = {'NAMESPACE_ID': '0', 'NAMESPACE_COUNT': '2'}
    if fault == 'missing-sibling':
        policy.pop()
    elif fault == 'wrong-socket':
        database['INSTANCES']['redis']['unix_socket_path'] = '/var/run/redis1/redis.sock'
    elif fault == 'duplicate-policy':
        policy.append(policy[0])
    elif fault == 'foreign-policy':
        policy[0]['unix_socket'] = '/var/run/redis9/redis.sock'
    elif fault == 'wrong-count':
        environ['NAMESPACE_COUNT'] = '4'
    else:
        environ['NAMESPACE_ID'] = '2'
    with pytest.raises(ValueError):
        configure.configure(database, additional, policy, inventory, environ=environ,
                            local_ip='172.17.0.2', chassis=False)


def test_logical_tcp_exception_applies_to_every_namespace(multi):
    intent, runtime, _ = multi
    intent.write_text('{"mode":"enforce","tcp_exceptions":["redis_bmp"]}')
    selected, inventory, policy = prepared(multi)
    assert selected == ['asic0/redis', 'asic1/redis', 'redis']
    for number in ('', '0', '1'):
        database, additional = container_map(inventory, number)
        configure.configure(database, additional, policy, inventory,
            environ={'NAMESPACE_ID': number, 'NAMESPACE_COUNT': '2'}, local_ip='172.17.0.2', chassis=False)
        assert database['INSTANCES']['redis']['port'] == 0
        assert database['INSTANCES']['redis_bmp']['port'] == 6400


def test_saved_namespace_override_survives_restore(multi):
    intent, runtime, _ = multi
    inventory = startup.configured_instances()
    override, _ = container_map(inventory, '0', '127.0.0.1')
    override['INSTANCES']['redis']['port'] = 6391
    saved = startup.CONFIG / 'old_config'
    saved.mkdir()
    path = saved / 'database_config0.json'
    path.write_text(json.dumps(override))
    _, inventory, _ = prepared(multi)
    assert inventory['asic0/redis']['port'] == 6391
    path.rename(startup.CONFIG / path.name)
    assert startup.runtime_required(intent_path=intent, runtime=runtime)


def test_real_redis_sibling_isolation_and_independent_recovery(multi, root_directory):
    _, runtime, _ = multi
    _, inventory, policy = prepared(multi)
    harness = existing.StartupTest(methodName='runTest')
    harness.root = root_directory / 'harness'
    harness.root.mkdir(mode=0o755)
    harness.runtime = harness.root / 'run'
    import shutil
    shutil.copytree(runtime, harness.runtime)
    runtime = harness.runtime
    processes, clients = {}, {}
    try:
        for identity in inventory:
            path = harness.root / 'sockets' / (identity.replace('/', '-') + '.sock')
            processes[identity] = harness.start_socket_server(path, enforce=True)
            clients[identity] = Redis(path)
            client = clients[identity]
            assert client.command('AUTH', startup.USERNAME, (runtime / 'local-system/writer.secret').read_bytes()) == b'OK'
            assert client.command('CONFIG', 'GET', 'port') == [b'port', b'0']
            assert client.command('SET', 'same-key', identity) == b'OK'
            assert harness.as_reader(path, 'GET', 'same-key', gid=1001).stdout.strip() == identity
            denial = harness.as_reader(path, 'SET', 'same-key', 'forbidden', gid=1001)
            assert 'NOPERM' in denial.stdout + denial.stderr
            private_reader = harness.as_reader(path, 'GET', 'same-key', gid=65534)
            assert private_reader.returncode == 0
            assert private_reader.stdout.strip() == identity
            private_denial = harness.as_reader(path, 'SET', 'same-key', 'forbidden', gid=65534)
            assert 'NOPERM' in private_denial.stdout + private_denial.stderr
            assert client.command('GET', 'same-key') == identity.encode()
        target = 'asic0/redis'
        clients.pop(target).close()
        processes[target].terminate()
        assert processes[target].wait(timeout=5) == 0
        path = harness.root / 'sockets' / 'asic0-redis.sock'
        harness.start_socket_server(path, enforce=True)
        writer, subscriber = Redis(path), Redis(path)
        try:
            assert writer.command('AUTH', startup.USERNAME, (runtime / 'local-system/writer.secret').read_bytes()) == b'OK'
            assert subscriber.command('SUBSCRIBE', 'probe') == [b'subscribe', b'probe', 1]
            assert writer.command('PUBLISH', 'probe', 'recovered') == 1
            assert subscriber.reply() == [b'message', b'probe', b'recovered']
            for identity, client in clients.items():
                assert client.command('GET', 'same-key') == identity.encode()
        finally:
            writer.close()
            subscriber.close()
    finally:
        for client in clients.values():
            client.close()
        harness.doCleanups()


def test_declared_layout_is_valid_before_and_after_namespace_creation(multi):
    assert not startup.NETNS.exists()
    assert startup.check_topology() == ['', '0', '1']
    for number in ('0', '1'):
        startup.NETNS.mkdir(exist_ok=True)
        (startup.NETNS / ('asic' + number)).touch()
        (startup.CONFIG / ('config_db' + number + '.json')).write_text('{}')
    assert startup.check_topology() == ['', '0', '1']
    (startup.NETNS / 'asic2').touch()
    with pytest.raises(startup.CredentialStateError, match='namespace layout'):
        startup.check_topology()


@pytest.mark.parametrize('saved', [False, True])
def test_global_include_map_must_cover_exact_declared_layout(multi, saved):
    intent, runtime, _ = multi
    directory = startup.CONFIG / ('old_config' if saved else '')
    directory.mkdir(exist_ok=True)
    path = directory / 'database_global.json'
    includes = [
        {'include': '../../redis/sonic-db/database_config.json'},
        {'namespace': 'asic0', 'include': '../../redis0/sonic-db/database_config.json'},
        {'namespace': 'asic1', 'include': '../../redis1/sonic-db/database_config.json'},
    ]
    path.write_text(json.dumps({'INCLUDES': includes}))
    assert startup.check_topology() == ['', '0', '1']
    includes[-1]['namespace'] = 'asic2'
    path.write_text(json.dumps({'INCLUDES': includes}))
    with pytest.raises(startup.CredentialStateError, match='global database includes'):
        startup.prepare_selected(intent_path=intent, runtime=runtime)
    assert not runtime.exists()
