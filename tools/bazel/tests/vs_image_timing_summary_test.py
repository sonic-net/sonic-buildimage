"""Check attribution of Bazel's overlapping VS recipe profile events."""

import gzip
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import textwrap
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import time_vs_image
from tools.bazel.ci.container_profile import batch_profile, combine_batches
from tools.bazel.ci.summarize_vs_image_timings import (
    BUILD_EVIDENCE, build_evidence, build_explanation, build_nonlock,
    markdown_row, named_spans, read_owner, report, summarize_profile,
)


def event(category, name, start, duration, tid=1, mnemonic=None):
    record = {"ph": "X", "pid": 1, "tid": tid, "cat": category, "name": name,
              "ts": start * 1_000_000, "dur": duration * 1_000_000}
    if mnemonic:
        record["args"] = {"mnemonic": mnemonic}
    return record


class ImageTimingSummaryTest(unittest.TestCase):
    def test_shared_command_and_individual_exports_have_distinct_timing_coverage(self):
        with tempfile.TemporaryDirectory() as tmp:
            workspace = Path(tmp)
            group = Path(subprocess.check_output([
                sys.executable, time_vs_image.__file__, "start-recipe", tmp, "vs-container-images",
                "--bazel", "build", "--profiles", "build", "--exports",
            ], text=True).strip())
            command = ["bazel", "--output_base=local", "build", "//:first", "//:second"]
            with mock.patch.object(time_vs_image.subprocess, "run") as execute:
                time_vs_image.run_phase(group, "build", command, cwd=workspace)
                execute.assert_called_once_with(
                    [*command[:3], f"--profile={group}/build.profile.json.gz", "--noslim_profile",
                     "--experimental_profile_include_target_label", *command[3:]],
                    check=True, cwd=workspace, env=None)
                execute.reset_mock()
                time_vs_image.run_phase(None, "build", command, cwd=workspace)
                execute.assert_called_once_with(command, check=True, cwd=workspace, env=None)
            with gzip.open(group / "build.profile.json.gz", "wt") as stream:
                json.dump({"traceEvents": [event("Local execution process wall time", "CppCompile", 0, 5)]}, stream)
            for name in ("first", "second"):
                owner = time_vs_image.start_recipe(workspace, name, [], [], ["export"])
                with time_vs_image.time_phase(owner, "export"):
                    pass
            receipt, markdown = report(workspace / "artifacts/vs/image-timings")
            rows = {row["owner"]: row for row in receipt["recipes"]}
            self.assertEqual(set(rows), {"first", "second", "vs-container-images"})
            self.assertEqual(rows["vs-container-images"]["compile_seconds"], 5)
            self.assertEqual(sum(row["bazel_phases"] for row in rows.values()), 1)
            self.assertEqual(sum(row["profiles"] for row in rows.values()), 1)
            self.assertEqual(sum(row["exports"] for row in rows.values()), 2)
            self.assertIn("| first | n/a | n/a | n/a | n/a | n/a |", markdown)

    def test_batch_profile_counts_parallel_owners_once_and_covers_launcher_time(self):
        archives = {"one.gz": ("one.gz", "//dockers/one:one.gz"),
                    "one-dbg.gz": ("one.gz", "//dockers/one:one-dbg.gz"),
                    "two.gz": ("two.gz", "//platform/vs/two:two.gz")}

        def action(label, mnemonic, start, duration, thread):
            record = event("action processing", "uses a file in dockers/one", start, duration, thread, mnemonic)
            record["args"]["target"] = label
            return record

        events = [action("//dockers/one/config:table", "OCITarball", 1, 4, 1),
                  action("@@//dockers/one:debug", "GzipCompress", 4, 3, 2),
                  action("//platform/vs/two:image", "OCITarball", 5, 3, 3),
                  action("@@dependency+//dockers/one:shared", "CppCompile", 0, 6, 4),
                  action("//src/shared:lib", "CppCompile", 9, 1, 5),
                  event("remote output download", "stage remote inputs", 3, 2, 1),
                  event("remote output download", "download outputs", 4, 1, 2),
                  event("remote output download", "download outputs", 3, 2, 4),
                  event("remote output download", "download outputs", 3, 5, 3)]
        result = batch_profile(list(reversed(events)), 12, archives, legacy=False)
        self.assertEqual(result, {
            "rows": [{"owner": "one.gz", "actions": 2, "action_seconds": 6, "archive_seconds": 4,
                      "compress_seconds": 3, "fetch_seconds": 2},
                     {"owner": "two.gz", "actions": 1, "action_seconds": 3, "archive_seconds": 3,
                      "compress_seconds": 0, "fetch_seconds": 0}],
            "other_action_seconds": 2, "no_action_seconds": 3, "parallel_seconds": 2})
        both = combine_batches([result, result])
        self.assertEqual(both["rows"][0]["action_seconds"], 12)
        self.assertEqual(both["rows"][0]["actions"], 4)
        self.assertEqual(both["other_action_seconds"], 4)

    def test_reprocess_previous_profiles_using_confirmed_archive_outputs(self):
        archives = {"one.gz": ("one.gz", ""), "one-dbg.gz": ("one.gz", "")}
        events = [event("action processing", "OCI Image //dockers/one:one", 1, 1, mnemonic="OCIImage"),
                  event("action processing", "OCITarball dockers/one/one.load/tarball.tar", 2, 2,
                        mnemonic="OCITarball"),
                  event("action processing", "Compressing o to dockers/one/one.gz", 4, 1, mnemonic="GzipCompress"),
                  event("action processing", "Compressing d to dockers/one/one-dbg.gz", 5, 1, mnemonic="GzipCompress"),
                  event("action processing", "OCI Descriptor dockers/one/input.tar", 6, 1, mnemonic="OCIDescriptor")]
        row, = batch_profile(events, 7, archives, legacy=True)["rows"]
        self.assertEqual(row, {"owner": "one.gz", "actions": 4, "action_seconds": 5, "fetch_seconds": 0,
                               "archive_seconds": 2, "compress_seconds": 2})
        self.assertIsNone(batch_profile(events[:-2], 7, archives, legacy=True))
        # The same basename appearing in two packages cannot identify a legacy owner.
        events.append(event("action processing", "Compressing other to unrelated/one.gz", 7, 1,
                            mnemonic="GzipCompress"))
        self.assertIsNone(batch_profile(events, 8, archives, legacy=True))

    def test_github_receipt_retains_owner_rows_and_independent_bounded_markers(self):
        with tempfile.TemporaryDirectory() as tmp:
            workspace = Path(tmp)
            group = time_vs_image.start_recipe(workspace, "vs-container-images", ["build"], ["build"], [])
            (group / "batch-targets.json").write_text('{"demo.gz":["demo.gz","//example:demo.gz"]}')
            (group / "build.timing.json").write_text('{"phase":"build","elapsed_seconds":7,"exit_code":0}')
            record = event("action processing", "action", 0, 3, mnemonic="OCITarball")
            record["args"]["target"] = "//example:demo"
            with gzip.open(group / "build.profile.json.gz", "wt") as stream:
                json.dump({"traceEvents": [record]}, stream)
            time_vs_image.start_recipe(workspace, "demo.gz", [], [], ["export-demo.gz"])
            root = group.parent.parent
            script = Path(__file__).resolve().parents[1] / "ci/summarize_vs_image_timings.py"
            run = subprocess.run([sys.executable, "-B", script, str(root), "--github-actions"],
                                 capture_output=True, text=True, check=True)
            receipt = json.loads((root / "summary.json").read_text())
            v1 = [json.loads(line.split(" ", 1)[1]) for line in run.stdout.splitlines()
                  if line.startswith("SONIC_VS_CONTAINER_TIMINGS_V1 ")]
            self.assertEqual(v1[0], {"row": receipt["container_breakdown"]["rows"][0]})
            self.assertEqual(v1[1], {"count": 1, "no_action_seconds": 4, "other_action_seconds": 0,
                                     "parallel_seconds": 0})
            v2 = [json.loads(line.split(" ", 1)[1]) for line in run.stdout.splitlines()
                  if line.startswith("SONIC_VS_BAZEL_TIMINGS_V2 ")]
            self.assertEqual(v2[-1], {"count": 2, "evidence_count": 6})
            phase_evidence = [item["evidence"]["values"] for item in v2 if "evidence" in item
                              and item["evidence"]["owner"] == "vs-container-images"
                              and item["evidence"]["group"] == "phases"]
            self.assertEqual(phase_evidence[0][0]["seconds"], 7)
            (group / "build.profile.json.gz").unlink()
            self.assertNotIn("container_breakdown", report(root)[0])

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
            self.assertEqual(receipt["recipes"][0].pop("build"), {
                "expected": 1, "timings": 1, "profiles": 0, "seconds": 2, "lock_seconds": None,
                **{key: None for key in BUILD_EVIDENCE},
            })
            details = receipt["recipes"][0].pop("details")
            self.assertEqual((details["profiles"], details["expected"], details["phase_count"]), (0, 1, 4))
            self.assertEqual(details["repositories"], [])
            self.assertIsNone(next(p for p in details["phases"] if p["phase"] == "build")["lock_seconds"])
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
            self.assertIsNone(kernel["build"])
            self.assertIn("n/a", markdown)

    def test_build_evidence_unions_overlaps_and_excludes_duplicate_or_private_names(self):
        events = [
            event("Starlark repository function call", "//:rules_distroless++apt+trixie_native-amd64_1", 0, 10),
            event("Starlark builtin function call", "read", 2, 6),
            event("Starlark repository function call", "//:rules_distroless++apt+trixie_native-arm64_1", 4, 8, tid=2),
            event("Starlark builtin function call", "read", 5, 6, tid=2),
            event("Starlark repository function call", "local: --token=must-not-publish", 0, 30, tid=3),
            event("Starlark repository function call", "//:rules_distroless++apt+trixie_native-armhf_1", 20, 5, tid=4),
            event("Starlark builtin function call", "read", 21, 2, tid=4),
            event("Starlark builtin function call", "read", 90, 9),
            event("critical path component", "action 'OCI Image //example:image'", 30, 4),
            event("action processing", "OCI Image //example:image", 30, 4),
            event("critical path component", "action 'OCITarball archive.tar'", 34, 6),
            event("critical path component", "action 'Compressing archive.tar'", 40, 5),
            event("critical path component", "action 'Selecting debug APT additions'", 45, 3),
            event("critical path component", "action 'Validate source identities'", 48, 2),
            event("remote output download", "download outputs", 31, 4),
            event("Remote execution upload time", "upload outputs", 40, 8),
            event("Remote execution upload time", "upload missing blobs", 43, 4, tid=5),
        ]
        evidence = build_evidence(list(reversed(events)), summarize_profile(events))
        self.assertEqual(evidence, {
            "repository_seconds": 17, "repository_read_seconds": 11,
            "compile_seconds": 0, "foreign_seconds": 0, "fetch_seconds": 4,
            "packaging_seconds": 15, "cache_write_seconds": 8,
            "package_selection_seconds": 3, "validation_seconds": 2,
            "arm64_repository_seconds": 8, "armhf_repository_seconds": 5,
        })
        self.assertNotIn("must-not-publish", json.dumps(evidence))

    def test_repeated_builds_keep_query_waits_separate_and_require_complete_build_coverage(self):
        with tempfile.TemporaryDirectory() as tmp:
            owner = Path(tmp) / "docker-example.gz"
            for index in range(2):
                instance = owner / f"recipe-{index}"
                instance.mkdir(parents=True)
                (instance / "recipe.json").write_text(json.dumps({
                    "bazel": ["build", "cquery-example"], "profiles": ["build", "cquery-example"], "exports": []}))
                for phase, elapsed, lock in (("build", 30, 5), ("cquery-example", 101, 100)):
                    (instance / f"{phase}.timing.json").write_text(json.dumps({
                        "phase": phase, "elapsed_seconds": elapsed, "exit_code": 0}))
                    with gzip.open(instance / f"{phase}.profile.json.gz", "wt") as stream:
                        json.dump({"traceEvents": [
                            event("build phase marker", "Blocking on busy Bazel server (in client)", 0, lock),
                        ]}, stream)
            row, _ = read_owner(owner)
            self.assertEqual(build_nonlock(row), 50)
            self.assertEqual(row["lock_seconds"], 210)
            self.assertEqual(row["build"]["lock_seconds"], 10)
            self.assertEqual(row["details"]["phase_count"], 4)
            self.assertEqual(row["details"]["profiles"], 2)
            self.assertEqual(sum(p["lock_seconds"] for p in row["details"]["phases"]), 210)
            self.assertIn("Other-command lock wait: 200.00s", build_explanation(row))
            (instance / "build.profile.json.gz").unlink()
            row, _ = read_owner(owner)
            self.assertIsNone(build_nonlock(row))
            self.assertEqual(row["build"]["profiles"], 1)

    def test_named_spans_bound_public_labels_and_preserve_nested_read_overlap(self):
        events = [
            event("Starlark repository function call", "//:apt+example", 0, 20),
            event("Starlark builtin function call", "read", 1, 8),
            event("Starlark builtin function call", "read", 5, 8),
            event("Starlark builtin function call", "read", 0, 20, tid=2),
            event("Starlark repository function call", "local: --token=must-not-publish", 0, 99, tid=3),
            event("Starlark repository function call", "//:unsafe/home/path", 0, 99),
            event("critical path component", "action 'Compressing private input to dockers/example/output.gz'", 0, 100),
            event("critical path component", "action 'Compressing x to /home/private/output.gz'", 0, 200),
            event("critical path component", "action 'GoLink external/../private/output'", 0, 200),
            event("critical path component", "action 'OCI Image //example:../private'", 0, 200),
            event("critical path component", "action 'Unknown command --token=must-not-publish'", 0, 200),
            *[event("Starlark repository function call", f"//:apt+package-{i}", 0, i, tid=4 + i)
              for i in range(1, 11)],
            *[event("critical path component", f"action 'OCI Image //example:image-{i}'", 100, i)
              for i in range(1, 11)],
        ]
        details = named_spans(list(reversed(events)))
        self.assertEqual(details["repositories"][0], {
            "name": "//:apt+example", "seconds": 20, "read_count": 2, "read_seconds": 12,
        })
        self.assertEqual(details["critical_path"][0], {
            "kind": "Gzip", "target": "dockers/example/output.gz", "seconds": 100,
        })
        self.assertEqual(len(details["repositories"]), 8)
        self.assertEqual(len(details["critical_path"]), 8)
        self.assertNotIn("private", json.dumps(details))
        self.assertNotIn("must-not-publish", json.dumps(details))

    def test_workflow_appends_collapsible_evidence_and_emits_bounded_records(self):
        root = Path(__file__).resolve().parents[3]
        workflow = (root / ".github/workflows/bazel-oci.yml").read_text()
        block = workflow.split("      - name: Summarize VS Bazel build timings\n", 1)[1]
        script = textwrap.dedent(block.split("\n      - name:", 1)[0].split("        run: |\n", 1)[1])
        with tempfile.TemporaryDirectory() as tmp:
            work = Path(tmp)
            (work / "tools").symlink_to(root / "tools", target_is_directory=True)
            instance = work / "artifacts/vs/image-timings/docker-example.gz/recipe-one"
            instance.mkdir(parents=True)
            (instance / "recipe.json").write_text('{"bazel":["build"],"profiles":["build"],"exports":[]}')
            (instance / "build.timing.json").write_text(json.dumps({
                "phase": "build", "elapsed_seconds": 30, "exit_code": 0}))
            with gzip.open(instance / "build.profile.json.gz", "wt") as stream:
                json.dump({"traceEvents": [
                    event("build phase marker", "Blocking on busy Bazel server (in client)", 0, 5),
                    event("Starlark repository function call", "//:apt+example", 5, 12),
                    event("Starlark repository function call", "local: --token=must-not-publish", 5, 12),
                    event("critical path component", "action 'OCI Image //example:image'", 17, 8),
                ]}, stream)
            output = work / "step-summary.md"
            result = subprocess.run(["bash", "-euo", "pipefail", "-c", script], cwd=work,
                                    env=dict(os.environ, GITHUB_STEP_SUMMARY=str(output)),
                                    capture_output=True, text=True, check=True)
            self.assertIn("Why each target took its time", output.read_text())
            self.assertIn("<summary><strong>docker-example.gz</strong> - 25s BUILD EXCLUDING LOCK", output.read_text())
            self.assertIn("Repository preparation: 12.00s", output.read_text())
            self.assertIn("`//:apt+example`", output.read_text())
            self.assertIn("`//example:image`", output.read_text())
            self.assertIn("#### Command receipts (1/1 shown)", output.read_text())
            self.assertNotIn("must-not-publish", result.stdout + output.read_text())
            records = [json.loads(line.split(" ", 1)[1]) for line in result.stdout.splitlines()]
            self.assertEqual(records[-1], {"count": 1, "evidence_count": 3})
            self.assertEqual(records[0]["recipe"]["build"]["repository_seconds"], 12)
            self.assertEqual(records[1]["evidence"]["values"][0]["name"], "//:apt+example")
            self.assertTrue(all(len(line.encode()) <= 4096 for line in result.stdout.splitlines()))


if __name__ == "__main__":
    unittest.main()
