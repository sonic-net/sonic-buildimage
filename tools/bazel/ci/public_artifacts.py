#!/usr/bin/env python3
"""Prepare an explicit, safe list of public container CI artifacts."""

import argparse
import hashlib
import json
import math
from pathlib import Path, PurePosixPath
import re
import stat
import tempfile
from urllib.parse import parse_qsl, urlsplit

SHA = re.compile(r"[0-9a-f]{64}")
REVISION = re.compile(r"[0-9a-f]{40}")
LABEL = re.compile(r"(?:@{1,2}[A-Za-z0-9_.+~-]+)?//[A-Za-z0-9_./+-]*:[A-Za-z0-9_./+-]+")
VERSION = re.compile(r"(?:bazel )?[0-9]+(?:\.[0-9]+){1,3}(?:[-.][A-Za-z0-9]+)*")
RELATIVE = re.compile(r"[A-Za-z0-9_./+-]+")
TOKEN = re.compile(
    r"-----BEGIN [A-Z ]*PRIVATE KEY-----|"
    r"\bgh[pousr]_[A-Za-z0-9_.-]{20,}|\bgithub_pat_[A-Za-z0-9_]{20,}|"
    r"\bsk-(?:proj-|svcacct-)?[A-Za-z0-9_-]{20,}|"
    r"\b(?:AKIA|ASIA)[A-Z0-9]{16}\b|"
    r"\beyJ[A-Za-z0-9_-]{10,}\.eyJ[A-Za-z0-9_-]{10,}\.[A-Za-z0-9_-]{10,}\b"
)
LOCAL_PATH = re.compile(
    r"/(?:home|Users)/[^/\s]+/(?:\.cache/|\.config/|\.local/|\.ssh/|work/|code/|actions-runner/|\.netrc\b|\.gitconfig\b)|"
    r"/__w/|/root/\.cache/|file://"
)
DIAGNOSTIC_KEYS = {
    "clientenv", "environment", "environmentvariables", "environ", "env", "argv",
    "commandline", "optionsparsed", "structuredcommandline",
    "unstructuredcommandline", "workingdirectory", "workspacedirectory",
}
STATUSES = {
    "PASSED", "FLAKY", "TIMEOUT", "FAILED", "INCOMPLETE", "REMOTE_FAILURE",
    "FAILED_TO_BUILD", "BLAZE_HALTED_BEFORE_TESTING", "NO_STATUS",
}
FILES = {
    "archive": [
        ("fixture-gzip", "artifacts/archive/archive-fixture.gz", "binary"),
        ("fixture-tar", "artifacts/archive/archive-fixture.tar", "binary"),
        ("fixture-hashes", "artifacts/archive/archive-sha256.txt", "hashes"),
        ("bazel-version", "artifacts/archive/bazel-version.txt", "version"),
        ("module-lock", "artifacts/archive/MODULE.bazel.lock", "lock"),
        ("module-graph", "artifacts/archive/module-graph.json", "graph"),
        ("cache-result", "artifacts/archive/agent-cache/result.json", "cache"),
        ("startup-hash", "artifacts/config-engine/sha256.txt", "hashes"),
        ("common-wheel", "artifacts/config-engine/sonic_py_common-1.0-py3-none-any.whl", "binary"),
        ("config-wheel", "artifacts/config-engine/sonic_config_engine-1.0-py3-none-any.whl", "binary"),
        ("yang-mgmt-wheel", "artifacts/config-engine/sonic_yang_mgmt-1.0-py3-none-any.whl", "binary"),
        ("containercfgd-wheel", "artifacts/config-engine/sonic_containercfgd-1.0-py3-none-any.whl", "binary"),
        ("supervisor-wheel", "artifacts/config-engine/sonic_supervisord_utilities-1.0-py3-none-any.whl", "binary"),
        ("wheel-hashes", "artifacts/config-engine/wheels-sha256.txt", "hashes"),
    ],
    "source": [
        ("swss-tar", "artifacts/swss/swss.tar", "binary"),
        ("protobuf-tar", "artifacts/swss/protobuf.tar", "binary"),
        ("runtime-dependencies", "artifacts/swss/rdeps.tar", "binary"),
        ("startup-files", "artifacts/swss/config.tar", "binary"),
        ("debug-symbols", "artifacts/swss/debug-symbols.tar", "binary"),
        ("module-lock", "artifacts/swss/MODULE.bazel.lock", "lock"),
        ("module-graph", "artifacts/swss/module-graph.json", "graph"),
    ],
    "syncd": [
        ("revision-file", "artifacts/syncd-vs/revision.txt", "revision"),
        ("input-locks", "artifacts/syncd-vs/input-locks.sha256", "hashes"),
        ("module-lock", "artifacts/syncd-vs/bazel/MODULE.bazel.lock", "lock"),
        ("module-graph", "artifacts/syncd-vs/bazel/module-graph.json", "graph"),
        ("installed-package-status", "artifacts/syncd-vs/bazel/installed_packages.status", "binary"),
        ("installed-package-metadata", "artifacts/syncd-vs/bazel/installed_packages.json", "binary"),
    ],
    "vs": [],
}
BEP = {
    "archive": [
        ("archive-tests", "artifacts/archive/test-events.jsonl", 12),
        ("python-tests", "artifacts/config-engine/test-events.jsonl", 18),
    ],
    "source": [("source-tests", "artifacts/swss/test-events.jsonl", None)],
    "syncd": [("syncd-tests", "artifacts/syncd-vs/bazel/test-events.jsonl", None)],
    "vs": [],
}
RECEIPT_TEST_COUNTS = {"archive": ("python", 13)}
SUMMARY = {
    "archive": "artifacts/archive/public-summary.json",
    "source": "artifacts/swss/public-summary.json",
    "syncd": "artifacts/syncd-vs/public-summary.json",
    "vs": "artifacts/vs/public-summary.json",
}
SOURCE_ARTIFACT_TARGETS = {
    "source": "//dockers/docker-orchagent:ci_artifacts",
    "syncd": "//platform/vs/docker-syncd-vs:ci_artifacts",
}


