import io
import sys
from sonic_py_common.sonic_db_dump_load import sonic_db_dump_load
from swsscommon.swsscommon import SonicDBKey
from unittest.mock import patch
import pytest

@patch("redisdl.dump")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname", return_value="127.0.0.1")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbPort", return_value=6379)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=0)
@patch("sys.argv", ["sonic-db-dump", "-n", "APPL_DB"])
def test_sonic_db_dump(mock_getDbHostname, mock_getDbPort, mock_getDbId, mock_dump):
    sonic_db_dump_load()
    mock_dump.assert_called_once_with(sys.stdout, **
        {
            "host": "127.0.0.1",
            "port": 6379,
            "unix_socket_path": None,
            "db": 0,
            "encoding": "utf-8",
        }
    )
    mock_getDbHostname.assert_called_once_with("APPL_DB", SonicDBKey())


@patch("redisdl.dump")
@patch("sonic_py_common.multi_asic.is_multi_asic", return_value=True)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname", return_value="127.0.0.1")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbPort", return_value=6379)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=0)
@patch("sys.argv", ["sonic-db-dump", "--netns", "asic0", "-n", "APPL_DB"])
def test_sonic_db_dump_multi_asic(mock_getDbHostname, mock_getDbPort, mock_getDbId, mock_is_multi_asic, mock_dump):
    sonic_db_dump_load()
    mock_dump.assert_called_once_with(sys.stdout, **
        {
            "host": "127.0.0.1",
            "port": 6379,
            "unix_socket_path": None,
            "db": 0,
            "encoding": "utf-8",
        }
    )
    mock_getDbHostname.assert_called_once_with("APPL_DB", SonicDBKey("asic0"))


@patch("redisdl.load")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname", return_value="127.0.0.1")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbPort", return_value=6379)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=0)
@patch("sys.argv", ["sonic-db-load", "-n", "APPL_DB"])
def test_sonic_db_load(mock_getDbHostname, mock_getDbPort, mock_getDbId, mock_load):
    sonic_db_dump_load()
    mock_load.assert_called_once_with(sys.stdin, **
        {
            "host": "127.0.0.1",
            "port": 6379,
            "unix_socket_path": None,
            "db": 0,
            "encoding": "utf-8",
        }
    )
    args, kwargs = mock_getDbHostname.call_args
    assert args[0] == "APPL_DB"
    assert args[1].netns == ""


@patch("redisdl.load")
@patch("sonic_py_common.multi_asic.is_multi_asic", return_value=True)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname", return_value="127.0.0.1")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbPort", return_value=6379)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=0)
@patch("sys.argv", ["sonic-db-load", "--netns", "asic0", "-n", "APPL_DB"])
def test_sonic_db_load_multi_asic(mock_getDbHostname, mock_getDbPort, mock_getDbId, mock_is_multi_asic, mock_load):
    sonic_db_dump_load()
    mock_load.assert_called_once_with(sys.stdin, **
        {
            "host": "127.0.0.1",
            "port": 6379,
            "unix_socket_path": None,
            "db": 0,
            "encoding": "utf-8",
        }
    )
    mock_getDbHostname.assert_called_once_with("APPL_DB", SonicDBKey("asic0"))


@pytest.fixture
def endpoint():
    with patch('swsscommon.swsscommon.SonicDBConfig') as config, \
            patch('sonic_py_common.multi_asic.is_multi_asic', return_value=False), \
            patch('swsscommon.redis.connection_pool') as pool:
        config.getDbId.return_value = 6
        config.getDbSock.return_value = '/run/redis/state.sock'
        config.getDbHostname.return_value = '192.0.2.10'
        config.getDbPort.return_value = 6381
        yield config, pool


@pytest.mark.parametrize('command', ['dump', 'load'])
@pytest.mark.parametrize('port', [0, 6381])
def test_tcp_retains_endpoint_without_native_pool(endpoint, command, port):
    config, pool = endpoint
    config.getDbPort.return_value = port
    with patch.object(sys, 'argv', ['sonic-db-' + command, '-n', 'CHASSIS_STATE_DB']), \
            patch('redisdl.' + command) as operation:
        sonic_db_dump_load()
    assert operation.call_args.kwargs == {
        'host': '192.0.2.10', 'port': port, 'db': 6,
        'unix_socket_path': None, 'encoding': 'utf-8'}
    pool.assert_not_called()
    config.getDbSock.assert_not_called()


@pytest.mark.parametrize('pretty', [False, True])
def test_unix_dump_keeps_reader_and_namespace(endpoint, pretty):
    config, pool = endpoint
    argv = ['sonic-db-dump', '-n', 'STATE_DB', '--netns', 'asic0', '-t', 'unix_socket']
    with patch.object(sys, 'argv', argv + (['-y'] if pretty else [])), patch('redisdl.dump') as dump:
        sonic_db_dump_load()
    assert dump.call_args.kwargs['unix_socket_path'] == '/run/redis/state.sock'
    assert dump.call_args.kwargs.get('pretty', False) == pretty
    assert config.getDbSock.call_args.args[1].netns == 'asic0'
    config.initializeGlobalConfig.assert_called_once_with()
    config.getDbPort.assert_not_called()
    pool.assert_not_called()


def test_unix_load_authenticates_without_forwarding_loader_options(endpoint):
    _, pool = endpoint
    argv = ['sonic-db-load', '-t', 'unix_socket', '-E', 'latin-1', '-e', '-A', '-B', 'ijson']
    with patch.object(sys, 'argv', argv), patch('sys.stdin', io.StringIO('{}')), patch('redisdl.load') as load:
        sonic_db_dump_load()
    pool.assert_called_once_with(
        unix_socket_path='/run/redis/state.sock', db=6,
        encoding='latin-1', password=None, authenticate=True)
    options = load.call_args.kwargs
    assert options['connection_pool'] is pool.return_value
    assert options['empty'] and options['use_expireat']
    assert options['streaming_backend'] == 'ijson'


def test_missing_unix_socket_fails_before_connection(endpoint):
    config, pool = endpoint
    config.getDbSock.return_value = ''
    with patch.object(sys, 'argv', ['sonic-db-load', '-t', 'unix_socket']), patch('redisdl.load') as load:
        with pytest.raises(ValueError, match='No Unix socket'):
            sonic_db_dump_load()
    pool.assert_not_called()
    load.assert_not_called()


@pytest.mark.parametrize('command', ['dump', 'load'])
def test_bare_tcp_uses_redis_client_defaults(endpoint, command):
    config, pool = endpoint
    with patch.object(sys, 'argv', ['sonic-db-' + command]), \
            patch('redisdl.' + command) as operation:
        sonic_db_dump_load()
    assert operation.call_args.kwargs == {'encoding': 'utf-8'}
    config.getDbId.assert_not_called()
    config.getDbHostname.assert_not_called()
    config.getDbPort.assert_not_called()
    pool.assert_not_called()


@pytest.mark.parametrize('command', ['dump', 'load'])
def test_unix_defaults_to_appl_db_in_empty_namespace(endpoint, command):
    config, _ = endpoint
    with patch.object(sys, 'argv', ['sonic-db-' + command, '-t', 'unix_socket']), \
            patch('sonic_py_common.multi_asic.is_multi_asic', return_value=True), \
            patch('sonic_py_common.multi_asic.get_current_namespace') as current, \
            patch('redisdl.' + command):
        sonic_db_dump_load()
    name, key = config.getDbId.call_args.args
    assert name == 'APPL_DB' and key.netns == ''
    current.assert_not_called()
