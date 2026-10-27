"""Exercise declared container CI targets with a controlled Bazel process."""

import contextlib
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import command_log, container
from tools.bazel.tests.integration import oci_source_layers_test


class ContainerRunnerTest(unittest.TestCase):
    """Protect the execution gate and bind published evidence to declared targets."""

    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        for name in container.SOURCE_FILES:
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("shared checked input\n")
        (self.root / ".bazelversion").write_text("8.5.1\n")
        (self.root / "checked-input.json").write_text("{}\n")
        (self.root / "preparation.mk").write_text("PREPARED_INPUT := sample\n")
        (self.root / "BUILD.bazel").write_text("# owner declarations\n")
        self.owner = "//dockers/example"
        self.tests = ["//sample:first_test", "//sample:second_test"]
        self.external = "@unrelated_component//native:external_test"
        self.canonical = "@@unrelated-component+//native:external_test"
        self.bazel = self.root / "bazel"
        self.bazel.write_text("#!" + sys.executable + "\n" + r'''
import hashlib
import json
from pathlib import Path
import sys
root = Path.cwd()
mode = (root / "mode").read_text()
command = sys.argv[1]
targets = ["//sample:first_test", "//sample:second_test"]
if mode == "external": targets.append("@unrelated_component//native:external_test")
with (root / "commands.jsonl").open("a") as stream:
    stream.write(json.dumps(sys.argv[1:]) + "\n")
if command == "--version":
    print("bazel 7.0.0" if mode == "wrong-version" else "bazel 8.5.1")
elif command == "query":
    if any(arg.startswith("tests(") for arg in sys.argv[2:]):
        if mode != "empty-suite": print("\n".join(targets))
    else:
        print("//:checked-input.json\n//:BUILD.bazel")
elif command == "aquery":
    action = {"outputIds": [1], "environmentVariables": [
        {"key": "PRIVATE", "value": "private-action-data"}]}
    if mode == "wrapper": action["arguments"] = ["make", "package"]
    print(json.dumps({"actions": [action],
        "artifacts": [{"id": 1, "pathFragmentId": 1}],
        "pathFragments": [{"id": 1, "label": "unsafe.deb" if mode == "deb" else "safe.tar"}]}))
    print("action query diagnostic", file=sys.stderr)
    if mode == "query-failure": sys.exit(7)
elif command == "cquery":
    if "--output=files" in sys.argv:
        if mode not in ("empty-artifacts", "validation-missing-archive"): print("bazel-bin/package.tar")
        if mode == "validation-extra-archive": print("bazel-bin/unvalidated.tar")
    else:
        print("@@unrelated-component+//native:external_test")
elif command == "build":
    output = root / "bazel-bin/package.tar"
    output.parent.mkdir(exist_ok=True)
    output.write_bytes(b"different configuration" if mode == "validation-changed-archive" else b"archive fixture")
    if mode == "validation-extra-archive": (root / "bazel-bin/unvalidated.tar").write_bytes(b"not validated")
    event_path = next(arg.split("=", 1)[1] for arg in sys.argv if arg.startswith("--build_event_json_file="))
    events = [
        {"id": {"started": {}}, "started": {"command": "build", "buildToolVersion": "8.5.1"}},
        {"id": {"targetCompleted": {"label": sys.argv[-1]}}, "completed": {"success": True}},
        {"id": {"buildFinished": {}}, "finished": {"overallSuccess": True, "exitCode": {"code": 0}}},
        {"id": {"buildMetrics": {}}, "lastMessage": True},
    ]
    if mode == "cache-incomplete-build": events.pop()
    Path(event_path).write_text("".join(json.dumps(event) + "\n" for event in events))
    logs = [arg.split("=", 1)[1] for arg in sys.argv if arg.startswith("--execution_log_json_file=")]
    if logs:
        log = Path(logs[0])
        assert log.stat().st_mode & 0o777 == 0o600
        record = {"targetLabel": "@@sonic_swss+//orchagent:orchagent", "mnemonic": "CppCompile",
                  "runner": "disk cache hit", "cacheHit": True,
                  "commandArgs": ["private-action-data"],
                  "environmentVariables": [{"name": "PRIVATE", "value": "private-action-data"}]}
        records = [record, record]
        if mode == "cache-executed":
            records.append({**record, "targetLabel": "//dockers/example:ci_copy", "mnemonic": "CopyFile",
                            "runner": "processwrapper-sandbox", "cacheHit": False})
        log.write_text("\n".join(json.dumps(record, indent=2) for record in records) + "\n")
        if mode == "cache-missing": log.unlink()
        elif mode == "cache-empty": log.write_text("")
        elif mode == "cache-malformed": log.write_text('{"mnemonic":')
        elif mode == "cache-failed-build": sys.exit(11)
elif command == "test":
    if mode == "test-failure": sys.exit(9)
    events = []
    for target in targets:
        name = target.rsplit(":", 1)[1]
        configuration = "default" if name == "first_test" else "python-3.11"
        directory = root / "configured testlogs" / configuration / name
        directory.mkdir(parents=True)
        outputs = []
        filenames = ["test.log", "test.xml"]
        if (mode.startswith("validation") or mode in ("multiple-validations", "invalid-validation")) and (name == "first_test" or mode == "multiple-validations"):
            filenames.append("test.outputs/validation.json")
        for filename in filenames:
            if mode == "missing-evidence" and name == "second_test" and filename == "test.xml": continue
            output = directory / filename
            output.parent.mkdir(exist_ok=True)
            value = "passed\n"
            if filename.endswith("validation.json"):
                validation = {"programs": ["usr/bin/example"],
                              "artifact_sha256": {"package.tar": hashlib.sha256(b"archive fixture").hexdigest()}}
                if mode == "validation-missing-bindings": del validation["artifact_sha256"]
                value = "not json\n" if mode == "invalid-validation" else json.dumps(validation)
            output.write_text(value)
            outputs.append({"name": filename, "uri": output.as_uri()})
        event_label = target.replace("@unrelated_component//", "@@unrelated-component+//")
        events.append({"id": {"testResult": {"label": event_label, "configuration": {"id": configuration}}},
                       "testResult": {"testActionOutput": outputs}})
    event_path = next(arg.split("=", 1)[1] for arg in sys.argv if arg.startswith("--build_event_json_file="))
    Path(event_path).write_text("".join(json.dumps(event) + "\n" for event in events))
    (root / "MODULE.bazel.lock").write_text("{}\n")
    if mode == "source-change": (root / "checked-input.json").write_text("changed\n")
    if mode == "preparation-change": (root / "preparation.mk").write_text("changed\n")
else:
    raise SystemExit("unexpected command: " + command)
''')
        self.bazel.chmod(0o755)
        self.artifacts = self.root / "artifacts"
        self.executed = []

    def execute(self, command, directory, receipt, name, **kwargs):
        self.executed.append(list(command))
        if command[0] == "git":
            mode = (self.root / "mode").read_text()
            if command[1] == "check-ignore" and mode == "unignored-lock":
                raise subprocess.CalledProcessError(1, command)
            receipt["commands"].append({"argv": command, "returncode": 0,
                                        "elapsed_seconds": 0, "log": name + ".log"})
            result = "a" * 40 + "\n" if command[1] == "rev-parse" else ""
            if command[1] == "ls-files" and mode == "tracked-lock":
                result = "MODULE.bazel.lock\n"
            (directory / (name + ".log")).write_text(result)
            return result
        return self.original_execute(command, directory, receipt, name, **kwargs)

    def run_ci(self, mode="pass", *, error=None, resolution_error=None, machine="x86_64", source_files=(), reuse_build=False):
        (self.root / "mode").write_text(mode)
        console = io.StringIO()
        self.original_execute = command_log.execute
        with mock.patch.object(container.platform, "machine", return_value=machine), \
             mock.patch.object(container.platform, "freedesktop_os_release", return_value={"VERSION_CODENAME": "trixie"}), \
             mock.patch.object(command_log, "execute", side_effect=self.execute), \
             mock.patch.object(container.resolution, "collect", side_effect=resolution_error, return_value={}), \
             mock.patch.object(container, "check_versions"), \
             contextlib.redirect_stdout(console), contextlib.redirect_stderr(console):
            if error is None:
                container.run(self.owner, workspace=self.root, artifacts=self.artifacts,
                              bazel=str(self.bazel), options=["--jobs=2"], source_files=source_files,
                              reuse_build=reuse_build)
            else:
                with self.assertRaises(error):
                    container.run(self.owner, workspace=self.root, artifacts=self.artifacts,
                                  bazel=str(self.bazel), options=["--jobs=2"], source_files=source_files,
                                  reuse_build=reuse_build)
        self.assertFalse((self.artifacts / "actions.raw.json").exists())
        self.assertFalse((self.artifacts / "tests.raw.json").exists())
        self.assertFalse((self.artifacts / "artifact-execution.raw.json").exists())
        self.assertNotIn("private-action-data", console.getvalue())
        self.console = console.getvalue()
        for path in self.artifacts.rglob("*"):
            if path.is_file(): self.assertNotIn("private-action-data", path.read_text())
        return json.loads((self.artifacts / "receipt.json").read_text())

    def bazel_commands(self):
        path = self.root / "commands.jsonl"
        return [json.loads(line) for line in path.read_text().splitlines()] if path.exists() else []

    def assert_failed_without_report(self, receipt):
        self.assertEqual(receipt["status"], "failed")
        self.assertFalse((self.artifacts / "report.json").exists())

    def assert_no_actions_executed(self):
        self.assertFalse(any(item[0] in ("test", "build") for item in self.bazel_commands()))

    def test_success_discovers_tests_and_artifacts_without_owner_python(self):
        """Discover each suite from Bazel and retain evidence for its concrete tests and files."""
        receipt = self.run_ci()
        self.assertEqual(receipt["status"], "passed")
        commands = self.bazel_commands()
        suite_query = next(command for command in commands if command[0] == "query" and "--strict_test_suite" in command)
        self.assertIn('tests("' + self.owner + ':ci_tests")', suite_query)
        self.assertIn("--output=label", suite_query)
        audit = next(command for command in commands if command[0] == "aquery")
        self.assertIn("deps(set(" + " ".join([*self.tests, self.owner + ":ci_artifacts"]) + "))", audit)
        test = next(command for command in commands if command[0] == "test")
        self.assertEqual(test[-1], self.owner + ":ci_tests")
        self.assertIn("--nocache_test_results", test)
        self.assertIn("--nozip_undeclared_test_outputs", test)
        operations = [command[0] for command in commands]
        self.assertLess(operations.index("aquery"), operations.index("test"))
        self.assertLess(operations.index("aquery"), operations.index("build"))
        self.assertLess(operations.index("test"), operations.index("build"))
        self.assertNotIn("cache_reuse", receipt)
        self.assertFalse(any(command[0] == "make" for command in self.executed))
        report = json.loads((self.artifacts / "report.json").read_text())
        self.assertEqual(report["targets"], self.tests)
        self.assertEqual(report["status"], "passed")
        self.assertEqual(report["ci_tests"], self.owner + ":ci_tests")
        self.assertEqual(report["ci_artifacts"], self.owner + ":ci_artifacts")
        self.assertEqual(receipt["tests"], dict.fromkeys(self.tests, "passed"))
        self.assertEqual(len(report["test_outputs"]), 2 * len(self.tests))
        self.assertEqual(report["source_hashes"]["checked-input.json"], container.sha(self.root / "checked-input.json"))
        self.assertEqual(report["module_lock_sha256"], container.sha(self.root / "MODULE.bazel.lock"))
        self.assertTrue((self.artifacts / "test-events.jsonl").exists())
        self.assertEqual((self.artifacts / "package.tar").read_bytes(), b"archive fixture")

    def test_empty_artifact_group_keeps_contract_only_ci_valid(self):
        """A container may declare tests without publishing production archives in that job."""
        receipt = self.run_ci("empty-artifacts")
        self.assertEqual(receipt["status"], "passed")
        self.assertEqual(receipt["artifacts"], {})
        self.assertTrue((self.artifacts / "test-events.jsonl").is_file())

    def test_external_test_identity_is_canonical_in_receipt_and_report(self):
        """Publish exact configured identities so an unrelated repository cannot supply evidence."""
        receipt = self.run_ci("external")
        report = json.loads((self.artifacts / "report.json").read_text())
        self.assertIn(self.canonical, report["targets"])
        self.assertIn(self.canonical, receipt["tests"])
        self.assertNotIn(self.external, receipt["tests"])
        self.assertIn("tests/external_test/test.xml", report["test_outputs"])

    def test_deb_output_blocks_execution(self):
        """A DEB-producing graph must fail before any selected test or archive runs."""
        self.assert_failed_without_report(self.run_ci("deb", error=ValueError))
        self.assert_no_actions_executed()

    def test_packaging_wrapper_blocks_execution(self):
        """Calling Make in a Bazel action must fail even when no output is named .deb."""
        self.assert_failed_without_report(self.run_ci("wrapper", error=ValueError))
        self.assert_no_actions_executed()

    def test_empty_suite_blocks_execution(self):
        """An empty declared suite cannot satisfy CI by doing no validation work."""
        self.assert_failed_without_report(self.run_ci("empty-suite", error=ValueError))
        self.assert_no_actions_executed()

    def test_failed_query_keeps_stderr_and_exit_status(self):
        """Retain query diagnostics while removing private graph stdout on failure."""
        receipt = self.run_ci("query-failure", error=subprocess.CalledProcessError)
        self.assert_failed_without_report(receipt)
        self.assertEqual(receipt["commands"][-1]["returncode"], 7)
        self.assertEqual((self.artifacts / "actions.log").read_text(), "action query diagnostic\n")
        self.assert_no_actions_executed()

    def test_failed_test_does_not_publish_success(self):
        """An unsuccessful Bazel test command must leave a failed receipt and no report."""
        receipt = self.run_ci("test-failure", error=subprocess.CalledProcessError)
        self.assert_failed_without_report(receipt)
        self.assertEqual(receipt["commands"][-1]["returncode"], 9)
        self.assertFalse(any(item[0] == "build" for item in self.bazel_commands()))

    def test_missing_configured_evidence_does_not_publish_success(self):
        """A zero exit code cannot replace required logs/XML for every expanded test."""
        receipt = self.run_ci("missing-evidence", error=ValueError)
        self.assert_failed_without_report(receipt)
        self.assertIn("second_test/test.xml", receipt["error"])

    def test_changed_declared_source_does_not_publish_success(self):
        """A test that changes a discovered source invalidates an otherwise passing run."""
        self.assert_failed_without_report(self.run_ci("source-change", error=ValueError))

    def test_preparation_metadata_is_hashed_alongside_declared_bazel_inputs(self):
        """Record Make metadata that prepares image inputs even when it is outside the Bazel graph."""
        receipt = self.run_ci(source_files=("preparation.mk",))
        self.assertEqual(receipt["status"], "passed")
        report = json.loads((self.artifacts / "report.json").read_text())
        self.assertEqual(report["source_hashes"]["preparation.mk"], container.sha(self.root / "preparation.mk"))
        self.assertIn("checked-input.json", report["source_hashes"])
        self.assertIn("tools/bazel/oci/source_contract.bzl", report["source_hashes"])

    def test_changed_preparation_metadata_invalidates_the_run(self):
        """Passing tests cannot publish a receipt for different Make preparation metadata."""
        receipt = self.run_ci("preparation-change", source_files=("preparation.mk",), error=ValueError)
        self.assert_failed_without_report(receipt)
        self.assertIn("changed a checked source", receipt["error"])

    def test_preparation_sources_must_be_workspace_relative(self):
        """Reject paths escaping the checkout before running any test or artifact build."""
        self.assert_failed_without_report(self.run_ci(source_files=("../outside.mk",), error=ValueError))
        self.assert_no_actions_executed()

    def test_validation_receipt_comes_from_bazel_test_output(self):
        """Package validation produced by a declared test is retained without an owner callback."""
        receipt = self.run_ci("validation")
        self.assertEqual(receipt["validation"]["programs"], ["usr/bin/example"])
        self.assertEqual(receipt["validation"]["artifact_sha256"],
                         {"package.tar": hashlib.sha256(b"archive fixture").hexdigest()})
        self.assertTrue((self.artifacts / "tests/first_test/validation.json").is_file())

    def test_published_archive_bytes_must_match_the_tested_configuration(self):
        """A passing test cannot validate the same archive name built with different bytes later."""
        receipt = self.run_ci("validation-changed-archive", error=ValueError)
        self.assert_failed_without_report(receipt)
        self.assertIn("published artifacts differ", receipt["error"])
        self.assertNotEqual(receipt["artifacts"]["package.tar"]["sha256"],
                            receipt["validation"]["artifact_sha256"]["package.tar"])

    def test_missing_or_extra_published_archives_cannot_satisfy_validation(self):
        """The complete archive inventory must match the test receipt in both directions."""
        for mode in ("validation-missing-archive", "validation-extra-archive"):
            with self.subTest(mode=mode):
                self.artifacts = self.root / mode
                (self.root / "MODULE.bazel.lock").unlink(missing_ok=True)
                # Each run must produce fresh configured-test output locations.
                import shutil
                shutil.rmtree(self.root / "configured testlogs", ignore_errors=True)
                receipt = self.run_ci(mode, error=ValueError)
                self.assert_failed_without_report(receipt)
                self.assertIn("published artifacts differ", receipt["error"])

    def test_validation_summary_must_bind_its_archive_bytes(self):
        """Program/debug checks without archive hashes cannot establish which files were tested."""
        receipt = self.run_ci("validation-missing-bindings", error=ValueError)
        self.assert_failed_without_report(receipt)
        self.assertIn("published artifacts differ", receipt["error"])

    def test_multiple_validation_outputs_do_not_silently_replace_each_other(self):
        """Reject ambiguous package evidence instead of choosing one test's validation receipt."""
        self.assert_failed_without_report(self.run_ci("multiple-validations", error=ValueError))

    def test_malformed_validation_output_does_not_publish_success(self):
        """A passing test with an unreadable validation receipt cannot establish archive validity."""
        self.assert_failed_without_report(self.run_ci("invalid-validation", error=ValueError))

    def test_resolution_failure_does_not_publish_success(self):
        """Incomplete dependency evidence must prevent publication of a success report."""
        receipt = self.run_ci(error=ValueError, resolution_error=ValueError("incomplete graph"))
        self.assert_failed_without_report(receipt)
        self.assertIn("incomplete graph", receipt["error"])

    def test_wrong_bazel_version_blocks_action_execution(self):
        """The runner must reject a tool version different from the workspace pin."""
        self.assert_failed_without_report(self.run_ci("wrong-version", error=ValueError))
        self.assertEqual([item[0] for item in self.bazel_commands()], ["--version"])

    def test_preexisting_module_lock_blocks_action_execution(self):
        """CI must resolve dependencies from a clean generated-lock state."""
        (self.root / "MODULE.bazel.lock").write_text("{}\n")
        self.assert_failed_without_report(self.run_ci(error=ValueError))
        self.assert_no_actions_executed()

    def test_post_build_validation_preserves_existing_outputs_and_records_lock(self):
        """A completed Make build can be validated without cleaning its lock or outputs."""
        lock = self.root / "MODULE.bazel.lock"
        lock.write_text('{"existing": true}\n')
        initial_hash = container.sha(lock)
        output = self.root / "bazel-bin/already-built"
        output.parent.mkdir()
        output.write_bytes(b"compiled by Make")
        receipt = self.run_ci(reuse_build=True)
        self.assertEqual(receipt["status"], "passed")
        self.assertEqual(receipt["dependency_state"], "existing-build")
        self.assertEqual(receipt["initial_module_lock_sha256"], initial_hash)
        self.assertEqual(output.read_bytes(), b"compiled by Make")
        self.assertIn(["git", "ls-files", "--", "MODULE.bazel.lock"], self.executed)
        self.assertIn(["git", "check-ignore", "--quiet", "MODULE.bazel.lock"], self.executed)
        operations = [command[0] for command in self.bazel_commands()]
        self.assertLess(operations.index("aquery"), operations.index("build"))
        self.assertLess(operations.index("build"), operations.index("test"))
        self.assertEqual(operations.count("build"), 1)
        for command in self.bazel_commands():
            logs = [arg for arg in command if arg.startswith("--execution_log_json_file=")]
            self.assertEqual(len(logs), int(command[0] == "build"))
        evidence = receipt["cache_reuse"]
        self.assertEqual(evidence["phase"], "before_tests")
        self.assertEqual(evidence["artifact_target"], self.owner + ":ci_artifacts")
        self.assertEqual((evidence["action_count"], evidence["cache_hit_count"], evidence["executed_count"]), (2, 2, 0))
        self.assertEqual(evidence, json.loads((self.artifacts / "report.json").read_text())["cache_reuse"])

    def test_post_build_cache_misses_are_reported_without_failing_validation(self):
        """A legitimate CI copy action is recorded distinctly from cached compilation."""
        (self.root / "MODULE.bazel.lock").write_text("{}\n")
        receipt = self.run_ci("cache-executed", reuse_build=True)
        evidence = receipt["cache_reuse"]
        self.assertEqual((evidence["action_count"], evidence["cache_hit_count"], evidence["executed_count"]), (3, 2, 1))
        self.assertEqual(receipt["status"], "passed")
        self.assertIn('"uncached_actions": [{"cache_hit": false', self.console)
        self.assertIn('"mnemonic": "CopyFile"', self.console)

    def test_post_build_invalid_cache_evidence_blocks_tests_and_removes_raw_log(self):
        """Absent, partial or failed build evidence cannot establish cache reuse."""
        (self.root / "MODULE.bazel.lock").write_text("{}\n")
        for mode in ("cache-missing", "cache-empty", "cache-malformed", "cache-incomplete-build", "cache-failed-build"):
            with self.subTest(mode=mode):
                self.artifacts = self.root / mode
                (self.root / "commands.jsonl").unlink(missing_ok=True)
                error = subprocess.CalledProcessError if mode == "cache-failed-build" else ValueError
                receipt = self.run_ci(mode, reuse_build=True, error=error)
                self.assert_failed_without_report(receipt)
                self.assertNotIn("cache_reuse", receipt)
                self.assertNotIn("test", [command[0] for command in self.bazel_commands()])

    def test_post_build_test_failure_preserves_safe_cache_audit(self):
        """Test failure retains production cache evidence, but never a success report."""
        (self.root / "MODULE.bazel.lock").write_text("{}\n")
        receipt = self.run_ci("test-failure", reuse_build=True, error=subprocess.CalledProcessError)
        self.assert_failed_without_report(receipt)
        self.assertEqual(receipt["cache_reuse"]["cache_hit_count"], 2)

    def test_post_build_precollected_bytes_still_match_tested_artifacts(self):
        """Moving collection earlier must retain the exact runtime/debug hash binding."""
        (self.root / "MODULE.bazel.lock").write_text("{}\n")
        receipt = self.run_ci("validation-changed-archive", reuse_build=True, error=ValueError)
        self.assert_failed_without_report(receipt)
        self.assertIn("published artifacts differ", receipt["error"])

    def test_post_build_validation_requires_a_nonempty_generated_lock(self):
        """A missing, empty or symlinked lock cannot masquerade as an existing build."""
        lock = self.root / "MODULE.bazel.lock"
        for mode in ("missing", "empty", "symlink"):
            with self.subTest(mode=mode):
                self.artifacts = self.root / mode
                lock.unlink(missing_ok=True)
                if mode == "empty":
                    lock.touch()
                elif mode == "symlink":
                    target = self.root / "other-lock"
                    target.write_text("{}\n")
                    lock.symlink_to(target)
                self.assert_failed_without_report(self.run_ci(reuse_build=True, error=ValueError))
                self.assert_no_actions_executed()

    def test_post_build_validation_rejects_tracked_or_unignored_locks(self):
        """Reuse must preserve the generated-lock policy instead of trusting any file."""
        (self.root / "MODULE.bazel.lock").write_text("{}\n")
        for mode, error in (("tracked-lock", ValueError),
                            ("unignored-lock", subprocess.CalledProcessError)):
            with self.subTest(mode=mode):
                self.artifacts = self.root / mode
                self.assert_failed_without_report(self.run_ci(mode, reuse_build=True, error=error))
                self.assert_no_actions_executed()

    def test_unsupported_machine_blocks_all_commands(self):
        """These native AMD64 jobs must reject execution on another architecture."""
        self.assert_failed_without_report(self.run_ci(machine="aarch64", error=ValueError))
        self.assertFalse(self.executed)

    def test_nonempty_artifact_directory_is_preserved(self):
        """Refuse to overwrite evidence from an earlier run before executing a command."""
        self.artifacts.mkdir()
        original = self.artifacts / "receipt.json"
        original.write_text("previous evidence\n")
        with self.assertRaises(ValueError):
            container.run(self.owner, workspace=self.root, artifacts=self.artifacts,
                          bazel=str(self.bazel), options=[])
        self.assertEqual(original.read_text(), "previous evidence\n")
        self.assertFalse(self.bazel_commands())

    def test_cli_selects_a_container_package_and_forwards_execution_options(self):
        """The CLI needs only a BUILD package; no Python profile import or target list is required."""
        with mock.patch.object(container, "run") as run:
            container.main(["--container", self.owner, "--bazel", "selected-bazel",
                            "--bazel-arg=--jobs=2", "--artifacts", str(self.artifacts),
                            "--source-file", "preparation.mk", "--source-file", "another.mk", "--reuse-build"])
        self.assertEqual(run.call_args.args, (self.owner,))
        self.assertEqual(run.call_args.kwargs["bazel"], "selected-bazel")
        self.assertEqual(run.call_args.kwargs["options"], [*container.OPTIONS, "--jobs=2"])
        self.assertEqual(run.call_args.kwargs["source_files"], ["preparation.mk", "another.mk"])
        self.assertTrue(run.call_args.kwargs["reuse_build"])


