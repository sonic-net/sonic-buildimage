"""Exercise a full cache miss, a cross-commit hit and host-input invalidation."""

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools/bazel/ci"))
import vs_host_base as host

KERNEL = "linux-image-6.12.41+deb13-sonic-amd64-unsigned_6.12.41-1_amd64.deb"
INITRAMFS = "initramfs-tools_0.148_all.deb"
DEBS = Path("target/debs/trixie")
MODULES = Path("files/initramfs-tools/modules")
RECEIPT = Path("artifacts/vs/host-base-cache.json")
BUILDER = "sha256:" + "a" * 64


def put(repo, name, content):
    path = repo / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)


def git(repo, *args):
    return subprocess.check_output(["git", "-C", str(repo), *args], text=True).strip()


def commit(repo, message):
    git(repo, "add", "-f", ".")
    git(repo, "-c", "user.name=Cache test", "-c", "user.email=test@example.org",
        "-c", "commit.gpgsign=false", "commit", "-qm", message)
    return git(repo, "rev-parse", "HEAD")


class VsHostBaseCacheTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.work = Path(temporary.name)
        self.repo = self.work / "source"
        self.repo.mkdir()
        git(self.repo, "init", "-q")
        for name in host.SOURCE_PATHS:
            if str(name) == "src/sonic-build-hooks/buildinfo":
                continue
            if (ROOT / name).is_dir():
                put(self.repo, Path(name) / "fixture-input", "fixture\n")
            else:
                put(self.repo, name, "fixture\n")
        for name in (MODULES, "files/initramfs-tools/arista-convertfs", "files/initramfs-tools/union-mount",
                     "target/versions/default/versions-mirror", "src/sonic-build-hooks/buildinfo/config/buildinfo.config",
                     "src/sonic-build-hooks/buildinfo/sonic-build-hooks_1.0_all.deb", DEBS / KERNEL,
                     DEBS / INITRAMFS, DEBS / "initramfs-tools-core_0.148_all.deb"):
            put(self.repo, name, "fixture\n")
        put(self.repo, "src/sonic-build-hooks/buildinfo/config/buildinfo.config", "export DISTRO=trixie\n")
        put(self.repo, MODULES, "squashfs\noverlay\n")
        for name in ("scripts/vs_host_base_state.sh", "tools/bazel/ci/vs_host_base.py", "tools/bazel/ci/vs_host_base.bzl"):
            target = self.repo / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / name, target)
        put(self.repo, "build_debian.sh", """#!/bin/sh
set -eu
echo run >> native-calls
mkdir -p base/etc/sonic target/versions/host-base-image
echo vs-host-base-v1 > base/etc/sonic/.vs-host-base-v1
cp files/initramfs-tools/modules base/etc/modules
mksquashfs base target/sonic-vs.bin__vs__rfs.squashfs -processors 1 -noappend >/dev/null
echo base-files=1 > target/versions/host-base-image/versions-deb-trixie-amd64
""")
        (self.repo / "build_debian.sh").chmod(0o755)
        self.cache = self.work / "cache"
        self.cache.mkdir(mode=0o700)
        self.config = dict(
            TARGET_PATH="target", TARGET_MACHINE="vs", TARGET_BOOTLOADER="grub", IMAGE_TYPE="onie",
            IMAGE_DISTRO="trixie", CONFIGURED_PLATFORM="vs", CONFIGURED_ARCH="amd64", BLDENV="trixie",
            RFS_SQUASHFS_NAME="sonic-vs.bin__vs__rfs.squashfs", RFS_SPLIT_FIRST_STAGE="y",
            RFS_SPLIT_LAST_STAGE="n", USERNAME="admin", debs_path=str(DEBS), kversion="6.12.41+deb13-sonic-amd64",
            linux_kernel=str(DEBS / KERNEL), initramfs_tools=str(DEBS / INITRAMFS),
            sonicadmin_user="admin", sonic_asic_platform="vs", image_type="onie", MIRROR_SNAPSHOT="y",
            BUILD_SNAPSHOT_URL="http://packages.trafficmanager.net/snapshot",
            BUILD_PUBLIC_URL="https://packages.trafficmanager.net/public",
            BUILD_PACKAGES_URL="https://packages.trafficmanager.net/packages",
            TRUSTED_GPG_URLS="https://packages.microsoft.com/keys/microsoft.asc",
        )

    def test_identity_tracks_host_inputs_across_commits(self):
        def identity():
            return host.inputs(self.repo, self.config, BUILDER)
        initial = commit(self.repo, "Configure the host")
        original = identity()
        put(self.repo, "dockers/docker-orchagent/README.md", "Update the SWSS image documentation\n")
        self.assertNotEqual(initial, commit(self.repo, "Document SWSS"))
        self.assertEqual(identity(), original)
        (self.repo / DEBS / KERNEL).write_bytes(b"changed package, same filename")
        self.assertNotEqual(identity(), original)
        (self.repo / DEBS / KERNEL).write_text("fixture\n")
        self.assertEqual(identity(), original)
        self.assertNotEqual(host.inputs(self.repo, self.config, "sha256:" + "b" * 64), original)
        put(self.repo, "target/versions/default/versions-mirror", "different snapshot\n")
        self.assertNotEqual(identity(), original)

    def test_verify_both_outputs_before_replacing_retained_targets(self):
        outputs = self.work / "bazel-bin"
        put(outputs, "host.squashfs", "fs")
        put(outputs, "host.inventory", "inventory")
        key = "a" * 64
        expected = {name: host.fingerprint(outputs / file) for name, file in
                    (("image", "host.squashfs"), ("inventory", "host.inventory"))}
        put(outputs, "host.manifest", json.dumps({"identity_sha256": key, "outputs": expected}))
        host.materialize(self.repo, outputs, key)
        image = self.repo / host.IMAGE
        inventory = self.repo / host.INVENTORY
        original = image.stat()
        inventory.write_text("stale")
        host.materialize(self.repo, outputs, key)
        self.assertEqual((image.stat().st_ino, image.stat().st_mtime_ns),
                         (original.st_ino, original.st_mtime_ns))
        self.assertEqual(inventory.read_text(), "inventory")
        inventory.write_text("retained inventory")
        (outputs / "host.squashfs").write_text("damaged cache")
        with self.assertRaisesRegex(ValueError, "image digest mismatch"):
            host.materialize(self.repo, outputs, key)
        self.assertEqual(inventory.read_text(), "retained inventory")
        self.assertEqual(image.read_text(), "fs")

    def test_cross_commit_disk_cache_and_invalidation(self):
        try:
            if not shutil.which("mksquashfs") or not shutil.which("unsquashfs"):
                raise FileNotFoundError("integration test needs squashfs-tools")
            host.bazel_binary()
        except (ValueError, FileNotFoundError) as unavailable:
            if os.getenv("SONIC_REQUIRE_HOST_BASE_TEST") == "1":
                raise
            self.skipTest(str(unavailable))

        def build(revision, worker):
            checkout = self.work / worker
            git(self.repo, "clone", "-q", "--local", ".", str(checkout))
            git(checkout, "checkout", "-q", revision)
            env = {key: os.environ[key] for key in ("HOME", "PATH", "BAZELISK_HOME") if key in os.environ}
            env.update(self.config, SONIC_HOSTBASE_CACHE_DIR=str(self.cache), SONIC_HOSTBASE_BUILDER_ID=BUILDER)
            result = subprocess.run([sys.executable, "-B", "tools/bazel/ci/vs_host_base.py"], cwd=checkout,
                                    env=env, capture_output=True, text=True, timeout=150)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertTrue((checkout / RECEIPT).is_file(), result.stdout + result.stderr)
            receipt = json.loads((checkout / RECEIPT).read_text())
            self.assertEqual(receipt["revision"], revision)
            self.assertEqual((checkout / "native-calls").exists(), not receipt["cache_hit"])
            return receipt, host.fingerprint(checkout / host.IMAGE)

        first = commit(self.repo, "Configure the host")
        put(self.repo, "dockers/docker-orchagent/README.md", "Describe the container\n")
        second = commit(self.repo, "Document SWSS")
        put(self.repo, MODULES, "squashfs\noverlay\nnvme\n")
        third = commit(self.repo, "Add nvme to initramfs")
        cold, cold_fs = build(first, "A")
        reused, reused_fs = build(second, "B")
        changed, changed_fs = build(third, "C")
        self.assertFalse(cold["cache_hit"])
        self.assertTrue(reused["cache_hit"])
        self.assertEqual(reused["native_build_seconds"], 0)
        self.assertEqual(cold["identity_sha256"], reused["identity_sha256"])
        self.assertEqual(cold_fs, reused_fs)
        self.assertFalse(changed["cache_hit"])
        self.assertNotEqual(changed["identity_sha256"], reused["identity_sha256"])
        self.assertNotEqual(changed_fs, reused_fs)


if __name__ == "__main__":
    unittest.main()
