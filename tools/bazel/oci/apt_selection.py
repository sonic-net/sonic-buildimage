"""Select APT payloads using a declared image policy and checked OCI/Make inputs."""

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import tarfile

from sonic_apt import dependencies, selection
from layer_assembly import assemble, read_patterns
from tools.bazel.ci.artifact_validation import require
from tools.bazel.oci import base_inventory as base_metadata
from tools.bazel.oci.oci_inventory import apply_layer, assert_overlay_paths
from tools.bazel.oci.package_contract import identity, receipt_records, source_identity


def read_policy(path):
    """Reject incomplete policies rather than silently weakening package checks."""
    policy = json.loads(path.read_bytes())
    fields = {"schema", "image", "architecture", "distribution", "retained_source", "features"}
    require(isinstance(policy, dict) and fields <= set(policy) <= fields | {"static_i386"},
            "invalid APT policy fields")
    require(type(policy["schema"]) is int and policy["schema"] == 1 and
            all(isinstance(policy[key], str) and policy[key]
                for key in ("image", "architecture", "distribution")) and
            policy["retained_source"] in ("none", "make", "source"), "invalid APT policy identity")
    require(isinstance(policy["features"], dict) and
            all(isinstance(key, str) and isinstance(value, str)
                for key, value in policy["features"].items()), "invalid APT policy features")
    require(policy["retained_source"] == "make" or not policy["features"],
            "Make policy requires retained_source=make")
    compatibility = policy.get("static_i386", {})
    require(isinstance(compatibility, dict) and all(
        isinstance(name, str) and not PurePosixPath(name).is_absolute() and
        ".." not in PurePosixPath(name).parts and str(PurePosixPath(name)) == name and
        isinstance(value, str) and re.fullmatch("[0-9a-f]{64}", value)
        for name, value in compatibility.items()), "invalid static i386 compatibility policy")
    require(not compatibility or policy["architecture"] == "amd64",
            "static i386 compatibility requires target amd64")
    return policy


def retained_packages(path, *, policy, variant):
    """Bind imported and source-built package controls to their declared inputs."""
    document_bytes = path.read_bytes()
    make = json.loads(document_bytes)
    require(make.get("schema") == 1 and make.get("image") == policy["image"] and
            make.get("variant") == variant and make.get("architecture") == policy["architecture"] and
            make.get("distribution") == policy["distribution"] and make.get("features") == policy["features"],
            "invalid Make package manifest")
    retained = {item["package"]: item for item in make.get("packages", [])}
    require(retained and len(retained) == len(make["packages"]), "missing or duplicate Make packages")
    for name, item in retained.items():
        fields = item.get("control_fields")
        require(isinstance(fields, dict) and
                all(fields.get(field) == item[key] for field, key in
                    (("Package", "package"), ("Version", "version"), ("Architecture", "architecture"))),
                "Make package handoff lacks full original control metadata: " + name)
        item["control"] = dependencies.control_fields(
            dependencies.package_from_fields(fields, origin="Make " + name))
    manifest_digest = hashlib.sha256(document_bytes).hexdigest()
    if "source_packages" in make:
        sources = make["source_packages"]
        require(isinstance(sources, list) and sources, "empty source package inventory")
        for field in ("make_manifest_sha256", "source_receipt_sha256"):
            require(isinstance(make.get(field), str) and re.fullmatch(r"[0-9a-f]{64}", make[field]),
                    "mixed package manifest lacks " + field)
        for item in sources:
            require(isinstance(item, dict), "invalid source package record")
            name = item.get("package")
            require(isinstance(name, str) and name and name not in retained,
                    "duplicate imported/source package identity: " + str(name))
            fields, source = item.get("control_fields"), item.get("source")
            require(isinstance(fields, dict) and all(fields.get(field) == item.get(key) for field, key in
                    (("Package", "package"), ("Version", "version"), ("Architecture", "architecture"))),
                    "source package lacks dependency controls: " + name)
            source_identity(source, name=name)
            require(isinstance(item.get("input_tar_sha256"), str) and
                    re.fullmatch(r"[0-9a-f]{64}", item["input_tar_sha256"]),
                    "source package lacks build provenance: " + name)
            retained[name] = {
                "control": dependencies.control_fields(
                    dependencies.package_from_fields(fields, origin="Bazel source " + name)),
                # Infrastructure calls this source_sha256 for every retained
                # input. For a source target it identifies its actual TAR,
                # never a Debian archive that did not produce these bytes.
                "source_sha256": item["input_tar_sha256"],
            }
        manifest_digest = make["make_manifest_sha256"]
    return retained, make, manifest_digest


