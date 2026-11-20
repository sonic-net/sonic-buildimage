#!/usr/bin/env python3
"""Build DASH HA images, then verify installed payloads and incremental outputs."""

import argparse
from functools import partial
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import base_images, bazel_commands, build, command_log, resolution
from tools.bazel.oci import image_validation, oci_layout, source_base_validation

OWNER = "//dockers/docker-dash-ha:"
IMAGES = ("docker-dash-ha", "docker-dash-ha-dbg")
BASE = "//dockers/docker-swss-layer-trixie:docker-swss-layer-trixie"
OPTIONS = ["--platforms=@sonic_build_infra//platforms:x86_64_trixie",
           "--host_platform=@sonic_build_infra//platforms:x86_64_trixie"]
TARGETS = {
    **{name + ".gz": OWNER + name + ".gz" for name in IMAGES},
    "test-redis.tar": OWNER + "test_redis",
    "runtime-apt.json": OWNER + "runtime_apt_selection",
    "debug-apt.json": OWNER + "debug_apt_selection",
    "dash_ha.debug.tar": OWNER + "all_debug_symbols",
    "dash_ha.debug-tools.tar": OWNER + "debug_tools",
    "dash_ha.source.tar": OWNER + "source_runtime",
    "dash_ha.files.tar": OWNER + "files",
    "dash_ha.packages.json": OWNER + "runtime_package_metadata",
    "dash_ha.status": OWNER + "installed_packages",
    "dash_ha-debug.packages.json": OWNER + "debug_package_metadata",
    "dash_ha-debug.status": OWNER + "debug_installed_packages",
    "base.packages.json": "//dockers/docker-base-trixie:runtime_package_metadata",
    "base.source.tar": "//dockers/docker-base-trixie:source_runtime",
    "config-engine.packages.json": "//dockers/docker-config-engine-trixie:runtime_package_metadata",
    "config-engine.source.tar": "//dockers/docker-config-engine-trixie:source_runtime",
    "swss.source.tar": "//dockers/docker-swss-layer-trixie:source_runtime",
    "openssh.source.tar": OWNER + "debug_client",
}


def clean_source(directory):
    check.require(not ROOT.is_relative_to(directory), "artifact directory cannot contain the source checkout")
    if directory.is_relative_to(ROOT):
        relative = directory.relative_to(ROOT)
        check.require(len(relative.parts) > 1 and relative.parts[0] == "artifacts",
                      "in-checkout evidence must use artifacts/<name>")
    subprocess.run(["git", "diff", "--exit-code", "--ignore-submodules=none", "HEAD"], cwd=ROOT, check=True)
    untracked = subprocess.check_output(
        ["git", "ls-files", "--others", "--exclude-standard", "-z"], cwd=ROOT).decode().split("\0")
    unexpected = [name for name in untracked if name and
                  not (ROOT / name).is_relative_to(directory)]
    check.require(not unexpected, "untracked source inputs: " + repr(unexpected))


def source_identity():
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    headers = subprocess.check_output(["git", "cat-file", "-p", "HEAD"], cwd=ROOT, text=True).split("\n\n", 1)[0]
    # The default Actions checkout is shallow: revision traversal hides its
    # parent list, but the immutable commit object retains the actual parents.
    parents = [line.split()[1] for line in headers.splitlines() if line.startswith("parent ")]
    expected_head, expected_base = os.environ.get("CI_HEAD_SHA"), os.environ.get("CI_BASE_SHA")
    if expected_head:
        check.require(expected_head in [revision, *parents], "tested source does not contain the requested PR head")
    if expected_base:
        check.require(expected_base in parents, "tested merge does not contain the requested base")
    return {"revision": revision, "tested_revision_parents": parents}


