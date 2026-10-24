"""Exercise actual dump/load data and login with the image's redis-py version."""

import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
from unittest.mock import patch

import pytest
import redis
import redisdl

from sonic_py_common import redis_auth
from sonic_py_common.sonic_db_dump_load import sonic_db_dump_load


@pytest.fixture
def protected_redis(monkeypatch, root_owned_auth_files):
    with tempfile.TemporaryDirectory(prefix='sonic-dump-auth-', dir='/var/tmp') as temporary:
        root = Path(temporary)
        root.chmod(0o755)
        root_owned_auth_files(root)
        socket = str(root / 'redis.sock')
        password = os.urandom(24).hex()
        (root / 'local-system').mkdir(mode=0o755)
        secret = root / 'local-system/writer.secret'
        secret.write_text(password)
        secret.chmod(0o400)
        (root / 'local.json').write_text(json.dumps({
            'username': 'sonic-trusted-writer', 'unix_socket': socket,
            'tcp_host': '127.0.0.1', 'tcp_port': 0}))
        acl = root / 'users.acl'
        acl.write_text('user default on nopass ~* &* -@all +@read +@transaction '
                       '+select +ping +info +client|setinfo\n'
                       'user sonic-trusted-writer on #' + hashlib.sha256(password.encode()).hexdigest()
                       + ' ~* &* +@all\n')
        server = subprocess.Popen(['redis-server', '--port', '0', '--unixsocket', socket,
                                   '--unixsocketperm', '666', '--save', '', '--appendonly', 'no',
                                   '--aclfile', str(acl)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            for _ in range(200):
                if Path(socket).exists():
                    break
                if server.poll() is not None:
                    pytest.fail('test Redis stopped before startup')
                time.sleep(0.01)
            monkeypatch.setattr(redis_auth, 'AUTH_DIRECTORY', str(root))
            with patch('swsscommon.swsscommon.SonicDBConfig.getDbSock', return_value=socket), \
                    patch('swsscommon.swsscommon.SonicDBConfig.getDbPort', return_value=0), \
                    patch('swsscommon.swsscommon.SonicDBConfig.getDbId', return_value=0), \
                    patch('swsscommon.swsscommon.SonicDBConfig.getDbHostname', return_value='127.0.0.1'), \
                    patch('sonic_py_common.multi_asic.is_multi_asic', return_value=False):
                writer = redis.Redis(unix_socket_path=socket, **redis_auth.connection_kwargs(unix_socket_path=socket))
                yield root, socket, writer
                writer.close()
        finally:
            server.terminate()
            server.wait(timeout=10)


@pytest.mark.parametrize('pretty', [False, True])
@pytest.mark.parametrize('streaming', [False, True])
def test_real_named_writer_round_trip(protected_redis, monkeypatch, pretty, streaming):
    _, _, writer = protected_redis
    writer.set('binary', b'\x00\xff')
    writer.hset('hash', mapping={'field': 'value'})
    writer.rpush('list', 'one', 'two')
    writer.sadd('set', 'one', 'two')
    writer.zadd('sorted', {'one': 1.5})
    writer.set('expiring', 'value', ex=60)
    output = io.StringIO()
    argv = ['sonic-db-dump', '-E', 'latin-1'] + (['-y'] if pretty else [])
    with patch('sys.argv', argv), patch('sys.stdout', output):
        sonic_db_dump_load()
    saved = json.loads(output.getvalue())
    assert saved['binary']['value'] == '\x00\xff'
    monkeypatch.setattr(redisdl, 'have_streaming_load', streaming)
    writer.flushdb()
    with patch('sys.argv', ['sonic-db-load', '-E', 'latin-1']), \
            patch('sys.stdin', io.StringIO(output.getvalue())):
        sonic_db_dump_load()
    assert writer.get('binary') == b'\x00\xff'
    assert writer.hgetall('hash') == {b'field': b'value'}
    assert writer.lrange('list', 0, -1) == [b'one', b'two']
    assert writer.smembers('set') == {b'one', b'two'}
    assert writer.zscore('sorted', 'one') == 1.5
    assert 0 < writer.ttl('expiring') <= 60


def test_real_ordinary_dump_and_load_denial(protected_redis):
    _, socket, writer = protected_redis
    writer.set('ordinary-readable', 'value')
    original_open = redis_auth._open

    def deny_secret(name, *args, **kwargs):
        if name == 'writer.secret':
            raise PermissionError()
        return original_open(name, *args, **kwargs)

    # Model missing credential access; real Redis enforces the reader role.
    with patch.object(redis_auth, '_open', side_effect=deny_secret), \
            patch.object(redis_auth.os, 'geteuid', return_value=65534):
        output = io.StringIO()
        with patch('sys.argv', ['sonic-db-dump']), patch('sys.stdout', output):
            sonic_db_dump_load()
        assert json.loads(output.getvalue())['ordinary-readable']['value'] == 'value'
        with patch('sys.argv', ['sonic-db-load']), \
                patch('sys.stdin', io.StringIO(output.getvalue())):
            with pytest.raises(redis.exceptions.ResponseError):
                sonic_db_dump_load()
    assert writer.get('ordinary-readable') == b'value'


def test_real_large_load_crosses_pipeline_batch(protected_redis):
    _, _, writer = protected_redis
    document = {str(index): {'type': 'string', 'value': str(index)} for index in range(10001)}
    with patch('sys.argv', ['sonic-db-load']), patch('sys.stdin', io.StringIO(json.dumps(document))):
        sonic_db_dump_load()
    assert writer.dbsize() == 10001
    assert writer.get('10000') == b'10000'


def test_library_pretty_dump_keeps_connection_options():
    with patch('redisdl.RedisWrapper') as client:
        client.return_value.keys.return_value = []
        redisdl.dump(io.StringIO(), pretty=True, unix_socket_path='/var/run/redis/redis.sock',
                     connection_kwargs={'username': 'test-writer'})
    assert client.call_args.kwargs['username'] == 'test-writer'
    assert client.call_args.kwargs['unix_socket_path'] == '/var/run/redis/redis.sock'
