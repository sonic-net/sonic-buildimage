#!/usr/bin/env python3
"""Probe an installed SWSS source edit through the consuming OCI image."""

import argparse
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci.artifact_validation import require, sha
from tools.bazel.submodule_provenance import git, gitlinks, verify

SOURCE_LABEL = "@sonic_swss//fpmsyncd:bgp_eoiu_marker.py"
SOURCE_PATH = Path("fpmsyncd/bgp_eoiu_marker.py")
SHEBANG = b"#!/usr/bin/env python\n"
PROBE = b"# SWSS source-TAR invalidation probe; removed after this check.\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bazel", required=True)
    parser.add_argument("--bazel-arg", action="append", default=[])
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--runtime-target", required=True)
    parser.add_argument("--symbols-target", required=True)
    parser.add_argument("--additional-runtime-target", action="append", default=[])
    parser.add_argument("--unchanged-target", action="append", default=[])
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    job_temp = os.environ.get("SONIC_BAZEL_JOB_TEMP")
    scratch = os.environ.get("RUNNER_TEMP")
    if job_temp:
        scratch = Path(job_temp) / "archive-bazel"
        scratch.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix="swss-source-probe.", dir=scratch)).resolve()
    workspace = scratch / "workspace"
    output_base = scratch / "bazel"
    bazel = [args.bazel, "--output_base=" + str(output_base)]
    try:
        private_owner, revision = checkout_probe_workspace(ROOT, workspace)
    except BaseException:
        shutil.rmtree(scratch)
        raise
    try:
        probe_image(args, bazel, output_base, workspace, private_owner, revision)
    finally:
        # Preserve the base for post-job cleanup if Bazel cannot confirm shutdown.
        subprocess.run([*bazel, "shutdown"], cwd=workspace, check=True)
        shutil.rmtree(scratch)


def clone_revision(source, destination, revision):
    git(source, "clone", "--quiet", "--shared", "--no-checkout", str(source), str(destination))
    git(destination, "checkout", "--quiet", "--detach", revision)


def checkout_probe_workspace(source, workspace):
    """Keep image provenance tied to its gitlink while editing a disposable SWSS checkout."""
    records = gitlinks(source)
    require("sonic-swss" in records, "buildimage must pin SWSS with a gitlink")
    clone_revision(source, workspace, git(source, "rev-parse", "HEAD"))
    swss_path, _, swss_revision = records["sonic-swss"]
    for path, _, revision in records.values():
        verify(source, path, revision)
    declarations = git(source, "config", "--file", ".gitmodules", "--get-regexp", r"^submodule\..*\.path$")
    for entry in declarations.splitlines():
        path = entry.split(maxsplit=1)[1]
        owner, destination = source / path, workspace / path
        if path == swss_path:
            clone_revision(owner, destination, swss_revision)
        elif (owner / ".git").exists():
            # Also preserve initialized nested inputs of in-tree owners, such as sysmgr/gnoi.
            destination.rmdir()
            destination.symlink_to(owner, target_is_directory=True)
    if (source / ".bazelrc.user").is_file():
        (workspace / ".bazelrc.user").symlink_to(source / ".bazelrc.user")
    # The preceding image build generated its Make labels outside Git.
    manifests = Path("target/bazel-manifests")
    if (source / manifests).is_dir():
        shutil.copytree(source / manifests, workspace / manifests)
    return workspace / swss_path, swss_revision


def probe_image(args, bazel, output_base, workspace, private_owner, revision):
    options = [*args.bazel_arg, "--repo_contents_cache=", "--symlink_prefix=/"]
    result = subprocess.run([*bazel, "info", *options, "output_base"],
                            capture_output=True, text=True, cwd=workspace, check=True)
    require(result.stdout.splitlines() == [str(output_base)], "Bazel did not select the private output base")
    # Resolve the disposable checkout through the same source label as the image.
    subprocess.run([*bazel, "build", *options, SOURCE_LABEL], cwd=workspace, check=True)
    execution_root = output_base / "execroot" / "_main"

    def query(target):
        result = subprocess.run([*bazel, "cquery", *options, target, "--output=files"],
                                capture_output=True, text=True, cwd=workspace, check=True)
        paths = [execution_root / line for line in result.stdout.splitlines() if line.strip()]
        require(len(paths) == 1 and paths[0].is_file(), "missing SWSS source or archive: " + target)
        return paths[0]

    source = query(SOURCE_LABEL)
    require(source.is_relative_to(execution_root), "SWSS source is outside the selected execution root")
    relative = source.relative_to(execution_root)
    require(len(relative.parts) == 4 and relative.parts[0] == "external" and
            relative.parts[1] not in (".", "..") and Path(*relative.parts[2:]) == SOURCE_PATH,
            "SWSS source is not the selected external owner file")
    owner = (execution_root / "external" / relative.parts[1]).resolve(strict=True)
    require(owner == private_owner and not source.is_symlink() and
            source.parent.resolve(strict=True) == owner / SOURCE_PATH.parent and source.stat().st_nlink == 1,
            "SWSS source escapes its selected owner")
    original = source.read_bytes()
    original_mode = stat.S_IMODE(source.stat().st_mode)
    require(original.startswith(SHEBANG) and PROBE not in original, "unexpected or already edited SWSS source")
    runtime_targets = [args.runtime_target, *args.additional_runtime_target]
    unchanged_targets = [args.symbols_target, *args.unchanged_target]
    targets = runtime_targets + unchanged_targets
    records = []
    status = "failed"

    def build(phase):
        start = time.monotonic()
        subprocess.run([*bazel, "build", *options, "--build_event_json_file=" + str(args.output / (phase + "-events.json")), *targets], cwd=workspace, check=True)
        digests = {}
        for target in targets:
            digests[target] = sha(query(target))
        records.append({"phase": phase, "seconds": round(time.monotonic() - start, 3),
                        "outputs_sha256": digests, "source_sha256": sha(source)})
        return digests

    try:
        baseline = build("baseline")
        require(build("no-op") == baseline, "unchanged inputs changed a SWSS TAR")
        source.write_bytes(SHEBANG + PROBE + original[len(SHEBANG):])
        changed = build("source-edit")
        require(all(changed[target] != baseline[target] for target in runtime_targets) and
                all(changed[target] == baseline[target] for target in unchanged_targets),
                "an installed Python edit must change its runtime outputs, preserving compiled symbols and bases")
        source.write_bytes(original)
        require(build("restored") == baseline, "restored inputs did not reproduce the SWSS outputs")
        status = "passed"
    finally:
        require(not source.is_symlink() and source.parent.resolve(strict=True) == owner / SOURCE_PATH.parent,
                "SWSS source path changed during the probe")
        source.write_bytes(original)
        source.chmod(original_mode)
        (args.output / "rebuild.json").write_text(json.dumps({
            "status": status, "source_label": SOURCE_LABEL, "source_path": str(SOURCE_PATH),
            "source_revision": revision,
            "scope": "Same-output-base no-op and installed-source invalidation, not a cold-cache or C++ recompilation claim",
            "source_restored": source.read_bytes() == original and stat.S_IMODE(source.stat().st_mode) == original_mode,
            "builds": records,
        }, indent=2) + "\n")


if __name__ == "__main__":
    main()
