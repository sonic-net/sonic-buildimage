#!/usr/bin/env python3
"""Build and inspect the native AMD64 shared SWSS OCI image from owner TARs."""

import argparse
from functools import partial
import json
from pathlib import Path
import platform
import re
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as checks
from tools.bazel.ci import base_images, bazel_commands, build, command_log, resolution
from tools.bazel.oci import image_validation, oci_layout, package_contract

OWNER = "//dockers/docker-swss-layer-trixie:"
IMAGE = OWNER + "docker-swss-layer-trixie"
TARGETS = {
    "docker-swss-layer-trixie.gz": IMAGE + ".gz",
    "source-runtime.tar": OWNER + "source_runtime",
    "swss-runtime.tar": "@sonic_swss//dist:swss_pkg",
    "source-debug.tar": OWNER + "source_debug_symbols",
    "image-debug.tar": IMAGE + ".debug_symbols",
    "apt-selection.json": OWNER + "runtime_apt_selection",
    "packages.json": OWNER + "runtime_package_metadata",
    "installed.status": OWNER + "installed_packages",
    "swss-policy.json": "//dockers/docker-orchagent:package_policy",
    "config-engine-debug.tar": "//dockers/docker-config-engine-trixie:docker-config-engine-trixie.debug_symbols",
}


def verify_source_runtime(files, archive):
    """Compare installed semantics while retaining original owner TAR metadata."""
    runtime = checks.payload(archive)
    installed = {}
    # Use the same layer interpretation as the final OCI inventory. For example,
    # Linux installs protobuf's mode-0000 TAR symlink with mode 0777; its target
    # and ownership, and every regular file's bytes/mode, must still match.
    image_validation.apply_layer(archive, installed)
    native = {}
    for name, item in runtime.items():
        if item["kind"] == "directory":
            continue
        actual = files.get(name, {})
        checks.require(all(actual.get(key) == value for key, value in installed[name].items()),
                       "OCI changed source-owned payload: " + name)
        if "elf_machine" in item:
            checks.require(item["elf_machine"] == 62, "non-AMD64 source ELF: " + name)
            native[name] = item
    checks.require(native, "shared layer has no native source payload")
    return runtime, native


def verify_libteam_metadata(metadata, native):
    """Bind the two installed libraries to the selected owner and native bytes."""
    expected = {
        # The selected Bazel 8.5.1 metadata producer emits canonical labels.
        # Retain the exact apparent forms without accepting other repositories.
        "libteam5": (("@libteam//:libteam_runtime", "@@libteam+//:libteam_runtime"),
                     "usr/lib/x86_64-linux-gnu/libteam.so.5.6.1"),
        "libteamdctl0": (("@libteam//:libteamdctl_runtime", "@@libteam+//:libteamdctl_runtime"),
                        "usr/lib/x86_64-linux-gnu/libteamdctl.so.0.1.5"),
    }
    checks.require(metadata.get("schema") == 1 and metadata.get("architecture") == "amd64",
                   "libteam package metadata schema or architecture differs")
    owner = metadata.get("source_modules", {}).get("libteam", {})
    checks.require(owner.get("version") == "1.31.0.sonic.1" and
                   re.fullmatch(r"[0-9a-f]{64}", owner.get("module_sha256", "")),
                   "libteam package metadata lacks the selected owner identity")
    records = metadata.get("source_packages", [])
    checks.require(len(records) == len(expected) and
                   {item.get("package") for item in records} == set(expected),
                   "shared layer must record exactly its two source libteam libraries")
    for item in records:
        targets, path = expected[item["package"]]
        checks.require(item.get("target") in targets and
                       item.get("source_revision") == owner["version"] and
                       re.fullmatch(r"[0-9a-f]{64}", item.get("payload_sha256", "")),
                       "libteam package provenance differs: " + item["package"])
        files = item.get("native_files", [])
        checks.require(len(files) == 1 and files[0].get("path") == path and path in native,
                       "libteam package native inventory differs: " + item["package"])
        checks.require(all(files[0].get(key) == native[path].get(key)
                           for key in ("sha256", "mode", "uid", "gid")),
                       "libteam package metadata changed native bytes or ownership: " + path)
    return {"owner": owner, "packages": records}


