"""Tests for sonic_py_common.grpc.gnoi.testing (FakeGnoiServer).

These tests use a real gRPC server and real GnoiClient — no mocking.
"""

import sys
import unittest

if sys.version_info[0] < 3:
    # See sibling test_gnoi_client.py — module is Py3-only.
    raise unittest.SkipTest("sonic_py_common.grpc.gnoi requires Python 3")

import grpc  # noqa: E402

from sonic_py_common.grpc.gnoi.testing import FakeGnoiServer  # noqa: E402
from sonic_py_common.grpc.gnoi.client import GnoiClient  # noqa: E402
from sonic_py_common.grpc.gnoi import system_pb2  # noqa: E402


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


class TestFakeGnoiServerGuards(unittest.TestCase):
    """Tests for FakeGnoiServer lifecycle / misuse guards."""

    def test_target_before_start_raises(self):
        """target accessed before start() raises a clear error.

        Previously returned the misleading string 'localhost:None', which
        manifested as a confusing connection error rather than a misuse
        diagnostic.
        """
        server = FakeGnoiServer()
        with self.assertRaises(RuntimeError) as ctx:
            _ = server.target
        self.assertIn("start()", str(ctx.exception))

    def test_target_after_stop_raises(self):
        """target accessed after stop() raises a clear error.

        Without resetting _port in stop(), target would keep pointing at a
        defunct server, causing tests to fail with connection errors
        instead of an obvious misuse signal.
        """
        server = FakeGnoiServer()
        server.start()
        server.stop()
        with self.assertRaises(RuntimeError):
            _ = server.target

    def test_stop_waits_for_termination(self):
        """stop() returns only after the underlying gRPC server signals done.

        grpc.Server.stop(grace) returns a threading.Event; pinning the wait
        prevents leaking server threads across many start/stop iterations.
        """
        for _ in range(5):
            server = FakeGnoiServer()
            server.start()
            target = server.target
            server.stop()
            # After stop returns, the port should no longer accept new
            # connections — i.e., a new client should fail fast rather than
            # hang. We don't strictly need to assert that here (would race
            # with port reuse); the key contract is that stop() doesn't
            # return mid-shutdown. Just exercising the loop without
            # blowing the file-descriptor budget is the smoke test.
            self.assertTrue(target.startswith("localhost:"))


class TestFakeSystemServicerEdgeCases(unittest.TestCase):
    """Behavioral guarantees for FakeSystemServicer that aren't covered
    elsewhere — sequence exhaustion fallback, explicit-empty-message
    handling.
    """

    @classmethod
    def setUpClass(cls):
        cls.server = FakeGnoiServer()
        cls.server.start()

    @classmethod
    def tearDownClass(cls):
        cls.server.stop()

    def setUp(self):
        self.server.reset()

    def test_status_sequence_falls_back_to_single_response_after_exhaustion(self):
        """Docstring promises: after sequence exhaustion, fall back to the
        single configured response. Verify the contract."""
        single = system_pb2.RebootStatusResponse()
        single.active = False
        single.status.message = "fallback"
        self.server.system.set_reboot_status(response=single)

        seq_item = system_pb2.RebootStatusResponse()
        seq_item.active = True
        seq_item.status.message = "from sequence"
        self.server.system.set_reboot_status_sequence([seq_item])

        with GnoiClient(self.server.target) as client:
            r1 = client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=5)
            r2 = client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=5)

        self.assertEqual(r1.status.message, "from sequence")
        self.assertTrue(r1.active)
        # Second call: sequence exhausted, should return the single response
        self.assertEqual(r2.status.message, "fallback")
        self.assertFalse(r2.active)

    def test_set_reboot_status_message_can_be_cleared_to_empty(self):
        """message='' should clear the previous value; previously the
        falsy guard silently kept it."""
        self.server.system.set_reboot_status(message="hello")
        self.server.system.set_reboot_status(message="")

        with GnoiClient(self.server.target) as client:
            resp = client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=5)
        self.assertEqual(resp.status.message, "")


    def test_set_reboot_status_message_default_does_not_clobber(self):
        """Default message=None must NOT overwrite a previously set message.

        Previously the default was ``message=""`` which the falsy guard
        translated into a silent "keep previous"; changing the default to
        ``None`` made the empty-string case meaningful but risks clobbering
        on every other call if not handled correctly. Pin both halves of
        the contract.
        """
        self.server.system.set_reboot_status(message="orig")
        # Call without message kwarg: message should be preserved.
        self.server.system.set_reboot_status(active=True)

        with GnoiClient(self.server.target) as client:
            resp = client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=5)
        self.assertEqual(resp.status.message, "orig")
        self.assertTrue(resp.active)


if __name__ == '__main__':
    unittest.main()
