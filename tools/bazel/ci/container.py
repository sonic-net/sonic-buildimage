#!/usr/bin/env python3
"""Run container CI from an owner's declared tests, archives and package checks."""

import argparse
from functools import partial
import json
from pathlib import Path, PurePosixPath
import platform
import shutil
import subprocess
import sys
import time
from urllib.parse import unquote, urlparse

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import build, cache_reuse, command_log, public_artifacts, resolution
from tools.bazel.ci.artifact_validation import require, sha
from tools.bazel.ci.bazel_commands import inspect_actions
from tools.bazel.gzip.source_archive import check_versions

OPTIONS = (
    "--jobs=4", "--local_resources=cpu=4", "--local_resources=memory=10000",
    "--lockfile_mode=update", "--noshow_progress", "--color=no", "--curses=no",
)
SOURCE_FILES = (
    ".bazelversion", ".bazelrc", ".bazelignore", "MODULE.bazel",
    "tools/bazel/ci/container.py", "tools/bazel/ci/build.py",
    "tools/bazel/ci/command_log.py", "tools/bazel/ci/bazel_commands.py",
    "tools/bazel/ci/cache_reuse.py",
    "tools/bazel/ci/artifact_validation.py", "tools/bazel/ci/resolution.py",
    "tools/bazel/gzip/source_archive.py", "tools/bazel/ci/public_artifacts.py",
    "tools/bazel/prepare_manifests.mk", "rules/functions", ".github/workflows/bazel-oci.yml",
    "tools/bazel/oci/source_contract.bzl",
)


QUERY_OPTIONS = ("--lockfile_mode=update", "--noshow_progress", "--color=no", "--curses=no")


def query_tests(suite, bazel, artifacts, receipt, execute):
    """Expand the owner's explicit test suite, including nested suites."""
    targets = execute(
        [bazel, "query", *QUERY_OPTIONS, "--strict_test_suite", "--output=label",
         'tests("' + suite + '")'], artifacts, receipt, "test-suite",
    ).splitlines()
    require(bool(targets) and len(set(targets)) == len(targets), "CI suite must select unique explicit tests")
    return sorted(public_artifacts.label(target) for target in targets)


def declared_sources(targets, bazel, workspace, artifacts, receipt, execute, extra=()):
    """Record the actual local source and BUILD inputs, without a second policy list."""
    dependencies = "deps(set(" + " ".join(targets) + "))"
    expression = 'kind("source file", ' + dependencies + ") union buildfiles(" + dependencies + ")"
    labels = execute([bazel, "query", *QUERY_OPTIONS, "--output=label", expression],
                     artifacts, receipt, "source-inputs").splitlines()
    names = set(SOURCE_FILES)
    names.update(public_artifacts.relative_name(name) for name in extra)
    for label in labels:
        if label.startswith("@@//"):
            label = label[2:]
        if not label.startswith("//"):
            continue  # External inputs are recorded in module/source receipts.
        public_artifacts.label(label)
        package, name = label[2:].split(":", 1)
        relative = str(PurePosixPath(package) / name)
        public_artifacts.relative_name(relative)
        require((workspace / relative).is_file(), "declared CI input is missing: " + relative)
        names.add(relative)
    return sorted(names)


def configured_test_labels(targets, bazel, options, artifacts, receipt, execute):
    """Resolve apparent repository labels to the exact labels emitted by BEP."""
    labels = {}
    for index, target in enumerate(targets):
        canonical = target
        if target.startswith("@") and not target.startswith("@@"):
            matches = execute(
                [bazel, "cquery", *options, "--output=starlark",
                 "--starlark:expr=str(target.label)", target],
                artifacts, receipt, "test-label-" + str(index),
            ).splitlines()
            require(len(matches) == 1 and matches[0].startswith("@@") and "//" in matches[0],
                    "expected one canonical test label for " + target)
            canonical = matches[0]
        require(canonical not in labels, "duplicate configured test label: " + canonical)
        labels[canonical] = target
    return labels


