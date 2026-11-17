#!/usr/bin/env python3
"""Build FRR images, then verify installed payloads and incremental outputs."""

import argparse
from functools import partial
import json
from pathlib import Path
import platform
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import base_images, bazel_commands, build, command_log, resolution
from tools.bazel.ci.sysmgr_image import source_identity
from tools.bazel.oci import image_validation, oci_layout, source_base_validation

OWNER = "//dockers/docker-fpm-frr:"
IMAGES = ("docker-fpm-frr", "docker-fpm-frr-dbg")
BASE = "//dockers/docker-swss-layer-trixie:docker-swss-layer-trixie"
OPTIONS = ["--jobs=2", "--local_resources=cpu=2", "--local_resources=memory=8000",
           "--//src/sonic-frr:snmp=true", "--//src/sonic-frr:tcmalloc=true",
           "--platforms=@sonic_build_infra//platforms:x86_64_trixie",
           "--host_platform=@sonic_build_infra//platforms:x86_64_trixie"]
TARGETS = {
    **{name + ".gz": OWNER + name + ".gz" for name in IMAGES},
    "test-redis.tar": OWNER + "test_redis",
    "frr.debug.tar": OWNER + "all_debug_symbols",
    "frr.debug-tools.tar": OWNER + "debug_tools",
    "frr.source.tar": OWNER + "source_runtime",
    "frr.python.tar": OWNER + "python_runtime",
    "frr.files.tar": OWNER + "files",
    "frr.runtime-apt.tar": OWNER + "debian_runtime_payload",
    "frr.packages.json": OWNER + "runtime_package_metadata",
    "frr.status": OWNER + "installed_packages",
    "frr-debug.packages.json": OWNER + "debug_package_metadata",
    "frr-debug.status": OWNER + "debug_installed_packages",
    "base.packages.json": "//dockers/docker-base-trixie:runtime_package_metadata",
    "base.source.tar": "//dockers/docker-base-trixie:source_runtime",
    "config-engine.packages.json": "//dockers/docker-config-engine-trixie:runtime_package_metadata",
    "config-engine.source.tar": "//dockers/docker-config-engine-trixie:source_runtime",
    "swss.source.tar": "//dockers/docker-swss-layer-trixie:source_runtime",
    "openssh.source.tar": OWNER + "debug_client",
}


