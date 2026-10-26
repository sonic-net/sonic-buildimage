#!/usr/bin/env python3
"""Validate retained Debian payloads against an explicit OCI image policy."""

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import tarfile

from tools.bazel.ci.artifact_validation import require, sha
from tools.bazel.oci.package_contract import (identity, read_contract, read_policy,
                                             reject_source_packages, validate_policy,
                                             validate_retained_packages)
from tools.bazel.oci.normalize_layer import DIRECTORY_ALIASES, normalized_path, normalized_member, base_aliases, normalize


def validate(manifest_path, payload_path, *, variant, policy, source_contract=None, runtime_manifest=None):
    """Bind source identity, feature policy and inherited runtime to actual TAR bytes."""
    validate_policy(policy)
    expected_identity = identity(policy)
    require(source_contract is not None or not policy.get("native", {}).get("require_source", False),
            "package policy requires its source contract")
    if source_contract is not None:
        require(identity(source_contract) == expected_identity, "source contract and package policy identify different images")
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    require(manifest.get("schema") == 1 and all(manifest.get(key) == value for key, value in expected_identity.items()),
            "unsupported OCI package manifest")
    require(manifest.get("variant") == variant and manifest.get("features") == policy["features"],
            "OCI package manifest has an unsupported configuration")
    records = manifest.get("packages")
    require(isinstance(records, list) and records, "OCI package manifest has no packages")
    require(all(isinstance(record, dict) for record in records), "invalid OCI package record")
    if source_contract is not None:
        reject_source_packages(records, source_contract)
    names = [record.get("package") for record in records]
    require(all(isinstance(name, str) and name for name in names) and len(names) == len(set(names)),
            "OCI package names are missing or duplicated")
    require(set(manifest.get("required_packages", [])).issubset(names), "OCI required package is absent")
    checked = []
    members_expected = 0
    for record in records:
        source = PurePosixPath(record.get("source_deb", ""))
        require(source.name == str(source) and source.name.endswith(".deb") and record.get("version") and
                record.get("architecture") in (policy["architecture"], "all"), "invalid package identity: " + record["package"])
        for kind in ("source", "payload", "control"):
            require(re.fullmatch(r"[0-9a-f]{64}", record.get(kind + "_sha256", "")) is not None,
                    "invalid package " + kind + " integrity: " + record["package"])
        require(isinstance(record.get("source_size"), int) and record["source_size"] > 0 and
                isinstance(record.get("payload_size"), int) and record["payload_size"] > 0 and
                isinstance(record.get("payload_members"), int) and record["payload_members"] > 0,
                "invalid package payload size or member count: " + record["package"])
        members_expected += record["payload_members"]
        checked.append({name: record[name] for name in
                        ("package", "version", "architecture", "source_sha256", "payload_sha256")})
    validate_retained_packages(records, policy, variant)
    payload = manifest.get("payload", {})
    require(payload.get("path") == "payload.tar" and payload_path.resolve() == (manifest_path.parent / "payload.tar").resolve(),
            "declared OCI payload file differs from the manifest")
    require(payload_path.is_file() and payload_path.stat().st_size == payload.get("size") and
            sha(payload_path) == payload.get("sha256"), "changed aggregate package payload")
    members = 0
    with tarfile.open(payload_path, "r:") as archive:
        for member in archive:
            name = PurePosixPath(member.name)
            require(not name.is_absolute() and ".." not in name.parts, "unsafe package payload path: " + member.name)
            require(not name.name.startswith(".wh."), "package payload uses a reserved OCI whiteout path: " + member.name)
            members += 1
    require(members == payload.get("members") == members_expected, "aggregate package member count differs")
    if variant == "debug":
        require(runtime_manifest is not None, "debug payload validation requires the runtime manifest")
        runtime_bytes = runtime_manifest.read_bytes()
        require(hashlib.sha256(runtime_bytes).hexdigest() == manifest.get("runtime_manifest_sha256"),
                "debug package handoff was prepared for a different runtime handoff")
        runtime = json.loads(runtime_bytes)
        require(runtime.get("schema") == 1 and all(runtime.get(key) == value for key, value in expected_identity.items()) and
                runtime.get("variant") == "runtime" and runtime.get("features") == policy["features"],
                "invalid runtime package manifest")
        if source_contract is not None:
            reject_source_packages(runtime["packages"], source_contract)
        validate_retained_packages(runtime["packages"], policy, "runtime")
        runtime_packages = {record["package"]: record for record in runtime["packages"]}
        for record in records:
            previous = runtime_packages.get(record["package"])
            require(previous is None or previous["source_sha256"] == record["source_sha256"],
                    "debug package handoff changes a runtime package: " + record["package"])
        require(set(manifest.get("debug_apt_packages", [])) == set(policy.get("debug_tools", [])),
                "debug package handoff has an unsupported tool set")
    else:
        require(runtime_manifest is None and "runtime_manifest_sha256" not in manifest and
                not manifest.get("debug_apt_packages"), "runtime package handoff contains debug inputs")
    return {"schema": 1, "manifest_sha256": hashlib.sha256(manifest_bytes).hexdigest(),
            "payload_sha256": payload["sha256"], "variant": variant, "package_count": len(records), "packages": checked}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--policy", required=True, type=Path)
    parser.add_argument("--source-contract", type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--payload", required=True, type=Path)
    parser.add_argument("--variant", required=True, choices=("runtime", "debug"))
    parser.add_argument("--runtime-manifest", type=Path)
    parser.add_argument("--base", required=True, type=Path)
    parser.add_argument("--out-tar", required=True, type=Path)
    parser.add_argument("--receipt", type=Path)
    args = parser.parse_args()
    try:
        policy = read_policy(args.policy)
        receipt = validate(args.manifest, args.payload, variant=args.variant, policy=policy,
                           source_contract=read_contract(args.source_contract) if args.source_contract else None,
                           runtime_manifest=args.runtime_manifest)
        receipt["normalization"] = normalize(args.payload, args.base, args.out_tar, expected_platform="linux/" + policy["architecture"])
        require(sha(args.payload) == receipt["payload_sha256"], "aggregate package payload changed during normalization")
        if args.receipt:
            args.receipt.parent.mkdir(parents=True, exist_ok=True)
            args.receipt.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
        else:
            print(json.dumps(receipt, sort_keys=True))
    except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError, tarfile.TarError) as error:
        parser.exit(1, "OCI payload validation failed: " + str(error) + "\n")


if __name__ == "__main__":
    main()
