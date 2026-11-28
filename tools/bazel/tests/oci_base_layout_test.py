#!/usr/bin/env python3
"""Validate temporary OCI layouts before they become Bazel image bases.

Small config and layer fixtures exercise platform, descriptor and blob checks
without loading an image into a container runtime."""

import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.tests.oci_base_fixture import digest, image_fixture, oci_files, write_layout
from tools.bazel.oci.oci_layout import validate_layout


class LayoutTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.layout = Path(temporary.name)

    def prepare(self, architecture="amd64", os_name="linux", **kwargs):
        config, layers = image_fixture(architecture, os_name)
        files = oci_files(config, layers, **kwargs)
        write_layout(self.layout, files)
        return files

    def test_valid_layout_is_not_modified(self):
        """Accept a complete Linux AMD64 base without rewriting any layout files."""
        files = self.prepare()
        image = validate_layout(self.layout, "linux/amd64")
        self.assertEqual(image.descriptor, json.loads(files["index.json"])["manifests"][0])
        self.assertEqual(image.config, json.loads(image_fixture()[0]))
        self.assertEqual([path.read_bytes() for path in image.layers], image_fixture()[1])
        self.assertEqual(files, {str(path.relative_to(self.layout)): path.read_bytes()
                                 for path in self.layout.rglob("*") if path.is_file()})

    def test_actual_config_and_descriptor_platform_must_both_match(self):
        """Reject platform mismatches in either the index descriptor or the referenced config."""
        for os_name, config_arch, descriptor_arch in (
            ("linux", "arm64", "amd64"),
            ("linux", "amd64", "arm64"),
            ("windows", "amd64", "amd64"),
        ):
            with self.subTest(os=os_name, config=config_arch, descriptor=descriptor_arch):
                self.prepare(config_arch, os_name, descriptor_platform={"os": "linux", "architecture": descriptor_arch})
                with self.assertRaisesRegex(ValueError, "platform"):
                    validate_layout(self.layout, "linux/amd64")

    def test_missing_config_platform_fails_closed(self):
        """Require platform fields in the config even when the index claims the expected platform."""
        for missing in ("os", "architecture"):
            with self.subTest(missing=missing):
                config, layers = image_fixture()
                parsed = json.loads(config)
                del parsed[missing]
                files = oci_files(json.dumps(parsed).encode(), layers,
                                  descriptor_platform={"os": "linux", "architecture": "amd64"})
                write_layout(self.layout, files)
                with self.assertRaisesRegex(ValueError, "platform"):
                    validate_layout(self.layout, "linux/amd64")

    def test_multiple_images_are_rejected(self):
        """Reject ambiguous indexes instead of silently selecting one of several images."""
        self.prepare()
        index_path = self.layout / "index.json"
        index = json.loads(index_path.read_bytes())
        index["manifests"] *= 2
        index_path.write_text(json.dumps(index))
        with self.assertRaisesRegex(ValueError, "single OCI image"):
            validate_layout(self.layout, "linux/amd64")

    def test_corrupt_and_missing_layer_fail(self):
        """Stop consumers from accepting a base with altered or unavailable layer bytes."""
        for missing in (False, True):
            with self.subTest(missing=missing):
                self.prepare()
                _, layers = image_fixture()
                path = self.layout / "blobs" / digest(layers[0]).replace(":", "/")
                if missing:
                    path.unlink()
                else:
                    path.write_bytes(b"corrupt layer")
                with self.assertRaises((ValueError, FileNotFoundError)):
                    validate_layout(self.layout, "linux/amd64")

    def test_bad_descriptor_cannot_escape_layout(self):
        """Reject a traversal-shaped digest before it can reference a file outside the layout."""
        self.prepare()
        path = self.layout / "index.json"
        index = json.loads(path.read_bytes())
        index["manifests"][0]["digest"] = "sha256:../../host-file"
        path.write_text(json.dumps(index))
        with self.assertRaisesRegex(ValueError, "blob reference"):
            validate_layout(self.layout, "linux/amd64")

    def test_wrong_blob_size_fails(self):
        """Require the declared manifest size to match its stored bytes."""
        self.prepare()
        path = self.layout / "index.json"
        index = json.loads(path.read_bytes())
        index["manifests"][0]["size"] += 1
        path.write_text(json.dumps(index))
        with self.assertRaisesRegex(ValueError, "size mismatch"):
            validate_layout(self.layout, "linux/amd64")

    def test_invalid_metadata_is_reported_as_value_error(self):
        """Report wrong-shaped layout and index JSON as validation errors."""
        for name, contents in (("oci-layout", b"null"), ("index.json", b"[]")):
            with self.subTest(name=name):
                self.prepare()
                (self.layout / name).write_bytes(contents)
                with self.assertRaises(ValueError):
                    validate_layout(self.layout, "linux/amd64")


if __name__ == "__main__":
    unittest.main()
