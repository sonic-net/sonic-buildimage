"""Check Syncd performance evidence without replacing integrity checks."""

import contextlib
import io
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

from tools.bazel.tests.integration import syncd_p4_images_test as subject


class SyncdValidationTimingTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.inputs = {}
        for name in (*subject.IMAGE_INPUTS, *subject.SOURCE_BASE_INPUTS, "policy",
                     "make_runtime_archive", "make_debug_archive", "readelf", "objcopy"):
            path = self.root / name
            path.write_bytes(b"same input bytes")
            self.inputs[name] = path
        self.arguments = ["syncd_p4_images_test.py"]
        for name, path in self.inputs.items():
            self.arguments.extend(["--" + name.replace("_", "-"), str(path)])
        self.output = self.root / "validation.json"

    @contextlib.contextmanager
    def validators(self):
        with mock.patch.object(sys, "argv", self.arguments), \
                mock.patch.dict(os.environ, TEST_UNDECLARED_OUTPUTS_DIR=str(self.root)), \
                mock.patch.object(subject, "read_policy", return_value={"architecture": "amd64"}), \
                mock.patch.object(subject, "check_p4_inputs", return_value={"p4": True}), \
                mock.patch.object(subject.image_validation, "validate_images", return_value={"images": True}) as images, \
                mock.patch.object(subject.native_packages, "validate_native", return_value={"native": True}) as native, \
                mock.patch.object(subject, "validate_source_base", return_value={"base": True}) as inherited, \
                contextlib.redirect_stdout(io.StringIO()) as logs:
            yield images, native, inherited, logs

    def test_reports_all_phases_and_cache_integrity_work(self):
        with self.validators() as (images, native, inherited, logs):
            subject.main()
        for validator in (images, native, inherited):
            validator.assert_called_once()
        report = json.loads(self.output.read_text())
        self.assertEqual(report["schema"], 1)
        self.assertEqual(report["images"], {"images": True})
        self.assertEqual(report["native"], {"native": True})
        self.assertEqual(report["source_base"], {"base": True})
        performance = report["performance"]
        phases = performance["phase_seconds"]
        self.assertEqual(set(phases), {"input_identity", "images_and_archives",
                                      "native_packages_and_symbols", "inherited_base_and_symbols",
                                      "input_integrity"})
        self.assertTrue(all(seconds >= 0 for seconds in phases.values()))
        self.assertGreaterEqual(performance["validation_seconds"], sum(phases.values()))
        stats = performance["cache"]
        self.assertEqual(stats["hash_reads"], 4)
        self.assertEqual(stats["verification_reads"], 4)
        self.assertEqual(stats["verification_bytes"], len(b"same input bytes") * 4)
        self.assertEqual(phases["input_integrity"], stats["verification_seconds"])
        self.assertIn("Syncd validation performance:", logs.getvalue())

    def test_failed_phase_is_logged_and_no_success_receipt_is_written(self):
        with self.validators() as (_, native, inherited, logs):
            native.side_effect = ValueError("invalid debug symbols")
            with self.assertRaisesRegex(ValueError, "invalid debug symbols"):
                subject.main()
            self.assertIn("native_packages_and_symbols failed", logs.getvalue())
            inherited.assert_not_called()
        self.assertFalse(self.output.exists())

    def test_final_integrity_check_still_rejects_input_mutation(self):
        def mutate(*args, **kwargs):
            self.inputs["make_runtime_archive"].write_bytes(b"modified bytes")
            return {"base": True}

        with self.validators() as (_, _, inherited, _):
            inherited.side_effect = mutate
            with self.assertRaisesRegex(ValueError, "validation input changed"):
                subject.main()
        self.assertFalse(self.output.exists())

    def test_archive_identity_failure_stops_all_image_checks(self):
        self.inputs["make_debug_archive"].write_bytes(b"wrong archive")
        with self.validators() as (images, native, inherited, logs):
            with self.assertRaisesRegex(ValueError, "Make archive differs"):
                subject.main()
            self.assertIn("input_identity failed", logs.getvalue())
            for validator in (images, native, inherited):
                validator.assert_not_called()
        self.assertFalse(self.output.exists())


if __name__ == "__main__":
    unittest.main()
