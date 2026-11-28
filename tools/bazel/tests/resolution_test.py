"""Fail-closed checks for module evidence collection; no build actions."""

import contextlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

from tools.bazel.ci import resolution


GRAPH = json.dumps({"key": "<root>", "root": True, "dependencies": [
    {"key": "rules_apple@3.5.1"}, {"key": "abseil-cpp@20240722.0"},
]})
APPLE_ERRORS = """ERROR: Traceback (most recent call last):
Error: 'struct' value has no field or method 'AppleDynamicFramework'
ERROR: Error loading '@@rules_apple+//apple:apple.bzl' for module extensions, requested by https://bcr.bazel.build/modules/rules_apple/3.5.1/MODULE.bazel:27:48: at /cache/external/rules_apple+/apple/apple.bzl:27:5: initialization of module 'apple/internal/apple_xcframework_import.bzl' failed: at /cache/external/rules_apple+/apple/apple.bzl:27:5: initialization of module 'apple/internal/apple_xcframework_import.bzl' failed
"""
ABSEIL_ERRORS = "ERROR: @@bazel_tools//tools/cpp:cc_configure.bzl does not export a module extension called cc_configure_extension, yet its use is requested at https://bcr.bazel.build/modules/abseil-cpp/20240722.0/MODULE.bazel:23:29\n"


def diagnostics(*groups):
    count = len(groups)
    return "".join(groups) + f"ERROR: Results may be incomplete as {count} extension{'s' if count != 1 else ''} failed.\n"


ERRORS = diagnostics(APPLE_ERRORS, ABSEIL_ERRORS)