class PublicArtifactError(ValueError):
    pass


def require(condition, reason):
    if not condition:
        raise PublicArtifactError(reason)


def integer(value):
    require(isinstance(value, int) and not isinstance(value, bool) and value >= 0, "invalid-count")
    return value


def digest(value):
    require(isinstance(value, str) and SHA.fullmatch(value), "invalid-digest")
    return value


def label(value):
    require(isinstance(value, str) and LABEL.fullmatch(value), "invalid-label")
    return value


def safe_text(value):
    require(all(ord(char) >= 32 or char in "\n\r\t" for char in value), "control-character")
    require(not TOKEN.search(value), "credential-pattern")
    require(not LOCAL_PATH.search(value), "local-path")
    for raw in re.findall(r"https?://[^\s\"'<>]+", value):
        parsed = urlsplit(raw.rstrip(".,;)]}"))
        require(parsed.username is None and parsed.password is None, "url-credentials")
        names = {name.casefold() for name, _ in parse_qsl(parsed.query)}
        require(not names & {"sig", "token", "access_token", "api_key", "key", "auth", "password", "secret"}, "sensitive-url-query")


def safe_json(value):
    if isinstance(value, dict):
        for key, child in value.items():
            require(isinstance(key, str), "invalid-json-key")
            normalized = re.sub(r"[^a-z0-9]", "", key.casefold())
            if normalized == "envvariables":
                require(isinstance(child, dict) and not child, "nonempty-recorded-environment")
            require(normalized not in DIAGNOSTIC_KEYS, "diagnostic-json-field")
            require(not normalized.endswith(("token", "tokens", "password", "passwords", "secret", "secrets",
                                             "credential", "credentials", "authorization", "apikey", "privatekey",
                                             "accesskey", "secretkey", "cookie", "cookies")), "credential-json-field")
            safe_text(key)
            safe_json(child)
    elif isinstance(value, list):
        for child in value:
            safe_json(child)
    elif isinstance(value, str):
        safe_text(value)
    else:
        require(value is None or isinstance(value, (bool, int)) or isinstance(value, float) and math.isfinite(value), "invalid-json-value")


