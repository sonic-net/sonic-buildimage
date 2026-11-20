"""Contracts from generate_manifest/manifest.json.j2 and installed_packages receipts."""

import argparse
import json
from pathlib import Path
import sys
import unittest

from generate import render

parser = argparse.ArgumentParser()
for field in ("runtime", "debug", "environment", "version", "components", "fallback-labels", "explicit-labels"):
    parser.add_argument("--" + field, type=Path, required=True)
INPUTS, remaining = parser.parse_known_args()


class MetadataTest(unittest.TestCase):
    def test_database_identity_and_service_from_make(self):
        # rules/docker-database.mk and the original common manifest template.
        manifest = json.loads(INPUTS.runtime.read_text())
        self.assertEqual(manifest["version"], "1.0.0")
        self.assertEqual(manifest["package"], {"name": "database", "version": "1.0.0", "depends": []})
        self.assertEqual(manifest["service"]["name"], "database")
        self.assertTrue(manifest["service"]["asic-service"])
        self.assertTrue(manifest["service"]["host-service"])

    def test_debug_suffix_preserves_manifest(self):
        expected = json.loads(INPUTS.runtime.read_text())
        expected["package"]["version"] += "+dbg"
        self.assertEqual(json.loads(INPUTS.debug.read_text()), expected)

    def test_template_suffix_with_existing_build_metadata(self):
        # The template uses '.' when '+' is already present. Unknown fields
        # survive; the source object must remain usable for the runtime variant.
        original = {"package": {"version": "9.2+vendor"}, "custom": {"enabled": True}}
        spec = {"schema": 1, "containers": {"unrelated": {"manifest": original}}}
        debug, env = render(spec, "unrelated", "custom-image", True)
        self.assertEqual(debug, {"package": {"version": "9.2+vendor.dbg"}, "custom": {"enabled": True}})
        self.assertEqual(original["package"]["version"], "9.2+vendor")
        self.assertEqual(env, "DEBIAN_FRONTEND=noninteractive\nIMAGE_VERSION=custom-image\n")

    def test_version_environment_and_label_selection(self):
        version = INPUTS.version.read_text().strip()
        # rules/config and docker-database/bazel/prepare_environment.py.
        self.assertEqual(INPUTS.environment.read_text(), "DEBIAN_FRONTEND=noninteractive\nINCLUDE_SYSTEM_EVENTD=y\n")
        for path, expected in ((INPUTS.fallback_labels, version), (INPUTS.explicit_labels, "fixture-explicit")):
            labels = dict(line.split("=", 1) for line in path.read_text().splitlines())
            self.assertEqual(labels["Tag"], expected)
            self.assertEqual(json.loads(labels["com.azure.sonic.manifest"]), json.loads(INPUTS.runtime.read_text()))

    def test_component_version_comes_from_installed_receipt(self):
        self.assertEqual(INPUTS.components.read_text(), "com.azure.sonic.versions.libswsscommon=1.0.0+fixture\n")

    def test_environment_rejects_shell_substitution_or_multiple_lines(self):
        spec = {"schema": 1, "containers": {"input": {"manifest": {}}}}
        for value in ("", "x\ny", "x\ry", "x\0y", "$(id)"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                render(spec, "input", value)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0], *remaining])
