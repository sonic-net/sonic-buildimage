"""Local Redis transport and slot-replacement checks without platform hardware."""

import importlib.util
from pathlib import Path
import sys
import types
from unittest.mock import MagicMock

import pytest
import redis


@pytest.fixture
def slot_module(monkeypatch):
    package = types.ModuleType('slot_redis_test')
    package.__path__ = []
    monkeypatch.setitem(sys.modules, package.__name__, package)
    modules = {
        'sonic_platform_base.module_base': types.SimpleNamespace(
            ModuleBase=type('ModuleBase', (), {'MODULE_REBOOT_DPU': 0})),
        'sonic_platform_base.chassis_base': types.SimpleNamespace(ChassisBase=object),
        'sonic_py_common.syslogger': types.SimpleNamespace(SysLogger=MagicMock()),
        'slot_redis_test.dpuctlplat': types.SimpleNamespace(
            DpuCtlPlat=MagicMock(), BootProgEnum=MagicMock(), PCI_DEV_BASE='/sys/bus/pci/devices'),
        'slot_redis_test.utils': MagicMock(),
        'slot_redis_test.device_data': types.SimpleNamespace(
            DeviceDataManager=MagicMock(), DpuInterfaceEnum=MagicMock()),
        'slot_redis_test.vpd_parser': types.SimpleNamespace(VpdParser=MagicMock()),
        'slot_redis_test.dpu_vpd_parser': types.SimpleNamespace(DpuVpdParser=MagicMock()),
        'swsscommon.swsscommon': types.SimpleNamespace(
            ConfigDBConnector=MagicMock(), SonicDBConfig=MagicMock(), SonicV2Connector=MagicMock()),
    }
    for name, module in modules.items():
        monkeypatch.setitem(sys.modules, name, module)
    spec = importlib.util.spec_from_file_location(
        'slot_redis_test.module', Path(__file__).parents[1] / 'sonic_platform/module.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.SonicDBConfig.getDbId.return_value = 16
    module.SonicDBConfig.getDbHostname.return_value = '127.0.0.2'
    module.SonicDBConfig.getDbSock.return_value = '/run/redis/custom state.sock'
    monkeypatch.setattr(redis, 'Redis', MagicMock())
    return module


@pytest.mark.parametrize('port', [0, 6387])
def test_slot_replacement_uses_configured_endpoint_and_cached_client(slot_module, port):
    module = slot_module
    module.SonicDBConfig.getDbPort.return_value = port
    redis.Redis.return_value.hget.side_effect = [b'1', b'2']
    slot = module.Module(1)
    slot.current_state = module.Module.STATE_ACTIVATED
    module.utils.read_int_from_file.return_value = module.Module.STATE_ACTIVATED
    slot._re_init = MagicMock()

    slot._check_state()
    slot._re_init.reset_mock()
    slot._check_state()

    endpoint = ({'unix_socket_path': '/run/redis/custom state.sock'} if port == 0
                else {'host': '127.0.0.2', 'port': 6387})
    redis.Redis.assert_called_once_with(db=16, **endpoint)
    assert slot.seq_no == '2'
    slot._re_init.assert_called_once_with()
    assert redis.Redis.return_value.hget.call_count == 2


def test_unavailable_protected_socket_does_not_retry_tcp(slot_module):
    module = slot_module
    module.SonicDBConfig.getDbPort.return_value = 0
    redis.Redis.return_value.hget.side_effect = redis.ConnectionError('socket unavailable')

    assert module.Module(1)._get_seq_no() == 0
    redis.Redis.assert_called_once_with(db=16, unix_socket_path='/run/redis/custom state.sock')
    module.SonicDBConfig.getDbHostname.assert_not_called()