def load_json(value):
    def unique_object(items):
        result = {}
        for key, child in items:
            require(key not in result, "duplicate-json-key")
            result[key] = child
        return result

    def reject_constant(_value):
        raise PublicArtifactError("invalid-json-number")

    return json.loads(value, object_pairs_hook=unique_object, parse_constant=reject_constant)


def public_text(value, kind):
    safe_text(value)
    if kind == "version":
        require(VERSION.fullmatch(value.strip()), "invalid-version-file")
    elif kind == "revision":
        require(REVISION.fullmatch(value.strip()), "invalid-revision-file")
    elif kind == "hashes":
        lines = value.splitlines()
        require(bool(lines), "empty-hash-file")
        for line in lines:
            fields = line.split("  ")
            require(len(fields) == 2, "invalid-hash-file")
            digest(fields[0])
            relative_name(fields[1])
    else:
        raise PublicArtifactError("unknown-text-kind")


def dependency_json(value, kind):
    require(isinstance(value, dict), "invalid-dependency-json")
    if kind == "lock":
        require({"lockFileVersion", "registryFileHashes", "moduleExtensions"} <= set(value), "invalid-module-lock")
        require(set(value) <= {"lockFileVersion", "registryFileHashes", "selectedYankedVersions", "moduleExtensions", "facts"}, "unknown-module-lock-field")
    elif kind == "graph":
        require({"key", "name", "version", "dependencies"} <= set(value), "invalid-module-graph")
        require(set(value) <= {"key", "name", "version", "apparentName", "root", "dependencies", "indirectDependencies", "cycles"}, "unknown-module-graph-field")
    elif kind == "cache":
        require(set(value) == {"target", "fresh_checkouts", "fresh_output_bases", "fresh_containers", "cold", "warm", "changed"}, "invalid-cache-result")
        label(value["target"])
        for name in ("fresh_checkouts", "fresh_output_bases", "fresh_containers"):
            integer(value[name])
        for phase in ("cold", "warm", "changed"):
            item = value[phase]
            require(set(item) == {"actions", "archive_sha256"}, "invalid-cache-phase")
            digest(item["archive_sha256"])
            require(set(item["actions"]) == {"Genrule", "OCIImage", "GzipCompress"}, "invalid-cache-actions")
            for action in item["actions"].values():
                require(set(action) == {"runner", "cache_hit"}, "invalid-cache-action")
                require(isinstance(action["cache_hit"], bool), "invalid-cache-hit")
                require(action["runner"] is None or isinstance(action["runner"], str) and re.fullmatch(r"[A-Za-z0-9_. -]{1,80}", action["runner"]), "invalid-cache-runner")
    safe_json(value)


def bep_summary(lines, partial=False):
    result = {"event_count": 0, "command": None, "bazel_version": None, "targets": {}, "tests": {}, "finished": None, "truncated": False}
    for raw in lines:
        try:
            event = load_json(raw)
        except json.JSONDecodeError:
            if partial:
                result["truncated"] = True
                break
            raise PublicArtifactError("invalid-bep")
        require(isinstance(event, dict) and isinstance(event.get("id"), dict), "invalid-bep-event")
        result["event_count"] += 1
        identifier = event["id"]
        if "started" in identifier:
            started = event.get("started", {})
            command = started.get("command")
            version = started.get("buildToolVersion")
            require(command in {"build", "test"}, "invalid-bep-command")
            require(isinstance(version, str) and VERSION.fullmatch(version), "invalid-bep-version")
            result["command"], result["bazel_version"] = command, version
        elif "targetCompleted" in identifier:
            name = label(identifier["targetCompleted"].get("label"))
            success = event.get("completed", {}).get("success", False)
            require(isinstance(success, bool), "invalid-target-status")
            result["targets"][name] = success
        elif "testSummary" in identifier:
            name = label(identifier["testSummary"].get("label"))
            summary = event.get("testSummary", {})
            status = summary.get("overallStatus")
            require(status in STATUSES, "invalid-test-status")
            result["tests"][name] = {"status": status, **{
                key: integer(summary[key]) for key in ("totalRunCount", "runCount", "attemptCount", "shardCount") if key in summary
            }}
        elif "buildFinished" in identifier:
            finished = event.get("finished", {})
            success = finished.get("overallSuccess", False)
            require(isinstance(success, bool), "invalid-build-status")
            code = finished.get("exitCode", {}).get("code", 0 if success else None)
            require(code is None or isinstance(code, int), "invalid-exit-code")
            result["finished"] = {"success": success, "exit_code": code}
    return result


