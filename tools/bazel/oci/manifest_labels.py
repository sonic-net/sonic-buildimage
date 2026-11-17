#!/usr/bin/env python3
"""Convert a Make-generated SONiC manifest into an OCI image label."""

import argparse
import json
from pathlib import Path


def reject_constant(value):
    raise ValueError(f"Container manifest contains a non-JSON number: {value}")


def manifest_label(source, image_version=None):
    """Preserve the JSON object as one line of OCI label-file content."""
    manifest = json.loads(source, parse_constant=reject_constant)
    if not isinstance(manifest, dict):
        raise ValueError("Container manifest must be a JSON object")
    label = "com.azure.sonic.manifest=" + json.dumps(manifest, separators=(",", ":"), allow_nan=False) + "\n"
    if image_version is not None:
        if "\n" in image_version or "\r" in image_version:
            raise ValueError("Image version must be one line")
        label += "Tag=" + image_version + "\n"
    return label


def component_labels(receipt, names):
    """Label Make's named components with the versions installed in the OCI image."""
    packages = json.loads(receipt)["dependency_check"]["packages"]
    lines = []
    for name in sorted(names):
        package = packages.get(name, {})
        version = package.get("Version")
        if package.get("Package") != name or not version or "\r" in version or "\n" in version:
            raise ValueError(f"Missing or invalid installed package version for {name}")
        lines.append(f"com.azure.sonic.versions.{name}={version}\n")
    return "".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--component-labels", type=Path)
    parser.add_argument("--package-metadata", type=Path)
    parser.add_argument("--version-package", action="append", default=[])
    parser.add_argument("--image-version", help="SONiC image version for the Make Tag label")
    args = parser.parse_args()
    # Read and validate before opening the output, including when it exists.
    label = manifest_label(args.manifest.read_text(encoding="utf-8"), args.image_version)
    if bool(args.package_metadata) != bool(args.version_package):
        parser.error("a package receipt and at least one version package must be supplied together")
    if args.package_metadata:
        label += component_labels(args.package_metadata.read_text(encoding="utf-8"), args.version_package)
    if args.component_labels:
        label += args.component_labels.read_text(encoding="utf-8")
    args.output.write_text(label, encoding="utf-8")


if __name__ == "__main__":
    main()