def collect_test_outputs(events, artifacts, targets, labels=None):
    """Collect actual configured outputs, including Python-transition tests."""
    labels = labels if labels is not None else {target: target for target in targets}
    outputs = {}
    for line in events.read_text().splitlines():
        event = json.loads(line)
        target = labels.get(event.get("id", {}).get("testResult", {}).get("label"))
        if target not in targets:
            continue
        for output in event.get("testResult", {}).get("testActionOutput", []):
            filename = output.get("name")
            if filename not in ("test.log", "test.xml", "test.outputs/validation.json"):
                continue
            uri = urlparse(output.get("uri", ""))
            require(uri.scheme == "file" and not uri.netloc,
                    "test evidence is not a local file: " + target + "/" + filename)
            source = Path(unquote(uri.path))
            key = (target, filename)
            require(key not in outputs or outputs[key] == source,
                    "ambiguous configured test evidence: " + target + "/" + filename)
            outputs[key] = source
    names = [target.rsplit(":", 1)[1] for target in targets]
    require(len(names) == len(set(names)), "test evidence filenames must be unique")
    collected, validations = [], {}
    for target, name in zip(targets, names):
        for filename in ("test.log", "test.xml"):
            source = outputs.get((target, filename))
            require(source is not None and source.is_file() and source.stat().st_size > 0,
                    "missing required test evidence: " + target + "/" + filename)
            destination = artifacts / "tests" / name / filename
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, destination)
            collected.append(str(destination.relative_to(artifacts)))
        validation = outputs.get((target, "test.outputs/validation.json"))
        if validation is not None:
            require(validation.is_file() and validation.stat().st_size > 0,
                    "missing package validation output: " + target)
            destination = artifacts / "tests" / name / "validation.json"
            shutil.copyfile(validation, destination)
            validations[target] = json.loads(destination.read_text())
            collected.append(str(destination.relative_to(artifacts)))
    return collected, validations


