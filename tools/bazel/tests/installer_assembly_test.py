"""Exercise final packing options, rebuild invalidation and best-effort step evidence."""

import json
import os
from pathlib import Path
import runpy
import shutil
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]
timing = runpy.run_path(str(ROOT / "scripts/installer_step_timing.py"))


class InstallerAssemblyTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.work = Path(temporary.name)

    def test_step_evidence_preserves_exit_status_and_excludes_command_arguments(self):
        for status in (0, 17):
            result = timing["run"]("initramfs", [sys.executable, "-c",
                f"import sys; sys.exit({status})", "do-not-record"], self.work)
            self.assertEqual(result, status)
        records = [json.loads(path.read_text()) for path in self.work.glob("*.json")]
        self.assertEqual({record["returncode"] for record in records}, {0, 17})
        for record in records:
            self.assertEqual(set(record), {"step", "seconds", "returncode"})
            self.assertGreaterEqual(record["seconds"], 0)
        blocked = self.work / "not-a-directory"
        blocked.write_bytes(b"blocked")
        self.assertEqual(timing["run"]("initramfs", [sys.executable, "-c", "pass"], blocked), 0)

    @unittest.skipUnless(shutil.which("mksquashfs") and shutil.which("unsquashfs"), "SquashFS tools required")
    def test_final_squashfs_profiles_preserve_contents_and_exclusions(self):
        root = self.work / "root"
        root.mkdir()
        (root / "payload").write_bytes(b"installer data\n" * 4096)
        for name in ("boot", "var/lib/docker", "platform"):
            (root / name).mkdir(parents=True)
            (root / name / "excluded").write_bytes(b"excluded")
        source = (ROOT / "build_debian.sh").read_text()
        start = source.index("squashfs_compression=()")
        block = source[start:source.index('\nif [ -n "${SONIC_IMAGE_TIMING_FILE:-}" ]; then', start)]
        script = '''set -eu
installer_timed() { shift; "$@"; }
sudo() {
    local tool=$1 source=$2 destination=$3
    shift 3
    "$tool" "$source" "$destination" -processors 2 -no-progress "$@" >/dev/null
}
''' + block
        env = {"PATH": os.defpath, "FILESYSTEM_ROOT": str(root), "PLATFORM_DIR": "platform"}
        for level in ("", "3"):
            archive = self.work / ("image-" + (level or "default") + ".squashfs")
            result = subprocess.run(["bash", "-c", script], env={**env,
                "SONIC_SQUASHFS_ZSTD_LEVEL": level, "FILESYSTEM_SQUASHFS": str(archive)},
                text=True, capture_output=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)
            extracted = self.work / ("extracted-" + (level or "default"))
            subprocess.run(["unsquashfs", "-processors", "2", "-no-progress", "-d", str(extracted),
                            str(archive)], check=True, capture_output=True, timeout=30)
            self.assertEqual((extracted / "payload").read_bytes(), (root / "payload").read_bytes())
            self.assertFalse(any(extracted.rglob("excluded")))
        for level in ("0", "23", "3 -noI"):
            archive = self.work / "invalid.squashfs"
            result = subprocess.run(["bash", "-c", script], env={**env,
                "SONIC_SQUASHFS_ZSTD_LEVEL": level, "FILESYSTEM_SQUASHFS": str(archive)},
                text=True, capture_output=True, timeout=10)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(archive.exists())

    def test_compression_changes_rebuild_installer_but_unchanged_settings_do_not(self):
        source = (ROOT / "slave.mk").read_text()
        start = source.index("ifeq ($(CONFIGURED_PLATFORM),vs)\nvs_boot_mode_stamp :=")
        block = source[start:source.index("\n# targets for building installers with base image", start)]
        (self.work / "Makefile").write_text("SHELL=/bin/bash\nTARGET_PATH=target\nCONFIGURED_PLATFORM=vs\n"
            + block + "\ntarget/image: $(vs_boot_mode_stamp)\n\t@echo built >> builds\n\t@touch $@\n")
        settings = [
            ("n", "", ""), ("n", "", ""), ("y", "", ""), ("y", "1", "3"),
            ("y", "1", "3"), ("y", "6", "3"), ("y", "6", ""),
        ]
        for mode, gzip, zstd in settings:
            subprocess.run(["make", "--no-print-directory", "target/image", f"DOCKER_LOAD_ON_BOOT={mode}",
                f"SONIC_BOOT_LAYER_GZIP_LEVEL={gzip}", f"SONIC_SQUASHFS_ZSTD_LEVEL={zstd}"],
                cwd=self.work, check=True, capture_output=True, timeout=10)
        self.assertEqual((self.work / "builds").read_text().splitlines(), ["built"] * 5)

    def test_package_batching_changes_rebuild_installer_but_unchanged_settings_do_not(self):
        source = (ROOT / "slave.mk").read_text()
        start = source.index("ifeq ($(CONFIGURED_PLATFORM),vs)\nvs_boot_mode_stamp :=")
        block = source[start:source.index("\n# targets for building installers with base image", start)]
        (self.work / "Makefile").write_text("SHELL=/bin/bash\nTARGET_PATH=target\nCONFIGURED_PLATFORM=vs\n"
            + block + "\ntarget/image: $(vs_boot_mode_stamp)\n\t@echo built >> builds\n\t@touch $@\n")
        for batching in ("", "", "1", "1", "0", "0"):
            subprocess.run(["make", "--no-print-directory", "target/image",
                f"SONIC_VS_BATCH_PACKAGES={batching}"], cwd=self.work, check=True,
                capture_output=True, timeout=10)
        self.assertEqual((self.work / "builds").read_text().splitlines(), ["built"] * 3)

    def test_streaming_changes_rebuild_only_the_final_installer(self):
        source = (ROOT / "slave.mk").read_text()
        start = source.index("ifeq ($(CONFIGURED_PLATFORM),vs)\nvs_boot_mode_stamp :=")
        block = source[start:source.index("\n# targets for building installers with base image", start)]
        (self.work / "Makefile").write_text("SHELL=/bin/bash\nTARGET_PATH=target\nCONFIGURED_PLATFORM=vs\n"
            + block + "\ntarget/producer:\n\t@mkdir -p $(@D)\n\t@echo producer >> producers\n\t@touch $@\n"
            "\ntarget/image: target/producer $(vs_boot_mode_stamp)\n\t@echo built >> builds\n\t@touch $@\n")
        for streaming in ("", "", "1", "1", "0", "0"):
            subprocess.run(["make", "--no-print-directory", "target/image",
                            f"SONIC_VS_STREAM_ONIE={streaming}"], cwd=self.work, check=True,
                           capture_output=True, timeout=10)
        self.assertEqual((self.work / "builds").read_text().splitlines(), ["built"] * 3)
        self.assertEqual((self.work / "producers").read_text().splitlines(), ["producer"])

    def test_configured_host_reassembles_on_release_change(self):
        source = (ROOT / "slave.mk").read_text()
        start = source.index("ifeq ($(CONFIGURED_PLATFORM):$(SONIC_VS_CONFIGURED_HOST_CACHE),vs:y)")
        block = source[start:source.index("\n$(TARGET_PATH)/sonic-vs.bin:", start)]
        (self.work / "Makefile").write_text(
            "TARGET_PATH=target\nCONFIGURED_PLATFORM=vs\nSONIC_VS_CONFIGURED_HOST_CACHE=y\n"
            ".PHONY: check-vs-boot-mode\nexport SONIC_IMAGE_VERSION\n" + block + "\nendif\n"
            "target/image: $(vs_configured_release)\n\t@echo built >> builds\n\t@touch $@\n")
        for release in ("release-a", "release-a", "release-b", "release-b"):
            subprocess.run(["make", "target/image", "SONIC_IMAGE_VERSION=" + release],
                           cwd=self.work, check=True, capture_output=True, timeout=10)
        self.assertEqual((self.work / "builds").read_text().splitlines(), ["built", "built"])


if __name__ == "__main__":
    unittest.main()