class GraphInspectionTest(unittest.TestCase):
    """Classify complete module graphs and narrowly recognized extension diagnostics."""

    def test_success(self):
        """Record the module count and complete inspection after a clean graph command."""
        console = io.StringIO()
        with contextlib.redirect_stderr(console):
            receipt = resolution.inspect_graph(GRAPH, "", 0)
        self.assertTrue(receipt["extension_inspection_complete"])
        self.assertEqual(receipt["module_count"], 3)
        self.assertEqual(console.getvalue(), "")

    def test_stored_failures_report_only_the_process_status_and_classified_clues(self):
        for graph, code, exception in ((GRAPH, 2, ValueError), ("{}", 0, ValueError),
                                       ("not JSON", 0, json.JSONDecodeError)):
            console = io.StringIO()
            primary = "ERROR: download https://private.invalid/file?token=fixture HTTP 503\n"
            with self.subTest(graph=graph, code=code), contextlib.redirect_stderr(console), \
                    self.assertRaises(exception):
                resolution.inspect_graph_with_diagnostics(graph, primary, code)
            diagnostic = json.loads(console.getvalue())
            self.assertEqual(diagnostic["returncode"], code)
            self.assertEqual(diagnostic["http_statuses"], [503])
            self.assertNotIn("private.invalid", console.getvalue())

    def test_long_graph_warnings_preserve_the_classified_download_error(self):
        warning = "WARNING: APT avertissement: accès différé\n"
        cause = "https://private.invalid/python/SHA256SUMS GET returned 503 Service Unavailable\n"
        for fatal in ("ERROR: interpreter extension failed\n" + "frame\n" * 100,
                      "Error in download: "):
            console = io.StringIO()
            stderr = warning * 100 + fatal + cause + warning * 1000
            with self.subTest(fatal=fatal), contextlib.redirect_stderr(console), \
                    self.assertRaisesRegex(ValueError, "Unexpected module graph inspection failure"):
                resolution.inspect_graph_with_diagnostics(GRAPH, stderr, 2)
            self.assertEqual(json.loads(console.getvalue())["http_statuses"], [503])
            self.assertLess(len(console.getvalue()), 1024)
            self.assertNotIn("private.invalid", console.getvalue())

    def test_known_unused_extensions_are_recorded(self):
        """Retain known failures without calling their extension inspection complete."""
        for groups in ((APPLE_ERRORS,), (ABSEIL_ERRORS,), (APPLE_ERRORS, ABSEIL_ERRORS)):
            with self.subTest(groups=groups):
                receipt = resolution.inspect_graph(GRAPH, diagnostics(*groups), 2)
                self.assertTrue(receipt["module_graph_complete"])
                self.assertFalse(receipt["extension_inspection_complete"])
                self.assertEqual(len(receipt["unused_extension_failures"]), len(groups))

    def test_unaffected_module_may_use_another_version(self):
        """Do not constrain the version of a module that emitted no failure."""
        for graph, errors in ((GRAPH.replace("20240722.0", "20250127.1"), APPLE_ERRORS),
                              (GRAPH.replace("3.5.1", "4.0.0"), ABSEIL_ERRORS)):
            with self.subTest(graph=graph):
                receipt = resolution.inspect_graph(graph, diagnostics(errors), 2)
                self.assertEqual(len(receipt["unused_extension_failures"]), 1)

    def test_failing_module_version_must_match_graph(self):
        """Reject known diagnostics when their module versions disagree with the graph."""
        for graph in (GRAPH.replace("20240722.0", "20250127.1"), GRAPH.replace("3.5.1", "4.0.0")):
            with self.subTest(graph=graph), self.assertRaises(ValueError):
                resolution.inspect_graph(graph, ERRORS, 2)

    def test_partial_and_duplicate_groups_rejected(self):
        """Require each recognized diagnostic group to be complete and appear only once."""
        apple_lines = APPLE_ERRORS.splitlines(keepends=True)
        for index in range(len(apple_lines)):
            partial = "".join(line for position, line in enumerate(apple_lines) if position != index)
            for errors in (diagnostics(partial), diagnostics(partial, ABSEIL_ERRORS)):
                with self.subTest(errors=errors), self.assertRaises(ValueError):
                    resolution.inspect_graph(GRAPH, errors, 2)
        for extra in (*apple_lines, ABSEIL_ERRORS, APPLE_ERRORS):
            with self.subTest(extra=extra), self.assertRaises(ValueError):
                resolution.inspect_graph(GRAPH, extra + ERRORS, 2)

    def test_summary_must_match_complete_group_count(self):
        """Reject missing, repeated or malformed summaries and incorrect failure counts."""
        cases = (
            ERRORS.replace("2 extensions", "1 extension"),
            diagnostics(APPLE_ERRORS).replace("1 extension", "2 extensions"),
            diagnostics(ABSEIL_ERRORS).replace("1 extension", "1 extensions"),
            ERRORS + "ERROR: Results may be incomplete as 2 extensions failed.\n",
            "".join(ERRORS.splitlines(keepends=True)[:-1]),
        )
        for errors in cases:
            with self.subTest(errors=errors), self.assertRaises(ValueError):
                resolution.inspect_graph(GRAPH, errors, 2)

    def test_other_failures_rejected(self):
        """Keep unknown messages, changed signatures and unexpected exit codes fatal."""
        for diagnostics, code in ((ERRORS + "ERROR: download failed\n", 2),
                                  (ERRORS.replace("AppleDynamicFramework", "another_symbol"), 2),
                                  (ERRORS.replace("20240722.0", "20250101.0"), 2),
                                  (ERRORS, 1), ("", 2)):
            with self.subTest(diagnostics=diagnostics, code=code), self.assertRaises(ValueError):
                resolution.inspect_graph(GRAPH, diagnostics, code)

    def test_empty_truncated_or_incomplete_graph_rejected(self):
        """Reject invalid graph data and modules whose only occurrence is unexpanded."""
        incomplete = json.dumps({"key": "<root>", "root": True, "dependencies": [
            {"key": "missing@1", "unexpanded": True},
        ]})
        for graph in ("", "{", "{}", incomplete):
            with self.subTest(graph=graph), self.assertRaises(ValueError):
                resolution.inspect_graph(graph, ERRORS, 2)

    def test_repeated_reference_requires_expanded_module(self):
        """Count an unexpanded repeat once when its module already has an expanded entry."""
        graph = json.loads(GRAPH)
        graph["dependencies"].append({"key": "rules_apple@3.5.1", "unexpanded": True})
        self.assertEqual(resolution.inspect_graph(json.dumps(graph), ERRORS, 2)["module_count"], 3)


