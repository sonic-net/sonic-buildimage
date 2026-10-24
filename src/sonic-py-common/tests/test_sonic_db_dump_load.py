from io import StringIO
import sys
from sonic_py_common.sonic_db_dump_load import sonic_db_dump_load
from swsscommon.swsscommon import SonicDBKey
from unittest.mock import patch

@patch("redisdl.dump")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbSock", return_value="/var/run/redis/redis.sock")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname", return_value="127.0.0.1")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbPort", return_value=6379)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=0)
@patch("sys.argv", ["sonic-db-dump", "-n", "APPL_DB"])
def test_sonic_db_dump(
        mock_getDbId, mock_getDbPort, mock_getDbHostname, mock_getDbSock,
        mock_dump):
    sonic_db_dump_load()
    mock_dump.assert_called_once_with(sys.stdout, **
        {
            "host": None,
            "port": None,
            "unix_socket_path": "/var/run/redis/redis.sock",
            "db": 0,
            "encoding": "utf-8",
        }
    )
    mock_getDbHostname.assert_called_once_with("APPL_DB", SonicDBKey())
    mock_getDbSock.assert_called_once_with("APPL_DB", SonicDBKey())
    mock_getDbPort.assert_called_once()


@patch("redisdl.RedisWrapper")
@patch("sonic_py_common.multi_asic.is_multi_asic", return_value=False)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbSock", return_value="/var/run/redis/redis.sock")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname", return_value="127.0.0.1")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbPort", return_value=6379)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=2)
@patch("sys.argv", ["sonic-db-dump", "-n", "COUNTERS_DB", "-y"])
def test_sonic_db_dump_pretty_uses_unix_socket(
        mock_getDbId, mock_getDbPort, mock_getDbHostname, mock_getDbSock,
        mock_is_multi_asic, mock_redis):
    mock_redis.return_value.keys.return_value = []

    with patch("sys.stdout", new_callable=StringIO) as output:
        sonic_db_dump_load()

    assert output.getvalue() == "{}"
    mock_redis.assert_called_once_with(
        unix_socket_path="/var/run/redis/redis.sock",
        password=None,
        db=2,
        encoding="utf-8",
    )
    mock_getDbHostname.assert_called_once_with("COUNTERS_DB", SonicDBKey())
    mock_getDbSock.assert_called_once_with("COUNTERS_DB", SonicDBKey())
    mock_getDbPort.assert_called_once()


@patch("redisdl.dump")
@patch("sonic_py_common.multi_asic.is_multi_asic", return_value=True)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbSock", return_value="/var/run/redis0/redis.sock")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname", return_value="127.0.0.1")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbPort", return_value=6379)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=0)
@patch("sys.argv", ["sonic-db-dump", "--netns", "asic0", "-n", "APPL_DB"])
def test_sonic_db_dump_multi_asic(
        mock_getDbId, mock_getDbPort, mock_getDbHostname, mock_getDbSock,
        mock_is_multi_asic, mock_dump):
    sonic_db_dump_load()
    mock_dump.assert_called_once_with(sys.stdout, **
        {
            "host": None,
            "port": None,
            "unix_socket_path": "/var/run/redis0/redis.sock",
            "db": 0,
            "encoding": "utf-8",
        }
    )
    mock_getDbHostname.assert_called_once_with("APPL_DB", SonicDBKey("asic0"))
    mock_getDbSock.assert_called_once_with("APPL_DB", SonicDBKey("asic0"))
    mock_getDbPort.assert_called_once()


@patch("redisdl.load")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbSock", return_value="/var/run/redis/redis.sock")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname", return_value="127.0.0.1")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbPort", return_value=6379)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=0)
@patch("sys.argv", ["sonic-db-load", "-n", "APPL_DB"])
def test_sonic_db_load(
        mock_getDbId, mock_getDbPort, mock_getDbHostname, mock_getDbSock,
        mock_load):
    sonic_db_dump_load()
    mock_load.assert_called_once_with(sys.stdin, **
        {
            "host": None,
            "port": None,
            "unix_socket_path": "/var/run/redis/redis.sock",
            "db": 0,
            "encoding": "utf-8",
        }
    )
    args, kwargs = mock_getDbHostname.call_args
    assert args[0] == "APPL_DB"
    assert args[1].netns == ""
    mock_getDbSock.assert_called_once_with("APPL_DB", SonicDBKey())
    mock_getDbPort.assert_called_once()


