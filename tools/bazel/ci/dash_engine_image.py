#!/usr/bin/env python3
"""Build DASH engine images, then verify installed payloads and incremental outputs."""

import argparse
from functools import partial
import json
import os
from pathlib import Path
import platform
import re
import shlex
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.build_helpers import cache_options
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import bazel_commands, build, cache_reuse, command_log, resolution
from tools.bazel.oci import oci_layout

OWNER = "//platform/vs/docker-dash-engine:"
IMAGES = ("docker-dash-engine", "docker-dash-engine-dbg")
BASE = "//dockers/docker-base-trixie:docker-base-trixie"
OPTIONS = ["--jobs=2", "--local_resources=cpu=2", "--local_resources=memory=8000",
           "--platforms=@sonic_build_infra//platforms:x86_64_trixie",
           "--host_platform=@sonic_build_infra//platforms:x86_64_trixie"]
APT_LOCK = "@sonic_build_infra//apt:apt.lock.json"
TARGETS = {
    **{name + ".gz": OWNER + name + ".gz" for name in IMAGES},
    "runtime-apt.json": OWNER + "runtime_apt_selection",
    "engine.runtime-apt.tar": OWNER + "debian_runtime_payload",
    "debug-apt.json": OWNER + "debug_apt_selection",
    "engine.debug.tar": OWNER + "all_debug_symbols",
    "engine.debug-tools.tar": OWNER + "debug_tools",
    "engine.source.tar": OWNER + "source_runtime",
    "engine.files.tar": OWNER + "files",
    "engine.packages.json": OWNER + "runtime_package_metadata",
    "engine.status": OWNER + "installed_packages",
    "engine-debug.packages.json": OWNER + "debug_package_metadata",
    "engine-debug.status": OWNER + "debug_installed_packages",
    "base.packages.json": "//dockers/docker-base-trixie:runtime_package_metadata",
    "base.source.tar": "//dockers/docker-base-trixie:source_runtime",
    "openssh.source.tar": OWNER + "debug_client",
    "pi.source.tar": "@p4lang_pi//:runtime",
    "bmv2.source.tar": "@p4lang_bmv2//:runtime",
    "nnpy.source.tar": "@nnpy//:runtime",
}


def root_options():
    cache = os.environ.get("SONIC_CONTAINER_BAZEL_CACHE_DIR", os.environ.get("SONIC_SWSS_BAZEL_CACHE_DIR", ""))
    extra = os.environ.get("SONIC_CONTAINER_BAZEL_OPTIONS", os.environ.get("SONIC_SWSS_BAZEL_OPTIONS", ""))
    return OPTIONS[:-2] + cache_options(cache) + shlex.split(extra) + OPTIONS[-2:]


def prepare_manifests(execute, directory, receipt, version):
    # The legacy recipe has a manifest but no image.env producer.
    execute(["make", "-f", "tools/bazel/prepare_manifests.mk",
             "MANIFEST_METADATA=rules/docker-base-trixie.mk platform/vs/docker-dash-engine.mk",
             "BUILD_WITH_BAZEL_WHEN_AVAILABLE=y", "BLDENV=trixie", "CONFIGURED_PLATFORM=vs",
             "CONFIGURED_ARCH=amd64", "INCLUDE_FIPS=y", "INCLUDE_VS_DASH_SAI=y", "ENABLE_ASAN=n",
             "DOCKERS_PATH=dockers", "PLATFORM_PATH=platform/vs", "SONIC_IMAGE_VERSION=" + version, "manifests"],
            directory, receipt, "manifests")



