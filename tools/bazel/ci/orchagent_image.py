#!/usr/bin/env python3
"""Build the source-owned Orchagent images and verify the installed payloads."""

import argparse
from functools import partial
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import platform
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import base_images, bazel_commands, build, command_log, resolution
from tools.bazel.ci.swss_layer import verify_source_runtime
from tools.bazel.oci import image_validation, oci_layout, source_base_validation

OWNER = "//dockers/docker-orchagent:"
IMAGES = ("docker-orchagent", "docker-orchagent-dbg")
BASE = "//dockers/docker-config-engine-trixie:docker-config-engine-trixie"
OPTIONS = ["--jobs=2", "--local_resources=cpu=2", "--local_resources=memory=8000",
           "--platforms=@sonic_build_infra//platforms:x86_64_trixie",
           "--host_platform=@sonic_build_infra//platforms:x86_64_trixie"]
TARGETS = {
    **{name + ".gz": OWNER + name + ".gz" for name in IMAGES},
    "scapy-2.6.1.dev0-py3-none-any.whl": "@scapy//:wheel",
    "test-redis.tar": "//src/sonic-eventd:test_runtime",
    "runtime-apt.json": OWNER + "runtime_apt_selection",
    "debug-apt.json": OWNER + "debug_apt_selection",
    "source-runtime.tar": OWNER + "rdeps",
    "source-debug.tar": OWNER + "rdeps_debug_symbols",
    "image-debug.tar": OWNER + "all_debug_symbols",
    "openssh.source.tar": "@sonic_fips//openssh:openssh_client_pkg",
    "runtime.alternatives.tar": OWNER + "runtime_alternatives",
    "runtime.status": OWNER + "installed_packages",
    "runtime.packages.json": OWNER + "runtime_package_metadata",
    "debug.accounts.tar": OWNER + "debug_accounts",
    "debug.tools.tar": OWNER + "debug_tools",
    "debug.status": OWNER + "debug_installed_packages",
    "debug.packages.json": OWNER + "debug_package_metadata",
    "config.tar": OWNER + "ci_config",
    "python.tar": OWNER + "python_runtime",
    "base.packages.json": "//dockers/docker-base-trixie:runtime_package_metadata",
    "base.source.tar": "//dockers/docker-base-trixie:source_runtime",
    "config-engine.packages.json": "//dockers/docker-config-engine-trixie:runtime_package_metadata",
    "config-engine.source.tar": "//dockers/docker-config-engine-trixie:source_runtime",
}


def identity(directory):
    subprocess.run(["git", "diff", "--exit-code", "--ignore-submodules=none", "HEAD"], cwd=ROOT, check=True)
    untracked = subprocess.check_output(["git", "ls-files", "--others", "--exclude-standard", "-z"], cwd=ROOT).decode().split("\0")
    check.require(not any(name and not (ROOT / name).is_relative_to(directory) for name in untracked),
                  "untracked source inputs")
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    commit = subprocess.check_output(["git", "cat-file", "-p", "HEAD"], cwd=ROOT, text=True)
    parents = [line.split()[1] for line in commit.split("\n\n", 1)[0].splitlines() if line.startswith("parent ")]
    head, base = os.environ.get("CI_HEAD_SHA"), os.environ.get("CI_BASE_SHA")
    check.require(not head or head in [revision, *parents], "checkout does not contain requested PR head")
    check.require(not base or base in parents, "checkout does not contain requested PR base")
    return revision


