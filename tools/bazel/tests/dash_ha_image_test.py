"""Exercise production DASH HA image boundaries with real OCI layer inventories."""

import json
from pathlib import Path
import tempfile
import unittest

from tools.bazel.ci import base_images
from tools.bazel.ci.dash_ha_image import verify_image_boundary
from tools.bazel.oci.oci_layout import validate_layout
from tools.bazel.tests.oci_base_fixture import digest, layer_tar, oci_files, write_layout


class DashHaImageTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.parent_files = {"usr/bin/redis-cli": b"redis client", "etc/base": b"SWSS parent"}
        self.base = layer_tar(self.parent_files)
        self.ha = layer_tar({"usr/bin/hamgrd": b"HA"})
        self.parent = self.image("swss", [self.base])

    def image(self, name, layers, diff_ids=None):
        config = {"os": "linux", "architecture": "amd64",
                  "rootfs": {"type": "layers", "diff_ids": diff_ids or [digest(layer) for layer in layers]}}
        path = self.root / name
        write_layout(path, oci_files(json.dumps(config).encode(), layers))
        return validate_layout(path, "linux/amd64")

    def check(self, child, parent, packages=()):
        verify_image_boundary("DASH HA", child, parent, base_images.installed_inventory(child),
                              {"selected": [{"package": name} for name in packages]})

    def test_real_chain_accepts_redis_client_and_debug_tools(self):
        runtime = self.image("runtime", [self.base, self.ha])
        self.check(runtime, self.parent, ["redis-tools"])
        debug = self.image("debug", [self.base, self.ha, layer_tar({"usr/bin/gdb": b"GDB"})])
        self.check(debug, runtime, ["gdb"])
        with self.assertRaisesRegex(ValueError, "exact OCI parent"):
            self.check(self.image("debug-reparented", [self.base, layer_tar({"usr/bin/gdb": b"GDB"})]), runtime)

    def test_repacked_parent_with_identical_installed_files_or_wrong_diff_id_fails(self):
        repacked = layer_tar(dict(reversed(list(self.parent_files.items()))))
        child = self.image("repacked", [repacked, self.ha])
        self.assertEqual(base_images.installed_inventory(child),
                         base_images.installed_inventory(self.image("original", [self.base, self.ha])))
        with self.assertRaisesRegex(ValueError, "exact OCI parent"):
            self.check(child, self.parent)
        wrong = self.image("wrong-id", [self.base, self.ha], [digest(repacked), digest(self.ha)])
        with self.assertRaisesRegex(ValueError, "exact OCI parent"):
            self.check(wrong, self.parent)

    def test_leaked_server_is_rejected_by_path_or_package_receipt(self):
        clean = self.image("clean", [self.base, self.ha])
        with self.assertRaisesRegex(ValueError, "test-only Redis"):
            self.check(clean, self.parent, ["redis-server"])
        copied = self.image("copied", [self.base, self.ha, layer_tar({"usr/bin/redis-server": b"server"})])
        with self.assertRaisesRegex(ValueError, "test-only Redis"):
            self.check(copied, self.parent)


if __name__ == "__main__":
    unittest.main()
