"""Test the installer-selected input graph of the Bazel boot bundle."""
from contextlib import chdir
import errno
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))

from tools.bazel.image import build_boot_bundle as bridge
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
        self.assertTrue((inputs / "archive").samefile(archive))
        self.assertFalse((inputs / "archive").is_symlink())
        self.assertTrue((inputs / "receipt").samefile(target / ".boot-image-layouts/docker-a.gz.json"))
        self.assertTrue((inputs / "index").samefile(selected.layout / "index.json"))

        (selected.layout / "blobs/sha256" / digest(self.layers[0])[7:]).unlink()
        selected = Image(export, ["docker-a:latest"], False)
        stage = self.work / "second"
        write_archive_package(stage, [selected], PLATFORM)
        self.assertIsNone(selected.layout)
        self.assertTrue((stage / "inputs" / selected.archive / "archive").samefile(archive))
        self.assertNotIn("receipt =", (stage / "BUILD.bazel").read_text())

    def test_injected_repository_is_private_and_visible_from_the_checkout(self):
        workspace = self.work / "checkout"
        workspace.mkdir()
        archive = self.work / "extra.tar"
        docker_save(archive, self.config, self.layers)
        output = self.work / "out"
        result = self.work / "result"
        result.write_bytes(b"built")
        selected = [Image(self.layout, ["native:v1"], False), Image(archive, ["extra:v1"], False)]
        phase = []

        def verify_graph(repository, package, target, expected, evidence, options):
            self.assertEqual(repository.parents[1], workspace / "target")
            self.assertEqual(repository.parent.stat().st_mode & 0o777, 0o700)
            self.assertFalse(any(p.is_symlink() for p in repository.rglob("*")))
            phase.append(package)
            if package == "archives":
                # Simulate the archive action's metadata. Source files are already
                # present in the repository even if the original /tmp is hidden.
                self.assertTrue(any(p.samefile(archive) for p in (repository / "archives/inputs").rglob("archive")))
                native = validate_layout(self.layout, PLATFORM)
                result_info = self.work / "archive.json"
                result_info.write_text(json.dumps({"use_sidecar": False, "config": digest(self.config),
                    "diff_ids": native.config["rootfs"]["diff_ids"], "layers": native.manifest["layers"],
                    "original_tags": []}))
                return {name: result_info for name in expected}
            self.assertEqual(package, "bundle")
            self.assertEqual(len(list((repository / "bundle/blobs").iterdir())), len(self.layers))
            return {name: result for name in expected}

        with chdir(workspace), mock.patch.object(bridge, "build_target", side_effect=verify_graph):
            bridge.build(selected, PLATFORM, output, None, None)
        self.assertEqual(phase, ["archives", "bundle"])
        self.assertEqual(output.read_bytes(), b"built")
        self.assertEqual(list((workspace / "target").iterdir()), [])

    def test_cross_device_staging_copies_bytes_and_removes_failed_partial(self):
        source = self.work / "real-source"
        source.write_bytes(b"content")
        alias = self.work / "logical-source"
        alias.symlink_to(source)
        repo = self.work / "stage"
        with mock.patch.object(os, "link", side_effect=OSError(errno.EXDEV, "cross-device")):
            bridge.link_input(repo, "inputs/archive", alias)
            with mock.patch.object(shutil, "copyfileobj", side_effect=OSError(errno.ENOSPC, "disk full")):
                with self.assertRaises(OSError) as failure:
                    bridge.link_input(repo, "inputs/receipt", alias)
                self.assertEqual(failure.exception.errno, errno.ENOSPC)
        staged = repo / "inputs/archive"
        self.assertFalse(staged.is_symlink())
        self.assertFalse(staged.samefile(source))
        self.assertEqual(staged.read_bytes(), source.read_bytes())
        self.assertEqual(list(staged.parent.iterdir()), [staged])

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
        expected = [d / "blobs/sha256" / digest(layer)[7:]
                    for d in [self.layout, separate] for layer in self.layers]
        self.assertTrue(all(any(p.samefile(original) for p in sources) for original in expected))
        self.assertFalse(any(p.is_symlink() for p in sources))
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
