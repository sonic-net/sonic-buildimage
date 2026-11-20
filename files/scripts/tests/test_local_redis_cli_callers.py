"""Check the native scan endpoint used by local configuration cleanup."""

import json
from pathlib import Path
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[3]


@pytest.mark.parametrize('port', [0, 6380])
@pytest.mark.parametrize('socket_path', ['/run/redis/redis.sock', '/run/redis/socket with spaces'])
def test_micas_scan_keeps_configured_endpoint_and_database(tmp_path, port, socket_path):
    metadata = tmp_path / 'database_config.json'
    metadata.write_text(json.dumps({
        'DATABASES': {'CONFIG_DB': {'id': 44, 'instance': 'custom'}},
        'INSTANCES': {'custom': {'hostname': '127.0.0.2', 'port': port, 'unix_socket_path': socket_path}}}))
    path = ROOT / 'platform/broadcom/sonic-platform-modules-micas/common/script/config-hwsku-cpo.sh'
    source = path.read_text()
    start = source.index('    local redis_settings\n')
    end = source.index('    cursor=0\n', start)
    setup = source[start:end].replace('/var/run/redis/sonic-db/database_config.json', str(metadata))
    program = "make_endpoint() {\n" + setup + "printf '%s\\0' \"${native_cli[@]}\";\n}\nmake_endpoint"
    result = subprocess.run(['bash', '-c', program], capture_output=True, check=True)
    arguments = result.stdout.decode().split('\0')[:-1]
    expected = ['redis-cli', '-n', '44']
    expected += ['-s', socket_path] if port == 0 else ['-h', '127.0.0.2', '-p', '6380']
    assert arguments == expected
    assert 'sonic-db-cli -s --authenticate CONFIG_DB DEL $keys' in source
    assert 'sonic-db-cli -s --authenticate CONFIG_DB SET "CONFIG_DB_INITIALIZED" "1"' in source