def source_packages(path, *, policy):
    """Read the actual source TAR receipt without an imported-package envelope."""
    raw = path.read_bytes()
    receipt = json.loads(raw)
    records = receipt_records(receipt)
    require(identity(receipt) == identity(policy), "source receipt differs from APT image policy")
    retained = {item["package"]: {
        "control": dependencies.control_fields(dependencies.package_from_fields(
            item["control_fields"], origin="Bazel source " + item["package"])),
        "source_sha256": item["input_tar_sha256"],
    } for item in records}
    return retained, receipt, hashlib.sha256(raw).hexdigest()


def check_runtime_manifest(metadata_path, *, variant, make):
    """Bind the debug package inventory to its exact runtime Make handoff."""
    if variant == "runtime":
        require(metadata_path is None, "runtime cannot consume inherited package metadata")
        return
    require(metadata_path is not None, "debug requires the runtime package selection receipt")
    document = json.loads(metadata_path.read_bytes())
    require(document.get("variant") == "runtime" and
            make.get("runtime_manifest_sha256") == document.get("make_manifest_sha256") and
            re.fullmatch(r"[0-9a-f]{64}", make.get("runtime_manifest_sha256", "")),
            "debug manifest does not match the runtime Make package selection")
    require(document.get("source_receipt_sha256") == make.get("source_receipt_sha256"),
            "debug manifest does not match the runtime source packages")


