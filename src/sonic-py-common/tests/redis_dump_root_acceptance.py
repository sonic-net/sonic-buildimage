"""Actual dump/load UID checks. Run explicitly as root in an isolated container."""
import io
import json
import os
from pathlib import Path
import socket as socket_module
import subprocess
import sys
import tempfile
import time
from unittest.mock import patch

import pytest
import redis

from .test_sonic_db_dump_load_auth import protected_redis


@pytest.fixture(autouse=True)
def require_real_root():
    if sys.flags.optimize:
        pytest.fail('Run acceptance with PYTHONOPTIMIZE=0')
    if os.geteuid() != 0:
        pytest.fail('Run this acceptance file explicitly in an isolated root container')


def test_real_ordinary_dump_and_load_denial(protected_redis):
    root, socket, writer = protected_redis
    writer.set('ordinary-readable', 'value')
    code = '''
import io, json, sys
from unittest.mock import patch
import redis
from sonic_py_common import redis_auth
from sonic_py_common.sonic_db_dump_load import sonic_db_dump_load
redis_auth.AUTH_DIRECTORY = sys.argv[1]
socket = sys.argv[2]
with patch('swsscommon.swsscommon.SonicDBConfig.getDbSock', return_value=socket), \\
     patch('swsscommon.swsscommon.SonicDBConfig.getDbPort', return_value=0), \\
     patch('swsscommon.swsscommon.SonicDBConfig.getDbId', return_value=0), \\
     patch('swsscommon.swsscommon.SonicDBConfig.getDbHostname', return_value='127.0.0.1'), \\
     patch('sonic_py_common.multi_asic.is_multi_asic', return_value=False):
    output = io.StringIO()
    with patch('sys.argv', ['sonic-db-dump']), patch('sys.stdout', output):
        sonic_db_dump_load()
    assert json.loads(output.getvalue())['ordinary-readable']['value'] == 'value'
    with patch('sys.argv', ['sonic-db-load']), patch('sys.stdin', io.StringIO(output.getvalue())):
        try:
            sonic_db_dump_load()
        except redis.exceptions.ResponseError:
            pass
        else:
            raise AssertionError('ordinary load wrote to Redis')
'''
    completed = subprocess.run([sys.executable, '-c', code, str(root), socket],
                               user=65534, group=65534, extra_groups=[], capture_output=True, text=True)
    assert completed.returncode == 0, completed.stderr
    assert writer.get('ordinary-readable') == b'value'



@pytest.mark.parametrize('uid,transport,metadata_port,socket_mode,works', [
    (0, None, 'tcp', '760', True),
    (65534, None, 'tcp', '760', True),
    (0, 'unix_socket', 'tcp', '760', True),
    (65534, 'unix_socket', 'tcp', '760', False),
    (0, 'tcp', 'tcp', '760', True),
    (65534, 'tcp', 'tcp', '760', True),
    (0, None, '0', '666', True),
    (65534, None, '0', '666', True),
    (0, 'tcp', '0', '666', False),
    (65534, 'tcp', '0', '666', False),
    (65534, None, '0', '760', False),
])
def test_real_legacy_transport_permissions(uid, transport, metadata_port, socket_mode, works):
    with tempfile.TemporaryDirectory(prefix='sonic-dump-legacy-', dir='/var/tmp') as temporary:
        root = Path(temporary)
        root.chmod(0o755)
        path = str(root / 'redis.sock')
        with socket_module.socket() as reservation:
            reservation.bind(('127.0.0.1', 0))
            port = reservation.getsockname()[1]
        server = subprocess.Popen([
            'redis-server', '--bind', '127.0.0.1', '--port', str(port),
            '--unixsocket', path, '--unixsocketperm', socket_mode,
            '--save', '', '--appendonly', 'no',
        ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        writer = redis.Redis(host='127.0.0.1', port=port)
        try:
            for _ in range(200):
                if Path(path).exists():
                    break
                if server.poll() is not None:
                    pytest.fail('legacy test Redis exited before startup')
                time.sleep(0.01)
            writer.set('legacy-readable', 'value')
            code = '''
import io, json, sys
from unittest.mock import patch
from sonic_py_common import redis_auth
from sonic_py_common.sonic_db_dump_load import sonic_db_dump_load
redis_auth.AUTH_DIRECTORY = sys.argv[1] + '/absent-auth'
argv = ['sonic-db-dump', '-n', 'APPL_DB'] + (['-t', sys.argv[4]] if sys.argv[4] else [])
with patch('swsscommon.swsscommon.SonicDBConfig.getDbSock', return_value=sys.argv[1] + '/redis.sock'), \\
     patch('swsscommon.swsscommon.SonicDBConfig.getDbPort', return_value=int(sys.argv[2])), \\
     patch('swsscommon.swsscommon.SonicDBConfig.getDbId', return_value=0), \\
     patch('swsscommon.swsscommon.SonicDBConfig.getDbHostname', return_value='127.0.0.1'), \\
     patch('sonic_py_common.multi_asic.is_multi_asic', return_value=False), \\
     patch('sys.argv', argv), patch('sys.stdout', new_callable=io.StringIO) as output:
    sonic_db_dump_load()
    if json.loads(output.getvalue())['legacy-readable']['value'] != 'value':
        raise RuntimeError('legacy dump returned incorrect data')
'''
            configured_port = port if metadata_port == 'tcp' else 0
            completed = subprocess.run([
                sys.executable, '-c', code, str(root), str(configured_port),
                socket_mode, transport or '',
            ], user=uid, group=uid, extra_groups=[], capture_output=True, text=True, timeout=15)
            assert (completed.returncode == 0) == works, completed.stderr
        finally:
            writer.close()
            server.terminate()
            server.wait(timeout=10)
