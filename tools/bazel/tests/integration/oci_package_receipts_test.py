"""Exercise direct upstream extraction, OCI consumption and local package policy."""

import argparse
import hashlib
import json
from pathlib import Path
import tarfile
import tempfile
import unittest

from tools.bazel.oci import package_receipts
from tools.bazel.tests.oci_contract_fixture import owner_policies


parser = argparse.ArgumentParser()
for name in ("payload", "manifest", "repeat-payload", "repeat-manifest", "inherited-payload", "image"):
    parser.add_argument("--" + name, required=True, type=Path)
parser.add_argument("--package", action="append", type=json.loads, required=True)
args, remaining = parser.parse_known_args()
OWNER_POLICIES, remaining = owner_policies(remaining)


class DirectPackageInputsTest(unittest.TestCase):
    def test_direct_tools_preserve_original_package_identities(self):
        """Keep original DEB and control hashes when the shared importer creates an OCI payload."""
        receipt = json.loads(args.manifest.read_text())
        self.assertEqual([p["package"] for p in receipt["packages"]], ["example-gz", "example-xz"])
        expected = {
            "example-gz_1.0-1_all.deb": "d2225512e6d3052263f96a633c72878c1bf9669ecfb9edda6119017be9706e00",
            "example-xz_1.0-1_all.deb": "9871ca31583ac0db20d3fd18c8d994370b16ae3edf2ebbd39a6ea5c7e5b02c57",
        }
        original_controls = {
            "example-gz": "2ddb8dd20164c5c2b9af866916939dcd8a1f2efb3bad72752a51ef23bdc9b74c",
            "example-xz": "8267883c8cb9fe3cdebf8e95fda2be7d90f428db64edd1499eb6bdee6e8192b2",
        }
        for record in receipt["packages"]:
            self.assertEqual(record["source_sha256"], expected[record["source_deb"]])
            self.assertEqual(record["control_sha256"], original_controls[record["package"]])
            self.assertEqual(record["control_files"]["postinst"], hashlib.sha256(b"#!/bin/sh\nexit 99\n").hexdigest())
            self.assertEqual(record["architecture"], "all")
        self.assertEqual(receipt["payload"]["sha256"], hashlib.sha256(args.payload.read_bytes()).hexdigest())

    def test_order_ownership_links_and_extended_metadata_survive(self):
        """Preserve declared package order, hardlinks, owners, modes and extended TAR metadata."""
        with tarfile.open(args.payload) as archive:
            members = archive.getmembers()
            values = [archive.extractfile(member).read() for member in members if member.isfile()]
            self.assertEqual(values, [b"gz", b"xz"])
            self.assertEqual(archive.extractfile("./usr/share/example/value").read(), b"xz")
            self.assertEqual(archive.extractfile("./usr/share/example/hardlink").read(), b"xz")
            self.assertEqual(len(members), 8)
            for member in members:
                self.assertEqual((member.uid, member.gid), (123, 456))
                self.assertEqual((member.uname, member.gname), ("sonic-test", "sonic-test-group"))
                self.assertEqual(member.mtime, 1700000000)
                self.assertEqual(member.pax_headers["SCHILY.xattr.user.example"], "preserved")
                if member.isfile():
                    self.assertEqual(member.mode, 0o640)

    def test_oci_consumes_the_exact_declared_payload(self):
        """Require the OCI layer descriptor and bytes to match the imported payload exactly."""
        index = json.loads((args.image / "index.json").read_text())
        descriptor = index["manifests"][0]
        manifest_bytes = (args.image / "blobs" / descriptor["digest"].replace(":", "/")).read_bytes()
        self.assertEqual("sha256:" + hashlib.sha256(manifest_bytes).hexdigest(), descriptor["digest"])
        manifest = json.loads(manifest_bytes)
        self.assertEqual(len(manifest["layers"]), 1)
        layer = manifest["layers"][0]
        self.assertEqual(layer["digest"], "sha256:" + hashlib.sha256(args.payload.read_bytes()).hexdigest())
        self.assertEqual((args.image / "blobs" / layer["digest"].replace(":", "/")).read_bytes(), args.payload.read_bytes())

    def test_repeat_actions_are_byte_identical(self):
        """Repeated extraction of identical declared packages must produce identical payloads and receipts."""
        self.assertEqual(args.payload.read_bytes(), args.repeat_payload.read_bytes())
        self.assertEqual(args.manifest.read_bytes(), args.repeat_manifest.read_bytes())

    def test_debug_receipt_reuses_the_runtime_payload(self):
        """Keep two variant receipts bound to one TAR through a declared runfiles link."""
        runtime = json.loads(args.manifest.read_text())
        debug = package_receipts.receipt(
            args.package[:2], {"variant": "debug"}, args.inherited_payload,
            args.manifest, architecture="amd64")
        self.assertNotEqual(args.payload.parent, args.inherited_payload.parent)
        self.assertEqual(args.inherited_payload.name, "payload.tar")
        self.assertEqual(args.payload.read_bytes(), args.inherited_payload.read_bytes())
        self.assertEqual(debug["variant"], "debug")
        self.assertEqual(debug["payload"], runtime["payload"])
        self.assertEqual(debug["packages"], runtime["packages"])
        self.assertEqual(debug["runtime_manifest_sha256"],
                         hashlib.sha256(args.manifest.read_bytes()).hexdigest())

    def test_rejects_foreign_and_duplicate_packages(self):
        """Reject a foreign package architecture or repeated package identity before publication."""
        with self.assertRaisesRegex(ValueError, "foreign Debian architecture"):
            package_receipts.receipt([args.package[2]], {}, args.payload, architecture="amd64")
        with self.assertRaisesRegex(ValueError, "duplicate Debian package"):
            package_receipts.receipt([args.package[0], args.package[0]], {}, args.payload, architecture="amd64")

    def test_debug_receipt_cannot_replace_a_runtime_package(self):
        """Debug provenance must not silently change a package already recorded in runtime."""
        with tempfile.TemporaryDirectory() as directory:
            parent = Path(directory) / "runtime.json"
            runtime = json.loads(args.manifest.read_text())
            runtime["packages"][0]["source_sha256"] = "0" * 64
            parent.write_text(json.dumps(runtime))
            with self.assertRaisesRegex(ValueError, "changes a runtime package"):
                package_receipts.receipt(args.package[:2], {}, args.payload, parent, architecture="amd64")

    def test_required_packages_and_verified_fields_fail_closed(self):
        """Missing required packages and overrides of verified facts cannot produce a valid receipt."""
        with self.assertRaisesRegex(ValueError, "missing required package"):
            package_receipts.receipt(args.package[:2], {"required_packages": ["absent"]}, args.payload, architecture="amd64")
        with self.assertRaisesRegex(ValueError, "overrides package facts"):
            package_receipts.receipt(args.package[:2], {"packages": []}, args.payload, architecture="amd64")


if __name__ == "__main__":
    unittest.main(argv=[__file__, *remaining])
