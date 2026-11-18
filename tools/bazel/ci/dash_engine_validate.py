"""Bind DASH engine archives to source owners, package state and base layers."""

from copy import deepcopy
import json
from pathlib import Path
import tempfile

from tools.bazel.ci import artifact_validation as check, base_images, build
from tools.bazel.ci.dash_engine_image import ROOT, IMAGES, TARGETS, clean_source, source_identity
from tools.bazel.oci import image_validation, oci_layout, source_base_validation


# PI's launcher imports its main.c implementation from this declared SONAME.
# Its native toolkit helpers also have local DWARF; verify both debug identities
# and CRCs while retaining the implementation-provider contract.
DEBUG_FORWARDING = {"usr/bin/pi_CLI_dummy": {
    "launcher_compile_units": ["lib/_assert.c", "lib/read_file.c", "lib/vector.c"],
    "provider": "usr/lib/x86_64-linux-gnu/libpicli.so.0", "symbol": "main", "source": "main.c"}}


def require_item(files, path, item, *, image=None):
    # The shared OCI inventory records Linux's installed symlink mode (0777),
    # even when an owner's TAR header has mode 0000. Keep the raw TAR-to-TAR
    # comparison below exact; only the installed link's mode has this meaning.
    expected = {**item, "mode": 0o777} if item.get("kind") == "symlink" else item
    actual = files.get(path, {})
    differences = {key: {"expected": value, "actual": actual.get(key)}
                   for key, value in expected.items() if actual.get(key) != value}
    check.require(not differences, "installed payload differs: " + path + " " +
                  json.dumps({"image": image, "differences": differences}, sort_keys=True))


def verify_config(parent, actual, manifest):
    expected = deepcopy(parent)
    expected["Entrypoint"] = ["/usr/bin/start.sh"]
    # rules_oci resets a base Cmd when an entrypoint is supplied.
    expected["Cmd"] = []
    environment = {entry.split("=", 1)[0]: entry.split("=", 1)[1] for entry in expected.get("Env", [])}
    environment["DEBIAN_FRONTEND"] = "noninteractive"
    entries = actual.get("Env", [])
    check.require(len({entry.split("=", 1)[0] for entry in entries}) == len(entries), "duplicate OCI environment name")
    check.require(dict(entry.split("=", 1) for entry in entries) == environment, "OCI environment differs")
    expected["Env"] = entries
    expected.setdefault("Labels", {})["com.azure.sonic.manifest"] = actual.get("Labels", {}).get("com.azure.sonic.manifest", "")
    check.require(json.loads(expected["Labels"]["com.azure.sonic.manifest"]) == manifest, "Make manifest label differs")
    # OCI permits omitted empty Cmd; every other field must remain exact.
    normalized = deepcopy(actual)
    normalized.setdefault("Cmd", [])
    check.require(normalized == expected, "OCI configuration changed beyond declared entrypoint/environment/manifest")


def verify_groups(before, after, base_item, item, debug):
    names = [row.split(":", 1)[0] for row in after]
    check.require(len(names) == len(set(names)), "duplicate group name")
    expected = list(before)
    if debug and "_ssh:x:104:" not in expected:
        expected.append("_ssh:x:104:")
    check.require(after == expected, "inherited group rows changed")
    check.require(all(base_item[key] == item[key] for key in ("kind", "mode", "uid", "gid")),
                  "inherited group metadata changed")


def read_groups(layout):
    selected, origins = source_base_validation.inventory(layout)
    with tempfile.TemporaryDirectory(prefix="engine-groups-") as temporary:
        root = Path(temporary)
        source_base_validation.extract_selected(layout, selected, origins, {"etc/group"}, root)
        return (root / "etc/group").read_text().splitlines()


def verify_package_binding(report, selection, parent, apt_path, variant, lock_sha256):
    check.require(selection["schema"] == 1 and selection["image"] == "docker-dash-engine" and
                  selection["group"] == variant and selection["apt_lock_sha256"] == lock_sha256,
                  "APT lock/image/stage binding differs")
    check.require(selection["base_manifest_digest"] == parent.descriptor["digest"] and
                  selection["dependency_check"]["status"] == "satisfied", "APT base/dependency binding differs")
    check.require(report["base_manifest_digest"] == parent.descriptor["digest"] and
                  report["apt_selection_sha256"] == check.sha(apt_path) and
                  report["dependency_check"]["status"] == "satisfied", "installed package binding differs")
    check.require(all(item["package"] != "nnpy" for item in report["source_packages"]),
                  "nnpy must retain Python metadata, not an invented Debian package")


def verify_layer_binding(layout, parent, path):
    added = layout.manifest["layers"][len(parent.layers):]
    check.require(added and added[0]["digest"] == "sha256:" + check.sha(path) and
                  added[0]["size"] == path.stat().st_size,
                  "exported package payload differs from the first declared stage layer")


