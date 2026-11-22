#!/usr/bin/env python3
"""Select verified kernel artifacts and a fixed public validation summary."""

import argparse
import hashlib
import json
import re
from pathlib import Path
import shutil
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import public_artifacts
from tools.bazel.kernel import packages as root_packages, prepare as kernel


VS_RECEIPT = "vs-kernel-handoff.json"


def checked_metadata(packages, output, name, digest, fields):
    path = output / name
    shutil.copyfile(kernel.regular(packages / name), path)
    kernel.require(kernel.sha256(path) == digest, "kernel metadata changed during collection")
    metadata = public_artifacts.load_json(path.read_text())
    kernel.require(set(metadata) == fields, "unexpected kernel metadata fields in " + name)
    public_artifacts.safe_json(metadata)


def verify_retained_resolution(workspace, packages, records):
    directory = packages / kernel.RESOLUTION_DIRECTORY
    kernel.require(not directory.is_symlink(), "redirected kernel resolution directory")
    restored = workspace / kernel.remote_bundle.RESTORED_INDEX
    if not restored.exists() and not restored.is_symlink():
        for digest in records.values():
            kernel.require(kernel.sha256(kernel.regular(directory / digest)) == digest,
                           "kernel dependency record differs from verified provenance")
        return

    # On a fresh remote hit, the master's original provenance binds the exact
    # dependency evidence. The restore already checked the lock against today's
    # registry and verified every DEB before Make installed them.
    index = public_artifacts.load_json(kernel.regular(restored).read_text())
    trusted = workspace / kernel.remote_bundle.INPUT_INDEX
    kernel.require(index == public_artifacts.load_json(kernel.regular(trusted).read_text()),
                   "kernel restore differs from the trusted master index")
    for name in (kernel.MANIFEST, kernel.PROVENANCE):
        path = kernel.regular(packages / name)
        kernel.require(index["metadata"][name] == {"size": path.stat().st_size, "sha256": kernel.sha256(path)},
                       "kernel handoff differs from the verified master index")


def verified_summary(workspace, state, packages, verified, *, vs_handoff=False,
                     from_vs_handoff=False, kernel_config=None, receipt_sha256=None):
    architecture = verified["manifest"]["architecture"]
    provenance = verified["provenance"]
    root = provenance["schema"] == 3
    selected = provenance.get("resolution", {})
    if root:
        mapping = kernel.ROOT_RESOLUTION_RECORDS if selected else {}
        kernel.require(not (vs_handoff or from_vs_handoff) or selected, "VS kernel is missing root resolution evidence")
        kernel.require(set(selected) in (set(), {"lock_sha256"}, {"lock_sha256", "root_request_key"})
                       and all(isinstance(value, str) and re.fullmatch(r"[0-9a-f]{64}", value)
                               for value in selected.values()), "invalid root kernel resolution evidence")
        kernel.require(provenance["launcher"] == {"architecture": architecture}
                       and re.fullmatch(r"[0-9a-f]{40}", provenance["source_commit"]),
                       "invalid root kernel publication identity")
        if vs_handoff or from_vs_handoff:
            expected, debs = kernel.contract(workspace, architecture, kernel_config)
            root_packages.verify_make_contract(packages, verified, expected, debs)
    else:
        mapping = kernel.RESOLUTION_RECORDS
    records = {Path(relative).name: selected[field] for relative, field in mapping.items()}
    if vs_handoff or (root and records and not from_vs_handoff):
        verify_retained_resolution(workspace, packages, records)
    elif not root and not from_vs_handoff:
        kernel.require(state is not None, "legacy kernel collection requires its producer state")
        for relative, field in mapping.items():
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
    receipt = None
    if vs_handoff or from_vs_handoff:
        # Make can legitimately retain packages compiled at an earlier commit.
        # Bind their verified package set to the checkout that produced the VS installer.
        receipt = {"schema": 1, "vs_revision": kernel.git(workspace, "rev-parse", "HEAD"), "summary": summary}
        if from_vs_handoff:
            data = kernel.regular(packages / VS_RECEIPT).read_bytes()
            if receipt_sha256 is not None:
                kernel.require(hashlib.sha256(data).hexdigest() == receipt_sha256,
                               "VS kernel handoff receipt digest differs")
            supplied = public_artifacts.load_json(data)
            kernel.require(supplied == receipt, "kernel handoff differs from this VS revision or its verified inputs")
    return summary, receipt


def verify_make_handoff(workspace, packages, architecture, kernel_config, receipt_sha256):
    """Consume an immutable same-run VS artifact using the digest pinned by CI."""
    kernel.require(not kernel.git(workspace, "diff", "--no-ext-diff", "--name-only",
                                  "--ignore-submodules=none", "HEAD", "--"),
                   "VS kernel handoff requires the original checkout")
    expected, debs = kernel.contract(workspace, architecture, kernel_config)
    for name in (*debs, kernel.MANIFEST, kernel.PROVENANCE):
        kernel.regular(packages / name)
    verified = root_packages.verify(packages, workspace, architecture, expected["kernel_config"])
    verified_summary(workspace, None, packages, verified, from_vs_handoff=True,
                     kernel_config=kernel_config, receipt_sha256=receipt_sha256)


def collect(workspace, state, output, architecture=None, debs_directory=None,
            *, vs_handoff=False, from_vs_handoff=False):
    kernel.require(not output.exists(), "kernel artifact destination already exists")
    packages = Path(debs_directory or kernel.DEBS_DIRECTORY)
    if not packages.is_absolute():
        packages = workspace / packages
    verified = kernel.verify(packages, workspace, architecture=architecture)
    summary, receipt = verified_summary(workspace, state, packages, verified,
                                       vs_handoff=vs_handoff, from_vs_handoff=from_vs_handoff)
    artifacts = summary["artifacts"]
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
            config, _ = kernel.contract(workspace, summary["architecture"])
            checked_metadata(packages, output, kernel.MANIFEST, summary["manifest_sha256"], set(config) | {
                "source_date_epoch", "source_files", "source_archives", "source_tree_sha256",
                "build_tools", "packages", "created_packages", "debug",
            })
            fields = {"schema", "source_commit", "kernel_gitlink", "launcher", "manifest_sha256", "target", "resolution"}
            if verified["provenance"]["schema"] == 3:
                fields.add("module_version")
            else:
                fields.update({"consumer_inputs_sha256", "modules", "kernel_config"})
            checked_metadata(packages, output, kernel.PROVENANCE, summary["provenance_sha256"], fields)
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
    evidence = parser.add_mutually_exclusive_group()
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
