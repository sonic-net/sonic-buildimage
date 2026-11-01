"""Verify explicit dump/load endpoints and native-pool forwarding."""
import io
import sys
from unittest.mock import patch

import pytest

from sonic_py_common.sonic_db_dump_load import sonic_db_dump_load


@pytest.fixture
def metadata():
    with patch('swsscommon.swsscommon.SonicDBConfig') as config, \
            patch('sonic_py_common.multi_asic.is_multi_asic', return_value=False), \
            patch('swsscommon.redis.connection_pool') as pool:
        config.getDbSock.return_value = '/run/redis/example.sock'
        config.getDbHostname.return_value = '127.0.0.2'
        config.getDbPort.return_value = 6385
        config.getDbId.return_value = 6
        yield config, pool


def test_explicit_socket_preserves_namespace_and_database(metadata):
    config, pool = metadata
    with patch.object(sys, 'argv', ['sonic-db-dump', '-n', 'STATE_DB', '--netns', 'asic0', '-t', 'unix_socket']), \
            patch('redisdl.dump') as dump:
        sonic_db_dump_load()
    pool.assert_called_once_with(unix_socket_path='/run/redis/example.sock', db=6, encoding='utf-8', host=None, port=None)
    assert config.getDbSock.call_args.args[0] == 'STATE_DB'
    assert config.getDbSock.call_args.args[1].netns == 'asic0'
    config.getDbPort.assert_not_called()
    assert dump.call_args.kwargs['connection_pool'] is pool.return_value
    assert dump.call_args.kwargs['db'] == 6


@pytest.mark.parametrize('port', [0, 6385])
@pytest.mark.parametrize('name', ['STATE_DB', 'CHASSIS_STATE_DB'])
def test_default_keeps_explicit_tcp_contract(metadata, port, name):
    config, pool = metadata
    config.getDbPort.return_value = port
    with patch.object(sys, 'argv', ['sonic-db-dump', '-n', name]), patch('redisdl.dump'):
        sonic_db_dump_load()
    config.getDbSock.assert_not_called()
    pool.assert_called_once_with(host='127.0.0.2', port=port, db=6, unix_socket_path=None, encoding='utf-8')


def test_missing_explicit_socket_fails_before_creating_pool(metadata):
    config, pool = metadata
    config.getDbSock.return_value = ''
    with patch.object(sys, 'argv', ['sonic-db-dump', '-n', 'STATE_DB', '-t', 'unix_socket']), patch('redisdl.dump') as dump:
        with pytest.raises(ValueError, match='No Unix socket'):
            sonic_db_dump_load()
    pool.assert_not_called()
    dump.assert_not_called()


def test_pretty_dump_forwards_native_pool(metadata):
    _, pool = metadata
    output = io.StringIO()
    with patch.object(sys, 'argv', ['sonic-db-dump', '-n', 'STATE_DB', '-t', 'unix_socket', '-y']), \
            patch('sys.stdout', output), patch('redisdl.dumps', return_value='{}') as dumps, patch('redisdl.dump') as dump:
        sonic_db_dump_load()
    assert output.getvalue() == '{}'
    assert dumps.call_args.kwargs['connection_pool'] is pool.return_value
    dump.assert_not_called()


def test_load_keeps_encoding_on_the_native_pool(metadata):
    _, pool = metadata
    with patch.object(sys, 'argv', ['sonic-db-load', '-n', 'STATE_DB', '-t', 'unix_socket', '-E', 'latin-1']), \
            patch('sys.stdin', io.StringIO('{}')), patch('redisdl.load') as load:
        sonic_db_dump_load()
    assert pool.call_args.kwargs['encoding'] == 'latin-1'
    assert load.call_args.kwargs['connection_pool'] is pool.return_value


@pytest.mark.parametrize('action', ['dump', 'load'])
def test_omitted_database_selects_configured_appl_db(metadata, action):
    config, pool = metadata
    config.getDbId.return_value = 0
    with patch.object(sys, 'argv', ['sonic-db-' + action]), \
            patch('redisdl.' + action) as operation:
        sonic_db_dump_load()
    name, key = config.getDbId.call_args.args
    assert name == 'APPL_DB' and key.netns == ''
    assert pool.call_args.kwargs['db'] == 0
    assert operation.call_args.kwargs['db'] == 0


@pytest.mark.parametrize('namespace', ['asic3', ''])
def test_multi_asic_infers_namespace_before_endpoint_lookup(metadata, namespace):
    config, pool = metadata
    with patch('sonic_py_common.multi_asic.is_multi_asic', return_value=True), \
            patch('sonic_py_common.multi_asic.get_current_namespace', return_value=namespace), \
            patch.object(sys, 'argv', ['sonic-db-dump', '-t', 'unix_socket']), \
            patch('redisdl.dump'):
        sonic_db_dump_load()
    config.initializeGlobalConfig.assert_called_once_with()
    name, key = config.getDbSock.call_args.args
    assert name == 'APPL_DB' and key.netns == namespace
    assert pool.call_args.kwargs['unix_socket_path'] == '/run/redis/example.sock'


def test_explicit_tcp_never_looks_up_socket(metadata):
    config, pool = metadata
    with patch.object(sys, 'argv', ['sonic-db-dump', '-t', 'tcp', '-n', 'CHASSIS_APP_DB']), \
            patch('redisdl.dump'):
        sonic_db_dump_load()
    config.getDbSock.assert_not_called()
    assert pool.call_args.kwargs['host'] == '127.0.0.2'
    assert pool.call_args.kwargs['port'] == 6385
