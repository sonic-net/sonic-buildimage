#!/usr/bin/env python3
"""Select verified kernel artifacts and a fixed public validation summary."""

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import public_artifacts
from tools.bazel.kernel import prepare as kernel


def collect(workspace, state, output, architecture="amd64", debs_directory=None):
    kernel.require(not output.exists(), "kernel artifact destination already exists")
    packages = Path(debs_directory or kernel.DEBS_DIRECTORY)
    if not packages.is_absolute():
        packages = workspace / packages
    verified = kernel.verify(packages, workspace, architecture=architecture)
    selected = verified["provenance"]["resolution"]
    records = {}
    for name, path, hash_field in (
        ("MODULE.bazel.lock", state / "consumer/MODULE.bazel.lock", "lock_sha256"),
        ("module-graph.json", state / "build/module-graph.json", "graph_sha256"),
        ("kernel-source.json", state / "build/kernel-source.json", "kernel_source_json_sha256"),
        ("infra-source.json", state / "build/infra-source.json", "infra_source_json_sha256"),
    ):
        data = kernel.regular(path).read_bytes()
        kernel.require(hashlib.sha256(data).hexdigest() == selected[hash_field],
                       "kernel dependency record differs from verified provenance")
        records[name] = hashlib.sha256(data).hexdigest()
    artifacts = []
    for item in verified["manifest"]["packages"]:
        artifacts.append({key: item[key] for key in ("name", "sha256", "size")})
    summary = {
        "schema": 1,
        "architecture": architecture,
        "source_commit": verified["provenance"]["source_commit"],
        "kernel_gitlink": verified["provenance"]["kernel_gitlink"],
        "target": kernel.TARGET,
        "artifact_count": len(artifacts),
        "artifacts": sorted(artifacts, key=lambda item: item["name"]),
        "manifest_sha256": kernel.sha256(packages / kernel.MANIFEST),
        "provenance_sha256": kernel.sha256(packages / kernel.PROVENANCE),
        "resolution_sha256": records,
    }
    public_artifacts.safe_json(summary)
    # Only values constrained by kernel.verify's exact package/source contract
    # enter the summary. Complete metadata and raw diagnostic files are not
    # uploaded; CI command diagnostics are visible in the Actions console.
    output.mkdir(parents=True)
    try:
        for item in artifacts:
            destination = output / item["name"]
            shutil.copyfile(kernel.regular(packages / item["name"]), destination)
            kernel.require(destination.stat().st_size == item["size"]
                           and kernel.sha256(destination) == item["sha256"],
                           "kernel artifact changed during artifact collection")
        (output / "summary.json").write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n")
    except Exception:
        shutil.rmtree(output)
        raise
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workspace", type=Path, default=Path.cwd())
    parser.add_argument("--state-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--debs-directory", type=Path)
    parser.add_argument("--architecture", choices=("amd64", "arm64"), default=kernel.native_architecture())
    args = parser.parse_args()
    collect(args.workspace.resolve(strict=True), args.state_dir.resolve(strict=True), args.output,
            args.architecture, args.debs_directory)
    print("Kernel artifacts and public validation summary verified.")


if __name__ == "__main__":
    main()
