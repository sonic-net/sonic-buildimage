"""Exercise the DPU management reader against a real socket-only Redis fixture."""
import json
from pathlib import Path
import shutil
import subprocess
import time

import pytest

ROOT = Path(__file__).resolve().parents[3]


@pytest.fixture
def config_reader(tmp_path):
    if not shutil.which('redis-server') or not shutil.which('redis-cli'):
        pytest.skip('requires Redis fixture binaries')
    socket = tmp_path / 'socket with spaces.sock'
    acl = tmp_path / 'users.acl'
    acl.write_text('user default on nopass ~* -@all +@read +select +ping\n'
                   'user seed on >dpu-fixture-only ~* +@all\n')
    server = subprocess.Popen(['redis-server', '--port', '0', '--unixsocket', str(socket),
        '--save', '', '--appendonly', 'no', '--aclfile', str(acl)],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        for _ in range(100):
            if socket.exists(): break
            if server.poll() is not None: raise RuntimeError('Redis fixture failed')
            time.sleep(0.01)
        else: raise RuntimeError('Redis fixture socket missing')
        import os
        for key, field, value in [('DPUS|dpu0', 'midplane_interface', 'Ethernet0'),
                                  ('DPUS|dpu1', 'midplane_interface', 'Ethernet4'),
                                  ('DHCP_SERVER_IPV4_PORT|bridge-midplane|Ethernet0', 'ips@', '169.254.1.1')]:
            subprocess.run(['redis-cli', '-s', str(socket), '--user', 'seed', '-n', '44',
                'HSET', key, field, value], env=dict(os.environ, REDISCLI_AUTH='dpu-fixture-only'),
                capture_output=True, check=True)
        metadata = tmp_path / 'database_config.json'
        metadata.write_text(json.dumps({'DATABASES': {'CONFIG_DB': {'id': 44, 'instance': 'owner'}},
            'INSTANCES': {'owner': {'port': 0, 'unix_socket_path': str(socket)}}}))
        source = (ROOT / 'files/scripts/sonic-dpu-mgmt-traffic.sh').read_text()
        start = source.index('config_db_read() {')
        end = source.index('\n}\n', start) + 3
        helper = source[start:end].replace('/var/run/redis/sonic-db/database_config.json', str(metadata))
        def run(command):
            return subprocess.run(['bash', '-c', helper + '\n' + command],
                                  capture_output=True, text=True, check=True).stdout
        yield source, run
    finally:
        server.terminate()
        server.wait(timeout=5)


def test_keys_keep_existing_dpu_shell_parser(config_reader):
    source, run = config_reader
    start = source.index('    while IFS= read -r line; do', source.index('inbound_validation(){'))
    end = source.index('    len1=', start)
    command = 'dpu_l=()\n' + source[start:end] + 'printf "%s\n" "${dpu_l[@]}"'
    assert sorted(run(command).splitlines()) == ['dpu0', 'dpu1']


def test_hash_reads_keep_scalar_output(config_reader):
    _, run = config_reader
    assert run('config_db_read hget "DPUS|dpu0" midplane_interface') == 'Ethernet0\n'
    assert run('config_db_read hget "DHCP_SERVER_IPV4_PORT|bridge-midplane|Ethernet0" "ips@"') == '169.254.1.1\n'
