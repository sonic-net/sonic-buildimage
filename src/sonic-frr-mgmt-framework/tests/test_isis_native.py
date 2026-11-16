"""Transport failure boundaries without real routing daemons."""
from unittest.mock import Mock

import pytest

from frrcfgd.isis_native import (NativeClient, NativeError, Operation,
                                 complete_running_config)


class Socket:
    def __init__(self, replies):
        self.replies = iter(replies)
        self.sent = []
        self.closed = False

    def settimeout(self, timeout):
        assert timeout > 0

    def connect(self, path):
        self.path = path

    def sendall(self, command):
        self.sent.append(command)

    def recv(self, size):
        reply = next(self.replies, b'')
        if isinstance(reply, Exception):
            raise reply
        return reply

    def close(self):
        self.closed = True


def client(*replies, **kwargs):
    connection = Socket(replies)
    return NativeClient(socket_factory=lambda *args: connection, **kwargs), connection


def test_fragmented_native_readback():
    transport, connection = client(b'\0\0', b'\0\0', b'frr version 10.5.4\n', b'!\n\0\0\0', b'\0')
    assert transport.query('isisd') == 'frr version 10.5.4\n!\n'
    assert connection.sent == [b'enable\0', b'do write terminal\0']
    assert connection.closed


@pytest.mark.parametrize('reply', [b'% Unknown command\n\0\0\0\0', b'failure\0\0\0\x01', b''])
def test_no_dependent_mutation_after_failure(reply):
    transport, connection = client(b'\0\0\0\0', reply)
    with pytest.raises(NativeError):
        transport.apply([Operation('isisd', ('configure terminal', 'router isis default', 'net 49.00'))])
    assert connection.sent == [b'enable\0', b'configure terminal\0']
    assert connection.closed


def test_uncertain_response_discards_session():
    failed = Socket([b'\0\0\0\0', TimeoutError('lost reply')])
    fresh = Socket([b'\0\0\0\0', b'frr version 10.5.4\n!\n\0\0\0\0'])
    factory = Mock(side_effect=[failed, fresh])
    transport = NativeClient(socket_factory=factory)
    with pytest.raises(NativeError):
        transport.apply([Operation('isisd', ('configure terminal', 'router isis default'))])
    assert failed.closed
    assert transport.query('isisd').startswith('frr version')
    assert fresh.sent == [b'enable\0', b'do write terminal\0']
    assert fresh.closed


def test_reply_limit_and_invalid_command():
    transport, connection = client(b'\0\0\0\0', b'x' * 17, max_reply=16)
    with pytest.raises(NativeError, match='limit'):
        transport.query('isisd')
    transport, connection = client(b'\0\0\0\0')
    with pytest.raises(NativeError, match='Invalid VTY command'):
        transport.query('isisd', 'show version\nconfigure terminal')
    assert connection.sent == [b'enable\0']


def test_expired_budget_does_not_send_command():
    transport, connection = client(clock=iter([0, 2]).__next__)
    with pytest.raises(NativeError, match='deadline'):
        transport.query('isisd', budget=1)
    assert not connection.sent


def test_only_supported_scoped_daemons():
    transport, connection = client()
    with pytest.raises(NativeError, match='Unsupported daemon'):
        transport.query('bgpd')
    assert not connection.sent


def test_framed_writer_wrapper_requires_real_nonempty_output():
    with pytest.raises(NativeError):
        complete_running_config([('isisd', '')])
    with pytest.raises(NativeError):
        complete_running_config([('isisd', '% Error\n')])
    with pytest.raises(NativeError):
        complete_running_config([('isisd', '!\n')])
    result = complete_running_config([('isisd', 'frr version 10.5.4\n!\n')])
    assert result.endswith('end\n')
