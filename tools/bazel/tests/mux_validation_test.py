"""Exercise the OCI ancestry and installed-image handoff used by MUX CI."""

import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import mux_image, mux_loaded
from tools.bazel.tests.oci_base_fixture import digest, layer_tar, oci_files, write_layout


class MuxValidationTest(unittest.TestCase):
    def test_runtime_and_debug_must_extend_their_exact_parents(self):
        base, service, symbols = (layer_tar({name: b"data"}) for name in ("base", "service", "symbols"))
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)

            def layout(name, layers, diff_ids=None):
                config = {"architecture": "amd64", "os": "linux", "rootfs": {
                    "type": "layers", "diff_ids": diff_ids or [digest(layer) for layer in layers]}}
                path = root / name
                write_layout(path, oci_files(json.dumps(config).encode(), layers))
                return path

            paths = {
                mux_image.PARENT: layout("parent", [base]),
                mux_image.OWNER + "docker-mux": layout("runtime", [base, service]),
                mux_image.OWNER + "docker-mux-dbg": layout("debug", [base, service, symbols]),
            }
            _, images = mux_image.checked_layouts(paths)
            self.assertEqual(set(images), {"docker-mux", "docker-mux-dbg"})

            layout("runtime", [service, base])
            with self.assertRaisesRegex(ValueError, "docker-mux does not extend"):
                mux_image.checked_layouts(paths)

            layout("runtime", [base, service])
            layout("debug", [base, service, symbols], [digest(service), digest(base), digest(symbols)])
            with self.assertRaisesRegex(ValueError, "docker-mux-dbg does not extend"):
                mux_image.checked_layouts(paths)

    def test_installed_check_rejects_an_incomplete_or_unexpected_archive_set(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "validation.json"
            for names in ((), ("docker-mux",), ("docker-mux-dbg",), (*mux_image.IMAGES, "other")):
                with self.subTest(names=names):
                    path.write_text(json.dumps({"status": "passed", "architecture": "amd64",
                                                "revision": "test-revision", "tested_revision_parents": [],
                                                "images": dict.fromkeys(names)}))
                    with mock.patch.object(sys, "argv", ["mux_loaded.py", "--artifacts", directory]), \
                            mock.patch.object(mux_image, "clean_source"), \
                            mock.patch.object(mux_image, "source_identity", return_value={"revision": "test-revision", "tested_revision_parents": []}):
                        with self.assertRaisesRegex(ValueError, "MUX acceptance requires both runtime and debug images"):
                            mux_loaded.main()


if __name__ == "__main__":
    unittest.main()
