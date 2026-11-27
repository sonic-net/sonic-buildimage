"""Exercise Scapy installation in the host build's split-stage control flow."""

import os
from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[1]


class ScapyInstallerTest(unittest.TestCase):
    def run_stage(self, first, last, wheel):
        source = (ROOT / "build_debian.sh").read_text()
        boundary = "## Check if not a last stage of RFS build\nfi\n"
        stage = source.split(boundary, 1)[1].split("## Version file part 2", 1)[0]
        self.assertEqual(source.count('pip3 install "$scapy_wheel_basename"'), 1)
        self.assertIn('pip3 install "$scapy_wheel_basename"', stage)
        harness = """
set -e
sudo() { printf 'SUDO'; printf ' <%s>' "$@"; printf '\\n'; }
pushd() { :; }
popd() { :; }
trap_push() { :; }
FILESYSTEM_ROOT=fsroot
TARGET_PATH=target
RFS_SQUASHFS_NAME=base.squashfs
"""
        env = dict(os.environ, RFS_SPLIT_FIRST_STAGE=first,
                   RFS_SPLIT_LAST_STAGE=last, scapy_wheel=wheel)
        result = subprocess.run(
            [os.environ.get("BASH", "bash"), "-c", harness + stage],
            env=env, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        return result.stdout

    def test_second_stage_installs_after_restore(self):
        output = self.run_stage("n", "y", "target/scapy.whl")
        self.assertLess(output.index("<unsquashfs>"), output.index("<pip3>"))
        self.assertEqual(output.count("<pip3> <install> <scapy.whl>"), 1)
        self.assertIn("<cp> <target/scapy.whl> <fsroot/>", output)
        self.assertIn("<rm> <-f> <fsroot/scapy.whl>", output)

    def test_first_stage_does_not_install(self):
        output = self.run_stage("y", "n", "target/scapy.whl")
        self.assertIn("<mksquashfs>", output)
        self.assertNotIn("<pip3>", output)

    def test_unsplit_build_installs(self):
        output = self.run_stage("n", "n", "target/scapy.whl")
        self.assertNotIn("<unsquashfs>", output)
        self.assertEqual(output.count("<pip3> <install> <scapy.whl>"), 1)

    def test_no_wheel_skips_install(self):
        output = self.run_stage("n", "y", "")
        self.assertIn("<unsquashfs>", output)
        self.assertNotIn("<pip3>", output)


if __name__ == "__main__":
    unittest.main()
