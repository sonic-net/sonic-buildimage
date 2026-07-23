"""
gNOI gRPC client for SONiC.

Manages a gRPC channel and provides access to gNOI service stubs.
All RPCs are accessed through service properties:

    with GnoiClient("10.0.0.1:8080") as client:
        client.system.Reboot(request, timeout=60)
        client.system.RebootStatus(request, timeout=10)

The client is service-agnostic — new gNOI services (Healthz, Cert,
File, OS, etc.) can be added as properties without modifying existing
code.
"""

import grpc
from sonic_py_common.grpc.gnoi import system_pb2_grpc


class GnoiClient:
    """gNOI gRPC client for SONiC components.

    Manages a single insecure gRPC channel and exposes gNOI service stubs
    as properties. Use as a context manager for automatic cleanup.

    Args:
        target: gRPC target address, e.g. "10.0.0.1:8080".
        options: Optional list of gRPC channel options.

    Example:
        with GnoiClient("10.0.0.1:8080") as client:
            req = system_pb2.RebootRequest(method=system_pb2.HALT)
            client.system.Reboot(req, timeout=60)
    """

    def __init__(self, target, options=None):
        self._target = target
        self._options = options
        self._channel = None

    def __enter__(self):
        self._channel = grpc.insecure_channel(self._target, options=self._options)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
        return False

    def close(self):
        """Close the gRPC channel."""
        if self._channel:
            self._channel.close()
            self._channel = None

    @property
    def channel(self):
        """The underlying gRPC channel for constructing custom stubs."""
        return self._channel

    # ---- gNOI service stubs ----

    @property
    def system(self):
        """gNOI System service stub (gnoi.system.System).

        Provides: Reboot, RebootStatus, CancelReboot, Time, Ping,
                  Traceroute, SwitchControlProcessor, etc.
        """
        return system_pb2_grpc.SystemStub(self._channel)

    # Future services can be added here:
    #
    # @property
    # def healthz(self):
    #     """gNOI Healthz service stub."""
    #     from sonic_py_common.grpc.gnoi import healthz_pb2_grpc
    #     return healthz_pb2_grpc.HealthzStub(self._channel)
    #
    # @property
    # def cert(self):
    #     """gNOI CertificateManagement service stub."""
    #     from sonic_py_common.grpc.gnoi import cert_pb2_grpc
    #     return cert_pb2_grpc.CertificateManagementStub(self._channel)
    #
    # @property
    # def file(self):
    #     """gNOI File service stub."""
    #     from sonic_py_common.grpc.gnoi import file_pb2_grpc
    #     return file_pb2_grpc.FileStub(self._channel)
