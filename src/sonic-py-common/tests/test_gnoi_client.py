"""Tests for sonic_py_common.grpc.gnoi.client.GnoiClient."""

import sys
import unittest
from unittest.mock import MagicMock, patch, PropertyMock

# Mock grpc
mock_grpc = MagicMock()
mock_grpc.RpcError = type('RpcError', (Exception,), {})
sys.modules['grpc'] = mock_grpc

# Mock proto stubs
sys.modules['sonic_py_common.grpc.gnoi.system_pb2'] = MagicMock()
sys.modules['sonic_py_common.grpc.gnoi.system_pb2_grpc'] = MagicMock()
sys.modules['sonic_py_common.grpc.gnoi.types_pb2'] = MagicMock()
sys.modules['sonic_py_common.grpc.gnoi.common_pb2'] = MagicMock()

from sonic_py_common.grpc.gnoi.client import GnoiClient
from sonic_py_common.grpc.gnoi import system_pb2_grpc


class TestGnoiClient(unittest.TestCase):

    def test_context_manager_creates_channel(self):
        """Test that entering context creates an insecure channel."""
        with GnoiClient("10.0.0.1:8080") as client:
            mock_grpc.insecure_channel.assert_called_with("10.0.0.1:8080", options=None)
            self.assertIsNotNone(client.channel)

    def test_context_manager_closes_channel(self):
        """Test that exiting context closes the channel."""
        with GnoiClient("10.0.0.1:8080") as client:
            channel = client.channel
        channel.close.assert_called()

    def test_close_idempotent(self):
        """Test that close() can be called multiple times safely."""
        client = GnoiClient("10.0.0.1:8080")
        client.__enter__()
        client.close()
        client.close()  # Should not raise

    def test_system_property_returns_stub(self):
        """Test that .system returns a SystemStub."""
        with GnoiClient("10.0.0.1:8080") as client:
            stub = client.system
            system_pb2_grpc.SystemStub.assert_called_with(client.channel)

    def test_channel_options_passed(self):
        """Test that gRPC channel options are forwarded."""
        opts = [('grpc.keepalive_time_ms', 10000)]
        with GnoiClient("10.0.0.1:8080", options=opts) as client:
            mock_grpc.insecure_channel.assert_called_with("10.0.0.1:8080", options=opts)

    def test_system_reboot_rpc(self):
        """Test calling Reboot through the system stub."""
        mock_system_stub = MagicMock()
        system_pb2_grpc.SystemStub.return_value = mock_system_stub

        with GnoiClient("10.0.0.1:8080") as client:
            request = MagicMock()
            client.system.Reboot(request, timeout=60)
            mock_system_stub.Reboot.assert_called_once_with(request, timeout=60)

    def test_system_reboot_status_rpc(self):
        """Test calling RebootStatus through the system stub."""
        mock_system_stub = MagicMock()
        system_pb2_grpc.SystemStub.return_value = mock_system_stub

        with GnoiClient("10.0.0.1:8080") as client:
            request = MagicMock()
            client.system.RebootStatus(request, timeout=10)
            mock_system_stub.RebootStatus.assert_called_once_with(request, timeout=10)

    def test_grpc_error_propagation(self):
        """Test that gRPC errors propagate to callers."""
        mock_system_stub = MagicMock()
        rpc_error = mock_grpc.RpcError()
        mock_system_stub.Reboot.side_effect = rpc_error
        system_pb2_grpc.SystemStub.return_value = mock_system_stub

        with GnoiClient("10.0.0.1:8080") as client:
            with self.assertRaises(mock_grpc.RpcError):
                client.system.Reboot(MagicMock(), timeout=60)

    def test_channel_not_available_before_enter(self):
        """Test that channel is None before entering context."""
        client = GnoiClient("10.0.0.1:8080")
        self.assertIsNone(client.channel)


if __name__ == '__main__':
    unittest.main()