class GraphFailureCollectionTest(unittest.TestCase):
    """The real collector retains safe clues without replacing strict failures."""

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.workspace = Path(self.temporary.name)
        self.directory = self.workspace / "evidence"
        self.directory.mkdir()
        (self.workspace / "MODULE.bazel.lock").write_text('{"generated":true}\n')
        self.failure = self.directory / "module-graph-failure.json"

    def test_invalid_graph_keeps_original_exception_and_safe_status(self):
        secret = "ghs_" + "x" * 36
        stderr = ("ERROR: downloading https://snapshot.debian.org/private?token=" + secret +
                  " to /home/runner/private: GET returned response code 504\n::error::" + secret)
        original = json.JSONDecodeError("Expecting value", "", 0)
        log = io.StringIO()
        result = subprocess.CompletedProcess([], 2, "", stderr)
        with mock.patch.object(resolution.subprocess, "run", return_value=result) as command, \
                mock.patch.object(resolution, "inspect_graph", side_effect=original), \
                mock.patch.object(resolution, "retain") as retain, contextlib.redirect_stderr(log):
            with self.assertRaises(json.JSONDecodeError) as failed:
                resolution.collect(self.workspace, self.directory, bazel=("wrapper",),
                                   options=("--repository_cache=/private/cache",))
        self.assertIs(failed.exception, original)
        retain.assert_not_called()
        self.assertEqual(command.call_args.args[0], ["wrapper", "mod", "graph", "--output=json",
                         "--extension_info=hidden", "--lockfile_mode=off", "--color=no", "--curses=no",
                         "--repository_cache=/private/cache"])
        summary = json.loads(self.failure.read_text())
        self.assertEqual(json.loads(log.getvalue()), summary)
        self.assertEqual(summary["returncode"], 2)
        self.assertEqual(summary["origins"], ["debian_snapshot"])
        self.assertEqual(summary["http_statuses"], [504])
        for private in (secret, "https://", "/home/", "token", "::error::", "wrapper", "/private"):
            self.assertNotIn(private, log.getvalue() + self.failure.read_text())
        self.assertEqual((self.directory / "module-graph.stderr").read_text(), stderr)
        self.assertEqual((self.workspace / "MODULE.bazel.lock").read_text(), '{"generated":true}\n')

    def test_actual_parser_rejection_remains_failed_for_empty_and_unknown_errors(self):
        for stdout, stderr, code, exception in (("", "", 2, json.JSONDecodeError),
                                               (GRAPH, "ERROR: new failure\n", 2, ValueError)):
            with self.subTest(stdout=stdout), mock.patch.object(resolution.subprocess, "run",
                    return_value=subprocess.CompletedProcess([], code, stdout, stderr)), \
                    mock.patch.object(resolution, "retain") as retain, contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(exception):
                    resolution.collect(self.workspace, self.directory)
                retain.assert_not_called()
                self.assertEqual(json.loads(self.failure.read_text())["status"], "failed")

    def test_success_and_known_unused_errors_remove_stale_failure_receipt(self):
        for stderr, code in (("", 0), (ERRORS, 2)):
            self.failure.write_text('{"stale":true}\n')
            with self.subTest(code=code), mock.patch.object(resolution.subprocess, "run",
                    return_value=subprocess.CompletedProcess([], code, GRAPH, stderr)), \
                    mock.patch.object(resolution, "retain", return_value={}), contextlib.redirect_stderr(io.StringIO()) as log:
                result = resolution.collect(self.workspace, self.directory)
            self.assertTrue(result["inspection"]["module_graph_complete"])
            self.assertFalse(self.failure.exists())
            self.assertEqual(log.getvalue(), "")

    def test_diagnostic_projection_or_write_failure_does_not_mask_original(self):
        original = ValueError("original graph error")
        for fault in ("projection", "write", "console"):
            with self.subTest(fault=fault), mock.patch.object(resolution.subprocess, "run",
                    return_value=subprocess.CompletedProcess([], 2, "", "")), \
                    mock.patch.object(resolution, "inspect_graph", side_effect=original), \
                    contextlib.ExitStack() as stack:
                if fault == "projection":
                    stack.enter_context(mock.patch("tools.bazel.ci.public_artifacts.graph_failure_summary",
                                                   side_effect=ValueError("projection")))
                elif fault == "write":
                    real_write = Path.write_text
                    def write(path, *args, **kwargs):
                        if path == self.failure:
                            raise OSError("disk full")
                        return real_write(path, *args, **kwargs)
                    stack.enter_context(mock.patch.object(Path, "write_text", write))
                else:
                    stack.enter_context(mock.patch("builtins.print", side_effect=OSError("closed output")))
                with self.assertRaises(ValueError) as failed:
                    resolution.collect(self.workspace, self.directory)
                self.assertIs(failed.exception, original)

    def test_copied_parser_works_without_optional_artifact_helpers(self):
        """Kernel fixtures import the copied parser without the tools package."""
        copied = self.workspace / "resolution.py"
        copied.write_text(Path(resolution.__file__).read_text())
        code = '''
import importlib.util, json, pathlib, subprocess
from unittest import mock
spec = importlib.util.spec_from_file_location("copied_resolution", "resolution.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
graph = json.dumps({"key":"<root>", "root":True, "dependencies":[{"key":"component@1"}]})
assert module.inspect_graph(graph, "", 0)["module_graph_complete"]
root = pathlib.Path.cwd()
with mock.patch.object(module.subprocess, "run", return_value=subprocess.CompletedProcess([], 2, "", "")):
    try:
        module.collect(root, root / "evidence")
    except json.JSONDecodeError:
        pass
    else:
        raise AssertionError("Missing diagnostics must not hide the graph failure")
assert not (root / "evidence/module-graph-failure.json").exists()
'''
        env = dict(os.environ)
        env.pop("PYTHONPATH", None)
        result = subprocess.run([sys.executable, "-I", "-B", "-c", code], cwd=self.workspace,
                                env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)


