#!/usr/bin/env python3
"""Check that public CI uploads contain reviewed outputs and status fields."""

import ast
import json
import hashlib
from pathlib import Path
import subprocess
import shutil
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import public_artifacts as artifacts


REVISION = "1" * 40
DIGEST = "2" * 64
SENTINEL = "fixture_private_value_never_publish"
SYNTHETIC_JOB_TOKEN = "ghs_" + "x" * 36
SYNTHETIC_INSTALLATION_TOKEN = "ghs_12345_eyJ" + "a" * 12 + ".eyJ" + "b" * 12 + "." + "c" * 12


def write(root, name, value):
    path = root / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(value if isinstance(value, bytes) else value.encode())
    return path


def write_json(root, name, value):
    return write(root, name, json.dumps(value) + "\n")


def lock():
    return {"lockFileVersion": 25,
            "registryFileHashes": {"https://raw.githubusercontent.com/example/public/main/source.json": DIGEST},
            "moduleExtensions": {"//:extension.bzl%extension": {"general": {"envVariables": {}}}}}


def cache():
    actions = {name: {"runner": "processwrapper-sandbox", "cache_hit": False}
               for name in ("Genrule", "OCIImage", "GzipCompress")}
    return {"target": "//tools/bazel/tests:archive-fixture.gz",
            "fresh_checkouts": 2, "fresh_output_bases": 2, "fresh_containers": 0,
            **{phase: {"actions": actions, "archive_sha256": DIGEST}
               for phase in ("cold", "warm", "changed")}}


def cache_reuse(kind):
    return {
        "artifact_target": artifacts.SOURCE_ARTIFACT_TARGETS[kind], "phase": "before_tests",
        "action_count": 4, "cache_hit_count": 3, "executed_count": 1,
        "actions": [
            {"target": None, "mnemonic": "Tar", "runner": "processwrapper-sandbox",
             "cache_hit": False, "count": 1},
            {"target": "@@sonic-swss+//orchagent:orchagent", "mnemonic": "CppCompile",
             "runner": "disk cache hit", "cache_hit": True, "count": 3},
        ],
    }


def events(count=2, *, targets=None):
    values = [{"id": {"started": {}}, "started": {"command": "test", "buildToolVersion": "8.5.1",
               "host": SENTINEL, "user": SENTINEL, "optionsDescription": SENTINEL,
               "workingDirectory": "/home/runner/" + SENTINEL}},
              {"id": {"structuredCommandLine": {"commandLineLabel": "original"}},
               "structuredCommandLine": {"sections": [{"optionList": {"option": [
                   {"optionName": "client_env", "optionValue": "BAZELISK_GITHUB_TOKEN=" + SENTINEL}]}}]}},
              {"id": {"unstructuredCommandLine": {}},
               "unstructuredCommandLine": {"args": [
                   "--client_env=BAZELISK_GITHUB_TOKEN=" + SYNTHETIC_JOB_TOKEN,
                   "--client_env=PRIVATE_PATH=/home/runner/" + SENTINEL]}}]
    targets = targets if targets is not None else ["//tests:contract_" + str(number) for number in range(count)]
    for name in targets:
        values.extend([
            {"id": {"targetCompleted": {"label": name}},
             "completed": {"success": True, "outputGroup": [{"files": [{"uri": "file:///home/runner/" + SENTINEL}]}]}},
            {"id": {"testSummary": {"label": name}},
             "testSummary": {"overallStatus": "PASSED", "totalRunCount": 1,
                             "passed": [{"uri": "file:///home/runner/" + SENTINEL}]}},
        ])
    values.append({"id": {"buildFinished": {}}, "finished": {"overallSuccess": True, "exitCode": {"code": 0}}})
    return "".join(json.dumps(value) + "\n" for value in values)


def receipt(kind, architecture="amd64"):
    unsafe = {"commands": [{"argv": [SENTINEL]}], "error": SENTINEL}
    if kind == "python":
        return {"status": "passed", "architecture": architecture, **unsafe,
                "tests": {"//tests:python_" + str(number): {
                    "result": {"tests": 3, "failures": 0, "errors": 0, "skipped": 0},
                    "test.log": {"path": SENTINEL}}
                    for number in range(artifacts.RECEIPT_TEST_COUNTS["archive"][1])}}
    if kind == "source":
        return {"status": "passed", "revision": REVISION, **unsafe,
                "tests": {"//tests:contract_" + str(number): "passed" for number in range(2)},
                "validation": {"programs": [SENTINEL], "debug_pairs": [{"path": SENTINEL}],
                               "source_contract": {"dist/BUILD.bazel": DIGEST},
                               "artifact_sha256": {Path(name).name: hashlib.sha256(b"public build output\n").hexdigest()
                                                   for _, name, kind in artifacts.FILES["source"] if kind == "binary"}}}
    if kind == "syncd":
        return {"status": "passed", "targets": ["//tests:contract_" + str(number) for number in range(2)],
                "source_hashes": {"bazel/apt.lock.json": DIGEST}, "module_lock_sha256": DIGEST,
                "test_outputs": [SENTINEL], **unsafe}
    return {"status": "passed", "runtime_sha256": DIGEST, "dwp_sha256": DIGEST,
            "source_sha256": DIGEST, "dwp_size": 42, "compilation_units": 3,
            "with_dwp": {"found": True, "line": 7, "file": "/home/runner/" + SENTINEL,
                         "diagnostics": {"stderr": SENTINEL}},
            "without_dwp": {"error": SENTINEL}, "source_line": SENTINEL,
            "packages": {"runtime_sha256": DIGEST, "debug_sha256": DIGEST}}


