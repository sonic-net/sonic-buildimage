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
    monkeypatch.setitem(sys.modules, 'sonic_py_common._redis_transport',
                        types.SimpleNamespace(use_local_redis_socket=MagicMock(return_value=True)))
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
@pytest.mark.parametrize('use_unix', [True, False])
def test_configured_endpoint_without_auth(led, db_name, db_id, use_unix):
    led.use_local_redis_socket.return_value = use_unix
    endpoint = ({'unix_socket_path': '/run/redis/' + db_name + '.sock'} if use_unix
                else {'host': '127.0.0.1', 'port': 6381})
    assert led._get_db_client(db_name) is led.redis.Redis.return_value
    led.use_local_redis_socket.assert_called_once_with(db_name)
    led.redis.Redis.assert_called_once_with(db=db_id, decode_responses=True, **endpoint)


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


def test_display_values_and_port_order(led):
    monitor = led.LedSwitchMonitor()
    monitor.redis_db4 = MagicMock()
    monitor.redis_db6 = MagicMock()
    monitor.redis_db4.hgetall.return_value = {'brkout_mode': '4x100G'}
    monitor.redis_db6.hgetall.return_value = {'netdev_oper_status': 'up'}
    monitor.redis_db4.keys.return_value = ['BREAKOUT_CFG|Ethernet8', 'BREAKOUT_CFG|Ethernet0']
    assert monitor.get_brkout_mode('Ethernet0') == '4x100G'
    assert monitor.get_netdev_oper_status('Ethernet0') == 'up'
    assert monitor.get_all_ethernet_ports() == ['Ethernet0', 'Ethernet8']
    monitor.redis_db4.hgetall.assert_called_once_with('BREAKOUT_CFG|Ethernet0')
    monitor.redis_db6.hgetall.assert_called_once_with('PORT_TABLE|Ethernet0')


def test_missing_values_keep_existing_result(led):
    monitor = led.LedSwitchMonitor()
    monitor.redis_db4 = MagicMock()
    monitor.redis_db6 = MagicMock()
    monitor.redis_db4.hgetall.return_value = {}
    monitor.redis_db6.hgetall.return_value = {}
    assert monitor.get_brkout_mode('Ethernet0') is None
    assert monitor.get_netdev_oper_status('Ethernet0') is None


@pytest.mark.parametrize('first,second,color', [
    ('up', 'up', 'LED_GREEN_ON'),
    ('down', 'down', 'LED_OFF'),
    ('up', 'down', 'LED_YELLOW_ON'),
])
def test_breakout_led_decision_and_duplicate_suppression(led, first, second, color):
    monitor = led.LedSwitchMonitor()
    monitor.redis_db6 = MagicMock()
    statuses = {'PORT_TABLE|Ethernet0': first, 'PORT_TABLE|Ethernet4': second}
    monitor.redis_db6.hgetall.side_effect = lambda key: {'netdev_oper_status': statuses[key]}
    monitor.execute_sap_command = MagicMock()
    last = {}
    monitor.handle_2x400g('Ethernet0', last, 'Ethernet0')
    monitor.handle_2x400g('Ethernet0', last, 'Ethernet0')
    monitor.execute_sap_command.assert_called_once_with('Ethernet0', getattr(led, color))


def test_special_port_led_decision(led):
    monitor = led.LedSwitchMonitor()
    monitor.redis_db6 = MagicMock()
    monitor.redis_db6.hgetall.return_value = {'netdev_oper_status': 'up'}
    monitor.execute_sap_command = MagicMock()
    monitor.handle_special_port()
    monitor.execute_sap_command.assert_called_once_with('Ethernet1024', led.LED_GREEN_ON)
    monitor.redis_db6.hgetall.return_value = {'netdev_oper_status': 'down'}
    monitor.execute_sap_command.reset_mock()
    monitor.handle_special_port()
    monitor.execute_sap_command.assert_called_once_with('Ethernet1024', led.LED_OFF)