def clean_source(directory):
    check.require(not ROOT.is_relative_to(directory), "artifact directory cannot contain the source checkout")
    if directory.is_relative_to(ROOT):
        relative = directory.relative_to(ROOT)
        check.require(len(relative.parts) > 1 and relative.parts[0] == "artifacts",
                      "in-checkout evidence must use artifacts/<name>")
    subprocess.run(["git", "diff", "--exit-code", "--ignore-submodules=none", "HEAD"], cwd=ROOT, check=True)
    untracked = subprocess.check_output(
        ["git", "ls-files", "--others", "--exclude-standard", "-z"], cwd=ROOT).decode().split("\0")
    # Earlier archive/config-engine CI steps retain their evidence here too.
    # The reserved artifacts tree supplies no source inputs; tracked files and
    # submodules are still covered by the complete HEAD diff above.
    unexpected = [name for name in untracked if name and
                  not (ROOT / name).is_relative_to(ROOT / "artifacts")]
    check.require(not unexpected, "untracked source inputs: " + repr(unexpected))


def source_identity():
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    headers = subprocess.check_output(["git", "cat-file", "-p", "HEAD"], cwd=ROOT, text=True).split("\n\n", 1)[0]
    # The default Actions checkout is shallow: revision traversal hides its
    # parent list, but the immutable commit object retains the actual parents.
    parents = [line.split()[1] for line in headers.splitlines() if line.startswith("parent ")]
    expected_head, expected_base = os.environ.get("CI_HEAD_SHA"), os.environ.get("CI_BASE_SHA")
    if expected_head:
        check.require(expected_head in [revision, *parents], "tested source does not contain the requested PR head")
    if expected_base:
        check.require(expected_base in parents, "tested merge does not contain the requested base")
    return {"revision": revision, "tested_revision_parents": parents}


