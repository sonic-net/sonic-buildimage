#!/usr/bin/env python3
"""Validate container package contracts using explicit owner declarations.

Source archives, imported-package metadata and installed-file expectations keep
separate input formats. This module checks their declared identities and actual
contents without a built-in list of containers, packages or source repositories.
"""

import argparse
import ast
import copy
import fnmatch
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import tarfile

from tools.bazel.ci.validation_inputs import cached_inventory
import tempfile

from tools.bazel.ci.artifact_validation import file_metadata, metadata, path_name, require, sha
from tools.bazel.oci.oci_inventory import apply_layer, assert_overlay_paths, resolve_path
from tools.bazel.oci.oci_layout import validate_layout
from tools.bazel.oci import normalize_layer as normalization


# Required dependency keys in schema-1 contracts. Other Debian fields stay in
# the contract and receipt so the owner record is preserved in full.
CONTROL_FIELDS = {"Package", "Version", "Architecture", "Depends", "Pre-Depends", "Provides", "Multi-Arch"}
SOURCE_FIELDS = {"module", "version", "target"}
RECEIPT_SCHEMA = 2


def json_bytes(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def elf_machine(architecture):
    machines = {"amd64": 62, "arm64": 183}
    require(architecture in machines, "unsupported package ELF architecture: " + str(architecture))
    return machines[architecture]


def identity(document):
    """Read the owner identity shared by contracts, policies and receipts."""
    require(isinstance(document, dict) and all(isinstance(document.get(key), str) and document[key]
                for key in ("image", "architecture", "distribution")), "invalid package contract identity")
    elf_machine(document["architecture"])
    return {key: document[key] for key in ("image", "architecture", "distribution")}


def package_names(contract):
    return frozenset(record["package"] for record in contract["packages"])


def source_package_names(contract):
    """Names forbidden in imports because their source owner supplies the runtime and symbols."""
    names = package_names(contract)
    return names | {name + "-dbgsym" for name in names}


def reject_source_packages(records, contract):
    unexpected = source_package_names(contract).intersection(record.get("package") for record in records)
    require(not unexpected, "import contains packages now built from source: " + ", ".join(sorted(unexpected)))


def validate_policy(policy):
    """Check the common policy interface; consumer sections retain their own validators."""
    required = {"schema", "image", "architecture", "distribution", "retained_source", "features",
                "debug_tools", "retained_requirements"}
    optional = {"install_inventory", "libraries", "required_debug_paths", "config_paths", "native", "image_validation"}
    require(isinstance(policy, dict) and required <= policy.keys() and policy.keys() <= required | optional,
            "invalid package policy fields")
    require(type(policy["schema"]) is int and policy["schema"] == 1, "unsupported package policy schema")
    identity(policy)
    require(policy["retained_source"] in ("none", "make", "source"), "invalid retained package source")
    require(isinstance(policy["features"], dict) and all(isinstance(k, str) and isinstance(v, str)
                for k, v in policy["features"].items()), "invalid package policy features")
    tools = policy["debug_tools"]
    require(isinstance(tools, list) and all(isinstance(name, str) and name for name in tools) and
            len(tools) == len(set(tools)), "invalid debug tool policy")
    requirements = policy["retained_requirements"]
    require(isinstance(requirements, list), "invalid retained package requirements")
    names = set()
    for rule in requirements:
        require(isinstance(rule, dict) and {"package", "version_contains", "debug"} <= rule.keys() and
                rule.keys() <= {"package", "version_contains", "debug", "runtime_elf_paths"} and
                isinstance(rule["package"], str) and rule["package"] and
                isinstance(rule["version_contains"], str) and rule["version_contains"] and
                rule["debug"] == "inherit" and rule["package"] not in names,
                "invalid or duplicate retained package requirement")
        paths = rule.get("runtime_elf_paths", [])
        require(isinstance(paths, list) and all(isinstance(name, str) and path_name(name) == name for name in paths),
                "invalid retained runtime ELF paths")
        names.add(rule["package"])
    for section in optional - {"required_debug_paths"}:
        if section in policy:
            require(isinstance(policy[section], dict), "invalid package policy section: " + section)
    if "required_debug_paths" in policy:
        require(isinstance(policy["required_debug_paths"], list) and
                all(isinstance(name, str) and path_name(name) == name for name in policy["required_debug_paths"]),
                "invalid required debug paths")
    return policy


def read_policy(path):
    return validate_policy(json.loads(path.read_bytes()))


def validate_retained_packages(records, policy, variant):
    """Enforce declared runtime requirements and unchanged debug inheritance."""
    validate_policy(policy)
    require(variant in ("runtime", "debug"), "invalid retained package variant")
    require(isinstance(records, list) and all(isinstance(record, dict) for record in records),
            "invalid retained package records")
    for rule in policy["retained_requirements"]:
        matches = [record for record in records if record.get("package") == rule["package"]]
        if variant == "debug":
            require(not matches, "debug must inherit runtime package: " + rule["package"])
            continue
        require(len(matches) == 1 and isinstance(matches[0].get("version"), str) and
                rule["version_contains"] in matches[0]["version"],
                "runtime package handoff requires retained package " + rule["package"] +
                " with version containing " + rule["version_contains"])
        record = matches[0]
        fields = record.get("control_fields", {})
        architectures = (policy["architecture"],) if rule.get("runtime_elf_paths") else (policy["architecture"], "all")
        require(isinstance(fields, dict) and
                (fields.get("Package"), fields.get("Version"), fields.get("Architecture")) ==
                (record["package"], record["version"], record.get("architecture")) and
                record.get("architecture") in architectures,
                "retained package identity differs from its Debian control: " + rule["package"])


def package_identity(record, *, architecture):
    """Validate package controls and commit-pinned public source provenance."""
    require(isinstance(record, dict), "invalid source package record")
    name = record.get("package")
    require(isinstance(name, str) and re.fullmatch(r"[a-z0-9][a-z0-9+.-]*", name) and
            isinstance(record.get("version"), str) and record["version"] and
            record.get("architecture") == architecture, "unsupported source package identity")
    fields = record.get("control_fields")
    require(isinstance(fields, dict) and CONTROL_FIELDS <= fields.keys() and
            all(isinstance(key, str) and re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9-]*", key) for key in fields) and
            all(isinstance(value, str) for value in fields.values()) and
            (fields["Package"], fields["Version"], fields["Architecture"]) ==
            (name, record["version"], record["architecture"]),
            "invalid source package dependency controls: " + name)
    source = record.get("source")
    source_identity(source, name=name)


def source_identity(source, *, name):
    """Check SONiC commit and foreign release identities through one contract."""
    require(isinstance(source, dict) and set(source) in
            (SOURCE_FIELDS | {"commit"}, SOURCE_FIELDS | {"release"}) and
            all(isinstance(value, str) and value for value in source.values()),
            "invalid source package provenance: " + name)
    if "commit" in source:
        pinned = (re.fullmatch(r"[0-9a-f]{40}", source["commit"]) and
                  source["version"].endswith("-" + source["commit"]))
    else:
        # Foreign packages retain their upstream release and SONiC patch number.
        # Bazel verifies the registry archive/patch integrity; verify_modules
        # below binds this contract to the module actually selected by Bazel.
        pinned = (re.fullmatch(r"[0-9]+(?:\.[0-9]+){2,3}", source["release"]) and
                  re.fullmatch(re.escape(source["release"]) + r"\.sonic\.[1-9][0-9]*", source["version"]))
    require(re.fullmatch(r"@{1,2}[A-Za-z0-9_.+~-]+//[A-Za-z0-9_./+-]*:[A-Za-z0-9_./+-]+", source["target"]) and pinned,
            "source package does not identify its public owner target: " + name)


def read_contract(path):
    """Read reviewed source metadata; actual public target bindings are checked at assembly."""
    value = json.loads(path.read_bytes())
    identity(value)
    require(type(value.get("schema")) is int and value["schema"] == 1, "unsupported source package contract")
    packages = value.get("packages")
    require(isinstance(packages, list) and packages, "source package contract must describe its packages")
    names = set()
    for record in packages:
        package_identity(record, architecture=value["architecture"])
        require(record["package"] not in names, "duplicate source package declaration: " + record["package"])
        names.add(record["package"])
        paths = record.get("required_paths")
        require(isinstance(paths, dict) and
                all(isinstance(name, str) and kind in ("file", "elf", "symlink") for name, kind in paths.items()),
                "source package contract lacks required paths: " + record["package"])
        for name in paths:
            require(path_name(name) == name, "noncanonical required source path: " + name)
        require(not {"path_modes", "path_owners"}.intersection(record),
                "source package metadata must come from its producer TAR")
        inherited = record.get("required_inherited_files", {})
        require(isinstance(inherited, dict), "invalid inherited source package files")
        for name, item in inherited.items():
            require(path_name(name) == name and isinstance(item, dict) and item.get("kind") in ("file", "symlink"),
                    "invalid inherited source package file: " + name)
    return value


def verify_modules(contract, module_files):
    """Bind provenance to the resolved registry modules, including their versions."""
    expected = {record["source"]["module"]: record["source"]["version"] for record in contract["packages"]}
    require(all(expected[record["source"]["module"]] == record["source"]["version"]
                for record in contract["packages"]), "source packages disagree on their owner module version")
    require(set(module_files) == set(expected), "missing or unexpected source module files")
    digests = {}
    for name, version in expected.items():
        raw = module_files[name].read_bytes()
        definitions = re.findall(r"(?m)^module\s*\((.*?)\)", raw.decode(), re.DOTALL)
        require(len(definitions) == 1, "source module declaration is missing or ambiguous: " + name)
        fields = {key: re.findall(r"\b" + key + r"\s*=\s*[\"']([^\"']+)[\"']", definitions[0])
                  for key in ("name", "version")}
        require(fields == {"name": [name], "version": [version]},
                "resolved source module differs from the reviewed contract: " + name)
        digests[name] = hashlib.sha256(raw).hexdigest()
    return digests


def normalized_source_member(member):
    """Validate a direct producer layer without rewriting its paths or metadata."""
    validate_member_metadata(member)
    return normalization.normalized_member(member, directory_aliases={})


def validate_member_metadata(member):
    require(all(type(value) is int and 0 <= value < 2**32 - 1 for value in (member.uid, member.gid)) and
            type(member.mode) is int and 0 <= member.mode <= 0o7777,
            "source package archive has invalid ownership or permissions: " + member.name)


def inventory(path):
    """Inventory normalized tar bytes without following host filesystem links."""
    return cached_inventory(path, "source_package", lambda: _inventory(path))


def _inventory(path):
    files = {}
    with tarfile.open(path, "r:*") as archive:
        for member in archive:
            name = path_name(member.name)
            require(name not in files, "duplicate source package archive path: " + name)
            require(member.isfile() or member.isdir() or member.issym() or member.islnk(),
                    "unsupported source package archive member: " + name)
            require(not PurePosixPath(name).name.startswith(".wh."), "source package archive contains a whiteout")
            item = metadata(member)
            validate_member_metadata(member)
            if member.isfile():
                item.update(file_metadata(archive.extractfile(member), name))
            files[name] = item
    assert_overlay_paths(files, {})
    return files


def combine_tars(contract, paths, output_path, base_files):
    """Normalize declared owner tars, preserving per-package inventories and bytes."""
    require(set(paths) == package_names(contract), "missing or unexpected shared-library tar inputs")
    files, records = {}, {}
    with tarfile.open(output_path, "w", format=tarfile.PAX_FORMAT) as output:
        for record in contract["packages"]:
            name = record["package"]
            before = sha(paths[name])
            package_files = {}
            with tarfile.open(paths[name], "r:*") as source:
                for member in source:
                    normalized = normalized_source_member(member)
                    if normalized is None:
                        continue
                    path = path_name(normalized.name)
                    item = metadata(normalized)
                    if member.isfile():
                        item.update(file_metadata(source.extractfile(member), path))
                    require(path not in package_files or
                            (item["kind"] == "directory" and package_files[path] == item),
                            "duplicate source package path: " + name + ":" + path)
                    require("elf_machine" not in item or item["elf_machine"] == elf_machine(contract["architecture"]),
                            "source package has a foreign ELF architecture: " + name + ":" + path)
                    package_files[path] = item
                    if path in files:
                        require(item["kind"] == "directory" and files[path] == item,
                                "shared source packages overlap: " + path)
                        continue
                    files[path] = item
                    output.addfile(normalized, source.extractfile(member) if member.isfile() else None)
            require(sha(paths[name]) == before, "source package tar changed during assembly: " + name)
            for path, kind in record["required_paths"].items():
                item = package_files.get(path, {})
                require(item.get("kind") == ("file" if kind == "elf" else kind) and
                        (kind != "elf" or item.get("elf_machine") == elf_machine(contract["architecture"])),
                        "source package lacks a required installed path: " + name + ":" + path)
            records[name] = {"input_tar_sha256": before, "files": package_files}
    assert_overlay_paths(files, base_files)
    installed = base_files | files
    for path, item in files.items():
        if item["kind"] in ("symlink", "hardlink"):
            require(resolve_path(path, installed) in installed,
                    "source package has an unresolved or unsafe link: " + path)
    return records, {"sha256": sha(output_path), "size": output_path.stat().st_size, "members": len(files)}


def assemble(contract_path, packages, module_files, base, runtime_tar, *, package_targets):
    """Normalize runtime owner archives and record their dependency metadata.

    Debug symbols follow the owners' DebugSymbolsInfo graph independently;
    a runtime build never consumes a debug-symbol archive.
    """
    contract = read_contract(contract_path)
    expected_targets = {record["package"]: record["source"]["target"] for record in contract["packages"]}
    require(package_targets == expected_targets, "source inputs differ from the declared public owner targets")
    modules = verify_modules(contract, module_files)
    base_digest = normalization.base_aliases(base, expected_platform="linux/" + contract["architecture"])
    base_files = {}
    for layer in validate_layout(base, "linux/" + contract["architecture"]).layers:
        apply_layer(layer, base_files)
    inherited = {}
    for record in contract["packages"]:
        for path, expected in record.get("required_inherited_files", {}).items():
            actual = base_files.get(path, {})
            require(all(actual.get(key) == value for key, value in expected.items()),
                    "source package requires unchanged inherited content: " + path)
            inherited[path] = actual
    runtime_tar.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".source-packages-", dir=runtime_tar.parent) as temporary:
        runtime_output = Path(temporary) / "runtime.tar"
        runtime_records, payload = combine_tars(contract, packages, runtime_output, base_files)
        runtime_files = dict(base_files)
        apply_layer(runtime_output, runtime_files, checked_overlay=True)
        require(all(runtime_files.get(path) == expected for path, expected in inherited.items()),
                "source package changes required inherited content")
        records = []
        for record in contract["packages"]:
            name = record["package"]
            records.append({key: copy.deepcopy(record[key]) for key in
                            ("package", "version", "architecture", "control_fields", "source")} |
                           runtime_records[name])
        result = {**identity(contract), "schema": RECEIPT_SCHEMA, "kind": "bazel_source", "contract_sha256": sha(contract_path),
                  "base_manifest_digest": base_digest, "module_file_sha256": modules,
                  "packages": records, "payload": payload,
                  "inherited_files": inherited}
        runtime_output.replace(runtime_tar)
    return result


