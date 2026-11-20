"""Exercise native OCI and legacy Docker inputs, shared content and release tags."""

import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import shutil
import sys
import tarfile
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
from boot_image_bundle import build, descriptor, emit, json_bytes, LAYER, OCI
from boot_layout import publish_layout, layout_for_archive


def sha(data):
    return "sha256:" + hashlib.sha256(data).hexdigest()


class BootBundleTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.work = Path(temporary.name)
        self.layout = self.work / "source.oci"
        self.pool = self.layout / "blobs/sha256"
        self.pool.mkdir(parents=True)
        data = io.BytesIO()
        with tarfile.open(fileobj=data, mode="w") as target:
            emit(target, "test/file", b"42")
        self.layer = data.getvalue()
        config = {"os": "linux", "architecture": "amd64", "rootfs": {"type": "layers", "diff_ids": [sha(self.layer)]},
                  "config": {"Labels": {"com.azure.sonic.manifest": '{"service":"builtin"}', "Tag": "v1"}}}
        self.config = json_bytes(config)
        def blob(media, data):
            digest = sha(data)
            (self.pool / digest[7:]).write_bytes(data)
            return descriptor(media, digest, len(data))

        layer = blob(LAYER, self.layer)
        manifest = {"schemaVersion": 2, "layers": [layer], "config": blob(OCI + "config.v1+json", self.config)}
        index = {"schemaVersion": 2, "manifests": [blob(OCI + "manifest.v1+json", json_bytes(manifest))]}
        (self.layout / "index.json").write_bytes(json_bytes(index))
        (self.layout / "oci-layout").write_bytes(b'{"imageLayoutVersion":"1.0.0"}')
        self.legacy = self.work / "docker-package.gz"
        config["config"]["Labels"] = {"Tag": "pkg"}
        self.package_cfg = json_bytes(config)
        with tarfile.open(self.legacy, "w:gz") as out:
            emit(out, "layer/layer.tar", gzip.compress(self.layer, mtime=0))
            emit(out, "config.json", self.package_cfg)
            emit(out, "manifest.json", json_bytes([{"Config": "config.json", "Layers": ["layer/layer.tar"],
                                                    "RepoTags": ["pkg:original"]}]))
        self.images = [(self.layout, ["builtin:v1", "builtin:latest"], True),
                       (self.legacy, ["pkg:latest", "pkg:v1"], False)]
        self.output = self.work / "images.tar.gz"
        self.inventory = self.work / "inventory.json"
        self.labels = self.work / "labels.json"

    def bundle(self, images=None):
        build(self.images if images is None else images, "linux/amd64", self.output, self.inventory,
              PIGZ, "zstd", self.labels)

    def test_hybrid_bundle_and_reproducibility(self):
        self.bundle()
        expected = {"builtin:v1": sha(self.config), "builtin:latest": sha(self.config),
                    **dict.fromkeys(["pkg:latest", "pkg:v1", "pkg:original"], sha(self.package_cfg))}
        self.assertEqual(json.loads(self.inventory.read_bytes()), expected)
        labels = json.loads(self.labels.read_bytes())
        self.assertEqual(labels["builtin:latest"], json.loads(self.config)["config"]["Labels"])
        self.assertEqual(labels["pkg:original"], {"Tag": "pkg"})
        with tarfile.open(self.output) as bundle:
            def read(name):
                return json.load(bundle.extractfile(name))
            self.assertEqual(bundle.getnames()[0], "manifest.json")
            docker = read("manifest.json")
            index = read("index.json")["manifests"]
            self.assertEqual(len(docker), 2)
            self.assertEqual(len(index), 5)
            self.assertEqual(len(set(bundle.getnames())), len(bundle.getnames()))
            layer, = {i["Layers"][0] for i in docker}
            self.assertEqual(gzip.decompress(bundle.extractfile(layer).read()), self.layer)
            for image in index:
                tag = image["annotations"]["io.containerd.image.name"].removeprefix("docker.io/library/")
                manifest = read("blobs/sha256/" + image["digest"][7:])
                self.assertEqual(manifest["config"]["digest"], expected[tag])
            for name in bundle.getnames():
                if name.startswith("blobs/sha256/"):
                    self.assertEqual(sha(bundle.extractfile(name).read())[7:], name.split("/")[-1])
        archived = self.output.read_bytes()
        self.bundle(list(reversed(self.images)))
        self.assertEqual(self.output.read_bytes(), archived)

    def test_reject_bad_inputs_before_replacing_bundle(self):
        self.bundle()
        original = self.output.read_bytes()
        with self.assertRaisesRegex(ValueError, "claim tag"):
            self.bundle([*self.images, (self.legacy, ["builtin:latest"], False)])
        with self.assertRaisesRegex(ValueError, "valid com.azure.sonic.manifest"):
            self.bundle([(self.legacy, ["pkg:latest"], True)])
        layer = self.pool / sha(self.layer)[7:]
        layer.write_bytes(b"broken")
        with self.assertRaisesRegex(ValueError, "digest or size mismatch"):
            self.bundle()
        self.assertEqual(self.output.read_bytes(), original)

    def test_matching_archive_uses_persisted_sidecar(self):
        with tarfile.open(self.legacy, "w:gz") as out:
            emit(out, "config.json", self.config)
            emit(out, "layer.tar", self.layer)
            emit(out, "manifest.json", json_bytes([{"Config": "config.json", "Layers": ["layer.tar"],
                                                    "RepoTags": ["builtin:latest"]}]))
        publish_layout(self.layout, self.legacy, self.legacy)
        shutil.rmtree(self.layout)
        layout = layout_for_archive(self.legacy)
        self.assertTrue(layout.is_dir())
        self.bundle([(self.legacy, ["builtin:latest"], True)])
        (self.legacy.parent / ".boot-image-layouts" / (self.legacy.name + ".json")).write_text("{}")
        self.assertEqual(layout_for_archive(self.legacy), self.legacy)
        self.bundle([(self.legacy, ["builtin:latest"], True)])
        self.legacy.write_bytes(b"different archive")
        self.assertEqual(layout_for_archive(self.legacy), self.legacy)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--pigz", default="pigz")
    args, remaining = parser.parse_known_args()
    PIGZ = args.pigz
    unittest.main(argv=[sys.argv[0], *remaining])
