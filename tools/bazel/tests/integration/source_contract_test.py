"""Image contracts follow owner metadata without copied package versions."""

import hashlib
import io
import json
from pathlib import Path
import tempfile
import tarfile
import struct
import unittest

from tools.bazel.oci.source_contract import generate


class SourceContractTest(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        self.architecture = "amd64"
        self.payload = self.root / "runtime.tar"
        self.owner = self.root / "runtime_packages.json"
        self.module = self.root / "MODULE.bazel"
        self.output = self.root / "contract.json"
        self.metadata = {"schema": 1, "module": "example", "packages": {"libexample": {
            "runtime_target": "//:runtime", "required_paths": ["usr/lib/libexample.so.2"], "control_fields": {"Package": "libexample", "Version": "2.4.0-1",
            "Depends": "libc6 (>= 2.38)", "Description": "Example library"}}}}
        self.owner.write_text(json.dumps(self.metadata))
        self.module.write_text('module(name = "example", version = "2.4.0.sonic.1")\n')
        self.write_payload()

    def write_payload(self, paths=("usr/lib/libexample.so.2",), *, machine=62, directory=False):
        contents = bytearray(64)
        contents[:6] = b"\x7fELF\x02\x01"
        struct.pack_into("<HH", contents, 16, 3, machine)
        with tarfile.open(self.payload, "w") as archive:
            for path in paths:
                member = tarfile.TarInfo(path)
                member.mode = 0o755
                member.type = tarfile.DIRTYPE if directory else tarfile.REGTYPE
                member.size = 0 if directory else len(contents)
                archive.addfile(member, None if directory else io.BytesIO(contents))

    def run_contract(self, payloads=None):
        return generate({"@@example+//:runtime_packages.json": self.owner},
                        {"example": self.module}, {"libexample": "@@example+//:runtime"},
                        {"libexample": "@example//:runtime"}, self.output,
                        {"libexample": self.payload} if payloads is None else payloads,
                        image="example", architecture=self.architecture, distribution="trixie")

    def test_owner_changes_update_contract_without_editing_image_policy(self):
        first = self.run_contract()["packages"][0]
        self.metadata["packages"]["libexample"]["control_fields"].update(Version="2.4.0-2", Depends="libc6 (>= 2.40)")
        self.owner.write_text(json.dumps(self.metadata))
        self.module.write_text('module(name = "example", version = "2.4.0.sonic.2")\n')
        second = self.run_contract()["packages"][0]
        self.assertEqual(first["version"], "2.4.0-1")
        self.assertEqual(second["version"], "2.4.0-2")
        self.assertEqual(second["control_fields"]["Depends"], "libc6 (>= 2.40)")
        self.assertEqual(second["source"], {"module": "example", "version": "2.4.0.sonic.2",
                                           "release": "2.4.0", "target": "@example//:runtime"})
        self.assertEqual(second["required_paths"], first["required_paths"])
        self.assertIn("DO NOT EDIT", json.loads(self.output.read_bytes())["_generated"])

    def test_target_architecture_and_commit_come_from_selected_inputs(self):
        self.architecture = "arm64"
        self.write_payload(machine=183)
        revision = "a" * 40
        self.module.write_text('module(name = "example", version = "0.0.1-' + revision + '")\n')
        package = self.run_contract()["packages"][0]
        self.assertEqual(package["architecture"], "arm64")
        self.assertEqual(package["control_fields"]["Architecture"], "arm64")
        self.assertEqual(package["source"]["commit"], revision)

    def test_complete_debian_controls_keep_their_original_fields(self):
        """Preserve optional relationships and multiline text without promoting Recommends to Depends."""
        fields = self.metadata["packages"]["libexample"]["control_fields"]
        fields.pop("Depends")
        fields.update({"Recommends": "libc6 (>= 2.38)", "Breaks": "liboldexample (<< 2.4)",
                       "Replaces": "liboldexample", "Description": "Example library\n Detailed description."})
        self.owner.write_text(json.dumps(self.metadata))
        controls = self.run_contract()["packages"][0]["control_fields"]
        for key, value in fields.items():
            self.assertEqual(controls[key], value)
        self.assertEqual(controls["Depends"], "")
        self.assertEqual(controls["Pre-Depends"], "")
        self.assertEqual(controls["Multi-Arch"], "no")

    def test_generated_controls_are_bound_to_the_actual_runtime_tar(self):
        """Reject stale dependency metadata even when its public TAR label is unchanged."""
        self.metadata["architecture"] = "amd64"
        self.metadata["packages"]["libexample"]["payload_sha256"] = hashlib.sha256(self.payload.read_bytes()).hexdigest()
        self.owner.write_text(json.dumps(self.metadata))
        with self.assertRaisesRegex(ValueError, "requires every declared runtime TAR"):
            self.run_contract({})
        self.run_contract()
        self.payload.write_bytes(self.payload.read_bytes() + b"changed runtime payload")
        with self.assertRaisesRegex(ValueError, "differs from runtime TAR"):
            self.run_contract()

    def test_required_paths_and_kinds_come_from_producer_and_actual_tar(self):
        self.metadata["packages"]["libexample"]["required_paths"] = ["usr/lib/{multiarch}/libexample.so.3"]
        self.owner.write_text(json.dumps(self.metadata))
        with self.assertRaisesRegex(ValueError, "lacks its producer-required path"):
            self.run_contract()
        path = "usr/lib/x86_64-linux-gnu/libexample.so.3"
        self.write_payload([path])
        record = self.run_contract()["packages"][0]
        self.assertEqual(record["required_paths"], {path: "elf"})
        self.assertNotIn("path_modes", record)
        self.assertNotIn("path_owners", record)

    def test_legacy_owner_without_required_paths_does_not_invent_image_requirements(self):
        entry = self.metadata["packages"]["libexample"]
        entry.pop("required_paths")
        self.owner.write_text(json.dumps(self.metadata))
        record = self.run_contract()["packages"][0]
        self.assertEqual(record["required_paths"], {})
        self.assertEqual(record["control_fields"]["Package"], "libexample")

    def test_invalid_producer_path_kind_and_unsafe_tar_path_are_rejected(self):
        self.write_payload(["usr/lib/libexample.so.2"], directory=True)
        # A directory-only TAR is not a runtime package.
        with self.assertRaisesRegex(ValueError, "no runtime files"):
            self.run_contract()
        with tarfile.open(self.payload, "a") as archive:
            data = b"payload"
            member = tarfile.TarInfo("usr/share/example/data")
            member.size = len(data)
            archive.addfile(member, io.BytesIO(data))
        with self.assertRaisesRegex(ValueError, "unsupported producer-required path kind"):
            self.run_contract()
        self.write_payload(["../escape"])
        with self.assertRaises(ValueError):
            self.run_contract()

    def test_foreign_elf_architecture_is_rejected(self):
        self.write_payload(machine=183)
        with self.assertRaisesRegex(ValueError, "foreign ELF architecture"):
            self.run_contract()



if __name__ == "__main__":
    unittest.main()
