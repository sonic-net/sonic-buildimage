"""Actual filesystem ownership and cross-UID checks. Run explicitly as root."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

import pytest

from sonic_py_common import redis_auth
from .test_redis_auth import publish


@pytest.fixture(autouse=True)
def require_real_root():
    if sys.flags.optimize:
        pytest.fail('Run acceptance with PYTHONOPTIMIZE=0')
    if os.geteuid() != 0:
        pytest.fail('Run this acceptance file explicitly in an isolated root container')


def test_real_redis_writer_reader_and_reconnect(monkeypatch):
    import redis

    with tempfile.TemporaryDirectory(prefix='sonic-python-auth-', dir='/var/tmp') as temporary:
        runtime = Path(temporary)
        runtime.chmod(0o755)
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
            code = '''
import sys
import redis
from sonic_py_common import redis_auth
redis_auth.AUTH_DIRECTORY = sys.argv[1]
options = redis_auth.connection_kwargs(unix_socket_path=sys.argv[2])
assert options == {}
reader = redis.Redis(unix_socket_path=sys.argv[2], **options)
assert reader.get('auth-test-key') == b'after-reconnect'
try:
    reader.set('auth-test-key', 'forbidden')
except redis.exceptions.NoPermissionError:
    pass
else:
    raise AssertionError('ordinary write succeeded')
reader.close()
'''
            completed = subprocess.run([sys.executable, '-c', code, str(runtime), socket_path],
                                       user=65534, group=65534, extra_groups=[],
                                       capture_output=True, text=True)
            assert completed.returncode == 0, completed.stderr
            assert writer.get('auth-test-key') == b'after-reconnect'
            # A service granted group read access uses the same login path.
            os.chown(secret, 0, 12345)
            secret.chmod(0o440)
            approved_code = '''
import sys
import redis
from sonic_py_common import redis_auth
redis_auth.AUTH_DIRECTORY = sys.argv[1]
options = redis_auth.connection_kwargs(unix_socket_path=sys.argv[2])
assert options['username'] == 'sonic-trusted-writer'
writer = redis.Redis(unix_socket_path=sys.argv[2], **options)
assert writer.set('approved-service-key', 'value')
writer.close()
'''
            approved = subprocess.run([sys.executable, '-c', approved_code, str(runtime), socket_path],
                                      user=65533, group=12345, extra_groups=[],
                                      capture_output=True, text=True)
            assert approved.returncode == 0, approved.stderr
            assert writer.get('approved-service-key') == b'value'
            writer.close()
        finally:
            server.terminate()
            server.wait(timeout=10)


@pytest.mark.parametrize('target', ['.', 'local.json', 'local-system', 'local-system/writer.secret'])
def test_real_nonroot_owner_is_rejected(tmp_path, monkeypatch, target):
    runtime = tmp_path
    runtime.chmod(0o755)
    monkeypatch.setattr(redis_auth, 'AUTH_DIRECTORY', str(runtime))
    publish(runtime)
    os.chown(runtime / target, 65534, 65534)
    with pytest.raises(RuntimeError, match='permissions'):
        redis_auth.connection_kwargs(unix_socket_path='/var/run/redis/redis.sock')