def verify_parent_payload(parent, installed, payload, selection, owned=None):
    """Bind each APT-declared non-ELF replacement to its retained layer record."""
    expected = dict(owned or {})
    selected = {item["package"] for item in selection["selected"]}
    replacements = selection["changed_non_elf_base_paths"]
    paths = [item["path"] for item in replacements]
    check.require(len(paths) == len(set(paths)), "duplicate APT replacement path")
    for record in replacements:
        path = record["path"]
        check.require(check.path_name(path) == path and path not in expected and
                      record["package"] in selected, "undeclared APT replacement owner/path")
        before, after = parent.get(path, {}), payload.get(path, {})
        check.require(before.get("kind") == after.get("kind") == "file" and
                      "elf_machine" not in before and "elf_machine" not in after,
                      "APT replacement must be an inherited non-ELF file: " + path)
        require_item(installed, path, after)
        expected[path] = after
    for path, item in parent.items():
        if item["kind"] == "directory" or path in ("var/lib/dpkg/status", "etc/group"):
            continue
        require_item(installed, path, expected.get(path, item))


def debug_alternatives(payload):
    # Exact links supplied by the declared upstream Vim alternatives target and
    # this image's debug_vim_link target; unrelated links remain inherited.
    paths = {"etc/alternatives/" + name: "/usr/bin/vim.basic" for name in
             ("editor", "ex", "rview", "rvim", "vi", "view", "vim", "vimdiff")}
    paths["usr/bin/vim"] = "/etc/alternatives/vim"
    expected = {path: {"kind": "symlink", "mode": 0o777, "uid": 0, "gid": 0,
                       "linkname": target} for path, target in paths.items()}
    for path, item in expected.items():
        require_item(payload, path, item)
    return expected


def verify_source_payloads(archives, layer, excludes, includes):
    """Preserve every retained owner payload under the layer's declared filters."""
    source = check.payload(layer)
    original = {}
    for archive in archives:
        for path, item in check.payload(archive).items():
            if item["kind"] == "directory":
                continue
            normalized = "usr/" + path if path.startswith(("lib/", "bin/", "sbin/")) else path
            check.require(normalized not in original or original[normalized] == item, "source owners collide: " + normalized)
            original[normalized] = item
    expected = image_validation.oci_filtered(original, excludes, includes)
    check.require(expected == {path: item for path, item in source.items() if item["kind"] != "directory"},
                  "flattened source packages changed payloads")
    return source