def relative_name(value):
    require(isinstance(value, str) and RELATIVE.fullmatch(value), "invalid-relative-name")
    path = PurePosixPath(value)
    require(not path.is_absolute() and ".." not in path.parts, "invalid-relative-name")
    return value


def artifact_hashes(value):
    """Accept only public archive basenames and full SHA256 validation bindings."""
    require(isinstance(value, dict), "invalid-artifact-bindings")
    hashes = {}
    for name, item in value.items():
        name = relative_name(name)
        require(PurePosixPath(name).name == name and name not in (".", ".."), "invalid-artifact-basename")
        hashes[name] = digest(item)
    return hashes


def cache_reuse_summary(value):
    """Publish only aggregate action identities and verified cache-hit counts."""
    require(isinstance(value, dict) and set(value) == {
        "artifact_target", "phase", "action_count", "cache_hit_count", "executed_count", "actions",
    }, "invalid-cache-reuse")
    require(value["phase"] == "before_tests", "invalid-cache-reuse-phase")
    counts = {name: integer(value[name]) for name in ("action_count", "cache_hit_count", "executed_count")}
    require(counts["action_count"] > 0 and isinstance(value["actions"], list) and value["actions"],
            "empty-cache-reuse")
    actions, keys = [], []
    for item in value["actions"]:
        require(isinstance(item, dict) and set(item) == {"target", "mnemonic", "runner", "cache_hit", "count"},
                "invalid-cache-reuse-action")
        target = None if item["target"] is None else label(item["target"])
        mnemonic, runner = item["mnemonic"], item["runner"]
        require(isinstance(mnemonic, str) and re.fullmatch(r"[A-Za-z][A-Za-z0-9_]{0,79}", mnemonic),
                "invalid-cache-reuse-mnemonic")
        require(isinstance(runner, str) and re.fullmatch(r"[A-Za-z0-9_. -]{1,80}", runner),
                "invalid-cache-reuse-runner")
        require(isinstance(item["cache_hit"], bool) and integer(item["count"]) > 0,
                "invalid-cache-reuse-count")
        require(item["cache_hit"] == (runner in {"disk cache hit", "remote cache hit"}),
                "contradictory-cache-reuse-runner")
        actions.append({"target": target, "mnemonic": mnemonic, "runner": runner,
                        "cache_hit": item["cache_hit"], "count": item["count"]})
        keys.append((target or "", mnemonic, runner, item["cache_hit"]))
    require(keys == sorted(set(keys)), "duplicate-or-unsorted-cache-reuse-actions")
    hits = sum(item["count"] for item in actions if item["cache_hit"])
    executed = sum(item["count"] for item in actions if not item["cache_hit"])
    require(counts == {"action_count": hits + executed, "cache_hit_count": hits, "executed_count": executed},
            "inconsistent-cache-reuse-counts")
    return {"artifact_target": label(value["artifact_target"]), "phase": "before_tests", **counts, "actions": actions}


