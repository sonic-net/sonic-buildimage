"""Publish and verify the original AMD64 Python package outputs from the VS build."""
import argparse
from email.parser import BytesParser
import json
from pathlib import Path
import re
import shutil
import tarfile
import tempfile
import zipfile

from tools.bazel.ci.artifact_validation import sha


WHEELS = {
    "redis_dump_load-1.1-py3-none-any.whl": "@redis_dump_load//:wheel",
    "sonic_supervisord_utilities-1.0-py3-none-any.whl": "//src/sonic-supervisord-utilities:sonic_supervisord_utilities_wheel",
    "sonic_yang_mgmt-1.0-py3-none-any.whl": "//src/sonic-yang-mgmt:sonic_yang_mgmt_wheel",
    "sonic_containercfgd-1.0-py3-none-any.whl": "//src/sonic-containercfgd:sonic_containercfgd_wheel",
    "sonic_py_common-1.0-py3-none-any.whl": "//src/sonic-py-common:sonic_py_common_wheel",
    "sonic_config_engine-1.0-py3-none-any.whl": "//src/sonic-config-engine:sonic_config_engine_wheel",
}
YANG_TREE = "yang-models.tree.tar"
YANG_WHEEL = "sonic_yang_models-1.0-py3-none-any.whl"
YANG_RUNTIME = "yang_models_pkg.tar"
YANG = {
    YANG_WHEEL: "@sonic_yang_models//:yang_models_wheel",
    YANG_RUNTIME: "@sonic_yang_models//:yang_models_pkg",
    YANG_TREE: "@sonic_yang_models//:yang_models",
}
SCAPY_WHEEL = "scapy-2.6.1.dev0-py3-none-any.whl"
PDDF_WHEEL = "sonic_platform_pddf_common-1.0-py3-none-any.whl"
PDDF_OWNER = "//platform/pddf/platform-api-pddf-base:"
FRR_COMPONENTS = ("sonic-frr-mgmt-framework", "sonic-bgpcfgd")


def frr_outputs(component):
    owner = "//src/" + component + ":"
    distribution = component.replace("-", "_")
    return {
        distribution + "-1.0-py3-none-any.whl": owner + distribution + "_wheel",
        component + ".runtime.tar": owner + "runtime",
    }


FRR = {name: label for component in FRR_COMPONENTS for name, label in frr_outputs(component).items()}
DHCP = {
    "sonic_dhcp_utilities-1.0-py3-none-any.whl": "//src/sonic-dhcp-utilities:sonic_dhcp_utilities_wheel",
}
WHEEL_GROUPS = {"config-engine": WHEELS, "frr": FRR, "dhcp": DHCP, "yang-models": YANG}
PACKAGES = {**WHEELS, **FRR, **DHCP, **YANG, SCAPY_WHEEL: "@scapy//:wheel"}


def package_targets(pddf_enabled):
    if pddf_enabled:
        return {**PACKAGES, PDDF_WHEEL: PDDF_OWNER + "sonic_platform_pddf_common_wheel"}
    return PACKAGES