@patch("redisdl.load")
@patch("sonic_py_common.multi_asic.is_multi_asic", return_value=True)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbSock", return_value="/var/run/redis0/redis.sock")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname", return_value="127.0.0.1")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbPort", return_value=6379)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=0)
@patch("sys.argv", ["sonic-db-load", "--netns", "asic0", "-n", "APPL_DB"])
def test_sonic_db_load_multi_asic(
        mock_getDbId, mock_getDbPort, mock_getDbHostname, mock_getDbSock,
        mock_is_multi_asic, mock_load):
    sonic_db_dump_load()
    mock_load.assert_called_once_with(sys.stdin, **
        {
            "host": None,
            "port": None,
            "unix_socket_path": "/var/run/redis0/redis.sock",
            "db": 0,
            "encoding": "utf-8",
        }
    )
    mock_getDbHostname.assert_called_once_with("APPL_DB", SonicDBKey("asic0"))
    mock_getDbSock.assert_called_once_with("APPL_DB", SonicDBKey("asic0"))
    mock_getDbPort.assert_called_once()


import pytest

LOCAL_SOCKET = "/var/run/redis/redis.sock"


def run_command(argv, action="dump", multi_asic=False):
    action_path = "redisdl.{}".format(action)
    with patch("sys.argv", argv), \
            patch("sonic_py_common.multi_asic.is_multi_asic", return_value=multi_asic), \
            patch(action_path) as mock_action:
        sonic_db_dump_load()
    return mock_action


@patch("swsscommon.swsscommon.SonicDBConfig.getDbSock", return_value=LOCAL_SOCKET)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname", return_value="127.0.0.1")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=0)
def test_dump_without_dbname_preserves_db_zero_and_uses_local_socket(
        mock_get_db_id, mock_get_hostname, mock_get_socket):
    mock_dump = run_command(["sonic-db-dump"])

    mock_dump.assert_called_once_with(sys.stdout, **{
        "host": None,
        "port": None,
        "unix_socket_path": LOCAL_SOCKET,
        "db": 0,
        "encoding": "utf-8",
    })
    mock_get_db_id.assert_called_once_with("APPL_DB", SonicDBKey())
    mock_get_hostname.assert_called_once_with("APPL_DB", SonicDBKey())
    mock_get_socket.assert_called_once_with("APPL_DB", SonicDBKey())


@patch("swsscommon.swsscommon.SonicDBConfig.getDbSock", return_value=LOCAL_SOCKET)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname", return_value="127.0.0.1")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=0)
def test_load_without_dbname_preserves_db_zero_and_uses_local_socket(
        mock_get_db_id, mock_get_hostname, mock_get_socket):
    mock_load = run_command(["sonic-db-load"], action="load")

    mock_load.assert_called_once_with(sys.stdin, **{
        "host": None,
        "port": None,
        "unix_socket_path": LOCAL_SOCKET,
        "db": 0,
        "encoding": "utf-8",
    })
    mock_get_db_id.assert_called_once_with("APPL_DB", SonicDBKey())


@patch("swsscommon.swsscommon.SonicDBConfig.initializeGlobalConfig")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbSock", return_value="/var/run/redis0/redis.sock")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname", return_value="127.0.0.1")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=1)
def test_auto_uses_namespace_socket_and_configured_db_id(
        mock_get_db_id, mock_get_hostname, mock_get_socket, mock_initialize):
    mock_dump = run_command(
        ["sonic-db-dump", "--netns", "asic0", "-n", "ASIC_DB"],
        multi_asic=True,
    )

    mock_dump.assert_called_once_with(sys.stdout, **{
        "host": None,
        "port": None,
        "unix_socket_path": "/var/run/redis0/redis.sock",
        "db": 1,
        "encoding": "utf-8",
    })
    mock_get_db_id.assert_called_once_with("ASIC_DB", SonicDBKey("asic0"))
    mock_initialize.assert_called_once_with()


@patch("swsscommon.swsscommon.SonicDBConfig.getDbSock")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbPort", return_value=6385)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname", return_value="192.0.2.10")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=17)
def test_explicit_tcp_is_strict(
        mock_get_db_id, mock_get_hostname, mock_get_port, mock_get_socket):
    mock_dump = run_command([
        "sonic-db-dump", "-n", "DPU_STATE_DB", "-t", "tcp",
    ])

    mock_dump.assert_called_once_with(sys.stdout, **{
        "host": "192.0.2.10",
        "port": 6385,
        "unix_socket_path": None,
        "db": 17,
        "encoding": "utf-8",
    })
    mock_get_socket.assert_not_called()