class SourceArchiveBindingTest(unittest.TestCase):
    """Tie the actual layer validator's inputs to unique published archive names."""

    def test_archive_bindings_include_every_input_and_detect_changed_bytes(self):
        """Use content hashes so a later build of the same names cannot stand in for tested inputs."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            paths = [root / name for name in ("runtime.tar", "dependencies.tar", "config.tar", "symbols.tar", "library.tar")]
            for path in paths: path.write_bytes(path.name.encode())
            checked = oci_source_layers_test.archive_hashes(paths)
            self.assertEqual(set(checked), {path.name for path in paths})
            self.assertEqual(checked["runtime.tar"], hashlib.sha256(b"runtime.tar").hexdigest())
            paths[0].write_bytes(b"different configuration")
            self.assertNotEqual(oci_source_layers_test.archive_hashes(paths), checked)

    def test_duplicate_basenames_cannot_hide_a_validated_input(self):
        """Reject ambiguous role/library names even when their paths or archive bytes differ."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            paths = [root / "first/output.tar", root / "second/output.tar"]
            for path in paths:
                path.parent.mkdir()
                path.write_bytes(b"archive bytes")
            with self.assertRaisesRegex(ValueError, "basenames must be unique"):
                oci_source_layers_test.archive_hashes(paths)