def publish(directory, revision, outputs, *, pddf_enabled=False):
    """Copy the top-level package outputs from one configured VS build."""
    directory.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=directory.parent) as work:
        staged = Path(work) / "packages"
        staged.mkdir()
        entries = {}
        for name, target in package_targets(pddf_enabled).items():
            dest = staged / name
            if name == YANG_TREE:
                # Bazel produces both this directory and the wheel in PrepareYangModels.
                # Retain the original tree as a third input for the wheel/layer comparison.
                with tarfile.open(dest, "w") as archive:
                    for model in sorted(outputs[target].iterdir()):
                        if not model.is_file() or model.is_symlink() or model.suffix != ".yang":
                            raise ValueError("Unexpected file in VS YANG model tree: " + model.name)
                        info = tarfile.TarInfo(model.name)
                        info.size = model.stat().st_size
                        with model.open("rb") as source:
                            archive.addfile(info, source)
            else:
                shutil.copyfile(outputs[target], dest)
            entries[name] = {"target": target, "bytes": dest.stat().st_size, "sha256": sha(dest)}
        manifest = {"schema": 2, "architecture": "amd64", "revision": revision,
                    "pddf_enabled": pddf_enabled, "packages": entries}
        (staged / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
        verify(staged, revision)
        if directory.exists():
            if verify(directory, revision) != manifest:
                raise ValueError("A different VS Python package bundle already exists")
        else:
            staged.rename(directory)


def verify(directory, revision):
    """Bind the complete, named payload to this checkout, architecture and original Bazel targets."""
    if not re.fullmatch(r"[0-9a-f]{40}", revision):
        raise ValueError("Expected a full Git commit")
    manifest_file = directory / "manifest.json"
    if manifest_file.is_symlink() or not manifest_file.is_file():
        raise ValueError("Missing regular VS package manifest")
    manifest = json.loads(manifest_file.read_text())
    if type(manifest.get("pddf_enabled")) is not bool:
        raise ValueError("Missing VS PDDF feature selection")
    packages = package_targets(manifest["pddf_enabled"])
    expected_files = {"manifest.json", *packages}
    if ({p.name for p in directory.iterdir()} != expected_files
            or any(p.is_symlink() or not p.is_file() for p in directory.iterdir())):
        raise ValueError("Unexpected VS package bundle files")
    if (manifest.get("schema") != 2 or manifest.get("architecture") != "amd64"
            or manifest.get("revision") != revision or set(manifest.get("packages", {})) != set(packages)):
        raise ValueError("Unexpected VS package manifest, architecture or revision")
    for name, target in packages.items():
        path = directory / name
        size = path.stat().st_size
        if not size or manifest["packages"][name] != {"target": target, "bytes": size, "sha256": sha(path)}:
            raise ValueError("VS package does not match its target, size or SHA-256: " + name)
    return manifest


def verify_portable(directory, names):
    """Reject native wheel tags or compiled payloads before reusing AMD64 outputs on ARM64."""
    def check_file(name, stream):
        if stream.read(8).startswith((b"\x7fELF", b"!<arch>\n")) or re.search(r"\.(?:so(?:\.\d+)*|a|o)$", name):
            raise ValueError("Native file in a portable Python package: " + name)

    for name in names:
        path = directory / name
        if name.endswith(".whl"):
            with zipfile.ZipFile(path) as wheel:
                records = [entry for entry in wheel.namelist() if entry.endswith(".dist-info/WHEEL")]
                if len(records) != 1 or not name.endswith("-py3-none-any.whl"):
                    raise ValueError("Expected a universal Python wheel: " + name)
                metadata = BytesParser().parsebytes(wheel.read(records[0]))
                if metadata.get("Root-Is-Purelib") != "true" or metadata.get_all("Tag") != ["py3-none-any"]:
                    raise ValueError("Expected a pure py3-none-any wheel: " + name)
                for entry in wheel.infolist():
                    if not entry.is_dir():
                        with wheel.open(entry) as stream:
                            check_file(entry.filename, stream)
        else:
            with tarfile.open(path) as archive:
                for entry in archive:
                    if entry.isdir():
                        continue
                    if not entry.isfile():
                        raise ValueError("Expected regular files in a portable Python TAR: " + entry.name)
                    with archive.extractfile(entry) as stream:
                        check_file(entry.name, stream)


def verify_frr_runtime(wheel, runtime_tar):
    """Compare the installed code, metadata and FRR templates in the original TAR with its wheel."""
    site = "usr/local/lib/python3.13/dist-packages/"
    with zipfile.ZipFile(wheel) as source, tarfile.open(runtime_tar) as runtime:
        files = {member.name.removeprefix("./"): member for member in runtime if member.isfile()}
        checked_code = False
        checked_metadata = False
        for entry in source.infolist():
            name = entry.filename
            if name.endswith(".py"):
                checked_code = True
                installed = site + name
            elif name.endswith(".dist-info/METADATA"):
                checked_metadata = True
                installed = site + name
            elif ".data/data/" in name and name.endswith(".j2"):
                installed = "usr/local/" + name.split(".data/data/", 1)[1]
            else:
                continue
            if installed not in files or runtime.extractfile(files[installed]).read() != source.read(entry):
                raise ValueError("FRR runtime TAR differs from its wheel: " + installed)
        if not checked_code or not checked_metadata:
            raise ValueError("FRR runtime check requires Python code and wheel metadata")


def package_path(path):
    """Return a Git-ignored Bazel package path inside the checkout's target directory."""
    if (not re.fullmatch(r"target/(?:[A-Za-z0-9_-][A-Za-z0-9_.-]*/)*[A-Za-z0-9_-][A-Za-z0-9_.-]*", str(path))
            or path.resolve() != Path.cwd() / path):
        raise ValueError("Expected a checkout-relative package under target/ without symlinks")
    return path


def stage(directory, revision, package):
    """Expose verified payloads as Bazel inputs. BUILD content comes from this checkout."""
    packages = verify(directory, revision)["packages"]
    package = package_path(package)
    if package.exists():
        raise ValueError("Expected a fresh Bazel package directory")
    package.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=package.parent) as work:
        staged = Path(work) / "inputs"
        staged.mkdir()
        for name in packages:
            shutil.copyfile(directory / name, staged / name)
        models = staged / "models"
        models.mkdir()
        with tarfile.open(directory / YANG_TREE) as archive:
            for member in archive:
                if not member.isfile() or not re.fullmatch(r"[A-Za-z0-9_.@-]+\.yang", member.name):
                    raise ValueError("Unexpected VS YANG model member: " + member.name)
                with (models / member.name).open("xb") as output:
                    shutil.copyfileobj(archive.extractfile(member), output)
        (staged / "BUILD.bazel").write_text(
            'load("@aspect_bazel_lib//lib:copy_to_directory.bzl", "copy_to_directory")\n'
            'exports_files(' + json.dumps(sorted(packages)) + ')\n'
            'copy_to_directory(name="test_models", srcs=glob(["models/*.yang"]), '
            'root_paths=[' + json.dumps(str(package / "models")) + '], visibility=["//visibility:public"])\n'
        )
        staged.rename(package)