def receipt_records(value, contract=None):
    """Check receipt structure and, when supplied, its independently declared package owners."""
    identity(value)
    require(type(value.get("schema")) is int and value["schema"] == RECEIPT_SCHEMA and
            value.get("kind") == "bazel_source" and "debug_payload" not in value,
            "unsupported source package receipt; expected runtime-only schema 2")
    require(re.fullmatch(r"[0-9a-f]{64}", value.get("contract_sha256", "")) and
            re.fullmatch(r"sha256:[0-9a-f]{64}", value.get("base_manifest_digest", "")),
            "source receipt lacks its contract or base identity")
    records = value.get("packages")
    require(isinstance(records, list) and records, "source receipt must contain its source packages")
    names = set()
    for record in records:
        package_identity(record, architecture=value["architecture"])
        require(record["package"] not in names, "duplicate source receipt package: " + record["package"])
        names.add(record["package"])
        require("debug" not in record, "runtime source receipt must not contain debug inputs")
        require(re.fullmatch(r"[0-9a-f]{64}", record.get("input_tar_sha256", "")) and
                isinstance(record.get("files"), dict) and record["files"],
                "source receipt lacks a package tar identity and inventory")
    modules = value.get("module_file_sha256")
    expected_modules = {record["source"]["module"] for record in records}
    require(isinstance(modules, dict) and set(modules) == expected_modules and
            all(isinstance(digest, str) and re.fullmatch(r"[0-9a-f]{64}", digest) for digest in modules.values()),
            "source receipt lacks its resolved module identities")
    if contract is not None:
        require(identity(value) == identity(contract), "source receipt differs from the contract identity")
        expected_records = {record["package"]: record for record in contract["packages"]}
        require(names == set(expected_records), "source receipt differs from the declared package owners")
        for record in records:
            expected = expected_records[record["package"]]
            require(all(record[key] == expected[key] for key in
                        ("package", "version", "architecture", "control_fields", "source")),
                    "source receipt package differs from the reviewed contract: " + record["package"])
    return records