class SourceScopeTest(unittest.TestCase):
    """Exercise post-Make provenance checks against real Git submodule state."""

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        directory = Path(self.temporary.name)
        self.root = directory / "workspace"
        component = directory / "component"
        for repository in (self.root, component):
            repository.mkdir()
            self.git(repository, "init", "--quiet")
            self.git(repository, "config", "user.name", "SONiC test")
            self.git(repository, "config", "user.email", "securely1g@gmail.com")
        (component / "source.cc").write_text("original producer source\n")
        self.git(component, "add", "source.cc")
        self.git(component, "commit", "--quiet", "-m", "Seed producer fixture")
        (self.root / "BUILD.bazel").write_text("# Declared container build input\n")
        (self.root / ".gitignore").write_text("MODULE.bazel.lock\n")
        self.git(self.root, "-c", "protocol.file.allow=always", "submodule", "add", "--quiet",
                 str(component), "deps/component")
        self.component = self.root / "deps/component"
        self.git(self.component, "config", "user.name", "SONiC test")
        self.git(self.component, "config", "user.email", "securely1g@gmail.com")
        self.git(self.root, "add", ".")
        self.git(self.root, "commit", "--quiet", "-m", "Pin producer fixture")
        self.lock = self.root / "MODULE.bazel.lock"
        self.lock.write_text(json.dumps({"registryFileHashes": {}}) + "\n")
        self.output = directory / "evidence"
        self.output.mkdir()

    @staticmethod
    def git(repository, *args):
        return subprocess.run(["git", "-C", str(repository), *args], check=True,
                              capture_output=True, text=True).stdout

    def change_component(self, *, commit=False):
        (self.component / "source.cc").write_text("patched by the legacy Make producer\n")
        if commit:
            self.git(self.component, "commit", "--quiet", "-am", "Apply producer fixture patch")

    def test_default_rejects_unrelated_submodule_dirt_and_commit(self):
        """Cold source jobs retain their strict whole-checkout cleanliness check."""
        for commit in (False, True):
            with self.subTest(commit=commit):
                self.change_component(commit=commit)
                with self.assertRaises(subprocess.CalledProcessError):
                    resolution.retain(self.root, self.output, GRAPH)
                self.assertFalse((self.output / "MODULE.bazel.lock").exists())

    def test_scoped_inputs_allow_unrelated_make_submodule_changes(self):
        """An FRR-style patched commit must not invalidate unchanged OCI inputs."""
        for commit in (False, True):
            with self.subTest(commit=commit):
                self.change_component(commit=commit)
                result = resolution.retain(self.root, self.output, GRAPH,
                                           source_files=["BUILD.bazel"])
                self.assertEqual(result["checked_source_files"], ["BUILD.bazel"])
                self.assertTrue(result["tracked_files_unchanged"])
                self.assertEqual((self.output / "MODULE.bazel.lock").read_bytes(), self.lock.read_bytes())

    def test_scoped_inputs_reject_changes_to_checked_root_file(self):
        """Post-Make mode still rejects altered build declarations against HEAD."""
        (self.root / "BUILD.bazel").write_text("# Changed source target\n")
        with self.assertRaises(subprocess.CalledProcessError):
            resolution.retain(self.root, self.output, GRAPH, source_files=["BUILD.bazel"])
        self.assertFalse((self.output / "MODULE.bazel.lock").exists())

    def test_scoped_submodule_input_checks_its_gitlink(self):
        """A file inside a producer submodule also pins that producer's HEAD."""
        result = resolution.retain(self.root, self.output, GRAPH,
                                   source_files=["deps/component/source.cc"])
        self.assertEqual(result["checked_source_files"],
                         ["deps/component", "deps/component/source.cc"])
        self.change_component(commit=True)
        # A checkout's local ignore policy must not hide a changed producer pin.
        self.git(self.root, "config", "submodule.deps/component.ignore", "all")
        with self.assertRaises(subprocess.CalledProcessError):
            resolution.retain(self.root, self.output, GRAPH,
                              source_files=["deps/component/source.cc"])

    def test_invalid_or_escaping_scope_is_rejected(self):
        """An empty, unsafe or Git-magic scope cannot bypass source validation."""
        (self.root / "escape").symlink_to(Path(self.temporary.name) / "component", target_is_directory=True)
        for files in ([], (), "BUILD.bazel", [""], ["."], ["../component/source.cc"],
                      ["/etc/passwd"], ["deps/../BUILD.bazel"], ["BUILD.bazel\n"],
                      [":(exclude)BUILD.bazel"], ["*.bazel"], ["escape/source.cc"]):
            with self.subTest(files=files), self.assertRaises(ValueError):
                resolution.retain(self.root, self.output, GRAPH, source_files=files)

    def test_scoped_inputs_still_reject_a_tracked_lock(self):
        """Source scope never relaxes the generated module-lock contract."""
        self.git(self.root, "add", "--force", "MODULE.bazel.lock")
        with self.assertRaisesRegex(ValueError, "generated, not tracked"):
            resolution.retain(self.root, self.output, GRAPH, source_files=["BUILD.bazel"])

    def test_collect_forwards_scope_without_changing_generated_lock(self):
        """The graph collector uses the same scoped guard and preserves the tested lock."""
        self.change_component(commit=True)
        before = self.lock.read_bytes()
        graph_command = Path(self.temporary.name) / "graph.py"
        graph_command.write_text("print(" + repr(GRAPH) + ")\n")
        result = resolution.collect(self.root, self.output,
                                    bazel=(sys.executable, str(graph_command)),
                                    source_files=["BUILD.bazel"])
        self.assertEqual(result["checked_source_files"], ["BUILD.bazel"])
        self.assertEqual(self.lock.read_bytes(), before)
        self.assertTrue(result["inspection"]["module_graph_complete"])


if __name__ == "__main__":
    unittest.main()
