#!/usr/bin/env python3
"""Build LLDP archives, then check completed contents and incremental reuse."""

import argparse
from functools import partial
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import base_images, bazel_commands, build, command_log, resolution
from tools.bazel.oci import image_validation, oci_layout

OWNER = "//dockers/docker-lldp:"
IMAGES = ("docker-lldp", "docker-lldp-dbg")
PARENT = "//dockers/docker-config-engine-trixie:docker-config-engine-trixie"
TARGETS = {
    **{name + ".gz": OWNER + name + ".gz" for name in IMAGES},
    "lldp-statoverride.txt": "//dockers/docker-lldp/config:statoverride",
    "lldpd-owner.tar": "@lldpd//:runtime",
    "source-runtime.tar": OWNER + "source_runtime",
    "dbsyncd-runtime.tar": "@sonic_dbsyncd//:sonic_dbsyncd_runtime",
    "config-files.tar": "//dockers/docker-lldp/config:files",
    "runtime-packages.json": OWNER + "runtime_package_metadata",
    "runtime.status": OWNER + "installed_packages",
    "all-debug.tar": OWNER + "all_debug_symbols",
    "debug-tools.tar": OWNER + "debug_tools",
    "test-redis.tar": OWNER + "test_redis",
    "base-source.tar": "//dockers/docker-base-trixie:source_runtime",
    "base-packages.json": "//dockers/docker-base-trixie:runtime_package_metadata",
    "config-engine-source.tar": "//dockers/docker-config-engine-trixie:source_runtime",
    "config-engine-packages.json": "//dockers/docker-config-engine-trixie:runtime_package_metadata",
    "openssh-source.tar": OWNER + "debug_client",
    "debug-packages.json": OWNER + "debug_package_metadata",
    "debug.status": OWNER + "debug_installed_packages",
}
OPTIONS = ["--jobs=2", "--local_resources=cpu=2", "--local_resources=memory=10000"]
CONFIGS = {
    "start.sh": ("usr/bin/start.sh", 0o755),
    "docker-lldp-init.sh": ("usr/bin/docker-lldp-init.sh", 0o755),
    "waitfor_lldp_ready.sh": ("usr/bin/waitfor_lldp_ready.sh", 0o755),
    "lldpmgrd": ("usr/bin/lldpmgrd", 0o755),
    "lldpd": ("etc/default/lldpd", 0o644),
    "critical_processes": ("etc/supervisor/critical_processes", 0o644),
    **{name: ("usr/share/sonic/templates/" + name, 0o644) for name in
       ("lldpd.conf.j2", "lldpdSysDescr.conf.j2", "supervisord.conf.j2", "supervisord.conf.common.j2")},
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
    parents = [line.split()[1] for line in headers.splitlines() if line.startswith("parent ")]
    if os.environ.get("GITHUB_SHA"):
        check.require(revision == os.environ["GITHUB_SHA"], "source differs from the requested GitHub revision")
    if os.environ.get("CI_HEAD_SHA"):
        check.require(revision == os.environ["CI_HEAD_SHA"] or
                      (len(parents) == 2 and parents[1] == os.environ["CI_HEAD_SHA"]),
                      "CI source differs from requested PR head")
    if os.environ.get("CI_BASE_SHA"):
        check.require(len(parents) == 2 and parents[0] == os.environ["CI_BASE_SHA"], "CI merge differs from requested PR base")
    return {"revision": revision, "tested_revision_parents": parents}


def build_images(args, directory):
    check.require(platform.machine() == "x86_64" and
                  platform.freedesktop_os_release().get("VERSION_CODENAME") == "trixie",
                  "LLDP image build requires native AMD64 Debian Trixie")
    options = OPTIONS + ["--//dockers/docker-lldp:image_version=" + args.image_version]
    receipt = {"status": "building", "architecture": "amd64", "commands": [], "image_version": args.image_version}
    execute = partial(command_log.execute, cwd=ROOT)
    try:
        clean_source(directory)
        receipt.update(source_identity())
        receipt["commit_headers"] = subprocess.check_output(
            ["git", "cat-file", "-p", "HEAD"], cwd=ROOT, text=True).split("\n\n", 1)[0]
        execute(["make", "-f", "tools/bazel/prepare_manifests.mk", "manifests",
                 "target/bazel-manifests/docker-lldp/image.env",
                 "MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk rules/docker-lldp.mk",
                 "BUILD_WITH_BAZEL_WHEN_AVAILABLE=y", "BLDENV=trixie", "CONFIGURED_PLATFORM=vs",
                 "CONFIGURED_ARCH=amd64", "INCLUDE_FIPS=y", "ENABLE_ASAN=n", "DBG_IMAGE_MARK=dbg",
                 "DOCKERS_PATH=dockers", "SONIC_IMAGE_VERSION=" + args.image_version], directory, receipt, "manifests")
        layouts = [OWNER + name for name in IMAGES] + [PARENT]
        receipt["action_audit"] = bazel_commands.audit_targets(
            args.bazel, options, [*TARGETS.values(), *layouts], workspace=ROOT,
            output=directory / "action-audit.json")
        build.collect_archives(ROOT, directory, receipt, TARGETS, bazel=args.bazel, options=options)
        execute([args.bazel, "build", *options, *layouts], directory, receipt, "layouts")
        receipt["layouts"] = {target: execute(
            [args.bazel, "cquery", *options, "config(" + target + ", target)", "--output=files"], directory, receipt,
            "layout-" + str(index)).strip() for index, target in enumerate(layouts)}
        receipt["resolution"] = resolution.collect(ROOT, directory, bazel=[args.bazel])
        clean_source(directory)
        check.require(source_identity()["revision"] == receipt["revision"], "build checkout changed")
        receipt["status"] = "built"
    except Exception as error:
        receipt.update(status="failed", error=str(error))
        raise
    finally:
        (directory / "build-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


def validate_images(directory):
    clean_source(directory)
    receipt = json.loads((directory / "build-receipt.json").read_text())
    check.require(receipt["status"] == "built", "image build did not complete")
    check.require(source_identity()["revision"] == receipt["revision"], "validation checkout differs from built source")
    check.require(source_identity() == {key: receipt[key] for key in ("revision", "tested_revision_parents")},
                  "receipt source identity differs")
    for name, item in receipt["artifacts"].items():
        check.require(check.sha(directory / name) == item["sha256"], "artifact changed: " + name)
    inherited = base_images.installed_inventory(oci_layout.validate_layout(ROOT / receipt["layouts"][PARENT], "linux/amd64"))
    tools = check.payload(directory / "debug-tools.tar", require_root=False)
    reports, inventories = {}, {}
    for name in IMAGES:
        debug = name.endswith("-dbg")
        layout = oci_layout.validate_layout(ROOT / receipt["layouts"][OWNER + name], "linux/amd64")
        base_images.verify_unique_layer_paths(layout)
        files = base_images.installed_inventory(layout)
        base_images.verify_directory_aliases(files)
        check.require(all(item.get("elf_machine", 62) == 62 for item in files.values()), "non-AMD64 ELF")
        replaced = {destination for destination, _ in CONFIGS.values()} | {"var/lib/dpkg/status", "var/lib/dpkg/statoverride", "etc/passwd", "etc/shadow", "etc/group", "etc/gshadow"}
        for path, item in inherited.items():
            if item["kind"] != "directory" and path not in replaced:
                expected = tools.get(path, item) if debug else item
                check.require(all(files.get(path, {}).get(key) == value for key, value in expected.items()),
                              "image replaced an inherited file: " + path)
        if debug:
            base_images.verify_installed_layer(files, directory / "debug-tools.tar")
        base_images.verify_installed_layer(files, directory / "config-files.tar")
        base_images.verify_installed_layer(files, directory / "dbsyncd-runtime.tar")
        for path, expected in check.payload(directory / "source-runtime.tar", require_root=False).items():
            if expected["kind"] == "directory" or path in {destination for destination, _ in CONFIGS.values()}:
                continue
            check.require(all(files.get(path, {}).get(key) == value for key, value in expected.items()),
                          "installed LLDP/DBSyncd source payload differs: " + path)
        check.require(files["usr/sbin/lldpcli"]["uid"] == 101 and
                      files["usr/sbin/lldpcli"]["gid"] == 4 and
                      files["usr/sbin/lldpcli"]["mode"] == 0o4750,
                      "source lldpcli permissions differ")
        for source, (destination, mode) in CONFIGS.items():
            item = files.get(destination, {})
            check.require(item.get("sha256") == check.sha(ROOT / "dockers/docker-lldp" / source)
                          and (item.get("uid"), item.get("gid"), item.get("mode")) == (0, 0, mode),
                          "original service file differs: " + destination)
        check.require("usr/sbin/lldpd" in files and files["usr/sbin/lldpd"].get("elf_machine") == 62,
                      "locked AMD64 lldpd is missing")
        check.require(files["usr/share/sonic/bazel/lldp-packages.json"]["sha256"] == check.sha(directory / "runtime-packages.json"),
                      "runtime imported package records differ")
        status = "debug.status" if debug else "runtime.status"
        check.require(files["var/lib/dpkg/status"]["sha256"] == check.sha(directory / status), "package status differs")
        check.require(files["var/lib/dpkg/statoverride"]["sha256"] == check.sha(directory / "lldp-statoverride.txt"),
                      "LLDP statoverride or inherited records differ")
        check.require("_lldpd adm 4750 /usr/sbin/lldpcli" in (directory / "lldp-statoverride.txt").read_text().splitlines(),
                      "LLDP statoverride record is absent")
        config = layout.config.get("config", {})
        check.require(config.get("Entrypoint") == ["/usr/bin/docker-lldp-init.sh"], "entrypoint changed")
        check.require("DEBIAN_FRONTEND=noninteractive" in config.get("Env", []) and
                      "IMAGE_VERSION=" + receipt["image_version"] in config.get("Env", []), "image environment changed")
        expected = json.loads((ROOT / "target/bazel-manifests" / name / "manifest.json").read_text())
        check.require(json.loads(config["Labels"]["com.azure.sonic.manifest"]) == expected, "Make labels differ")
        labels = config["Labels"]
        check.require(labels.get("Tag") == receipt["image_version"], "Make image version label differs")
        packages = json.loads((directory / "runtime-packages.json").read_text())["dependency_check"]["packages"]
        for package in ("libswsscommon", "sonic-supervisord-utilities-rs"):
            check.require(labels.get("com.azure.sonic.versions." + package) == packages[package]["Version"],
                          "inherited Make component version label differs: " + package)
        archive = image_validation.docker_archive(directory / (name + ".gz"), name + ":latest",
                                                  layout.manifest["config"]["digest"], layout.manifest["layers"])
        reports[name] = {"manifest_digest": layout.descriptor["digest"],
                         "config_digest": layout.manifest["config"]["digest"], "rootfs": layout.config["rootfs"],
                         "archive": archive, "parent_payload_preserved": True}
        inventories[name] = files
    runtime, debug = (inventories[name] for name in IMAGES)
    for path, item in runtime.items():
        if "elf_machine" in item:
            expected = tools.get(path, item)
            check.require(all(debug.get(path, {}).get(key) == value for key, value in expected.items()),
                          "debug variant replaced an undeclared runtime ELF: " + path)
    check.require(debug["var/lib/dpkg/status"]["sha256"] == check.sha(directory / "debug.status") and
                  debug["usr/share/sonic/bazel/lldp-debug-packages.json"]["sha256"] == check.sha(directory / "debug-packages.json"),
                  "debug package inventory differs")
    native = base_images.verify_source_files(debug, [json.loads((directory / name).read_text()) for name in
        ("base-packages.json", "config-engine-packages.json", "debug-packages.json")], 62)
    owner_native = {path: item for path, item in check.payload(directory / "lldpd-owner.tar", require_root=False).items()
                    if "elf_machine" in item}
    check.require(len(owner_native) == 3, "LLDP owner must supply all three native ELFs")
    for path, expected in owner_native.items():
        check.require(path not in native and all(debug.get(path, {}).get(key) == value
                      for key, value in expected.items()), "LLDP source ELF differs: " + path)
        native[path] = {**debug[path], "source_package": "lldpd"}
    pairs = base_images.verify_symbols(native, [directory / name for name in
        ("base-source.tar", "config-engine-source.tar", "openssh-source.tar", "lldpd-owner.tar")], directory / "all-debug.tar", directory)
    for path, item in check.payload(directory / "all-debug.tar").items():
        if item["kind"] != "directory":
            check.require(all(debug.get(path, {}).get(key) == value for key, value in item.items())
                          and path not in runtime, "debug symbols differ: " + path)
    check.require(debug["usr/bin/ssh-agent"]["gid"] == 104 and debug["usr/bin/ssh-agent"]["mode"] == 0o2755,
                  "source OpenSSH permissions differ")
    for name in IMAGES:
        reports[name]["native_paths"] = sorted(set(native).intersection(inventories[name]))
    report = {"status": "passed", "revision": receipt["revision"], "architecture": "amd64", "image_version": receipt["image_version"],
              "tested_revision_parents": receipt["tested_revision_parents"], "images": reports, "source_debug_pairs": pairs, "native": native, "artifacts": receipt["artifacts"]}
    (directory / "validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": "passed", "source_elf_debug_pairs": len(pairs)}))


