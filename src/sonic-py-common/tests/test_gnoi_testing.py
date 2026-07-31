"""Tests for sonic_py_common.grpc.gnoi.testing (FakeGnoiServer).

These tests use a real gRPC server and real GnoiClient — no mocking.
"""

import unittest
import grpc

from sonic_py_common.grpc.gnoi.testing import FakeGnoiServer, FakeSystemServicer
from sonic_py_common.grpc.gnoi.client import GnoiClient
from sonic_py_common.grpc.gnoi import system_pb2


class TestFakeSystemServicer(unittest.TestCase):
    """Unit tests for FakeSystemServicer behavior."""

    @classmethod
    def setUpClass(cls):
        cls.server = FakeGnoiServer()
        cls.server.start()

    @classmethod
    def tearDownClass(cls):
        cls.server.stop()

    def setUp(self):
        self.server.reset()

    def test_reboot_default_success(self):
        with GnoiClient(self.server.target) as client:
            resp = client.system.Reboot(
                system_pb2.RebootRequest(method=system_pb2.HALT, message="test"),
                timeout=5,
            )
        self.assertIsNotNone(resp)
        self.assertEqual(len(self.server.system.reboot_calls), 1)
        self.assertEqual(self.server.system.reboot_calls[0].method, system_pb2.HALT)
        self.assertEqual(self.server.system.reboot_calls[0].message, "test")

    def test_reboot_error(self):
        self.server.system.set_reboot_response(
            error_code=grpc.StatusCode.UNAVAILABLE,
            error_message="DPU unreachable",
        )
        with GnoiClient(self.server.target) as client:
            with self.assertRaises(grpc.RpcError) as ctx:
                client.system.Reboot(system_pb2.RebootRequest(), timeout=5)
            self.assertEqual(ctx.exception.code(), grpc.StatusCode.UNAVAILABLE)
            self.assertIn("DPU unreachable", ctx.exception.details())

    def test_reboot_status_default_complete(self):
        with GnoiClient(self.server.target) as client:
            resp = client.system.RebootStatus(
                system_pb2.RebootStatusRequest(), timeout=5
            )
        self.assertFalse(resp.active)
        self.assertEqual(
            resp.status.status,
            system_pb2.RebootStatus.Status.STATUS_SUCCESS,
        )

    def test_reboot_status_active(self):
        self.server.system.set_reboot_status(active=True)
        with GnoiClient(self.server.target) as client:
            resp = client.system.RebootStatus(
                system_pb2.RebootStatusRequest(), timeout=5
            )
        self.assertTrue(resp.active)

    def test_reboot_status_sequence(self):
        """Polling scenario: active → active → done."""
        active_resp = system_pb2.RebootStatusResponse()
        active_resp.active = True

        done_resp = system_pb2.RebootStatusResponse()
        done_resp.active = False
        done_resp.status.status = system_pb2.RebootStatus.Status.STATUS_SUCCESS

        self.server.system.set_reboot_status_sequence([active_resp, active_resp, done_resp])

        with GnoiClient(self.server.target) as client:
            r1 = client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=5)
            self.assertTrue(r1.active)
            r2 = client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=5)
            self.assertTrue(r2.active)
            r3 = client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=5)
            self.assertFalse(r3.active)

        self.assertEqual(len(self.server.system.reboot_status_calls), 3)

    def test_reboot_status_sequence_with_error(self):
        """Polling with transient error then success."""
        done_resp = system_pb2.RebootStatusResponse()
        done_resp.active = False
        done_resp.status.status = system_pb2.RebootStatus.Status.STATUS_SUCCESS

        self.server.system.set_reboot_status_sequence([
            (grpc.StatusCode.UNAVAILABLE, "transient"),
            done_resp,
        ])

        with GnoiClient(self.server.target) as client:
            with self.assertRaises(grpc.RpcError):
                client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=5)
            r2 = client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=5)
            self.assertFalse(r2.active)

    def test_reboot_status_error(self):
        self.server.system.set_reboot_status(
            error_code=grpc.StatusCode.INTERNAL,
            error_message="internal failure",
        )
        with GnoiClient(self.server.target) as client:
            with self.assertRaises(grpc.RpcError) as ctx:
                client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=5)
            self.assertEqual(ctx.exception.code(), grpc.StatusCode.INTERNAL)

    def test_cancel_reboot(self):
        with GnoiClient(self.server.target) as client:
            resp = client.system.CancelReboot(
                system_pb2.CancelRebootRequest(message="abort"), timeout=5
            )
        self.assertIsNotNone(resp)
        self.assertEqual(len(self.server.system.cancel_reboot_calls), 1)

    def test_reset_clears_history(self):
        with GnoiClient(self.server.target) as client:
            client.system.Reboot(system_pb2.RebootRequest(), timeout=5)
        self.assertEqual(len(self.server.system.reboot_calls), 1)
        self.server.reset()
        self.assertEqual(len(self.server.system.reboot_calls), 0)


class TestFakeGnoiServer(unittest.TestCase):
    """Test server lifecycle."""

    def test_context_manager(self):
        with FakeGnoiServer() as server:
            self.assertIn("localhost:", server.target)
            with GnoiClient(server.target) as client:
                resp = client.system.RebootStatus(
                    system_pb2.RebootStatusRequest(), timeout=5
                )
                self.assertFalse(resp.active)

    def test_end_to_end_reboot_flow(self):
        """Full reboot + poll flow like gnoi_shutdown_daemon."""
        with FakeGnoiServer() as server:
            active = system_pb2.RebootStatusResponse()
            active.active = True
            done = system_pb2.RebootStatusResponse()
            done.active = False
            done.status.status = system_pb2.RebootStatus.Status.STATUS_SUCCESS
            server.system.set_reboot_status_sequence([active, done])

            with GnoiClient(server.target) as client:
                # Send reboot
                client.system.Reboot(
                    system_pb2.RebootRequest(method=system_pb2.HALT, message="shutdown"),
                    timeout=5,
                )
                # Poll
                r1 = client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=5)
                self.assertTrue(r1.active)
                r2 = client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=5)
                self.assertFalse(r2.active)

            self.assertEqual(len(server.system.reboot_calls), 1)
            self.assertEqual(server.system.reboot_calls[0].method, system_pb2.HALT)
            self.assertEqual(len(server.system.reboot_status_calls), 2)


if __name__ == '__main__':
    unittest.main()
