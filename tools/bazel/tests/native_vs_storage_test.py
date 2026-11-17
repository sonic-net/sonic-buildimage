#!/usr/bin/env python3
"""Exercise the native installer's measured free-space admission and receipt."""

import contextlib
import importlib.util
import io
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location(
    "native_vs_storage", ROOT / "tools/bazel/ci/check_native_vs_storage.py")
storage = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(storage)
GIB = 1024**3


class NativeVsStorageTest(unittest.TestCase):
    def run_check(self, available, *, error=None):
        with tempfile.TemporaryDirectory() as directory:
            workspace = Path(directory) / "checkout with spaces"
            workspace.mkdir()
            receipt = workspace / "artifacts/vs/workspace-capacity.json"
            # Reserved blocks must not count as space available to UID 1000.
            usage = SimpleNamespace(f_bavail=available // 4096, f_frsize=4096,
                                    f_bfree=1024 * GIB // 4096)
            stdout = io.StringIO()
            with mock.patch.object(storage.os, "statvfs", return_value=usage,
                                   side_effect=error) as measured, \
                    contextlib.redirect_stdout(stdout), \
                    contextlib.redirect_stderr(io.StringIO()):
                status = storage.check(workspace, receipt)
            measured.assert_called_once_with(workspace.resolve())
            report = json.loads(receipt.read_text())
            self.assertEqual(json.loads(stdout.getvalue()), report)
            return status, report

    def test_156_gib_workspace_fails_with_measured_evidence(self):
        status, receipt = self.run_check(156 * GIB)
        self.assertEqual(status, 1)
        self.assertEqual(receipt["status"], "failed")
        self.assertEqual(receipt["available_bytes"], 156 * GIB)
        self.assertEqual(receipt["required_available_bytes"], 256 * GIB)

    def test_larger_workspace_passes(self):
        status, receipt = self.run_check(512 * GIB)
        self.assertEqual(status, 0)
        self.assertEqual(receipt["status"], "passed")
        self.assertEqual(receipt["available_bytes"], 512 * GIB)

    def test_exact_threshold_passes_but_one_block_less_fails(self):
        self.assertEqual(self.run_check(256 * GIB)[0], 0)
        self.assertEqual(self.run_check(256 * GIB - 4096)[0], 1)

    def test_unmeasurable_workspace_fails_with_receipt(self):
        status, receipt = self.run_check(0, error=OSError("unavailable filesystem"))
        self.assertEqual(status, 1)
        self.assertIsNone(receipt["available_bytes"])
        self.assertIn("Cannot measure", receipt["error"])

    def test_host_installer_uses_host_preflight_before_preparation(self):
        workflow = (ROOT / ".github/workflows/bazel-oci.yml").read_text()
        native = workflow.split("\n  vs_container:\n", 1)[1]
        invocation = "python3 -B tools/ci/runner/preflight.py --host-agent"
        self.assertNotIn("check_native_vs_storage.py", native)
        self.assertIn("runs-on: [self-hosted, linux, bazel-x64, vs]", native)
        self.assertLess(native.index(invocation), native.index("Prepare the host-driven SONiC slave"))
        self.assertIn('--workspace "$PWD"', native)
        self.assertIn("--build-target sonic-vs.bin", native)
        evidence = native.split("- name: Preserve build receipts\n", 1)[1].split("\n      - name:", 1)[0]
        self.assertIn("artifacts/vs/*.json", evidence)


if __name__ == "__main__":
    unittest.main()
