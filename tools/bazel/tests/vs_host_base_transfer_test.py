"""Exercise private cache handoff through GitHub's normalized artifact directories."""

import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "ci"))
import vs_host_base_transfer as transfer


KEY = "abcd" * 16
OTHER = "f" * 64


class HostBaseTransferTest(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.work = Path(temp.name)
        self.cold = self.work / "cold cache"
        self.warm = self.work / "warm cache"
        self.publish = self.work / "published"
        self.download = self.work / "downloaded"
        self.cold.mkdir(mode=0o700)
        self.src = self.cold / KEY
        self.src.mkdir(mode=0o700)
        (self.src / "cas").mkdir()
        (self.src / "cas/blob").write_bytes(b"cached SONiC filesystem")
        (self.cold / OTHER).mkdir(mode=0o700)

    def test_cli_moves_only_the_selected_cache_without_copying_and_restores_privacy(self):
        cli = [sys.executable, "-B", transfer.__file__]
        inode = (self.src / "cas/blob").stat().st_ino
        subprocess.run([*cli, "pack", self.cold, KEY, self.publish], check=True)
        self.assertEqual([p.name for p in self.cold.iterdir()], [OTHER])
        self.assertEqual([p.name for p in self.publish.iterdir()], [KEY])
        self.assertEqual((self.publish / KEY / "cas/blob").stat().st_ino, inode)
        shutil.copytree(self.publish, self.download)
        for path in self.download.rglob("*"):
            path.chmod(0o755 if path.is_dir() else 0o644)
        self.warm.mkdir(mode=0o755)
        inode = (self.download / KEY / "cas/blob").stat().st_ino
        result = subprocess.check_output([*cli, "restore", self.warm, self.download], text=True)
        self.assertEqual(result.strip(), KEY)
        for path in (self.warm, self.warm / KEY):
            self.assertEqual(stat.S_IMODE(path.stat().st_mode), 0o700)
        blob = self.warm / KEY / "cas/blob"
        self.assertEqual(blob.stat().st_ino, inode)
        self.assertEqual(blob.read_bytes(), b"cached SONiC filesystem")
        self.assertEqual(list(self.download.iterdir()), [])

    def test_pack_and_restore_under_a_symlinked_runner_workspace(self):
        workspace = self.work / "runner"
        workspace.symlink_to(self.work, target_is_directory=True)
        transfer.pack(workspace / self.cold.name, KEY, workspace / self.publish.name)
        transfer.restore(workspace / self.warm.name, workspace / self.publish.name)
        self.assertEqual((self.warm / KEY / "cas/blob").read_bytes(), b"cached SONiC filesystem")
        leaf_alias = self.work / "cache-alias"
        leaf_alias.symlink_to(self.warm, target_is_directory=True)
        with self.assertRaises(ValueError):
            transfer.pack(workspace / leaf_alias.name, KEY, self.publish)

    def test_restore_keeps_an_existing_valid_cache(self):
        shutil.copytree(self.cold, self.warm)
        transfer.pack(self.cold, KEY, self.publish)
        blob = self.warm / KEY / "cas/blob"
        blob.write_bytes(b"already on the worker")
        self.assertEqual(transfer.restore(self.warm, self.publish), KEY)
        self.assertEqual(blob.read_bytes(), b"already on the worker")
        self.assertTrue((self.publish / KEY).is_dir())

    def test_invalid_keys_overlapping_directories_and_mixed_downloads_are_rejected(self):
        for key in ("../" + KEY, KEY.upper(), "", KEY[:-1]):
            with self.subTest(key=key), self.assertRaises(ValueError):
                transfer.pack(self.cold, key, self.publish)
        with self.assertRaises(ValueError):
            transfer.pack(self.cold, KEY, self.src / "artifact")
        self.assertFalse(self.publish.exists())
        self.download.mkdir()
        (self.download / KEY).mkdir()
        (self.download / OTHER).mkdir()
        with self.assertRaises(ValueError):
            transfer.restore(self.warm, self.download)
        with self.assertRaises(ValueError):
            transfer.pack(self.cold, KEY, self.download)
        self.assertTrue(self.src.exists())
        self.assertFalse(self.warm.exists())

    def test_source_links_special_files_and_destination_symlinks_are_rejected(self):
        bad = self.src / "cas/entry"
        for kind in ("symlink", "directory link", "fifo"):
            with self.subTest(kind=kind):
                if kind == "fifo":
                    os.mkfifo(bad)
                else:
                    bad.symlink_to(self.work if kind == "directory link" else self.src / "cas/blob")
                with self.assertRaises(ValueError):
                    transfer.pack(self.cold, KEY, self.publish)
                self.assertTrue(self.src.exists())
                self.assertFalse(self.publish.exists())
                bad.unlink()
        transfer.pack(self.cold, KEY, self.publish)
        self.warm.mkdir(mode=0o700)
        (self.warm / KEY).symlink_to(self.work)
        with self.assertRaises(ValueError):
            transfer.restore(self.warm, self.publish)
        self.assertTrue((self.warm / KEY).is_symlink())
        (self.warm / KEY).unlink()
        (self.publish / KEY / "cas/entry").symlink_to(self.work)
        with self.assertRaises(ValueError):
            transfer.restore(self.warm, self.publish)
        self.assertEqual(list(self.warm.iterdir()), [])