def build_images(args, directory):
    check.require(platform.machine() == "x86_64" and
                  platform.freedesktop_os_release().get("VERSION_CODENAME") == "trixie",
                  "FRR image acceptance requires native AMD64/Trixie")
    receipt = {"status": "building", "architecture": "amd64", "commands": []}
    options = [*OPTIONS, "--//dockers/docker-fpm-frr:image_version=" + args.image_version]
    receipt["image_version"] = args.image_version
    execute = partial(command_log.execute, cwd=ROOT)
    try:
        receipt.update(source_identity())
        execute(["make", "-f", "tools/bazel/prepare_manifests.mk",
                 "MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk rules/docker-swss-layer-trixie.mk rules/docker-fpm-frr.mk",
                 "BUILD_WITH_BAZEL_WHEN_AVAILABLE=y", "BLDENV=trixie", "CONFIGURED_PLATFORM=vs",
                 "CONFIGURED_ARCH=amd64", "INCLUDE_FIPS=y", "ENABLE_ASAN=n", "DBG_IMAGE_MARK=dbg",
                 "ENABLE_FRR_TCMALLOC=y", "ENABLE_FRR_SNMP_AGENT=y", "FRR_USER_UID=300", "FRR_USER_GID=300",
                 "DOCKERS_PATH=dockers"], directory, receipt, "manifests")
        layouts = [BASE, *[OWNER + name for name in IMAGES]]
        receipt["action_audit"] = bazel_commands.audit_targets(args.bazel, options, [*TARGETS.values(), *layouts],
            workspace=ROOT, output=directory / "action-audit.json")
        build.collect_archives(ROOT, directory, receipt, TARGETS, bazel=args.bazel, options=options)
        # Remote/cache hits for compressed archives need not materialize the
        # layouts inspected below. Request those outputs explicitly.
        execute([args.bazel, "build", *options, *layouts], directory, receipt, "layouts")
        receipt["layouts"] = {name: execute([args.bazel, "cquery", *options, "--output=files", OWNER + name],
            directory, receipt, name + ".layout").strip() for name in IMAGES}
        receipt["base_layout"] = execute([args.bazel, "cquery", *options, "--output=files", BASE],
            directory, receipt, "base.layout").strip()
        receipt["resolution"] = resolution.collect(ROOT, directory, bazel=[args.bazel])
        receipt["status"] = "built"
    except Exception as error:
        receipt.update(status="failed", error=str(error))
        raise
    finally:
        (directory / "build-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


def validate_images(directory):
    receipt = json.loads((directory / "build-receipt.json").read_text())
    check.require(receipt["revision"] == source_identity()["revision"], "checkout differs from the image build")
    check.require(receipt["status"] == "built", "FRR image build did not finish")
    for name, artifact in receipt["artifacts"].items():
        check.require(check.sha(directory / name) == artifact["sha256"], "artifact changed: " + name)
    parent = oci_layout.validate_layout(ROOT / receipt["base_layout"], "linux/amd64")
    inherited = base_images.installed_inventory(parent)
    configuration = check.payload(directory / "frr.files.tar", require_root=False)
    source = check.payload(directory / "frr.source.tar")
    python = check.payload(directory / "frr.python.tar")
    runtime_apt = check.payload(directory / "frr.runtime-apt.tar", require_root=False)
    expected = runtime_apt | source | python | configuration
    debug_tools = check.payload(directory / "frr.debug-tools.tar", require_root=False)
    parent_packages = json.loads((directory / "config-engine.packages.json").read_text())["dependency_check"]["packages"]
    inventories = {}
    for name in IMAGES:
        layout = oci_layout.validate_layout(ROOT / receipt["layouts"][name], "linux/amd64")
        base_images.verify_unique_layer_paths(layout)
        files = base_images.installed_inventory(layout)
        base_images.verify_directory_aliases(files)
        for path, item in expected.items():
            if item["kind"] == "directory" and path not in configuration:
                continue
            if name.endswith("-dbg") and path in runtime_apt and path not in source and path not in python and path not in configuration:
                item = debug_tools.get(path, item)
            check.require(all(files.get(path, {}).get(key) == value for key, value in item.items()),
                          "installed APT, source or configuration payload changed: " + path)
        for path, item in inherited.items():
            if item["kind"] == "directory" or path in ("etc/passwd", "etc/group", "var/lib/dpkg/status"):
                continue
            # Docker-owned COPY/template paths may intentionally replace an
            # inherited file; preserve every other parent byte and metadata.
            # Both legacy setup.py files use find_packages(), including their
            # top-level tests package. Installing those wheels replaces this
            # one inherited package initializer, just as pip does in Make's
            # Dockerfile. Require the exact owner TAR bytes checked above;
            # no other Python or parent-file replacement is permitted.
            if path == "usr/local/lib/python3.13/dist-packages/tests/__init__.py":
                expected_item = python[path]
            else:
                expected_item = item
            expected_item = configuration.get(path, expected_item)
            if name.endswith("-dbg"):
                expected_item = debug_tools.get(path, expected_item)
            check.require(all(files.get(path, {}).get(key) == value for key, value in expected_item.items()),
                          "FRR changed inherited SWSS/base payload: " + path)
        if name.endswith("-dbg"):
            for path, item in debug_tools.items():
                if item["kind"] != "directory":
                    check.require(all(files.get(path, {}).get(key) == value for key, value in item.items()),
                                  "debug tool payload changed: " + path)
        prefix = "frr" if name == "docker-fpm-frr" else "frr-debug"
        for path, artifact in (("var/lib/dpkg/status", prefix + ".status"),
                               ("usr/share/sonic/bazel/" + prefix + "-packages.json", prefix + ".packages.json")):
            check.require(files.get(path, {}).get("sha256") == check.sha(directory / artifact),
                          "installed package state differs from checked input: " + path)
        check.require(all(item.get("elf_machine", 62) == 62 for item in files.values()), "image contains a non-AMD64 ELF")
        config = layout.config.get("config", {})
        check.require(config.get("Entrypoint") == ["/usr/bin/docker_init.sh"], "FRR entrypoint changed")
        check.require("DEBIAN_FRONTEND=noninteractive" in config.get("Env", []), "missing FRR environment")
        check.require(config.get("ExposedPorts", {}) == parent.config.get("config", {}).get("ExposedPorts", {}),
                      "FRR unexpectedly changed inherited exposed ports")
        labels = config["Labels"]
        check.require(labels.get("Tag") == receipt["image_version"], "Make image version label differs: " + name)
        for component in ("libswsscommon", "sonic-supervisord-utilities-rs"):
            check.require(labels.get("com.azure.sonic.versions." + component) == parent_packages[component]["Version"],
                          "inherited component label differs: " + component)
        manifest = json.loads(labels["com.azure.sonic.manifest"])
        check.require(manifest == json.loads((ROOT / "target/bazel-manifests" / name / "manifest.json").read_text()),
                      "FRR Make metadata label differs")
        selected, origins = source_base_validation.inventory(layout)
        with tempfile.TemporaryDirectory(prefix="frr-accounts-") as temporary:
            root = Path(temporary)
            source_base_validation.extract_selected(layout, selected, origins, {"etc/passwd", "etc/group"}, root)
            users = (root / "etc/passwd").read_text().splitlines()
            groups = (root / "etc/group").read_text().splitlines()
            check.require(any(line.startswith("frr:x:300:300:") and line.endswith(":/home/frr:/bin/false") for line in users),
                          "FRR service account changed")
            check.require("frr:x:300:" in groups and "frrvty:x:102:frr" in groups, "FRR groups changed")
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
               ("base.packages.json", "config-engine.packages.json", "frr-debug.packages.json")]
    native = base_images.verify_source_files(debug, reports, 62)
    runtimes = [directory / name for name in
                ("base.source.tar", "config-engine.source.tar", "swss.source.tar", "frr.source.tar", "openssh.source.tar")]
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
    pairs = base_images.verify_symbols(native, runtimes, directory / "frr.debug.tar", directory)
    check.require(debug["usr/bin/ssh-agent"]["gid"] == 104 and debug["usr/bin/ssh-agent"]["mode"] == 0o2755,
                  "debug FIPS OpenSSH client permissions differ")
    report = {"status": "passed", "revision": receipt["revision"], "architecture": "amd64",
              "tested_revision_parents": receipt["tested_revision_parents"], "image_version": receipt["image_version"],
              "source_debug_pairs": pairs, "native": native,
              "runtime_native": {path: item for path, item in native.items() if path in runtime},
              "artifacts": receipt["artifacts"]}
    (directory / "validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": "passed", "source_elf_debug_pairs": len(pairs)}))