def seed(root, kind, architecture="amd64"):
    for _logical, name, file_kind in artifacts.FILES[kind]:
        if file_kind == "binary":
            write(root, name, b"public build output\n")
        elif file_kind == "hashes":
            write(root, name, DIGEST + "  output.bin\n")
        elif file_kind == "version":
            write(root, name, "bazel 8.5.1\n")
        elif file_kind == "revision":
            write(root, name, REVISION + "\n")
        else:
            values = {"lock": lock(), "graph": {"key": "<root>", "name": "sonic-buildimage", "version": "", "dependencies": []}, "cache": cache()}
            write_json(root, name, values[file_kind])
    for _logical, name, count in artifacts.BEP[kind]:
        write(root, name, events(2 if count is None else count))
    receipts = {"archive": ("artifacts/config-engine/receipt.json", "python"),
                "source": ("artifacts/swss/receipt.json", "source"),
                "syncd": ("artifacts/syncd-vs/bazel/report.json", "syncd"),
                "vs": ("artifacts/vs/p4rt-debug-verification.json", "p4rt")}
    name, receipt_kind = receipts[kind]
    value = receipt(receipt_kind, architecture)
    if kind == "syncd":
        value["module_lock_sha256"] = hashlib.sha256((root / "artifacts/syncd-vs/bazel/MODULE.bazel.lock").read_bytes()).hexdigest()
    write_json(root, name, value)
    if kind == "syncd":
        write_json(root, "artifacts/syncd-vs/bazel/execution-gate-audit.json",
                   {"action_count": 12, "output_count": 24, "deb_outputs": [], "packaging_wrappers": [], "raw": SENTINEL})
    if kind == "vs":
        for source_kind in ("source", "syncd"):
            seed(root, source_kind, architecture)
            name = "artifacts/swss/receipt.json" if source_kind == "source" else "artifacts/syncd-vs/bazel/report.json"
            source_receipt = json.loads((root / name).read_text())
            source_receipt["cache_reuse"] = cache_reuse(source_kind)
            write_json(root, name, source_receipt)
            _, _, ready = artifacts.prepare(source_kind, root, root, "success", REVISION, architecture,
                                            head_sha="3" * 40, base_sha="4" * 40)
            assert ready, source_kind
        for name in ("sonic-vs.img.gz", "docker-orchagent.gz", "docker-orchagent-dbg.gz",
                     "docker-syncd-vs.gz", "docker-syncd-vs-dbg.gz"):
            write(root, "target/" + name, b"validated image output\n")
    write(root, "artifacts/unexpected.log", SENTINEL)


class PublicArtifactsTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="public-artifact-test-")
        self.root = Path(self.temporary.name)

    def tearDown(self):
        self.temporary.cleanup()

    def prepare(self, kind, status="success", architecture="amd64"):
        return artifacts.prepare(kind, self.root, self.root, status, REVISION, architecture,
                                 head_sha="3" * 40, base_sha="4" * 40)

    def test_receipt_counts_match_the_actual_producer_suites(self):
        """Adding a package suite must not make successful CI unpublishable."""
        for kind, filename in (
            ("archive", "tools/bazel/ci/python_packages.py"),
        ):
            tree = ast.parse((ROOT / filename).read_text())
            suites = next(ast.literal_eval(node.value) for node in tree.body
                          if isinstance(node, ast.Assign) and any(
                              isinstance(target, ast.Name) and target.id == "TESTS"
                              for target in node.targets))
            self.assertEqual(artifacts.RECEIPT_TEST_COUNTS[kind][1], len(suites), filename)

    def test_archive_test_counts_match_the_native_workflow(self):
        """New contract tests must not prevent successful CI artifact uploads."""
        workflow = (ROOT / ".github/workflows/bazel-oci.yml").read_text()
        for logical, event_path, count in artifacts.BEP["archive"]:
            with self.subTest(logical=logical):
                command = workflow.split(event_path, 1)[1].split("2>&1 | tee", 1)[0]
                labels = [line.strip().rstrip("\\").strip()
                          for line in command.splitlines() if line.strip().startswith("//")]
                self.assertEqual(len(labels), len(set(labels)))
                self.assertEqual(count, len(labels))

    def test_bep_summary_omits_environment_paths_and_raw_events(self):
        summary = artifacts.bep_summary(events(2).splitlines())
        self.assertEqual(len(summary["tests"]), 2)
        self.assertEqual(summary["finished"], {"success": True, "exit_code": 0})
        encoded = json.dumps(summary)
        self.assertNotIn(SENTINEL, encoded)
        self.assertNotIn(SYNTHETIC_JOB_TOKEN, encoded)
        self.assertNotIn("client_env", encoded)
        self.assertNotIn("BAZELISK_GITHUB_TOKEN", encoded)
        self.assertNotIn("file://", encoded)

    def test_cli_success_never_selects_secret_bearing_raw_inputs(self):
        for kind in ("archive", "source"):
            with self.subTest(kind=kind):
                root = self.root / kind
                root.mkdir()
                seed(root, kind)
                for name in ("artifacts/archive/agent-cache/cold/events.jsonl",
                             "artifacts/archive/agent-cache/warm/events.jsonl",
                             "artifacts/archive/agent-cache/changed/events.jsonl",
                             "artifacts/swss/build-events.jsonl",
                             "artifacts/config-engine/build-events.jsonl"):
                    write(root, name, events(1))
                write(root, "artifacts/unexpected.log", SYNTHETIC_JOB_TOKEN)
                output = root / "github-output"
                result = subprocess.run(
                    [sys.executable, "-B", artifacts.__file__, kind,
                     "--workspace", str(root), "--upload-root", str(root),
                     "--job-status", "success", "--revision", REVISION,
                     "--architecture", "amd64", "--github-output", str(output)],
                    capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                selected = output.read_text().splitlines()[1:-1]
                self.assertFalse(any(name.endswith((".jsonl", ".log", ".xml", "receipt.json"))
                                     for name in selected))
                self.assertIn(artifacts.SUMMARY[kind], selected)
                public_bytes = (result.stdout + result.stderr + output.read_text()).encode()
                public_bytes += b"".join((root / name).read_bytes() for name in selected)
                for private_value in (SYNTHETIC_JOB_TOKEN, SENTINEL, "BAZELISK_GITHUB_TOKEN"):
                    self.assertNotIn(private_value.encode(), public_bytes)

    def test_cli_failed_or_cancelled_jobs_publish_only_safe_partial_status(self):
        for status in ("failure", "cancelled"):
            for kind in ("archive", "source"):
                with self.subTest(status=status, kind=kind):
                    root = self.root / (status + "-" + kind)
                    root.mkdir()
                    seed(root, kind)
                    for _logical, name, _count in artifacts.BEP[kind]:
                        write(root, name, events(1) + '{"id":')
                    output = root / "github-output"
                    result = subprocess.run(
                        [sys.executable, "-B", artifacts.__file__, kind,
                         "--workspace", str(root), "--upload-root", str(root),
                         "--job-status", status, "--revision", REVISION,
                         "--architecture", "amd64", "--github-output", str(output)],
                        capture_output=True, text=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    selected = output.read_text().splitlines()[1:-1]
                    self.assertEqual(selected, [artifacts.SUMMARY[kind]])
                    summary_bytes = (root / selected[0]).read_bytes()
                    self.assertFalse(json.loads(summary_bytes)["complete"])
                    public_bytes = (result.stdout + result.stderr + output.read_text()).encode() + summary_bytes
                    for private_value in (SYNTHETIC_JOB_TOKEN, SENTINEL, "BAZELISK_GITHUB_TOKEN"):
                        self.assertNotIn(private_value.encode(), public_bytes)

    def test_success_selects_only_declared_outputs_and_summary(self):
        for kind in artifacts.FILES:
            with self.subTest(kind=kind):
                root = self.root / kind
                root.mkdir()
                seed(root, kind)
                summary, paths, ready = artifacts.prepare(kind, root, self.root, "success", REVISION, "amd64",
                                                         head_sha="3" * 40, base_sha="4" * 40)
                self.assertTrue(ready)
                self.assertTrue(summary["complete"])
                self.assertEqual(len(paths), len(artifacts.FILES[kind]) + 1)
                self.assertTrue(all(path.startswith(kind + "/") for path in paths))
                self.assertFalse(any(path.endswith((".log", ".jsonl", ".xml", "receipt.json")) for path in paths))
                self.assertNotIn(SENTINEL, json.dumps(summary))
                self.assertEqual(summary["architecture"], "amd64")

    def test_vs_success_reports_both_container_variants_and_the_installer(self):
        """A passing VS publication must account for runtime/debug archives from both OCI owners."""
        seed(self.root, "vs")
        for kind in ("source", "syncd"):
            path = self.root / artifacts.SUMMARY[kind]
            value = json.loads(path.read_text())
            value["private_diagnostics"] = SENTINEL
            write_json(self.root, artifacts.SUMMARY[kind], value)
        summary, paths, ready = self.prepare("vs")
        self.assertTrue(ready)
        self.assertTrue(summary["complete"])
        self.assertEqual(summary["receipts"]["outputs"], {
            name: {"bytes": len(b"validated image output\n")}
            for name in (
                "sonic-vs.img.gz", "docker-orchagent.gz", "docker-orchagent-dbg.gz",
                "docker-syncd-vs.gz", "docker-syncd-vs-dbg.gz",
            )
        })
        self.assertEqual(summary["receipts"]["source_validation"],
                         {kind: {"test_count": 2, "cache_reuse": {
                             key: value for key, value in cache_reuse(kind).items() if key != "actions"
                         }} for kind in ("source", "syncd")})
        self.assertEqual(paths, [artifacts.SUMMARY["vs"]])
        self.assertNotIn(SENTINEL, json.dumps(summary))

    def test_vs_requires_both_completed_source_summaries(self):
        """An installer and P4RT pass cannot replace the moved SWSS or Syncd checks."""
        for kind in ("source", "syncd"):
            for mode in ("missing", "null", "incomplete", "failed", "blocked", "missing-output"):
                with self.subTest(kind=kind, mode=mode):
                    seed(self.root, "vs")
                    path = self.root / artifacts.SUMMARY[kind]
                    value = json.loads(path.read_text())
                    if mode == "missing":
                        path.unlink()
                    else:
                        if mode == "null": value = None
                        elif mode == "incomplete": value["complete"] = False
                        elif mode == "failed": value["job_status"] = "failure"
                        elif mode == "blocked": value["blocked"] = [{"input": "tests", "reason": "failed"}]
                        else: value["missing"] = ["module-lock"]
                        write_json(self.root, artifacts.SUMMARY[kind], value)
                    summary, paths, ready = self.prepare("vs")
                    self.assertFalse(ready)
                    self.assertFalse(summary["complete"])
                    self.assertEqual(paths, [])
                    self.assertIn({"input": kind + "-public-summary", "reason": "invalid-source-validation"},
                                  summary["blocked"])

    def test_vs_rejects_source_summaries_from_a_different_build(self):
        """A passing result from another revision, PR merge or architecture is stale evidence."""
        replacements = {"kind": "archive", "revision": "5" * 40, "head_sha": "6" * 40,
                        "base_sha": "7" * 40, "architecture": "arm64"}
        for kind in ("source", "syncd"):
            for field, replacement in replacements.items():
                with self.subTest(kind=kind, field=field):
                    seed(self.root, "vs")
                    path = self.root / artifacts.SUMMARY[kind]
                    value = json.loads(path.read_text())
                    value[field] = replacement
                    write_json(self.root, artifacts.SUMMARY[kind], value)
                    summary, paths, ready = self.prepare("vs")
                    self.assertFalse(ready)
                    self.assertFalse(summary["complete"])
                    self.assertEqual(paths, [])

    def test_vs_rejects_failed_or_absent_tests_even_with_complete_source_status(self):
        """Do not trust a complete flag when the embedded test results contradict it."""
        for kind in ("source", "syncd"):
            for mode in ("no-tests", "failed-test", "unfinished-build", "truncated"):
                with self.subTest(kind=kind, mode=mode):
                    seed(self.root, "vs")
                    path = self.root / artifacts.SUMMARY[kind]
                    value = json.loads(path.read_text())
                    build = value["builds"][kind + "-tests"]
                    if mode == "no-tests": build["tests"] = {}
                    elif mode == "failed-test": next(iter(build["tests"].values()))["status"] = "FAILED"
                    elif mode == "unfinished-build": build["finished"] = None
                    else: build["truncated"] = True
                    write_json(self.root, artifacts.SUMMARY[kind], value)
                    summary, paths, ready = self.prepare("vs")
                    self.assertFalse(ready)
                    self.assertFalse(summary["complete"])
                    self.assertEqual(paths, [])

    def test_failed_vs_still_publishes_partial_status_without_source_summaries(self):
        """Early build failures must remain visible even when source checks never ran."""
        for status in ("failure", "cancelled"):
            with self.subTest(status=status):
                summary, paths, ready = self.prepare("vs", status)
                self.assertTrue(ready)
                self.assertFalse(summary["complete"])
                self.assertEqual(paths, [artifacts.SUMMARY["vs"]])
                self.assertNotIn("source_validation", summary["receipts"])

    def test_source_publication_keeps_optional_cache_action_counts(self):
        """Cold jobs remain supported; post-build receipts retain misses as evidence."""
        for kind in ("source", "syncd"):
            with self.subTest(kind=kind):
                seed(self.root, kind)
                summary, _, ready = self.prepare(kind)
                self.assertTrue(ready)
                self.assertNotIn("cache_reuse", summary["receipts"][kind])
                name = "artifacts/swss/receipt.json" if kind == "source" else "artifacts/syncd-vs/bazel/report.json"
                value = json.loads((self.root / name).read_text())
                value["cache_reuse"] = cache_reuse(kind)
                write_json(self.root, name, value)
                summary, _, ready = self.prepare(kind)
                self.assertTrue(ready)
                self.assertEqual(summary["receipts"][kind]["cache_reuse"], cache_reuse(kind))
                self.assertNotIn(SENTINEL, json.dumps(summary))

    def test_cache_action_counts_reject_raw_fields_and_inconsistent_evidence(self):
        """Only complete aggregate action counts can cross the public artifact boundary."""
        for mode in ("raw-command", "raw-action", "wrong-phase", "missing-actions", "no-actions",
                     "wrong-total", "wrong-hits", "wrong-executed", "boolean-count", "zero-count",
                     "nonboolean-hit", "unsafe-target", "unsafe-runner", "unsafe-mnemonic",
                     "duplicate", "unsorted", "false-cache-hit", "miss-with-cache-runner"):
            with self.subTest(mode=mode):
                value = cache_reuse("source")
                if mode == "raw-command": value["argv"] = [SENTINEL]
                elif mode == "raw-action": value["actions"][0]["commandArgs"] = [SENTINEL]
                elif mode == "wrong-phase": value["phase"] = "after_tests"
                elif mode == "missing-actions": del value["actions"]
                elif mode == "no-actions": value.update(action_count=0, cache_hit_count=0, executed_count=0, actions=[])
                elif mode == "wrong-total": value["action_count"] += 1
                elif mode == "wrong-hits": value["cache_hit_count"] += 1
                elif mode == "wrong-executed": value["executed_count"] += 1
                elif mode == "boolean-count": value["actions"][0]["count"] = True
                elif mode == "zero-count": value["actions"][0]["count"] = 0
                elif mode == "nonboolean-hit": value["actions"][0]["cache_hit"] = 1
                elif mode == "unsafe-target": value["actions"][0]["target"] = "/home/runner/" + SENTINEL
                elif mode == "unsafe-runner": value["actions"][0]["runner"] = "/home/runner/" + SENTINEL
                elif mode == "unsafe-mnemonic": value["actions"][0]["mnemonic"] = "compile\n" + SENTINEL
                elif mode == "duplicate": value["actions"].append(value["actions"][0].copy())
                elif mode == "unsorted": value["actions"].reverse()
                elif mode == "false-cache-hit":
                    value["actions"][0]["cache_hit"] = True
                    value.update(cache_hit_count=4, executed_count=0)
                else:
                    value["actions"][1]["cache_hit"] = False
                    value.update(cache_hit_count=0, executed_count=4)
                for kind in ("source", "syncd"):
                    source_receipt = receipt(kind)
                    source_receipt["cache_reuse"] = value
                    with self.assertRaises(artifacts.PublicArtifactError):
                        artifacts.receipt_summary(source_receipt, kind)

    def test_vs_requires_cache_evidence_for_each_owners_production_artifacts(self):
        """Passing tests alone cannot prove reuse of Make's production packages."""
        for kind in ("source", "syncd"):
            for mode in ("missing", "wrong-target", "after-tests", "inconsistent-count"):
                with self.subTest(kind=kind, mode=mode):
                    seed(self.root, "vs")
                    path = self.root / artifacts.SUMMARY[kind]
                    value = json.loads(path.read_text())
                    if mode == "missing": del value["receipts"][kind]["cache_reuse"]
                    elif mode == "wrong-target": value["receipts"][kind]["cache_reuse"]["artifact_target"] = "//tools/bazel/tests:ci_artifacts"
                    elif mode == "after-tests": value["receipts"][kind]["cache_reuse"]["phase"] = "after_tests"
                    else: value["receipts"][kind]["cache_reuse"]["action_count"] += 1
                    write_json(self.root, artifacts.SUMMARY[kind], value)
                    summary, paths, ready = self.prepare("vs")
                    self.assertFalse(ready)
                    self.assertFalse(summary["complete"])
                    self.assertEqual(paths, [])
                    self.assertIn({"input": kind + "-public-summary", "reason": "invalid-source-validation"},
                                  summary["blocked"])

    def test_vs_missing_syncd_debug_archive_blocks_successful_publication(self):
        """A successful installer/P4RT result cannot hide an unbuilt Syncd debug image."""
        seed(self.root, "vs")
        (self.root / "target/docker-syncd-vs-dbg.gz").unlink()
        summary, paths, ready = self.prepare("vs")
        self.assertFalse(ready)
        self.assertFalse(summary["complete"])
        self.assertEqual(paths, [])
        self.assertEqual(summary["missing"], ["output-docker-syncd-vs-dbg.gz"])
        self.assertEqual(len(summary["receipts"]["outputs"]), 4)
        self.assertNotIn("docker-syncd-vs-dbg.gz", summary["receipts"]["outputs"])
        self.assertNotIn(SENTINEL, json.dumps(summary))

    def test_archive_supports_native_arm64_metadata(self):
        seed(self.root, "archive", "arm64")
        summary, _paths, ready = self.prepare("archive", architecture="arm64")
        self.assertTrue(ready)
        self.assertEqual(summary["receipts"]["python"]["architecture"], "arm64")

    def test_previous_python_receipt_cannot_cover_new_yang_checks(self):
        """Neither the original nor shared-base receipts cover the YANG checks."""
        seed(self.root, "archive")
        for previous_count in (5, 9, 11):
            with self.subTest(previous_count=previous_count):
                value = receipt("python")
                value["tests"] = dict(list(value["tests"].items())[:previous_count])
                write_json(self.root, "artifacts/config-engine/receipt.json", value)
                summary, paths, ready = self.prepare("archive")
                self.assertFalse(ready)
                self.assertEqual(paths, [])
                self.assertIn({"input": "python-receipt", "reason": "incomplete-receipt"}, summary["blocked"])

    def test_container_jobs_accept_any_nonempty_declared_test_inventory(self):
        """Container CI may add tests in BUILD without changing a hard-coded Python count."""
        targets = ["//sample:first", "//sample:second", "@@component+//native:third"]
        for kind in ("source", "syncd"):
            with self.subTest(kind=kind):
                seed(self.root, kind)
                bep = artifacts.BEP[kind][0][1]
                write(self.root, bep, events(targets=targets))
                name = "artifacts/swss/receipt.json" if kind == "source" else "artifacts/syncd-vs/bazel/report.json"
                value = json.loads((self.root / name).read_text())
                if kind == "source": value["tests"] = dict.fromkeys(targets, "passed")
                else: value["targets"] = targets
                write_json(self.root, name, value)
                summary, paths, ready = self.prepare(kind)
                self.assertTrue(ready, summary)
                self.assertTrue(paths)
                self.assertEqual(set(summary["builds"][kind + "-tests"]["tests"]), set(targets))

    def test_container_test_evidence_must_match_exact_declared_identities(self):
        """Equal counts, extra tests or missing tests cannot disguise the wrong validation graph."""
        mutations = {
            "different-same-count": ["//foreign:one", "//foreign:two"],
            "missing": ["//tests:contract_0"],
            "extra": ["//tests:contract_0", "//tests:contract_1", "//foreign:extra"],
        }
        for kind in ("source", "syncd"):
            for mode, targets in mutations.items():
                with self.subTest(kind=kind, mode=mode):
                    seed(self.root, kind)
                    write(self.root, artifacts.BEP[kind][0][1], events(targets=targets))
                    summary, paths, ready = self.prepare(kind)
                    self.assertFalse(ready, summary)
                    self.assertEqual(paths, [])

    def test_empty_container_suite_cannot_publish_success(self):
        """Matching empty BEP and declared inventories still provide no successful validation."""
        for kind in ("source", "syncd"):
            with self.subTest(kind=kind):
                seed(self.root, kind)
                write(self.root, artifacts.BEP[kind][0][1], events(targets=[]))
                name = "artifacts/swss/receipt.json" if kind == "source" else "artifacts/syncd-vs/bazel/report.json"
                value = json.loads((self.root / name).read_text())
                if kind == "source": value["tests"] = {}
                else: value["targets"] = []
                write_json(self.root, name, value)
                summary, paths, ready = self.prepare(kind)
                self.assertFalse(ready, summary)
                self.assertEqual(paths, [])

    def test_source_publication_requires_exact_validated_archive_hashes(self):
        """Do not upload untested, missing or changed archives under a passing source receipt."""
        for change in ("missing-bindings", "missing-archive", "extra-archive", "changed-bytes"):
            with self.subTest(change=change):
                seed(self.root, "source")
                name = "artifacts/swss/receipt.json"
                value = json.loads((self.root / name).read_text())
                validated = value["validation"]["artifact_sha256"]
                if change == "missing-bindings": del value["validation"]["artifact_sha256"]
                elif change == "missing-archive": del validated["swss.tar"]
                elif change == "extra-archive": validated["unpublished.tar"] = DIGEST
                else: write(self.root, "artifacts/swss/swss.tar", b"different configured output")
                write_json(self.root, name, value)
                summary, paths, ready = self.prepare("source")
                self.assertFalse(ready, summary)
                self.assertEqual(paths, [])
                self.assertIn({"input": "source-receipt", "reason": "validated-artifact-mismatch"}, summary["blocked"])

    def test_source_summary_keeps_safe_tested_archive_bindings(self):
        """Preserve the exact filename-to-digest proof in the public source summary."""
        seed(self.root, "source")
        summary, _, ready = self.prepare("source")
        self.assertTrue(ready)
        self.assertEqual(summary["receipts"]["source"]["artifact_sha256"],
                         receipt("source")["validation"]["artifact_sha256"])

    def test_archive_binding_names_and_digests_are_validated_before_publication(self):
        """Archive bindings cannot leak paths or disguise malformed digests as validation evidence."""
        for value in ({"../secret.tar": DIGEST}, {"subdir/output.tar": DIGEST}, {"output.tar": SENTINEL}, []):
            with self.subTest(value=value), self.assertRaises(artifacts.PublicArtifactError):
                document = receipt("source")
                document["validation"]["artifact_sha256"] = value
                artifacts.receipt_summary(document, "source")

    def test_source_requires_package_validation_evidence(self):
        """A discovered test suite cannot replace required archive/program/debug/source checks."""
        for missing in ("validation", "programs", "debug_pairs", "source_contract"):
            with self.subTest(missing=missing):
                seed(self.root, "source")
                name = "artifacts/swss/receipt.json"
                value = json.loads((self.root / name).read_text())
                if missing == "validation": del value["validation"]
                else: del value["validation"][missing]
                write_json(self.root, name, value)
                summary, paths, ready = self.prepare("source")
                self.assertFalse(ready, summary)
                self.assertEqual(paths, [])

    def test_container_failed_status_cannot_publish_success(self):
        """Complete test identities cannot override a failed runner receipt or report."""
        for kind in ("source", "syncd"):
            with self.subTest(kind=kind):
                seed(self.root, kind)
                name = "artifacts/swss/receipt.json" if kind == "source" else "artifacts/syncd-vs/bazel/report.json"
                value = json.loads((self.root / name).read_text())
                value["status"] = "failed"
                write_json(self.root, name, value)
                summary, paths, ready = self.prepare(kind)
                self.assertFalse(ready, summary)
                self.assertEqual(paths, [])

    def test_archive_requires_all_twelve_native_workflow_tests(self):
        for architecture in ("amd64", "arm64"):
            with self.subTest(architecture=architecture):
                seed(self.root, "archive", architecture)
                event_path = "artifacts/archive/test-events.jsonl"
                write(self.root, event_path, events(12))
                summary, paths, ready = self.prepare("archive", architecture=architecture)
                self.assertTrue(ready)
                self.assertEqual(len(summary["builds"]["archive-tests"]["tests"]), 12)
                self.assertTrue(paths)

                write(self.root, event_path, events(11))
                summary, paths, ready = self.prepare("archive", architecture=architecture)
                self.assertFalse(ready)
                self.assertEqual(paths, [])
                self.assertIn({"input": "archive-tests", "reason": "invalid-bep-summary"},
                              summary["blocked"])

    def test_failure_publishes_only_a_safe_partial_summary(self):
        seed(self.root, "archive")
        name = artifacts.BEP["archive"][0][1]
        write(self.root, name, events(1).rsplit("\n", 2)[0] + "\n{\"id\":")
        failed = receipt("python")
        failed["status"] = "failed"
        write_json(self.root, "artifacts/config-engine/receipt.json", failed)
        summary, paths, ready = self.prepare("archive", "failure")
        self.assertTrue(ready)
        self.assertFalse(summary["complete"])
        self.assertEqual(paths, [artifacts.SUMMARY["archive"]])
        self.assertEqual(summary["files"], {})
        self.assertTrue(summary["builds"]["archive-tests"]["truncated"])
        self.assertNotIn(SENTINEL, json.dumps(summary))

    def test_missing_required_output_blocks_a_success_upload(self):
        seed(self.root, "source")
        (self.root / "artifacts/swss/swss.tar").unlink()
        summary, paths, ready = self.prepare("source")
        self.assertFalse(ready)
        self.assertEqual(paths, [])
        self.assertIn("swss-tar", summary["missing"])

    def test_mismatched_receipt_identity_blocks_upload(self):
        cases = (
            ("archive", "artifacts/config-engine/receipt.json", "architecture", "arm64", "architecture-mismatch"),
            ("source", "artifacts/swss/receipt.json", "revision", "5" * 40, "revision-mismatch"),
            ("syncd", "artifacts/syncd-vs/bazel/report.json", "module_lock_sha256", "6" * 64, "module-lock-mismatch"),
        )
        for kind, name, field, value, reason in cases:
            with self.subTest(kind=kind):
                root = self.root / kind
                root.mkdir()
                seed(root, kind)
                path = root / name
                data = json.loads(path.read_text())
                data[field] = value
                path.write_text(json.dumps(data))
                summary, paths, ready = artifacts.prepare(kind, root, self.root, "success", REVISION, "amd64")
                self.assertFalse(ready)
                self.assertEqual(paths, [])
                self.assertIn(reason, [item["reason"] for item in summary["blocked"]])

    def test_unsafe_dependency_data_blocks_upload_without_returning_it(self):
        seed(self.root, "source")
        value = lock()
        value["facts"] = {"authorization": SENTINEL}
        write_json(self.root, "artifacts/swss/MODULE.bazel.lock", value)
        summary, paths, ready = self.prepare("source")
        self.assertFalse(ready)
        self.assertEqual(paths, [])
        self.assertIn({"input": "module-lock", "reason": "unsafe-public-file"}, summary["blocked"])
        self.assertNotIn(SENTINEL, json.dumps(summary))

    def test_recorded_environment_must_be_empty(self):
        value = lock()
        artifacts.dependency_json(value, "lock")
        value["moduleExtensions"]["//:extension.bzl%extension"]["general"]["envVariables"] = {"FIXTURE_VALUE": SENTINEL}
        with self.assertRaises(artifacts.PublicArtifactError):
            artifacts.dependency_json(value, "lock")

    def test_installation_job_token_in_dependency_data_blocks_upload(self):
        unsafe_lock = lock()
        unsafe_lock["facts"] = {"fixture": SYNTHETIC_INSTALLATION_TOKEN}
        unsafe_graph = {"key": "<root>", "name": SYNTHETIC_INSTALLATION_TOKEN,
                        "version": "", "dependencies": []}
        for logical, path, value in (
                ("module-lock", "MODULE.bazel.lock", unsafe_lock),
                ("module-graph", "module-graph.json", unsafe_graph)):
            with self.subTest(logical=logical):
                seed(self.root, "source")
                write_json(self.root, "artifacts/swss/" + path, value)
                summary, paths, ready = self.prepare("source")
                self.assertFalse(ready)
                self.assertEqual(paths, [])
                self.assertIn({"input": logical, "reason": "unsafe-public-file"}, summary["blocked"])
                self.assertNotIn(SYNTHETIC_INSTALLATION_TOKEN, json.dumps(summary))

    def test_duplicate_json_keys_and_sensitive_urls_are_rejected(self):
        with self.assertRaises(artifacts.PublicArtifactError):
            artifacts.load_json('{"facts": {"one": 1}, "facts": {"two": 2}}')
        for value in ("https://name:fixture@example.com/source", "https://example.com/source?token=fixture",
                      "/root/.cache/fixture", "/home/example/code/project", "/home/example/.ssh/id_rsa"):
            with self.subTest(value=value), self.assertRaises(artifacts.PublicArtifactError):
                artifacts.safe_text(value)
        artifacts.safe_text("./home/admin/.profile")

    def test_secret_formats_are_rejected_using_synthetic_values(self):
        for value in ("ghp_" + "x" * 24, SYNTHETIC_INSTALLATION_TOKEN, "sk-" + "x" * 24, "AKIA" + "A" * 16,
                      "eyJ" + "a" * 12 + ".eyJ" + "b" * 12 + "." + "c" * 12):
            with self.subTest(prefix=value[:4]), self.assertRaises(artifacts.PublicArtifactError):
                artifacts.safe_text(value)

    def test_symlink_inputs_and_external_summary_directories_are_rejected(self):
        seed(self.root, "source")
        output = self.root / "artifacts/swss/swss.tar"
        output.unlink()
        outside = self.root.parent / (self.root.name + "-outside")
        outside.mkdir()
        self.addCleanup(lambda: outside.rmdir())
        output.symlink_to(outside / "not-read")
        summary, paths, ready = self.prepare("source")
        self.assertFalse(ready)
        self.assertEqual(paths, [])
        self.assertIn({"input": "swss-tar", "reason": "unreadable-or-unsafe-input"}, summary["blocked"])
        other = self.root / "other"
        other.mkdir()
        (other / "artifacts").symlink_to(outside, target_is_directory=True)
        with self.assertRaises(artifacts.PublicArtifactError):
            artifacts.prepare("vs", other, self.root, "failure", REVISION, "amd64")

    def test_symlink_input_directories_are_rejected(self):
        seed(self.root, "source")
        original = self.root / "artifacts/swss"
        alias = self.root / "alias"
        shutil.move(original, alias)
        original.symlink_to(alias, target_is_directory=True)
        with self.assertRaises(artifacts.PublicArtifactError):
            self.prepare("source")

    def test_receipts_keep_validation_fields_and_omit_diagnostics(self):
        for kind in ("python", "source", "syncd", "p4rt"):
            with self.subTest(kind=kind):
                summary = artifacts.receipt_summary(receipt(kind), kind)
                artifacts.safe_json(summary)
                self.assertNotIn(SENTINEL, json.dumps(summary))
        p4rt = artifacts.receipt_summary(receipt("p4rt"), "p4rt")
        self.assertTrue(p4rt["source_found"])
        self.assertEqual(p4rt["source_line_number"], 7)
        self.assertEqual(p4rt["packages"]["debug_sha256"], DIGEST)

    def test_malformed_receipts_and_text_fail_closed(self):
        invalid = receipt("python")
        invalid["tests"] = []
        with self.assertRaises(artifacts.PublicArtifactError):
            artifacts.receipt_summary(invalid, "python")
        for value, kind in ((SENTINEL, "revision"), (DIGEST + "  /absolute/path\n", "hashes"), ("bazel " + SENTINEL, "version")):
            with self.subTest(kind=kind), self.assertRaises(artifacts.PublicArtifactError):
                artifacts.public_text(value, kind)

    def test_cli_emits_an_explicit_upload_list_after_validation(self):
        seed(self.root, "syncd")
        output = self.root / "github-output"
        result = subprocess.run([sys.executable, "-B", artifacts.__file__, "syncd", "--workspace", str(self.root),
                                 "--upload-root", str(self.root), "--job-status", "success", "--revision", REVISION,
                                 "--architecture", "amd64", "--github-output", str(output)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(json.loads(result.stdout)["ready"])
        lines = output.read_text().splitlines()
        self.assertEqual(lines[0], "paths<<SONIC_PUBLIC_ARTIFACT_PATHS")
        self.assertEqual(lines[-1], "SONIC_PUBLIC_ARTIFACT_PATHS")
        self.assertIn(artifacts.SUMMARY["syncd"], lines)
        self.assertFalse(any(line.endswith((".log", ".jsonl", ".xml")) for line in lines))
        self.assertNotIn(SENTINEL, output.read_text())


if __name__ == "__main__":
    unittest.main()
