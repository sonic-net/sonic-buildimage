from unittest import mock

import pytest

from sonic_py_common import _redis_transport


@pytest.mark.parametrize('namespace', [None, '', 'asic0'])
@pytest.mark.parametrize('connect_allowed', [False, True])
@mock.patch('sonic_py_common._redis_transport.os.access')
@mock.patch('sonic_py_common._redis_transport.swsscommon.SonicDBConfig')
@mock.patch('sonic_py_common._redis_transport.swsscommon.SonicDBKey')
def test_legacy_uses_socket_only_when_connect_allowed(
        key_type, config, access, namespace, connect_allowed):
    config.getDbPort.return_value = 6379
    config.getDbSock.return_value = '/var/run/redis/redis.sock'
    access.return_value = connect_allowed

    assert _redis_transport.use_local_redis_socket('CONFIG_DB', namespace) is connect_allowed
    key_type.assert_called_once_with(namespace or '')
    config.getDbPort.assert_called_once_with('CONFIG_DB', key_type.return_value)
    config.getDbSock.assert_called_once_with('CONFIG_DB', key_type.return_value)
    access.assert_called_once_with('/var/run/redis/redis.sock', _redis_transport.os.W_OK)


@mock.patch('sonic_py_common._redis_transport.os.access')
@mock.patch('sonic_py_common._redis_transport.swsscommon.SonicDBConfig')
def test_legacy_without_socket_keeps_tcp(config, access):
    config.getDbPort.return_value = 6379
    config.getDbSock.return_value = ''

    assert _redis_transport.use_local_redis_socket('STATE_DB') is False
    access.assert_not_called()


@pytest.mark.parametrize('path', ['', '/missing/socket', '/forbidden/socket'])
@mock.patch('sonic_py_common._redis_transport.os.access', return_value=False)
@mock.patch('sonic_py_common._redis_transport.swsscommon.SonicDBConfig')
def test_port_zero_requires_socket_without_tcp_fallback(config, access, path):
    config.getDbPort.return_value = 0
    config.getDbSock.return_value = path

    assert _redis_transport.use_local_redis_socket('CONFIG_DB') is True
    config.getDbSock.assert_not_called()
    access.assert_not_called()
