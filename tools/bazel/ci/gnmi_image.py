#!/usr/bin/env python3
"""Build gNMI images, then validate completed source and OCI artifacts."""

import argparse
from functools import partial
import hashlib
import importlib.util
import json
from pathlib import Path
import platform
import subprocess
import sys
import time
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import base_images, bazel_commands, build, command_log, resolution
from tools.bazel.oci import image_validation, oci_layout, source_base_validation

OWNER = "//dockers/docker-sonic-gnmi:"
IMAGES = ("docker-sonic-gnmi", "docker-sonic-gnmi-dbg")
PARENT = "//dockers/docker-config-engine-trixie:docker-config-engine-trixie"
TARGETS = {
    **{name + ".gz": OWNER + name + ".gz" for name in IMAGES},
    "source-runtime.tar": OWNER + "source_runtime",
    "gnmi-runtime.tar": "@sonic_gnmi//dist:runtime",
    "gnmi-debug.tar": "@sonic_gnmi//dist:runtime.debug_symbols",
    "common-runtime.tar": "@sonic_mgmt_common//dist:runtime",
    "all-debug.tar": OWNER + "all_debug_symbols",
    "debug-tools.tar": OWNER + "debug_tools",
    "test-redis.tar": OWNER + "test_redis",
    "gnmi-installed-probe.py": OWNER + "installed_probe",
    "base-source.tar": "//dockers/docker-base-trixie:source_runtime",
    "base-packages.json": "//dockers/docker-base-trixie:runtime_package_metadata",
    "config-engine-source.tar": "//dockers/docker-config-engine-trixie:source_runtime",
    "config-engine-packages.json": "//dockers/docker-config-engine-trixie:runtime_package_metadata",
    "openssh-source.tar": OWNER + "debug_client",
    "runtime-apt.json": OWNER + "runtime_apt_selection",
    "debug-apt.json": OWNER + "debug_apt_selection",
    "packages.json": OWNER + "runtime_package_metadata",
    "installed.status": OWNER + "installed_packages",
    "debug-packages.json": OWNER + "debug_package_metadata",
    "debug.status": OWNER + "debug_installed_packages",
}
OPTIONS = ["--jobs=2", "--local_resources=cpu=2", "--local_resources=memory=10000"]


def source_revision():
    check.require(not subprocess.check_output(
        ["git", "status", "--porcelain"], cwd=ROOT, text=True),
        "Commit image inputs before acceptance")
    return subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()


def profile_options(enabled):
    return [*OPTIONS, "--@sonic_gnmi//tools/bazel:enable_dialout=" + ("true" if enabled else "false")]


