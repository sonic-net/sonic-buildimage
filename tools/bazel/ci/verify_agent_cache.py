#!/usr/bin/env python3
"""Verify disk-cache reuse across fresh checkouts and Bazel output bases."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from urllib.parse import unquote, urlparse

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.build_helpers import cache_options
from tools.bazel.ci.public_artifacts import LOCAL_PATH, PublicArtifactError, safe_text


TARGET = "//tools/bazel/tests:archive-fixture.gz"
ACTIONS = {
    "Genrule": "//tools/bazel/tests:archive_fixture_layer",
    "OCIImage": "//tools/bazel/tests:archive-fixture",
    "GzipCompress": TARGET,
}


def json_records(path):
    """Bazel's execution log contains consecutive, possibly multiline JSON objects."""
    decoder = json.JSONDecoder()
    text = path.read_text()
    position = 0
    while position < len(text):
        if text[position].isspace():
            position += 1
            continue
        value, position = decoder.raw_decode(text, position)
        yield value


def action_evidence(path):
    found = {}
    for event in json_records(path):
        mnemonic = event.get("mnemonic")
        if mnemonic in ACTIONS and event.get("targetLabel") == ACTIONS[mnemonic]:
            found[mnemonic] = {
                "runner": event.get("runner"),
                "cache_hit": event.get("cacheHit", False),
            }
    if set(found) != set(ACTIONS):
        raise ValueError("missing fixture actions in execution log: " + str(set(ACTIONS) - set(found)))
    return found


def archive_output(path):
    outputs = set()
    for event in json_records(path):
        for file in event.get("namedSetOfFiles", {}).get("files", []):
            uri = file.get("uri", "")
            if uri.startswith("file:") and uri.endswith("/archive-fixture.gz"):
                outputs.add(Path(unquote(urlparse(uri).path)))
    if len(outputs) != 1:
        raise ValueError("expected one fixture archive in build events, got " + str(outputs))
    return outputs.pop()


def snapshot(source, destination):
    """Copy current tracked contents, preserving local edits but no Bazel state."""
    destination.mkdir()
    files = subprocess.check_output(["git", "ls-files", "-z"], cwd=source).split(b"\0")
    for name in files:
        if not name:
            continue
        relative = Path(os.fsdecode(name))
        original = source / relative
        if original.is_file() or original.is_symlink():
            target = destination / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(original, target, follow_symlinks=False)


def print_failure_tail(path):
    """Expose a bounded diagnostic without publishing raw command/environment data."""
    with path.open("rb") as log:
        log.seek(0, os.SEEK_END)
        start = max(0, log.tell() - 65536)
        log.seek(start)
        tail = log.read(65536)
        if start:
            tail = tail.partition(b"\n")[2]  # Discard a possibly truncated credential or URL.
        lines = tail.decode("utf-8", errors="replace").splitlines()[-80:]
    secrets = [part for name, value in os.environ.items()
               if any(word in name.upper() for word in
                      ("TOKEN", "SECRET", "PASSWORD", "CREDENTIAL", "AUTH", "API_KEY", "ACCESS_KEY", "PRIVATE_KEY"))
               for part in value.splitlines() if part]
    rendered = []
    remaining = 16384
    for line in reversed(lines):
        for secret in secrets:
            line = line.replace(secret, "[redacted]")
        line = re.sub(r"\x1b\[[0-?]*[ -/]*[@-~]", "", line)
        line = LOCAL_PATH.sub("[local]/", line)
        try:
            # Actions trims whitespace for :: commands and recognizes ##[ anywhere.
            if line.lstrip().startswith("::") or "##[" in line:
                raise PublicArtifactError("workflow-command")
            if re.search(r"authorization|cookie|token|secret|password|credential|api[_-]?key", line, re.I):
                raise PublicArtifactError("credential-diagnostic")
            safe_text(line)
        except PublicArtifactError:
            line = "[withheld unsafe diagnostic line]"
        # Redaction can expand short values; bound rendered UTF-8 bytes as well.
        line = line.encode("utf-8")[:min(2048, remaining) - 3].decode("utf-8", errors="ignore")
        output = "  " + line + "\n"
        rendered.append(output)
        remaining -= len(output.encode("utf-8"))
        if remaining < 3:
            break
    sys.stderr.write("".join(reversed(rendered)))


