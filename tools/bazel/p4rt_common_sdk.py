#!/usr/bin/env python3
"""Build Common with Bazel 8 and prepare its binary/header input for Bazel 6."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import struct
import subprocess
import tempfile
import tarfile


BAZEL_VERSION = "8.5.1"
SDK_TARGET = "//dist:libswsscommon_sdk"
SDK_KIND = "sonic-swss-common-bazel-sdk"
TEMPLATE = Path(__file__).resolve().parents[2] / "src/sonic-p4rt/common-sdk.BUILD.bazel"
LIBNL_STEMS = ("libnl-genl-3", "libnl-nf-3", "libnl-route-3", "libnl-3")


def native_architecture():
    machines = {"x86_64": "amd64", "aarch64": "arm64"}
    try:
        return machines[platform.machine()]
    except KeyError as error:
        raise ValueError("P4RT Common SDK supports native AMD64 and ARM64") from error


def validate_sdk(directory, arch):
    """Reject incomplete or wrong-architecture inputs before replacing an SDK."""
    library = directory / "lib/libswsscommon.so.0"
    if library.is_symlink() or not library.is_file():
        raise ValueError("SDK must contain the real lib/libswsscommon.so.0")
    with library.open("rb") as stream:
        header = stream.read(20)
    if (len(header) < 20 or header[:6] != b"\x7fELF\x02\x01"
            or struct.unpack_from("<H", header, 18)[0] != {"amd64": 62, "arm64": 183}[arch]):
        raise ValueError(f"SDK library must be a little-endian {arch} ELF")
    for name in ("dbconnector.h", "cfg_schema.h"):
        if not (directory / "include/swss" / name).is_file():
            raise ValueError(f"SDK is missing include/swss/{name}")
    if any(directory.rglob("*.cpp")) or any(directory.rglob("*.cc")):
        raise ValueError("SDK must contain headers and libraries, not Common sources")


def prepare_libnl(staging, archives, parent_archive, arch):
    """Import the existing libnl owner's SDKs, bound to the selected image bytes."""
    from tools.bazel.ci import artifact_validation as check

    check.require(arch == "amd64" and parent_archive is not None,
                  "Declared libnl SDK handoff requires an AMD64 image parent")
    selected, sources = {}, {}
    prefix = "usr/lib/x86_64-linux-gnu/"
    runtime_paths = {prefix + stem + suffix for stem in LIBNL_STEMS
                     for suffix in (".so.200", ".so.200.26.0")}
    development_paths = {prefix + stem + ".so" for stem in LIBNL_STEMS}
    paths = runtime_paths | development_paths
    for target, archive in archives.items():
        # Aggregate SDK archives may have producer UID metadata. Installed image
        # ownership is checked on the parent; SDK files are private build inputs.
        inventory = check.payload(archive, require_root=False)
        with tarfile.open(archive) as package:
            for member in package:
                name = check.path_name(member.name)
                if name not in paths and not name.startswith("usr/include/libnl3/"):
                    continue
                item = inventory[name]
                if item["kind"] == "directory":
                    continue
                check.require(item["kind"] in ("file", "symlink"), "Unsupported libnl SDK member: " + name)
                if name in selected:
                    check.require(item == selected[name], "Conflicting libnl SDK member: " + name)
                    continue
                selected[name] = item
                relative = name.removeprefix(prefix) if name.startswith(prefix) else name.removeprefix("usr/")
                destination = staging / ("lib/" + relative if name.startswith(prefix) else relative)
                check.require(not destination.exists() and not destination.is_symlink(),
                              "SDK input already exists: " + str(destination))
                destination.parent.mkdir(parents=True, exist_ok=True)
                if member.issym():
                    # Flattened owner archives may add a harmless './' prefix.
                    # metadata() normalizes that prefix without resolving '..'.
                    development = name in development_paths
                    suffix = ".200" if development else ".26.0"
                    check.require(name in paths and item["linkname"] == destination.name + suffix,
                                  "Unexpected libnl " + ("development" if development else "SONAME") +
                                  " link: " + name)
                    destination.symlink_to(item["linkname"])
                else:
                    with package.extractfile(member) as source, destination.open("wb") as output:
                        shutil.copyfileobj(source, output)
                    destination.chmod(member.mode)
        sources[target] = check.sha(archive)
    check.require(paths <= selected.keys() and
                  selected.get("usr/include/libnl3/netlink/netlink.h", {}).get("kind") == "file",
                  "Incomplete libnl SDK: all four libraries and headers are required")
    parent = check.payload(parent_archive)
    for name in paths:
        item = selected[name]
        check.require(item["kind"] == ("file" if name.endswith(".so.200.26.0") else "symlink"),
                      "Unexpected libnl SDK library kind: " + name)
        # Development aliases are declared SDK inputs, not installed runtime files.
        # Their exact relative targets lead to the parent-bound SONAME and bytes.
        if name in development_paths:
            continue
        if item["kind"] == "file":
            check.require(item.get("elf_machine") == 62 and
                          parent.get(name, {}).get("sha256") == item["sha256"],
                          "libnl SDK differs from the selected parent runtime: " + name)
        else:
            check.require(parent.get(name, {}).get("kind") == "symlink" and
                          parent[name]["linkname"] == item["linkname"],
                          "libnl SDK SONAME differs from the selected parent: " + name)
    return {"producer_targets": sources, "parent_source_tar_sha256": check.sha(parent_archive),
            "files": selected}


