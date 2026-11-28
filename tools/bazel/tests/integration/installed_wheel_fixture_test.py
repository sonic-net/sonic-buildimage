"""Check wheel-test isolation against checkout imports and ambient Python paths."""

import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
import zipfile

from tools.bazel.tests.integration.installed_wheel_fixture import installed_wheels, write_wheel_receipt


def sample_wheel(directory):
    """Create a minimal Python wheel; no compiler or package builder runs."""
    wheel = directory / "fixture_component-1.0-py3-none-any.whl"
    records = {
        "fixture_component/__init__.py": "ORIGIN = 'installed wheel'\n",
        "fixture_component-1.0.dist-info/METADATA":
            "Metadata-Version: 2.1\nName: fixture-component\nVersion: 1.0\n",
        "fixture_component-1.0.dist-info/WHEEL":
            "Wheel-Version: 1.0\nGenerator: isolation-test\nRoot-Is-Purelib: true\nTag: py3-none-any\n",
    }
    records["fixture_component-1.0.dist-info/RECORD"] = "".join(
        name + ",,\n" for name in [*records, "fixture_component-1.0.dist-info/RECORD"])
    with zipfile.ZipFile(wheel, "w") as archive:
        for name, content in records.items():
            archive.writestr(name, content)
    return wheel


class InstalledWheelFixtureTest(unittest.TestCase):
    def test_owner_sources_are_hidden_while_declared_external_dependencies_work(self):
        """A successful import must come from the wheel, never the owner's checkout."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            checkout, external = root / "checkout", root / "external"
            checkout.mkdir()
            external.mkdir()
            (checkout / "fixture_component.py").write_text("ORIGIN = 'checkout'\n")
            (checkout / "checkout_only.py").write_text("ORIGIN = 'must not import'\n")
            (external / "declared_dependency.py").write_text("VALUE = 'declared dependency'\n")
            wheel = sample_wheel(root)
            with mock.patch.object(sys, "path", [str(checkout), str(external), *sys.path]), \
                    mock.patch.dict(os.environ, {"TEST_TMPDIR": temporary,
                                                 "PYTHONPATH": str(checkout), "PYTHONHOME": str(checkout)}):
                with installed_wheels([wheel], exclude_source_roots=[checkout]) as installed:
                    self.assertNotIn("PYTHONPATH", installed.env)
                    self.assertNotIn("PYTHONHOME", installed.env)
                    subprocess.run([str(installed.python), "-I", "-c", """
import importlib.util, pathlib, sys
import fixture_component, declared_dependency
assert fixture_component.ORIGIN == 'installed wheel'
assert pathlib.Path(fixture_component.__file__).is_relative_to(pathlib.Path(sys.argv[1]))
assert importlib.util.find_spec('checkout_only') is None
assert declared_dependency.VALUE == 'declared dependency'
""", str(installed.site)], cwd=installed.root, env=installed.env, check=True)
                self.assertEqual(os.environ["PYTHONPATH"], str(checkout))
                self.assertFalse(installed.root.exists())

    def test_receipt_names_exact_tested_wheel_bytes(self):
        """Publication can reject replacement wheel bytes using the test receipt."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            wheel = sample_wheel(root)
            with mock.patch.dict(os.environ, {"TEST_UNDECLARED_OUTPUTS_DIR": str(root / "outputs")}):
                write_wheel_receipt([wheel], required=True)
            receipt = json.loads((root / "outputs/wheels.json").read_text())
            self.assertEqual(receipt["status"], "passed")
            self.assertEqual(receipt["wheels"], [{
                "filename": wheel.name, "sha256": hashlib.sha256(wheel.read_bytes()).hexdigest(),
            }])
            wheel.write_bytes(b"different bytes")
            self.assertNotEqual(receipt["wheels"][0]["sha256"], hashlib.sha256(wheel.read_bytes()).hexdigest())

    def test_required_receipt_cannot_be_silently_omitted(self):
        """Keep the stricter YANG receipt contract and optional standalone checks."""
        with mock.patch.dict(os.environ, {}, clear=True):
            with self.assertRaises(KeyError):
                write_wheel_receipt([], required=True)
            write_wheel_receipt([])

    def test_failed_receipt_retains_component_metadata_and_completed_probes(self):
        """A later probe failure must retain the installed-wheel evidence so far."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            wheel = sample_wheel(root)
            details = {
                "installed_modules": {"fixture_component": "fixture_component/__init__.py"},
                "redis_dump_load": {"version": "1.0", "source_sha256": "probe-hash", "pipeline_reader": True},
                "commands": ["fixture-command"], "tests_run": 2,
            }
            with mock.patch.dict(os.environ, {"TEST_UNDECLARED_OUTPUTS_DIR": str(root)}):
                write_wheel_receipt([wheel], status="failed", details=details,
                                    wheel_metadata={wheel: {"distribution": "fixture-component", "version": "1.0"}})
            receipt = json.loads((root / "wheels.json").read_text())
            self.assertEqual(receipt["status"], "failed")
            self.assertEqual(receipt["wheels"], [{
                "filename": wheel.name, "sha256": hashlib.sha256(wheel.read_bytes()).hexdigest(),
                "distribution": "fixture-component", "version": "1.0",
            }])
            for name, value in details.items():
                self.assertEqual(receipt[name], value)

    def test_setup_failure_receipt_omits_uninstalled_wheels(self):
        """Failed setup cannot claim that any wheel installation was checked."""
        with tempfile.TemporaryDirectory() as temporary:
            with mock.patch.dict(os.environ, {"TEST_UNDECLARED_OUTPUTS_DIR": temporary}):
                write_wheel_receipt(None, status="failed", details={"tests_run": 0})
            receipt = json.loads((Path(temporary) / "wheels.json").read_text())
            self.assertEqual(receipt["status"], "failed")
            self.assertEqual(receipt["tests_run"], 0)
            self.assertNotIn("wheels", receipt)

    def test_component_details_cannot_replace_computed_evidence(self):
        """Extra probe metadata cannot turn a failed run or other bytes into a pass."""
        for field in ("status", "python", "architecture", "wheels"):
            with self.subTest(field=field), self.assertRaisesRegex(ValueError, "computed receipt fields"):
                write_wheel_receipt([], status="failed", details={field: "replacement"})
        with tempfile.TemporaryDirectory() as temporary:
            wheel = sample_wheel(Path(temporary))
            for field in ("filename", "sha256"):
                with self.subTest(field=field), self.assertRaisesRegex(ValueError, "tested filename or hash"):
                    write_wheel_receipt([wheel], wheel_metadata={wheel: {field: "replacement"}})


if __name__ == "__main__":
    unittest.main()