def run_build(args, phase, checkout, cache, output_base):
    evidence = args.artifacts / phase
    evidence.mkdir()
    in_container = bool(args.docker_image)
    visible_cache = "/bazel_cache" if in_container else str(cache)
    options = [option.replace(str(cache), visible_cache) for option in cache_options(cache)]
    if args.repository_cache and not in_container:
        options.append("--repository_cache=" + str(args.repository_cache))
    visible_checkout = "/workspace" if in_container else str(checkout)
    visible_evidence = "/artifacts" if in_container else str(evidence)
    user_root = output_base.with_name(output_base.name + "-user")
    visible_output = "/output-base" if in_container else str(output_base)
    visible_user = "/bazel-user" if in_container else str(user_root)
    command = [args.bazel, "--batch", *args.bazel_startup_arg,
               "--output_base=" + visible_output,
               "--output_user_root=" + visible_user,
               "build", *args.bazel_arg, *options,
               "--remote_cache=", "--remote_executor=", "--experimental_remote_downloader=",
               "--lockfile_mode=update",
               "--execution_log_json_file=" + visible_evidence + "/execution.json",
               "--build_event_json_file=" + visible_evidence + "/events.jsonl", TARGET]
    if in_container:
        output_base.mkdir(exist_ok=True)
        user_root.mkdir(exist_ok=True)
        mounts = ["-v", str(checkout) + ":/workspace",
                  "-v", str(cache) + ":/bazel_cache",
                  "-v", str(evidence) + ":/artifacts",
                  "-v", str(output_base) + ":/output-base",
                  "-v", str(user_root) + ":/bazel-user"]
        if args.repository_cache:
            mounts += ["-v", str(args.repository_cache) + ":/bazel_cache/repository_cache"]
        command = ["docker", "run", "--rm", "--user", f"{os.getuid()}:{os.getgid()}",
                   *mounts, *args.docker_arg, "-w", visible_checkout,
                   args.docker_image, *command]
    (evidence / "command.json").write_text(json.dumps(command, indent=2) + "\n")
    with (evidence / "build.log").open("w") as log:
        result = subprocess.run(command, cwd=checkout, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        print(f"{phase} build failed with exit code {result.returncode}; last log lines:",
              file=sys.stderr, flush=True)
        print_failure_tail(evidence / "build.log")
        sys.stderr.flush()
        raise RuntimeError(
            f"{phase} build failed with exit code {result.returncode}; see {evidence / 'build.log'}")
    actions = action_evidence(evidence / "execution.json")
    output = archive_output(evidence / "events.jsonl")
    if in_container:
        output = output_base / output.relative_to("/output-base")
    archive = evidence / "archive-fixture.gz"
    shutil.copyfile(output, archive)
    return {"actions": actions, "archive_sha256": hashlib.sha256(archive.read_bytes()).hexdigest()}


def verify(cold, warm, changed):
    for name in ACTIONS:
        if cold["actions"][name]["cache_hit"]:
            raise ValueError(name + " unexpectedly hit the new, empty cache")
        hit = warm["actions"][name]
        if not hit["cache_hit"] or "disk" not in str(hit["runner"]).lower():
            raise ValueError(name + " was not restored from the shared disk cache: " + str(hit))
        if changed["actions"][name]["cache_hit"]:
            raise ValueError(name + " incorrectly reused the old output after the source changed")
    if cold["archive_sha256"] != warm["archive_sha256"]:
        raise ValueError("unchanged source produced different archives across fresh checkouts")
    if changed["archive_sha256"] == warm["archive_sha256"]:
        raise ValueError("changed source did not change the archive")


def remove_temporary_work(directory):
    """Bazel makes output directories read-only; never follow their symlinks."""
    directory.chmod(0o700)
    for root, directories, _ in os.walk(directory, followlinks=False):
        for name in directories:
            child = Path(root) / name
            if not child.is_symlink():
                child.chmod(0o700)
    shutil.rmtree(directory)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", type=Path, required=True, help="new evidence directory")
    parser.add_argument("--bazel", default="bazel")
    parser.add_argument("--bazel-startup-arg", action="append", default=[])
    parser.add_argument("--bazel-arg", action="append", default=[])
    parser.add_argument("--repository-cache", type=Path)
    parser.add_argument("--docker-image", help="optional fresh container for each build")
    parser.add_argument("--docker-arg", action="append", default=[], help="extra docker run argument")
    args = parser.parse_args()
    args.artifacts = args.artifacts.resolve()
    if args.repository_cache:
        args.repository_cache = args.repository_cache.resolve()
    args.artifacts.mkdir(parents=True, exist_ok=False)
    # Keep large temporary build trees out of uploaded evidence, then remove
    # only this verifier's new directories. The agent's real cache is untouched.
    work = args.artifacts.with_name("." + args.artifacts.name + "-work")
    work.mkdir(exist_ok=False)
    try:
        cache = work / "cache"
        first, second = work / "checkout-first", work / "checkout-second"
        snapshot(ROOT, first)
        snapshot(ROOT, second)
        # A new action cache is mandatory. Only dependency downloads may be reused.
        cold = run_build(args, "cold", first, cache, work / "output-first")
        warm = run_build(args, "warm", second, cache, work / "output-second")
        source = second / "tools/bazel/tests/archive_fixture.py"
        original = source.read_text()
        old, new = "SONiC archive reproducibility fixture", "SONiC changed source cache fixture"
        if original.count(old) != 1:
            raise ValueError("fixture payload changed; update this cache regression's mutation")
        source.write_text(original.replace(old, new))
        changed = run_build(args, "changed", second, cache, work / "output-second")
        verify(cold, warm, changed)
        receipt = {
            "target": TARGET, "fresh_checkouts": 2, "fresh_output_bases": 2,
            "fresh_containers": 3 if args.docker_image else 0,
            "cold": cold, "warm": warm, "changed": changed,
        }
        (args.artifacts / "result.json").write_text(json.dumps(receipt, indent=2) + "\n")
        print(json.dumps(receipt, indent=2))
    finally:
        remove_temporary_work(work)


if __name__ == "__main__":
    main()