def incremental(args, directory):
    baseline = json.loads((directory / "build-receipt.json").read_text())
    check.require(baseline["status"] == "built" and baseline["revision"] == source_identity()["revision"],
                  "incremental checkout differs from the completed build")
    options = [*OPTIONS, "--//dockers/docker-fpm-frr:image_version=" + baseline["image_version"]]

    def output(label):
        return ROOT / subprocess.check_output([args.bazel, "cquery", *options, "--output=files", label], cwd=ROOT, text=True).strip()
    archives = {name: output(OWNER + name + ".gz") for name in IMAGES}
    owners = {name: output(TARGETS[name]) for name in ("frr.source.tar", "frr.python.tar", "swss.source.tar")}
    base = output(BASE)
    layouts = {name: ROOT / path for name, path in baseline["layouts"].items()}
    targets = [OWNER + name + suffix for name in IMAGES for suffix in ("", ".gz")]
    targets += [BASE, *[TARGETS[name] for name in owners]]
    command = [args.bazel, "build", *options, *targets]
    def snapshot():
        return {"images": {name: oci_layout.validate_layout(path, "linux/amd64").descriptor["digest"] for name, path in layouts.items()},
                "archives": {name: check.sha(path) for name, path in archives.items()},
                "owners": {name: check.sha(path) for name, path in owners.items()},
                "base": oci_layout.validate_layout(base, "linux/amd64").descriptor["digest"]}
    subprocess.run(command, cwd=ROOT, check=True)
    original = snapshot()
    check.require(all(original["archives"][name] == baseline["artifacts"][name + ".gz"]["sha256"] for name in IMAGES),
                  "incremental baseline differs from verified archives")
    subprocess.run(command, cwd=ROOT, check=True)
    unchanged = snapshot()
    check.require(unchanged == original, "no-op build changed installed source/image bytes")
    source = ROOT / "dockers/docker-fpm-frr/frr/frr.conf.j2"
    saved = source.read_bytes()
    try:
        source.write_bytes(saved + b"\n{# Bazel incremental image input probe #}\n")
        subprocess.run(command, cwd=ROOT, check=True)
        changed = snapshot()
        check.require(all(changed[field][name] != original[field][name] for field in ("images", "archives") for name in IMAGES),
                      "FRR template edit did not invalidate both images and archives")
        check.require(changed["owners"] == original["owners"] and changed["base"] == original["base"],
                      "FRR image input edit changed an unrelated source owner or base")
    finally:
        source.write_bytes(saved)
    subprocess.run(command, cwd=ROOT, check=True)
    restored = snapshot()
    check.require(restored == original, "restoring the template did not reproduce exact image/archive bytes")
    (directory / "incremental.json").write_text(json.dumps({"status": "passed", "input": str(source.relative_to(ROOT)),
        "baseline": original, "unchanged": unchanged, "changed": changed, "restored": restored}, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phase", choices=("build", "validate", "incremental"))
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--image-version", default="bazel-frr-ci")
    parser.add_argument("--bazel", default="tools/bazel/ci/libteam_draft_bazel.sh")
    args = parser.parse_args()
    directory = args.artifacts.resolve(); directory.mkdir(parents=True, exist_ok=True)
    if args.phase == "build": build_images(args, directory)
    elif args.phase == "validate": validate_images(directory)
    else: incremental(args, directory)


if __name__ == "__main__":
    main()
