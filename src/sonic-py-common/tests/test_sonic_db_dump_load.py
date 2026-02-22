import sys
from sonic_py_common.sonic_db_dump_load import sonic_db_dump_load
from swsscommon.swsscommon import SonicDBKey
from unittest.mock import patch

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