"""Reject Debian package actions before CI builds or runs tests."""

import json
from pathlib import Path
import tempfile
import sys
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import bazel_commands


class ActionAuditTest(unittest.TestCase):
    def audit(self, output_name, arguments):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            graph = {
                "actions": [{"outputIds": [1], "arguments": arguments,
                             "environmentVariables": [{"key": "PRIVATE", "value": "private-value"}]}],
                "artifacts": [{"id": 1, "pathFragmentId": 1}],
                "pathFragments": [{"id": 1, "label": output_name}],
            }
            def query(command, *, cwd, stdout, check):
                self.assertEqual(command[1], "aquery")
                self.assertIn("--platforms=native", command)
                stdout.write(json.dumps(graph))
                stdout.flush()
            with mock.patch("subprocess.run", side_effect=query):
                try:
                    return bazel_commands.audit_targets(
                        "bazel", ["--platforms=native"], ["//:checked"],
                        workspace=root, output=root / "audit.json")
                finally:
                    summary = (root / "audit.json").read_text()
                    self.assertNotIn("private-value", summary)

    def test_accepts_tar_assembly_and_debian_extraction(self):
        result = self.audit("runtime.tar", ["dpkg-deb", "--fsys-tarfile", "input.deb"])
        self.assertEqual(result["targets"], ["//:checked"])
        self.assertEqual(result["action_count"], 1)

    def test_rejects_debian_output(self):
        with self.assertRaisesRegex(ValueError, "DEB or packaging wrapper"):
            self.audit("package.deb", ["packager"])

    def test_accepts_declared_make_tool_for_source_generation(self):
        # A source generator receives a declared tool path as data. It is not
        # an action invoking an unreviewed Make packaging target directly.
        result = self.audit("generated_sources", [
            "bazel-out/bin/external/component/bazel/prepare", "prepare",
            "--make", "bazel-out/bin/external/infra/tools/build_tools/make",
            "--output", "bazel-out/bin/generated_sources",
        ])
        self.assertEqual(result["packaging_wrappers"], [])

    def test_rejects_wrapped_packaging_with_hidden_output(self):
        for command in (["dpkg-deb", "--build", "tree", "output"],
                        ["sh", "-c", "make package"], ["dpkg-buildpackage", "-b"],
                        ["/declared/tools/make", "package"],
                        ["/usr/bin/env", "LC_ALL=C", "/declared/tools/make", "package"],
                        ["bash", "-ec", "/declared/tools/dpkg-buildpackage -b"],
                        ["package-helper", "--builder", "/usr/bin/dpkg-buildpackage"],
                        ["timeout", "30", "gmake", "package"]):
            with self.subTest(command=command):
                with self.assertRaisesRegex(ValueError, "DEB or packaging wrapper"):
                    self.audit("output", command)


if __name__ == "__main__":
    unittest.main()