def select(base, lock, policy_path, mapping, *, variant, retained_manifest=None,
           base_package_metadata=None, base_inventory=None):
    """Apply a declarative policy without importing any container-owned Python."""
    require(variant in ("runtime", "debug"), "unsupported APT variant")
    policy = read_policy(policy_path)
    retained = {}
    if policy["retained_source"] == "make":
        require(retained_manifest is not None, "Make policy requires a retained manifest")
        retained, make, manifest_digest = retained_packages(
            retained_manifest, policy=policy, variant=variant)
        check_runtime_manifest(base_package_metadata, variant=variant, make=make)
    elif policy["retained_source"] == "source":
        require(retained_manifest is not None, "source policy requires a source receipt")
        retained, source, source_digest = source_packages(retained_manifest, policy=policy)
        if variant == "debug":
            require(base_package_metadata is not None, "debug selection requires matching runtime source metadata")
            metadata = json.loads(base_package_metadata.read_bytes())
            require(metadata.get("variant") == "runtime" and
                    metadata.get("source_receipt_sha256") == source_digest,
                    "debug selection requires matching runtime source metadata")
        else:
            require(base_package_metadata is None, "runtime cannot consume inherited package metadata")
    else:
        require(retained_manifest is None, "APT policy does not permit a retained manifest")

    architecture = policy["architecture"]
    base_state = base_metadata.read(base, architecture, inventory=base_inventory,
                                    static_i386=policy.get("static_i386"))
    base_digest = base_state["manifest_digest"]
    if policy["retained_source"] == "source" and variant == "runtime":
        require(source["base_manifest_digest"] == base_digest, "source receipt uses a different OCI base")
    files = base_metadata.installed_files(base_state["files"])
    require(base_state["dpkg_status"] is not None, "OCI base lacks dpkg status")
    installed = dependencies.installed_packages(base_state["dpkg_status"], origin="OCI base")
    require(installed, "OCI base has no installed packages")
    require(all(package.architecture in (architecture, "all") for package in installed.values()),
            "invalid or foreign installed package in OCI base")

    def inspect_payload(path):
        entries = {}
        apply_layer(path, entries, checked_overlay=True)
        return entries

    selected, receipt = selection.select(
        lock, mapping, group=variant, architecture=policy["architecture"],
        installed=installed,
        base_files=files, retained_packages=retained,
        inspect_payload=inspect_payload, check_overlay=assert_overlay_paths,
        base_package_metadata=base_package_metadata)
    receipt["base_manifest_digest"] = base_digest
    # Keep the existing receipt contracts consumed by image/package-state checks.
    if policy["retained_source"] == "make":
        receipt["provided_package_replacements"] = []
        receipt["variant"] = receipt.pop("group")
        receipt["skipped_make"] = receipt.pop("skipped_retained")
        receipt["make_manifest_sha256"] = manifest_digest
        if "source_packages" in make:
            names = {item["package"] for item in make["source_packages"]}
            receipt["skipped_source"] = [item for item in receipt["skipped_make"] if item["package"] in names]
            receipt["skipped_make"] = [item for item in receipt["skipped_make"] if item["package"] not in names]
            receipt["source_receipt_sha256"] = make["source_receipt_sha256"]
            receipt["retained_manifest_sha256"] = hashlib.sha256(retained_manifest.read_bytes()).hexdigest()
            receipt["source_packages"] = [{key: item[key] for key in
                ("package", "version", "architecture", "source", "input_tar_sha256")}
                for item in make["source_packages"]]
    elif policy["retained_source"] == "source":
        receipt["variant"] = receipt.pop("group")
        receipt["skipped_source"] = receipt.pop("skipped_retained")
        receipt["skipped_make"] = []
        receipt["make_manifest_sha256"] = None
        receipt["source_receipt_sha256"] = source_digest
        receipt["source_packages"] = [{key: item[key] for key in
            ("package", "version", "architecture", "source", "input_tar_sha256")}
            for item in source["packages"]]
        receipt["provided_package_replacements"] = []
    else:
        receipt.update(image=policy["image"], policy_sha256=hashlib.sha256(policy_path.read_bytes()).hexdigest())
    return selected, receipt


def main():
    """The shared apt_layer action entry point; every input is declared by Bazel."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", required=True, type=Path)
    parser.add_argument("--lock", required=True, type=Path)
    parser.add_argument("--policy", required=True, type=Path)
    parser.add_argument("--retained-manifest", "--make-manifest", dest="retained_manifest", type=Path)
    parser.add_argument("--mapping", required=True, type=Path)
    parser.add_argument("--base-package-metadata", type=Path)
    parser.add_argument("--variant", required=True, choices=("runtime", "debug"))
    parser.add_argument("--base-inventory", type=Path)
    parser.add_argument("--exclude", action="append", default=[], type=Path)
    parser.add_argument("--include", action="append", default=[])
    output = parser.add_mutually_exclusive_group(required=True)
    output.add_argument("--out-dir", type=Path)
    output.add_argument("--out-tar", type=Path)
    parser.add_argument("--receipt", required=True, type=Path)
    args = parser.parse_args()
    try:
        selected, receipt = select(
            args.base, args.lock, args.policy, args.mapping, variant=args.variant,
            retained_manifest=args.retained_manifest, base_package_metadata=args.base_package_metadata,
            base_inventory=args.base_inventory)
        if args.out_tar is not None:
            assemble(selected, args.out_tar, **read_patterns(args.exclude, args.include))
        else:
            selection.stage_payloads(selected, args.out_dir)
        args.receipt.parent.mkdir(parents=True, exist_ok=True)
        args.receipt.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError, tarfile.TarError) as error:
        parser.exit(1, "APT selection failed: " + str(error) + "\n")


if __name__ == "__main__":
    main()
