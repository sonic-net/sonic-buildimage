"""Check command evidence and artifact-path separation."""

import contextlib
import io
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

from tools.bazel.ci import command_log


class CommandEvidenceTest(unittest.TestCase):
    """Keep command results separate from their retained diagnostic evidence."""

    def test_diagnostics_cannot_become_an_artifact_path(self):
        """Return only stdout for path lookup while retaining stderr and success status."""
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            receipt = {"commands": []}
            result = command_log.execute(
                [sys.executable, "-c", "import sys; print('runtime.tar'); print('INFO: query', file=sys.stderr)"],
                directory, receipt, "query", cwd=directory)
            self.assertEqual(result.splitlines(), ["runtime.tar"])
            self.assertIn("INFO: query", (directory / "query.log").read_text())
            self.assertEqual(receipt["commands"][0]["returncode"], 0)

    def test_failed_build_retains_its_diagnostics(self):
        """Record a failing command's message and exit status before propagating failure."""
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            receipt = {"commands": []}
            with self.assertRaises(subprocess.CalledProcessError):
                command_log.execute([sys.executable, "-c", "raise SystemExit('build failed')"],
                           directory, receipt, "build", cwd=directory)
            self.assertIn("build failed", (directory / "build.log").read_text())
            self.assertEqual(receipt["commands"][0]["returncode"], 1)

    def test_private_query_output_stays_out_of_logs_even_on_failure(self):
        """Keep large action JSON private while preserving diagnostics and exit status."""
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            output = directory / "actions.json"
            output.touch(mode=0o600)
            for returncode in (0, 7):
                with self.subTest(returncode=returncode):
                    receipt = {"commands": []}
                    console = io.StringIO()
                    command = [sys.executable, "-c",
                               "import sys; print('private-action-data'); "
                               "print('query diagnostic', file=sys.stderr); "
                               "sys.exit(" + str(returncode) + ")"]
                    with contextlib.redirect_stdout(console):
                        if returncode:
                            with self.assertRaises(subprocess.CalledProcessError):
                                command_log.execute(command, directory, receipt, "query",
                                                    cwd=directory, output_path=output)
                        else:
                            self.assertEqual(command_log.execute(
                                command, directory, receipt, "query", cwd=directory,
                                output_path=output), "")
                    self.assertEqual(output.read_text(), "private-action-data\n")
                    self.assertEqual(output.stat().st_mode & 0o777, 0o600)
                    self.assertEqual((directory / "query.log").read_text(), "query diagnostic\n")
                    self.assertEqual(console.getvalue(), "query diagnostic\n")
                    self.assertEqual(receipt["commands"][0]["returncode"], returncode)


if __name__ == "__main__":
    unittest.main()
