"""Compatibility checks for build-generated gNOI modules."""

from sonic_grpc.gnoi import common_pb2
from sonic_grpc.gnoi import common_pb2_grpc
from sonic_grpc.gnoi import file_pb2
from sonic_grpc.gnoi import file_pb2_grpc
from sonic_grpc.gnoi import system_pb2
from sonic_grpc.gnoi import system_pb2_grpc
from sonic_grpc.gnoi import types_pb2
from sonic_grpc.gnoi import types_pb2_grpc


def test_generated_modules_preserve_descriptor_names():
    assert types_pb2.DESCRIPTOR.name == "github.com/openconfig/gnoi/types/types.proto"
    assert common_pb2.DESCRIPTOR.name == "github.com/openconfig/gnoi/common/common.proto"
    assert system_pb2.DESCRIPTOR.name == "github.com/openconfig/gnoi/system/system.proto"
    assert file_pb2.DESCRIPTOR.name == "github.com/openconfig/gnoi/file/file.proto"


def test_generated_messages_use_packaged_module_names():
    assert types_pb2.Path.__module__ == "sonic_grpc.gnoi.types_pb2"
    assert common_pb2.RemoteDownload.__module__ == "sonic_grpc.gnoi.common_pb2"
    assert system_pb2.TimeRequest.__module__ == "sonic_grpc.gnoi.system_pb2"
    assert file_pb2.StatRequest.__module__ == "sonic_grpc.gnoi.file_pb2"


def test_all_generated_grpc_modules_import():
    assert types_pb2_grpc is not None
    assert common_pb2_grpc is not None
    assert system_pb2_grpc.SystemStub is not None
    assert file_pb2_grpc.FileStub is not None
