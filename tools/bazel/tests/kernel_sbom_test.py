#!/usr/bin/env python3
"""Exercise Debian kernel inventory against real installed fixture files."""

from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / "scripts"))
from tools.bazel.tests import kernel_test
from tools.bazel.kernel import prepare as kernel
import build_sbom
import sbom_kernel_deb


@contextmanager
def chdir(path):
    previous = Path.cwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(previous)


class KernelSbomTest(unittest.TestCase):
    def setUp(self):
        self.fixture = kernel_test.KernelBundleTest()
        self.fixture.setUp()
        self.addCleanup(self.fixture.doCleanups)
        self.abi = self.fixture.config["kernel_abi"]
        self.copyright = "usr/share/doc/linux-image-" + self.abi + "-unsigned/copyright"
        self.runtime = next(item for item in self.fixture.manifest["packages"] if item["kind"] == "runtime")
        self.rootfs = self.fixture.root / "fsroot-vs"
        self.rootfs.mkdir()
        # A real fixture DEB is unpacked and configured by dpkg, with an isolated
        # dpkg database; production smoke additionally exercises actual hooks.
        subprocess.run(["dpkg", "--force-not-root", "--force-architecture", "--log=" + str(self.rootfs / "dpkg.log"),
                        "--root=" + str(self.rootfs), "--install",
                        str(self.fixture.bundle / self.runtime["name"])], check=True, stdout=subprocess.DEVNULL)

    def component(self):
        return sbom_kernel_deb.component(self.fixture.bundle, self.fixture.root, self.rootfs)

    def test_installed_deb_has_truthful_identity_and_kernel_edges(self):
        unrelated = self.fixture.bundle / "unrelated_1_all.deb"
        unrelated.write_bytes(b"another package")
        before = unrelated.stat().st_mtime_ns
        component = self.component()
        self.assertEqual(unrelated.read_bytes(), b"another package")
        self.assertEqual(unrelated.stat().st_mtime_ns, before)
        props = {item["name"]: item["value"] for item in component["properties"]}
        self.assertTrue(component["purl"].startswith("pkg:deb/sonic/linux-image-"))
        self.assertEqual(props["sonic:scope"], "host-image")
        self.assertEqual(props["sonic:artifact_format"], "deb")
        self.assertEqual(props["sonic:kernel_abi"], self.abi)
        self.assertEqual(props["sonic:submodule_commit"], self.fixture.gitlink)
        self.assertEqual(props["sonic:installed_vmlinuz_sha256"],
                         hashlib.sha256(b"kernel fixture").hexdigest())
        ancestor = component["pedigree"]["ancestors"][0]
        self.assertEqual(ancestor["purl"], "pkg:deb/debian/linux@6.12.41-1?arch=source")
        self.assertEqual(len(ancestor["externalReferences"]), 3)
        self.assertEqual(component["externalReferences"][1]["hashes"][0]["content"],
                         kernel.sha256(self.rootfs / self.copyright))
        module = {"bom-ref": "fixture-vs-module", "type": "library", "name": "sonic-platform-modules-vs",
                  "properties": [{"name": "sonic:scope", "value": "host-image"}]}
        host = build_sbom.host_image_component("vs")
        edges = build_sbom.build_dependency_graph([component, module, host], root_ref="image")
        graph = {item["ref"]: item["dependsOn"] for item in edges}
        self.assertIn(component["bom-ref"], graph[module["bom-ref"]])
        self.assertIn(component["bom-ref"], graph[host["bom-ref"]])
        self.assertTrue((self.rootfs / "var/lib/dpkg/status").is_file())

    def test_rejects_missing_installed_package_record(self):
        (self.rootfs / "var/lib/dpkg/status").write_text("")
        with self.assertRaises(subprocess.CalledProcessError):
            self.component()

    def test_rejects_tampered_installed_kernel_and_modules(self):
        for name in ("boot/vmlinuz-" + self.abi, "usr/lib/modules/" + self.abi + "/kernel/test.ko"):
            with self.subTest(path=name):
                path = self.rootfs / name
                previous = path.read_bytes()
                path.write_bytes(b"x" * len(previous))
                with self.assertRaisesRegex(ValueError, "installed bytes differ"):
                    self.component()
                path.write_bytes(previous)

    def test_rejects_missing_or_changed_copyright(self):
        (self.rootfs / self.copyright).unlink()
        with self.assertRaises(FileNotFoundError):
            self.component()

    def test_rejects_missing_or_tampered_bundle(self):
        runtime = self.fixture.bundle / self.runtime["name"]
        original = runtime.read_bytes()
        runtime.write_bytes(b"x" * len(original))
        with self.assertRaisesRegex(ValueError, "SHA256"):
            self.component()
        runtime.unlink()
        with self.assertRaisesRegex(ValueError, "missing"):
            self.component()

    def test_rejects_installed_symlink_escape(self):
        image = self.rootfs / ("boot/vmlinuz-" + self.abi)
        copy = self.fixture.root / "outside-kernel"
        shutil.copyfile(image, copy)
        image.unlink()
        image.symlink_to(copy)
        with self.assertRaisesRegex(ValueError, "redirected"):
            self.component()

    def test_records_only_applied_sonic_patches_and_their_cves(self):
        patch_dir = self.fixture.source / "patches-sonic"
        applied = patch_dir / "CVE-2026-12345-fix.patch"
        applied.write_text("Subject: Fix CVE-2026-12345\n\n--- a/file\n+++ b/file\n")
        (patch_dir / "series").write_text(applied.name + "\n")
        (patch_dir / "unused.patch").write_text("not applied\n")
        manifest = {"source_files": [{"name": "patches-sonic/" + applied.name, "sha256": kernel.sha256(applied)}]}
        entries, properties = sbom_kernel_deb._patches(self.fixture.root, manifest, self.fixture.gitlink)
        self.assertEqual(len(entries), 1)
        self.assertEqual(entries[0]["resolves"][0]["id"], "CVE-2026-12345")
        self.assertEqual(entries[0]["diff"]["url"], sbom_kernel_deb.SOURCE_URL + "/blob/" +
                         self.fixture.gitlink + "/patches-sonic/" + applied.name)
        self.assertIn(kernel.sha256(applied), properties[0]["value"])
        relocated = self.fixture.source / "patches-debian/bazel-relocatable-tools.patch"
        relocated.write_text("Subject: Use declared kernel tools\n\n--- a/debian/rules\n+++ b/debian/rules\n")
        manifest["source_files"].append({"name": "patches-debian/" + relocated.name,
                                        "sha256": kernel.sha256(relocated)})
        entries, properties = sbom_kernel_deb._patches(self.fixture.root, manifest, self.fixture.gitlink)
        self.assertEqual(len(entries), 2)
        self.assertTrue(any(entry["diff"]["url"].endswith("/patches-debian/" + relocated.name)
                            for entry in entries))
        applied.write_text("changed\n")
        with self.assertRaisesRegex(ValueError, "compiled source inventory"):
            sbom_kernel_deb._patches(self.fixture.root, manifest, self.fixture.gitlink)

    def test_final_aggregator_replaces_stale_deb_and_rejects_missing_bundle(self):
        target = self.fixture.root / "target"
        stale = {
            "bom-ref": "pkg:deb/sonic/old-kernel@6.12.41-1",
            "name": "linux-image-" + self.abi + "-unsigned", "version": "6.12.41-1",
            "type": "library",
            "properties": [{"name": "sonic:src_path", "value": "src/sonic-linux-kernel"},
                           {"name": "sonic:artifact_filename", "value": "old-kernel.deb"}],
        }
        (target / "old-kernel.deb.cdx.json").write_text(json.dumps({
            "metadata": {"properties": [{"name": "sonic:fragment_kind", "value": "recipe-emit"}]},
            "components": [stale],
        }))
        environment = {
            "ENABLE_SBOM": "y", "SBOM_STRICT": "y", "SBOM_SCAN_TOOL": "none",
            "SBOM_INCLUDE_LICENSES": "n", "SBOM_FORMAT": "cyclonedx",
            "TARGET_PATH": "target", "TARGET_MACHINE": "vs", "CONFIGURED_ARCH": "amd64",
            "CONFIGURED_PLATFORM": "vs", "BUILD_WITH_BAZEL_WHEN_AVAILABLE": "y",
            "IMAGE_DISTRO_DEBS_PATH": str(self.fixture.bundle),
        }
        with chdir(self.fixture.root), patch.dict(os.environ, environment, clear=True), \
                patch.object(sys, "argv", ["build_sbom.py"]), \
                patch.object(build_sbom, "validate_document"), patch.object(build_sbom, "emit_provenance"):
            self.assertEqual(build_sbom.main(), 0)
            document = json.loads((target / "sonic-vs.bin.cdx.json").read_text())
            kernels = [c for c in document["components"] if c["name"].startswith("linux-image-")]
            self.assertEqual(len(kernels), 1)
            self.assertTrue(kernels[0]["purl"].startswith("pkg:deb/sonic/"))
            self.assertNotIn(stale["bom-ref"], {c["bom-ref"] for c in document["components"]})
            (self.fixture.bundle / self.runtime["name"]).unlink()
            self.assertEqual(build_sbom.main(), 1)


if __name__ == "__main__":
    unittest.main()