def validate(directory):
    clean_source(directory)
    identity = source_identity()
    receipt = json.loads((directory / "build-receipt.json").read_text())
    check.require(receipt["status"] == "built" and identity["revision"] == receipt["revision"], "unverified source build")
    check.require(set(receipt["artifacts"]) == set(TARGETS), "incomplete artifact inventory")
    for name, item in receipt["artifacts"].items():
        path = directory / name
        check.require(path.stat().st_size == item["bytes"] and check.sha(path) == item["sha256"], "artifact changed: " + name)
    root = build.execution_root_from_bep(directory / "build-events.jsonl")
    parent = oci_layout.validate_layout(root / receipt["base_layout"], "linux/amd64")
    layouts = {name: oci_layout.validate_layout(root / receipt["layouts"][name], "linux/amd64") for name in IMAGES}
    inherited = base_images.installed_inventory(parent)
    parent_groups = read_groups(parent)
    configuration = check.payload(directory / "engine.files.tar")
    files_only = {path: item for path, item in configuration.items() if item["kind"] != "directory"}
    check.require(set(files_only) == {"usr/bin/start.sh"}, "startup file inventory differs")
    start = files_only["usr/bin/start.sh"]
    check.require(start["sha256"] == check.sha(ROOT / "platform/vs/docker-dash-engine/start.sh") and
                  (start["kind"], start["mode"], start["uid"], start["gid"]) == ("file", 0o755, 0, 0), "original startup script changed")
    source = verify_source_payloads(
        [directory / name for name in ("pi.source.tar", "bmv2.source.tar", "nnpy.source.tar")],
        directory / "engine.source.tar", directory / "source-excludes.txt", directory / "source-includes.txt")
    nnpy = check.payload(directory / "nnpy.source.tar")
    check.require("usr/lib/python3/dist-packages/_nnpy.so" in nnpy and
                  "usr/lib/python3/dist-packages/nnpy-1.4.2.egg-info/PKG-INFO" in nnpy, "nnpy extension/distribution metadata absent")
    debug_tools = check.payload(directory / "engine.debug-tools.tar", require_root=False)
    runtime_apt = check.payload(directory / "engine.runtime-apt.tar", require_root=False)
    manifest = json.loads((ROOT / "target/bazel-manifests/docker-dash-engine/manifest.json").read_text())
    inventories, reports = {}, [json.loads((directory / "base.packages.json").read_text())]
    for name in IMAGES:
        debug = name.endswith("-dbg")
        layout = layouts[name]
        base = layouts[IMAGES[0]] if debug else parent
        image_validation.assert_ancestry(base, layout, "engine stage ancestry")
        image_validation.verify_unique_layer_paths(layout)
        # Bind the exported archive before inspecting the live OCI directory.
        # A failed payload check must not hide a disagreement between them.
        archive = image_validation.docker_archive(directory / (name + ".gz"), name + ":latest",
            layout.manifest["config"]["digest"], layout.manifest["layers"])
        files = base_images.installed_inventory(layout)
        image_validation.verify_directory_aliases(files)
        for path, item in (source | configuration).items():
            if item["kind"] != "directory":
                require_item(files, path, item, image=name)
        stage_parent = inventories[IMAGES[0]] if debug else inherited
        prefix = "engine-debug" if debug else "engine"
        report = json.loads((directory / (prefix + ".packages.json")).read_text())
        apt_path = directory / ("debug-apt.json" if debug else "runtime-apt.json")
        selection = json.loads(apt_path.read_text())
        verify_package_binding(report, selection, base, apt_path, "debug" if debug else "runtime",
                               receipt["apt_lock_sha256"])
        payload = debug_tools if debug else runtime_apt
        verify_layer_binding(layout, base, directory / ("engine.debug-tools.tar" if debug else "engine.runtime-apt.tar"))
        verify_parent_payload(stage_parent, files, payload, selection,
                              debug_alternatives(debug_tools) if debug else configuration)
        for path, item in payload.items():
            if item["kind"] != "directory":
                require_item(files, path, item, image=name)
        verify_groups(parent_groups, read_groups(layout), inherited["etc/group"], files["etc/group"], debug)
        reports.append(report)
        for path, artifact in (("var/lib/dpkg/status", prefix + ".status"),
                               ("usr/share/sonic/bazel/dash-engine" + ("-debug" if debug else "") + "-packages.json", prefix + ".packages.json")):
            check.require(files.get(path, {}).get("sha256") == check.sha(directory / artifact), "installed package state differs: " + path)
        check.require(all(item.get("elf_machine", 62) == 62 for item in files.values()), "foreign ELF architecture")
        verify_config(parent.config.get("config", {}), layout.config.get("config", {}), manifest)
        inventories[name] = files
        (directory / (name + ".validation.json")).write_text(json.dumps({"status": "passed",
            "manifest_digest": layout.descriptor["digest"], "rootfs": layout.config["rootfs"],
            "config_digest": layout.manifest["config"]["digest"], "archive": archive}, indent=2) + "\n")
    runtime, debug = (inventories[name] for name in IMAGES)
    for path, item in runtime.items():
        if "elf_machine" in item:
            check.require(debug.get(path) == item, "debug image changed runtime ELF: " + path)
    native = base_images.verify_source_files(debug, reports, 62)
    runtimes = [directory / name for name in ("base.source.tar", "engine.source.tar", "openssh.source.tar")]
    for archive in runtimes:
        for path, item in check.payload(archive, require_root=False).items():
            if "elf_machine" not in item:
                continue
            require_item(debug, path, item)
            if path in native:
                check.require(native[path]["sha256"] == item["sha256"], "source owners disagree: " + path)
            else:
                native[path] = {**item, "source_package": archive.name}
    pairs = base_images.verify_symbols(native, runtimes, directory / "engine.debug.tar", directory,
                                       forwarding=DEBUG_FORWARDING)
    check.require((debug["usr/bin/ssh-agent"]["gid"], debug["usr/bin/ssh-agent"]["mode"]) == (104, 0o2755), "SSH client privilege differs")
    selected, origins = source_base_validation.inventory(layouts[IMAGES[1]])
    elfs = []
    with tempfile.TemporaryDirectory(prefix="engine-native-") as temporary:
        root = Path(temporary)
        source_base_validation.extract_selected(layouts[IMAGES[1]], selected, origins, set(native), root)
        for path, item in sorted(native.items()):
            info = check.elf_info(root / path, dynamic=True)
            elfs.append({"path": path, "needed": info["needed"], "sha256": item["sha256"]})
    clean_source(directory)
    check.require(source_identity() == identity, "validation source changed")
    report = {"status": "passed", "architecture": "amd64", "revision": receipt["revision"],
              "image_version": receipt["image_version"], "native": native, "source_debug_pairs": pairs,
              "runtime_native": {path: item for path, item in native.items() if path in runtime},
              "elfs": elfs, "artifacts": receipt["artifacts"]}
    (directory / "validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": "passed", "source_elf_debug_pairs": len(pairs)}))
