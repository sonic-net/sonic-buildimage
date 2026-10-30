import json
import os
import hashlib
import subprocess
import tempfile
import time
from pathlib import Path
from unittest.mock import patch

import pytest

from sonic_py_common import redis_auth


@pytest.fixture
def runtime(tmp_path, monkeypatch, root_owned_auth_files):
    tmp_path.chmod(0o755)
    root_owned_auth_files(tmp_path)
    monkeypatch.setattr(redis_auth, 'AUTH_DIRECTORY', str(tmp_path))
    return tmp_path


def publish(runtime, entries=None):
    if entries is None:
        entries = {'username': 'sonic-trusted-writer',
                   'unix_socket': '/var/run/redis/redis.sock',
                   'tcp_host': '127.0.0.1', 'tcp_port': 0}
    policy = runtime / 'local.json'
    policy.write_text(json.dumps(entries))
    policy.chmod(0o644)
    secrets = runtime / 'local-system'
    secrets.mkdir(mode=0o755, exist_ok=True)
    secret = secrets / 'writer.secret'
    secret.write_text('test-value-never-log')
    secret.chmod(0o400)
    return secret


def test_absent_policy_keeps_legacy(runtime):
    assert redis_auth.connection_kwargs() == {}


def test_absent_directory_keeps_legacy(runtime, monkeypatch):
    monkeypatch.setattr(redis_auth, 'AUTH_DIRECTORY', str(runtime / 'absent'))
    assert redis_auth.connection_kwargs() == {}


def test_selected_unix_writer(runtime):
    publish(runtime)
    options = redis_auth.connection_kwargs(unix_socket_path='/var/run/redis/redis.sock')
    assert options['username'] == 'sonic-trusted-writer'
    assert options['password']


def test_unmatched_socket_does_not_read_password(runtime):
    publish(runtime).unlink()
    assert redis_auth.connection_kwargs(unix_socket_path='/var/run/redis/other.sock') == {}


def test_selected_tcp_fails_before_reading_password(runtime):
    publish(runtime, {'username': 'sonic-trusted-writer',
                      'unix_socket': '/var/run/redis/redis.sock',
                      'tcp_host': '127.0.0.1', 'tcp_port': 6379}).unlink()
    with pytest.raises(RuntimeError, match='TLS-aware'):
        redis_auth.connection_kwargs(host='127.0.0.1', port=6379)


def test_unmatched_tcp_does_not_read_password(runtime):
    publish(runtime).unlink()
    assert redis_auth.connection_kwargs(host='remote.example', port=6379) == {}


def test_two_local_unix_processes(runtime):
    entries = [{'username': 'sonic-trusted-writer', 'unix_socket': socket,
                'tcp_host': '127.0.0.1', 'tcp_port': 0}
               for socket in ('/var/run/redis/redis.sock', '/var/run/redis/redis_bmp.sock')]
    publish(runtime, entries)
    for entry in entries:
        assert redis_auth.connection_kwargs(unix_socket_path=entry['unix_socket'])['username'] == entry['username']


@pytest.mark.parametrize('target', ['local.json', 'local-system', 'local-system/writer.secret'])
def test_symlink_rejected(runtime, target):
    publish(runtime)
    path = runtime / target
    held = path.with_name(path.name + '.held')
    path.rename(held)
    path.symlink_to(held)
    with pytest.raises(OSError):
        redis_auth.connection_kwargs(unix_socket_path='/var/run/redis/redis.sock')


@pytest.mark.parametrize('target,mode', [('local.json', 0o666), ('local-system', 0o777),
                                      ('local-system/writer.secret', 0o644)])
def test_unsafe_permissions_rejected(runtime, target, mode):
    publish(runtime)
    (runtime / target).chmod(mode)
    with pytest.raises(RuntimeError, match='permissions'):
        redis_auth.connection_kwargs(unix_socket_path='/var/run/redis/redis.sock')


def test_missing_password_fails(runtime):
    publish(runtime).unlink()
    with pytest.raises(FileNotFoundError):
        redis_auth.connection_kwargs(unix_socket_path='/var/run/redis/redis.sock')


@pytest.mark.parametrize('content', ['', 'x' * 129])
def test_invalid_password_size(runtime, content):
    secret = publish(runtime)
    secret.chmod(0o600)
    secret.write_text(content)
    secret.chmod(0o400)
    with pytest.raises(RuntimeError, match='size'):
        redis_auth.connection_kwargs(unix_socket_path='/var/run/redis/redis.sock')


@pytest.mark.parametrize('entries', [[], {}, {'username': 'x'}, None, [None]])
def test_invalid_policy(runtime, entries):
    publish(runtime)
    (runtime / 'local.json').write_text(json.dumps(entries))
    with pytest.raises(RuntimeError, match='policy'):
        redis_auth.connection_kwargs(unix_socket_path='/var/run/redis/redis.sock')


@pytest.mark.parametrize('field,value', [('username', ''), ('username', '\0'),
                                       ('unix_socket', 'relative'), ('tcp_host', 'remote'),
                                       ('tcp_port', True), ('tcp_port', 65536)])
def test_invalid_entry(runtime, field, value):
    publish(runtime)
    path = runtime / 'local.json'
    entry = json.loads(path.read_text())
    entry[field] = value
    path.write_text(json.dumps(entry))
    with pytest.raises(RuntimeError, match='policy'):
        redis_auth.connection_kwargs(unix_socket_path='/var/run/redis/redis.sock')


def test_duplicate_socket_rejected(runtime):
    publish(runtime)
    entry = json.loads((runtime / 'local.json').read_text())
    (runtime / 'local.json').write_text(json.dumps([entry, entry]))
    with pytest.raises(RuntimeError, match='policy'):
        redis_auth.connection_kwargs(unix_socket_path=entry['unix_socket'])


