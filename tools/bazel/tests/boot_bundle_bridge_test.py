"""Test the installer-selected input graph of the Bazel boot bundle."""
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))

from tools.bazel.image.build_boot_bundle import BundlePackage, Image, write_archive_package
from tools.bazel.oci.boot_layout import publish_layout
from tools.bazel.oci.oci_layout import validate_layout
from tools.bazel.tests.oci_base_fixture import digest, docker_save, image_fixture, oci_files, write_layout

PLATFORM = "linux/amd64"


class BootBundleBridgeTest(unittest.TestCase):
    def setUp(self):
        work = tempfile.TemporaryDirectory()
        self.addCleanup(work.cleanup)
        self.work = Path(work.name)
        self.config, self.layers = image_fixture()
        self.layout = self.work / "native"
        write_layout(self.layout, oci_files(self.config, self.layers))

    def test_make_symlink_receipt_and_pruned_sidecar_fallback(self):
        archive = self.work / "cached-export.gz"
        docker_save(archive, self.config, self.layers)
        target = self.work / "target"
        target.mkdir()
        export = target / "docker-a.gz"
        export.symlink_to(archive)
        publish_layout(self.layout, archive, export)
        selected = Image(export, ["docker-a:latest"], False)
        stage = self.work / "first"
        write_archive_package(stage, [selected], PLATFORM)
        inputs = stage / "inputs" / selected.archive
        self.assertEqual((inputs / "archive").resolve(), archive)
        self.assertEqual((inputs / "receipt").resolve(), target / ".boot-image-layouts/docker-a.gz.json")
        self.assertEqual((inputs / "index").resolve(), selected.layout / "index.json")

        (selected.layout / "blobs/sha256" / digest(self.layers[0])[7:]).unlink()
        selected = Image(export, ["docker-a:latest"], False)
        stage = self.work / "second"
        write_archive_package(stage, [selected], PLATFORM)
        self.assertIsNone(selected.layout)
        self.assertEqual((stage / "inputs" / selected.archive / "archive").resolve(), archive)
        self.assertNotIn("receipt =", (stage / "BUILD.bazel").read_text())

    def test_archive_target_survives_new_tags_extra_images_and_new_checkout(self):
        def graph(directory, names):
            root = self.work / directory
            root.mkdir()
            images = [Image(root / name, [tag], False) for name, tag in names]
            for image in images:
                image.source.write_bytes(b"export")
            stage = root / "graph"
            write_archive_package(stage, images, PLATFORM)
            return images[-1].archive, sorted(p.name for p in (stage / "inputs").iterdir())

        first, _ = graph("checkout-a", [("docker-z.gz", "z:old")])
        second, all_archives = graph("checkout-b", [("optional.tar", "a:latest"), ("docker-z.gz", "z:new")])
        self.assertEqual(first, second)
        self.assertIn(first, all_archives)
        self.assertEqual(len(all_archives), 2)

    def test_independent_files_claiming_same_digest_each_get_a_verification_action(self):
        separate = self.work / "separate"
        shutil.copytree(self.layout, separate)
        # The importer checks metadata eagerly; each per-layer Bazel action must
        # key on and verify the original bytes, even for equal digest claims.
        path = separate / "blobs/sha256" / digest(self.layers[0])[7:]
        path.write_bytes(b"x" * len(self.layers[0]))
        with self.assertRaisesRegex(ValueError, "digest or size mismatch"):
            validate_layout(separate, PLATFORM)

        package = BundlePackage(self.work / "bundle")
        images = [Image(directory, [tag], False, layout=directory)
                  for directory, tag in ((self.layout, "pkg:a"), (separate, "pkg:b"))]
        package.select(images, PLATFORM)
        package.finish(PLATFORM, False)
        self.assertEqual(len(package.layer_targets), 2 * len(self.layers))
        sources = {package.directory / src for _, src in package.layer_targets}
        self.assertEqual({p.resolve() for p in sources}, {
            d / "blobs/sha256" / digest(layer)[7:] for d in [self.layout, separate] for layer in self.layers
        })
        self.assertFalse(any(i["check_label"] for i in package.spec))

    def test_fallback_projection_and_version_inventory(self):
        native = Image(self.layout, ["docker-a:latest", "docker-a:release"], False, layout=self.layout)
        info = validate_layout(self.layout, PLATFORM)
        legacy = Image(self.work / "legacy.gz", ["legacy:release"], False, archive="legacy", info={
            "config": digest(self.config), "original_tags": ["legacy:embedded"],
            "diff_ids": info.config["rootfs"]["diff_ids"], "layers": info.manifest["layers"],
        })
        package = BundlePackage(self.work / "bundle")
        package.select([native, legacy], PLATFORM)
        package.finish(PLATFORM, True)
        self.assertEqual(len(package.layer_targets), len(self.layers))
        # The declared archive action validated and sized its layers. Its JSON
        # remains an input even when a native layer supplies their identical bytes.
        self.assertEqual(package.members["legacy"], {"legacy_files/configs/" + digest(self.config)[7:]})
        self.assertIn("//archives:metadata", package.metadata)
        self.assertEqual(json.loads((package.directory / "images.json").read_bytes())["images"], package.spec)
        self.assertEqual(package.spec[1]["original_tags"], ["legacy:embedded"])

    def test_installer_chooses_packager_and_forwards_every_selection(self):
        template = (ROOT / "files/build_templates/sonic_debian_extension.j2").read_text()
        body = template[template.index("    build_boot_bundle() {"):].split("    build_boot_bundle\n", 1)[0]
        for bazel, expected in (("y", "build_boot_bundle.py"), ("n", "boot_image_bundle.py")):
            with self.subTest(bazel=bazel):
                script = '''set -eu
python3() { printf '%s\\n' "$@"; }
CONFIGURED_ARCH=amd64
FILESYSTEM_DOCKERFS=out.gz
boot_work=tmp
BUILD_WITH_BAZEL_WHEN_AVAILABLE=$1
boot_images=(--image app.gz "app:latest,app:test" --package-image "extra file.tar" "extra:tag")
''' + body + "\nbuild_boot_bundle"
                actual = subprocess.check_output(["bash", "-c", script, "fixture", bazel], text=True).splitlines()
                self.assertEqual(actual, ["tools/bazel/image/" + expected, "--platform", PLATFORM,
                    "--output", "out.gz", "--inventory", "tmp/inventory.json", "--labels", "tmp/labels.json",
                    "--image", "app.gz", "app:latest,app:test", "--package-image", "extra file.tar", "extra:tag"])


if __name__ == "__main__":
    unittest.main()
