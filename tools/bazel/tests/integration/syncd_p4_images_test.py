"""Validate real Syncd images and their source-built P4 package identities."""

import argparse
import json
import os
from pathlib import Path

from tools.bazel.ci.artifact_validation import require, sha
from tools.bazel.oci import image_validation, native_packages
from tools.bazel.oci.package_contract import read_contract, read_policy, receipt_records
from tools.bazel.oci.source_base_validation import validate_source_base

IMAGE_INPUTS = (
    "runtime_path", "debug_path",
    "runtime_manifest_path", "debug_manifest_path", "source_contract_path",
    "apt_lock_path", "dpkg_filter_path", "base_path", "apt_layer", "debug_tools_layer", "runtime_links_layer",
    "runtime_apt_selection", "debug_apt_selection",
    "runtime_archive", "debug_archive", "source_runtime_tar", "source_debug_tar",
    "source_receipt",
)
NATIVE_INPUTS = (
    "source_contract_path", "source_runtime_tar", "source_debug_tar", "source_receipt",
    "base_path",
)
SOURCE_BASE_INPUTS = ("base_symbols", "base_metadata", "config_metadata", "workspace_module")
P4_RUNTIME_PACKAGES = {"p4lang-pi", "p4lang-bmv2", "p4lang-p4c", "libsai"}


def check_p4_inputs(contract_path, receipt_path):
    contract = read_contract(contract_path)
    receipt = json.loads(receipt_path.read_bytes())
    records = receipt_records(receipt, contract)
    packages = [item for item in records if item["package"] in P4_RUNTIME_PACKAGES]
    require(len(packages) == 4 and {item["package"] for item in packages} == P4_RUNTIME_PACKAGES,
            "source receipt does not contain exactly the four P4 runtime packages")
    require(contract["architecture"] == "amd64" and contract["distribution"] == "trixie",
            "P4 source contract is not the supported AMD64 Trixie tuple")
    for record in packages:
        require(not any(key.startswith("source_deb") for key in record),
                "P4 source receipt contains a DEB import: " + record["package"])
    return {"contract_sha256": sha(contract_path), "receipt_sha256": sha(receipt_path),
            "packages": [{key: item[key] for key in ("package", "version", "architecture", "source", "input_tar_sha256")}
                         for item in packages]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in (*IMAGE_INPUTS, *SOURCE_BASE_INPUTS, "policy", "make_runtime_archive", "make_debug_archive", "readelf", "objcopy"):
        parser.add_argument("--" + name.replace("_", "-"), required=True, type=Path)
    inputs = vars(parser.parse_args())
    policy = read_policy(inputs["policy"])
    p4 = check_p4_inputs(inputs["source_contract_path"], inputs["source_receipt"])
    for variant in ("runtime", "debug"):
        require(sha(inputs["make_" + variant + "_archive"]) == sha(inputs[variant + "_archive"]),
                variant + " Make archive differs from the Bazel output")
    images = image_validation.validate_images(
        source_root=Path.cwd(), policy=policy, runtime_handoff=None, debug_handoff=None,
        **{name: inputs[name] for name in IMAGE_INPUTS})
    native = native_packages.validate_native(
        None, None, policy=policy,
        readelf=str(inputs["readelf"].resolve()), objcopy=str(inputs["objcopy"].resolve()),
        **{name: inputs[name] for name in NATIVE_INPUTS})
    inherited = validate_source_base(
        inputs["runtime_path"], inputs["debug_path"],
        metadata_paths=[inputs["base_metadata"], inputs["config_metadata"]],
        source_debug_tar=inputs["base_symbols"], architecture=policy["architecture"],
        workspace_module=inputs["workspace_module"],
        readelf=str(inputs["readelf"].resolve()), objcopy=str(inputs["objcopy"].resolve()))
    report = {"_generated": {"notice": "AUTO-GENERATED. DO NOT EDIT MANUALLY.",
                             "generator": "tools/bazel/tests/integration/syncd_p4_images_test.py"},
              "schema": 1, "p4": p4, "images": images, "native": native, "source_base": inherited,
              "scope": "Actual AMD64 Syncd runtime/debug images, package inputs and exported archives; boot and forwarding are separate."}
    output = Path(os.environ.get("TEST_UNDECLARED_OUTPUTS_DIR", ".")) / "validation.json"
    output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print("Validated both Syncd images and archives against all four P4 source runtime/debug TAR pairs.")


if __name__ == "__main__":
    main()
