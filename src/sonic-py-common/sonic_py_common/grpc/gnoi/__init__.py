"""
sonic_py_common.grpc.gnoi - gNOI Python framework for SONiC.

Provides:
  - Vendored gNOI proto stubs (System, types, common)
  - GnoiClient: gRPC channel manager with service stub access

Usage:
    from sonic_py_common.grpc.gnoi import GnoiClient
    from sonic_py_common.grpc.gnoi import system_pb2

    with GnoiClient("10.0.0.1:8080") as client:
        request = system_pb2.RebootRequest(method=system_pb2.HALT)
        client.system.Reboot(request, timeout=60)
"""

from sonic_py_common.grpc.gnoi.client import GnoiClient

__all__ = ['GnoiClient']
