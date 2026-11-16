"""Check attribution of Bazel's overlapping VS recipe profile events."""

import gzip
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci.summarize_vs_image_timings import markdown_row, read_owner, report, summarize_profile


def event(category, name, start, duration, tid=1, mnemonic=None):
    record = {"ph": "X", "pid": 1, "tid": tid, "cat": category, "name": name,
              "ts": start * 1_000_000, "dur": duration * 1_000_000}
    if mnemonic:
        record["args"] = {"mnemonic": mnemonic}
    return record


class ImageTimingSummaryTest(unittest.TestCase):
    def test_compilation_counts_running_processes_and_unions_parallel_workers(self):
        events = [
            event("action processing", "Compiling a.cc", 0, 10, mnemonic="CppCompile"),
            event("general information", "subprocess.run", 2, 5),
            event("action processing", "Cached compile", 0, 18, tid=2, mnemonic="CppCompile"),
            event("Local execution process wall time", "CppCompile", 4, 6, tid=3),
            event("action processing", "Building lib", 7, 7, tid=4, mnemonic="CcCmakeMakeRule"),
            event("general information", "subprocess.run", 9, 4, tid=4),
            event("action processing", "Linking", 11, 3, mnemonic="CppLink"),
            event("general information", "subprocess.run", 12, 1),
        ]
        self.assertEqual(summarize_profile(list(reversed(events))),
                         {"compile": 8, "foreign": 4, "lock": 0, "fetch": 0})

    def test_fetch_excludes_output_reuse_uploads_and_cache_lookups(self):
        events = [
            event("build phase marker", "Blocking on busy Bazel server (in client)", -4, 4),
            event("build phase marker", "Blocking on busy Bazel server (in server)", 0, 2),
            event("remote output download", "download outputs", 2, 5),
            event("remote output download", "Download outputs", 3, 1, tid=2),
            event("remote output download", "stage remote inputs", 6, 3, tid=3),
            event("remote output download", "reuse outputs", 9, 2),
            event("remote output download", "await output reuse", 11, 2),
            event("remote output download", "download server logs", 13, 3),
            event("remote action cache check", "check cache hit", 14, 2),
            event("Remote execution upload time", "upload outputs", 15, 2),
        ]
        self.assertEqual(summarize_profile(events),
                         {"compile": 0, "foreign": 0, "lock": 6, "fetch": 7})

    def test_recipe_uses_independent_profiles_and_handles_failed_or_missing_phases(self):
        with tempfile.TemporaryDirectory() as root:
            directory = Path(root) / "docker-teamd.gz"
            row, issues = read_owner(directory)
            self.assertIsNone(row)
            self.assertFalse(issues)
            directory.mkdir()
            for phase in ("build", "cquery-docker-teamd.gz", "cquery-docker-teamd-dbg.gz",
                          "export-docker-teamd.gz", "export-docker-teamd-dbg.gz"):
                (directory / f"{phase}.timing.json").write_text(json.dumps(
                    {"phase": phase, "elapsed_seconds": 2, "exit_code": 0}))
                if not phase.startswith("export"):
                    with gzip.open(directory / f"{phase}.profile.json.gz", "wt") as stream:
                        json.dump({"traceEvents": [event(
                            "build phase marker", "Blocking on busy Bazel server (in server)", 0, 1)
                        ]}, stream)
            row, issues = read_owner(directory)
            self.assertEqual(markdown_row(row)[1:], ["6.00", "3.00", "0.00", "0.00", "0.00", "4.00"])
            self.assertEqual(issues, [])
            (directory / "build.profile.json.gz").write_bytes(b"truncated profile")
            (directory / "export-docker-teamd.gz.timing.json").unlink()
            (directory / "build.timing.json").write_text(json.dumps(
                {"phase": "build", "elapsed_seconds": 2, "exit_code": 1}))
            row, issues = read_owner(directory)
            self.assertEqual(markdown_row(row)[2], "2.00 (partial)")
            self.assertEqual(markdown_row(row)[-1], "2.00 (partial)")
            self.assertIn("build exited 1", issues)
            self.assertIn("build profile missing or unreadable", issues)
            output = Path(root) / "summary.json"
            script = Path(__file__).resolve().parents[1] / "ci/summarize_vs_image_timings.py"
            run = subprocess.run([sys.executable, "-B", script, root, "--dashboard-json", output],
                                 text=True, capture_output=True, check=True)
            self.assertIn("2.00 (partial)", run.stdout)
            receipt = json.loads(output.read_text())
            self.assertEqual(receipt["version"], 2)
            self.assertEqual(receipt["recipes"], [{
                "owner": "docker-teamd.gz", "bazel_seconds": 6, "lock_seconds": 2,
                "compile_seconds": 0, "foreign_seconds": 0, "fetch_seconds": 0, "export_seconds": 2,
                "bazel_phases": 3, "bazel_expected": 3, "profiles": 2, "profiles_expected": 3,
                "exports": 1, "exports_expected": 2, "failed": True,
            }])

    def test_read_discovered_packages_and_repeated_independent_invocations(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for index in range(2):
                instance = root / "linux-kernel" / f"instance-{index}"
                instance.mkdir(parents=True)
                (instance / "recipe.json").write_text(json.dumps({
                    "bazel": ["launcher"], "profiles": ["build", "cquery", "mod"], "exports": ["stage-debs"]}))
                (instance / "launcher.timing.json").write_text(json.dumps({
                    "phase": "launcher", "elapsed_seconds": 12.25, "exit_code": 0}))
                with gzip.open(instance / "build.profile.json.gz", "wt") as stream:
                    json.dump({"traceEvents": [
                        event("Local execution process wall time", "SonicKernelBuild", 0, 8)]}, stream)
            instance = root / "example-package" / "another-invocation"
            instance.mkdir(parents=True)
            (instance / "recipe.json").write_text('{"bazel":["build"],"profiles":["build"],"exports":[]}')
            receipt, markdown = report(root)
            kernel, package = receipt["recipes"]
            self.assertEqual((kernel["owner"], kernel["bazel_seconds"], kernel["bazel_expected"]),
                             ("linux-kernel", 24.5, 2))
            self.assertEqual((kernel["foreign_seconds"], kernel["profiles"], kernel["profiles_expected"]),
                             (16, 2, 6))
            self.assertEqual(package["exports_expected"], 0)
            self.assertIsNone(package["bazel_seconds"])
            self.assertIn("n/a", markdown)


if __name__ == "__main__":
    unittest.main()
