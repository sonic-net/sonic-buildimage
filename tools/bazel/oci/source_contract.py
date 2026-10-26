"""Build an image receipt contract from producer metadata and runtime TARs."""

import argparse
import hashlib
from pathlib import Path
import re

from sonic_oci.owner_metadata import resolve
from sonic_oci.paths import expand_required_paths
from tools.bazel.oci.package_contract import CONTROL_FIELDS, elf_machine, inventory, json_bytes, read_contract


def generate(owners, modules, sources, public_sources, output, payloads, *, image, architecture, distribution):
    if set(payloads) != set(sources) or set(public_sources) != set(sources):
        raise ValueError("source metadata requires every declared runtime TAR and public target")
    inventories = {name: inventory(payloads[name]) for name in sources}
    observed = {}
    for name, files in inventories.items():
        paths = {}
        for path, item in files.items():
            kind = "elf" if "elf_machine" in item else item["kind"]
            if kind == "elf" and item["elf_machine"] != elf_machine(architecture):
                raise ValueError("source TAR has a foreign ELF architecture: " + name + ":" + path)
            if kind in ("file", "elf", "symlink"):
                paths[path] = kind
        if not paths:
            raise ValueError("source TAR has no runtime files: " + name)
        observed[name] = {"required_paths": paths}
    # Shared resolution needs a path policy. Supply observed TAR paths solely
    # to bind owner identities; observations are not final-image requirements.
    policy = {"schema": 1, "image": image, "architecture": architecture, "distribution": distribution,
              "packages": observed}
    records, identities = resolve(policy, owners, modules, sources, architecture)
    packages = []
    for name, item in records["packages"].items():
        if "payload_sha256" in item:
            with payloads[name].open("rb") as stream:
                digest = hashlib.file_digest(stream, "sha256").hexdigest()
            if digest != item["payload_sha256"]:
                raise ValueError("generated metadata differs from runtime TAR: " + name)
        files = inventories[name]
        paths = {}
        for path in sorted(expand_required_paths(item.get("producer_required_paths", []), architecture)):
            if path not in files:
                raise ValueError("source TAR lacks its producer-required path: " + name + ":" + path)
            actual = files[path]
            kind = "elf" if "elf_machine" in actual else actual["kind"]
            if kind not in ("file", "elf", "symlink"):
                raise ValueError("unsupported producer-required path kind: " + name + ":" + path)
            if kind == "elf" and actual["elf_machine"] != elf_machine(architecture):
                raise ValueError("source TAR has a foreign ELF architecture: " + name + ":" + path)
            paths[path] = kind
        module = item["source_module"]
        version = identities[module]["version"]
        source = {"module": module, "version": version, "target": public_sources[name]}
        commit = re.search(r"-([0-9a-f]{40})$", version)
        if commit:
            source["commit"] = commit[1]
        else:
            source["release"] = version.rsplit(".sonic.", 1)[0]
        # Keep the complete Debian record, including Recommends, Breaks and the
        # description. The dependency subset supplies defaults for existing
        # receipt consumers; it must not filter the owner's generated fields.
        controls = {key: "no" if key == "Multi-Arch" else "" for key in CONTROL_FIELDS}
        controls.update(item["control_fields"])
        packages.append({"required_paths": paths, "package": name,
                         "version": controls["Version"], "architecture": architecture,
                         "control_fields": controls, "source": source})
    result = {**policy, "_generated": "AUTO-GENERATED from source-owner metadata. Required paths are producer guarantees only. DO NOT EDIT.",
              "packages": packages}
    output.write_bytes(json_bytes(result))
    read_contract(output)
    return result


def bindings(values, paths=True):
    result = {}
    for value in values:
        name, path = value.split("=", 1)
        if name in result:
            raise ValueError("duplicate input: " + name)
        result[name] = Path(path) if paths else path
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("image", "architecture", "distribution"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--output", type=Path, required=True)
    for name in ("metadata", "module", "source", "public-source", "payload"):
        parser.add_argument("--" + name, action="append", default=[])
    args = parser.parse_args()
    generate(bindings(args.metadata), bindings(args.module),
             bindings(args.source, False), bindings(args.public_source, False), args.output,
             bindings(args.payload), image=args.image, architecture=args.architecture, distribution=args.distribution)


if __name__ == "__main__":
    main()