def run(container, *, workspace, artifacts, bazel, options, source_files=(), reuse_build=False):
    """Run the owner's Bazel CI targets and retain their outputs and failure evidence."""
    require(container.startswith("//") and ":" not in container and
            all(part not in ("", ".", "..") for part in container[2:].split("/")),
            "container must be a local Bazel package such as //dockers/docker-orchagent")
    suite, artifact_target = container + ":ci_tests", container + ":ci_artifacts"
    public_artifacts.label(suite)
    require(not artifacts.exists() or not any(artifacts.iterdir()),
            "artifact directory must be empty: " + str(artifacts))
    artifacts.mkdir(parents=True, exist_ok=True)
    execute = partial(command_log.execute, cwd=workspace)
    scope = "Owner-declared CI targets; complete installer, boot and forwarding are separate checks."
    receipt = {"status": "running", "commands": [], "scope": scope,
               "ci_tests": suite, "ci_artifacts": artifact_target}
    started = time.monotonic()
    try:
        require(platform.machine() == "x86_64" and
                platform.freedesktop_os_release().get("VERSION_CODENAME") == "trixie",
                "container CI requires native AMD64 Debian Trixie")
        receipt["platform"] = platform.platform()
        receipt["architecture"] = "amd64"
        receipt["revision"] = execute(["git", "rev-parse", "HEAD"], artifacts, receipt, "revision").strip()
        version = execute([bazel, "--version"], artifacts, receipt, "bazel-version").strip()
        require(version == "bazel " + (workspace / ".bazelversion").read_text().strip(),
                "Bazel version does not match .bazelversion: " + version)
        receipt["bazel_version"] = version
        lock = workspace / "MODULE.bazel.lock"
        if reuse_build:
            require(lock.is_file() and not lock.is_symlink() and lock.stat().st_size > 0,
                    "post-build validation requires the build's generated MODULE.bazel.lock")
            require(not execute(["git", "ls-files", "--", "MODULE.bazel.lock"],
                                artifacts, receipt, "tracked-lock").strip(),
                    "MODULE.bazel.lock must be generated, not tracked")
            execute(["git", "check-ignore", "--quiet", "MODULE.bazel.lock"],
                    artifacts, receipt, "ignored-lock")
            receipt["initial_module_lock_sha256"] = sha(lock)
        else:
            require(not lock.exists(), "CI must start without a preexisting MODULE.bazel.lock")
        receipt["dependency_state"] = "existing-build" if reuse_build else "cold"
        tests = query_tests(suite, bazel, artifacts, receipt, execute)
        targets = [*tests, artifact_target]
        sources = declared_sources(targets, bazel, workspace, artifacts, receipt, execute, source_files)
        before = {name: sha(workspace / name) for name in sources}
        actions = artifacts / "actions.raw.json"
        actions.touch(mode=0o600, exist_ok=False)
        try:
            execute([bazel, "aquery", *options, "deps(set(" + " ".join(targets) + "))",
                     "--output=jsonproto"], artifacts, receipt, "actions", output_path=actions)
            audit = inspect_actions(actions)
        finally:
            actions.unlink(missing_ok=True)
        audit["targets"] = targets
        (artifacts / "execution-gate-audit.json").write_text(json.dumps(audit, indent=2, sort_keys=True) + "\n")
        receipt["action_audit"] = audit
        require(not audit["deb_outputs"] and not audit["packaging_wrappers"],
                "container CI contains a DEB or packaging wrapper action")

        exported = None
        if reuse_build:
            # Materialize production outputs before tests can populate their
            # cache. Keep the complete spawn commands and environments private.
            execution_log = artifacts / "artifact-execution.raw.json"
            execution_log.touch(mode=0o600, exist_ok=False)
            try:
                exported = build.collect_artifacts(
                    workspace, artifacts, receipt, artifact_target, bazel=bazel,
                    options=options, execution_log=execution_log)
                receipt["cache_reuse"] = cache_reuse.collect(
                    execution_log, artifacts / "build-events.jsonl", artifact_target)
            finally:
                execution_log.unlink(missing_ok=True)
            evidence = receipt["cache_reuse"]
            print(json.dumps({"cache_reuse": {
                **{key: value for key, value in evidence.items() if key != "actions"},
                "uncached_actions": [action for action in evidence["actions"] if not action["cache_hit"]]}},
                sort_keys=True), flush=True)

        labels = configured_test_labels(tests, bazel, options, artifacts, receipt, execute)
        events = artifacts / "test-events.jsonl"
        events.touch(mode=0o600, exist_ok=False)
        execute([bazel, "test", *options, "--nocache_test_results", "--test_output=errors",
                 "--nozip_undeclared_test_outputs", "--build_event_json_file=" + str(events), suite],
                artifacts, receipt, "tests")
        test_outputs, validations = collect_test_outputs(events, artifacts, tests, labels)
        receipt["tests"] = {canonical: "passed" for canonical in labels}
        require(len(validations) <= 1, "CI suite produced ambiguous package validation summaries")
        if validations:
            receipt["validation"] = next(iter(validations.values()))
        if exported is None:
            exported = build.collect_artifacts(workspace, artifacts, receipt, artifact_target, bazel=bazel, options=options)
        if validations:
            validation = receipt["validation"]
            require(isinstance(validation, dict), "package validation must be an object")
            validated = public_artifacts.artifact_hashes(validation.get("artifact_sha256", {}))
            published = {name: receipt["artifacts"][name]["sha256"] for name in exported}
            require(validated and validated == published,
                    "published artifacts differ from validated archive names or bytes")
        receipt["resolution"] = resolution.collect(
            workspace, artifacts, bazel=[bazel], source_files=sources if reuse_build else None)
        check_versions(artifacts / "module-graph.json")
        require({name: sha(workspace / name) for name in sources} == before,
                "container CI changed a checked source or package lock")
        report = {"schema": 1, "status": "passed", "bazel_version": version, "targets": list(labels),
                  "test_outputs": test_outputs, "source_hashes": before,
                  "module_lock_sha256": sha(workspace / "MODULE.bazel.lock"),
                  "execution_gate_audit": "execution-gate-audit.json", "scope": scope,
                  "ci_tests": suite, "ci_artifacts": artifact_target}
        if reuse_build:
            report["cache_reuse"] = receipt["cache_reuse"]
        (artifacts / "report.json").write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
        receipt["status"] = "passed"
    except Exception as error:
        receipt.update(status="failed", error=str(error))
        raise
    finally:
        receipt["elapsed_seconds"] = time.monotonic() - started
        (artifacts / "receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--container", required=True, help="Owner Bazel package, e.g. //dockers/docker-orchagent")
    parser.add_argument("--bazel", default="bazel")
    parser.add_argument("--bazel-arg", action="append", default=[])
    parser.add_argument("--source-file", action="append", default=[],
                        help="Additional workspace-relative preparation input to retain in source evidence")
    parser.add_argument("--artifacts", required=True, type=Path)
    parser.add_argument("--reuse-build", action="store_true",
                        help="Validate in the Make build workspace, keeping its generated lock and cached outputs")
    args = parser.parse_args(argv)
    try:
        run(args.container, workspace=ROOT, artifacts=args.artifacts.resolve(), bazel=args.bazel,
            options=[*OPTIONS, *args.bazel_arg], source_files=args.source_file,
            reuse_build=args.reuse_build)
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        parser.exit(1, "container CI failed: " + str(error) + "\n")


if __name__ == "__main__":
    main()
