"""Redis endpoint tests; these do not access the physical EEPROM."""

import importlib.util
from pathlib import Path
import sys
import types
from unittest.mock import MagicMock

import pytest


@pytest.fixture
def eeprom(monkeypatch):
    modules = {
        'sonic_py_common._redis_transport': types.SimpleNamespace(
            use_local_redis_socket=MagicMock(return_value=True)),
        'swsscommon.redis': types.SimpleNamespace(
            connection_pool=MagicMock(return_value={})),
        'sonic_platform_base.sonic_eeprom.eeprom_base': types.SimpleNamespace(
            EepromDecoder=type('EepromDecoder', (), {})),
        'sonic_platform_base.sonic_eeprom.eeprom_tlvinfo': types.SimpleNamespace(
            TlvInfoDecoder=type('TlvInfoDecoder', (), {})),
        'swsscommon.swsscommon': types.SimpleNamespace(SonicDBConfig=MagicMock()),
    }
    for name, module in modules.items():
        monkeypatch.setitem(sys.modules, name, module)
    spec = importlib.util.spec_from_file_location(
        's6000_eeprom_test', Path(__file__).parents[1] / 'sonic_platform/eeprom.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.SonicDBConfig.getDbSock.return_value = '/run/redis/custom-state.sock'
    module.SonicDBConfig.getDbHostname.return_value = '127.0.0.1'
    module.SonicDBConfig.getDbPort.return_value = 6381
    module.SonicDBConfig.getDbId.return_value = 16
    monkeypatch.setattr(module.redis, 'Redis', MagicMock())
    return module


@pytest.mark.parametrize('use_unix', [True, False])
def test_configured_endpoint_uses_native_pool(eeprom, use_unix):
    eeprom.use_local_redis_socket.return_value = use_unix
    pool = MagicMock(name='native-pool')
    eeprom.connection_pool.return_value = pool
    endpoint = ({'unix_socket_path': '/run/redis/custom-state.sock'} if use_unix
                else {'host': '127.0.0.1', 'port': 6381})

    assert eeprom._get_state_db_client() is eeprom.redis.Redis.return_value

    eeprom.use_local_redis_socket.assert_called_once_with('STATE_DB')
    eeprom.connection_pool.assert_called_once_with(db=16, **endpoint)
    eeprom.redis.Redis.assert_called_once_with(connection_pool=pool)


def test_authentication_setup_error_is_not_ignored(eeprom):
    eeprom.connection_pool.side_effect = RuntimeError('missing selected credential')
    with pytest.raises(RuntimeError, match='missing selected credential'):
        eeprom._get_state_db_client()
    eeprom.redis.Redis.assert_not_called()


def test_cache_reader_keeps_existing_byte_response_behavior(eeprom):
    client = eeprom.redis.Redis.return_value
    client.hget.return_value = b'1'
    decoder = eeprom.EepromS6000.__new__(eeprom.EepromS6000)

    # The existing decoder compares the byte reply with a string. This PR
    # preserves that behavior instead of also changing response decoding.
    assert decoder.read_eeprom_db() == -1
    client.hget.assert_called_once_with('EEPROM_INFO|State', 'Initialized')
    client.hset.assert_not_called()
    eeprom.redis.Redis.assert_called_once()


def test_cache_writer_uses_selected_client(eeprom, capsys):
    client = eeprom.redis.Redis.return_value
    decoder = eeprom.EepromS6000.__new__(eeprom.EepromS6000)
    # Exercise the existing invalid-block path without a hardware read.
    assert decoder.update_eeprom_db(bytes(128)) == 0
    assert 'Invalid Block starting at EEPROM offset 0' in capsys.readouterr().out
    assert client.hset.call_args_list == [
        (('EEPROM_INFO|MFG BLOCK', 'Valid', '0'),),
        (('EEPROM_INFO|State', 'Initialized', '1'),),
    ]
    eeprom.redis.Redis.assert_called_once()