def flags(group, package, architecture):
    """Print one source-or-VS input override per installed-wheel probe."""
    path = package_path(package)
    if group == "yang-models":
        if architecture == "arm64":
            verify_portable(path, YANG)
        for selector, source in (("test_models", "test_models"), ("test_wheel", YANG_WHEEL),
                                 ("test_layer", YANG_RUNTIME)):
            print(f"--//src/sonic-yang-models:{selector}=//{path}:{source}")
        return
    selected = WHEEL_GROUPS[group]
    if group == "config-engine":
        print(f"--@sonic_yang_models//:test_models=//{path}:test_models")
    if architecture == "arm64":
        verify_portable(path, selected)
    for name in selected:
        if name.endswith(".whl"):
            selector = name.split("-", 1)[0] + "_test_wheel"
            print(f"--//tools/bazel/tests:{selector}=//{path}:{name}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    for name in ("verify", "stage", "flags"):
        sub = commands.add_parser(name)
        if name != "flags":
            sub.add_argument("--directory", type=Path, required=True)
            sub.add_argument("--revision", required=True)
        if name != "verify":
            sub.add_argument("--package", type=Path, required=True)
        if name == "flags":
            sub.add_argument("--architecture", choices=("amd64", "arm64"), default="amd64")
            sub.add_argument("--group", choices=WHEEL_GROUPS, required=True)
    args = parser.parse_args()
    if args.command == "verify":
        verify(args.directory, args.revision)
    elif args.command == "stage":
        stage(args.directory, args.revision, args.package)
    else:
        flags(args.group, args.package, args.architecture)


if __name__ == "__main__":
    main()