def validate_receipt(receipt_path, runtime_tar, debug_tar=None, *, contract_path=None, fixture=False):
    """Bind source archives to the reviewed contract, package owners and observed bytes.

    Production validation always supplies the expected contract. A synthetic
    fixture can explicitly request structural checks without an owner contract.
    """
    require(fixture or contract_path is not None, "source receipt validation requires its reviewed contract")
    value = json.loads(receipt_path.read_bytes())
    contract = read_contract(contract_path) if contract_path is not None else None
    records = receipt_records(value, contract)
    if contract is not None:
        require(value["contract_sha256"] == sha(contract_path), "source receipt uses a different reviewed contract")
        expected_records = {record["package"]: record for record in contract["packages"]}
        expected_inherited = {path: item for record in contract["packages"]
                              for path, item in record.get("required_inherited_files", {}).items()}
        inherited = value.get("inherited_files", {})
        require(set(inherited) == set(expected_inherited) and all(
                    all(inherited[path].get(key) == val for key, val in expected.items())
                    for path, expected in expected_inherited.items()),
                "source receipt differs from required inherited content")
        for record in records:
            expected = expected_records[record["package"]]
            for path, kind in expected["required_paths"].items():
                item = record["files"].get(path, {})
                require(item.get("kind") == ("file" if kind == "elf" else kind) and
                        (kind != "elf" or item.get("elf_machine") == elf_machine(contract["architecture"])),
                        "source receipt lacks a required installed path: " + path)
    descriptor = value.get("payload", {})
    require(runtime_tar.is_file() and sha(runtime_tar) == descriptor.get("sha256") and
            runtime_tar.stat().st_size == descriptor.get("size"), "source receipt archive hash differs: payload")
    expected = {}
    for record in records:
        for name, item in record["files"].items():
            require(name not in expected or (item.get("kind") == "directory" and expected[name] == item),
                    "source receipt packages overlap: " + name)
            expected[name] = item
    require(inventory(runtime_tar) == expected and len(expected) == descriptor.get("members"),
            "source receipt archive inventory differs: payload")
    require(all(item.get("elf_machine", elf_machine(value["architecture"])) == elf_machine(value["architecture"])
                for item in expected.values()), "source receipt archive has a foreign ELF architecture")
    if debug_tar is not None:
        debug_inventory(debug_tar, architecture=value["architecture"])
    return value