def build_images(args, directory):
    check.require(platform.machine() == "x86_64" and
                  platform.freedesktop_os_release().get("VERSION_CODENAME") == "trixie",
                  "DASH HA image acceptance requires native AMD64/Trixie")
    receipt = {"status": "building", "architecture": "amd64", "commands": [],
               "invocation": {"bazel": args.bazel,
                              "review_registry_revision": os.environ.get("SONIC_BASE_REVIEW_REGISTRY", "main")}}
    execute = partial(command_log.execute, cwd=ROOT)
    try:
        clean_source(directory)
        receipt.update(source_identity())
        execute(["make", "-f", "tools/bazel/prepare_manifests.mk",
                 "MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk rules/docker-swss-layer-trixie.mk rules/docker-dash-ha.mk",
                 "BUILD_WITH_BAZEL_WHEN_AVAILABLE=y", "BLDENV=trixie", "CONFIGURED_PLATFORM=vs",
                 "CONFIGURED_ARCH=amd64", "INCLUDE_FIPS=y", "ENABLE_ASAN=n", "DBG_IMAGE_MARK=dbg",
                 "DOCKERS_PATH=dockers"], directory, receipt, "manifests")
        layouts = [BASE, *[OWNER + name for name in IMAGES]]
        receipt["action_audit"] = bazel_commands.audit_targets(args.bazel, OPTIONS, [*TARGETS.values(), *layouts],
            workspace=ROOT, output=directory / "action-audit.json")
        build.collect_archives(ROOT, directory, receipt, TARGETS, bazel=args.bazel, options=OPTIONS)
        # Remote/cache hits for compressed archives need not materialize the
        # layouts inspected below. Request those outputs explicitly.
        execute([args.bazel, "build", *OPTIONS, *layouts], directory, receipt, "layouts")
        receipt["layouts"] = {name: execute([args.bazel, "cquery", *OPTIONS, "--output=files", OWNER + name],
            directory, receipt, name + ".layout").strip() for name in IMAGES}
        receipt["base_layout"] = execute([args.bazel, "cquery", *OPTIONS, "--output=files", BASE],
            directory, receipt, "base.layout").strip()
        receipt["resolution"] = resolution.collect(ROOT, directory, bazel=[args.bazel])
        clean_source(directory)
        check.require(source_identity()["revision"] == receipt["revision"], "build checkout changed")
        receipt["status"] = "built"
    except Exception as error:
        receipt.update(status="failed", error=str(error))
        raise
    finally:
        (directory / "build-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


def verify_image_boundary(name, layout, parent, files, selection):
    """Preserve the exact parent layers and keep the test service outside both images."""
    parent_layers = parent.manifest["layers"]
    parent_ids = parent.config["rootfs"]["diff_ids"]
    check.require(layout.manifest["layers"][:len(parent_layers)] == parent_layers and
                  layout.config["rootfs"]["diff_ids"][:len(parent_ids)] == parent_ids,
                  name + " does not extend its exact OCI parent")
    check.require("usr/bin/redis-server" not in files and
                  all(item["package"] != "redis-server" for item in selection["selected"]),
                  name + " includes the test-only Redis server")


def validate_images(directory):
    clean_source(directory)
    receipt = json.loads((directory / "build-receipt.json").read_text())
    check.require(receipt["status"] == "built", "DASH HA image build did not finish")
    check.require(source_identity()["revision"] == receipt["revision"], "validation checkout differs from built source")
    for name, artifact in receipt["artifacts"].items():
        check.require(check.sha(directory / name) == artifact["sha256"], "artifact changed: " + name)
    parent = oci_layout.validate_layout(ROOT / receipt["base_layout"], "linux/amd64")
    inherited = base_images.installed_inventory(parent)
    parent_selected, parent_origins = source_base_validation.inventory(parent)
    with tempfile.TemporaryDirectory(prefix="dash_ha-parent-accounts-") as temporary:
        account_root = Path(temporary)
        source_base_validation.extract_selected(parent, parent_selected, parent_origins, {"etc/group"}, account_root)
        inherited_groups = (account_root / "etc/group").read_bytes()
    inherited_names = [row.split(b":", 1)[0] for row in inherited_groups.splitlines() if row and not row.startswith(b"#")]
    check.require(len(inherited_names) == len(set(inherited_names)), "parent contains duplicate group names")
    configuration = check.payload(directory / "dash_ha.files.tar", require_root=False)
    config_sources = {
        "supervisord.conf": "etc/supervisor/conf.d/supervisord.conf",
        "critical_processes": "etc/supervisor/critical_processes",
    }
    check.require({path for path, item in configuration.items() if item["kind"] != "directory"} ==
                  set(config_sources.values()), "Dockerfile configuration inventory differs")
    for source, path in config_sources.items():
        item = configuration[path]
        check.require(item["kind"] == "file" and item["sha256"] == check.sha(ROOT / "dockers/docker-dash-ha" / source)
                      and (item["mode"], item["uid"], item["gid"]) == (0o644, 0, 0),
                      "Dockerfile configuration bytes/mode differ: " + path)
    source = check.payload(directory / "dash_ha.source.tar")
    expected = source | configuration
    debug_tools = check.payload(directory / "dash_ha.debug-tools.tar", require_root=False)
    inventories = {}
    image_parent = parent
    for name, apt_receipt in zip(IMAGES, ("runtime-apt.json", "debug-apt.json"), strict=True):
        layout = oci_layout.validate_layout(ROOT / receipt["layouts"][name], "linux/amd64")
        base_images.verify_unique_layer_paths(layout)
        files = base_images.installed_inventory(layout)
        verify_image_boundary(name, layout, image_parent, files, json.loads((directory / apt_receipt).read_bytes()))
        image_parent = layout
        base_images.verify_directory_aliases(files)
        for path, item in expected.items():
            if item["kind"] == "directory" and path not in configuration:
                continue
            check.require(all(files.get(path, {}).get(key) == value for key, value in item.items()),
                          "installed source/configuration payload changed: " + path)
        for path, item in inherited.items():
            if item["kind"] == "directory" or path in ("var/lib/dpkg/status",):
                continue
            # Docker-owned COPY/template paths may intentionally replace an
            # inherited file; preserve every other parent byte and metadata.
            if name.endswith("-dbg") and path == "etc/group":
                continue
            expected_item = configuration.get(path, item)
            if name.endswith("-dbg"):
                expected_item = debug_tools.get(path, expected_item)
            check.require(all(files.get(path, {}).get(key) == value for key, value in expected_item.items()),
                          "DASH HA changed inherited SWSS/base payload: " + path)
        if name.endswith("-dbg"):
            for path, item in debug_tools.items():
                if item["kind"] != "directory":
                    check.require(all(files.get(path, {}).get(key) == value for key, value in item.items()),
                                  "debug tool payload changed: " + path)
        prefix = "dash_ha" if name == "docker-dash-ha" else "dash_ha-debug"
        for path, artifact in (("var/lib/dpkg/status", prefix + ".status"),
                               ("usr/share/sonic/bazel/" + prefix.replace("dash_ha", "dash-ha") + "-packages.json", prefix + ".packages.json")):
            check.require(files.get(path, {}).get("sha256") == check.sha(directory / artifact),
                          "installed package state differs from checked input: " + path)
        check.require(all(item.get("elf_machine", 62) == 62 for item in files.values()), "image contains a non-AMD64 ELF")
        config = layout.config.get("config", {})
        check.require(config.get("Entrypoint") == ["/usr/local/bin/supervisord"], "DASH HA entrypoint changed")
        check.require("DEBIAN_FRONTEND=noninteractive" in config.get("Env", []), "missing DASH HA environment")
        check.require(config.get("ExposedPorts", {}) == parent.config.get("config", {}).get("ExposedPorts", {}),
                      "DASH HA unexpectedly changed inherited exposed ports")
        manifest = json.loads(config["Labels"]["com.azure.sonic.manifest"])
        check.require(manifest == json.loads((ROOT / "target/bazel-manifests" / name / "manifest.json").read_text()),
                      "DASH HA Make metadata label differs")
        selected, origins = source_base_validation.inventory(layout)
        with tempfile.TemporaryDirectory(prefix="dash_ha-accounts-") as temporary:
            root = Path(temporary)
            source_base_validation.extract_selected(layout, selected, origins, {"etc/group"}, root)
            groups = (root / "etc/group").read_bytes()
            group_names = [row.split(b":", 1)[0] for row in groups.splitlines() if row and not row.startswith(b"#")]
            check.require(len(group_names) == len(set(group_names)), "image contains duplicate group names")
            expected_groups = inherited_groups
            if name.endswith("-dbg") and b"_ssh:x:104:" not in inherited_groups.splitlines():
                expected_groups += (b"\n" if inherited_groups and not inherited_groups.endswith(b"\n") else b"") + b"_ssh:x:104:\n"
            check.require(groups == expected_groups, "inherited group rows or debug ssh-agent group differ")
            check.require(all(files["etc/group"].get(key) == inherited["etc/group"].get(key)
                              for key in ("kind", "mode", "uid", "gid")),
                          "group file kind, ownership or mode differs from the parent")
        archive = image_validation.docker_archive(directory / (name + ".gz"), name + ":latest",
            layout.manifest["config"]["digest"], layout.manifest["layers"])
        inventories[name] = files
        (directory / (name + ".validation.json")).write_text(json.dumps({
            "status": "passed", "manifest_digest": layout.descriptor["digest"],
            "rootfs": layout.config["rootfs"], "config_digest": layout.manifest["config"]["digest"],
            "archive": archive}, indent=2) + "\n")
    runtime, debug = (inventories[name] for name in IMAGES)
    for path, item in runtime.items():
        if "elf_machine" in item:
            check.require(debug.get(path) == item, "debug image changed runtime ELF: " + path)
    reports = [json.loads((directory / name).read_text()) for name in
               ("base.packages.json", "config-engine.packages.json", "dash_ha-debug.packages.json")]
    native = base_images.verify_source_files(debug, reports, 62)
    runtimes = [directory / name for name in
                ("base.source.tar", "config-engine.source.tar", "swss.source.tar", "dash_ha.source.tar", "openssh.source.tar")]
    for archive in runtimes:
        for path, item in check.payload(archive, require_root=False).items():
            if "elf_machine" not in item:
                continue
            check.require(all(debug.get(path, {}).get(key) == value for key, value in item.items()),
                          "debug image changed source-owned ELF: " + path)
            if path in native:
                check.require(native[path]["sha256"] == item["sha256"], "source owners disagree: " + path)
            else:
                native[path] = {**item, "source_package": archive.name}
    check.require({"usr/bin/hamgrd", "usr/bin/swbusd", "usr/bin/swbus-cli"} ==
                  {path for path, item in source.items() if "elf_machine" in item},
                  "HA owner executable inventory differs")
    check.require({path for path, item in source.items() if item["kind"] != "directory"} ==
                  {"usr/bin/hamgrd", "usr/bin/swbusd", "usr/bin/swbus-cli", "usr/bin/wait_for_loopback.py"},
                  "HA native install inventory differs")
    pairs = base_images.verify_symbols(native, runtimes, directory / "dash_ha.debug.tar", directory)
    check.require(debug["usr/bin/ssh-agent"]["gid"] == 104 and debug["usr/bin/ssh-agent"]["mode"] == 0o2755,
                  "debug FIPS OpenSSH client permissions differ")
    debug_layout = image_parent
    elfs = []
    selected, origins = source_base_validation.inventory(debug_layout)
    with tempfile.TemporaryDirectory(prefix="dash_ha-native-") as temporary:
        root = Path(temporary)
        source_base_validation.extract_selected(debug_layout, selected, origins, set(native), root)
        for path, item in sorted(native.items()):
            info = check.elf_info(root / path, dynamic=True)
            elfs.append({"path": path, "needed": info["needed"], "sha256": item["sha256"]})
    report = {"elfs": elfs, "status": "passed", "revision": receipt["revision"], "architecture": "amd64",
              "source_debug_pairs": pairs, "native": native,
              "runtime_native": {path: item for path, item in native.items() if path in runtime},
              "artifacts": receipt["artifacts"]}
    (directory / "validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": "passed", "source_elf_debug_pairs": len(pairs)}))


