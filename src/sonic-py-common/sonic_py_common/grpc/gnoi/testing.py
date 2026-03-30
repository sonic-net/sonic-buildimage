"""
Fake gNOI gRPC server for testing.

Spins up a real gRPC server on localhost with controllable service
implementations. Tests use the real GnoiClient against this server —
no mocking of gRPC internals.

Usage:
    from sonic_py_common.grpc.gnoi.testing import FakeGnoiServer
    from sonic_py_common.grpc.gnoi import GnoiClient, system_pb2

    server = FakeGnoiServer()
    server.start()

    # Configure responses
    server.system.set_reboot_response()  # default success
    server.system.set_reboot_status(active=False, status=STATUS_SUCCESS)

    # Test with real client
    with GnoiClient(server.target) as client:
        client.system.Reboot(system_pb2.RebootRequest(method=system_pb2.HALT), timeout=5)
        resp = client.system.RebootStatus(system_pb2.RebootStatusRequest(), timeout=5)
        assert not resp.active

    server.stop()
"""

from concurrent import futures
import grpc

from sonic_py_common.grpc.gnoi import system_pb2
from sonic_py_common.grpc.gnoi import system_pb2_grpc


class FakeSystemServicer(system_pb2_grpc.SystemServicer):
    """Fake gNOI System servicer with configurable responses.

    Each RPC returns a configured response or raises a configured error.
    Supports response sequences for polling tests.
    Call history is recorded for assertions.
    """

    def __init__(self):
        self.reset()

    def reset(self):
        """Reset all configured responses and call history."""
        self._reboot_response = system_pb2.RebootResponse()
        self._reboot_error = None

        self._reboot_status_responses = None  # None = use single response
        self._reboot_status_response = self._default_status_response()
        self._reboot_status_error = None

        self._cancel_reboot_response = system_pb2.CancelRebootResponse()
        self._cancel_reboot_error = None

        self.reboot_calls = []
        self.reboot_status_calls = []
        self.cancel_reboot_calls = []

    @staticmethod
    def _default_status_response():
        resp = system_pb2.RebootStatusResponse()
        resp.active = False
        resp.status.status = system_pb2.RebootStatus.Status.STATUS_SUCCESS
        return resp

    # ---- Configuration ----

    def set_reboot_response(self, response=None, error_code=None, error_message=""):
        """Set Reboot RPC response or error.

        Args:
            response: RebootResponse proto (default: empty success).
            error_code: grpc.StatusCode to return as error (e.g. grpc.StatusCode.UNAVAILABLE).
            error_message: Error detail string.
        """
        if response is not None:
            self._reboot_response = response
        self._reboot_error = (error_code, error_message) if error_code else None

    def set_reboot_status(self, active=None, status=None, message="",
                          response=None, error_code=None, error_message=""):
        """Set RebootStatus single response.

        Args:
            active: Whether reboot is in progress.
            status: RebootStatus.Status enum value.
            message: Status message.
            response: Full RebootStatusResponse (overrides field args).
            error_code: grpc.StatusCode for error response.
            error_message: Error detail string.
        """
        self._reboot_status_responses = None
        if response is not None:
            self._reboot_status_response = response
        else:
            if active is not None:
                self._reboot_status_response.active = active
            if status is not None:
                self._reboot_status_response.status.status = status
            if message:
                self._reboot_status_response.status.message = message
        self._reboot_status_error = (error_code, error_message) if error_code else None

    def set_reboot_status_sequence(self, responses):
        """Set a sequence of RebootStatus responses for polling tests.

        Each call pops the next item. Items can be:
          - RebootStatusResponse proto → returned as success
          - (grpc.StatusCode, message) tuple → returned as error

        After exhaustion, falls back to the single response.

        Args:
            responses: List of RebootStatusResponse or (StatusCode, msg) tuples.
        """
        self._reboot_status_responses = list(responses)

    def set_cancel_reboot_response(self, response=None, error_code=None, error_message=""):
        """Set CancelReboot RPC response or error."""
        if response is not None:
            self._cancel_reboot_response = response
        self._cancel_reboot_error = (error_code, error_message) if error_code else None

    # ---- RPC implementations ----

    def Reboot(self, request, context):
        self.reboot_calls.append(request)
        if self._reboot_error:
            context.abort(self._reboot_error[0], self._reboot_error[1])
        return self._reboot_response

    def RebootStatus(self, request, context):
        self.reboot_status_calls.append(request)
        if self._reboot_status_responses:
            item = self._reboot_status_responses.pop(0)
            if isinstance(item, tuple):
                context.abort(item[0], item[1])
            return item
        if self._reboot_status_error:
            context.abort(self._reboot_status_error[0], self._reboot_status_error[1])
        return self._reboot_status_response

    def CancelReboot(self, request, context):
        self.cancel_reboot_calls.append(request)
        if self._cancel_reboot_error:
            context.abort(self._cancel_reboot_error[0], self._cancel_reboot_error[1])
        return self._cancel_reboot_response


class FakeGnoiServer:
    """Fake gNOI gRPC server for integration-style tests.

    Starts a real gRPC server on a random localhost port.
    Tests use GnoiClient(server.target) for real channel communication.

    Example:
        server = FakeGnoiServer()
        server.start()
        try:
            with GnoiClient(server.target) as client:
                client.system.Reboot(req, timeout=5)
            assert len(server.system.reboot_calls) == 1
        finally:
            server.stop()

    Or as context manager:
        with FakeGnoiServer() as server:
            with GnoiClient(server.target) as client:
                ...
    """

    def __init__(self, max_workers=2):
        self._max_workers = max_workers
        self._server = None
        self._port = None
        self.system = FakeSystemServicer()

    @property
    def target(self):
        """gRPC target string, e.g. 'localhost:50051'."""
        return f"localhost:{self._port}"

    def start(self):
        """Start the fake gRPC server on a random port."""
        self._server = grpc.server(futures.ThreadPoolExecutor(max_workers=self._max_workers))
        system_pb2_grpc.add_SystemServicer_to_server(self.system, self._server)
        self._port = self._server.add_insecure_port("localhost:0")
        self._server.start()
        return self

    def stop(self, grace=0):
        """Stop the server."""
        if self._server:
            self._server.stop(grace)
            self._server = None

    def reset(self):
        """Reset all service state (responses + call history)."""
        self.system.reset()

    def __enter__(self):
        return self.start()

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.stop()
        return False
