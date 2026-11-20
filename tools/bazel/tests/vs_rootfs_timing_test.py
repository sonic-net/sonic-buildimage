"""Run the stage-one recipe and its timing transport without building a Debian chroot."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]
TARGET = "target/sonic-vs.bin__vs__rfs.squashfs"
PREPARE = "[ vs timing ] [ rootfs-prepare ]"
COMPRESS = "[ vs timing ] [ rootfs-compress ]"
SAVE = "[ vs timing ] [ rootfs-cache-save ]"


class VsRootfsTimingTest(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.work = Path(tmp.name)
        for directory in ("target", "files/initramfs-tools", "scripts"):
            (self.work / directory).mkdir(parents=True)
        slave = (ROOT / "slave.mk").read_text()
        timing = slave[slave.index("vs_rfs_timing ="):slave.index("\n$(addprefix", slave.index("vs_rfs_timing ="))]
        stage = slave.split("$(SONIC_RFS_TARGETS)) : $(TARGET_PATH)/% :", 1)[1]
        recipe = stage[stage.index("\t$(HEADER)"):stage.index("# targets for building installers with base image")]
        cache = (ROOT / "Makefile.cache").read_text()
        gate = cache[cache.index("WCACHE_OPTIONS :="):cache.index("endef", cache.index("define CHECK_WCACHE_ENABLED"))+5]
        save = cache[cache.index("define SAVE_CACHE\n"):cache.index("endef", cache.index("define SAVE_CACHE\n"))+5]
        # Keep the production recipe, write gate and log pipe. Substitute only the
        # external chroot/archiving work and Make header/footer.
        (self.work / "Makefile").write_text(
            f"SHELL=/bin/bash\n.SHELLFLAGS=-e -c\n.ONESHELL:\ninclude {ROOT}/rules/functions\n"
            "TARGET_PATH=target\nPROJECT_ROOT=.\nHEADER=@echo BEGIN\nFOOTER=echo FINISH\n"
            "SAVE_INTO_CACHE=./save\n" + gate + "\n" + save + "\n" + timing
            + "\ntarget/%:\n" + recipe)
        (self.work / "scripts/process_log.sh").symlink_to(ROOT / "scripts/process_log.sh")
        self.script("j2", ":")
        self.script("save", 'echo SAVE; exit "${SAVE_STATUS:-0}"')
        self.script("update_screen.sh", ":")
        stage = (ROOT / "build_debian.sh").read_text().split("if [[ $RFS_SPLIT_FIRST_STAGE == y ]]; then", 1)[1]
        stage = "if [[ $RFS_SPLIT_FIRST_STAGE == y ]]; then" + stage.split("\nfi\n", 1)[0] + "\nfi\n"
        self.script("build_debian.sh", '''set -xe
sudo() {
    if [[ $1 == mksquashfs ]]; then
        echo MKSQUASHFS
        return "${COMPRESS_STATUS:-0}"
    fi
}
''' + stage)
        self.env = {"PATH": str(self.work) + ":" + os.defpath}

    def script(self, name, body):
        script = self.work / name
        script.write_text("#!/bin/bash\n" + body + "\n")
        script.chmod(0o755)

    def run_make(self, target=TARGET, timings="1", cache="rwcache", **statuses):
        result = subprocess.run(
            ["make", "--no-print-directory", target, "SONIC_VS_IMAGE_TIMINGS=" + timings,
             "SONIC_DPKG_CACHE_METHOD=" + cache, Path(target).name + "_CACHE_MODE=enabled"],
            cwd=self.work, env={**self.env, **statuses}, capture_output=True, text=True)
        events = [line for line in result.stdout.splitlines() if line.startswith("[ vs timing ]")]
        return result, events

    def test_spans_reach_job_stdout_and_normal_output_stays_in_target_log(self):
        result, events = self.run_make()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(events, [PREPARE + " [ start ]", PREPARE + " [ end ]", COMPRESS + " [ start ]",
                                  COMPRESS + " [ end ]", SAVE + " [ start ]", SAVE + " [ end ]"])
        self.assertNotIn("MKSQUASHFS", result.stdout)
        log = (self.work / (TARGET + ".log")).read_text()
        self.assertIn("MKSQUASHFS", log)
        self.assertNotIn("[ vs timing ]", log)

    def test_read_only_cache_and_uninstrumented_targets(self):
        result, events = self.run_make(cache="rcache")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(events, [PREPARE + " [ start ]", PREPARE + " [ end ]", COMPRESS + " [ start ]",
                                  COMPRESS + " [ end ]"])
        for target, timings in ((TARGET, ""), (TARGET.replace(".bin", ".img.gz"), "1")):
            with self.subTest(target=target):
                result, events = self.run_make(target, timings, SONIC_VS_RFS_TIMING_FD="999")
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(events, [])
                self.assertIn("SAVE", result.stdout)
                self.assertIn("MKSQUASHFS", (self.work / (target + ".log")).read_text())

    def test_compression_and_cache_failures_are_preserved(self):
        result, events = self.run_make(COMPRESS_STATUS="31")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(events[-1], COMPRESS + " [ start ]")
        result, events = self.run_make(SAVE_STATUS="32")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(events[-1], SAVE + " [ start ]")

    def test_direct_invocation_ignores_stale_or_invalid_timing_descriptors(self):
        for fd in ("999", "invalid-file"):
            result = subprocess.run(["./build_debian.sh"], cwd=self.work, capture_output=True, text=True,
                                    env={**self.env, "RFS_SPLIT_FIRST_STAGE": "y", "SONIC_VS_RFS_TIMING_FD": fd})
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertNotIn("Bad file descriptor", result.stderr)
            self.assertNotIn("[ vs timing ]", result.stdout)
            self.assertFalse((self.work / fd).exists())
