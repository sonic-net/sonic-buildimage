"""Tests for sonic_py_common.grpc.gnoi.testing (FakeGnoiClient)."""

import sys
import unittest
from unittest.mock import MagicMock

# Mock grpc module and submodules before any imports
mock_grpc = MagicMock()
mock_grpc.__version__ = '1.80.0'
mock_grpc.RpcError = type('RpcError', (Exception,), {
    'code': lambda self: 'UNAVAILABLE',
    'details': lambda self: 'fake error',
})
mock_utilities = MagicMock()
mock_utilities.first_version_is_lower = lambda a, b: False
sys.modules['grpc'] = mock_grpc
sys.modules['grpc._utilities'] = mock_utilities

from sonic_py_common.grpc.gnoi.testing import FakeGnoiClient, FakeSystemStub
from sonic_py_common.grpc.gnoi import system_pb2


class TestFakeSystemStub(unittest.TestCase):

    def test_reboot_default_success(self):
        stub = FakeSystemStub()
        req = system_pb2.RebootRequest(method=system_pb2.HALT)
        resp = stub.Reboot(req, timeout=60)
        self.assertIsNotNone(resp)
        self.assertEqual(len(stub.reboot_calls), 1)
        self.assertEqual(stub.reboot_calls[0]['timeout'], 60)

    def test_reboot_side_effect_exception(self):
        stub = FakeSystemStub()
        rpc_error = mock_grpc.RpcError()
        stub.set_reboot_response(side_effect=rpc_error)
        with self.assertRaises(mock_grpc.RpcError):
            stub.Reboot(system_pb2.RebootRequest(), timeout=10)

    def test_reboot_status_default_complete(self):
        stub = FakeSystemStub()
        resp = stub.RebootStatus(system_pb2.RebootStatusRequest(), timeout=10)
        self.assertFalse(resp.active)
        self.assertEqual(resp.status.status, system_pb2.RebootStatus.Status.STATUS_SUCCESS)

    def test_reboot_status_active(self):
        stub = FakeSystemStub()
        stub.set_reboot_status(active=True)
        resp = stub.RebootStatus(system_pb2.RebootStatusRequest())
        self.assertTrue(resp.active)

    def test_reboot_status_sequence(self):
        """Test list side_effect for polling scenarios."""
        stub = FakeSystemStub()
        active_resp = system_pb2.RebootStatusResponse()
        active_resp.active = True
        done_resp = system_pb2.RebootStatusResponse()
        done_resp.active = False
        done_resp.status.status = system_pb2.RebootStatus.Status.STATUS_SUCCESS

        stub.set_reboot_status(side_effect=[active_resp, active_resp, done_resp])

        r1 = stub.RebootStatus(system_pb2.RebootStatusRequest())
        self.assertTrue(r1.active)
        r2 = stub.RebootStatus(system_pb2.RebootStatusRequest())
        self.assertTrue(r2.active)
        r3 = stub.RebootStatus(system_pb2.RebootStatusRequest())
        self.assertFalse(r3.active)
        self.assertEqual(len(stub.reboot_status_calls), 3)

    def test_reboot_status_sequence_with_error(self):
        """Test mixed responses and errors in sequence."""
        stub = FakeSystemStub()
        rpc_error = mock_grpc.RpcError()
        done_resp = system_pb2.RebootStatusResponse()
        done_resp.active = False

        stub.set_reboot_status(side_effect=[rpc_error, done_resp])

        with self.assertRaises(mock_grpc.RpcError):
            stub.RebootStatus(system_pb2.RebootStatusRequest())
        r2 = stub.RebootStatus(system_pb2.RebootStatusRequest())
        self.assertFalse(r2.active)

    def test_cancel_reboot(self):
        stub = FakeSystemStub()
        resp = stub.CancelReboot(system_pb2.CancelRebootRequest(), timeout=30)
        self.assertIsNotNone(resp)
        self.assertEqual(len(stub.cancel_reboot_calls), 1)

    def test_call_history_tracks_requests(self):
        stub = FakeSystemStub()
        req = system_pb2.RebootRequest(method=system_pb2.HALT, message="test")
        stub.Reboot(req, timeout=60)
        self.assertEqual(stub.reboot_calls[0]['request'], req)


class TestFakeGnoiClient(unittest.TestCase):

    def test_context_manager(self):
        with FakeGnoiClient() as client:
            self.assertIsNotNone(client.system)

    def test_system_stub_persists(self):
        """Same stub instance across property accesses."""
        fake = FakeGnoiClient()
        self.assertIs(fake.system, fake.system)

    def test_end_to_end_reboot_flow(self):
        """Simulate a reboot + poll flow like gnoi_shutdown_daemon."""
        fake = FakeGnoiClient()

        # Configure: reboot succeeds, status shows active then done
        active_resp = system_pb2.RebootStatusResponse()
        active_resp.active = True
        done_resp = system_pb2.RebootStatusResponse()
        done_resp.active = False
        done_resp.status.status = system_pb2.RebootStatus.Status.STATUS_SUCCESS
        fake.system.set_reboot_status(side_effect=[active_resp, done_resp])

        with fake as client:
            # Send reboot
            client.system.Reboot(
                system_pb2.RebootRequest(method=system_pb2.HALT),
                timeout=60,
            )

            # Poll status
            r1 = client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=10)
            self.assertTrue(r1.active)
            r2 = client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=10)
            self.assertFalse(r2.active)

        self.assertEqual(len(fake.system.reboot_calls), 1)
        self.assertEqual(len(fake.system.reboot_status_calls), 2)

    def test_close_is_noop(self):
        fake = FakeGnoiClient()
        fake.close()  # Should not raise


if __name__ == '__main__':
    unittest.main()