def incremental(args, directory):
    clean_source(directory)
    receipt = json.loads((directory / "build-receipt.json").read_text())
    options = OPTIONS + ["--//dockers/docker-lldp:image_version=" + receipt["image_version"]]
    check.require(receipt["status"] == "built", "missing accepted build")
    revision = source_identity()["revision"]
    check.require(receipt["revision"] == revision, "incremental checkout differs from built source")
    labels = [OWNER + name + suffix for name in IMAGES for suffix in ("", ".gz")]
    labels += [PARENT, TARGETS["config-engine-source.tar"], TARGETS["all-debug.tar"],
               TARGETS["lldpd-owner.tar"], TARGETS["dbsyncd-runtime.tar"]]
    paths = {label: ROOT / subprocess.check_output(
        [args.bazel, "cquery", *options, "config(" + label + ", target)", "--output=files"], cwd=ROOT, text=True).strip() for label in labels}

    def snapshot(stage):
        started = time.monotonic()
        with (directory / ("incremental-" + stage + ".log")).open("w") as log:
            subprocess.run([args.bazel, "build", *options, *labels], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
        return {"seconds": time.monotonic() - started, "sha256": {
            label: (oci_layout.validate_layout(path, "linux/amd64").descriptor["digest"] if path.is_dir()
                    else check.sha(path)) for label, path in paths.items()}}

    results = {"baseline": snapshot("baseline"), "noop": snapshot("noop")}
    original = results["baseline"]["sha256"]
    check.require(results["noop"]["sha256"] == original, "no-op changed outputs")
    for name in IMAGES:
        check.require(original[OWNER + name + ".gz"] == receipt["artifacts"][name + ".gz"]["sha256"], "incremental baseline changed")
    source = ROOT / "dockers/docker-lldp/lldpd.conf.j2"
    content = source.read_bytes()
    try:
        source.write_bytes(b"# LLDP incremental probe\n" + content)
        results["edit"] = snapshot("edit")
        changed = {label for label, digest in results["edit"]["sha256"].items() if original[label] != digest}
        check.require(changed == {OWNER + name + suffix for name in IMAGES for suffix in ("", ".gz")},
                      "LLDP config edit changed unrelated source outputs: " + repr(changed))
    finally:
        source.write_bytes(content)
    results["restore"] = snapshot("restore")
    check.require(results["restore"]["sha256"] == original, "restored inputs changed output digests")
    clean_source(directory)
    check.require(source_identity()["revision"] == revision, "incremental checkout changed")
    (directory / "incremental.json").write_text(json.dumps(
        {"status": "passed", "revision": revision, **results}, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phase", choices=("build", "validate", "incremental"))
    parser.add_argument("--bazel", default="bazel")
    parser.add_argument("--image-version", default="bazel-lldp-ci")
    parser.add_argument("--artifacts", type=Path, required=True)
    args = parser.parse_args()
    directory = args.artifacts.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    if args.phase == "validate":
        validate_images(directory)
    elif args.phase == "incremental":
        incremental(args, directory)
    else:
        build_images(args, directory)


if __name__ == "__main__":
    main()
