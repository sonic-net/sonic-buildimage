"""Regenerate installed P4 bindings without changing their protobuf schemas."""

from __future__ import annotations

import importlib
import pkgutil
import subprocess
import sysconfig
import tempfile
from pathlib import Path

import p4
from google.protobuf.descriptor import FileDescriptor
from google.protobuf.descriptor_pb2 import FileDescriptorSet


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
    descriptors: dict[str, FileDescriptor] = {}
    for module_info in pkgutil.walk_packages(p4.__path__, prefix="p4."):
        if not module_info.name.endswith("_pb2"):
            continue
        descriptor = importlib.import_module(module_info.name).DESCRIPTOR
        if not isinstance(descriptor, FileDescriptor):
            raise TypeError(f"Invalid protobuf descriptor in {module_info.name}")
        descriptors[descriptor.name] = descriptor
    if not descriptors:
        raise RuntimeError("No installed P4 protobuf bindings were found")

    descriptor_set = FileDescriptorSet()
    seen: set[str] = set()
    for descriptor in descriptors.values():
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
                *sorted(descriptors),
            ],
            check=True,
        )
    print(f"Regenerated {len(descriptors)} P4 protobuf bindings")


if __name__ == "__main__":
    main()
