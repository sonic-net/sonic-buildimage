"""Check inspected OCI layout materialization without running image producers."""

import json
from pathlib import Path
import subprocess
from types import SimpleNamespace
import tempfile
import unittest
from unittest.mock import patch

import yaml

from tools.bazel.ci import snmp_image as image
from tools.bazel.ci import snmp_loaded


class SnmpImageBuildTest(unittest.TestCase):
    def build(self, *, failure=None):
        self.events = []
        self.parent = "//dockers/docker-config-engine-trixie:docker-config-engine-trixie"
        self.layouts = [self.parent, *(image.OWNER + name for name in image.IMAGES)]
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            artifacts = root / "artifacts"
            artifacts.mkdir()
            audited = set()
            materialized = set()

            def audit(bazel, options, targets, **kwargs):
                self.events.append("audit")
                audited.update(targets)
                self.assertEqual(set(targets), set(image.TARGETS.values()) | set(self.layouts))
                if failure == "audit":
                    raise ValueError("audit rejected")
                return {"status": "passed"}

            def collect(workspace, directory, receipt, targets, **kwargs):
                self.events.append("cached-archives")
                self.assertEqual(targets, image.TARGETS)
                self.assertEqual(len(targets), 16)
                receipt["artifacts"] = {}  # Cache hits do not materialize OCI directories.

            def execute(command, directory, receipt, phase, **kwargs):
                self.events.append(phase)
                if phase == "revision":
                    return "a" * 40 + "\n"
                if phase == "make-manifests":
                    return ""
                if phase == "layouts":
                    targets = [arg for arg in command if arg.startswith("//")]
                    self.assertEqual(command[:2], ["bazel", "build"])
                    self.assertEqual(targets, self.layouts)
                    self.assertTrue(set(targets) <= audited)
                    self.assertLess(self.events.index("audit"), self.events.index("cached-archives"))
                    if failure == "build":
                        raise subprocess.CalledProcessError(17, command)
                    for label in targets:
                        path = root / label.split(":", 1)[1]
                        path.mkdir()
                        (path / "oci-layout").write_text('{"imageLayoutVersion":"1.0.0"}')
                        materialized.add(label)
                    return ""
                if command[1] == "cquery":
                    label = command[-1]
                    self.assertIn(label, materialized, "cquery cannot materialize a cached parent layout")
                    return label.split(":", 1)[1] + "\n"
                self.fail("Unexpected command: " + repr(command))

            def headers(command, **kwargs):
                self.assertEqual(command, ["git", "cat-file", "-p", "HEAD"])
                return "tree " + "b" * 40 + "\n\nmessage\n"

            with patch.object(image, "ROOT", root), patch.object(image.platform, "machine", return_value="x86_64"), \
                    patch.object(image.command_log, "execute", side_effect=execute), \
                    patch.object(image.subprocess, "check_output", side_effect=headers), \
                    patch.object(image.bazel_commands, "audit_targets", side_effect=audit), \
                    patch.object(image.build, "collect_archives", side_effect=collect), \
                    patch.object(image.resolution, "collect", return_value={}) as resolution:
                try:
                    image.build_images(SimpleNamespace(bazel="bazel", image_version="bazel-snmp-ci"), artifacts)
                finally:
                    self.receipt = json.loads((artifacts / "build-receipt.json").read_text())
                    if failure:
                        resolution.assert_not_called()
                self.assertTrue((root / self.receipt["base_layout"] / "oci-layout").is_file())
                self.assertEqual(set(self.receipt["layouts"]), set(image.IMAGES))

    def test_shared_archive_job_hands_its_exact_outputs_to_docker(self):
        workflow = yaml.safe_load((image.ROOT / ".github/workflows/bazel-oci.yml").read_text())
        archive = workflow["jobs"]["archive"]["steps"]
        build_step = next(step for step in archive if step.get("id") == "snmp_build")
        self.assertEqual(build_step["if"], "matrix.architecture == 'amd64'")
        self.assertEqual(build_step["run"].count("snmp_image.py build "), 1)
        self.assertIn("snmp_image.py validate ", build_step["run"])
        self.assertIn("snmp_image.py incremental ", build_step["run"])
        upload = next(step for step in archive if step.get("name") == "Retain SNMP archives and build evidence")
        publish = next(step for step in archive if step.get("id") == "snmp_public")
        self.assertIn('mktemp -d "$RUNNER_TEMP/snmp.XXXXXXXX"', build_step["run"])
        self.assertEqual(publish["env"]["snmp_artifacts"], "${{ steps.snmp_build.outputs.directory }}")
        self.assertIn("steps.snmp_build.outputs.directory != ''", publish["if"])
        self.assertEqual(upload["with"]["path"], "${{ steps.snmp_build.outputs.directory }}/public")
        loaded = workflow["jobs"]["snmp_runtime"]
        self.assertEqual(loaded["needs"], "archive")
        download = next(step for step in loaded["steps"] if "actions/download-artifact" in step.get("uses", ""))
        self.assertEqual(upload["with"]["name"], "snmp-images-amd64-${{ github.sha }}")
        self.assertEqual(download["with"]["name"], upload["with"]["name"])
        self.assertTrue(any("snmp_loaded.py" in step.get("run", "") for step in loaded["steps"]))

    def test_incomplete_or_duplicate_image_receipts_stop_before_docker(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            report = directory / "validation.json"
            for images in ([], ["docker-snmp"], ["docker-snmp-dbg", "docker-snmp-dbg"]):
                with self.subTest(images=images):
                    report.write_text(json.dumps({"status": "passed", "architecture": "amd64", "images": images}))
                    with self.assertRaisesRegex(ValueError, "both SNMP"):
                        snmp_loaded.run_loaded(directory)

    def test_cached_archives_materialize_every_audited_layout_before_inspection(self):
        self.build()
        self.assertEqual(self.receipt["status"], "built")
        self.assertLess(self.events.index("layouts"), self.events.index("base.layout"))

    def test_failed_materialization_preserves_failure_and_stops_queries(self):
        with self.assertRaises(subprocess.CalledProcessError) as error:
            self.build(failure="build")
        self.assertEqual(error.exception.returncode, 17)
        self.assertEqual(self.receipt["status"], "failed")
        self.assertNotIn("base.layout", self.events)
        self.assertNotIn("layouts", self.receipt)

    def test_failed_audit_stops_all_producer_requests(self):
        with self.assertRaisesRegex(ValueError, "audit rejected"):
            self.build(failure="audit")
        self.assertEqual(self.receipt["status"], "failed")
        self.assertNotIn("cached-archives", self.events)
        self.assertNotIn("layouts", self.events)


if __name__ == "__main__":
    unittest.main()
