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


def events(count):
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
    for number in range(count):
        name = "//tests:contract_" + str(number)
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
                "tests": {"//tests:source_" + str(number): "passed" for number in range(14)},
                "validation": {"programs": [SENTINEL], "debug_pairs": [{"path": SENTINEL}],
                               "source_contract": {"dist/BUILD.bazel": DIGEST}}}
    if kind == "syncd":
        return {"targets": ["//tests:syncd_" + str(number) for number in range(7)],
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
        write(root, name, events(count))
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
        for name in ("sonic-vs.img.gz", "docker-orchagent.gz", "docker-orchagent-dbg.gz"):
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
            ("source", "dockers/docker-orchagent/bazel/ci.py"),
        ):
            tree = ast.parse((ROOT / filename).read_text())
            suites = next(ast.literal_eval(node.value) for node in tree.body
                          if isinstance(node, ast.Assign) and any(
                              isinstance(target, ast.Name) and target.id == "TESTS"
                              for target in node.targets))
            self.assertEqual(artifacts.RECEIPT_TEST_COUNTS[kind][1], len(suites), filename)

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
                summary, paths, ready = artifacts.prepare(kind, root, self.root, "success", REVISION, "amd64")
                self.assertTrue(ready)
                self.assertTrue(summary["complete"])
                self.assertEqual(len(paths), len(artifacts.FILES[kind]) + 1)
                self.assertTrue(all(path.startswith(kind + "/") for path in paths))
                self.assertFalse(any(path.endswith((".log", ".jsonl", ".xml", "receipt.json")) for path in paths))
                self.assertNotIn(SENTINEL, json.dumps(summary))
                self.assertEqual(summary["architecture"], "amd64")

    def test_archive_supports_native_arm64_metadata(self):
        seed(self.root, "archive", "arm64")
        summary, _paths, ready = self.prepare("archive", architecture="arm64")
        self.assertTrue(ready)
        self.assertEqual(summary["receipts"]["python"]["architecture"], "arm64")

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
