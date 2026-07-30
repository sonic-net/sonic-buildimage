"""Generate the Python modules shipped by sonic-grpc."""

from importlib.metadata import version
import os
from pathlib import Path
import tempfile


GENERATOR_VERSION = "1.66.2"
PROTO_NAMES = ("types", "common", "system", "file")
PROJECT_ROOT = Path(__file__).resolve().parent
PROTO_ROOT = PROJECT_ROOT / "proto"
GNOI_ROOT = PROTO_ROOT / "github.com" / "openconfig" / "gnoi"
PACKAGE_ROOT = PROJECT_ROOT / "sonic_grpc" / "gnoi"

IMPORT_REPLACEMENTS = {
    "from github.com.openconfig.gnoi.types import types_pb2":
        "from sonic_grpc.gnoi import types_pb2",
    "from github.com.openconfig.gnoi.common import common_pb2":
        "from sonic_grpc.gnoi import common_pb2",
    "from github.com.openconfig.gnoi.system import system_pb2":
        "from sonic_grpc.gnoi import system_pb2",
    "from github.com.openconfig.gnoi.file import file_pb2":
        "from sonic_grpc.gnoi import file_pb2",
}


def _generated_sources(root, name):
    return (
        root / "github" / "com" / "openconfig" / "gnoi" / name /
            f"{name}_pb2.py",
        root / "github.com" / "openconfig" / "gnoi" / name /
            f"{name}_pb2_grpc.py",
    )


def _rewrite(source, name):
    content = source.read_text(encoding="utf-8")
    for upstream_import, package_import in IMPORT_REPLACEMENTS.items():
        content = content.replace(upstream_import, package_import)

    if "from github.com.openconfig.gnoi" in content:
        raise RuntimeError(f"unrewritten gNOI import in {source}")

    if source.name == f"{name}_pb2.py":
        upstream_module = f"github.com.openconfig.gnoi.{name}.{name}_pb2"
        package_module = f"sonic_grpc.gnoi.{name}_pb2"
        if content.count(upstream_module) != 1:
            raise RuntimeError(f"unexpected module identity in {source}")
        content = content.replace(upstream_module, package_module)

    return content


def _write_if_changed(destination, content):
    if destination.exists() and destination.read_text(encoding="utf-8") == content:
        os.chmod(destination, 0o644)
        return

    destination.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(
        dir=destination.parent,
        prefix=f".{destination.name}.",
    )
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8") as temporary_file:
            temporary_file.write(content)
        os.chmod(temporary_name, 0o644)
        os.replace(temporary_name, destination)
    except BaseException:
        if os.path.exists(temporary_name):
            os.unlink(temporary_name)
        raise


def generate(package_root=PACKAGE_ROOT):
    """Generate all gNOI Python modules into ``package_root``."""
    installed_version = version("grpcio-tools")
    if installed_version != GENERATOR_VERSION:
        raise RuntimeError(
            f"grpcio-tools {GENERATOR_VERSION} is required; found "
            f"{installed_version}"
        )

    import grpc_tools
    from grpc_tools import protoc

    package_root = Path(package_root)
    with tempfile.TemporaryDirectory() as temporary_directory:
        generated_root = Path(temporary_directory)
        arguments = [
            "grpc_tools.protoc",
            f"--proto_path=github.com/openconfig/gnoi={GNOI_ROOT}",
            f"--proto_path={Path(grpc_tools.__file__).parent / '_proto'}",
            f"--python_out={generated_root}",
            f"--grpc_python_out={generated_root}",
        ]
        arguments.extend(
            str(GNOI_ROOT / name / f"{name}.proto") for name in PROTO_NAMES
        )
        if protoc.main(arguments) != 0:
            raise RuntimeError("gNOI protobuf generation failed")

        sources = [
            source
            for name in PROTO_NAMES
            for source in _generated_sources(generated_root, name)
        ]
        missing = [str(source) for source in sources if not source.is_file()]
        if missing:
            raise RuntimeError(f"missing generated modules: {', '.join(missing)}")

        generated_files = sorted(generated_root.rglob("*_pb2*.py"))
        if generated_files != sorted(sources):
            raise RuntimeError("unexpected modules generated from gNOI protos")

        for name in PROTO_NAMES:
            for source in _generated_sources(generated_root, name):
                _write_if_changed(package_root / source.name, _rewrite(source, name))


if __name__ == "__main__":
    generate()
