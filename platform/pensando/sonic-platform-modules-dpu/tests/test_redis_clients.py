"""Exercise local Redis operations without initializing DPU hardware."""
import ast
from pathlib import Path
import sys
import types
from unittest.mock import MagicMock

import pytest


ROOT = Path(__file__).parents[1]


def load_definitions(path, names, namespace):
    tree = ast.parse(path.read_text())
    tree.body = [node for node in tree.body
                 if isinstance(node, (ast.FunctionDef, ast.ClassDef)) and node.name in names]
    assert len(tree.body) == len(names)
    exec(compile(tree, str(path), 'exec'), namespace)
    return types.SimpleNamespace(**namespace)


@pytest.fixture
def clients(monkeypatch):
    sdk, pool, redis = (MagicMock() for _ in range(3))
    sdk.SonicDBConfig.getDbId.side_effect = {'CONFIG_DB': 4, 'STATE_DB': 6}.__getitem__
    sdk.SonicDBConfig.getDbSock.side_effect = lambda name: '/run/redis/' + name + '.sock'
    sdk.SonicDBConfig.getDbHostname.return_value = '127.0.0.1'
    sdk.SonicDBConfig.getDbPort.return_value = 6381
    parent = types.ModuleType('swsscommon')
    parent.__path__ = []
    parent.swsscommon = sdk
    monkeypatch.setitem(sys.modules, 'swsscommon', parent)
    monkeypatch.setitem(sys.modules, 'swsscommon.redis', types.SimpleNamespace(connection_pool=pool))
    monkeypatch.setitem(sys.modules, 'redis', redis)
    return sdk, pool, redis


def test_disable_containers_authenticates_config_writes(clients):
    _, pool, redis = clients
    redis.StrictRedis.return_value.exists.side_effect = [True, False]
    module = load_definitions(ROOT / 'dpu/utils/dpu_pensando_util.py', ['disable_unused_containers'], {
        'DISABLE_CONTAINER_LIST': ['bgp', 'lldp'], 'log_err': MagicMock()})
    module.disable_unused_containers()
    pool.assert_called_once_with(unix_socket_path='/run/redis/CONFIG_DB.sock', db=4,
                                 decode_responses=True, authenticate=True)
    redis.StrictRedis.return_value.hset.assert_called_once_with('FEATURE|bgp', 'state', 'disabled')
    module.log_err.assert_not_called()


@pytest.mark.parametrize('remote_available', [True, False])
def test_health_writer_preserves_remote_and_authenticates_local_fallback(clients, remote_available):
    _, pool, redis = clients
    remote, local = MagicMock(), MagicMock()
    if not remote_available:
        remote.ping.side_effect = ConnectionError('remote unavailable')
    redis.Redis.side_effect = [remote, local]
    module = load_definitions(ROOT / 'dpu/utils/fetch_dpu_status', ['DPUHealthUpdater'], {
        'redis': redis, 'get_slot_id': lambda chassis: 2, 'log_err': MagicMock(),
        'DPU_HEALTH_INFO_TABLE_NAME': 'DPU_HEALTH_INFO', 'REDIS_CHASSIS_SERVER_IP': '192.0.2.10',
        'REDIS_CHASSIS_SERVER_PORT': 6380, 'CHASSIS_STATE_DB': 13})
    updater = module.DPUHealthUpdater(object())
    assert updater.db is (remote if remote_available else local)
    assert redis.Redis.call_args_list[0].kwargs == {
        'host': '192.0.2.10', 'port': 6380, 'decode_responses': True, 'db': 13}
    if remote_available:
        pool.assert_not_called()
    else:
        pool.assert_called_once_with(unix_socket_path='/run/redis/STATE_DB.sock', db=6,
                                     decode_responses=True, authenticate=True)


@pytest.fixture
def eeprom(clients):
    _, _, redis = clients
    return load_definitions(ROOT / 'sonic_platform/fru_tlvinfo_decoder.py',
                            ['FruTlvInfoEncoder', 'EepromDefaultVisitor', 'EepromRedisVisitor'], {
        'redis': redis, 'eeprom_base': types.SimpleNamespace(EepromDecoder=object)})


@pytest.mark.parametrize('use_socket', [False, True])
def test_eeprom_reader_keeps_transport_and_cached_connection(clients, eeprom, use_socket):
    _, pool, redis = clients
    encoder = eeprom.FruTlvInfoEncoder.__new__(eeprom.FruTlvInfoEncoder)
    encoder._redis_client = None
    encoder._state_db_use_tcp = not use_socket
    assert encoder.redis_client is encoder.redis_client
    options = ({'unix_socket_path': '/run/redis/STATE_DB.sock'} if use_socket
               else {'host': '127.0.0.1', 'port': 6381})
    redis.Redis.assert_called_once_with(db=6, **options)
    pool.assert_not_called()


def test_eeprom_visitor_uses_a_separate_authenticated_writer(clients, eeprom):
    _, pool, redis = clients
    encoder = eeprom.FruTlvInfoEncoder.__new__(eeprom.FruTlvInfoEncoder)
    encoder._redis_client = MagicMock(name='reader')
    visitor = eeprom.EepromRedisVisitor(encoder)
    pool.assert_called_once_with(unix_socket_path='/run/redis/STATE_DB.sock', db=6, authenticate=True)
    visitor.visit_header('TlvInfo', 1, 12)
    redis.Redis.return_value.hmset.assert_called_once()
    encoder._redis_client.hmset.assert_not_called()
