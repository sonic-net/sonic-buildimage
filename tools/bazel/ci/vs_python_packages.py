"""Publish and verify the original AMD64 Python package outputs from the VS build."""
import argparse
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
FRR_COMPONENTS = ("sonic-frr-mgmt-framework", "sonic-bgpcfgd")


def frr_outputs(component):
    owner = "//src/" + component + ":"
    distribution = component.replace("-", "_")
    return {
        distribution + "-1.0-py3-none-any.whl": owner + distribution + "_wheel",
        component + ".runtime.tar": owner + "runtime",
    }


FRR = {name: label for component in FRR_COMPONENTS for name, label in frr_outputs(component).items()}
PACKAGES = {**WHEELS, **FRR}


def publish(directory, revision, outputs):
    """Copy all ten top-level outputs from a single configured VS build."""
    directory.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=directory.parent) as work:
        staged = Path(work) / "packages"
        staged.mkdir()
        entries = {}
        for name, target in PACKAGES.items():
            dest = staged / name
            shutil.copyfile(outputs[target], dest)
            entries[name] = {"target": target, "bytes": dest.stat().st_size, "sha256": sha(dest)}
        manifest = {"schema": 1, "architecture": "amd64", "revision": revision, "packages": entries}
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
    expected_files = {"manifest.json", *PACKAGES}
    if ({p.name for p in directory.iterdir()} != expected_files
            or any(p.is_symlink() or not p.is_file() for p in directory.iterdir())):
        raise ValueError("Unexpected VS package bundle files")
    manifest = json.loads((directory / "manifest.json").read_text())
    if (manifest.get("schema") != 1 or manifest.get("architecture") != "amd64"
            or manifest.get("revision") != revision or set(manifest.get("packages", {})) != set(PACKAGES)):
        raise ValueError("Unexpected VS package manifest, architecture or revision")
    for name, target in PACKAGES.items():
        path = directory / name
        size = path.stat().st_size
        if not size or manifest["packages"][name] != {"target": target, "bytes": size, "sha256": sha(path)}:
            raise ValueError("VS package does not match its target, size or SHA-256: " + name)
    return manifest


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
    verify(directory, revision)
    package = package_path(package)
    if package.exists():
        raise ValueError("Expected a fresh Bazel package directory")
    package.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=package.parent) as work:
        staged = Path(work) / "inputs"
        staged.mkdir()
        for name in PACKAGES:
            shutil.copyfile(directory / name, staged / name)
        (staged / "BUILD.bazel").write_text('exports_files(' + json.dumps(sorted(PACKAGES)) + ')\n')
        staged.rename(package)


def flags(group, package):
    """Print one source-or-VS input override per installed-wheel probe."""
    path = package_path(package)
    for name in WHEELS if group == "config-engine" else FRR:
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
            sub.add_argument("--group", choices=("config-engine", "frr"), required=True)
    args = parser.parse_args()
    if args.command == "verify":
        verify(args.directory, args.revision)
    elif args.command == "stage":
        stage(args.directory, args.revision, args.package)
    else:
        flags(args.group, args.package)


if __name__ == "__main__":
    main()