def receipt_summary(value, kind):
    require(isinstance(value, dict), "invalid-receipt")
    if kind == "python":
        require(value.get("status") in {"running", "passed", "failed"}, "invalid-receipt-status")
        require(value.get("architecture") in {"amd64", "arm64"}, "invalid-architecture")
        require(isinstance(value.get("tests", {}), dict), "invalid-tests")
        tests = {}
        for name, item in value.get("tests", {}).items():
            require(isinstance(item, dict), "invalid-test-result")
            counts = item.get("result", {})
            require(isinstance(counts, dict), "invalid-test-counts")
            tests[label(name)] = {key: integer(counts[key]) for key in ("tests", "failures", "errors", "skipped")}
        return {"status": value["status"], "architecture": value["architecture"], "tests": tests}
    if kind == "source":
        require(value.get("status") in {"running", "passed", "failed"}, "invalid-receipt-status")
        revision = value.get("revision")
        require(revision is None or isinstance(revision, str) and REVISION.fullmatch(revision), "invalid-revision")
        require(isinstance(value.get("tests", {}), dict), "invalid-tests")
        tests = {}
        for name, status in value.get("tests", {}).items():
            require(status in {"passed", "failed", "running"}, "invalid-test-status")
            tests[label(name)] = status
        validation = value.get("validation", {})
        require(isinstance(validation, dict), "invalid-validation")
        require(isinstance(validation.get("programs", []), list) and isinstance(validation.get("debug_pairs", []), list), "invalid-validation-lists")
        require(isinstance(validation.get("source_contract", {}), dict), "invalid-source-contract")
        contract = {relative_name(name): digest(item) for name, item in validation.get("source_contract", {}).items()}
        result = {"status": value["status"], "revision": revision, "tests": tests,
                  "program_count": len(validation.get("programs", [])),
                  "debug_pair_count": len(validation.get("debug_pairs", [])), "source_contract": contract,
                  "artifact_sha256": artifact_hashes(validation.get("artifact_sha256", {}))}
        if "cache_reuse" in value:
            result["cache_reuse"] = cache_reuse_summary(value["cache_reuse"])
        return result
    if kind == "syncd":
        require(isinstance(value.get("targets", []), list) and isinstance(value.get("source_hashes", {}), dict), "invalid-syncd-receipt")
        targets = [label(item) for item in value.get("targets", [])]
        hashes = {relative_name(name): digest(item) for name, item in value.get("source_hashes", {}).items()}
        require(value.get("status") in {"running", "passed", "failed"}, "invalid-receipt-status")
        require(len(set(targets)) == len(targets), "duplicate-test-target")
        result = {"status": value["status"], "targets": targets, "source_hashes": hashes,
                  "module_lock_sha256": digest(value.get("module_lock_sha256"))}
        if "cache_reuse" in value:
            result["cache_reuse"] = cache_reuse_summary(value["cache_reuse"])
        return result
    if kind == "p4rt":
        require(value.get("status") == "passed", "invalid-p4rt-status")
        require(isinstance(value.get("with_dwp", {}), dict) and isinstance(value.get("packages", {}), dict), "invalid-p4rt-receipt")
        found = value.get("with_dwp", {}).get("found")
        require(isinstance(found, bool), "invalid-p4rt-lookup")
        return {"status": "passed", "runtime_sha256": digest(value.get("runtime_sha256")),
                "dwp_sha256": digest(value.get("dwp_sha256")), "source_sha256": digest(value.get("source_sha256")),
                "dwp_size": integer(value.get("dwp_size")), "compilation_units": integer(value.get("compilation_units")),
                "source_found": found, "source_line_number": integer(value.get("with_dwp", {}).get("line")),
                "packages": {name: digest(value.get("packages", {}).get(name)) for name in ("runtime_sha256", "debug_sha256")}}
    raise PublicArtifactError("unknown-receipt-kind")


