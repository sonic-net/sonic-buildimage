"""Tests for sonic_py_common.grpc.gnoi.client.GnoiClient.

Uses FakeGnoiServer for real gRPC — no mocking.
"""

import unittest
import grpc

from sonic_py_common.grpc.gnoi.testing import FakeGnoiServer
from sonic_py_common.grpc.gnoi.client import GnoiClient
from sonic_py_common.grpc.gnoi import system_pb2


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


if __name__ == '__main__':
    unittest.main()