def debug_inventory(path, *, architecture):
    """Check a symbol-only layer; native validation separately matches its actual runtime bytes."""
    files = inventory(path)
    require(all(item["uid"] == item["gid"] == 0 for item in files.values()),
            "source debug archive differs from root ownership")
    machine = elf_machine(architecture)
    require(any(item.get("elf_machine") == machine for item in files.values()), "source layer has no debug symbols")
    for name, item in files.items():
        require(item["kind"] == "directory" or (
            re.fullmatch(r"usr/lib/debug/\.build-id/[0-9a-f]{2}/[0-9a-f]+\.debug", name) and
            item.get("elf_machine") == machine), "source symbols contain an unexpected file: " + name)
    return files


def retained_manifest(manifest_path, receipt, receipt_sha256, *, variant, contract, policy):
    """Augment selection metadata while keeping source outputs distinct from imported DEBs."""
    expected = read_contract(contract)
    validate_policy(policy)
    require(policy["retained_source"] == "make", "retained manifest merge requires the declared imported-package adapter")
    require(identity(expected) == identity(policy), "package policy differs from its source contract identity")
    receipt_records(receipt, expected)
    require(receipt["contract_sha256"] == sha(contract), "source receipt uses a different reviewed contract")
    require(variant in ("runtime", "debug"), "invalid retained source package variant")
    raw = manifest_path.read_bytes()
    value = json.loads(raw)
    require(isinstance(value, dict) and value.get("schema") == 1 and identity(value) == identity(policy) and
            value.get("variant") == variant and value.get("features") == policy["features"],
            "invalid Make manifest for source package selection")
    require(not any(key in value for key in ("source_packages", "source_receipt_sha256", "make_manifest_sha256")),
            "Make manifest already contains source package extensions")
    records = value.get("packages", [])
    require(isinstance(records, list) and all(isinstance(record, dict) for record in records),
            "invalid imported package records")
    reject_source_packages(records, expected)
    names = {record["package"] for record in records}
    require(len(names) == len(records), "duplicate imported package records")
    require(set(value.get("required_packages", [])).issubset(names), "missing required imported package")
    validate_retained_packages(records, policy, variant)
    if variant == "runtime":
        require(not value.get("debug_apt_packages"), "runtime import contains debug tools")
    else:
        require(set(value.get("debug_apt_packages", [])) == set(policy["debug_tools"]),
                "debug import has an unsupported tool set")
    value.update(source_packages=copy.deepcopy(receipt["packages"]),
                 source_receipt_sha256=receipt_sha256,
                 make_manifest_sha256=hashlib.sha256(raw).hexdigest())
    return value