def source_validation_summary(value, kind, identity):
    """Bind completed source checks to this image build without republishing them."""
    require(isinstance(value, dict) and value.get("schema") == 1, "invalid-source-summary")
    require(value.get("kind") == kind and value.get("job_status") == "success" and
            value.get("complete") is True and value.get("missing") == [] and
            value.get("blocked") == [], "incomplete-source-summary")
    require(all(key in value and value[key] == expected for key, expected in identity.items()),
            "source-summary-identity-mismatch")
    build = value.get("builds", {}).get(kind + "-tests", {})
    require(isinstance(build, dict) and build.get("command") == "test" and
            build.get("finished") == {"success": True, "exit_code": 0} and
            build.get("truncated") is False, "incomplete-source-tests")
    tests = build.get("tests")
    require(isinstance(tests, dict) and bool(tests), "missing-source-tests")
    for name, result in tests.items():
        label(name)
        require(isinstance(result, dict) and result.get("status") in {"PASSED", "FLAKY"},
                "failed-source-test")
    reuse = cache_reuse_summary(value.get("receipts", {}).get(kind, {}).get("cache_reuse"))
    require(reuse["artifact_target"] == SOURCE_ARTIFACT_TARGETS[kind], "wrong-cache-reuse-target")
    return {"test_count": len(tests),
            "cache_reuse": {key: item for key, item in reuse.items() if key != "actions"}}


