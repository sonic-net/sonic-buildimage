"""Tests for sonic_grpc.gnoi.client.GnoiClient.

Uses FakeGnoiServer for real gRPC — no mocking.
"""

import sys
import unittest

if sys.version_info[0] < 3:
    # sonic_grpc.gnoi is only packaged for Py3 (see setup.py),
    # and grpcio dropped Py2 support years ago. Skip the whole module on
    # Py2 so the legacy ENABLE_PY2_MODULES=y wheel build doesn't fail on
    # an unsatisfiable import. ``raise SkipTest`` at module level is
    # recognized by both ``unittest`` and ``pytest`` collection.
    raise unittest.SkipTest("sonic_grpc.gnoi requires Python 3")

from unittest import mock  # noqa: E402
import grpc  # noqa: E402

from sonic_grpc.gnoi.testing import FakeGnoiServer  # noqa: E402
from sonic_grpc.gnoi.client import GnoiClient  # noqa: E402
from sonic_grpc.gnoi import system_pb2  # noqa: E402


class TestGnoiClient(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.server = FakeGnoiServer()
        cls.server.start()

    @classmethod
    def tearDownClass(cls):
        cls.server.stop()

    def setUp(self):
        self.server.reset()

    def test_context_manager_creates_and_closes_channel(self):
        with GnoiClient(self.server.target) as client:
            self.assertIsNotNone(client.channel)
        # After exit, channel is None
        self.assertIsNone(client.channel)

    def test_close_idempotent(self):
        client = GnoiClient(self.server.target)
        client.__enter__()
        client.close()
        client.close()  # Should not raise

    def test_channel_not_available_before_enter(self):
        client = GnoiClient(self.server.target)
        self.assertIsNone(client.channel)

    def test_system_reboot_rpc(self):
        with GnoiClient(self.server.target) as client:
            resp = client.system.Reboot(
                system_pb2.RebootRequest(method=system_pb2.HALT),
                timeout=5,
            )
        self.assertIsNotNone(resp)
        self.assertEqual(len(self.server.system.reboot_calls), 1)

    def test_system_reboot_status_rpc(self):
        with GnoiClient(self.server.target) as client:
            resp = client.system.RebootStatus(
                system_pb2.RebootStatusRequest(), timeout=5
            )
        self.assertFalse(resp.active)

    def test_grpc_error_propagation(self):
        self.server.system.set_reboot_response(
            error_code=grpc.StatusCode.UNAVAILABLE,
            error_message="connection refused",
        )
        with GnoiClient(self.server.target) as client:
            with self.assertRaises(grpc.RpcError) as ctx:
                client.system.Reboot(system_pb2.RebootRequest(), timeout=5)
            self.assertEqual(ctx.exception.code(), grpc.StatusCode.UNAVAILABLE)

    def test_channel_options_accepted(self):
        """GnoiClient accepts gRPC channel options."""
        opts = [('grpc.keepalive_time_ms', 10000)]
        with GnoiClient(self.server.target, options=opts) as client:
            resp = client.system.RebootStatus(
                system_pb2.RebootStatusRequest(), timeout=5
            )
        self.assertIsNotNone(resp)

    def test_multiple_rpcs_on_same_channel(self):
        with GnoiClient(self.server.target) as client:
            client.system.Reboot(system_pb2.RebootRequest(), timeout=5)
            client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=5)
            client.system.CancelReboot(system_pb2.CancelRebootRequest(), timeout=5)
        self.assertEqual(len(self.server.system.reboot_calls), 1)
        self.assertEqual(len(self.server.system.reboot_status_calls), 1)
        self.assertEqual(len(self.server.system.cancel_reboot_calls), 1)

    def test_credentials_none_uses_insecure_channel(self):
        """When credentials is None, GnoiClient opens an insecure channel."""
        with mock.patch(
            "sonic_grpc.gnoi.client.grpc.insecure_channel"
        ) as m_insecure, mock.patch(
            "sonic_grpc.gnoi.client.grpc.secure_channel"
        ) as m_secure:
            m_insecure.return_value = mock.MagicMock()
            client = GnoiClient(self.server.target)
            client.__enter__()
            client.close()
        m_insecure.assert_called_once()
        m_secure.assert_not_called()

    def test_credentials_provided_uses_secure_channel(self):
        """When credentials is provided, GnoiClient opens a secure channel."""
        creds = grpc.ssl_channel_credentials()
        with mock.patch(
            "sonic_grpc.gnoi.client.grpc.insecure_channel"
        ) as m_insecure, mock.patch(
            "sonic_grpc.gnoi.client.grpc.secure_channel"
        ) as m_secure:
            m_secure.return_value = mock.MagicMock()
            client = GnoiClient("dut:50052", credentials=creds)
            client.__enter__()
            client.close()
        m_secure.assert_called_once()
        # First positional arg is the target, second is the credentials
        args, kwargs = m_secure.call_args
        self.assertEqual(args[0], "dut:50052")
        self.assertIs(args[1], creds)
        m_insecure.assert_not_called()

    def test_unix_socket_target_uses_insecure_channel(self):
        """unix:// targets are passed through to insecure_channel as-is.

        gRPC supports the ``unix://`` scheme natively for local IPC, so no
        special handling is needed in GnoiClient — but pin the behavior so
        a future refactor doesn't accidentally break it.
        """
        target = "unix:///var/run/gnmi/gnmi.sock"
        with mock.patch(
            "sonic_grpc.gnoi.client.grpc.insecure_channel"
        ) as m_insecure, mock.patch(
            "sonic_grpc.gnoi.client.grpc.secure_channel"
        ) as m_secure:
            m_insecure.return_value = mock.MagicMock()
            client = GnoiClient(target)
            client.__enter__()
            client.close()
        m_insecure.assert_called_once()
        args, kwargs = m_insecure.call_args
        self.assertEqual(args[0], target)
        m_secure.assert_not_called()

    def test_service_stub_before_open_raises(self):
        """Accessing a service stub before __enter__ raises a clear error.

        Without the explicit guard, callers got a confusing AttributeError
        from inside the generated stub code when self._channel was None.
        """
        client = GnoiClient(self.server.target)
        with self.assertRaises(RuntimeError) as ctx:
            client.system  # noqa: B018 — accessing the property is the test
        self.assertIn("context manager", str(ctx.exception))

    def test_service_stub_after_close_raises(self):
        """Accessing a service stub after close() raises a clear error."""
        client = GnoiClient(self.server.target)
        client.__enter__()
        client.close()
        with self.assertRaises(RuntimeError):
            client.system  # noqa: B018


if __name__ == '__main__':
    unittest.main()
