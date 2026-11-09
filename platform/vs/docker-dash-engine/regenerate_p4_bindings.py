"""Regenerate installed P4 bindings without changing their protobuf schemas."""

from __future__ import annotations

import subprocess
import sysconfig
import tempfile
from pathlib import Path

from google.protobuf.descriptor import FileDescriptor
from google.protobuf.descriptor_pb2 import FileDescriptorSet
from p4.bm import dataplane_interface_pb2
from p4.config.v1 import p4info_pb2, p4types_pb2
from p4.server.v1 import config_pb2
from p4.tmp import p4config_pb2
from p4.v1 import p4data_pb2, p4runtime_pb2


def add_descriptor(
    descriptor: FileDescriptor,
    descriptor_set: FileDescriptorSet,
    seen: set[str],
) -> None:
    if descriptor.name in seen:
        return
    for dependency in descriptor.dependencies:
        add_descriptor(dependency, descriptor_set, seen)
    descriptor.CopyToProto(descriptor_set.file.add())
    seen.add(descriptor.name)


def main() -> None:
    descriptors: tuple[FileDescriptor, ...] = (
        dataplane_interface_pb2.DESCRIPTOR,
        p4info_pb2.DESCRIPTOR,
        p4types_pb2.DESCRIPTOR,
        config_pb2.DESCRIPTOR,
        p4config_pb2.DESCRIPTOR,
        p4data_pb2.DESCRIPTOR,
        p4runtime_pb2.DESCRIPTOR,
    )

    descriptor_set = FileDescriptorSet()
    seen: set[str] = set()
    for descriptor in descriptors:
        add_descriptor(descriptor, descriptor_set, seen)

    package_path = Path(sysconfig.get_path("purelib")).relative_to("/")
    output_path = Path("/p4-bindings") / package_path
    output_path.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="p4-codegen-") as temporary_path:
        descriptor_path = Path(temporary_path) / "p4-descriptors.pb"
        descriptor_path.write_bytes(descriptor_set.SerializeToString())
        subprocess.run(
            [
                "/usr/bin/protoc",
                f"--descriptor_set_in={descriptor_path}",
                f"--python_out={output_path}",
                *sorted(descriptor.name for descriptor in descriptors),
            ],
            check=True,
        )
    print(f"Regenerated {len(descriptors)} P4 protobuf bindings")


if __name__ == "__main__":
    main()
