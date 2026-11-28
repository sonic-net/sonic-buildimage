"""Check command evidence and artifact-path separation."""

import contextlib
import io
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

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

    def test_transient_fetch_retries_preserve_arguments_logs_and_final_result(self):
        """Recover from EOF with bounded backoff, or retain the last failure on exhaustion."""
        for failures, returncode, attempts in ((1, 0, 2), (2, 0, 3), (5, 1, 3)):
            with self.subTest(failures=failures), tempfile.TemporaryDirectory() as temporary:
                directory = Path(temporary)
                receipt = {"commands": []}
                command = [sys.executable, "-c", f'''
from pathlib import Path
import sys
counter = Path("fetch-count")
count = int(counter.read_text()) + 1 if counter.exists() else 1
counter.write_text(str(count))
if count <= {failures}:
    print("partial query output")
    print('Error in fail: repository: fetch_repo: module@v1: Get "https://proxy.example/module": EOF',
          file=sys.stderr)
    sys.exit(1)
print("runtime.tar")
''', "argument with spaces"]
                with mock.patch.object(command_log.time, "sleep") as sleep, \
                        contextlib.redirect_stdout(io.StringIO()):
                    if returncode:
                        with self.assertRaises(subprocess.CalledProcessError) as error:
                            command_log.execute(command, directory, receipt, "query", cwd=directory,
                                                retry_fetch=True)
                        self.assertEqual(error.exception.returncode, 1)
                    else:
                        result = command_log.execute(command, directory, receipt, "query",
                                                     cwd=directory, retry_fetch=True)
                        self.assertEqual(result, "runtime.tar\n")
                self.assertEqual(int((directory / "fetch-count").read_text()), attempts)
                self.assertEqual(sleep.call_args_list, [mock.call(delay) for delay in (10, 30)[:attempts - 1]])
                self.assertEqual(len(receipt["commands"]), 1)
                evidence = receipt["commands"][0]
                self.assertEqual(evidence["argv"], command)
                self.assertEqual(evidence["returncode"], returncode)
                self.assertEqual([item["returncode"] for item in evidence["attempts"]],
                                 [1] * (attempts - 1) + [returncode])
                for item in evidence["attempts"][:-1]:
                    self.assertIn("EOF", (directory / item["log"]).read_text())
                self.assertEqual(evidence["log"], "query.log")
                if not returncode:
                    self.assertEqual((directory / "query.log").read_text(), "runtime.tar\n")

    def test_retries_require_opt_in_and_do_not_mask_permanent_or_cancelled_failures(self):
        """Do not retry compiler/audit-adjacent errors, missing modules, or cancellation."""
        transient = 'fetch_repo: module: Get "https://proxy.example/module": EOF\n'
        cases = (
            (transient, False, 1),
            (transient, True, 8),
            ("unexpected EOF in compiler input\n", True, 1),
            ('fetch_repo: module: Get "https://proxy.example/module": 404 Not Found\n', True, 1),
            ('fetch_repo: module: Get "https://proxy.example/module": 403 Forbidden\n', True, 1),
            (transient + "SECURITY ERROR: checksum mismatch\n", True, 1),
            (transient + "Error in fail: invalid version: unknown revision\n", True, 1),
            (transient + "ERROR: /workspace/BUILD: Compiling source failed\n", True, 1),
            (transient + "source.cc:2: error: syntax error\n", True, 1),
            (transient + "ERROR: no such package '//missing': BUILD file not found\n", True, 1),
            (transient + 'fetch_repo: other: Get "https://proxy.example/other": 404 Not Found\n', True, 1),
        )
        for diagnostic, enabled, returncode in cases:
            with self.subTest(diagnostic=diagnostic, enabled=enabled, returncode=returncode), \
                    tempfile.TemporaryDirectory() as temporary:
                directory = Path(temporary)
                receipt = {"commands": []}
                completed = subprocess.CompletedProcess(["bazel"], returncode, "", diagnostic)
                with mock.patch.object(command_log.subprocess, "run", return_value=completed) as run, \
                        mock.patch.object(command_log.time, "sleep") as sleep, \
                        contextlib.redirect_stdout(io.StringIO()):
                    with self.assertRaises(subprocess.CalledProcessError):
                        command_log.execute(["bazel"], directory, receipt, "query", cwd=directory,
                                            retry_fetch=enabled)
                self.assertEqual(run.call_count, 1)
                sleep.assert_not_called()
                self.assertNotIn("attempts", receipt["commands"][0])

    def test_transport_and_temporary_http_fetch_failures_retry_but_mixed_errors_do_not(self):
        """Accept scoped transport/HTTP errors, rejecting independent repository failures."""
        for failure in ("connection reset by peer", "i/o timeout", "TLS handshake timeout",
                        "429 Too Many Requests", "500 Internal Server Error", "503 Service Unavailable"):
            diagnostic = f'Error in fail: Error downloading [https://proxy.example/archive]: {failure}\n'
            for suffix, attempts in (("", 2),
                                     ("ERROR: another repository failed with 404 Not Found\n", 1)):
                with self.subTest(failure=failure, suffix=suffix), \
                        tempfile.TemporaryDirectory() as temporary:
                    directory = Path(temporary)
                    receipt = {"commands": []}
                    command = ["bazel", "aquery", "//:target"]
                    results = [subprocess.CompletedProcess(command, 1, "", diagnostic + suffix),
                               subprocess.CompletedProcess(command, 0, "runtime.tar\n", "")]
                    with mock.patch.object(command_log.subprocess, "run", side_effect=results) as run, \
                            mock.patch.object(command_log.time, "sleep") as sleep, \
                            contextlib.redirect_stdout(io.StringIO()):
                        if attempts == 1:
                            with self.assertRaises(subprocess.CalledProcessError):
                                command_log.execute(command, directory, receipt, "query",
                                                    cwd=directory, retry_fetch=True)
                            sleep.assert_not_called()
                        else:
                            self.assertEqual(command_log.execute(
                                command, directory, receipt, "query", cwd=directory,
                                retry_fetch=True), "runtime.tar\n")
                            sleep.assert_called_once_with(10)
                    self.assertEqual(run.call_count, attempts)

    def test_private_output_is_truncated_between_fetch_retries_and_never_logged(self):
        """Discard partial action JSON without leaking raw actions or changing file permissions."""
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            output = directory / "actions.json"
            output.touch(mode=0o600)
            receipt = {"commands": []}
            command = [sys.executable, "-c", '''
from pathlib import Path
import sys
counter = Path("count")
if not counter.exists():
    counter.touch()
    print("private partial action graph" * 100)
    print('fetch_repo: module: Get "https://proxy.example/module": EOF', file=sys.stderr)
    sys.exit(1)
print("private final graph")
''']
            console = io.StringIO()
            with mock.patch.object(command_log.time, "sleep"), contextlib.redirect_stdout(console):
                self.assertEqual(command_log.execute(
                    command, directory, receipt, "actions", cwd=directory,
                    output_path=output, retry_fetch=True), "")
            self.assertEqual(output.read_text(), "private final graph\n")
            self.assertEqual(output.stat().st_mode & 0o777, 0o600)
            self.assertNotIn("private", console.getvalue())
            for item in receipt["commands"][0]["attempts"]:
                self.assertNotIn("private", (directory / item["log"]).read_text())


if __name__ == "__main__":
    unittest.main()
