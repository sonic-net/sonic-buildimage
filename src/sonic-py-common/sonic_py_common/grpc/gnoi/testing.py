"""
Fake gNOI client for testing.

Provides a test double that behaves like GnoiClient but with
controllable responses and no real gRPC connections.

Usage:
    from sonic_py_common.grpc.gnoi.testing import FakeGnoiClient
    from sonic_py_common.grpc.gnoi import system_pb2

    fake = FakeGnoiClient()
    fake.system.set_reboot_status(active=False, status=system_pb2.RebootStatus.Status.STATUS_SUCCESS)

    with fake as client:
        resp = client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=10)
        assert not resp.active
"""

from unittest.mock import MagicMock
from sonic_py_common.grpc.gnoi import system_pb2


class FakeSystemStub:
    """Fake gNOI System service stub with controllable RPC responses.

    Each RPC can be configured with:
      - A return value (response proto)
      - A side_effect (exception or callable)
      - A call history for assertions
    """

    def __init__(self):
        self._reboot_response = system_pb2.RebootResponse()
        self._reboot_status_response = self._default_reboot_status()
        self._cancel_reboot_response = system_pb2.CancelRebootResponse()

        self._reboot_side_effect = None
        self._reboot_status_side_effect = None
        self._cancel_reboot_side_effect = None

        self.reboot_calls = []
        self.reboot_status_calls = []
        self.cancel_reboot_calls = []

    @staticmethod
    def _default_reboot_status():
        resp = system_pb2.RebootStatusResponse()
        resp.active = False
        resp.status.status = system_pb2.RebootStatus.Status.STATUS_SUCCESS
        return resp

    # ---- Configuration methods ----

    def set_reboot_response(self, response=None, side_effect=None):
        """Configure Reboot RPC response or side_effect (exception/callable)."""
        if response is not None:
            self._reboot_response = response
        self._reboot_side_effect = side_effect

    def set_reboot_status(self, active=None, status=None, message="",
                          response=None, side_effect=None):
        """Configure RebootStatus response fields or provide full response."""
        if response is not None:
            self._reboot_status_response = response
        else:
            if active is not None:
                self._reboot_status_response.active = active
            if status is not None:
                self._reboot_status_response.status.status = status
            if message:
                self._reboot_status_response.status.message = message
        self._reboot_status_side_effect = side_effect

    def set_cancel_reboot_response(self, response=None, side_effect=None):
        """Configure CancelReboot RPC response or side_effect."""
        if response is not None:
            self._cancel_reboot_response = response
        self._cancel_reboot_side_effect = side_effect

    # ---- RPC methods (match real SystemStub interface) ----

    def Reboot(self, request, timeout=None, metadata=None, credentials=None):
        self.reboot_calls.append({
            'request': request,
            'timeout': timeout,
        })
        if self._reboot_side_effect:
            if callable(self._reboot_side_effect) and not isinstance(self._reboot_side_effect, type):
                return self._reboot_side_effect(request, timeout=timeout)
            raise self._reboot_side_effect
        return self._reboot_response

    def RebootStatus(self, request, timeout=None, metadata=None, credentials=None):
        self.reboot_status_calls.append({
            'request': request,
            'timeout': timeout,
        })
        if self._reboot_status_side_effect:
            if isinstance(self._reboot_status_side_effect, list):
                effect = self._reboot_status_side_effect.pop(0)
                if isinstance(effect, Exception):
                    raise effect
                return effect
            if callable(self._reboot_status_side_effect) and not isinstance(self._reboot_status_side_effect, type):
                return self._reboot_status_side_effect(request, timeout=timeout)
            raise self._reboot_status_side_effect
        return self._reboot_status_response

    def CancelReboot(self, request, timeout=None, metadata=None, credentials=None):
        self.cancel_reboot_calls.append({
            'request': request,
            'timeout': timeout,
        })
        if self._cancel_reboot_side_effect:
            if callable(self._cancel_reboot_side_effect) and not isinstance(self._cancel_reboot_side_effect, type):
                return self._cancel_reboot_side_effect(request, timeout=timeout)
            raise self._cancel_reboot_side_effect
        return self._cancel_reboot_response


class FakeGnoiClient:
    """Fake GnoiClient for testing — drop-in replacement with no real gRPC.

    Supports context manager protocol and exposes configurable service stubs.

    Example:
        fake = FakeGnoiClient()
        fake.system.set_reboot_status(active=False)

        # Patch GnoiClient to return fake
        with patch('my_module.GnoiClient', return_value=fake):
            result = my_function_that_uses_gnoi()
    """

    def __init__(self, target="fake:0"):
        self._target = target
        self._system = FakeSystemStub()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        return False

    def close(self):
        pass

    @property
    def channel(self):
        return MagicMock()

    @property
    def system(self):
        return self._system
