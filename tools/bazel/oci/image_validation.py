#!/usr/bin/env python3
"""Validate OCI configuration, installed payloads and the Docker archive handoff."""

import argparse
import fnmatch
import hashlib
import json
from pathlib import Path
import re
import tarfile

ROOT = Path(__file__).absolute().parents[3]
from tools.bazel.ci.artifact_validation import path_name as member_name, require, sha
from tools.bazel.oci import oci_inventory
from tools.bazel.oci.oci_inventory import assert_overlay_paths
from tools.bazel.oci.oci_layout import validate_layout
from tools.bazel.oci import package_payloads as validate_payloads
from tools.bazel.oci import package_contract
from tools.bazel.oci.native_packages import ELF_MACHINES
from tools.bazel.oci.package_contract import identity, read_contract, read_policy, validate_policy


def image(directory, *, expected_platform="linux/amd64"):
    return validate_layout(directory, expected_platform)


def apply_layer(path, files, *, merged_usr=False, checked_overlay=False):
    return oci_inventory.apply_layer(
        path, files, checked_overlay=checked_overlay,
        normalize_member=validate_payloads.normalized_member if merged_usr else None)


def apply_debug_tools_layer(path, files, *, alternatives=(), elf_machine=62):
    """Permit reviewed alternative links while preserving every inherited ELF."""
    entries = {}
    apply_layer(path, entries)
    approved = {}
    for replacement in alternatives:
        old_link = {"kind": "symlink", "mode": 0o777, "uid": 0, "gid": 0, "linkname": replacement["before"]}
        new_link = dict(old_link, linkname=replacement["after"])
        for name in replacement["paths"]:
            require(files.get(name) == old_link and entries.get(name) == new_link,
                    "unexpected debug alternative: " + name)
            approved[name] = entries[name]
        inherited = oci_inventory.resolve_path(replacement["before"].lstrip("/"), files)
        added = oci_inventory.resolve_path(replacement["after"].lstrip("/"), entries)
        require(files.get(inherited, {}).get("elf_machine") == elf_machine and
                entries.get(added, {}).get("elf_machine") == elf_machine,
                "debug alternatives require binaries for the expected architecture")
    checked = dict(files)
    checked.update(approved)
    apply_layer(path, checked, checked_overlay=True)
    assert_payload({name: item for name, item in files.items() if "elf_machine" in item},
                   checked, "debug tools change an inherited ELF")
    files.clear()
    files.update(checked)


def payloads(manifest_path, variant, runtime_manifest=None, *, policy, source_contract):
    payload = manifest_path.parent / "payload.tar"
    receipt = validate_payloads.validate(manifest_path, payload, variant=variant, runtime_manifest=runtime_manifest,
                                         policy=policy, source_contract=source_contract)
    manifest = json.loads(manifest_path.read_bytes())
    files = {}
    apply_layer(payload, files, merged_usr=True)
    owners = {}
    with tarfile.open(payload, "r:") as archive:
        members = iter(archive)
        for record in manifest["packages"]:
            for _ in range(record["payload_members"]):
                member = next(members, None)
                require(member is not None, "aggregate package ownership segments are incomplete")
                normalized = validate_payloads.normalized_member(member)
                if normalized is not None:
                    owners[member_name(normalized.name)] = record["package"]
        require(next(members, None) is None, "aggregate package ownership has extra members")
    return receipt, files, owners, manifest


def source_payloads(runtime_tar, debug_tar, receipt_path, *, contract_path=None, fixture=False):
    """Bind runtime to its receipt and inspect the separate shared symbol layer."""
    receipt = package_contract.validate_receipt(receipt_path, runtime_tar, debug_tar, contract_path=contract_path, fixture=fixture)
    runtime, debug = {}, {}
    apply_layer(runtime_tar, runtime)
    apply_layer(debug_tar, debug)
    return receipt, runtime, debug


def assert_disjoint_source(make_files, source_files):
    overlaps = sorted(name for name, item in source_files.items()
                      if name in make_files and not (item["kind"] == make_files[name]["kind"] == "directory"))
    require(not overlaps, "source package overlaps a Make payload: " + ", ".join(overlaps[:20]))


