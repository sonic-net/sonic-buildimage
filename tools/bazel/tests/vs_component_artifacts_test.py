"""Prove component selection, artifact identity and disabled-debug behavior for VS builds."""

import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from tools.bazel.ci import vs_component_artifacts as components


class ComponentHandoffTest(unittest.TestCase):
    def test_two_images_share_one_configured_component(self):
        module = SimpleNamespace(IMAGES=("a", "b"), RUNTIME={"runtime.tar": "//c:runtime"},
                                 DEBUG={"symbols.tar": "//c:symbols"})
        families = {"a": {"members": {"runtime": ()}}, "b": {"members": {"runtime": ()}}}
        revision = "f" * 40
        with tempfile.TemporaryDirectory() as scratch, patch.object(components, "components", return_value={"example": module}):
            root = Path(scratch)
            payload = root / "input"
            payload.write_bytes(b"VS package")
            directory = root / "components"
            directory.mkdir()
            self.assertEqual(components.targets(families), {"//c:runtime"})
            components.publish(directory, families, {"//c:runtime": payload}, revision, "amd64",
                               {"//tools/bazel/oci:image_version": "test"})
            bundle = directory / "example"
            self.assertIs(components.verify_inputs("example", bundle, revision, False), module)
            (root / "selection.json").write_text(json.dumps({"components": ["example"]}))
            steps = {"component_example": {"outputs": {"artifact-id": "10"}}}
            self.assertEqual(components.matrix(directory, steps)["include"],
                             [{"component": "example", "artifact_id": "10", "debug": "n"}])
            with self.assertRaises(ValueError):
                components.verify_inputs("example", bundle, "e" * 40, False)
            with self.assertRaises(ValueError):
                components.verify_inputs("example", bundle, revision, True)
            (bundle / "runtime.tar").write_bytes(b"substitute")
            with self.assertRaises(ValueError):
                components.verify_inputs("example", bundle, revision, False)
            families["b"]["members"]["debug"] = ()
            self.assertEqual(components.targets(families), {"//c:runtime", "//c:symbols"})

    def test_all_configured_component_uploads_precede_matrix_selection(self):
        """The matrix can reference an artifact ID only after its upload has completed."""
        import yaml

        workflow = Path(__file__).resolve().parents[3] / ".github/workflows/bazel-oci.yml"
        producer = yaml.safe_load(workflow.read_text())["jobs"]["vs_container"]
        steps = producer["steps"]
        matrix_index = next(i for i, step in enumerate(steps) if step.get("id") == "component_matrix")
        uploads = {step["id"].removeprefix("component_"): step for step in steps[:matrix_index]
                   if step.get("id", "").startswith("component_")}
        self.assertEqual(set(uploads), set(components.components()))
        for name, step in uploads.items():
            with self.subTest(component=name):
                self.assertIn("upload-artifact@", step["uses"])
                self.assertTrue(step["with"]["path"].endswith("/components/" + name + "/"))
                if getattr(components.components()[name], "REUSABLE_WORKFLOW", False):
                    self.assertEqual(producer["outputs"][name + "_artifact_id"],
                                     "${{ steps.component_" + name + ".outputs.artifact-id }}")