def _source_path(name):
    require(isinstance(name, str) and name and path_name(name) == name and name != ".",
            "invalid source install path: " + str(name))
    return Path(name)


def _label_path(label):
    require(isinstance(label, str) and label.startswith("//") and label.count(":") == 1,
            "expected a source-local install label: " + str(label))
    package, name = label[2:].split(":", 1)
    require(name, "empty source install label")
    return _source_path(str(PurePosixPath(package) / name))


def validate_install_inventory(source, files, policy):
    """Compare installed bytes with literal source declarations and Debian install metadata.

    Owners supply declaration names, destinations and generated-program paths.
    No source expressions are evaluated, and the inventory remains source-owned.
    """
    required = {"declaration_file", "groups", "install_file", "generated_programs",
                "executable_directories", "expected_program_count", "architecture"}
    require(isinstance(policy, dict) and set(policy) == required, "invalid install inventory policy fields")
    machine = elf_machine(policy["architecture"])
    groups = policy["groups"]
    require(isinstance(groups, list) and groups, "install inventory needs declaration groups")
    variables = set()
    for group in groups:
        require(isinstance(group, dict) and {"variable", "destination", "kind", "mode"} <= group.keys() and
                group.keys() <= {"variable", "destination", "kind", "mode", "aliases_variable"} and
                isinstance(group["variable"], str) and group["variable"].isidentifier() and
                group["kind"] in ("elf", "file") and type(group["mode"]) is int and
                0 <= group["mode"] <= 0o777, "invalid install inventory declaration group")
        _source_path(group["destination"])
        require(group["variable"] not in variables, "duplicate install inventory declaration group")
        variables.add(group["variable"])
        if "aliases_variable" in group:
            require(isinstance(group["aliases_variable"], str) and group["aliases_variable"].isidentifier() and
                    group["aliases_variable"] not in variables, "invalid install alias declaration")
            variables.add(group["aliases_variable"])
    values = {}
    declarations = source / _source_path(policy["declaration_file"])
    for node in ast.parse(declarations.read_text()).body:
        if isinstance(node, ast.Assign) and len(node.targets) == 1:
            name = getattr(node.targets[0], "id", None)
            if name in variables:
                require(name not in values, "duplicate source install declaration: " + name)
                values[name] = ast.literal_eval(node.value)
    require(values.keys() == variables, "missing source install declarations")
    expected, programs = {}, set()
    for group in groups:
        labels = values[group["variable"]]
        require(isinstance(labels, (list, tuple)) and all(isinstance(label, str) for label in labels),
                "invalid source install label list")
        aliases = values[group["aliases_variable"]] if "aliases_variable" in group else {}
        require(isinstance(aliases, dict) and aliases.keys() <= set(labels),
                "install alias has no declared source file")
        for label in labels:
            name = str(PurePosixPath(group["destination"]) / _label_path(label).name)
            require(name not in expected, "duplicate source install path: " + name)
            binary = group["kind"] == "elf"
            origin = None if binary else source / _label_path(aliases.get(label, label))
            expected[name] = (group["mode"], origin, binary)
            if binary:
                programs.add(name)
    generated = policy["generated_programs"]
    executable_dirs = policy["executable_directories"]
    require(isinstance(generated, list) and isinstance(executable_dirs, list), "invalid install program policy")
    for name in [*generated, *executable_dirs]:
        _source_path(name)
    for line in (source / _source_path(policy["install_file"])).read_text().splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        origin, directory = line.split()
        _source_path(origin)
        _source_path(directory)
        name = str(PurePosixPath(directory) / PurePosixPath(origin).name)
        require(name not in expected, "duplicate source install path: " + name)
        binary = origin in generated
        expected[name] = (0o755 if directory in executable_dirs else 0o644,
                          None if binary else source / origin, binary)
        if binary:
            programs.add(name)
    actual = {name for name, item in files.items() if item["kind"] != "directory"}
    require(actual == set(expected), "package differs from the source install contract")
    count = policy["expected_program_count"]
    require(type(count) is int and count >= 0 and len(programs) == count,
            "package differs from the declared complete program count")
    for name, (mode, origin, binary) in expected.items():
        require(files[name]["kind"] == "file" and files[name]["mode"] == mode,
                "installed type or mode differs: " + name)
        if binary:
            require(files[name].get("elf_machine") == machine, "installed executable has an unexpected ELF architecture: " + name)
        else:
            require(files[name]["sha256"] == sha(origin), "installed data differs from source: " + name)
    return sorted(programs)


