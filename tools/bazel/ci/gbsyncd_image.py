#!/usr/bin/env python3
"""Build gearbox archives and validate their original package and image contract."""

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
from tools.bazel.ci import bazel_commands, build, command_log, resolution
from tools.bazel.oci import image_validation, native_packages, oci_layout, source_base_validation
from tools.bazel.tests.integration.syncd_p4_images_test import check_p4_inputs

OWNER = "//platform/vs/docker-gbsyncd-vs:"
SHARED = "//platform/vs/docker-syncd-vs:"
BASE = "//dockers/docker-config-engine-trixie:docker-config-engine-trixie"
IMAGES = ("docker-gbsyncd-vs", "docker-gbsyncd-vs-dbg")
OPTIONS = [build.MAKE_METADATA_OPTION, "--config=syncd-dash"]
TARGETS = {
    "source-excludes.txt": "//tools/bazel/dpkg:dpkg_excludes.txt",
    "source-includes.txt": "//tools/bazel/dpkg:oci_includes",
    **{name + ".gz": OWNER + name + ".gz" for name in IMAGES},
    "source-runtime.tar": OWNER + "source_runtime",
    "source-debug.tar": SHARED + "source_debug_payloads",
    "source-contract.json": OWNER + "source_contract",
    "source-receipt.json": OWNER + "source_package_receipt",
    "policy.json": OWNER + "package_policy",
    "config-files.tar": "//platform/vs/docker-gbsyncd-vs/config:files",
    "apt.lock.json": SHARED + "ci_apt_lock",
    "runtime-apt.tar": OWNER + "runtime_layer",
    "runtime-apt.json": OWNER + "runtime_apt_selection",
    "debug-tools.tar": OWNER + "debug_tools",
    "debug-apt.json": OWNER + "debug_apt_selection",
    "installation-links.tar": SHARED + "installation_links",
    "runtime-packages.json": OWNER + "runtime_package_metadata",
    "runtime.status": OWNER + "installed_packages",
    "debug-packages.json": OWNER + "debug_package_metadata",
    "debug.status": OWNER + "debug_installed_packages",
    "base-debug.tar": BASE + ".debug_symbols",
    "base-packages.json": "//dockers/docker-base-trixie:runtime_package_metadata",
    "config-engine-packages.json": "//dockers/docker-config-engine-trixie:runtime_package_metadata",
    "config-engine-source.tar": "//dockers/docker-config-engine-trixie:source_runtime",
    "test-redis.tar": OWNER + "test_redis",
}


def clean_source(directory):
    check.require(not ROOT.is_relative_to(directory), "artifact directory contains checkout")
    subprocess.run(["git", "diff", "--exit-code", "--ignore-submodules=none", "HEAD"], cwd=ROOT, check=True)
    untracked = subprocess.check_output(["git", "ls-files", "--others", "--exclude-standard", "-z"], cwd=ROOT).decode().split("\0")
    unexpected = [name for name in untracked if name and not (ROOT / name).is_relative_to(directory)]
    check.require(not unexpected, "commit all source inputs before acceptance: " + repr(unexpected))


def identity():
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    headers = subprocess.check_output(["git", "cat-file", "-p", "HEAD"], cwd=ROOT, text=True).split("\n\n", 1)[0]
    parents = [line.split()[1] for line in headers.splitlines() if line.startswith("parent ")]
    if os.environ.get("CI_HEAD_SHA"):
        check.require(os.environ["CI_HEAD_SHA"] in [revision, *parents], "CI source differs from requested PR head")
    if os.environ.get("CI_BASE_SHA"):
        check.require(os.environ["CI_BASE_SHA"] in parents, "CI merge differs from requested PR base")
    return {"revision": revision, "tested_revision_parents": parents}