def dpkg_filtered(files, filter_path):
    rules = []
    for line in filter_path.read_text().splitlines():
        match = re.fullmatch(r"(path-exclude|path-include)(?:=|\s+)(\S+)", line.strip())
        if match:
            rules.append((match[1] == "path-include", match[2]))
    require(rules, "dpkg path filtering configuration has no rules")
    result = {}
    for name, item in files.items():
        keep = True
        for include, pattern in rules:
            if fnmatch.fnmatchcase("/" + name, pattern):
                keep = include
        if keep:
            result[name] = item
    return result


def assert_payload(expected, actual, description):
    differences = [name for name, item in expected.items() if actual.get(name) != item]
    require(not differences, description + " differs at: " + ", ".join(differences[:20]))


def expected_state(path):
    contract = json.loads(path.read_bytes())
    require(contract.get("schema") == 1, "invalid runtime package state contract")
    result = {}
    for entry in contract["entries"]:
        name = member_name(entry["path"])
        if entry["kind"] == "symlink":
            result[name] = {key: entry[key] for key in ("kind", "mode", "uid", "gid", "linkname")}
        else:
            result[name] = {key: entry[key] for key in ("kind", "mode", "uid", "gid", "sha256", "size")}
    for entry in contract["aliases"]:
        result[member_name(entry["path"])] = {"kind": "file", **{key: entry[key] for key in ("mode", "uid", "gid", "sha256", "size")}}
    require(len(result) == len(contract["entries"]) + len(contract["aliases"]), "duplicate runtime package state path")
    return result


def docker_archive(path, expected_tag, config_digest, layer_descriptors):
    require(re.fullmatch(r"sha256:[0-9a-f]{64}", config_digest) is not None, "invalid expected OCI config digest")
    with path.open("rb") as stream:
        header = stream.read(10)
    require(len(header) == 10 and header[:3] == b"\x1f\x8b\x08" and header[4:8] == b"\0\0\0\0" and not header[3] & 0x08,
            "Docker archive gzip header is not reproducible")
    files = {}
    config_paths = set()
    manifest = None
    with tarfile.open(path, "r|gz") as archive:
        for member in archive:
            name = member_name(member.name)
            if not member.isfile():
                continue
            require(name not in files, "duplicate Docker archive file: " + name)
            stream = archive.extractfile(member)
            hasher = hashlib.sha256()
            data = bytearray() if name == "manifest.json" else None
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                hasher.update(chunk)
                if data is not None:
                    require(len(data) + len(chunk) <= 16 * 1024 * 1024, "Docker archive manifest is too large")
                    data.extend(chunk)
            digest = hasher.hexdigest()
            files[name] = {"size": member.size, "sha256": digest}
            if data is not None:
                manifest = json.loads(data)
            if digest == config_digest[7:]:
                config_paths.add(name)
    require(isinstance(manifest, list) and len(manifest) == 1, "Docker archive does not contain one image")
    entry = manifest[0]
    require(entry.get("RepoTags") == [expected_tag], "Docker archive tag differs from Make")
    require(member_name(entry.get("Config", "")) in config_paths, "Docker archive config differs from its OCI image")
    layers = entry.get("Layers")
    require(isinstance(layers, list) and len(layers) == len(layer_descriptors), "Docker archive layer set is incomplete")
    for name, descriptor in zip(layers, layer_descriptors):
        require(files.get(member_name(name)) == {"size": descriptor["size"], "sha256": descriptor["digest"][7:]},
                "Docker archive layer differs from its OCI image: " + name)
    return {"sha256": sha(path), "bytes": path.stat().st_size, "tag": expected_tag,
            "config_digest": config_digest, "layers": len(layer_descriptors)}