def verify(layout, parent, paths, source):
    base_images.verify_unique_layer_paths(layout)
    files = base_images.installed_inventory(layout)
    inherited = base_images.installed_inventory(parent)
    base_images.verify_directory_aliases(files)
    runtime, native = verify_source_runtime(files, paths["source-runtime.tar"])
    # A child must preserve parent configuration, accounts and libraries. The
    # source TAR does not repeat inherited Common files.
    changed_parent = []
    for name, item in inherited.items():
        if item["kind"] == "directory" or name == "var/lib/dpkg/status":
            continue
        if files.get(name) != item:
            changed_parent.append(name)
    checks.require(not changed_parent, "SWSS layer changed inherited payload: " + repr(changed_parent))
    for installed, generated in (("var/lib/dpkg/status", "installed.status"),
                                 ("usr/share/sonic/bazel/swss-layer-packages.json", "packages.json")):
        checks.require(files.get(installed, {}).get("sha256") == checks.sha(paths[generated]),
                       "OCI package metadata differs from its generated input: " + installed)
    libteam = verify_libteam_metadata(json.loads(paths["packages.json"].read_bytes()), native)
    policy = package_contract.read_policy(paths["swss-policy.json"])
    swss = checks.payload(paths["swss-runtime.tar"])
    programs = package_contract.validate_install_inventory(source, swss, policy["install_inventory"])
    checks.require(all(runtime.get(name) == item for name, item in swss.items()
                       if item["kind"] != "directory"), "shared TAR changed SWSS owner bytes")
    debug = checks.debug_archives(paths["source-runtime.tar"], paths["source-debug.tar"], expected_machine=62)
    pairs = {record["path"] for record in debug["debug_pairs"]}
    checks.require(pairs == set(native), "source runtime/debug coverage differs")
    checks.require(all(record["debug_path"] not in files for record in debug["debug_pairs"]),
                   "runtime image unexpectedly contains its debug extension")
    image_debug = checks.payload(paths["image-debug.tar"])
    for archive in ("source-debug.tar", "config-engine-debug.tar"):
        for name, item in checks.payload(paths[archive]).items():
            checks.require(image_debug.get(name) == item, "image lost matching inherited/source debug file: " + name)
    for name in ("usr/lib/x86_64-linux-gnu/libteam.so.5.6.1",
                 "usr/lib/x86_64-linux-gnu/libteamdctl.so.0.1.5"):
        checks.require(name in native, "patched libteam is absent: " + name)
    selection = json.loads(paths["apt-selection.json"].read_bytes())
    checks.require(not {"libteam5", "libteamdctl0"}.intersection(item["package"] for item in selection["selected"]),
                   "APT replaced a source libteam library")
    config = layout.config.get("config", {})
    checks.require(config.get("Entrypoint") == ["/usr/local/bin/supervisord"], "shared-layer entrypoint changed")
    checks.require("DEBIAN_FRONTEND=noninteractive" in config.get("Env", []), "noninteractive environment missing")
    archive_receipt = image_validation.docker_archive(
        paths["docker-swss-layer-trixie.gz"], "docker-swss-layer-trixie:latest",
        layout.manifest["config"]["digest"], layout.manifest["layers"])
    elfs = []
    with tempfile.TemporaryDirectory(prefix="swss-layer-elf-") as temporary:
        root = Path(temporary)
        with tarfile.open(paths["source-runtime.tar"]) as archive:
            archive.extractall(root, filter="data")
        for name in sorted(native):
            info = checks.elf_info(root / name, dynamic=True)
            elfs.append({"path": name, "needed": info["needed"], "sha256": native[name]["sha256"]})
    return {
        "status": "passed", "architecture": "amd64", "manifest_digest": layout.descriptor["digest"],
        "parent_manifest_digest": parent.descriptor["digest"], "rootfs": layout.config["rootfs"],
        "archive": archive_receipt,
        "config_digest": layout.manifest["config"]["digest"], "native": native, "swss_programs": programs,
        "source_debug_pairs": debug["debug_pairs"], "source_runtime_sha256": checks.sha(paths["source-runtime.tar"]),
        "parent_payload_unchanged": True, "elfs": elfs, "libteam": libteam,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bazel", required=True)
    parser.add_argument("--artifacts", required=True, type=Path)
    args = parser.parse_args()
    checks.require(platform.machine() == "x86_64" and
                   platform.freedesktop_os_release().get("VERSION_CODENAME") == "trixie",
                   "shared SWSS validation requires native AMD64 Debian Trixie")
    directory = args.artifacts.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    receipt = {"status": "running", "architecture": "amd64", "commands": []}
    execute = partial(command_log.execute, cwd=ROOT)
    options = ["--jobs=2", "--local_resources=cpu=2", "--local_resources=memory=10000"]
    parent_target = "//dockers/docker-config-engine-trixie:docker-config-engine-trixie"
    try:
        receipt["revision"] = execute(["git", "rev-parse", "HEAD"], directory, receipt, "revision").strip()
        execute(["make", "-f", "tools/bazel/prepare_manifests.mk",
                 "MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk rules/docker-swss-layer-trixie.mk",
                 "BUILD_WITH_BAZEL_WHEN_AVAILABLE=y", "BLDENV=trixie", "CONFIGURED_PLATFORM=vs",
                 "CONFIGURED_ARCH=amd64", "INCLUDE_FIPS=y", "DOCKERS_PATH=dockers"],
                directory, receipt, "manifests")
        receipt["action_audit"] = bazel_commands.audit_targets(
            args.bazel, options, [*TARGETS.values(), IMAGE, parent_target], workspace=ROOT,
            output=directory / "action-audit.json")
        paths = build.collect_archives(ROOT, directory, receipt, TARGETS, bazel=args.bazel, options=options)
        execute([args.bazel, "build", *options, IMAGE, parent_target], directory, receipt, "layouts")

        def output(target):
            return execute([args.bazel, "cquery", *options, target, "--output=files"],
                           directory, receipt, "query-" + str(len(receipt["commands"]))).strip()

        layout = oci_layout.validate_layout(ROOT / output(IMAGE), "linux/amd64")
        parent = oci_layout.validate_layout(ROOT / output(parent_target), "linux/amd64")
        anchor = output("@sonic_swss//fpmsyncd:bgp_eoiu_marker.py")
        execution_root = (ROOT / "bazel-out").resolve().parent
        source = (execution_root / anchor).resolve().parents[1]
        report = verify(layout, parent, paths, source)
        (directory / "validation.json").write_text(json.dumps(report, indent=2) + "\n")
        receipt["resolution"] = resolution.collect(ROOT, directory, bazel=[args.bazel])
        receipt["status"] = "passed"
    except Exception as error:
        receipt.update(status="failed", error=str(error))
        raise
    finally:
        (directory / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


if __name__ == "__main__":
    main()
