import json
from pathlib import Path
import runpy
import subprocess
import tempfile
import unittest
from unittest.mock import Mock, patch


ROOT = Path(__file__).resolve().parents[3]
load_images = runpy.run_path(str(ROOT / "files/docker/sonic-load-images"))["load_images"]


class LoadOnBootTest(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.store = Path(tmp.name)
        self.bundle = self.store / "sonic-images.tar.gz"
        self.inventory = self.store / "inventory.json"
        self.expected = {
            "docker-orchagent:latest": "sha256:" + "a" * 64,
            "docker-orchagent:release": "sha256:" + "a" * 64,
            "docker-database:latest": "sha256:" + "b" * 64,
        }
        self.inventory.write_text(json.dumps(self.expected))
        self.bundle.write_bytes(b"image-bundle")
        self.load = self.enterContext(patch("subprocess.run"))
        self.inspect = self.enterContext(patch("subprocess.check_output"))
        self.inspect.return_value = "\n".join(self.expected.values()) + "\n"

    def run_loader(self):
        load_images(self.store, self.inventory)

    def test_success_verifies_aliases_and_second_start_preserves_package_updates(self):
        self.run_loader()
        self.assertFalse(self.bundle.exists())
        self.assertEqual(self.inspect.call_args.args[0][-3:], list(self.expected))
        self.run_loader()
        self.assertEqual(self.load.call_count, 2)  # docker load, followed by sync -f
        self.inspect.assert_called_once()

    def test_interrupted_load_retries_the_same_archive(self):
        self.load.side_effect = subprocess.CalledProcessError(1, "docker image load")
        with self.assertRaises(subprocess.CalledProcessError):
            self.run_loader()
        self.assertTrue(self.bundle.is_file())
        self.assertFalse((self.store / ".sonic-images-loaded").exists())
        self.load.side_effect = None
        self.run_loader()
        self.assertFalse(self.bundle.exists())

    def test_wrong_image_id_keeps_source(self):
        self.inspect.return_value = "\n".join(reversed(self.expected.values())) + "\n"
        with self.assertRaisesRegex(RuntimeError, "image IDs"):
            self.run_loader()
        self.assertTrue(self.bundle.is_file())
        self.assertFalse((self.store / ".sonic-images-loaded").exists())

    def test_store_sync_failure_retains_source_for_retry(self):
        self.load.side_effect = [None, subprocess.CalledProcessError(1, "sync")]
        with self.assertRaises(subprocess.CalledProcessError):
            self.run_loader()
        self.assertTrue(self.bundle.is_file())
        self.assertFalse((self.store / ".sonic-images-loaded").exists())
        self.load.side_effect = None
        self.run_loader()
        self.assertFalse(self.bundle.exists())

    def test_upgrade_waits_even_with_valid_receipt(self):
        self.run_loader()
        ready = Mock()
        with patch.dict(load_images.__globals__, wait_for_daemon=ready):
            load_images(self.store, self.inventory, wait=True)
        ready.assert_called_once_with()

    def test_missing_source_or_stale_receipt_fails(self):
        self.bundle.unlink()
        (self.store / ".sonic-images-loaded").write_text("other release")
        with self.assertRaisesRegex(RuntimeError, "missing"):
            self.run_loader()
        self.load.assert_not_called()

    def test_ram_store_unlinks_only_the_link(self):
        source = self.store / "image-release" / self.bundle.name
        source.parent.mkdir()
        self.bundle.rename(source)
        self.bundle.symlink_to(source)
        self.run_loader()
        self.assertTrue(source.is_file())
        self.assertFalse(self.bundle.is_symlink())


if __name__ == "__main__":
    unittest.main()