def prepare_libyang(staging, archive, parent_archive, arch):
    """Import the existing runtime TAR's SONAME and exact selected parent bytes."""
    from tools.bazel.ci import artifact_validation as check

    check.require(arch == "amd64" and parent_archive is not None,
                  "Declared libyang SDK handoff requires an AMD64 image parent")
    prefix = "usr/lib/x86_64-linux-gnu/"
    library, soname = prefix + "libyang.so.3.9.1", prefix + "libyang.so.3"
    inventory, parent = check.payload(archive), check.payload(parent_archive)
    item, link = inventory.get(library, {}), inventory.get(soname, {})
    check.require(item.get("kind") == "file" and item.get("elf_machine") == 62,
                  "libyang SDK requires the real AMD64 libyang.so.3.9.1")
    check.require(parent.get(library, {}).get("kind") == "file" and
                  parent[library].get("sha256") == item["sha256"],
                  "libyang SDK differs from the selected parent runtime")
    check.require(link.get("kind") == "symlink" and link.get("linkname") == "libyang.so.3.9.1" and
                  parent.get(soname, {}).get("kind") == "symlink" and
                  parent[soname].get("linkname") == link["linkname"],
                  "libyang SDK SONAME differs from the selected parent")
    with tarfile.open(archive) as package:
        for member in package:
            name = check.path_name(member.name)
            if name not in (library, soname):
                continue
            destination = staging / "lib" / name.removeprefix(prefix)
            check.require(not destination.exists() and not destination.is_symlink(),
                          "SDK input already exists: " + str(destination))
            destination.parent.mkdir(parents=True, exist_ok=True)
            if member.issym():
                destination.symlink_to(link["linkname"])
            else:
                with package.extractfile(member) as source, destination.open("wb") as output:
                    shutil.copyfileobj(source, output)
                destination.chmod(member.mode)
    return {"producer_targets": {"@libyang//:libyang_pkg": check.sha(archive)},
            "parent_source_tar_sha256": check.sha(parent_archive),
            "files": {name: inventory[name] for name in (library, soname)}}


def prepare_repository(archive, output_dir, arch, receipt, *, libnl_archives=None,
                       libyang_archive=None, parent_archive=None):
    """Use a fresh directory so removed producer headers cannot remain cached."""
    output_dir = output_dir.resolve()
    if output_dir.exists():
        marker = output_dir / "sdk-build.json"
        if not marker.is_file() or json.loads(marker.read_text()).get("kind") != SDK_KIND:
            raise ValueError(f"Refusing to replace a directory not generated by this tool: {output_dir}")
    output_dir.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".common-sdk-", dir=output_dir.parent) as temporary:
        staging = Path(temporary) / "repository"
        staging.mkdir()
        with tarfile.open(archive) as package:
            package.extractall(staging, filter="data")
        validate_sdk(staging, arch)
        libnl = prepare_libnl(staging, libnl_archives, parent_archive, arch) if libnl_archives else None
        libyang = prepare_libyang(staging, libyang_archive, parent_archive, arch) if libyang_archive else None
        generated = "# Auto-generated by tools/bazel/p4rt_common_sdk.py; do not edit manually.\n"
        (staging / "WORKSPACE.bazel").write_text(generated + 'workspace(name = "sonic_swss_common")\n')
        template = TEMPLATE.read_text()
        if libnl is not None:
            if template.count("_DECLARED_LIBNL = False") != 1:
                raise ValueError("SDK template must explicitly select the libnl dependency contract")
            template = template.replace("_DECLARED_LIBNL = False", "_DECLARED_LIBNL = True")
        if libyang is not None:
            if template.count("_DECLARED_LIBYANG = False") != 1:
                raise ValueError("SDK template must explicitly select the libyang dependency contract")
            template = template.replace("_DECLARED_LIBYANG = False", "_DECLARED_LIBYANG = True")
        (staging / "BUILD.bazel").write_text(generated + template)
        (staging / "sdk-build.json").write_text(json.dumps({
            "_generated": "Auto-generated by tools/bazel/p4rt_common_sdk.py; do not edit manually.",
            "kind": SDK_KIND,
            "architecture": arch,
            "archive_sha256": hashlib.sha256(archive.read_bytes()).hexdigest(),
            **receipt,
            **({"libnl": libnl} if libnl is not None else {}),
            **({"libyang": libyang} if libyang is not None else {}),
        }, indent=2, sort_keys=True) + "\n")
        if output_dir.exists():
            shutil.rmtree(output_dir)
        staging.rename(output_dir)


