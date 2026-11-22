"""Exercise native OCI and legacy Docker inputs, shared content and release tags."""

import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
from boot_bundle_action import archive_input, bundle_input, normalize_layer, project_members
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

    def cached_layer(self, src, desc, diff, name):
        receipt = self.work / (name + ".json")
        normalized = (src if desc["mediaType"].endswith(("+gzip", ".gzip")) else self.work / (name + ".gz"))
        normalize_layer(src, desc, diff, receipt, normalized, PIGZ, ZSTD)
        return receipt, normalized

    def test_cached_hybrid_preserves_every_byte_and_invalidates_tags(self):
        # Two different encodings of the same DiffID: the existing gzip source
        # must win, even when the raw layer was visited first.
        self.bundle()
        expected = [p.read_bytes() for p in (self.output, self.inventory, self.labels)]
        extracted = self.work / "archive"
        archive_info = self.work / "archive.json"
        archive_input(self.legacy, None, None, extracted, archive_info, PIGZ)
        archive = json.loads(archive_info.read_bytes())
        self.assertFalse(archive["use_sidecar"])
        self.assertEqual(archive["diff_ids"], [sha(self.layer)])
        self.assertEqual(archive_info.read_bytes(), (extracted / "image.json").read_bytes())
        native_desc = descriptor(LAYER, sha(self.layer), len(self.layer))
        archive_desc, = archive["layers"]
        raw_src = self.pool / sha(self.layer)[7:]
        gzip_src = extracted / "layers" / archive_desc["digest"][7:]
        configs = [self.pool / sha(self.config)[7:], extracted / "configs" / archive["config"][7:]]
        layers = [self.cached_layer(raw_src, native_desc, sha(self.layer), "raw"),
                  self.cached_layer(gzip_src, archive_desc, sha(self.layer), "gzip")]
        images = [{"config": sha(self.config), "tags": self.images[0][1], "layers": [native_desc], "check_label": True},
                  {**archive, "tags": self.images[1][1], "check_label": False}]
        spec = self.work / "images.json"

        def cached():
            spec.write_bytes(json_bytes({"images": images}))
            bundle_input(spec, configs, layers, "linux/amd64", self.output, self.inventory, self.labels, PIGZ)
            return [p.read_bytes() for p in (self.output, self.inventory, self.labels)]

        self.assertEqual(cached(), expected)
        images[0]["tags"] = ["builtin:release"]
        self.assertNotEqual(cached()[0], expected[0])
        self.assertIn("builtin:release", json.loads(self.inventory.read_bytes()))
        images[0]["tags"] = ["pkg:original"]
        with self.assertRaisesRegex(ValueError, "multiple images claim tag"):
            cached()
        images[0]["tags"] = ["builtin:v1"]
        images[1]["check_label"] = True
        with self.assertRaisesRegex(ValueError, "valid com.azure.sonic.manifest"):
            cached()

    def test_cached_encoding_validation_and_zstd_normalization(self):
        raw = self.pool / sha(self.layer)[7:]
        zstd = self.work / "layer.zst"
        with zstd.open("wb") as out:
            subprocess.run([ZSTD, "-q", "-c", str(raw)], stdout=out, check=True)
        data = zstd.read_bytes()
        desc = descriptor(LAYER + "+zstd", sha(data), len(data))
        _, normalized = self.cached_layer(zstd, desc, sha(self.layer), "zstd")
        raw_desc = descriptor(LAYER, sha(self.layer), len(self.layer))
        _, raw_normalized = self.cached_layer(raw, raw_desc, sha(self.layer), "raw")
        self.assertEqual(normalized.read_bytes(), raw_normalized.read_bytes())
        with self.assertRaisesRegex(ValueError, "incorrect source layer size"):
            self.cached_layer(zstd, {**desc, "size": len(data) - 1}, sha(self.layer), "bad-size")
        with self.assertRaisesRegex(ValueError, "incorrect source layer digest"):
            self.cached_layer(zstd, {**desc, "digest": sha(b"wrong")}, sha(self.layer), "bad-encoded")
        with self.assertRaisesRegex(ValueError, "incorrect uncompressed layer digest"):
            self.cached_layer(zstd, desc, sha(b"wrong"), "bad-diff")
        with self.assertRaisesRegex(ValueError, "encoding does not match"):
            self.cached_layer(zstd, {**desc, "mediaType": LAYER}, sha(self.layer), "bad-media")

    def test_fast_layer_profile_preserves_diffids_tags_and_existing_gzip_bytes(self):
        raw = self.pool / sha(self.layer)[7:]
        desc = descriptor(LAYER, sha(self.layer), len(self.layer))
        inventory = None
        for level in (1, 6):
            receipt = self.work / ("layer-" + str(level) + ".json")
            encoded = self.work / ("layer-" + str(level) + ".gz")
            normalize_layer(raw, desc, sha(self.layer), receipt, encoded, PIGZ, ZSTD, level)
            self.assertEqual(gzip.decompress(encoded.read_bytes()), self.layer)
            report = json.loads(receipt.read_bytes())
            self.assertEqual(report["diff_id"], sha(self.layer))
            self.assertEqual(report["normalized"]["digest"], sha(encoded.read_bytes()))
            spec = self.work / "fast.json"
            spec.write_bytes(json_bytes({"images": [{"config": sha(self.config),
                "tags": ["builtin:latest", "builtin:v1"], "layers": [desc], "check_label": True}]}))
            bundle_input(spec, [self.pool / sha(self.config)[7:]], [(receipt, encoded)],
                         "linux/amd64", self.output, self.inventory, self.labels, PIGZ)
            current = self.inventory.read_bytes()
            if inventory is not None:
                self.assertEqual(current, inventory)
            inventory = current
            with tarfile.open(self.output) as bundle:
                manifest = json.load(bundle.extractfile("manifest.json"))[0]
                self.assertEqual(gzip.decompress(bundle.extractfile(manifest["Layers"][0]).read()), self.layer)
            with self.assertRaisesRegex(ValueError, "uncompressed layer digest"):
                normalize_layer(raw, desc, sha(b"bad"), receipt, encoded, PIGZ, ZSTD, level)

        compressed = self.work / "original.gz"
        compressed.write_bytes(gzip.compress(self.layer, mtime=0))
        original = compressed.read_bytes()
        desc = descriptor(LAYER + "+gzip", sha(original), len(original))
        normalize_layer(compressed, desc, sha(self.layer), self.work / "gzip.json",
                        None, PIGZ, ZSTD, 1)
        self.assertEqual(compressed.read_bytes(), original)
        with self.assertRaisesRegex(ValueError, "gzip level"):
            normalize_layer(raw, desc, sha(self.layer), receipt, encoded, PIGZ, ZSTD, 0)

    def test_projected_members_are_independent_and_preserve_bytes(self):
        source = self.work / "archive/layers" / sha(self.layer)[7:]
        source.parent.mkdir(parents=True)
        source.write_bytes(self.layer)
        destination = self.work / "project/layers" / source.name
        destination.parent.mkdir(parents=True)
        project_members(self.work / "archive", [destination])
        self.assertEqual(destination.read_bytes(), self.layer)
        self.assertFalse(destination.samefile(source))
        destination.write_bytes(b"changed")
        self.assertEqual(source.read_bytes(), self.layer)

    def test_cached_archive_route_verifies_receipt_and_falls_back(self):
        publish_layout(self.layout, self.legacy, self.legacy)
        store = self.legacy.parent / ".boot-image-layouts"
        receipt = store / (self.legacy.name + ".json")
        index = store / json.loads(receipt.read_bytes())["index"] / "index.json"

        def route(name):
            dest = self.work / name
            info = self.work / (name + ".json")
            archive_input(self.legacy, receipt, index, dest, info, PIGZ)
            return json.loads(info.read_bytes())

        self.assertEqual(route("matched"), {"use_sidecar": True})
        receipt.write_bytes(json_bytes({"index": sha(index.read_bytes())[7:], "archive": sha(b"old")[7:]}))
        self.assertEqual(route("stale")["config"], sha(self.package_cfg))
        receipt.write_text("{}")
        self.assertEqual(route("malformed")["original_tags"], ["pkg:original"])

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
    parser.add_argument("--zstd", default="zstd")
    args, remaining = parser.parse_known_args()
    PIGZ, ZSTD = args.pigz, args.zstd
    unittest.main(argv=[sys.argv[0], *remaining])
