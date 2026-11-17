"""Check real ONIE TAR/ZIP/SquashFS metadata without executing the installer."""

import io
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest
import zipfile

import yaml


ROOT = Path(__file__).resolve().parents[3]
METADATA = {
    "build_version": "20261101.1",
    "asic_type": "vs",
    "kernel_version": "6.12.41+deb13-sonic-amd64",
    "commit_id": "123456789",
    "secure_boot_image": "no",
}


class VSInstallerMetadataTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="vs-installer-metadata-test-")
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        self.installer = self.directory / "sonic-vs.bin"
        self.receipt = self.directory / "metadata.json"

    def package(self, metadata, *, tar_name="installer/fs.zip", zip_name="fs.squashfs"):
        rootfs = self.directory / "rootfs"
        (rootfs / "etc/sonic").mkdir(parents=True)
        (rootfs / "etc/sonic/sonic_version.yml").write_text(metadata)
        squashfs = self.directory / "fs.squashfs"
        subprocess.run(
            ["mksquashfs", str(rootfs), str(squashfs), "-comp", "zstd",
             "-processors", "1", "-noappend", "-no-progress"],
            check=True, capture_output=True, timeout=30,
        )
        payload = io.BytesIO()
        with zipfile.ZipFile(payload, "w") as archive:
            archive.write(squashfs, zip_name)
        outer = io.BytesIO()
        with tarfile.open(fileobj=outer, mode="w") as archive:
            member = tarfile.TarInfo(tar_name)
            member.size = len(payload.getvalue())
            payload.seek(0)
            archive.addfile(member, payload)
        # The production shell header would fail (unexpanded placeholders) if
        # the validator accidentally executed it instead of reading the TAR.
        self.installer.write_bytes(
            (ROOT / "installer/sharch_body.sh").read_bytes() + outer.getvalue()
        )

    def validate(self, commit_id="123456789"):
        return subprocess.run(
            [sys.executable, "-B", str(ROOT / "scripts/validate_sonic_version.py"),
             str(self.installer), "--installer", "--secure-upgrade-mode", "no_sign",
             "--expected-commit-id", commit_id, "--json-output", str(self.receipt)],
            capture_output=True, text=True, timeout=30,
        )

    def test_reads_actual_packaged_metadata_and_ignores_trailing_signature_bytes(self):
        self.package(yaml.safe_dump(METADATA))
        with self.installer.open("ab") as stream:
            stream.write(b"fixture-signature-trailer")
        result = self.validate()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(json.loads(self.receipt.read_text()), METADATA)

    def test_rejects_metadata_from_another_build(self):
        self.package(yaml.safe_dump(METADATA))
        result = self.validate("987654321")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("commit_id does not match", result.stderr)
        self.assertFalse(self.receipt.exists())

    def test_rejects_empty_metadata_inside_installer(self):
        self.package("")
        result = self.validate()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("nonempty YAML mapping", result.stderr)
        self.assertFalse(self.receipt.exists())

    def test_rejects_missing_filesystem(self):
        self.package(yaml.safe_dump(METADATA), zip_name="other.squashfs")
        result = self.validate()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("fs.squashfs", result.stderr)
        self.assertFalse(self.receipt.exists())

    def test_rejects_invalid_installer_without_running_shell_commands(self):
        sentinel = self.directory / "executed"
        self.installer.write_text(f"#!/bin/sh\ntouch {sentinel}\n")
        result = self.validate()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("no exit_marker", result.stderr)
        self.assertFalse(sentinel.exists())
        self.assertFalse(self.receipt.exists())


if __name__ == "__main__":
    unittest.main()