def incremental(args, directory):
    clean_source(directory)
    baseline = json.loads((directory / "build-receipt.json").read_text())
    check.require(baseline["status"] == "built" and source_identity()["revision"] == baseline["revision"],
                  "incremental checkout differs from built source")
    def output(label):
        return ROOT / subprocess.check_output([args.bazel, "cquery", *OPTIONS, "--output=files", label], cwd=ROOT, text=True).strip()
    archives = {name: output(OWNER + name + ".gz") for name in IMAGES}
    owners = {name: output(TARGETS[name]) for name in ("dash_ha.source.tar", "swss.source.tar")}
    base = output(BASE)
    layouts = {name: ROOT / path for name, path in baseline["layouts"].items()}
    targets = [OWNER + name + suffix for name in IMAGES for suffix in ("", ".gz")]
    targets += [BASE, *[TARGETS[name] for name in owners]]
    command = [args.bazel, "build", *OPTIONS, *targets]
    def snapshot():
        return {"images": {name: oci_layout.validate_layout(path, "linux/amd64").descriptor["digest"] for name, path in layouts.items()},
                "archives": {name: check.sha(path) for name, path in archives.items()},
                "owners": {name: check.sha(path) for name, path in owners.items()},
                "base": oci_layout.validate_layout(base, "linux/amd64").descriptor["digest"]}
    timings = {}
    def timed_build(name):
        start = time.monotonic()
        subprocess.run(command, cwd=ROOT, check=True)
        timings[name] = round(time.monotonic() - start, 3)
    timed_build("baseline")
    original = snapshot()
    check.require(all(original["archives"][name] == baseline["artifacts"][name + ".gz"]["sha256"] for name in IMAGES),
                  "incremental baseline differs from verified archives")
    timed_build("no-op")
    unchanged = snapshot()
    check.require(unchanged == original, "no-op build changed installed source/image bytes")
    source = ROOT / "dockers/docker-dash-ha/supervisord.conf"
    saved = source.read_bytes()
    try:
        source.write_bytes(saved + b"\n; Bazel incremental image input probe\n")
        timed_build("configuration-edit")
        changed = snapshot()
        check.require(all(changed[field][name] != original[field][name] for field in ("images", "archives") for name in IMAGES),
                      "DASH HA configuration edit did not invalidate both images and archives")
        check.require(changed["owners"] == original["owners"] and changed["base"] == original["base"],
                      "DASH HA image input edit changed an unrelated source owner or base")
    finally:
        source.write_bytes(saved)
    timed_build("restored")
    restored = snapshot()
    clean_source(directory)
    check.require(source_identity()["revision"] == baseline["revision"], "incremental checkout changed")
    check.require(restored == original, "restoring the configuration did not reproduce exact image/archive bytes")
    (directory / "incremental.json").write_text(json.dumps({"status": "passed", "input": str(source.relative_to(ROOT)),
        "timings_seconds": timings, "baseline": original, "unchanged": unchanged, "changed": changed, "restored": restored}, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phase", choices=("build", "validate", "incremental"))
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--bazel", default="tools/bazel/ci/review_bazel.sh")
    args = parser.parse_args()
    directory = args.artifacts.resolve(); directory.mkdir(parents=True, exist_ok=True)
    if args.phase == "build": build_images(args, directory)
    elif args.phase == "validate": validate_images(directory)
    else: incremental(args, directory)


if __name__ == "__main__":
    main()