def validate_library_payload(combined, original, policy):
    """Preserve a source library payload and its declared SONAME links without conflicting copies."""
    require(isinstance(policy, dict) and set(policy) == {
        "architecture", "runtime_path", "soname_links", "exclusive_pattern"}, "invalid library payload policy fields")
    runtime = str(_source_path(policy["runtime_path"]))
    require(original.get(runtime, {}).get("elf_machine") == elf_machine(policy["architecture"]),
            "missing source-built runtime library: " + runtime)
    for name, item in original.items():
        if item["kind"] != "directory":
            require(combined.get(name) == dict(item, uid=0, gid=0),
                    "runtime layer changes source library bytes or modes: " + name)
    links = policy["soname_links"]
    require(isinstance(links, dict), "invalid library SONAME links")
    for name, target in links.items():
        _source_path(name)
        require(isinstance(target, str) and target and combined.get(name, {}).get("kind") == "symlink" and
                combined[name].get("linkname") == target, "incorrect source library SONAME link: " + name)
    pattern = policy["exclusive_pattern"]
    require(isinstance(pattern, str) and pattern, "missing exclusive library path pattern")
    require({name for name, item in combined.items() if fnmatch.fnmatchcase(name, pattern) and item["kind"] != "directory"} ==
            {runtime, *links}, "conflicting runtime library in declared layer")