def build_images(args, directory):
    check.require(platform.machine() == "x86_64" and
                  platform.freedesktop_os_release().get("VERSION_CODENAME") == "trixie",
                  "gNMI image build requires native AMD64 Debian Trixie")
    receipt = {"status": "building", "architecture": "amd64", "commands": [],
               "dialout_enabled": args.dialout == "enabled", "image_version": args.image_version}
    options = profile_options(receipt["dialout_enabled"])
    execute = partial(command_log.execute, cwd=ROOT)
    try:
        receipt["revision"] = source_revision()
        receipt["commit_headers"] = subprocess.check_output(
            ["git", "cat-file", "-p", "HEAD"], cwd=ROOT, text=True).split("\n\n", 1)[0]
        execute(["make", "-f", "tools/bazel/prepare_manifests.mk",
                 "MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk rules/docker-gnmi.mk",
                 "BUILD_WITH_BAZEL_WHEN_AVAILABLE=y", "BLDENV=trixie", "CONFIGURED_PLATFORM=vs",
                 "CONFIGURED_ARCH=amd64", "INCLUDE_FIPS=y", "ENABLE_ASAN=n", "DBG_IMAGE_MARK=dbg",
                 "DOCKERS_PATH=dockers", "ENABLE_TRANSLIB_WRITE=y", "ENABLE_NATIVE_WRITE=y",
                 "ENABLE_DIALOUT=" + ("y" if args.dialout == "enabled" else "n"),
                 "SONIC_IMAGE_VERSION=" + args.image_version,
                 "manifests", "target/bazel-manifests/docker-sonic-gnmi/image.env"], directory, receipt, "manifests")
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
        receipt["status"] = "built"
    except Exception as error:
        receipt.update(status="failed", error=str(error))
        raise
    finally:
        (directory / "build-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


def verify_groups(runtime, debug, runtime_rows, debug_rows):
    for rows in (runtime_rows, debug_rows):
        names = [row.split(":", 1)[0] for row in rows]
        check.require(len(names) == len(set(names)), "duplicate group name")
    expected = runtime_rows + ([] if "_ssh:x:104:" in runtime_rows else ["_ssh:x:104:"])
    check.require(debug_rows == expected, "debug account merge changed existing group rows")
    check.require(all(debug["etc/group"][key] == runtime["etc/group"][key]
                      for key in ("kind", "uid", "gid", "mode")), "debug group metadata changed")
    for path in ("etc/passwd", "etc/gshadow"):
        check.require(debug.get(path) == runtime.get(path), "debug changed account state: " + path)


def group_rows(layout):
    files, origins = source_base_validation.inventory(layout)
    with tempfile.TemporaryDirectory(prefix="gnmi-accounts-") as temporary:
        root = Path(temporary)
        source_base_validation.extract_selected(layout, files, origins, {"etc/group"}, root)
        return (root / "etc/group").read_text().splitlines()


def verify_config(config, parent, manifest, image_version, *, runtime):
    expected = dict(parent)
    if runtime:
        expected["Entrypoint"] = ["/usr/local/bin/supervisord"]
        # Declaring an entrypoint resets Cmd in the selected rules_oci version.
        expected.pop("Cmd", None)
    def environment(values):
        pairs = [value.split("=", 1) for value in values]
        check.require(all(len(pair) == 2 for pair in pairs) and
                      len({pair[0] for pair in pairs}) == len(pairs), "invalid or duplicate image environment")
        return dict(pairs)
    env = environment(parent.get("Env", []))
    if runtime:
        env.update(DEBIAN_FRONTEND="noninteractive", IMAGE_VERSION=image_version)
    check.require(environment(config.get("Env", [])) == env, "undeclared image environment change")
    expected.pop("Env", None)
    expected["Labels"] = {**parent.get("Labels", {}),
                          "com.azure.sonic.manifest": config.get("Labels", {}).get("com.azure.sonic.manifest")}
    check.require(json.loads(expected["Labels"]["com.azure.sonic.manifest"]) == manifest, "Make labels differ")
    check.require({key: value for key, value in config.items() if key != "Env"} == expected,
                  "undeclared inherited OCI configuration change")


def validate_images(directory):
    receipt = json.loads((directory / "build-receipt.json").read_text())
    check.require(receipt["status"] == "built", "image build did not complete")
    check.require(source_revision() == receipt["revision"], "source changed after image build")
    for name, item in receipt["artifacts"].items():
        check.require(check.sha(directory / name) == item["sha256"], "artifact changed: " + name)
    parent = oci_layout.validate_layout(ROOT / receipt["layouts"][PARENT], "linux/amd64")
    inherited = base_images.installed_inventory(parent)
    debug_tools = check.payload(directory / "debug-tools.tar", require_root=False)
    reports, inventories, layouts = {}, {}, {}
    configs = {"start.sh": "usr/bin/start.sh", "gnmi-native.sh": "usr/bin/gnmi-native.sh",
               "telemetry_vars.j2": "usr/share/sonic/templates/telemetry_vars.j2",
               "critical_processes": "etc/supervisor/critical_processes",
               "supervisord.conf": "etc/supervisor/conf.d/supervisord.conf"}
    if receipt["dialout_enabled"]:
        configs["dialout.sh"] = "usr/bin/dialout.sh"
    expected_config = {destination: (ROOT / "dockers/docker-sonic-gnmi" / source).read_bytes()
                       for source, destination in configs.items()}
    expected_config["etc/rsyslog.conf"] = (ROOT / "files/image_config/rsyslog/rsyslog-container-gnmi.conf").read_bytes()
    if not receipt["dialout_enabled"]:
        spec = importlib.util.spec_from_file_location("gnmi_supervisor", ROOT / "dockers/docker-sonic-gnmi/config/render_supervisor.py")
        render = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(render)
        path = "etc/supervisor/conf.d/supervisord.conf"
        expected_config[path] = render.without_dialout(expected_config[path].decode()).encode()
    for name in IMAGES:
        layout = oci_layout.validate_layout(ROOT / receipt["layouts"][OWNER + name], "linux/amd64")
        layouts[name] = layout
        base_images.verify_unique_layer_paths(layout)
        files = base_images.installed_inventory(layout)
        base_images.verify_directory_aliases(files)
        check.require(all(item.get("elf_machine", 62) == 62 for item in files.values()), "non-AMD64 ELF")
        # The service owns these startup files and its new package inventory.
        # Debug tools also add their explicitly declared _ssh group.
        changed = set(expected_config) | {"var/lib/dpkg/status"}
        if name.endswith("-dbg"):
            changed.add("etc/group")
        for path, item in inherited.items():
            if item["kind"] == "directory" or path in changed:
                continue
            # Full Vim deliberately replaces the parent's vim-tiny alternatives.
            # Require the declared debug-tool payload for those exact paths.
            expected_item = debug_tools.get(path, item) if name.endswith("-dbg") else item
            check.require(all(files.get(path, {}).get(key) == value for key, value in expected_item.items()),
                          "image replaced an inherited parent file: " + path)
        if name.endswith("-dbg"):
            for path, item in debug_tools.items():
                if item["kind"] != "directory":
                    check.require(all(files.get(path, {}).get(key) == value for key, value in item.items()),
                                  "debug tool payload changed: " + path)
        checked = {}
        for archive in ("gnmi-runtime.tar", "common-runtime.tar"):
            payload = {path: item for path, item in check.payload(directory / archive).items()
                       if item["kind"] != "directory"}
            for path, item in payload.items():
                if item["kind"] == "file":
                    image_validation.verify_file(files, path, item, "image changed source owner payload")
                else:
                    check.require(all(files.get(path, {}).get(key) == value for key, value in item.items()),
                                  "image changed source owner link: " + path)
            checked[archive] = len(payload)
        for destination, content in expected_config.items():
            item = files.get(destination, {})
            expected_mode = 0o755 if destination.endswith(".sh") else 0o644
            check.require(item.get("sha256") == hashlib.sha256(content).hexdigest()
                          and (item.get("uid"), item.get("gid"), item.get("mode")) == (0, 0, expected_mode),
                          "startup bytes or metadata changed: " + destination)
        check.require(("usr/bin/dialout.sh" in files) == receipt["dialout_enabled"]
                      and ("usr/sbin/dialout_client_cli" in files) == receipt["dialout_enabled"],
                      "dialout runtime selection differs")
        debug = name.endswith("-dbg")
        status, metadata = (("debug.status", "debug-packages.json") if debug else ("installed.status", "packages.json"))
        installed_metadata = "usr/share/sonic/bazel/gnmi" + ("-debug" if debug else "") + "-packages.json"
        check.require(files["var/lib/dpkg/status"]["sha256"] == check.sha(directory / status)
                      and files[installed_metadata]["sha256"] == check.sha(directory / metadata),
                      "installed package inventory differs")
        config = layout.config.get("config", {})
        expected = json.loads((ROOT / "target/bazel-manifests" / name / "manifest.json").read_text())
        base_layout = layouts[IMAGES[0]] if debug else parent
        verify_config(config, base_layout.config.get("config", {}), expected,
                      receipt["image_version"], runtime=not debug)
        selection_path = directory / ("debug-apt.json" if debug else "runtime-apt.json")
        selection = json.loads(selection_path.read_text())
        package_state = json.loads((directory / metadata).read_text())
        check.require(selection["base_manifest_digest"] == package_state["base_manifest_digest"] ==
                      base_layout.descriptor["digest"], "APT/package state uses a different completed OCI base")
        check.require(package_state["apt_selection_sha256"] == check.sha(selection_path),
                      "package state does not bind the checked APT selection")
        for record in (selection, package_state):
            check.require(record["dependency_check"]["status"] == "satisfied", "unsatisfied image dependencies")
        archive = image_validation.docker_archive(directory / (name + ".gz"), name + ":latest",
                                                  layout.manifest["config"]["digest"], layout.manifest["layers"])
        # Retain only public verification values, never a serialized environment.
        reports[name] = {"manifest_digest": layout.descriptor["digest"],
                         "config_digest": layout.manifest["config"]["digest"],
                         "rootfs": layout.config["rootfs"], "archive": archive,
                         "owner_payloads": checked, "parent_payload_preserved": True}
        inventories[name] = files
    runtime, debug = (inventories[name] for name in IMAGES)
    runtime_layout, debug_layout = (layouts[name] for name in IMAGES)
    check.require(debug_layout.manifest["layers"][:len(runtime_layout.layers)] == runtime_layout.manifest["layers"],
                  "debug image does not extend runtime")
    verify_groups(runtime, debug, group_rows(runtime_layout), group_rows(debug_layout))
    for path, item in runtime.items():
        if "elf_machine" in item:
            expected = debug_tools.get(path, item)
            check.require(all(debug.get(path, {}).get(key) == value for key, value in expected.items()),
                          "debug variant replaced an undeclared runtime ELF: " + path)
    gnmi = check.debug_archives(directory / "gnmi-runtime.tar", directory / "gnmi-debug.tar",
                                     expected_machine=62)
    check.require(len(gnmi["debug_pairs"]) == (9 if receipt["dialout_enabled"] else 8), "gNMI debug coverage differs")
    native = base_images.verify_source_files(debug, [json.loads((directory / name).read_text()) for name in
        ("base-packages.json", "config-engine-packages.json", "debug-packages.json")], 62)
    for path, item in check.payload(directory / "gnmi-runtime.tar").items():
        if "elf_machine" in item:
            check.require(path not in native, "duplicate native owner: " + path)
            native[path] = {**item, "source_package": "sonic-gnmi"}
    pairs = base_images.verify_symbols(native, [directory / name for name in
        ("base-source.tar", "config-engine-source.tar", "openssh-source.tar", "gnmi-runtime.tar")],
        directory / "all-debug.tar", directory)
    for path, item in check.payload(directory / "all-debug.tar").items():
        if item["kind"] != "directory":
            check.require(all(debug.get(path, {}).get(key) == value for key, value in item.items())
                          and path not in runtime, "debug layer differs: " + path)
    check.require(debug["usr/bin/ssh-agent"]["gid"] == 104 and debug["usr/bin/ssh-agent"]["mode"] == 0o2755,
                  "source OpenSSH permissions differ")
    for name in IMAGES:
        reports[name]["native_paths"] = sorted(set(native).intersection(inventories[name]))
    report = {"status": "passed", "revision": receipt["revision"], "architecture": "amd64",
              "dialout_enabled": receipt["dialout_enabled"], "image_version": receipt["image_version"],
              "images": reports, "source_debug_pairs": pairs, "native": native, "artifacts": receipt["artifacts"]}
    (directory / "validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": "passed", "source_elf_debug_pairs": len(pairs)}))


