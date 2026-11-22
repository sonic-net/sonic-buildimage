#!/usr/bin/env python3
"""Select verified kernel artifacts and a fixed public validation summary."""

import argparse
import json
from pathlib import Path
import shutil
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import public_artifacts
from tools.bazel.kernel import prepare as kernel


VS_RECEIPT = "vs-kernel-handoff.json"


def checked_metadata(packages, output, name, digest, fields):
    path = output / name
    shutil.copyfile(kernel.regular(packages / name), path)
    kernel.require(kernel.sha256(path) == digest, "kernel metadata changed during collection")
    metadata = public_artifacts.load_json(path.read_text())
    kernel.require(set(metadata) == fields, "unexpected kernel metadata fields in " + name)
    public_artifacts.safe_json(metadata)


def verify_vs_resolution(workspace, packages, selected):
    directory = packages / kernel.RESOLUTION_DIRECTORY
    kernel.require(not directory.is_symlink(), "redirected kernel resolution directory")
    restored = workspace / kernel.remote_bundle.RESTORED_INDEX
    missing = False
    for field in kernel.RESOLUTION_RECORDS.values():
        record = directory / selected[field]
        if restored.is_file() and not record.exists() and not record.is_symlink():
            missing = True
            continue
        kernel.require(kernel.sha256(kernel.regular(record)) == record.name,
                       "kernel dependency record differs from verified provenance")
    if not missing:
        return

    # On a fresh remote hit, the master's original provenance binds all four
    # resolution hashes. The restore already checked the lock against today's
    # registry and verified every DEB before Make installed them.
    index = public_artifacts.load_json(kernel.regular(restored).read_text())
    trusted = workspace / kernel.remote_bundle.INPUT_INDEX
    kernel.require(index == public_artifacts.load_json(kernel.regular(trusted).read_text()),
                   "kernel restore differs from the trusted master index")
    for name in (kernel.MANIFEST, kernel.PROVENANCE):
        path = kernel.regular(packages / name)
        kernel.require(index["metadata"][name] == {"size": path.stat().st_size, "sha256": kernel.sha256(path)},
                       "kernel handoff differs from the verified master index")


def collect(workspace, state, output, architecture=None, debs_directory=None,
            *, vs_handoff=False, from_vs_handoff=False):
    kernel.require(not output.exists(), "kernel artifact destination already exists")
    packages = Path(debs_directory or kernel.DEBS_DIRECTORY)
    if not packages.is_absolute():
        packages = workspace / packages
    verified = kernel.verify(packages, workspace, architecture=architecture)
    architecture = verified["manifest"]["architecture"]
    selected = verified["provenance"]["resolution"]
    records = {Path(relative).name: selected[field] for relative, field in kernel.RESOLUTION_RECORDS.items()}
    if vs_handoff:
        verify_vs_resolution(workspace, packages, selected)
    elif not from_vs_handoff:
        for relative, field in kernel.RESOLUTION_RECORDS.items():
            kernel.require(kernel.sha256(kernel.regular(state / relative)) == selected[field],
                           "kernel dependency record differs from verified provenance")
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
    if vs_handoff or from_vs_handoff:
        # Make can legitimately retain packages compiled at an earlier commit.
        # Bind their verified package set to the checkout that produced the VS installer.
        receipt = {"schema": 1, "vs_revision": kernel.git(workspace, "rev-parse", "HEAD"), "summary": summary}
        if from_vs_handoff:
            supplied = public_artifacts.load_json(kernel.regular(packages / VS_RECEIPT).read_text())
            kernel.require(supplied == receipt, "kernel handoff differs from this VS revision or its verified inputs")
    # Publish only verified package bytes and the metadata needed for this handoff.
    output.mkdir(parents=True)
    try:
        for item in artifacts:
            destination = output / item["name"]
            shutil.copyfile(kernel.regular(packages / item["name"]), destination)
            kernel.require(destination.stat().st_size == item["size"]
                           and kernel.sha256(destination) == item["sha256"],
                           "kernel artifact changed during artifact collection")
        if vs_handoff:
            # Make's handoff requires the original manifest and provenance bytes.
            # The raw resolution lock and graph can contain private diagnostics.
            config, _ = kernel.contract(workspace, architecture)
            checked_metadata(packages, output, kernel.MANIFEST, summary["manifest_sha256"], set(config) | {
                "source_date_epoch", "source_files", "source_archives", "source_tree_sha256",
                "build_tools", "packages", "created_packages", "debug",
            })
            checked_metadata(packages, output, kernel.PROVENANCE, summary["provenance_sha256"], {
                "schema", "source_commit", "consumer_inputs_sha256", "kernel_gitlink", "modules",
                "launcher", "kernel_config", "manifest_sha256", "target", "resolution",
            })
            (output / VS_RECEIPT).write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
        else:
            (output / "summary.json").write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n")
    except Exception:
        shutil.rmtree(output)
        raise
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workspace", type=Path, default=Path.cwd())
    evidence = parser.add_mutually_exclusive_group(required=True)
    evidence.add_argument("--state-dir", type=Path, help="check the original native build evidence")
    evidence.add_argument("--vs-handoff", action="store_true", help="export Make's verified VS kernel package inputs")
    evidence.add_argument("--from-vs-handoff", action="store_true", help="verify the same-run VS kernel inputs")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--debs-directory", type=Path)
    parser.add_argument("--architecture", choices=("amd64", "arm64"), default=kernel.native_architecture())
    args = parser.parse_args()
    collect(args.workspace.resolve(strict=True), args.state_dir, args.output, args.architecture,
            args.debs_directory, vs_handoff=args.vs_handoff, from_vs_handoff=args.from_vs_handoff)
    print("Kernel artifacts and public validation summary verified.")


if __name__ == "__main__":
    main()
