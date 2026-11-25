"""Redis reader tests; hardware LED commands are not run."""

import importlib.util
import logging
from pathlib import Path
import sys
import types
from unittest.mock import MagicMock

import pytest


@pytest.fixture
def led(monkeypatch):
    config = MagicMock()
    monkeypatch.setitem(sys.modules, 'swsscommon.swsscommon',
                        types.SimpleNamespace(SonicDBConfig=config))
    monkeypatch.setattr(logging, 'FileHandler', lambda *args, **kwargs: logging.NullHandler())
    spec = importlib.util.spec_from_file_location(
        'micas_led_test', Path(__file__).parents[1] / 'ledswitch_monitor.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    config.getDbSock.side_effect = lambda name: '/run/redis/' + name + '.sock'
    config.getDbId.side_effect = {'CONFIG_DB': 14, 'STATE_DB': 16}.__getitem__
    config.getDbHostname.return_value = '127.0.0.1'
    config.getDbPort.return_value = 6381
    monkeypatch.setattr(module.redis, 'Redis', MagicMock())
    return module


@pytest.mark.parametrize('db_name,db_id', [('CONFIG_DB', 14), ('STATE_DB', 16)])
def test_configured_socket_without_auth(led, db_name, db_id):
    assert led._get_db_client(db_name) is led.redis.Redis.return_value
    led.redis.Redis.assert_called_once_with(
        db=db_id, decode_responses=True, unix_socket_path='/run/redis/' + db_name + '.sock')


def test_both_databases_connect_and_ping(led):
    clients = [MagicMock(), MagicMock()]
    led.redis.Redis.side_effect = clients
    monitor = led.LedSwitchMonitor()
    assert monitor.connect_redis()
    assert (monitor.redis_db4, monitor.redis_db6) == tuple(clients)
    for client in clients:
        client.ping.assert_called_once_with()


def test_connection_failure_keeps_retry_result(led):
    led.redis.Redis.return_value.ping.side_effect = ConnectionError('unavailable')
    assert not led.LedSwitchMonitor().connect_redis()