def build_images(args, directory):
    check.require(platform.machine() == "x86_64" and platform.freedesktop_os_release().get("VERSION_CODENAME") == "trixie",
                  "Orchagent source acceptance requires native AMD64/Trixie")
    receipt = {"status": "building", "architecture": "amd64", "commands": [], "revision": identity(directory),
               "invocation": {"bazel": args.bazel}}
    execute = partial(command_log.execute, cwd=ROOT)
    try:
        execute(["make", "-f", "tools/bazel/prepare_manifests.mk",
                 "MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk rules/docker-orchagent.mk",
                 "BUILD_WITH_BAZEL_WHEN_AVAILABLE=y", "BLDENV=trixie", "CONFIGURED_PLATFORM=vs", "CONFIGURED_ARCH=amd64",
                 "INCLUDE_FIPS=y", "ENABLE_ASAN=n", "DBG_IMAGE_MARK=dbg", "DOCKERS_PATH=dockers"], directory, receipt, "manifests")
        layouts = [BASE, *[OWNER + name for name in IMAGES]]
        receipt["action_audit"] = bazel_commands.audit_targets(args.bazel, OPTIONS, [*TARGETS.values(), *layouts],
            workspace=ROOT, output=directory / "action-audit.json")
        build.collect_archives(ROOT, directory, receipt, TARGETS, bazel=args.bazel, options=OPTIONS)
        execute([args.bazel, "build", *OPTIONS, *layouts], directory, receipt, "layouts")
        receipt["layouts"] = {name: execute([args.bazel, "cquery", *OPTIONS, "--output=files", OWNER + name],
            directory, receipt, name + ".layout").strip() for name in IMAGES}
        receipt["base_layout"] = execute([args.bazel, "cquery", *OPTIONS, "--output=files", BASE], directory, receipt, "base.layout").strip()
        receipt["resolution"] = resolution.collect(ROOT, directory, bazel=[args.bazel])
        check.require(identity(directory) == receipt["revision"], "build checkout changed")
        receipt["status"] = "built"
    except Exception as error:
        receipt.update(status="failed", error=str(error))
        raise
    finally:
        (directory / "build-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


def verify_scapy(files, wheel):
    """Compare owner wheel payload bytes after standard installer relocation."""
    paths = {}
    with zipfile.ZipFile(wheel) as source:
        for item in source.infolist():
            if item.is_dir():
                continue
            name = item.filename
            check.require(not PurePosixPath(name).is_absolute() and ".." not in PurePosixPath(name).parts,
                          "unsafe wheel path")
            # RECORD is regenerated by the upstream installer; its own package
            # test validates metadata and the wheel's full original RECORD.
            if name.endswith(".dist-info/RECORD"):
                continue
            if ".data/" in name:
                prefix, name = name.split(".data/", 1)
                scheme, name = name.split("/", 1)
                check.require(scheme == "data", "unexpected Scapy install scheme: " + scheme)
                name = "usr/local/" + name
            else:
                name = "usr/local/lib/python3.13/dist-packages/" + name
            digest = hashlib.sha256(source.read(item)).hexdigest()
            actual = files.get(name, {})
            check.require(actual.get("kind") == "file" and actual.get("sha256") == digest
                          and (actual.get("uid"), actual.get("gid")) == (0, 0),
                          "image changed source wheel payload: " + name)
            paths[name] = digest
    check.require(len(paths) > 365 and files.get("usr/local/bin/scapy", {}).get("mode") == 0o755,
                  "Scapy inventory or console entry point missing")
    return paths


def validate_images(directory):
    receipt = json.loads((directory / "build-receipt.json").read_text())
    check.require(receipt["status"] == "built" and identity(directory) == receipt["revision"], "checkout differs from completed build")
    for name, item in receipt["artifacts"].items():
        check.require(check.sha(directory / name) == item["sha256"], "artifact changed: " + name)
    parent = oci_layout.validate_layout(ROOT / receipt["base_layout"], "linux/amd64")
    inherited = base_images.installed_inventory(parent)
    configuration = {}
    image_validation.apply_layer(directory / "runtime.alternatives.tar", configuration)
    image_validation.apply_layer(directory / "config.tar", configuration)
    image_validation.apply_layer(directory / "python.tar", configuration)
    client = check.payload(directory / "openssh.source.tar", require_root=False)
    accounts = check.payload(directory / "debug.accounts.tar", require_root=False)
    debug_tools = check.payload(directory / "debug.tools.tar", require_root=False)
    inventories, layouts, source_native = {}, {}, {}
    for name in IMAGES:
        layout = oci_layout.validate_layout(ROOT / receipt["layouts"][name], "linux/amd64")
        layouts[name] = layout
        base_images.verify_unique_layer_paths(layout)
        files = base_images.installed_inventory(layout)
        base_images.verify_directory_aliases(files)
        _, source_native = verify_source_runtime(files, directory / "source-runtime.tar")
        for path, item in configuration.items():
            if item["kind"] != "directory":
                check.require(all(files.get(path, {}).get(key) == value for key, value in item.items()), "image changed owner configuration: " + path)
        check.require(files.get("usr/bin/traceroute6", {}).get("linkname") == "/etc/alternatives/traceroute6"
                      and files.get("etc/alternatives/traceroute6", {}).get("linkname") == "/usr/bin/rltraceroute6"
                      and files.get("usr/bin/rltraceroute6", {}).get("elf_machine") == 62
                      and files.get("var/lib/dpkg/alternatives/traceroute6", {}).get("kind") == "file"
                      and files.get("var/lib/dpkg/alternatives/traceroute6", {}).get("size") == 148
                      and files.get("var/lib/dpkg/alternatives/traceroute6", {}).get("sha256")
                      == "3d9e0f58161329a1f8a511fe4b89e107d55785eb2db138f1d21d62d1fdec109a",
                      "missing or malformed ndisc6 traceroute6 alternatives state")
        check.require(files.get("usr/share/sonic/bazel/orchagent-packages.json", {}).get("sha256") == check.sha(directory / "runtime.packages.json"),
                      "installed runtime package metadata differs")
        for path, item in inherited.items():
            if item["kind"] == "directory" or path in configuration or path == "var/lib/dpkg/status":
                continue
            if name.endswith("-dbg") and path in accounts:
                continue
            expected = debug_tools.get(path, item) if name.endswith("-dbg") else item
            check.require(all(files.get(path, {}).get(key) == value for key, value in expected.items()),
                          "image changed config-engine parent: " + path)
        if name.endswith("-dbg"):
            for path, item in (debug_tools | client | accounts).items():
                if item["kind"] != "directory":
                    check.require(all(files.get(path, {}).get(key) == value for key, value in item.items()),
                                  "debug image changed source client or account metadata: " + path)
            for path, artifact in (("var/lib/dpkg/status", "debug.status"),
                                   ("usr/share/sonic/bazel/orchagent-debug-packages.json", "debug.packages.json")):
                check.require(files.get(path, {}).get("sha256") == check.sha(directory / artifact),
                              "installed debug package state differs: " + path)
        else:
            check.require(files.get("var/lib/dpkg/status", {}).get("sha256") == check.sha(directory / "runtime.status"),
                          "installed runtime dpkg status differs")
        scapy = verify_scapy(files, directory / "scapy-2.6.1.dev0-py3-none-any.whl")
        config = layout.config.get("config", {})
        check.require(config.get("Entrypoint") == ["/usr/bin/docker-init.sh"], "Orchagent entrypoint changed")
        check.require("DEBIAN_FRONTEND=noninteractive" in config.get("Env", []), "missing environment")
        manifest = json.loads(config["Labels"]["com.azure.sonic.manifest"])
        check.require(manifest == json.loads((ROOT / "target/bazel-manifests" / name / "manifest.json").read_text()), "Make image labels differ")
        archive = image_validation.docker_archive(directory / (name + ".gz"), name + ":latest", layout.manifest["config"]["digest"], layout.manifest["layers"])
        inventories[name] = files
        (directory / (name + ".validation.json")).write_text(json.dumps({"status": "passed", "manifest_digest": layout.descriptor["digest"],
            "config_digest": layout.manifest["config"]["digest"], "rootfs": layout.config["rootfs"], "archive": archive}, indent=2) + "\n")
    runtime, debug = (inventories[name] for name in IMAGES)
    runtime_layout, debug_layout = (layouts[name] for name in IMAGES)
    check.require(debug_layout.manifest["layers"][:len(runtime_layout.layers)] == runtime_layout.manifest["layers"], "debug image does not extend runtime")
    for path, item in runtime.items():
        if "elf_machine" in item:
            check.require(item["elf_machine"] == 62 and debug.get(path) == item, "debug changed runtime ELF: " + path)
    reports = [json.loads((directory / name).read_text())
               for name in ("base.packages.json", "config-engine.packages.json", "runtime.packages.json")]
    native = base_images.verify_source_files(debug, reports, 62)
    for path, item in source_native.items():
        if path in native:
            check.require(native[path]["sha256"] == item["sha256"], "source owners disagree: " + path)
        else:
            native[path] = {**item, "source_package": "source-runtime.tar"}
    client_native = {path: item for path, item in client.items() if "elf_machine" in item}
    check.require(len(client_native) == 10 and all(item["elf_machine"] == 62 for item in client_native.values()),
                  "FIPS OpenSSH client ELF inventory differs")
    check.require(all(path not in runtime for path in client_native), "debug-only source client entered runtime image")
    native.update({path: {**item, "source_package": "openssh-client"} for path, item in client_native.items()})
    runtimes = [directory / name for name in ("base.source.tar", "config-engine.source.tar", "source-runtime.tar", "openssh.source.tar")]
    pairs = base_images.verify_symbols(native, runtimes, directory / "image-debug.tar", directory)
    check.require(debug["usr/bin/ssh-agent"]["gid"] == 104 and debug["usr/bin/ssh-agent"]["mode"] == 0o2755,
                  "debug FIPS OpenSSH client permissions differ")
    group_rows = []
    for layout in (runtime_layout, debug_layout):
        selected, origins = source_base_validation.inventory(layout)
        with tempfile.TemporaryDirectory(prefix="orchagent-accounts-") as temporary:
            root = Path(temporary)
            source_base_validation.extract_selected(layout, selected, origins, {"etc/group"}, root)
            rows = (root / "etc/group").read_text().splitlines()
            names = [row.split(":", 1)[0] for row in rows]
            check.require(len(names) == len(set(names)), "duplicate group name")
            group_rows.append(rows)
    expected_groups = group_rows[0] + ([] if "_ssh:x:104:" in group_rows[0] else ["_ssh:x:104:"])
    check.require(group_rows[1] == expected_groups, "debug account merge changed existing group rows")
    check.require(all(debug["etc/group"][key] == runtime["etc/group"][key]
                      for key in ("kind", "uid", "gid", "mode")), "debug group metadata changed")
    for path in ("etc/passwd", "etc/gshadow"):
        check.require(debug.get(path) == runtime.get(path), "debug changed inherited account state: " + path)
    elfs = []
    selected, origins = source_base_validation.inventory(debug_layout)
    with tempfile.TemporaryDirectory(prefix="orchagent-native-") as temporary:
        root = Path(temporary)
        source_base_validation.extract_selected(debug_layout, selected, origins, set(native), root)
        for path, item in sorted(native.items()):
            elfs.append({"path": path, "needed": check.elf_info(root / path, dynamic=True)["needed"], "sha256": item["sha256"]})
    report = {"status": "passed", "revision": receipt["revision"], "architecture": "amd64", "native": native,
              "runtime_native": {path: item for path, item in native.items() if path in runtime},
              "elfs": elfs, "source_debug_pairs": pairs, "scapy_files": scapy,
              "config_files": {path: item for path, item in configuration.items() if item["kind"] == "file"},
              "artifacts": receipt["artifacts"]}
    (directory / "validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": "passed", "source_elf_debug_pairs": len(pairs), "scapy_files": len(scapy)}))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phase", choices=("build", "validate"))
    parser.add_argument("--bazel", default="bazel")
    parser.add_argument("--artifacts", required=True, type=Path)
    args = parser.parse_args()
    directory = args.artifacts.resolve()
    check.require(not ROOT.is_relative_to(directory), "artifacts cannot contain source checkout")
    check.require(not directory.is_relative_to(ROOT) or directory.relative_to(ROOT).parts[:1] == ("artifacts",), "in-checkout evidence requires artifacts/<name>")
    directory.mkdir(parents=True, exist_ok=True)
    if args.phase == "build":
        build_images(args, directory)
    else:
        validate_images(directory)


if __name__ == "__main__":
    main()