def incremental(args, directory):
    receipt = json.loads((directory / "build-receipt.json").read_text())
    check.require(receipt["status"] == "built", "missing accepted build")
    check.require(source_revision() == receipt["revision"], "source changed before incremental check")
    options = profile_options(receipt["dialout_enabled"])
    labels = [OWNER + name + suffix for name in IMAGES for suffix in ("", ".gz")]
    labels += [PARENT, TARGETS["source-runtime.tar"], TARGETS["all-debug.tar"]]
    command = [args.bazel, "build", *options, *labels]
    paths = {}
    for label in labels:
        paths[label] = ROOT / subprocess.check_output(
            [args.bazel, "cquery", *options, "config(" + label + ", target)", "--output=files"], cwd=ROOT, text=True).strip()

    def snapshot(stage):
        started = time.monotonic()
        with (directory / ("incremental-" + stage + ".log")).open("w") as log:
            subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
        return {"seconds": time.monotonic() - started, "sha256": {
            label: (oci_layout.validate_layout(path, "linux/amd64").descriptor["digest"] if path.is_dir()
                    else check.sha(path)) for label, path in paths.items()}}

    results = {"baseline": snapshot("baseline"), "noop": snapshot("noop")}
    original = results["baseline"]["sha256"]
    check.require(results["noop"]["sha256"] == original, "no-op changed image outputs")
    for name in IMAGES:
        check.require(original[OWNER + name + ".gz"] == receipt["artifacts"][name + ".gz"]["sha256"],
                      "incremental baseline differs from accepted archives")
    source = ROOT / "dockers/docker-sonic-gnmi/telemetry_vars.j2"
    content = source.read_bytes()
    edited = content + b"\n{# gNMI image incremental probe #}\n"
    try:
        check.require(source.read_bytes() == content, "concurrent template change")
        source.write_bytes(edited)
        results["edit"] = snapshot("edit")
        changed = {label for label, digest in results["edit"]["sha256"].items() if original[label] != digest}
        check.require(changed == {OWNER + name + suffix for name in IMAGES for suffix in ("", ".gz")},
                      "template edit changed unrelated source outputs: " + repr(changed))
    finally:
        check.require(source.read_bytes() in (content, edited), "concurrent template change; refusing overwrite")
        source.write_bytes(content)
    results["restore"] = snapshot("restore")
    check.require(results["restore"]["sha256"] == original, "restored inputs changed image/archive digests")
    check.require(source_revision() == receipt["revision"], "source changed during incremental check")
    (directory / "incremental.json").write_text(json.dumps(results, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phase", choices=("build", "validate", "incremental"))
    parser.add_argument("--bazel", default="bazel")
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--dialout", choices=("enabled", "disabled"), default="enabled")
    parser.add_argument("--image-version", default="bazel-ci")
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