def build_sdk(common_source, output_dir, arch, bazel="bazel", output_base=None, bazel_args=()):
    common_source = common_source.resolve()
    output_dir = output_dir.resolve()
    output_base = (output_base or output_dir.parent / f".bazel-common-output-{arch}").resolve()
    if not (common_source / "MODULE.bazel").is_file():
        raise ValueError(f"Not a Common Bazel source checkout: {common_source}")
    if arch != native_architecture():
        raise ValueError("The P4RT SDK handoff requires a native build for the requested architecture")
    environment = dict(os.environ, USE_BAZEL_VERSION=BAZEL_VERSION)
    command = [bazel, f"--output_base={output_base}"]
    host_cpu = "aarch64" if arch == "arm64" else "x86_64"
    options = [
        *bazel_args,
        *(["--config=aarch64"] if arch == "arm64" else []),
        f"--host_platform=@sonic_build_infra//platforms:{host_cpu}_trixie",
        "--//tools/bazel:yang_modules=True",
    ]

    def run(*arguments, capture=False):
        result = subprocess.run(command + list(arguments), cwd=common_source, env=environment,
                                check=True, text=True, stdout=subprocess.PIPE if capture else None)
        return result.stdout.strip() if capture else None

    try:
        version = run("version", "--gnu_format", capture=True)
        # Bazelisk prints its own banner before Bazel's GNU version line.
        if [line for line in version.splitlines() if line.startswith("bazel ")] != [f"bazel {BAZEL_VERSION}"]:
            raise ValueError(f"Common must build with Bazel {BAZEL_VERSION}; got {version!r}")
        # Always let Bazel check the source graph. An existing SDK is not a stamp
        # proving that headers, generated schema, flags or dependencies are current.
        run("build", *options, SDK_TARGET)
        files = run("cquery", *options, SDK_TARGET, "--output=files", capture=True).splitlines()
        if len(files) != 1:
            raise ValueError(f"{SDK_TARGET} must produce exactly one SDK archive")
        # Another build can repoint the checkout's bazel-out convenience link.
        # Resolve against this private server's execution root instead.
        # Bazel 8.5.1 can misresolve root-module platform aliases for info.
        # This location-only query needs neither platforms nor build settings.
        execution_root = Path(run("info", "--platforms=", "execution_root", capture=True))
        archive = execution_root / files[0]
        source_commit = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=common_source, text=True).strip()
        source_dirty = bool(subprocess.check_output(
            ["git", "status", "--porcelain", "--untracked-files=no"], cwd=common_source, text=True).strip())
        prepare_repository(archive, output_dir, arch, {
            "producer_bazel_version": BAZEL_VERSION,
            "producer_target": SDK_TARGET,
            "producer_options": options,
            "common_source_commit": source_commit,
            "common_source_has_tracked_changes": source_dirty,
            "yang_modules": True,
        })
    finally:
        # This private producer server is independent of the Bazel 6 P4RT server.
        subprocess.run(command + ["shutdown"], cwd=common_source, env=environment, check=False)
    return output_dir


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--common-source", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--arch", choices=("amd64", "arm64"), default=native_architecture())
    parser.add_argument("--bazel", default="bazel", help="Bazelisk or the Bazel 8.5.1 executable")
    parser.add_argument("--output-base", type=Path)
    parser.add_argument("--bazel-arg", action="append", default=[], help="Additional Common Bazel build option")
    arguments = parser.parse_args()
    output = build_sdk(arguments.common_source, arguments.output_dir, arguments.arch,
                       arguments.bazel, arguments.output_base, arguments.bazel_arg)
    print(f"Prepared Bazel {BAZEL_VERSION} Common SDK for P4RT: {output}")


if __name__ == "__main__":
    main()