@patch("swsscommon.swsscommon.SonicDBConfig.getDbPort")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbSock", return_value=LOCAL_SOCKET)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=4)
def test_explicit_unix_socket_is_strict(
        mock_get_db_id, mock_get_socket, mock_get_hostname, mock_get_port):
    mock_dump = run_command([
        "sonic-db-dump", "-n", "CONFIG_DB", "-t", "unix_socket",
    ])

    mock_dump.assert_called_once_with(sys.stdout, **{
        "host": None,
        "port": None,
        "unix_socket_path": LOCAL_SOCKET,
        "db": 4,
        "encoding": "utf-8",
    })
    mock_get_hostname.assert_not_called()
    mock_get_port.assert_not_called()


@patch("swsscommon.swsscommon.SonicDBConfig.getDbSock", return_value="")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbPort", return_value=6385)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname", return_value="192.0.2.10")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=17)
def test_auto_keeps_empty_socket_remote_metadata_on_tcp(
        mock_get_db_id, mock_get_hostname, mock_get_port, mock_get_socket):
    mock_dump = run_command(["sonic-db-dump", "-n", "DPU_STATE_DB"])

    mock_dump.assert_called_once_with(sys.stdout, **{
        "host": "192.0.2.10",
        "port": 6385,
        "unix_socket_path": None,
        "db": 17,
        "encoding": "utf-8",
    })


@patch("swsscommon.swsscommon.SonicDBConfig.getDbSock", return_value="/var/run/redis-chassis/redis_chassis.sock")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbPort", return_value=6380)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname", return_value="redis_chassis.server")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=12)
def test_auto_keeps_chassis_metadata_on_tcp(
        mock_get_db_id, mock_get_hostname, mock_get_port, mock_get_socket):
    mock_dump = run_command(["sonic-db-dump", "-n", "CHASSIS_APP_DB"])

    mock_dump.assert_called_once_with(sys.stdout, **{
        "host": "redis_chassis.server",
        "port": 6380,
        "unix_socket_path": None,
        "db": 12,
        "encoding": "utf-8",
    })


@patch("redisdl.dump")
@patch("sonic_py_common.multi_asic.is_multi_asic", return_value=False)
@patch("swsscommon.swsscommon.SonicDBConfig.getDbSock", return_value="")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbPort")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbHostname")
@patch("swsscommon.swsscommon.SonicDBConfig.getDbId", return_value=17)
@patch("sys.argv", ["sonic-db-dump", "-n", "DPU_STATE_DB", "-t", "unix_socket"])
def test_explicit_unix_socket_fails_when_not_configured(
        mock_get_db_id, mock_get_hostname, mock_get_port, mock_get_socket,
        mock_is_multi_asic, mock_dump):
    with pytest.raises(ValueError, match="No Unix socket is configured"):
        sonic_db_dump_load()

    mock_dump.assert_not_called()
    mock_get_hostname.assert_not_called()
    mock_get_port.assert_not_called()


@pytest.fixture(autouse=True)
def accessible_configured_socket():
    with patch('os.access', return_value=True), \
            patch('swsscommon.swsscommon.SonicDBConfig.getDbPort', return_value=6379):
        yield


@pytest.mark.parametrize('port,accessible,expected', [
    (6379, False, 'tcp'), (6379, True, 'unix_socket'), (0, False, 'unix_socket'),
])
def test_auto_preserves_legacy_access_and_honors_port_zero(port, accessible, expected):
    with patch('swsscommon.swsscommon.SonicDBConfig.getDbSock', return_value=LOCAL_SOCKET), \
            patch('swsscommon.swsscommon.SonicDBConfig.getDbHostname', return_value='127.0.0.1'), \
            patch('swsscommon.swsscommon.SonicDBConfig.getDbPort', return_value=port), \
            patch('swsscommon.swsscommon.SonicDBConfig.getDbId', return_value=0), \
            patch('os.access', return_value=accessible):
        result = run_command(['sonic-db-dump']).call_args.kwargs
    assert result['unix_socket_path'] == (LOCAL_SOCKET if expected == 'unix_socket' else None)
    assert result['port'] == (None if expected == 'unix_socket' else port)