def bindings(values, *, paths=True):
    result = {}
    for value in values:
        name, separator, item = value.partition("=")
        require(separator and name and item and name not in result, "invalid or duplicate source input binding: " + value)
        result[name] = Path(item) if paths else item
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("contract", "policy", "base", "out-tar", "receipt", "merge-manifest", "source-receipt", "out-manifest"):
        parser.add_argument("--" + name, type=Path)
    for name in ("package", "package-target", "module"):
        parser.add_argument("--" + name, action="append", default=[])
    args = parser.parse_args()
    try:
        if args.merge_manifest is not None:
            require(all((args.contract, args.policy, args.source_receipt, args.out_manifest)),
                    "manifest merge requires a contract, policy, source receipt and output manifest")
            require(not any((args.base, args.out_tar, args.receipt, args.package, args.package_target, args.module)),
                    "manifest merge cannot also assemble source packages")
            result = json.loads(args.source_receipt.read_bytes())
            variant = json.loads(args.merge_manifest.read_bytes()).get("variant")
            combined = retained_manifest(args.merge_manifest, result, sha(args.source_receipt), variant=variant,
                                         contract=args.contract, policy=read_policy(args.policy))
            args.out_manifest.parent.mkdir(parents=True, exist_ok=True)
            args.out_manifest.write_bytes(json_bytes(combined))
            return
        require(args.source_receipt is None and args.out_manifest is None and args.policy is None and
                all((args.contract, args.base, args.out_tar, args.receipt)),
                "source assembly requires its contract, base, runtime archive and receipt")
        result = assemble(args.contract, bindings(args.package), bindings(args.module), args.base, args.out_tar,
                          package_targets=bindings(args.package_target, paths=False))
        args.receipt.parent.mkdir(parents=True, exist_ok=True)
        args.receipt.write_bytes(json_bytes(result))
    except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError, tarfile.TarError) as error:
        parser.exit(1, "Source package validation failed: " + str(error) + "\n")


if __name__ == "__main__":
    main()
