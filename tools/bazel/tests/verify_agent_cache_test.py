"""Keep cache verification strict about disk hits and source invalidation."""

import contextlib
import io
import json
from argparse import Namespace
from contextlib import redirect_stderr
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

from tools.bazel.ci.verify_agent_cache import ACTIONS, action_evidence, archive_output, remove_temporary_work, run_build, verify


class CacheEvidenceTest(unittest.TestCase):
    """Validate synthetic cache evidence and safe removal of temporary build state."""

    def evidence(self, hit, runner, digest):
        return {"actions": {name: {"cache_hit": hit, "runner": runner} for name in ACTIONS},
                "archive_sha256": digest}

    def test_isolated_build_disables_remote_cache_executor_and_downloader(self):
        for image in (None, "bazel-test-image"):
            with self.subTest(image=image), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                args = Namespace(
                    artifacts=root, docker_image=image, bazel="bazel",
                    bazel_startup_arg=[], docker_arg=[], repository_cache=None,
                    bazel_arg=["--remote_cache=grpc://cache:9092",
                               "--experimental_remote_downloader=grpc://cache:9092"])
                with mock.patch("tools.bazel.ci.verify_agent_cache.subprocess.run",
                                return_value=mock.Mock(returncode=1)) as run:
                    with redirect_stderr(io.StringIO()), self.assertRaisesRegex(RuntimeError, "cold build failed"):
                        run_build(args, "cold", root, root / "cache", root / "output")
                command = run.call_args.args[0]
                for flag in ("--remote_cache=", "--remote_executor=",
                             "--experimental_remote_downloader="):
                    self.assertIn(flag, command)
                    self.assertFalse(any(value.startswith(flag) and value != flag
                                         for value in command[command.index(flag) + 1:]))
                self.assertIn("--disk_cache=" + ("/bazel_cache" if image else str(root / "cache"))
                              + "/disk_cache", command)

    def test_build_failure_prints_retained_bazel_diagnostics(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            bazel = root / "bazel"
            bazel.write_text(
                "#!/bin/sh\n"
                "printf '%s\\n' 'fixture build stdout'\n"
                "printf '%s\\n' 'fixture build stderr' >&2\n"
                "exit 37\n")
            bazel.chmod(0o755)
            args = Namespace(
                artifacts=root, docker_image=None, bazel=str(bazel),
                bazel_startup_arg=[], bazel_arg=[], repository_cache=None)
            diagnostics = io.StringIO()
            with redirect_stderr(diagnostics):
                with self.assertRaisesRegex(RuntimeError, "cold build failed with exit code 37"):
                    run_build(args, "cold", root, root / "cache", root / "output")
            retained = (root / "cold/build.log").read_text()
            self.assertEqual(retained, "fixture build stdout\nfixture build stderr\n")
            for line in retained.splitlines():
                self.assertIn("  " + line, diagnostics.getvalue())
            self.assertIn("cold build failed with exit code 37", diagnostics.getvalue())
            self.assertTrue((root / "cold/command.json").is_file())

    def test_valid_disk_hits_and_source_invalidation(self):
        """Accept cold misses, unchanged disk hits and changed-source misses in receipts."""
        verify(self.evidence(False, "processwrapper-sandbox", "same"),
               self.evidence(True, "disk cache hit", "same"),
               self.evidence(False, "processwrapper-sandbox", "changed"))

    def test_local_action_reuse_does_not_prove_cross_checkout_cache(self):
        """Reject a local action-cache hit as evidence of shared disk-cache reuse."""
        with self.assertRaisesRegex(ValueError, "shared disk cache"):
            verify(self.evidence(False, "sandbox", "same"),
                   self.evidence(True, "local cache hit", "same"),
                   self.evidence(False, "sandbox", "changed"))

    def test_cache_cannot_hide_source_change(self):
        """Reject a changed-source receipt that still reports a cache hit."""
        with self.assertRaisesRegex(ValueError, "old output"):
            verify(self.evidence(False, "sandbox", "same"),
                   self.evidence(True, "disk cache hit", "same"),
                   self.evidence(True, "disk cache hit", "changed"))

    def test_multiline_execution_log_and_file_uri(self):
        """Read multiline action records and decode spaces in an archive's file URI."""
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "events.json"
            path.write_text("\n".join(json.dumps({"mnemonic": name, "targetLabel": label,
                                                 "cacheHit": True, "runner": "disk cache hit"}, indent=2)
                                      for name, label in ACTIONS.items()))
            self.assertEqual(set(action_evidence(path)), set(ACTIONS))
            path.write_text(json.dumps({"namedSetOfFiles": {"files": [
                {"uri": "file:///tmp/with%20space/archive-fixture.gz"}]}}))
            self.assertEqual(archive_output(path), Path("/tmp/with space/archive-fixture.gz"))

    def test_cleanup_handles_readonly_outputs_without_following_links(self):
        """Remove read-only work directories while preserving external symlink targets."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            work, outside = root / "work", root / "outside"
            nested = work / "readonly"
            nested.mkdir(parents=True)
            (nested / "index.json").write_text("{}")
            nested.chmod(0o555)
            outside.mkdir()
            (outside / "preserved").write_text("keep")
            outside.chmod(0o555)
            (work / "external").symlink_to(outside, target_is_directory=True)
            remove_temporary_work(work)
            self.assertFalse(work.exists())
            self.assertEqual((outside / "preserved").read_text(), "keep")
            self.assertEqual(outside.stat().st_mode & 0o777, 0o555)
            outside.chmod(0o755)

    def failed_build(self, text):
        """Exercise the real failing call path without starting Bazel or Docker."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            artifacts = root / "artifacts"
            artifacts.mkdir()
            args = SimpleNamespace(artifacts=artifacts, bazel="unused-bazel", bazel_startup_arg=[],
                                   bazel_arg=[], repository_cache=None, docker_image=None, docker_arg=[])

            def fail(command, **kwargs):
                self.assertEqual(command[-1], "//tools/bazel/tests:archive-fixture.gz")
                kwargs["stdout"].write(text)
                return SimpleNamespace(returncode=37)

            output = io.StringIO()
            with mock.patch("tools.bazel.ci.verify_agent_cache.subprocess.run", side_effect=fail) as process:
                with contextlib.redirect_stderr(output):
                    with self.assertRaisesRegex(RuntimeError, "warm build failed with exit code 37; see .*build.log"):
                        run_build(args, "warm", root, root / "cache", root / "output")
            process.assert_called_once()
            self.assertEqual((artifacts / "warm/build.log").read_text(), text)
            return output.getvalue()

    def test_build_failure_prints_real_error_without_replacing_failure(self):
        text = "ERROR: /__w/sonic/BUILD:7:1: fixture action failed: missing input\n"
        output = self.failed_build(text)
        self.assertIn("warm build failed with exit code 37", output)
        self.assertIn("fixture action failed: missing input", output)
        self.assertNotIn("/__w/", output)

    def test_build_failure_withholds_credentials_and_workflow_commands(self):
        secret = "opaque-value-from-environment"
        text = (f"download response: {secret}\n"
                "Authorization: Bearer another-opaque-value\n"
                "GET https://user:password@example.invalid/payload\n"
                "GET https://example.invalid/payload?sig=private-query\n"
                "ghp_abcdefghijklmnopqrstuvwxyz0123456789\n"
                "::warning::untrusted output\n"
                "  ::warning::indented untrusted output\n"
                "ordinary prefix ##[warning]embedded untrusted output\n"
                "ERROR: Namespace::method failed\n"
                "ERROR: download failed with HTTP 503\n")
        with mock.patch.dict("os.environ", {"CI_DOWNLOAD_TOKEN": secret}, clear=True):
            output = self.failed_build(text)
        for value in (secret, "another-opaque-value", "private-query", "ghp_", "user:password"):
            self.assertNotIn(value, output)
        self.assertIn("ERROR: download failed with HTTP 503", output)
        self.assertIn("ERROR: Namespace::method failed", output)
        self.assertNotIn("::warning::", output)
        self.assertNotIn("##[", output)

    def test_build_failure_bounds_tail_and_discards_partial_first_line(self):
        text = "discarded-prefix" + "x" * 70000 + "\n" + "old message\n" * 100 + "ERROR: final cause\n"
        output = self.failed_build(text)
        self.assertNotIn("discarded-prefix", output)
        self.assertNotIn("x" * 100, output)
        self.assertEqual(len(output.splitlines()), 81)
        self.assertLess(len(output), 65536)
        self.assertIn("ERROR: final cause", output)

    def test_build_failure_caps_rendered_bytes_after_redaction(self):
        text = ("x" * 4096 + "\n") * 80 + "Unicode: " + "\u2603" * 3000 + "\nERROR: final cause\n"
        with mock.patch.dict("os.environ", {"CI_DOWNLOAD_TOKEN": "x"}, clear=True):
            output = self.failed_build(text)
        tail = output.split("\n", 1)[1]
        self.assertLessEqual(len(tail.encode("utf-8")), 16384)
        self.assertTrue(all(len(line.encode("utf-8")) <= 2048 for line in tail.splitlines(keepends=True)))
        self.assertIn("[redacted]", tail)
        self.assertTrue(tail.endswith("ERROR: final cause\n"))

    def test_successful_build_keeps_output_and_action_evidence_without_diagnostics(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            artifacts = root / "artifacts"
            artifacts.mkdir()
            archive = root / "archive-fixture.gz"
            archive.write_bytes(b"same archive")
            args = SimpleNamespace(artifacts=artifacts, bazel="unused-bazel", bazel_startup_arg=[],
                                   bazel_arg=[], repository_cache=None, docker_image=None, docker_arg=[])
            output = io.StringIO()
            with mock.patch("tools.bazel.ci.verify_agent_cache.subprocess.run", return_value=SimpleNamespace(returncode=0)):
                with mock.patch("tools.bazel.ci.verify_agent_cache.action_evidence", return_value={"unchanged": True}):
                    with mock.patch("tools.bazel.ci.verify_agent_cache.archive_output", return_value=archive):
                        with contextlib.redirect_stderr(output):
                            result = run_build(args, "warm", root, root / "cache", root / "output")
            self.assertEqual(output.getvalue(), "")
            self.assertEqual(result["actions"], {"unchanged": True})
            self.assertEqual((artifacts / "warm/archive-fixture.gz").read_bytes(), b"same archive")


if __name__ == "__main__":
    unittest.main()
