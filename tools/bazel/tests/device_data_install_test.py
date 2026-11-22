"""Exercise the device-data package contract in an offline dpkg database."""

import io
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
from tempfile import TemporaryDirectory
import unittest

ROOT = Path(__file__).resolve().parents[3]
NAME = "sonic-device-data"
PLATFORM = "usr/share/sonic/device/x86_64-kvm_x86_64-r0/platform_asic"


class DeviceDataInstallTest(unittest.TestCase):
    def setUp(self):
        work = self.enterContext(TemporaryDirectory())
        self.root = Path(work) / "root"
        self.archive = Path(work) / "device.tar"
        self.db = self.root / "var/lib/dpkg"
        (self.db / "info").mkdir(parents=True)
        (self.db / "updates").mkdir()
        (self.db / "info/existing.list").touch()
        self.sentinel = ("Package: existing\nStatus: install ok installed\nVersion: 1\nArchitecture: all\n"
                         "Maintainer: Test <test@example.test>\nDescription: Existing package\n\n")
        (self.db / "status").write_text(self.sentinel)
        self.files = {PLATFORM: b"vs\n", f"usr/share/doc/{NAME}/copyright": b"",
                      f"usr/share/doc/{NAME}/changelog.Debian.gz": b"changelog"}

    def pack(self):
        with tarfile.open(self.archive, "w") as archive:
            for name, contents in self.files.items():
                member = tarfile.TarInfo(name)
                member.mode = 0o644
                member.size = len(contents)
                archive.addfile(member, io.BytesIO(contents))

    def install(self, success=True):
        cmd = ["python3", "-B", "scripts/install_device_data.py",
               "--root", str(self.root), "--tar", str(self.archive)]
        if os.geteuid() or os.getegid():
            if not shutil.which("fakeroot"):
                self.skipTest("fakeroot is required for offline installation")
            cmd = ["fakeroot", "--", *cmd]
        result = subprocess.run(cmd, cwd=ROOT, text=True, capture_output=True)
        self.assertEqual(result.returncode == 0, success, result.stderr)
        return result

    def query(self, *args):
        return subprocess.check_output(["dpkg-query", "--admindir=" + str(self.db), *args], text=True)

    def test_installed_files_version_ownership_checksums_and_retry(self):
        obsolete = "usr/share/sonic/device/retired"
        self.files[obsolete] = b"old"
        self.pack()
        self.install()
        del self.files[obsolete]
        self.pack()
        self.install()
        self.install()
        self.assertFalse((self.root / obsolete).exists())
        self.assertEqual(self.query("-W", "-f", "${Status} ${Version} ${Architecture}", NAME),
                         "install ok installed 1.0-1 all")
        self.assertEqual(self.query("-S", "/" + PLATFORM).strip(), NAME + ": /" + PLATFORM)
        self.assertEqual((self.root / PLATFORM).read_bytes(), b"vs\n")
        self.assertEqual((self.root / PLATFORM).stat().st_mode & 0o777, 0o644)
        subprocess.run(["md5sum", "--quiet", "-c", self.db / "info" / (NAME + ".md5sums")],
                       cwd=self.root, check=True)
        self.assertIn(self.sentinel, (self.db / "status").read_text())

    def test_refuses_foreign_package_files_and_unsafe_archive_paths(self):
        target = self.root / PLATFORM
        target.parent.mkdir(parents=True)
        target.write_text("foreign data")
        (self.db / "info/existing.list").write_text("/" + PLATFORM + "\n")
        self.pack()
        self.assertIn("existing already owns", self.install(success=False).stderr)
        self.files["../outside"] = b"invalid"
        self.pack()
        self.assertIn("Unexpected device-data member", self.install(success=False).stderr)
        self.assertEqual(target.read_text(), "foreign data")
        self.assertEqual((self.db / "status").read_text(), self.sentinel)

    def test_only_amd64_bazel_vs_onie_installer_selects_tar(self):
        makefile = """
.SILENT:
.SECONDEXPANSION:
TARGET_PATH=target
IMAGE_DISTRO_FILES_PATH=target/files/trixie
IMAGE_DISTRO_DEBS_PATH=target/debs/trixie
include rules/sonic-device-data.mk
target/sonic-vs.bin target/other.bin: target/%: $$(installer_device_data_inputs)
	@echo $^
"""
        defaults = dict(BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", CONFIGURED_PLATFORM="vs", CONFIGURED_ARCH="amd64",
                        BLDENV="trixie", IMAGE_DISTRO="trixie")
        for goal, changed in (("sonic-vs.bin", {}), ("other.bin", {}), ("sonic-vs.bin", {"CONFIGURED_ARCH": "arm64"}),
                              ("sonic-vs.bin", {"CONFIGURED_PLATFORM": "mellanox"}),
                              ("sonic-vs.bin", {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"}),
                              ("sonic-vs.bin", {"IMAGE_DISTRO": "bookworm"}),
                              ("sonic-vs.bin", {"BLDENV": "bookworm"})):
            with self.subTest(goal=goal, changed=changed):
                settings = defaults | changed
                output = subprocess.check_output(["make", "-s", "-f", "-",
                                                  "-o", "target/files/trixie/sonic-device-data-vs.tar",
                                                  "-o", "target/debs/trixie/sonic-device-data_1.0-1_all.deb",
                                                  "target/" + goal,
                                                  *[f"{k}={v}" for k, v in settings.items()]],
                                                 cwd=ROOT, input=makefile, text=True)
                if goal == "sonic-vs.bin" and not changed:
                    self.assertIn("target/files/trixie/sonic-device-data-vs.tar", output)
                    self.assertNotIn(".deb", output)
                else:
                    self.assertEqual(output.strip(), "target/debs/trixie/sonic-device-data_1.0-1_all.deb")


if __name__ == "__main__":
    unittest.main()
