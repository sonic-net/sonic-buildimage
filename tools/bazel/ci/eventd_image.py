#!/usr/bin/env python3
"""Build Eventd archives, then check completed contents and incremental reuse."""

import argparse
from functools import partial
import json
from pathlib import Path
import platform
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import base_images, bazel_commands, build, command_log, resolution, source_image_worker
from tools.bazel.oci import image_validation, oci_layout

OWNER = "//dockers/docker-eventd:"
IMAGES = ("docker-eventd", "docker-eventd-dbg")
PARENT = "//dockers/docker-config-engine-trixie:docker-config-engine-trixie"
TARGETS = {
    **{name + ".gz": OWNER + name + ".gz" for name in IMAGES},
    "eventd-runtime.tar": "//src/sonic-eventd:eventd_runtime",
    "eventd-debug.tar": "//src/sonic-eventd:eventd_runtime.debug_symbols",
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
CONFIGS = {"start.sh": "usr/bin/start.sh", "eventdb_wrapper.sh": "usr/bin/eventdb_wrapper.sh",
           "supervisord.conf": "etc/supervisor/conf.d/supervisord.conf",
           "critical_processes": "etc/supervisor/critical_processes"}
EVENTS = ("host", "bgp", "dhcp_relay", "swss", "syncd")


def build_images(args, directory):
    check.require(platform.machine() == "x86_64" and
                  platform.freedesktop_os_release().get("VERSION_CODENAME") == "trixie",
                  "Eventd image build requires native AMD64 Debian Trixie")
    receipt = {"status": "building", "architecture": "amd64", "commands": [], "image_version": args.image_version}
    execute = partial(command_log.execute, cwd=ROOT)
    try:
        receipt["revision"] = execute(["git", "rev-parse", "HEAD"], directory, receipt, "revision").strip()
        receipt["commit_headers"] = subprocess.check_output(
            ["git", "cat-file", "-p", "HEAD"], cwd=ROOT, text=True).split("\n\n", 1)[0]
        execute(["make", "-f", "tools/bazel/prepare_manifests.mk", "manifests",
                 "target/bazel-manifests/docker-eventd/image.env",
                 "MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk rules/docker-eventd.mk",
                 "BUILD_WITH_BAZEL_WHEN_AVAILABLE=y", "BLDENV=trixie", "CONFIGURED_PLATFORM=vs",
                 "CONFIGURED_ARCH=amd64", "INCLUDE_FIPS=y", "ENABLE_ASAN=n", "DBG_IMAGE_MARK=dbg",
                 "DOCKERS_PATH=dockers", "SONIC_IMAGE_VERSION=" + args.image_version], directory, receipt, "manifests")
        layouts = [OWNER + name for name in IMAGES] + [PARENT]
        receipt["action_audit"] = bazel_commands.audit_targets(
            args.bazel, OPTIONS, [*TARGETS.values(), *layouts], workspace=ROOT,
            output=directory / "action-audit.json")
        build.collect_archives(ROOT, directory, receipt, TARGETS, bazel=args.bazel, options=OPTIONS)
        execute([args.bazel, "build", *OPTIONS, *layouts], directory, receipt, "layouts")
        receipt["layouts"] = {target: execute(
            [args.bazel, "cquery", *OPTIONS, target, "--output=files"], directory, receipt,
            "layout-" + str(index)).strip() for index, target in enumerate(layouts)}
        receipt["resolution"] = resolution.collect(ROOT, directory, bazel=[args.bazel])
        receipt["status"] = "built"
    except Exception as error:
        receipt.update(status="failed", error=str(error))
        raise
    finally:
        (directory / "build-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


def validate_images(directory):
    receipt = json.loads((directory / "build-receipt.json").read_text())
    check.require(receipt["status"] == "built", "image build did not complete")
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
        replaced = set(CONFIGS.values()) | ({"var/lib/dpkg/status", "etc/group", "etc/gshadow"} if debug else set())
        for path, item in inherited.items():
            if item["kind"] != "directory" and path not in replaced:
                expected = tools.get(path, item) if debug else item
                check.require(all(files.get(path, {}).get(key) == value for key, value in expected.items()),
                              "image replaced an inherited file: " + path)
        if debug:
            base_images.verify_installed_layer(files, directory / "debug-tools.tar")
        base_images.verify_installed_layer(files, directory / "eventd-runtime.tar")
        for source, destination in CONFIGS.items():
            item = files.get(destination, {})
            check.require(item.get("sha256") == check.sha(ROOT / "dockers/docker-eventd" / source)
                          and (item.get("uid"), item.get("gid"), item.get("mode")) ==
                          (0, 0, 0o755 if source.endswith(".sh") else 0o644), "service configuration differs: " + destination)
        rendered = {}
        for event in EVENTS:
            path = "etc/rsyslog.d/rsyslog_plugin_conf/" + event + "_events.conf"
            item = files.get(path, {})
            check.require(item.get("kind") == "file" and
                          (item.get("uid"), item.get("gid"), item.get("mode")) == (0, 0, 0o644),
                          "rendered host configuration missing or has wrong metadata: " + path)
            rendered[path] = item["sha256"]
        check.require(not any(path.startswith("etc/rsyslog.d/rsyslog_plugin_conf/") and
                              path.endswith((".j2", ".json")) for path in files), "build-only inputs leaked into image")
        config = layout.config.get("config", {})
        check.require(config.get("Entrypoint") == ["/usr/local/bin/supervisord"], "entrypoint changed")
        check.require("DEBIAN_FRONTEND=noninteractive" in config.get("Env", []) and
                      "IMAGE_VERSION=" + receipt["image_version"] in config.get("Env", []), "image environment changed")
        expected = json.loads((ROOT / "target/bazel-manifests" / name / "manifest.json").read_text())
        check.require(json.loads(config["Labels"]["com.azure.sonic.manifest"]) == expected, "Make labels differ")
        archive = image_validation.docker_archive(directory / (name + ".gz"), name + ":latest",
                                                  layout.manifest["config"]["digest"], layout.manifest["layers"])
        reports[name] = {"manifest_digest": layout.descriptor["digest"],
                         "config_digest": layout.manifest["config"]["digest"], "rootfs": layout.config["rootfs"],
                         "archive": archive, "rendered_configs": rendered, "parent_payload_preserved": True}
        inventories[name] = files
    runtime, debug = (inventories[name] for name in IMAGES)
    for path, item in runtime.items():
        if "elf_machine" in item:
            expected = tools.get(path, item)
            check.require(all(debug.get(path, {}).get(key) == value for key, value in expected.items()),
                          "debug variant replaced an undeclared runtime ELF: " + path)
    check.require(debug["var/lib/dpkg/status"]["sha256"] == check.sha(directory / "debug.status") and
                  debug["usr/share/sonic/bazel/eventd-debug-packages.json"]["sha256"] == check.sha(directory / "debug-packages.json"),
                  "debug package inventory differs")
    own = check.debug_archives(directory / "eventd-runtime.tar", directory / "eventd-debug.tar", expected_machine=62)
    check.require(len(own["debug_pairs"]) == 3, "Eventd debug coverage differs")
    native = base_images.verify_source_files(debug, [json.loads((directory / name).read_text()) for name in
        ("base-packages.json", "config-engine-packages.json", "debug-packages.json")], 62)
    pairs = base_images.verify_symbols(native, [directory / name for name in
        ("base-source.tar", "config-engine-source.tar", "openssh-source.tar")], directory / "all-debug.tar", directory)
    for path, item in check.payload(directory / "all-debug.tar").items():
        if item["kind"] != "directory":
            check.require(all(debug.get(path, {}).get(key) == value for key, value in item.items())
                          and path not in runtime, "debug symbols differ: " + path)
    check.require(debug["usr/bin/ssh-agent"]["gid"] == 104 and debug["usr/bin/ssh-agent"]["mode"] == 0o2755,
                  "source OpenSSH permissions differ")
    check.require(debug.get("usr/bin/vim", {}).get("kind") == "symlink" and
                  debug["usr/bin/vim"].get("linkname") == "/etc/alternatives/vim",
                  "debug image does not point /usr/bin/vim at /etc/alternatives/vim")
    for name in IMAGES:
        reports[name]["native_paths"] = sorted(set(native).intersection(inventories[name]))
    report = {"status": "passed", "revision": receipt["revision"], "architecture": "amd64", "image_version": receipt["image_version"],
              "images": reports, "source_debug_pairs": pairs, "native": native, "artifacts": receipt["artifacts"]}
    (directory / "validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": "passed", "source_elf_debug_pairs": len(pairs)}))


