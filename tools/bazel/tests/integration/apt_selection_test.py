#!/usr/bin/env python3
"""Exercise shared APT selection with local OCI/control TARs and declarative policies.

The fixtures contain no Debian package archives and require no container build.
Container suites additionally exercise their actual BUILD policies and FIPS inputs.
"""

import contextlib
import hashlib
import io
import json
from pathlib import Path
import sys
import tarfile
import tempfile
import unittest
from unittest import mock

from tools.bazel.oci import apt_selection as subject
from tools.bazel.tests.oci_base_fixture import digest, layer_tar, oci_files, write_layout


class Fixture:
    """Supply a layered base, retained package and locked addition for any owner."""

    def __init__(self, root, image="telemetry", architecture="amd64"):
        self.root = root / image
        self.root.mkdir()
        self.architecture = architecture
        self.base = self.root / "base.oci"
        status = ("Package: platform-lib\nVersion: 3.0\nArchitecture: " + architecture +
                  "\nStatus: install ok installed\n\n").encode()
        self.layers = [
            layer_tar({"var/lib/dpkg/status": status, "etc/shared.conf": b"obsolete",
                       "etc/removed.conf": b"removed by next layer"}),
            layer_tar({"etc/shared.conf": b"current", "etc/.wh.removed.conf": b""}),
        ]
        self.write_base()
        self.retained = {"local-driver": {"source_sha256": "a" * 64, "control": {
            "Package": "local-driver", "Version": "2.0", "Architecture": architecture}}}
        self.addition = image + "-addon"
        self.paths, packages, mapping, roots = {}, {}, [], {}
        for name in ("platform-lib", "local-driver", self.addition):
            path = self.root / (name + ".tar")
            path.write_bytes(layer_tar({"etc/shared.conf": b"current",
                                        "etc/removed.conf": b"restored by addition"}))
            fields = {"Package": name, "Version": "1.0", "Architecture": architecture}
            if name == self.addition:
                fields["Depends"] = "platform-lib (>= 3.0), local-driver (>= 2.0)"
            control = self.root / (name + ".control.tar")
            control.write_bytes(layer_tar({"control": "".join(
                key + ": " + value + "\n" for key, value in fields.items()).encode()}))
            key = "/trixie/" + name + ":" + architecture + "=1.0"
            packages[key] = {
                "name": name, "version": "1.0", "architecture": architecture,
                "sha256": hashlib.sha256(name.encode()).hexdigest(), "depends_on": [],
                "payload_sha256": digest(path.read_bytes()).removeprefix("sha256:"),
                "payload_size": path.stat().st_size,
                "control_sha256": digest(control.read_bytes()).removeprefix("sha256:"),
                "control_size": control.stat().st_size,
            }
            roots[key.rsplit("=", 1)[0]] = "1.0"
            mapping.append({"key": key, "payload": str(path), "control": str(control)})
            self.paths[name] = path
        self.lock = self.root / "lock.json"
        self.lock.write_text(json.dumps({"version": 2, "packages": packages,
            "dependency_sets": {group: {"sets": {architecture: roots}}
                                for group in ("runtime", "debug")}}))
        self.mapping = self.root / "mapping.json"
        self.mapping.write_text(json.dumps({"architecture": architecture, "locked": mapping}))
        self.policy = self.root / "policy.json"
        self.policy.write_text(json.dumps({"schema": 1, "image": image,
            "architecture": architecture, "distribution": "trixie", "retained_source": "make",
            "features": {}}))
        self.manifest = self.root / "manifest.json"
        self.manifest.write_text(json.dumps({"schema": 1, "image": image,
            "architecture": architecture, "distribution": "trixie", "variant": "runtime",
            "features": {}, "packages": [{"package": "local-driver", "version": "2.0",
                "architecture": architecture, "source_sha256": "a" * 64,
                "control_fields": self.retained["local-driver"]["control"]}]}))

    def write_base(self):
        config = {"architecture": self.architecture, "os": "linux", "rootfs": {
            "type": "layers", "diff_ids": [digest(layer) for layer in self.layers]}}
        files = oci_files(json.dumps(config).encode(), self.layers)
        self.manifest_digest = json.loads(files["index.json"])["manifests"][0]["digest"]
        write_layout(self.base, files)

    def select(self, **options):
        return subject.select(self.base, self.lock, self.policy, self.mapping,
                              variant=options.pop("variant", "runtime"),
                              retained_manifest=self.manifest, **options)

    def replace_payload(self, entries):
        path = self.paths[self.addition]
        path.write_bytes(layer_tar(entries))
        lock = json.loads(self.lock.read_bytes())
        package = next(item for item in lock["packages"].values() if item["name"] == self.addition)
        package.update(payload_sha256=digest(path.read_bytes()).removeprefix("sha256:"),
                       payload_size=path.stat().st_size)
        self.lock.write_text(json.dumps(lock))


class AptSelectionTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="shared-apt-selection-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)

    def source_manifest(self, fixture):
        """Move the dependency to a Bazel target while retaining one Make import."""
        manifest = json.loads(fixture.manifest.read_bytes())
        source = manifest["packages"].pop()
        source.pop("source_sha256")
        source.update(input_tar_sha256="b" * 64, source={
            "module": "driver", "version": "1.0-" + "d" * 40,
            "commit": "d" * 40, "target": "@driver//:runtime_pkg"})
        manifest["packages"] = [{"package": "make-import", "version": "1", "architecture": fixture.architecture,
            "source_sha256": "a" * 64, "control_fields": {
                "Package": "make-import", "Version": "1", "Architecture": fixture.architecture,
                "Depends": "local-driver (>= 2.0)"}}]
        manifest["make_manifest_sha256"] = hashlib.sha256(json.dumps(manifest).encode()).hexdigest()
        manifest.update(source_packages=[source], source_receipt_sha256="c" * 64)
        fixture.manifest.write_text(json.dumps(manifest))
        return manifest

    def source_receipt(self, fixture):
        """A source-only receipt supplies the driver without a synthetic Make import."""
        policy = json.loads(fixture.policy.read_bytes())
        policy["retained_source"] = "source"
        fixture.policy.write_text(json.dumps(policy))
        control = {key: "" for key in ("Depends", "Pre-Depends", "Provides", "Multi-Arch")}
        control.update(fixture.retained["local-driver"]["control"])
        control["Multi-Arch"] = "no"
        receipt = {"schema": 2, "kind": "bazel_source", "image": policy["image"],
            "architecture": fixture.architecture, "distribution": "trixie", "contract_sha256": "b" * 64,
            "base_manifest_digest": fixture.manifest_digest, "module_file_sha256": {"driver": "c" * 64},
            "packages": [{"package": "local-driver", "version": "2.0", "architecture": fixture.architecture,
                "control_fields": control, "input_tar_sha256": "a" * 64,
                "source": {"module": "driver", "version": "1.0-" + "d" * 40,
                           "commit": "d" * 40, "target": "@driver//:runtime"},
                "files": {"usr/lib/driver.so": {"kind": "file"}}}]}
        fixture.manifest.write_text(json.dumps(receipt))
        return receipt

    def test_source_receipt_satisfies_dependencies_without_imported_packages(self):
        fixture = Fixture(self.root)
        self.source_receipt(fixture)
        paths, receipt = fixture.select()
        self.assertEqual(paths, [fixture.paths[fixture.addition]])
        self.assertEqual(receipt["skipped_make"], [])
        self.assertIsNone(receipt["make_manifest_sha256"])
        self.assertEqual(receipt["skipped_source"][0]["package"], "local-driver")
        self.assertEqual(receipt["source_receipt_sha256"], hashlib.sha256(fixture.manifest.read_bytes()).hexdigest())
        self.assertEqual(receipt["dependency_check"]["status"], "satisfied")
        self.assertEqual(receipt["source_packages"][0]["source"]["target"], "@driver//:runtime")

    def test_source_receipt_is_bound_to_base_architecture_and_runtime_metadata(self):
        fixture = Fixture(self.root)
        original = self.source_receipt(fixture)
        for key, value in (("base_manifest_digest", "sha256:" + "0" * 64), ("architecture", "arm64")):
            with self.subTest(key=key):
                fixture.manifest.write_text(json.dumps({**original, key: value}))
                with self.assertRaises(ValueError):
                    fixture.select()
        fixture.manifest.write_text(json.dumps(original))
        _, runtime = fixture.select()
        metadata = fixture.root / "runtime-receipt.json"
        for value in ({**runtime, "source_receipt_sha256": "e" * 64}, {**runtime, "variant": "debug"}):
            metadata.write_text(json.dumps(value))
            with self.assertRaisesRegex(ValueError, "matching runtime source metadata"):
                fixture.select(variant="debug", base_package_metadata=metadata)

    def test_source_controls_satisfy_imports_without_claiming_make_payloads(self):
        """A source target satisfies a Make dependency and keeps its own TAR provenance."""
        fixture = Fixture(self.root)
        manifest = self.source_manifest(fixture)
        paths, receipt = fixture.select()
        self.assertEqual(paths, [fixture.paths[fixture.addition]])
        self.assertEqual(receipt["skipped_make"], [])
        self.assertEqual(receipt["skipped_source"][0]["package"], "local-driver")
        self.assertEqual(receipt["skipped_source"][0]["source_sha256"], "b" * 64)
        self.assertEqual(receipt["make_manifest_sha256"], manifest["make_manifest_sha256"])
        self.assertEqual(receipt["source_receipt_sha256"], "c" * 64)
        self.assertEqual(receipt["retained_manifest_sha256"], hashlib.sha256(fixture.manifest.read_bytes()).hexdigest())
        self.assertEqual(receipt["dependency_check"]["packages"]["local-driver"]["Version"], "2.0")

    def test_source_inventory_rejects_missing_provenance_and_duplicate_owners(self):
        """A package cannot be supplied by both Make and source or lack a built TAR identity."""
        fixture = Fixture(self.root)
        original = self.source_manifest(fixture)
        for mutation in ("hash", "duplicate", "commit", "receipt"):
            manifest = json.loads(json.dumps(original))
            source = manifest["source_packages"][0]
            if mutation == "hash":
                source.pop("input_tar_sha256")
            elif mutation == "duplicate":
                manifest["packages"].append(dict(source, source_sha256="a" * 64))
            elif mutation == "commit":
                source["source"]["commit"] = "unreviewed"
            else:
                manifest.pop("source_receipt_sha256")
            fixture.manifest.write_text(json.dumps(manifest))
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                fixture.select()

    def test_source_packages_still_must_satisfy_versioned_dependencies(self):
        """Source ownership cannot waive a Make consumer's minimum dependency version."""
        fixture = Fixture(self.root)
        manifest = self.source_manifest(fixture)
        source = manifest["source_packages"][0]
        source["version"] = source["control_fields"]["Version"] = "1.0"
        fixture.manifest.write_text(json.dumps(manifest))
        with self.assertRaises(ValueError):
            fixture.select()

    def test_debug_source_inputs_must_match_the_runtime_receipt(self):
        """Debug cannot silently reuse package metadata from a different source build."""
        fixture = Fixture(self.root)
        manifest = self.source_manifest(fixture)
        _, runtime = fixture.select()
        metadata = fixture.root / "runtime-receipt.json"
        metadata.write_text(json.dumps(runtime))
        manifest.update(variant="debug", runtime_manifest_sha256=runtime["make_manifest_sha256"],
                        source_receipt_sha256="e" * 64)
        fixture.manifest.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, "runtime source packages"):
            fixture.select(variant="debug", base_package_metadata=metadata)

    def test_owner_state_and_layer_order_are_used_for_both_architectures(self):
        """Use actual retained versions and final OCI contents without an image or AMD64 default."""
        for image, architecture in (("telemetry", "amd64"), ("time-service", "arm64")):
            with self.subTest(image=image, architecture=architecture):
                fixture = Fixture(self.root, image, architecture)
                paths, receipt = fixture.select()
                self.assertEqual(paths, [fixture.paths[fixture.addition]])
                self.assertEqual(receipt["base_manifest_digest"], fixture.manifest_digest)
                self.assertEqual(receipt["skipped_base"][0]["base_version"], "3.0")
                self.assertEqual(receipt["skipped_make"][0]["source_sha256"], "a" * 64)
                self.assertEqual(receipt["changed_non_elf_base_paths"], [])
                self.assertEqual(receipt["dependency_check"]["status"], "satisfied")
                records = receipt["dependency_check"]["packages"]
                self.assertEqual(set(records), {"platform-lib", "local-driver", fixture.addition})
                self.assertEqual(records["local-driver"]["Version"], "2.0")
                self.assertTrue(all(record["Architecture"] == architecture for record in records.values()))

    def test_platform_mismatch_fails_before_package_selection(self):
        """Reject a base for another architecture before handing its state to infrastructure."""
        fixture = Fixture(self.root)
        policy = json.loads(fixture.policy.read_bytes())
        policy["architecture"] = "arm64"
        fixture.policy.write_text(json.dumps(policy))
        manifest = json.loads(fixture.manifest.read_bytes())
        manifest["architecture"] = "arm64"
        fixture.manifest.write_text(json.dumps(manifest))
        with mock.patch.object(subject.selection, "select") as select:
            with self.assertRaisesRegex(ValueError, "platform"):
                fixture.select()
        select.assert_not_called()

    def test_added_payload_uses_checked_inventory(self):
        """Allow base whiteouts but reject a reviewed package TAR that would delete inherited files."""
        fixture = Fixture(self.root)
        fixture.replace_payload({"etc/.wh.shared.conf": b""})
        with self.assertRaisesRegex(ValueError, "added OCI layer contains a whiteout"):
            fixture.select()

    def test_policy_rejects_incomplete_or_ambiguous_authorization(self):
        """An omitted selector mode or obsolete replacement option cannot relax validation."""
        fixture = Fixture(self.root)
        original = json.loads(fixture.policy.read_bytes())
        invalid = [dict(original, schema=2), dict(original, retained_source="automatic"),
                   dict(original, unknown_option=True),
                   dict(original, debug_replacements=[]),
                   dict(original, debug_replacements=[{"package": "driver", "version_contains": "fips"}]),
                   dict(original, retained_source="none", features={"fips": "y"}),
                   {key: value for key, value in original.items() if key != "retained_source"}]
        for policy in invalid:
            with self.subTest(policy=policy):
                fixture.policy.write_text(json.dumps(policy))
                with self.assertRaises(ValueError):
                    fixture.select()

    def test_manifest_cannot_be_injected_or_omitted(self):
        """BUILD policy explicitly decides whether Make controls are part of selection."""
        fixture = Fixture(self.root)
        with self.assertRaisesRegex(ValueError, "requires a retained manifest"):
            subject.select(fixture.base, fixture.lock, fixture.policy, fixture.mapping, variant="runtime")
        policy = json.loads(fixture.policy.read_bytes())
        policy["retained_source"] = "none"
        fixture.policy.write_text(json.dumps(policy))
        with self.assertRaisesRegex(ValueError, "does not permit a retained manifest"):
            fixture.select()

    def cli_arguments(self, fixture, *, manifest_flag="--retained-manifest", variant="runtime",
                      metadata=None):
        output, receipt = fixture.root / "selected", fixture.root / "receipts" / "selection.json"
        arguments = ["selector", "--base", str(fixture.base), "--lock", str(fixture.lock),
                     "--policy", str(fixture.policy), manifest_flag, str(fixture.manifest),
                     "--mapping", str(fixture.mapping),
                     "--variant", variant, "--out-dir", str(output), "--receipt", str(receipt)]
        if metadata is not None:
            arguments += ["--base-package-metadata", str(metadata)]
        return arguments, output, receipt

    def test_cli_stages_policy_selected_payloads_and_inherited_metadata(self):
        """Both manifest spellings use the shared executable path and exact runtime provenance."""
        cases = (("telemetry", "amd64", "runtime", "--retained-manifest"),
                 ("time-service", "arm64", "debug", "--make-manifest"))
        for image, architecture, variant, flag in cases:
            with self.subTest(image=image, variant=variant):
                fixture = Fixture(self.root, image, architecture)
                metadata = None
                if variant == "debug":
                    _, inherited = fixture.select()
                    metadata = fixture.root / "runtime-receipt.json"
                    metadata.write_text(json.dumps(inherited))
                    fixture.layers.append(fixture.paths[fixture.addition].read_bytes())
                    fixture.write_base()
                    manifest = json.loads(fixture.manifest.read_bytes())
                    manifest.update(variant="debug", runtime_manifest_sha256=inherited["make_manifest_sha256"])
                    fixture.manifest.write_text(json.dumps(manifest))
                arguments, output, receipt = self.cli_arguments(
                    fixture, manifest_flag=flag, variant=variant, metadata=metadata)
                with mock.patch.object(sys, "argv", arguments):
                    subject.main()
                document = json.loads(receipt.read_bytes())
                self.assertEqual(document["variant"], variant)
                self.assertEqual(document["base_manifest_digest"], fixture.manifest_digest)
                self.assertEqual(document["dependency_check"]["package_count"], 3)
                self.assertEqual(document["make_manifest_sha256"], hashlib.sha256(fixture.manifest.read_bytes()).hexdigest())
                self.assertEqual(document["provided_package_replacements"], [])
                expected = [] if variant == "debug" else ["000001.tar"]
                self.assertEqual(sorted(path.name for path in output.iterdir()),
                                 ["000000-empty.tar", *expected])
                with tarfile.open(output / "000000-empty.tar") as archive:
                    self.assertEqual(archive.getnames(), [])
                if expected:
                    self.assertEqual((output / expected[0]).read_bytes(),
                                     fixture.paths[fixture.addition].read_bytes())

    def test_cli_policy_failure_does_not_publish_outputs(self):
        """Keep an owner's rejected policy from creating a success receipt or staged archive."""
        fixture = Fixture(self.root)
        arguments, output, receipt = self.cli_arguments(fixture)
        fixture.policy.write_text('{"schema": 2}')
        diagnostic = io.StringIO()
        with mock.patch.object(sys, "argv", arguments), contextlib.redirect_stderr(diagnostic):
            with self.assertRaises(SystemExit) as failure:
                subject.main()
        self.assertEqual(failure.exception.code, 1)
        self.assertEqual(diagnostic.getvalue(), "APT selection failed: invalid APT policy fields\n")
        self.assertFalse(output.exists())
        self.assertFalse(receipt.exists())

    def test_cli_staging_failure_preserves_the_previous_receipt(self):
        """Do not replace prior success evidence when the declared staging directory is occupied."""
        fixture = Fixture(self.root)
        arguments, output, receipt = self.cli_arguments(fixture)
        output.mkdir()
        sentinel = output / "existing.tar"
        sentinel.write_bytes(b"previous output")
        receipt.parent.mkdir()
        receipt.write_bytes(b"previous receipt\n")
        diagnostic = io.StringIO()
        with mock.patch.object(sys, "argv", arguments), contextlib.redirect_stderr(diagnostic):
            with self.assertRaises(SystemExit) as failure:
                subject.main()
        self.assertEqual(failure.exception.code, 1)
        self.assertIn("APT selection failed: selected archive directory must be empty", diagnostic.getvalue())
        self.assertEqual(receipt.read_bytes(), b"previous receipt\n")
        self.assertEqual(sentinel.read_bytes(), b"previous output")
        self.assertEqual(list(output.iterdir()), [sentinel])


if __name__ == "__main__":
    unittest.main()
