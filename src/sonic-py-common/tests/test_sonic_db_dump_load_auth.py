"""Exercise actual dump/load data and login with the image's redis-py version."""

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

from swsscommon.redis import connection_pool
from sonic_py_common.sonic_db_dump_load import sonic_db_dump_load


@pytest.fixture
def protected_redis(monkeypatch):
    native_root = os.environ.get('REDIS_AUTH_TEST_DIR')
    if not native_root or os.geteuid() != 0:
        pytest.skip('requires an isolated native auth test library and real root')
    policy_root = Path(native_root) / 'policy'
    policy_root.mkdir(mode=0o755)
    (policy_root / 'local-system').mkdir(mode=0o755)
    with tempfile.TemporaryDirectory(prefix='sonic-dump-native-', dir='/var/tmp') as temporary:
        root = Path(temporary)
        root.chmod(0o755)
        socket = str(root / 'redis.sock')
        password = os.urandom(24).hex()
        (policy_root / 'local-system/writer.secret').write_text(password)
        (policy_root / 'local-system/writer.secret').chmod(0o400)
        (policy_root / 'local.json').write_text(json.dumps({
            'username': 'sonic-trusted-writer', 'unix_socket': socket,
            'tcp_host': '127.0.0.1', 'tcp_port': 0}))
        acl = root / 'users.acl'
        acl.write_text('user default on nopass ~* &* -@all +@read +@transaction +select +ping +info\n'
                       'user sonic-trusted-writer on >' + password + ' ~* &* +@all\n')
        log = (root / 'redis.log').open('w')
        server = subprocess.Popen(['redis-server', '--port', '0', '--unixsocket', socket,
                                   '--unixsocketperm', '666', '--save', '', '--appendonly', 'no',
                                   '--aclfile', str(acl)], stdout=log, stderr=subprocess.STDOUT)
        writer = None
        try:
            writer = redis.Redis(connection_pool=connection_pool(unix_socket_path=socket))
            for attempt in range(200):
                try:
                    writer.ping()
                    break
                except (RuntimeError, redis.ConnectionError):
                    if server.poll() is not None:
                        pytest.fail('fixture Redis stopped: ' + (root / 'redis.log').read_text())
                    time.sleep(0.01)
            else:
                pytest.fail('fixture Redis did not start')
            with patch('swsscommon.swsscommon.SonicDBConfig.getDbSock', return_value=socket), \
                    patch('swsscommon.swsscommon.SonicDBConfig.getDbPort', return_value=0), \
                    patch('swsscommon.swsscommon.SonicDBConfig.getDbId', return_value=0), \
                    patch('sonic_py_common.multi_asic.is_multi_asic', return_value=False):
                yield policy_root, socket, writer
        finally:
            if writer is not None:
                writer.close()
            server.terminate()
            server.wait(timeout=10)
            log.close()
            (policy_root / 'local.json').unlink()
            (policy_root / 'local-system/writer.secret').unlink()
            (policy_root / 'local-system').rmdir()
            policy_root.rmdir()


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
    argv = ['sonic-db-dump', '-t', 'unix_socket', '-E', 'latin-1'] + (['-y'] if pretty else [])
    with patch('sys.argv', argv), patch('sys.stdout', output):
        sonic_db_dump_load()
    saved = json.loads(output.getvalue())
    assert saved['binary']['value'] == '\x00\xff'
    monkeypatch.setattr(redisdl, 'have_streaming_load', streaming)
    writer.flushdb()
    with patch('sys.argv', ['sonic-db-load', '-t', 'unix_socket', '-E', 'latin-1']), \
            patch('sys.stdin', io.StringIO(output.getvalue())):
        sonic_db_dump_load()
    assert writer.get('binary') == b'\x00\xff'
    assert writer.hgetall('hash') == {b'field': b'value'}
    assert writer.lrange('list', 0, -1) == [b'one', b'two']
    assert writer.smembers('set') == {b'one', b'two'}
    assert writer.zscore('sorted', 'one') == 1.5
    assert 0 < writer.ttl('expiring') <= 60


def test_real_large_load_crosses_pipeline_batch(protected_redis):
    _, _, writer = protected_redis
    document = {str(index): {'type': 'string', 'value': str(index)} for index in range(10001)}
    with patch('sys.argv', ['sonic-db-load', '-t', 'unix_socket']), patch('sys.stdin', io.StringIO(json.dumps(document))):
        sonic_db_dump_load()
    assert writer.dbsize() == 10001
    assert writer.get('10000') == b'10000'


@pytest.mark.parametrize('missing', [False, True])
def test_native_credential_failure_never_retries_anonymous(protected_redis, missing):
    policy_root, _, writer = protected_redis
    writer.set('original', 'value')
    secret = policy_root / 'local-system/writer.secret'
    original = secret.read_bytes()
    try:
        if missing:
            secret.unlink()
        else:
            secret.write_text('incorrect-password')
        with patch('sys.argv', ['sonic-db-load', '-t', 'unix_socket']), \
                patch('sys.stdin', io.StringIO('{"unexpected":{"type":"string","value":"bad"}}')):
            with pytest.raises(RuntimeError, match='authentication|credential'):
                sonic_db_dump_load()
        assert writer.get('original') == b'value'
        assert writer.get('unexpected') is None
    finally:
        secret.write_bytes(original)
        secret.chmod(0o400)


def test_library_pretty_dump_keeps_connection_options():
    mock_pool = object()
    with patch('redisdl.RedisWrapper') as client:
        client.return_value.keys.return_value = []
        redisdl.dump(io.StringIO(), pretty=True, unix_socket_path='/var/run/redis/redis.sock',
                     connection_pool=mock_pool)
    assert client.call_args.kwargs['connection_pool'] is mock_pool
    assert client.call_args.kwargs['unix_socket_path'] == '/var/run/redis/redis.sock'