class TestEvidenceTest(unittest.TestCase):
    """Resolve test evidence from BEP without accepting ambiguous or remote files."""

    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.target = "//sample:test"
        self.events = self.root / "events.jsonl"
        self.outputs = self.root / "artifacts"

    def event(self, filename, uri):
        return {"id": {"testResult": {"label": self.target}},
                "testResult": {"testActionOutput": [{"name": filename, "uri": uri}]}}

    def test_nonlocal_test_output_is_rejected(self):
        """Evidence from HTTP or a file URI with a hostname must not be copied locally."""
        for uri in ("https://example.test/test.log", "file://remote/tmp/test.log"):
            with self.subTest(uri=uri):
                self.events.write_text(json.dumps(self.event("test.log", uri)) + "\n")
                with self.assertRaisesRegex(ValueError, "not a local file"):
                    container.collect_test_outputs(self.events, self.outputs, (self.target,))

    def test_two_configurations_cannot_silently_replace_evidence(self):
        """Different configured outputs for one test must fail instead of choosing one."""
        self.events.write_text("\n".join(json.dumps(self.event("test.log", (self.root / name).as_uri()))
                                         for name in ("first.log", "second.log")) + "\n")
        with self.assertRaisesRegex(ValueError, "ambiguous configured"):
            container.collect_test_outputs(self.events, self.outputs, (self.target,))

    def external_events(self, canonical):
        events = []
        for filename in ("test.log", "test.xml"):
            output = self.root / filename
            output.write_text("passed\n")
            event = self.event(filename, output.as_uri())
            event["id"]["testResult"]["label"] = canonical
            events.append(event)
        self.events.write_text("".join(json.dumps(event) + "\n" for event in events))

    def test_canonical_external_label_uses_queried_repository_identity(self):
        """Match Bazel's canonical BEP label to the requested apparent repository name."""
        target = "@sonic_swss//crates/countersyncd:common_rust_test"
        canonical = "@@sonic-swss+//crates/countersyncd:common_rust_test"
        self.external_events(canonical)
        outputs, validations = container.collect_test_outputs(self.events, self.outputs, (target,),
                                                 labels={canonical: target})
        self.assertEqual(outputs, ["tests/common_rust_test/test.log", "tests/common_rust_test/test.xml"])
        self.assertEqual(validations, {})

    def test_matching_suffix_from_another_repository_cannot_supply_evidence(self):
        """A foreign repository's equally named test cannot satisfy the requested SWSS test."""
        target = "@sonic_swss//crates/countersyncd:common_rust_test"
        canonical = "@@sonic-swss+//crates/countersyncd:common_rust_test"
        self.external_events("@@foreign+//crates/countersyncd:common_rust_test")
        with self.assertRaisesRegex(ValueError, "missing required test evidence"):
            container.collect_test_outputs(self.events, self.outputs, (target,),
                                           labels={canonical: target})


if __name__ == "__main__":
    unittest.main()