def require_clean_incremental_source(source, expected_revision):
    relative = str(source.relative_to(ROOT))
    check.require(not subprocess.check_output(
        ["git", "status", "--porcelain", "--", relative], cwd=ROOT, text=True),
        "uncommitted changes in incremental probe input: " + relative)
    subprocess.run(["git", "diff", "--exit-code", "--ignore-submodules=none", "HEAD"], cwd=ROOT, check=True)
    check.require(subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip() == expected_revision,
                  "source revision changed from build receipt")


def incremental(args, directory):
    receipt = json.loads((directory / "build-receipt.json").read_text())
    check.require(receipt["status"] == "built", "missing accepted build")
    source = ROOT / "dockers/docker-eventd/host_events_info.json"
    require_clean_incremental_source(source, receipt["revision"])
    labels = [OWNER + name + suffix for name in IMAGES for suffix in ("", ".gz")]
    labels += [PARENT, TARGETS["eventd-runtime.tar"], TARGETS["all-debug.tar"]]
    paths = {label: ROOT / subprocess.check_output(
        [args.bazel, "cquery", *OPTIONS, label, "--output=files"], cwd=ROOT, text=True).strip() for label in labels}

    def snapshot(stage):
        started = time.monotonic()
        with (directory / ("incremental-" + stage + ".log")).open("w") as log:
            subprocess.run([args.bazel, "build", *OPTIONS, *labels], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
        return {"seconds": time.monotonic() - started, "sha256": {
            label: (oci_layout.validate_layout(path, "linux/amd64").descriptor["digest"] if path.is_dir()
                    else check.sha(path)) for label, path in paths.items()}}

    results = {"baseline": snapshot("baseline"), "noop": snapshot("noop")}
    original = results["baseline"]["sha256"]
    check.require(results["noop"]["sha256"] == original, "no-op changed outputs")
    for name in IMAGES:
        check.require(original[OWNER + name + ".gz"] == receipt["artifacts"][name + ".gz"]["sha256"], "incremental baseline changed")
    content = source.read_bytes()
    data = json.loads(content)
    data["proclist"][0]["name"] = "eventd-ci-incremental"
    edited = (json.dumps(data, indent=2) + "\n").encode()
    try:
        check.require(source.read_bytes() == content, "concurrent host event config change")
        source.write_bytes(edited)
        results["edit"] = snapshot("edit")
        changed = {label for label, digest in results["edit"]["sha256"].items() if original[label] != digest}
        check.require(changed == {OWNER + name + suffix for name in IMAGES for suffix in ("", ".gz")},
                      "host event config edit changed unrelated source outputs: " + repr(changed))
    finally:
        check.require(source.read_bytes() in (content, edited), "concurrent host event config change; refusing overwrite")
        source.write_bytes(content)
    results["restore"] = snapshot("restore")
    check.require(results["restore"]["sha256"] == original, "restored inputs changed output digests")
    require_clean_incremental_source(source, receipt["revision"])
    (directory / "incremental.json").write_text(json.dumps(results, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phase", choices=("build", "validate", "incremental", "publish"))
    parser.add_argument("--bazel", default="bazel")
    parser.add_argument("--image-version", default="bazel-eventd-ci")
    parser.add_argument("--artifacts", type=Path, required=True)
    args = parser.parse_args()
    directory = args.artifacts.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    if args.phase == "validate":
        validate_images(directory)
    elif args.phase == "publish":
        source_image_worker.publish(args)
    elif args.phase == "incremental":
        incremental(args, directory)
    else:
        build_images(args, directory)


if __name__ == "__main__":
    main()
