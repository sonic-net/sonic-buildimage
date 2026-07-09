"""
sonic_grpc.gnoi - gNOI Python framework for SONiC.

Provides:
  - Vendored gNOI proto stubs (System, File, types, common)
  - GnoiClient: gRPC channel manager with service stub access

Usage:
    from sonic_grpc.gnoi import GnoiClient
    from sonic_grpc.gnoi import system_pb2, file_pb2

    with GnoiClient("10.0.0.1:8080") as client:
        request = system_pb2.RebootRequest(method=system_pb2.HALT)
        client.system.Reboot(request, timeout=60)
        client.file.Stat(file_pb2.StatRequest(path="/etc/hostname"))
"""

from sonic_grpc.gnoi.client import GnoiClient

__all__ = ['GnoiClient']