def test_ordinary_reader_does_not_authenticate(runtime):
    publish(runtime)
    original_open = redis_auth._open

    def deny_secret(name, *args, **kwargs):
        if name == 'writer.secret':
            raise PermissionError()
        return original_open(name, *args, **kwargs)

    with patch.object(redis_auth, '_open', side_effect=deny_secret), patch.object(os, 'geteuid', return_value=65534):
        assert redis_auth.connection_kwargs(unix_socket_path='/var/run/redis/redis.sock') == {}


def test_real_redis_writer_reader_and_reconnect(monkeypatch, root_owned_auth_files):
    import redis

    with tempfile.TemporaryDirectory(prefix='sonic-python-auth-', dir='/var/tmp') as temporary:
        runtime = Path(temporary)
        runtime.chmod(0o755)
        root_owned_auth_files(runtime)
        monkeypatch.setattr(redis_auth, 'AUTH_DIRECTORY', str(runtime))
        socket_path = str(runtime / 'redis.sock')
        secret = publish(runtime, {'username': 'sonic-trusted-writer',
                                   'unix_socket': socket_path,
                                   'tcp_host': '127.0.0.1', 'tcp_port': 0})
        digest = hashlib.sha256(secret.read_bytes()).hexdigest()
        acl = runtime / 'users.acl'
        acl.write_text('user default on nopass ~* &* -@all +@read +@transaction '
                       '+ping +select +acl|whoami +subscribe\n'
                       'user sonic-trusted-writer on #' + digest + ' ~* &* +@all\n')
        server = subprocess.Popen(['redis-server', '--port', '0', '--unixsocket', socket_path,
                                   '--unixsocketperm', '666', '--save', '', '--appendonly', 'no',
                                   '--aclfile', str(acl)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            for _ in range(200):
                if Path(socket_path).exists():
                    break
                if server.poll() is not None:
                    pytest.fail('test Redis exited before creating its socket')
                time.sleep(0.01)
            options = redis_auth.connection_kwargs(unix_socket_path=socket_path)
            writer = redis.Redis(unix_socket_path=socket_path, **options)
            assert writer.acl_whoami() == 'sonic-trusted-writer'
            assert writer.set('auth-test-key', 'before-reconnect')
            writer.connection_pool.disconnect()
            with writer.pipeline() as pipeline:
                pipeline.set('auth-test-key', 'after-reconnect')
                pipeline.get('auth-test-key')
                assert pipeline.execute() == [True, b'after-reconnect']
            # Model a caller without credential access; Redis still enforces
            # the anonymous read and write-denial checks on a real connection.
            original_open = redis_auth._open

            def deny_secret(name, *args, **kwargs):
                if name == 'writer.secret':
                    raise PermissionError()
                return original_open(name, *args, **kwargs)

            with patch.object(redis_auth, '_open', side_effect=deny_secret), \
                    patch.object(redis_auth.os, 'geteuid', return_value=65534):
                reader_options = redis_auth.connection_kwargs(unix_socket_path=socket_path)
            assert reader_options == {}
            reader = redis.Redis(unix_socket_path=socket_path, **reader_options)
            assert reader.get('auth-test-key') == b'after-reconnect'
            with pytest.raises(redis.exceptions.NoPermissionError):
                reader.set('auth-test-key', 'forbidden')
            reader.close()
            assert writer.get('auth-test-key') == b'after-reconnect'
            writer.close()
        finally:
            server.terminate()
            server.wait(timeout=10)


def test_public_policies_do_not_open_password(runtime):
    publish(runtime).unlink()
    original_open = redis_auth._open

    def public_only(name, *args, **kwargs):
        assert name not in ('local-system', 'writer.secret')
        return original_open(name, *args, **kwargs)

    with patch.object(redis_auth, '_open', side_effect=public_only):
        entries = redis_auth.local_policies()
    assert len(entries) == 1
    assert entries[0]['unix_socket'] == '/var/run/redis/redis.sock'
    assert set(entries[0]) == {'username', 'unix_socket', 'tcp_host', 'tcp_port'}


def test_public_policies_absent_is_legacy(runtime, monkeypatch):
    assert redis_auth.local_policies() == []
    monkeypatch.setattr(redis_auth, 'AUTH_DIRECTORY', str(runtime / 'absent'))
    assert redis_auth.local_policies() == []


def test_public_policies_validate_permissions_and_shape(runtime):
    publish(runtime)
    policy = runtime / 'local.json'
    policy.chmod(0o666)
    with pytest.raises(RuntimeError, match='permissions'):
        redis_auth.local_policies()
    policy.chmod(0o644)
    policy.write_text('{"username":"incomplete"}')
    with pytest.raises(RuntimeError, match='policy'):
        redis_auth.local_policies()


def test_nonroot_owned_runtime_is_rejected(runtime, root_owned_auth_files):
    publish(runtime)
    root_owned_auth_files(runtime, owner=65534)
    with pytest.raises(RuntimeError, match='permissions'):
        redis_auth.connection_kwargs(unix_socket_path='/var/run/redis/redis.sock')


def test_fixture_owner_model_does_not_change_other_files(runtime):
    with tempfile.NamedTemporaryFile(dir='/var/tmp') as unrelated:
        assert redis_auth.os.fstat(unrelated.fileno()).st_uid == os.fstat(unrelated.fileno()).st_uid
    fd = os.open(runtime, os.O_RDONLY | os.O_DIRECTORY)
    try:
        assert os.fstat(fd).st_uid == os.geteuid()
    finally:
        os.close(fd)
