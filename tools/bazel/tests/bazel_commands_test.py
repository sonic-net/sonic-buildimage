"""Reject Debian package actions before CI builds or runs tests."""

import json
from pathlib import Path
import subprocess
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
            def query(command, *, cwd, stdout, stderr, text):
                self.assertEqual(command[1], "aquery")
                self.assertIn("--platforms=native", command)
                stdout.write(json.dumps(graph))
                stdout.flush()
                return subprocess.CompletedProcess(command, 0, stderr="")
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

    def test_transient_fetch_retries_discard_partial_graph_and_still_enforce_audit(self):
        """Retry the observed Go fetch EOF without bypassing the no-DEB gate or exposing actions."""
        for output_name in ("runtime.tar", "package.deb"):
            with self.subTest(output_name=output_name), tempfile.TemporaryDirectory() as temporary:
                directory = Path(temporary)
                bazel = directory / "bazel"
                graph = {
                    "actions": [{"outputIds": [1], "arguments": ["packager"],
                                 "environmentVariables": [{"key": "PRIVATE", "value": "private-value"}]}],
                    "artifacts": [{"id": 1, "pathFragmentId": 1}],
                    "pathFragments": [{"id": 1, "label": output_name}],
                }
                bazel.write_text("#!" + sys.executable + "\n" + f'''
from pathlib import Path
import sys
counter = Path("count")
if not counter.exists():
    counter.touch()
    print("private-partial-graph" * 100)
    print('Error in fail: repository: fetch_repo: module@v1: Get "https://proxy.example/module": EOF',
          file=sys.stderr)
    sys.exit(1)
print({json.dumps(graph)!r})
''')
                bazel.chmod(0o755)
                with mock.patch("tools.bazel.ci.command_log.time.sleep") as sleep:
                    if output_name.endswith(".deb"):
                        with self.assertRaisesRegex(ValueError, "DEB or packaging wrapper"):
                            bazel_commands.audit_targets(
                                str(bazel), [], ["//:checked"], workspace=directory,
                                output=directory / "audit.json")
                    else:
                        report = bazel_commands.audit_targets(
                            str(bazel), [], ["//:checked"], workspace=directory,
                            output=directory / "audit.json")
                        self.assertEqual(report["action_count"], 1)
                sleep.assert_called_once_with(10)
                receipt = json.loads((directory / "audit.query.json").read_text())
                self.assertEqual([attempt["returncode"] for attempt in
                                  receipt["commands"][0]["attempts"]], [1, 0])
                for path in directory.glob("audit*"):
                    self.assertNotIn("private-value", path.read_text())
                    self.assertNotIn("private-partial-graph", path.read_text())


if __name__ == "__main__":
    unittest.main()