def validate_images(runtime_path, debug_path, runtime_handoff, debug_handoff,
                    runtime_manifest_path, debug_manifest_path, source_root, *, policy, source_contract_path=None,
                    apt_lock_path=None, dpkg_filter_path=None, fixture=False,
                    base_path=None, apt_layer=None, debug_tools_layer=None, runtime_links_layer=None,
                    runtime_apt_selection=None, debug_apt_selection=None, package_state_contract=None,
                    runtime_archive=None, debug_archive=None,
                    source_runtime_tar=None, source_debug_tar=None, source_receipt=None,
                    base_debug_symbols=None, base_debug_receipt=None, base_debug_package_manifest=None):
    validate_policy(policy)
    expectations = policy.get("image_validation", {})
    native_expectations = policy.get("native", {})
    architecture = policy["architecture"]
    require(architecture in ELF_MACHINES, "unsupported native ELF architecture: " + architecture)
    elf_machine = ELF_MACHINES[architecture]
    expected_platform = "linux/" + architecture
    source_contract = read_contract(source_contract_path) if source_contract_path is not None else None
    if source_contract is not None:
        require(identity(source_contract) == identity(policy), "source contract and package policy identify different images")
    elif fixture:
        # Synthetic tests can exercise receipts without a checked owner contract.
        source_contract = {"schema": 1, **identity(policy), "packages": []}
    retained = policy["retained_source"] == "make"
    require((runtime_handoff is not None and debug_handoff is not None) == retained and
            (retained or runtime_handoff is debug_handoff is None),
            "retained handoff inputs differ from the image policy")
    require(not retained or dpkg_filter_path is not None, "retained image validation requires the dpkg filter file")
    require(fixture or bool(package_state_contract) == bool(expectations.get("package_state", False)),
            "package state input differs from the image policy")
    complete_inputs = (base_path, apt_layer, debug_tools_layer, runtime_apt_selection, debug_apt_selection,
                       runtime_archive, debug_archive, apt_lock_path)
    complete = all(value is not None for value in complete_inputs)
    require(complete or not any(value is not None for value in complete_inputs), "complete overlay validation inputs must be supplied together")
    require(fixture or complete, "complete validation requires the base, APT layers/selections, package state contract, and archives")
    source_inputs = (source_runtime_tar, source_debug_tar, source_receipt)
    has_source = all(value is not None for value in source_inputs)
    require(fixture or source_contract_path is not None or
            not (has_source or native_expectations.get("require_source", False)),
            "image validation requires its source contract")
    require(has_source or not any(value is not None for value in source_inputs),
            "source runtime tar, debug tar, and receipt must be supplied together")
    require(fixture or not native_expectations.get("require_source", False) or has_source,
            "complete validation requires source runtime tar, debug tar, and receipt")
    require((base_debug_symbols is None) == (base_debug_receipt is None),
            "imported base symbols and receipt must be supplied together")
    require(fixture or not native_expectations.get("require_imported_symbols", False) or
            (base_debug_symbols is not None and base_debug_package_manifest is not None),
            "complete validation requires imported base symbols, package manifest, and receipt")
    source_provenance, expected_source_runtime, expected_source_debug = None, {}, {}
    if has_source:
        source_provenance, expected_source_runtime, expected_source_debug = source_payloads(
            source_runtime_tar, source_debug_tar, source_receipt,
            contract_path=source_contract_path, fixture=fixture)
    runtime_descriptor, runtime_manifest, runtime_config, runtime_layers = image(runtime_path, expected_platform=expected_platform)
    debug_descriptor, debug_manifest, debug_config, debug_layers = image(debug_path, expected_platform=expected_platform)
    inherited_symbols_receipt, expected_inherited_symbols = None, {}
    if base_debug_symbols is not None:
        from tools.bazel.oci.native_packages import imported_symbol_receipt
        inherited_symbols_receipt, expected_inherited_symbols = imported_symbol_receipt(
            base_debug_symbols, base_debug_receipt, runtime_descriptor["digest"], base_debug_package_manifest, policy=policy)
        require(set(native_expectations.get("required_imported_symbol_paths", [])).issubset(
                {pair["runtime_path"] for pair in inherited_symbols_receipt["pairs"]}),
                "required inherited runtime has no imported symbol pair")
    runtime_settings = runtime_config.get("config", {})
    debug_settings = debug_config.get("config", {})
    require(runtime_settings.get("Entrypoint") == expectations.get("entrypoint"),
            "runtime entrypoint differs from its policy")
    require(runtime_settings.get("Cmd") == expectations.get("command"), "runtime command differs from its policy")
    environment = dict(value.split("=", 1) for value in runtime_settings.get("Env", []))
    require(all(environment.get(key) == value for key, value in expectations.get("environment", {}).items()),
            "runtime environment differs from its policy")
    for key in ("Entrypoint", "Cmd", "Env", "User", "WorkingDir"):
        require(debug_settings.get(key) == runtime_settings.get(key), "debug image changes runtime " + key)
    for settings, manifest_path, variant in ((runtime_settings, runtime_manifest_path, "runtime"),
                                              (debug_settings, debug_manifest_path, "debug")):
        label = settings.get("Labels", {}).get("com.azure.sonic.manifest")
        require(isinstance(label, str) and json.loads(label) == json.loads(manifest_path.read_bytes()),
                variant + " OCI manifest label differs from Make")
    runtime_layer_descriptors = runtime_manifest["layers"]
    require(debug_manifest["layers"][:len(runtime_layer_descriptors)] == runtime_layer_descriptors,
            "debug image does not extend the exact runtime OCI layers")
    runtime_diff_ids = runtime_config["rootfs"]["diff_ids"]
    require(debug_config["rootfs"]["diff_ids"][:len(runtime_diff_ids)] == runtime_diff_ids,
            "debug image does not extend the exact runtime rootfs")
    runtime_files = {}
    for layer in runtime_layers:
        apply_layer(layer, runtime_files)
    debug_files = dict(runtime_files)
    for layer in debug_layers[len(runtime_layers):]:
        apply_layer(layer, debug_files)
    installation, runtime_links = {}, {}
    if runtime_links_layer is not None:
        installation = package_contract.inventory(runtime_links_layer)
        runtime_links = {name: item for name, item in installation.items()
                         if item["kind"] in ("symlink", "hardlink")}
        require(runtime_links, "runtime installation layer has no links")
        for variant, files in (("runtime", runtime_files), ("debug", debug_files)):
            assert_payload(installation, files, variant + " installation link layer")
            for name in runtime_links:
                resolved = oci_inventory.resolve_path(name, files)
                require(resolved in files,
                        variant + " installation link has no final target: " + name)
    runtime_receipt = debug_receipt = {"manifest_sha256": None, "package_count": 0}
    expected_runtime, expected_debug, runtime_owners = {}, {}, {}
    if retained:
        runtime_receipt, expected_runtime, runtime_owners, _ = payloads(
            runtime_handoff, "runtime", policy=policy, source_contract=source_contract)
        debug_receipt, expected_debug, _, _ = payloads(
            debug_handoff, "debug", runtime_handoff, policy=policy, source_contract=source_contract)
        expected_runtime = dpkg_filtered(expected_runtime, dpkg_filter_path)
        expected_debug = dpkg_filtered(expected_debug, dpkg_filter_path)
    assert_disjoint_source(expected_runtime, expected_source_runtime)
    assert_disjoint_source(expected_debug, expected_source_debug)
    assert_disjoint_source(expected_runtime, expected_source_debug)
    assert_disjoint_source(expected_debug, expected_source_runtime)
    assert_payload(expected_source_runtime, runtime_files, "runtime source package payload")
    assert_payload(expected_source_debug, debug_files, "debug source package payload")
    assert_payload(expected_inherited_symbols, debug_files, "debug imported base symbols")
    for pair in (inherited_symbols_receipt or {}).get("pairs", []):
        require(runtime_files.get(pair["runtime_path"], {}).get("sha256") == pair["runtime_sha256"],
                "imported base symbols use different runtime bytes")
    assert_payload({name: item for name, item in expected_source_runtime.items() if item["kind"] != "directory"},
                   debug_files, "debug image changes the source runtime payload")
    assert_payload(expected_runtime, runtime_files, "runtime package payload")
    assert_payload(expected_debug, debug_files, "debug package payload")
    for requirement in policy.get("retained_requirements", []):
        for name in requirement.get("runtime_elf_paths", []):
            require(runtime_owners.get(name) == requirement["package"] and
                    expected_runtime.get(name, {}).get("elf_machine") == elf_machine,
                    "runtime image lacks its required retained ELF: " + name)
    assert_payload({name: item for name, item in expected_runtime.items() if item["kind"] != "directory"},
                   debug_files, "debug image changes the runtime package payload")
    static_files = {}
    for installed, declared in expectations.get("startup_files", {}).items():
        path = source_root / declared["source"]
        mode = declared["mode"]
        expected = {"kind": "file", "mode": mode, "uid": 0, "gid": 0,
                    "sha256": sha(path), "size": path.stat().st_size}
        static_files[installed] = expected
        require(runtime_files.get(installed) == expected, "startup file differs: " + installed)
        require(debug_files.get(installed) == expected, "debug image changes startup file: " + installed)
    runtime_elfs = {name: item for name, item in runtime_files.items() if "elf_machine" in item}
    assert_payload(runtime_elfs, debug_files, "debug image changes a deployed ELF")
    if not fixture:
        for name in expectations.get("required_elf_paths", []):
            require(runtime_files.get(name, {}).get("elf_machine") == elf_machine,
                    "complete image lacks its required native ELF: " + name)
    overlay_report = None
    archive_report = None
    if complete:
        base_descriptor, base_manifest, base_config, base_layers = image(base_path, expected_platform=expected_platform)
        if has_source:
            require(source_provenance.get("base_manifest_digest") == base_descriptor["digest"],
                    "source packages were prepared for a different OCI base")
        require(runtime_manifest["layers"][:len(base_manifest["layers"])] == base_manifest["layers"] and
                runtime_config["rootfs"]["diff_ids"][:len(base_config["rootfs"]["diff_ids"])] == base_config["rootfs"]["diff_ids"],
                "runtime image does not extend the exact managed OCI base")
        base_settings = base_config.get("config", {})
        base_environment = dict(value.split("=", 1) for value in base_settings.get("Env", []))
        base_environment.update(expectations.get("environment", {}))
        require(environment == base_environment and runtime_settings.get("User") == base_settings.get("User") and
                runtime_settings.get("WorkingDir") == base_settings.get("WorkingDir"),
                "runtime image changes inherited base settings")
        for name, value in base_settings.get("Labels", {}).items():
            if name != "com.azure.sonic.manifest":
                require(runtime_settings.get("Labels", {}).get(name) == value, "runtime image drops a base label: " + name)
        inherited_runtime = {key: value for key, value in base_settings.items() if value is not None}
        inherited_runtime.update(Entrypoint=expectations.get("entrypoint"), Env=runtime_settings["Env"],
                                 Labels=runtime_settings["Labels"])
        if expectations.get("command") is None:
            inherited_runtime.pop("Cmd", None)
        else:
            inherited_runtime["Cmd"] = expectations["command"]
        require({key: value for key, value in runtime_settings.items() if value is not None} == inherited_runtime,
                "runtime image changes an inherited base configuration field")
        inherited_debug = dict(runtime_settings)
        inherited_debug["Labels"] = debug_settings["Labels"]
        require({key: value for key, value in debug_settings.items() if value is not None} ==
                {key: value for key, value in inherited_debug.items() if value is not None},
                "debug image changes an inherited runtime configuration field")
        checked_files = {}
        for layer in base_layers:
            apply_layer(layer, checked_files)
        if has_source:
            inherited = source_provenance.get("inherited_files", {})
            assert_payload(inherited, checked_files, "source receipt inherited base payload")
            assert_payload(inherited, runtime_files, "runtime image changes the required base payload")
            assert_payload(inherited, debug_files, "debug image changes the required base payload")
        for layer in runtime_layers[len(base_layers):]:
            apply_layer(layer, checked_files, checked_overlay=True)
        added_debug_layers = debug_layers[len(runtime_layers):]
        require(added_debug_layers, "debug image lacks the tools layer")
        apply_debug_tools_layer(added_debug_layers[0], checked_files,
                                alternatives=expectations.get("debug_alternatives", []), elf_machine=elf_machine)
        for layer in added_debug_layers[1:]:
            apply_layer(layer, checked_files, checked_overlay=True)
        validate_payloads.base_aliases(runtime_path, expected_platform=expected_platform)
        validate_payloads.base_aliases(debug_path, expected_platform=expected_platform)
        lock_sha = sha(apt_lock_path)
        runtime_selection = json.loads(runtime_apt_selection.read_bytes())
        debug_selection = json.loads(debug_apt_selection.read_bytes())
        for selection, variant, base_digest, make_sha in (
            (runtime_selection, "runtime", base_descriptor["digest"], runtime_receipt["manifest_sha256"]),
            (debug_selection, "debug", runtime_descriptor["digest"], debug_receipt["manifest_sha256"]),
        ):
            require(selection.get("schema") == 1 and selection.get("variant") == variant and
                    selection.get("base_manifest_digest") == base_digest and selection.get("apt_lock_sha256") == lock_sha and
                    selection.get("make_manifest_sha256") == make_sha, variant + " APT selection identity differs")
            if has_source:
                require(selection.get("source_receipt_sha256") == sha(source_receipt),
                        variant + " APT selection uses a different source receipt")
        runtime_overlay = {}
        apply_layer(apt_layer, runtime_overlay)
        runtime_overlay.update(expected_runtime)
        runtime_overlay.update(expected_source_runtime)
        state = {}
        if package_state_contract is not None:
            require(json.loads(package_state_contract.read_bytes()).get("apt_lock_sha256") == lock_sha,
                    "runtime package state contract uses a different APT lock")
            state = expected_state(package_state_contract)
        runtime_overlay.update(state)
        runtime_overlay.update(installation)
        directories = {"."}
        for path in static_files:
            directories.update(str(parent) for parent in Path(path).parents)
        runtime_overlay.update({name: {"kind": "directory", "mode": 0o755, "uid": 0, "gid": 0}
                                for name in directories})
        runtime_overlay.update(static_files)
        assert_payload(runtime_overlay, runtime_files, "runtime OCI overlay payload")
        assert_payload(state, debug_files, "debug image changes runtime generated files")
        debug_overlay = {}
        apply_layer(debug_tools_layer, debug_overlay)
        debug_overlay.update(expected_debug)
        debug_overlay.update(expected_source_debug)
        debug_overlay.update(expected_inherited_symbols)
        assert_payload(debug_overlay, debug_files, "debug OCI overlay payload")
        overlay_report = {"base_manifest_digest": base_descriptor["digest"], "runtime_entries": len(runtime_overlay),
                          "debug_entries": len(debug_overlay), "package_state_entries": len(state),
                          "debug_alternative_links": [path for item in expectations.get("debug_alternatives", []) for path in item["paths"]],
                          "runtime_apt_selected": len(runtime_selection["selected"]),
                          "runtime_apt_skipped_base": len(runtime_selection["skipped_base"]),
                          "runtime_apt_skipped_make": len(runtime_selection["skipped_make"]),
                          "debug_apt_selected": len(debug_selection["selected"]),
                          "runtime_non_elf_base_changes": runtime_selection["changed_non_elf_base_paths"],
                          "debug_non_elf_base_changes": debug_selection["changed_non_elf_base_paths"]}
        archive_report = {
            "runtime": docker_archive(runtime_archive, expectations["runtime_tag"], runtime_manifest["config"]["digest"], runtime_manifest["layers"]),
            "debug": docker_archive(debug_archive, expectations["debug_tag"], debug_manifest["config"]["digest"], debug_manifest["layers"]),
        }
    changed_non_elf = sorted(name for name, item in runtime_files.items()
                             if "elf_machine" not in item and debug_files.get(name) != item)
    remaining = ["native ELF SONAME and debug-symbol validation", "unpacked rootfs and parent-symlink application", "in-image dynamic loading and debugger lookup",
                 "package-manager database and generated loader/Python cache equivalence"]
    if not complete:
        remaining.append("complete APT, generated-file, and Docker archive checks")
    return {
        "schema": 1, "validation_mode": "fixture" if fixture else "complete",
        "runtime_manifest_digest": runtime_descriptor["digest"], "debug_manifest_digest": debug_descriptor["digest"],
        "runtime_package_manifest_sha256": runtime_receipt["manifest_sha256"],
        "debug_package_manifest_sha256": debug_receipt["manifest_sha256"],
        "runtime_layers": len(runtime_layers), "debug_layers": len(debug_layers),
        "runtime_package_count": runtime_receipt["package_count"], "debug_package_count": debug_receipt["package_count"],
        "runtime_payload_entries": len(expected_runtime), "debug_payload_entries": len(expected_debug),
        "source_receipt_sha256": sha(source_receipt) if has_source else None,
        "source_debug_tar_sha256": sha(source_debug_tar) if has_source else None,
        "source_packages": source_provenance,
        "inherited_base_symbols": inherited_symbols_receipt,
        "source_runtime_payload_entries": len(expected_source_runtime),
        "source_debug_payload_entries": len(expected_source_debug),
        "runtime_links_layer_sha256": sha(runtime_links_layer) if runtime_links_layer is not None else None,
        "runtime_link_count": len(runtime_links),
        "runtime_elf_count": len(runtime_elfs), "allowed_debug_package_replacements": [],
        "changed_non_elf_runtime_paths_in_debug": changed_non_elf, "overlay_checks": overlay_report,
        "archives": archive_report, "remaining_checks": remaining,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--policy", required=True, type=Path)
    parser.add_argument("--source-contract", type=Path)
    parser.add_argument("--apt-lock", type=Path)
    parser.add_argument("--dpkg-filter-file", type=Path)
    parser.add_argument("--runtime", required=True, type=Path)
    parser.add_argument("--debug", required=True, type=Path)
    parser.add_argument("--runtime-handoff", type=Path)
    parser.add_argument("--debug-handoff", type=Path)
    parser.add_argument("--runtime-manifest", required=True, type=Path)
    parser.add_argument("--debug-manifest", required=True, type=Path)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--base", type=Path)
    parser.add_argument("--apt-layer", type=Path)
    parser.add_argument("--debug-tools-layer", type=Path)
    parser.add_argument("--runtime-links-layer", type=Path)
    parser.add_argument("--runtime-apt-selection", type=Path)
    parser.add_argument("--debug-apt-selection", type=Path)
    parser.add_argument("--package-state-contract", type=Path)
    parser.add_argument("--runtime-archive", type=Path)
    parser.add_argument("--debug-archive", type=Path)
    parser.add_argument("--source-runtime-tar", type=Path)
    parser.add_argument("--source-debug-tar", type=Path)
    parser.add_argument("--source-receipt", type=Path)
    parser.add_argument("--base-debug-symbols", type=Path)
    parser.add_argument("--base-debug-receipt", type=Path)
    parser.add_argument("--base-debug-package-manifest", type=Path)
    parser.add_argument("--fixture", action="store_true")
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    try:
        result = validate_images(args.runtime, args.debug, args.runtime_handoff, args.debug_handoff,
                                 args.runtime_manifest, args.debug_manifest, args.source_root, policy=read_policy(args.policy),
                                 source_contract_path=args.source_contract, apt_lock_path=args.apt_lock,
                                 dpkg_filter_path=args.dpkg_filter_file, fixture=args.fixture,
                                 base_path=args.base, apt_layer=args.apt_layer, debug_tools_layer=args.debug_tools_layer,
                                 runtime_links_layer=args.runtime_links_layer,
                                 runtime_apt_selection=args.runtime_apt_selection, debug_apt_selection=args.debug_apt_selection,
                                 package_state_contract=args.package_state_contract, runtime_archive=args.runtime_archive,
                                 debug_archive=args.debug_archive, source_runtime_tar=args.source_runtime_tar,
                                 source_debug_tar=args.source_debug_tar, source_receipt=args.source_receipt, base_debug_symbols=args.base_debug_symbols,
                                 base_debug_receipt=args.base_debug_receipt,
                                 base_debug_package_manifest=args.base_debug_package_manifest)
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError, tarfile.TarError) as error:
        parser.exit(1, "OCI validation failed: " + str(error) + "\n")


if __name__ == "__main__":
    main()