def prepare(kind, workspace, upload_root, job_status, revision, architecture, head_sha=None, base_sha=None):
    workspace, upload_root = Path(workspace).resolve(), Path(upload_root).resolve()
    require(workspace.is_relative_to(upload_root), "workspace-outside-upload-root")
    require(kind in FILES and job_status in {"success", "failure", "cancelled"}, "invalid-mode")
    require(architecture in {"amd64", "arm64"} and (kind == "archive" or architecture == "amd64"), "invalid-architecture")
    require(isinstance(revision, str) and REVISION.fullmatch(revision), "invalid-revision")
    for value in (head_sha, base_sha):
        require(value in (None, "") or isinstance(value, str) and REVISION.fullmatch(value), "invalid-revision")
    missing, blocked, selected, paths = [], [], {}, []

    def input_path(logical, relative):
        path = workspace / relative_name(relative)
        try:
            for parent in path.parents:
                if parent == workspace:
                    break
                require(not parent.is_symlink(), "symlink-parent")
            info = path.lstat()
            require(stat.S_ISREG(info.st_mode) and not path.is_symlink(), "nonregular-input")
            require(path.resolve().is_relative_to(workspace), "input-outside-workspace")
            require(info.st_size > 0, "empty-input")
            return path
        except FileNotFoundError:
            missing.append(logical)
        except (OSError, PublicArtifactError):
            blocked.append({"input": logical, "reason": "unreadable-or-unsafe-input"})
        return None

    def read_json(logical, relative):
        path = input_path(logical, relative)
        if path is None:
            return None
        try:
            require(path.stat().st_size <= 64 * 1024 * 1024, "oversized-json")
            return load_json(path.read_bytes())
        except (OSError, ValueError, TypeError, RecursionError):
            blocked.append({"input": logical, "reason": "invalid-json"})
            return None

    # A failed job publishes only extracted status fields, never partial outputs.
    for logical, relative, file_kind in FILES[kind] if job_status == "success" else []:
        path = input_path(logical, relative)
        if path is None:
            continue
        try:
            if file_kind in {"lock", "graph", "cache"}:
                require(path.stat().st_size <= 64 * 1024 * 1024, "oversized-json")
                dependency_json(load_json(path.read_bytes()), file_kind)
            elif file_kind != "binary":
                require(path.stat().st_size <= 16 * 1024 * 1024, "oversized-text")
                value = path.read_text()
                public_text(value, file_kind)
                if file_kind == "revision":
                    require(value.strip() == revision, "revision-mismatch")
            before = path.stat()
            digest = hashlib.sha256()
            with path.open("rb") as stream:
                for block in iter(lambda: stream.read(1024 * 1024), b""):
                    digest.update(block)
            file_digest = digest.hexdigest()
            after = path.stat()
            require((before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns) ==
                    (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns), "changed-public-file")
            selected[logical] = {"sha256": file_digest, "bytes": after.st_size}
            paths.append(str(path.resolve().relative_to(upload_root)))
        except (OSError, ValueError, TypeError, RecursionError):
            blocked.append({"input": logical, "reason": "unsafe-public-file"})

    builds = {}
    for logical, relative, count in BEP[kind]:
        path = input_path(logical, relative)
        if path is None:
            continue
        try:
            require(path.stat().st_size <= 512 * 1024 * 1024, "oversized-bep")
            with path.open() as stream:
                summary = bep_summary(stream, partial=job_status != "success")
            if job_status == "success":
                require(summary["finished"] and summary["finished"]["success"], "incomplete-bep")
                require(bool(summary["tests"]) and (count is None or len(summary["tests"]) == count) and
                        all(item["status"] in {"PASSED", "FLAKY"} for item in summary["tests"].values()), "incomplete-tests")
            builds[logical] = summary
        except (OSError, ValueError, TypeError, RecursionError):
            blocked.append({"input": logical, "reason": "invalid-bep-summary"})

    receipts = {}
    receipt_specs = {
        "archive": [("python", "artifacts/config-engine/receipt.json", "python")],
        "source": [("source", "artifacts/swss/receipt.json", "source")],
        "syncd": [("syncd", "artifacts/syncd-vs/bazel/report.json", "syncd")],
        "vs": [("p4rt", "artifacts/vs/p4rt-debug-verification.json", "p4rt")],
    }
    for logical, relative, receipt_kind in receipt_specs[kind]:
        value = read_json(logical + "-receipt", relative)
        if value is None:
            continue
        try:
            receipts[logical] = receipt_summary(value, receipt_kind)
        except (ValueError, TypeError, KeyError, AttributeError, RecursionError):
            blocked.append({"input": logical + "-receipt", "reason": "invalid-receipt-summary"})
    if kind == "syncd":
        value = read_json("execution-audit", "artifacts/syncd-vs/bazel/execution-gate-audit.json")
        if value is not None:
            try:
                receipts["execution_audit"] = {"action_count": integer(value.get("action_count")),
                    "output_count": integer(value.get("output_count")),
                    "deb_output_count": len(value.get("deb_outputs", [])),
                    "packaging_wrapper_count": len(value.get("packaging_wrappers", []))}
                if job_status == "success":
                    require(not receipts["execution_audit"]["deb_output_count"] and not receipts["execution_audit"]["packaging_wrapper_count"], "unsafe-execution-audit")
            except (ValueError, TypeError):
                blocked.append({"input": "execution-audit", "reason": "invalid-execution-audit"})
    if kind == "vs" and job_status == "success":
        identity = {"revision": revision, "architecture": architecture,
                    "head_sha": head_sha or None, "base_sha": base_sha or None}
        receipts["source_validation"] = {}
        for source_kind in ("source", "syncd"):
            logical = source_kind + "-public-summary"
            value = read_json(logical, SUMMARY[source_kind])
            try:
                receipts["source_validation"][source_kind] = source_validation_summary(
                    value, source_kind, identity)
            except (ValueError, TypeError, KeyError, AttributeError, RecursionError):
                blocked.append({"input": logical, "reason": "invalid-source-validation"})
        outputs = {}
        for name in ("sonic-vs.img.gz", "docker-orchagent.gz", "docker-orchagent-dbg.gz",
                     "docker-syncd-vs.gz", "docker-syncd-vs-dbg.gz"):
            path = input_path("output-" + name, "target/" + name)
            if path is not None:
                outputs[name] = {"bytes": path.stat().st_size}
        receipts["outputs"] = outputs

    if job_status == "success":
        if kind == "archive":
            value = receipts.get("python", {})
            if len(value.get("tests", {})) != RECEIPT_TEST_COUNTS["archive"][1] or value.get("status") != "passed":
                blocked.append({"input": "python-receipt", "reason": "incomplete-receipt"})
        if kind in {"source", "syncd"}:
            value = receipts.get(kind, {})
            expected_tests = value.get("targets", value.get("tests", {}))
            observed = builds.get(kind + "-tests", {}).get("tests", {})
            if (not expected_tests or set(expected_tests) != set(observed) or
                    value.get("status") != "passed" or
                    any(status != "passed" for status in value.get("tests", {}).values())):
                blocked.append({"input": kind + "-receipt", "reason": "incomplete-receipt"})
        if kind == "source":
            value = receipts.get("source", {})
            if not value.get("program_count") or not value.get("debug_pair_count") or not value.get("source_contract"):
                blocked.append({"input": "source-receipt", "reason": "missing-package-validation"})
            published = {PurePosixPath(relative).name: selected[logical]["sha256"]
                         for logical, relative, file_kind in FILES[kind]
                         if file_kind == "binary" and logical in selected}
            if not value.get("artifact_sha256") or value["artifact_sha256"] != published:
                blocked.append({"input": "source-receipt", "reason": "validated-artifact-mismatch"})
        if kind == "archive" and receipts.get("python", {}).get("architecture") != architecture:
            blocked.append({"input": "python-receipt", "reason": "architecture-mismatch"})
        if kind == "source" and receipts.get("source", {}).get("revision") != revision:
            blocked.append({"input": "source-receipt", "reason": "revision-mismatch"})
        if kind == "syncd" and receipts.get("syncd", {}).get("module_lock_sha256") != selected.get("module-lock", {}).get("sha256"):
            blocked.append({"input": "syncd-receipt", "reason": "module-lock-mismatch"})
        if kind == "vs" and not receipts.get("p4rt", {}).get("source_found"):
            blocked.append({"input": "p4rt-receipt", "reason": "incomplete-p4rt-verification"})

    summary = {"schema": 1, "kind": kind, "job_status": job_status, "revision": revision, "architecture": architecture,
               "head_sha": head_sha or None, "base_sha": base_sha or None, "files": selected,
               "builds": builds, "receipts": receipts, "missing": sorted(set(missing)), "blocked": blocked,
               "complete": job_status == "success" and not missing and not blocked}
    safe_json(summary)
    summary_path = workspace / SUMMARY[kind]
    for parent in summary_path.parents:
        if parent == workspace:
            break
        require(not parent.is_symlink(), "symlink-summary-parent")
    require(summary_path.parent.resolve().is_relative_to(workspace), "summary-outside-workspace")
    summary_path.parent.mkdir(parents=True, exist_ok=True)
    require(summary_path.parent.resolve().is_relative_to(workspace), "summary-outside-workspace")
    with tempfile.NamedTemporaryFile("w", dir=summary_path.parent, delete=False) as stream:
        temporary = Path(stream.name)
        json.dump(summary, stream, indent=2, sort_keys=True)
        stream.write("\n")
    temporary.chmod(0o644)
    temporary.replace(summary_path)
    ready = not blocked and (job_status != "success" or not missing)
    if ready:
        paths.append(str(summary_path.resolve().relative_to(upload_root)))
    return summary, sorted(paths) if ready else [], ready


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("kind", choices=FILES)
    parser.add_argument("--workspace", type=Path, default=Path.cwd())
    parser.add_argument("--upload-root", type=Path, required=True)
    parser.add_argument("--job-status", choices=("success", "failure", "cancelled"), required=True)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--architecture", choices=("amd64", "arm64"), required=True)
    parser.add_argument("--head-sha")
    parser.add_argument("--base-sha")
    parser.add_argument("--github-output", type=Path, required=True)
    args = parser.parse_args()
    try:
        summary, paths, ready = prepare(args.kind, args.workspace, args.upload_root, args.job_status,
                                        args.revision, args.architecture, args.head_sha, args.base_sha)
    except (OSError, ValueError, TypeError, AttributeError, RecursionError):
        parser.exit(1, "public artifact preparation failed\n")
    if ready:
        with args.github_output.open("a") as stream:
            stream.write("paths<<SONIC_PUBLIC_ARTIFACT_PATHS\n")
            stream.write("\n".join(paths) + "\nSONIC_PUBLIC_ARTIFACT_PATHS\n")
    print(json.dumps({"kind": args.kind, "ready": ready, "complete": summary["complete"],
                      "public_file_count": len(paths), "missing": summary["missing"], "blocked": summary["blocked"]}))
    raise SystemExit(0 if ready else 1)


if __name__ == "__main__":
    main()
