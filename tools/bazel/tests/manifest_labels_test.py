#!/usr/bin/env python3
"""Check lossless conversion of Make JSON into an OCI label."""

import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.oci import manifest_labels


class ManifestLabelsTest(unittest.TestCase):
    def test_label_preserves_nested_values_and_unknown_fields(self):
        """Keep all Make manifest values when packaging the OCI label."""
        manifest = {
            "package": {"name": 'quoted "name"', "version": "1.0.0+custom.dbg"},
            "custom-field": {"number": 42, "enabled": True, "missing": None},
            "description": "line1\nline2\t café \\ end = $HOME `literal`",
            "values": ["a b", "c"],
        }
        label = manifest_labels.manifest_label(json.dumps(manifest, indent=2))
        self.assertEqual(len(label.splitlines()), 1)
        key, value = label.rstrip("\n").split("=", 1)
        self.assertEqual(key, "com.azure.sonic.manifest")
        self.assertEqual(json.loads(value), manifest)
        self.assertTrue(label.endswith("\n"))

    def test_image_version_sets_make_tag_including_an_empty_version(self):
        for version in ("", "SONiC.HEAD.12-34", "test=value"):
            with self.subTest(version=version):
                self.assertEqual(
                    manifest_labels.manifest_label("{}", version),
                    "com.azure.sonic.manifest={}\nTag=" + version + "\n",
                )

    def test_component_labels_use_only_requested_installed_versions(self):
        packages = {
            name: {"Package": name, "Version": version}
            for name, version in (("libswsscommon", "1.2.3"), ("other", "2:1.0-1"))
        }
        receipt = json.dumps({"dependency_check": {"packages": packages}})
        self.assertEqual(manifest_labels.component_labels(receipt, ["libswsscommon"]),
                         "com.azure.sonic.versions.libswsscommon=1.2.3\n")
        with self.assertRaisesRegex(ValueError, "unknown"):
            manifest_labels.component_labels(receipt, ["unknown"])

    def test_multiline_image_version_is_rejected(self):
        for version in ("a\nb", "a\rb"):
            with self.subTest(version=version), self.assertRaisesRegex(ValueError, "one line"):
                manifest_labels.manifest_label("{}", version)

    def test_json_formatting_does_not_change_label(self):
        """Avoid changing image labels when only JSON whitespace changes."""
        manifest = {"package": {"name": "swss", "version": "1.0.0"}}
        self.assertEqual(
            manifest_labels.manifest_label(json.dumps(manifest, indent=4)),
            manifest_labels.manifest_label(json.dumps(manifest)),
        )

    def test_malformed_json_is_rejected(self):
        """Stop invalid Make output before it becomes an image label."""
        for source in ('{"name": }', '{"name": "swss"} trailing', ''):
            with self.subTest(source=source), self.assertRaises(json.JSONDecodeError):
                manifest_labels.manifest_label(source)

    def test_manifest_must_be_a_json_object(self):
        """Require the object format consumed by SONiC image tooling."""
        for source in ('[]', 'null', '"text"', '0', 'true'):
            with self.subTest(source=source), self.assertRaisesRegex(ValueError, "JSON object"):
                manifest_labels.manifest_label(source)

    def test_non_json_numbers_are_rejected(self):
        """Reject Python numeric extensions that JSON readers cannot accept."""
        for value in ('NaN', 'Infinity', '-Infinity'):
            with self.subTest(value=value), self.assertRaisesRegex(ValueError, "non-JSON number"):
                manifest_labels.manifest_label('{"value": ' + value + '}')

    def test_number_overflow_does_not_emit_non_json_infinity(self):
        """Prevent a large JSON number from becoming invalid Infinity output."""
        with self.assertRaises(ValueError):
            manifest_labels.manifest_label('{"value": 1e400}')


if __name__ == "__main__":
    unittest.main()
