"""Exercise single-build rootfs reuse and the production extraction fallback."""

import importlib.util
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import tempfile
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location(
    "installer_rootfs_handoff", ROOT / "scripts/installer_rootfs_handoff.py")
handoff = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(handoff)


class InstallerRootfsHandoffTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.work = Path(temporary.name)
        self.session = self.work / ".vs-rootfs-handoff.fixture"
        self.session.mkdir(mode=0o700)
        self.root = self.work / "fsroot-vs"
        self.root.mkdir()
        self.file = self.root / "payload"
        self.file.write_bytes(b"fixture rootfs")
        self.file.chmod(0o640)
        (self.root / "hardlink").hardlink_to(self.file)
        (self.root / "symlink").symlink_to("payload")
        self.archive = self.work / "target/sonic-vs.bin__vs__rfs.squashfs"
        self.archive.parent.mkdir()
        self.archive.write_bytes(b"fixture squashfs")
        previous = Path.cwd()
        os.chdir(self.work)
        self.addCleanup(os.chdir, previous)

    def run_handoff(self, action):
        return handoff.handoff(action, self.session, self.root, self.archive)

    def test_fresh_tree_moves_without_copy_and_is_consumed_once(self):
        inode = self.file.stat().st_ino
        archive = self.archive.read_bytes()
        self.assertTrue(self.run_handoff("store"))
        self.assertFalse(self.root.exists())
        self.assertTrue(self.run_handoff("restore"))
        self.assertEqual(self.file.stat().st_ino, inode)
        self.assertEqual((self.root / "hardlink").stat().st_ino, inode)
        self.assertEqual(os.readlink(self.root / "symlink"), "payload")
        self.assertEqual(self.file.stat().st_mode & 0o777, 0o640)
        self.assertEqual(self.archive.read_bytes(), archive)
        self.assertFalse(self.run_handoff("restore"))
        handoff.cleanup(self.session)
        self.assertTrue(self.file.exists())
        self.assertFalse(self.session.exists())

    def test_absent_corrupt_or_nonobject_receipts_fall_back(self):
        for contents in (None, "{", "[]", '{"status":[]}', '{"status":"stored"}'):
            with self.subTest(contents=contents):
                state = self.session / "state.json"
                state.unlink(missing_ok=True)
                if contents is not None:
                    state.write_text(contents)
                self.assertFalse(self.run_handoff("restore"))
                self.assertEqual(self.file.read_bytes(), b"fixture rootfs")

    def test_host_base_receipt_rejects_cache_hits_and_requires_matching_fresh_outputs(self):
        receipt = self.work / "host-base-cache.json"
        archive = handoff.archive_identity(self.archive)
        image = {"sha256": archive["sha256"], "bytes": archive["stat"][2]}
        for contents, expected in (
                (None, False),
                ("{", False),
                ("[]", False),
                (json.dumps({"cache_hit": True, "outputs": {"image": image}}), False),
                (json.dumps({"cache_hit": False}), False),
                (json.dumps({"cache_hit": False, "outputs": {"image": {
                    "sha256": "0" * 64, "bytes": image["bytes"]}}}), False),
                (json.dumps({"cache_hit": False, "outputs": {"image": image}}), True)):
            with self.subTest(contents=contents):
                (self.session / "state.json").unlink(missing_ok=True)
                receipt.unlink(missing_ok=True)
                if contents is not None:
                    receipt.write_text(contents)
                self.assertEqual(handoff.handoff(
                    "store", self.session, self.root, self.archive, receipt), expected)
                if expected:
                    self.assertTrue(self.run_handoff("restore"))
                self.assertEqual(self.file.read_bytes(), b"fixture rootfs")

    @unittest.skipUnless(shutil.which("mksquashfs") and shutil.which("unsquashfs"),
                         "requires squashfs-tools")
    def test_real_squashfs_extraction_and_handoff_preserve_rootfs_metadata(self):
        os.mkfifo(self.root / "fifo")
        xattrs = True
        try:
            os.setxattr(self.file, b"user.handoff", b"fixture")
        except OSError:
            xattrs = False
        self.archive.unlink()
        subprocess.run(["mksquashfs", str(self.root), str(self.archive), "-noappend",
                        "-processors", "1", "-quiet"], check=True, capture_output=True, timeout=30)
        reference = self.work / "reference"
        subprocess.run(["unsquashfs", "-processors", "1", "-no-progress", "-d",
                        str(reference), str(self.archive)], check=True, capture_output=True, timeout=30)
        self.assertTrue(self.run_handoff("store"))
        self.assertTrue(self.run_handoff("restore"))
        self.assertEqual({path.name for path in self.root.iterdir()},
                         {path.name for path in reference.iterdir()})
        for name in ("payload", "hardlink", "symlink", "fifo"):
            actual, expected = self.root / name, reference / name
            left, right = actual.lstat(), expected.lstat()
            self.assertEqual((left.st_mode, left.st_uid, left.st_gid, int(left.st_mtime)),
                             (right.st_mode, right.st_uid, right.st_gid, int(right.st_mtime)))
            if stat.S_ISREG(left.st_mode):
                self.assertEqual(actual.read_bytes(), expected.read_bytes())
        self.assertEqual((self.root / "hardlink").stat().st_ino, self.file.stat().st_ino)
        self.assertEqual(os.readlink(self.root / "symlink"), os.readlink(reference / "symlink"))
        if xattrs:
            self.assertEqual(os.getxattr(self.file, b"user.handoff"),
                             os.getxattr(reference / "payload", b"user.handoff"))

    def test_changed_archive_invalidates_even_if_size_and_mtime_are_restored(self):
        self.assertTrue(self.run_handoff("store"))
        info = self.archive.stat()
        self.archive.write_bytes(b"changed squashfs")
        os.utime(self.archive, ns=(info.st_atime_ns, info.st_mtime_ns))
        self.assertFalse(self.run_handoff("restore"))
        self.assertFalse(self.root.exists())
        self.assertEqual(json.loads((self.session / "state.json").read_text())["status"], "miss")

    def test_existing_destination_or_replaced_tree_is_not_overwritten(self):
        self.assertTrue(self.run_handoff("store"))
        self.root.mkdir()
        sentinel = self.root / "other-build"
        sentinel.touch()
        self.assertFalse(self.run_handoff("restore"))
        self.assertTrue(sentinel.exists())
        handoff.cleanup(self.session)
        self.assertTrue(sentinel.exists())

    def test_mounted_source_is_not_moved_or_cleaned(self):
        with mock.patch.object(handoff, "has_mounts", return_value=True):
            self.assertFalse(self.run_handoff("store"))
            self.assertTrue(self.file.exists())
            with self.assertRaises(ValueError):
                handoff.cleanup(self.session)
        self.assertTrue(self.session.exists())

    def test_symlinked_payload_is_not_followed(self):
        self.assertTrue(self.run_handoff("store"))
        (self.session / "root").rename(self.work / "outside")
        (self.session / "root").symlink_to(self.work / "outside", target_is_directory=True)
        self.assertFalse(self.run_handoff("restore"))
        handoff.cleanup(self.session)
        self.assertTrue((self.work / "outside/payload").exists())

    def test_cleanup_rejects_broad_foreign_and_redirected_paths(self):
        link = self.work / ".vs-rootfs-handoff.redirect"
        link.symlink_to(self.root, target_is_directory=True)
        for path in (self.work, self.root, link):
            with self.subTest(path=path), self.assertRaises(ValueError):
                handoff.cleanup(path)
        self.assertTrue(self.file.exists())

    def test_production_restore_block_falls_back_only_on_a_miss(self):
        source = (ROOT / "build_debian.sh").read_text()
        start = source.index("    if [[ $installer_rootfs_handoff != y ]]")
        end = source.index("\n    ## make /", start)
        block = source[start:end]
        (self.work / "scripts").mkdir()
        (self.work / "scripts/installer_rootfs_handoff.py").symlink_to(
            ROOT / "scripts/installer_rootfs_handoff.py")
        timing = source[source.index("installer_timed()\n"):source.index("\nORGANIZATION_BUILD_HOOK=")]
        shell = '''set -eu
sudo() {
    if [[ $1 == python3 ]]; then command "$@"
    else printf '%s\\n' "$1" >> fallback-events
    fi
}
''' + timing + block
        env = dict(os.environ, installer_rootfs_handoff="y",
                   SONIC_VS_RFS_HANDOFF_SESSION=str(self.session),
                   FILESYSTEM_ROOT=str(self.root), TARGET_PATH=str(self.archive.parent),
                   RFS_SQUASHFS_NAME=self.archive.name)
        self.assertTrue(self.run_handoff("store"))
        result = subprocess.run(["bash", "-c", shell], cwd=self.work, env=env,
                                text=True, capture_output=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse((self.work / "fallback-events").exists())
        result = subprocess.run(["bash", "-c", shell], cwd=self.work, env=env,
                                text=True, capture_output=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((self.work / "fallback-events").read_text().splitlines(),
                         ["fuser", "rm", "unsquashfs"])

    def test_handoff_profile_excludes_other_images_and_cross_builds(self):
        source = (ROOT / "build_debian.sh").read_text()
        block = "installer_rootfs_handoff=n" + source.split("installer_rootfs_handoff=n", 1)[1].split(
            '[ -n "$ONIE_IMAGE_PART_SIZE"', 1)[0]
        base = dict(os.environ, SONIC_VS_RFS_HANDOFF_SESSION=str(self.session),
                    TARGET_MACHINE="vs", IMAGE_TYPE="onie", CONFIGURED_ARCH="amd64",
                    CROSS_BUILD_ENVIRON="n", MULTIARCH_QEMU_ENVIRON="n")
        for overrides, expected in (({}, "y"), ({"IMAGE_TYPE": "kvm"}, "n"),
                                    ({"TARGET_MACHINE": "other"}, "n"),
                                    ({"CONFIGURED_ARCH": "arm64"}, "n"),
                                    ({"CROSS_BUILD_ENVIRON": "y"}, "n"),
                                    ({"MULTIARCH_QEMU_ENVIRON": "y"}, "n"),
                                    ({"SONIC_VS_RFS_HANDOFF_SESSION": ""}, "n")):
            with self.subTest(overrides=overrides):
                result = subprocess.run(
                    ["bash", "-c", block + '\necho "$installer_rootfs_handoff"'],
                    env={**base, **overrides}, text=True, capture_output=True, timeout=10)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout.strip(), expected)


if __name__ == "__main__":
    unittest.main()