def validate_groups(base_layout, runtime_layout, debug_layout):
    rows, metadata = [], []
    for layout in (base_layout, runtime_layout, debug_layout):
        files, origins = image_validation.image_inventory(layout)
        with tempfile.TemporaryDirectory(prefix="gbsyncd-accounts-") as temporary:
            root = Path(temporary)
            source_base_validation.extract_selected(layout, files, origins, {"etc/group"}, root)
            records = (root / "etc/group").read_text().splitlines()
        check.require(len(records) == len({row.split(":", 1)[0] for row in records}),
                      "image has duplicate group names")
        rows.append(set(records))
        metadata.append({key: files["etc/group"][key] for key in ("kind", "mode", "uid", "gid")})
    check.require(rows[1] == rows[0] | {"_ssh:x:104:"}, "runtime account merge changed existing groups")
    check.require(rows[2] == rows[1], "debug image changed runtime groups")
    check.require(metadata[0] == metadata[1] == metadata[2], "group ownership or permissions changed")
    return {"base_groups": len(rows[0]), "runtime_groups": len(rows[1]), "debug_groups": len(rows[2]),
            "declared_addition": "_ssh:x:104:"}


def build_images(args, directory):
    check.require(platform.machine() == "x86_64" and platform.freedesktop_os_release().get("VERSION_CODENAME") == "trixie",
                  "gearbox acceptance requires native AMD64/Trixie")
    receipt = {"status": "building", "architecture": "amd64", "image_version": args.image_version, "commands": []}
    options = [*OPTIONS, build.image_version_option(args.image_version)]
    execute = partial(command_log.execute, cwd=ROOT)
    try:
        clean_source(directory)
        receipt.update(identity())
        execute(["make", "-s", "-f", "tools/bazel/prepare_manifests.mk",
                 "MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk platform/vs/docker-gbsyncd-vs.mk",
                 "BUILD_WITH_BAZEL_WHEN_AVAILABLE=y", "BLDENV=trixie", "CONFIGURED_PLATFORM=vs", "CONFIGURED_ARCH=amd64",
                 "INCLUDE_FIPS=y", "INCLUDE_VS_DASH_SAI=y", "ENABLE_ASAN=n", "DBG_IMAGE_MARK=dbg", "DOCKERS_PATH=dockers",
                 "PLATFORM_PATH=platform/vs", "SONIC_IMAGE_VERSION=" + args.image_version,
                 "manifests", "target/bazel-manifests/docker-gbsyncd-vs/image.env"], directory, receipt, "manifests")
        layouts = [BASE, *[OWNER + name for name in IMAGES]]
        receipt["action_audit"] = bazel_commands.audit_targets(args.bazel, options, [*TARGETS.values(), *layouts],
            workspace=ROOT, output=directory / "action-audit.json")
        # Retain completed independent source actions even if another target fails.
        build_options = [*options, "--keep_going"]
        build.collect_archives(ROOT, directory, receipt, TARGETS, bazel=args.bazel, options=build_options)
        execute([args.bazel, "build", *build_options, *layouts], directory, receipt, "layouts")
        receipt["layouts"] = {name: execute([args.bazel, "cquery", *options, "--output=files", OWNER + name],
            directory, receipt, name + ".layout").strip() for name in IMAGES}
        receipt["base_layout"] = execute([args.bazel, "cquery", *options, "--output=files", BASE], directory, receipt, "base.layout").strip()
        receipt["resolution"] = resolution.collect(ROOT, directory, bazel=[args.bazel])
        clean_source(directory)
        check.require(identity()["revision"] == receipt["revision"], "source changed during build")
        receipt["status"] = "built"
    except Exception as error:
        receipt.update(status="failed", error=str(error))
        raise
    finally:
        (directory / "build-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


def declared_image_policy(policy, image_version, environment_file):
    check.require(isinstance(image_version, str) and image_version and "\n" not in image_version,
                  "missing or malformed recorded image version")
    expected = {"DEBIAN_FRONTEND": "noninteractive", "IMAGE_VERSION": image_version}
    lines = environment_file.read_text().splitlines()
    check.require(len(lines) == len(expected) and all("=" in line for line in lines),
                  "Make image environment has missing or extra entries")
    declared = dict(line.split("=", 1) for line in lines)
    check.require(declared == expected, "Make image environment differs from the recorded build input")
    settings = policy["image_validation"]
    check.require(settings.get("environment") == {"DEBIAN_FRONTEND": "noninteractive"},
                  "package policy changed its declared public environment")
    return {**policy, "image_validation": {**settings, "environment": expected}}


def validate_images(directory):
    clean_source(directory)
    receipt = json.loads((directory / "build-receipt.json").read_text())
    check.require(receipt["status"] == "built" and identity()["revision"] == receipt["revision"], "build/source identity differs")
    check.require(set(receipt["artifacts"]) == set(TARGETS), "incomplete artifact inventory")
    for name, artifact in receipt["artifacts"].items():
        check.require(check.sha(directory / name) == artifact["sha256"], "artifact changed: " + name)
    runtime, debug = [ROOT / receipt["layouts"][name] for name in IMAGES]
    accounts = validate_groups(*[oci_layout.validate_layout(path, "linux/amd64") for path in
        (ROOT / receipt["base_layout"], runtime, debug)])
    policy = declared_image_policy(json.loads((directory / "policy.json").read_text()),
        receipt["image_version"], ROOT / "target/bazel-manifests/docker-gbsyncd-vs/image.env")
    source = {"source_contract_path": directory / "source-contract.json", "source_runtime_tar": directory / "source-runtime.tar",
              "source_debug_tar": directory / "source-debug.tar", "source_receipt": directory / "source-receipt.json",
              "base_path": ROOT / receipt["base_layout"]}
    p4 = check_p4_inputs(source["source_contract_path"], source["source_receipt"])
    images = image_validation.validate_images(runtime, debug, None, None,
        ROOT / "target/bazel-manifests/docker-gbsyncd-vs/manifest.json",
        ROOT / "target/bazel-manifests/docker-gbsyncd-vs-dbg/manifest.json", ROOT, policy=policy, **source,
        source_layer_excludes=directory / "source-excludes.txt", source_layer_includes=directory / "source-includes.txt",
        apt_lock_path=directory / "apt.lock.json",
        apt_layer=directory / "runtime-apt.tar", debug_tools_layer=directory / "debug-tools.tar",
        runtime_links_layer=directory / "installation-links.tar", runtime_apt_selection=directory / "runtime-apt.json",
        debug_apt_selection=directory / "debug-apt.json", runtime_archive=directory / (IMAGES[0] + ".gz"),
        debug_archive=directory / (IMAGES[1] + ".gz"))
    own = native_packages.validate_native(None, None, policy=policy, readelf="readelf", objcopy="objcopy", **source)
    inherited = source_base_validation.validate_source_base(runtime, debug,
        metadata_paths=[directory / "base-packages.json", directory / "config-engine-packages.json"],
        source_debug_tar=directory / "base-debug.tar", architecture="amd64", workspace_module=ROOT / "MODULE.bazel")
    pairs = {item["path"]: item for item in [*inherited["paired_symbols"], *own["paired_symbols"]]}
    inventories, summaries = {}, {}
    for name in IMAGES:
        layout = oci_layout.validate_layout(ROOT / receipt["layouts"][name], "linux/amd64")
        files, _ = image_validation.image_inventory(layout)
        variant = "debug" if name.endswith("-dbg") else "runtime"
        for path, artifact in [("var/lib/dpkg/status", variant + ".status"),
                               ("usr/share/sonic/bazel/gbsyncd" + ("-debug" if variant == "debug" else "") + "-packages.json", variant + "-packages.json")]:
            check.require(files[path]["sha256"] == check.sha(directory / artifact), "installed package state differs: " + path)
        check.require(files["usr/bin/gbsyncdmgrd"]["mode"] == 0o755, "installed gearbox manager is not executable")
        check.require(layout.config["config"].get("Labels", {}).get("Tag") == receipt["image_version"],
                      "Make image version label differs: " + name)
        env = dict(value.split("=", 1) for value in layout.config["config"]["Env"])
        check.require(env.get("IMAGE_VERSION") == receipt["image_version"], "Make image version was lost")
        inventories[name] = files
        summaries[name] = {"config_digest": layout.manifest["config"]["digest"], "rootfs": layout.config["rootfs"],
                           "manifest_digest": layout.descriptor["digest"], "native_paths": sorted(pairs)}
    native = {path: inventories[IMAGES[0]][path] for path in pairs}
    result = {"status": "passed", **identity(), "architecture": "amd64", "image_version": receipt["image_version"],
              "images": summaries, "native": native, "source_debug_pairs": list(pairs.values()),
              "accounts": accounts,
              "package_images": images, "source_native": own, "inherited_source": inherited, "p4": p4,
              "artifacts": receipt["artifacts"]}
    (directory / "validation.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"status": "passed", "source_elf_debug_pairs": len(pairs)}))


def incremental(args, directory):
    receipt = json.loads((directory / "build-receipt.json").read_text())
    options = [*OPTIONS, build.image_version_option(receipt["image_version"])]
    clean_source(directory)
    check.require(identity()["revision"] == receipt["revision"] and receipt["status"] == "built", "source/build identity differs")
    labels = [OWNER + name + suffix for name in IMAGES for suffix in ("", ".gz")]
    labels += [BASE, OWNER + "source_runtime", OWNER + "all_debug_symbols"]
    paths = {label: ROOT / subprocess.check_output([args.bazel, "cquery", *options, "--output=files", label], cwd=ROOT, text=True).strip() for label in labels}
    def snapshot(stage):
        start = time.monotonic()
        with (directory / ("incremental-" + stage + ".log")).open("w") as log:
            subprocess.run([args.bazel, "build", *options, *labels], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
        return {"seconds": time.monotonic() - start, "sha256": {label: (
            oci_layout.validate_layout(path, "linux/amd64").descriptor["digest"] if path.is_dir() else check.sha(path)) for label, path in paths.items()}}
    result = {"baseline": snapshot("baseline"), "noop": snapshot("noop")}
    check.require(result["baseline"]["sha256"] == result["noop"]["sha256"], "no-op changed outputs")
    for name in IMAGES:
        check.require(result["baseline"]["sha256"][OWNER + name + ".gz"] == receipt["artifacts"][name + ".gz"]["sha256"], "baseline archive differs")
    path = ROOT / "platform/vs/docker-gbsyncd-vs/supervisord.conf"
    original = path.read_bytes(); changed = b"# gearbox incremental probe\n" + original
    try:
        path.write_bytes(changed)
        result["edit"] = snapshot("edit")
        changed_labels = {label for label, digest in result["edit"]["sha256"].items() if digest != result["baseline"]["sha256"][label]}
        check.require(changed_labels == {OWNER + name + suffix for name in IMAGES for suffix in ("", ".gz")}, "config edit changed unrelated source outputs")
    finally:
        check.require(path.read_bytes() in (original, changed), "concurrent config edit; refusing restoration")
        path.write_bytes(original)
    result["restore"] = snapshot("restore")
    check.require(result["restore"]["sha256"] == result["baseline"]["sha256"], "restoration changed output bytes")
    clean_source(directory)
    (directory / "incremental.json").write_text(json.dumps(result, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phase", choices=("build", "validate", "incremental"))
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--bazel", default="bazel")
    parser.add_argument("--image-version", default="bazel-gbsyncd-ci")
    args = parser.parse_args(); directory = args.artifacts.resolve(); directory.mkdir(parents=True, exist_ok=True)
    if args.phase == "build": build_images(args, directory)
    elif args.phase == "validate": validate_images(directory)
    else: incremental(args, directory)

if __name__ == "__main__":
    main()
