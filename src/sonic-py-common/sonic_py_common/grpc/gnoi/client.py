"""
gNOI gRPC client for SONiC.

Manages a gRPC channel and provides access to gNOI service stubs.
All RPCs are accessed through service properties:

    with GnoiClient("10.0.0.1:8080") as client:
        client.system.Reboot(request, timeout=60)
        client.system.RebootStatus(request, timeout=10)

For Unix-domain-socket targets (e.g. the local gNMI/gNOI server's
``/var/run/gnmi/gnmi.sock``), use the ``unix://`` scheme; no TLS is
needed for loopback IPC:

    with GnoiClient("unix:///var/run/gnmi/gnmi.sock") as client:
        client.system.Time(system_pb2.TimeRequest(), timeout=5)

For mTLS targets (e.g. the SONiC telemetry/gNOI server), pass a
grpc.ChannelCredentials built from the relevant cert files:

    import grpc
    creds = grpc.ssl_channel_credentials(
        root_certificates=open("/etc/sonic/telemetry/streamingtelemetryserver.cer", "rb").read(),
        private_key=open("/etc/sonic/telemetry/dsmsroot.key", "rb").read(),
        certificate_chain=open("/etc/sonic/telemetry/dsmsroot.cer", "rb").read(),
    )
    options = (("grpc.ssl_target_name_override", "ndastreamingservertest"),)
    with GnoiClient("127.0.0.1:50052", credentials=creds, options=options) as client:
        client.system.Time(system_pb2.TimeRequest(), timeout=5)

The client is service-agnostic — new gNOI services (Healthz, Cert,
File, OS, etc.) can be added as properties without modifying existing
code.
"""

import grpc
from sonic_py_common.grpc.gnoi import system_pb2_grpc


class GnoiClient:
    """gNOI gRPC client for SONiC components.

    Manages a single gRPC channel and exposes gNOI service stubs as
    properties. Use as a context manager for automatic cleanup.

    Args:
        target: gRPC target address, e.g. "10.0.0.1:8080".
        options: Optional list of gRPC channel options, e.g.
            ``(("grpc.ssl_target_name_override", "server-cn"),)``.
        credentials: Optional ``grpc.ChannelCredentials``. When ``None``
            (default), an insecure channel is opened — suitable for
            localhost loopback or test fakes. When supplied, a secure
            channel is opened; build credentials with
            ``grpc.ssl_channel_credentials(...)``.

    Example (insecure, for FakeGnoiServer or a localhost helper):
        with GnoiClient("localhost:50051") as client:
            client.system.Reboot(req, timeout=60)

    Example (Unix domain socket, no TLS):
        with GnoiClient("unix:///var/run/gnmi/gnmi.sock") as client:
            client.system.Time(system_pb2.TimeRequest(), timeout=5)

    Example (mTLS):
        creds = grpc.ssl_channel_credentials(
            root_certificates=server_ca_pem,
            private_key=client_key_pem,
            certificate_chain=client_cert_pem,
        )
        with GnoiClient("dut:50052", credentials=creds) as client:
            client.system.Time(system_pb2.TimeRequest(), timeout=5)
    """

    def __init__(self, target, options=None, credentials=None):
        self._target = target
        self._options = options
        self._credentials = credentials
        self._channel = None

    def __enter__(self):
        if self._credentials is None:
            self._channel = grpc.insecure_channel(
                self._target, options=self._options
            )
        else:
            self._channel = grpc.secure_channel(
                self._target, self._credentials, options=self._options
            )
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