def build_images(args, directory):
    check.require(platform.machine() == "x86_64" and
                  platform.freedesktop_os_release().get("VERSION_CODENAME") == "trixie",
                  "DASH engine image acceptance requires native AMD64/Trixie")
    receipt = {"status": "building", "architecture": "amd64", "image_version": args.image_version, "commands": [],
               "invocation": {"bazel": args.bazel,
                              "review_registry_revision": os.environ.get("SONIC_BAZEL_REGISTRY_REVISION")}}
    execute = partial(command_log.execute, cwd=ROOT)
    try:
        clean_source(directory)
        receipt.update(source_identity())
        prepare_manifests(execute, directory, receipt, args.image_version)
        layouts = [BASE, *[OWNER + name for name in IMAGES]]
        receipt["action_audit"] = bazel_commands.audit_targets(args.bazel, root_options(), [*TARGETS.values(), *layouts],
            workspace=ROOT, output=directory / "action-audit.json")
        # Materialize layouts and archives together. A later layout build can
        # rebuild source packages after their archives have already been copied.
        build.collect_archives(ROOT, directory, receipt, TARGETS, bazel=args.bazel,
                               options=root_options(), use_execution_root=True,
                               additional_targets=[*layouts, APT_LOCK])
        lock_output = execute([args.bazel, "cquery", *root_options(), "--output=files", APT_LOCK],
                              directory, receipt, "apt-lock.query").strip()
        receipt["apt_lock_sha256"] = check.sha(build.execution_root_from_bep(directory / "build-events.jsonl") / lock_output)
        receipt["layouts"] = {name: execute([args.bazel, "cquery", *root_options(), "--output=files", OWNER + name],
            directory, receipt, name + ".layout").strip() for name in IMAGES}
        receipt["base_layout"] = execute([args.bazel, "cquery", *root_options(), "--output=files", BASE],
            directory, receipt, "base.layout").strip()
        receipt["resolution"] = resolution.collect(ROOT, directory, bazel=[args.bazel])
        clean_source(directory)
        check.require(source_identity()["revision"] == receipt["revision"], "build checkout changed")
        receipt["status"] = "built"
    except Exception as error:
        receipt.update(status="failed", error=str(error))
        raise
    finally:
        (directory / "build-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


def validate_images(directory):
    from tools.bazel.ci.dash_engine_validate import validate
    return validate(directory)


def incremental_actions(execution, events, log, *, no_op=False, evidence=None):
    """Distinguish proven cache materialization from actual image/source work."""
    result = cache_reuse.collect(execution, events, OWNER + IMAGES[0] + ".gz", allow_empty=True,
                                 include_action_summary=no_op)
    result["phase"] = "incremental"
    if evidence is not None:
        evidence.update(result)
        result = evidence
    executed = {}
    for action in result["actions"]:
        if not action["cache_hit"]:
            mnemonic = action["mnemonic"]
            executed[mnemonic] = executed.get(mnemonic, 0) + action["count"]
    check.require(not any(key.startswith(("Cpp", "Rustc", "GoCompile", "GoLink")) for key in executed),
                  "image-only probe unexpectedly compiled source owners: " + repr(executed))
    if no_op:
        check.require(result["executed_count"] == 0, "no-op build executed actions: " + repr(executed))
        # Compare this build's process summary with its validated spawn records.
        # Internal Bazel bookkeeping is absent from the spawn log; every cache
        # hit must agree by runner and count. Persistent action-cache hits are
        # reported separately from processes by Bazel's SpawnStats; bind them
        # to this invocation's BEP statistics instead of counting them as spawns.
        matches = re.findall(r"^INFO: ([0-9]+) process(?:es)?(?:: (.*))?\.$", log.read_text(), re.M)
        check.require(len(matches) == 1, "missing or ambiguous no-op process summary")
        total, details = matches[0]
        counts = {}
        result["process_summary"] = {"total": int(total), "runners": counts}
        for item in details.split(", ") if details else []:
            match = re.fullmatch(r"([0-9]+) (internal|action cache hit|disk cache hit|remote cache hit)", item)
            check.require(match is not None, "no-op process summary contains actual or unknown work")
            count, runner = match.groups()
            check.require(runner not in counts, "duplicate no-op process runner")
            counts[runner] = int(count)
        processes = {name: count for name, count in counts.items() if name != "action cache hit"}
        check.require(sum(processes.values()) == int(total), "no-op process count differs")
        metrics = result["build_action_summary"]
        runners = metrics["runners"]
        check.require(set(runners) <= {"total", "internal", "disk cache hit", "remote cache hit"},
                      "no-op BEP contains actual or unknown work")
        check.require(int(total) == metrics["actions_executed"] == runners["total"] and
                      processes == {name: count for name, count in runners.items() if name != "total"},
                      "no-op process and BEP runner evidence differ")
        # SpawnStats deliberately prints only '0 processes.' for a zero total,
        # even when the persistent action-cache counter is nonzero.
        check.require((int(total) > 0 and counts.get("action cache hit", 0) == metrics["action_cache_hits"]) or
                      (int(total) == 0 and not counts), "no-op process and BEP action-cache evidence differ")
        for runner in ("disk cache hit", "remote cache hit"):
            expected = sum(action["count"] for action in result["actions"] if action["runner"] == runner)
            check.require(counts.get(runner, 0) == expected, "no-op process and cache evidence differ")
    return executed, result


def incremental(args, directory):
    clean_source(directory)
    identity = source_identity()
    baseline = json.loads((directory / "build-receipt.json").read_text())
    check.require(baseline["status"] == "built" and identity["revision"] == baseline["revision"],
                  "incremental checkout differs from built source")
    root = build.execution_root_from_bep(directory / "build-events.jsonl")
    def output(label):
        paths = subprocess.check_output([args.bazel, "cquery", *root_options(), "--output=files", label], cwd=ROOT, text=True).splitlines()
        check.require(len(paths) == 1, "expected one declared output: " + label)
        return root / paths[0]
    archives = {name: output(OWNER + name + ".gz") for name in IMAGES}
    owners = {name: output(TARGETS[name]) for name in ("engine.source.tar", "base.source.tar")}
    base = output(BASE)
    layouts = {name: root / path for name, path in baseline["layouts"].items()}
    targets = [OWNER + name + suffix for name in IMAGES for suffix in ("", ".gz")]
    targets += [BASE, *[TARGETS[name] for name in owners]]
    def snapshot():
        return {"images": {name: oci_layout.validate_layout(path, "linux/amd64").descriptor["digest"] for name, path in layouts.items()},
                "archives": {name: check.sha(path) for name, path in archives.items()},
                "owners": {name: check.sha(path) for name, path in owners.items()},
                "base": oci_layout.validate_layout(base, "linux/amd64").descriptor["digest"]}
    timings, actions, cache_evidence = {}, {}, {}
    def timed_build(name):
        started = time.monotonic()
        execution = directory / ("incremental-" + name + ".execution.json")
        log_path = directory / ("incremental-" + name + ".log")
        events = directory / ("incremental-" + name + ".events.jsonl")
        with log_path.open("w") as log:
            result = subprocess.run([args.bazel, "build", *root_options(),
                "--execution_log_json_file=" + str(execution),
                "--build_event_json_file=" + str(events),
                *targets], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
        timings[name] = round(time.monotonic() - started, 3)
        result.check_returncode()
        cache_evidence[name] = {}
        actions[name], _ = incremental_actions(execution, events, log_path, no_op=name == "no-op",
                                               evidence=cache_evidence[name])
    evidence = {"status": "running", "timings_seconds": timings, "executed_actions": actions,
                "spawn_cache_evidence": cache_evidence}
    try:
        timed_build("baseline")
        original = snapshot()
        evidence["baseline"] = original
        check.require(all(original["archives"][name] == baseline["artifacts"][name + ".gz"]["sha256"] for name in IMAGES),
                      "incremental baseline differs from verified archives")
        timed_build("no-op")
        unchanged = snapshot()
        evidence["unchanged"] = unchanged
        check.require(unchanged == original, "no-op build changed installed source/image bytes")
    except Exception as error:
        evidence.update(status="failed", error=str(error))
        (directory / "incremental.json").write_text(json.dumps(evidence, indent=2) + "\n")
        raise
    source = ROOT / "platform/vs/docker-dash-engine/start.sh"
    saved, mode = source.read_bytes(), source.stat().st_mode
    evidence["input"] = str(source.relative_to(ROOT))
    try:
        source.write_bytes(saved + b"\n# Bazel incremental source input probe\n")
        timed_build("launcher-source-edit")
        changed = snapshot()
        evidence["changed"] = changed
        check.require(all(changed[field][name] != original[field][name] for field in ("images", "archives") for name in IMAGES),
                      "startup edit did not invalidate both images and archives")
        check.require(changed["owners"] == original["owners"] and changed["base"] == original["base"],
                      "startup edit changed an unrelated source owner or base")
        evidence["status"] = "passed"
    except Exception as error:
        evidence.update(status="failed", error=str(error))
        raise
    finally:
        source.write_bytes(saved)
        source.chmod(mode)
        try:
            timed_build("restored")
            restored = snapshot()
            evidence["restored"] = restored
            check.require(restored == original, "restoring startup did not reproduce exact archive bytes")
            clean_source(directory)
            check.require(source_identity() == identity, "incremental checkout changed")
        except Exception as error:
            evidence.update(status="failed", restoration_error=str(error))
            raise
        finally:
            (directory / "incremental.json").write_text(json.dumps(evidence, indent=2) + "\n")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phase", choices=("build", "validate", "incremental"))
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--image-version", default="bazel-dash-engine-ci")
    parser.add_argument("--bazel", default="tools/bazel/ci/libteam_draft_bazel.sh")
    args = parser.parse_args()
    directory = args.artifacts.resolve(); directory.mkdir(parents=True, exist_ok=True)
    if args.phase == "build": build_images(args, directory)
    elif args.phase == "validate": validate_images(directory)
    else: incremental(args, directory)


if __name__ == "__main__":
    main()
