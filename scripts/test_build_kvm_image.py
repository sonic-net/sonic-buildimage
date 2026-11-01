"""Exercise installer-media sizing without mounting a disk or booting KVM."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class KvmInstallerDiskTest(unittest.TestCase):
    def setUp(self):
        temporary_dir = tempfile.TemporaryDirectory(prefix="kvm-installer-")
        self.addCleanup(temporary_dir.cleanup)
        self.workdir = Path(temporary_dir.name)
        self.installer_disk = self.workdir / "sonic-installer.img"

    def prepare_installer_disk(self, installer_size):
        installer = self.workdir / "sonic-vs.bin"
        with installer.open("wb") as output:
            output.truncate(installer_size)

        source = (ROOT / "scripts/build_kvm_image.sh").read_text()
        first = source.index("prepare_installer_disk()\n")
        last = source.index("\nwait_kvm_ready()", first)
        harness = """
set -e
fallocate() { truncate -s "$2" "$3"; }
mount() { :; }
cp() { :; }
umount() { :; }
"""
        env = dict(os.environ, INSTALLER=str(installer),
                   INSTALLER_DISK=str(self.installer_disk),
                   TMPDIR=str(self.workdir))
        return subprocess.run(
            [os.environ.get("BASH", "bash"), "-c",
             harness + source[first:last] + "\nprepare_installer_disk\n"],
            env=env, text=True, stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        )

    def assert_installer_fits(self, installer_size):
        result = self.prepare_installer_disk(installer_size)
        self.assertEqual(result.returncode, 0, result.stderr)
        header = subprocess.check_output(
            ["dumpe2fs", "-h", str(self.installer_disk)], text=True,
        )
        fields = dict(line.split(":", 1) for line in header.splitlines()
                      if ":" in line)
        free_bytes = int(fields["Free blocks"]) * int(fields["Block size"])
        self.assertGreaterEqual(free_bytes, installer_size)

    def test_reported_debug_installer_fits(self):
        self.assert_installer_fits(5347842031)


if __name__ == "__main__":
    unittest.main()
