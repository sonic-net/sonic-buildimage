"""Validate actual source-built OCI layers against an explicitly supplied policy.

The adapter copies only declared runfiles into a temporary source tree, then
uses the same inventory, library and symbol checks exercised by the generic
fixture suites. Container BUILD targets choose the artifacts and policy.
"""

import argparse
import json
import os
from pathlib import Path
import shutil
import tempfile
import unittest

from tools.bazel.oci import package_contract
from tools.bazel.ci import artifact_validation


def bindings(values):
    """Keep declared source/library bindings unambiguous and workspace-relative."""
    result = {}
    for value in values:
        name, separator, origin = value.partition("=")
        artifact_validation.require(separator and name and origin and name not in result,
                                    "invalid or duplicate declared input binding: " + value)
        artifact_validation.require(artifact_validation.path_name(name) == name,
                                    "noncanonical declared input path: " + name)
        result[name] = Path(origin)
    return result


def archive_hashes(paths):
    """Bind validated archive bytes to the same unique names exported by CI."""
    paths = list(paths)
    names = [path.name for path in paths]
    artifact_validation.require(len(names) == len(set(names)), "validated archive basenames must be unique")
    return {path.name: artifact_validation.sha(path) for path in paths}


def verify_layers(runtime_path, dependencies_path, configuration_path, symbols_path,
                  libraries, source, policy, *, readelf, objcopy):
    """Compare real archives with source declarations and native debug requirements."""
    archives = [runtime_path, dependencies_path, configuration_path, symbols_path, *libraries.values()]
    validated_hashes = archive_hashes(archives)
    runtime = artifact_validation.payload(runtime_path)
    programs = package_contract.validate_install_inventory(source, runtime, policy["install_inventory"])
    dependencies = artifact_validation.payload(dependencies_path)
    for name, metadata in runtime.items():
        if metadata["kind"] != "directory":
            artifact_validation.require(dependencies.get(name) == metadata,
                                        "dependency layer changed source payload: " + name)
    artifact_validation.require(libraries.keys() == policy["libraries"].keys(),
                                "library inputs differ from the declared package policy")
    for name, library_policy in policy["libraries"].items():
        original = artifact_validation.payload(libraries[name], require_root=False)
        package_contract.validate_library_payload(dependencies, original, library_policy)
    debug = artifact_validation.debug_archives(
        dependencies_path, symbols_path,
        expected_machine=package_contract.elf_machine(policy["architecture"]),
        readelf=readelf, objcopy=objcopy,
    )
    artifact_validation.require(
        set(programs) | set(policy["required_debug_paths"]) <= {pair["path"] for pair in debug["debug_pairs"]},
        "incomplete declared native debug coverage",
    )
    configuration = artifact_validation.payload(configuration_path)
    for path, expected in policy["config_paths"].items():
        artifact_validation.require(
            all(configuration.get(path, {}).get(key) == value for key, value in expected.items()),
            "configuration differs from package policy: " + path,
        )
    artifact_validation.require(archive_hashes(archives) == validated_hashes,
                                "archive bytes changed during validation")
    return {
        "artifact_sha256": validated_hashes,
        "programs": programs,
        **debug,
        "source_contract": {
            name: artifact_validation.sha(source / name)
            for name in (policy["install_inventory"]["declaration_file"], policy["install_inventory"]["install_file"])
        },
        "scope": "Package payload, architecture, build IDs, DWARF and debuglink CRC; no container or boot execution.",
    }


class SourceLayersContractTest(unittest.TestCase):
    def test_actual_source_layers_and_matching_symbols(self):
        """Reject missing programs, changed data/modes, foreign ELF or mismatched symbols in built layers."""
        with tempfile.TemporaryDirectory(prefix="oci-install-contract-") as temporary:
            source = Path(temporary)
            copied = set()

            def copy_declared(relative, origin):
                self.assertNotIn(relative, copied, "duplicate declared install source")
                copied.add(relative)
                destination = source / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(origin, destination)

            for relative, origin in bindings(OPTIONS.source_file).items():
                copy_declared(Path(relative), origin)

            # Absolute runfile paths preserve the owner's source hierarchy.
            # Resolving symlinks here would escape into Bazel's external checkout.
            anchor = Path(os.path.abspath(OPTIONS.source_anchor))
            relative_anchor = artifact_validation.path_name(OPTIONS.source_anchor_relative)
            anchor_parts = Path(relative_anchor).parts
            self.assertEqual(anchor.parts[-len(anchor_parts):], anchor_parts)
            owner_root = anchor
            for _ in anchor_parts:
                owner_root = owner_root.parent
            for name in OPTIONS.source_data:
                origin = Path(os.path.abspath(name))
                copy_declared(origin.relative_to(owner_root), origin)

            validation = verify_layers(
                Path(OPTIONS.runtime), Path(OPTIONS.dependencies), Path(OPTIONS.config),
                Path(OPTIONS.debug_symbols), bindings(OPTIONS.library), source,
                package_contract.read_policy(Path(OPTIONS.policy)),
                readelf=os.path.abspath(OPTIONS.readelf), objcopy=os.path.abspath(OPTIONS.objcopy),
            )

        # The shared CI runner collects this through Bazel's build-event report.
        outputs = Path(os.environ["TEST_UNDECLARED_OUTPUTS_DIR"])
        outputs.mkdir(parents=True, exist_ok=True)
        (outputs / "validation.json").write_text(json.dumps(validation, indent=2) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    for argument in ("runtime", "dependencies", "config", "debug-symbols", "source-anchor",
                     "source-anchor-relative", "readelf", "objcopy", "policy"):
        parser.add_argument("--" + argument, required=True)
    parser.add_argument("--library", action="append", default=[])
    parser.add_argument("--source-file", action="append", default=[])
    parser.add_argument("--source-data", nargs="+", default=[])
    OPTIONS = parser.parse_args()
    unittest.main(argv=[__file__])
