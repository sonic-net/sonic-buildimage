import argparse
import json
import os
import sys
import tempfile
from pathlib import Path

import registry_lib
import existing_oci
import subprocess
import rules
import rules_engine
from collector import collect_artifacts
from comparator import compare_artifacts
from context import Context
from diagnostics import ArtifactIndex, ComparableArtifact, DiagnosticSink
from extractor import extract_all
from reporter import print_report, write_report
from tools import Bazel, Tools


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    _ = parser.add_argument(
        "--bldenv",
        default="trixie",
        help="Debian release whose Make debs to compare against (slave.mk DEBS_PATH).",
    )
    _ = parser.add_argument(
        "--print-sources",
        action="store_true",
        help="Print the Make artifacts this comparison needs, and exit.",
    )
    _ = parser.add_argument(
        "--print-all-artifacts",
        action="store_true",
        help="Print the artifacts this comparison will check, and exit.",
    )
    _ = parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Assume the Bazel artifacts are already built (they are still located with cquery).",
    )
    _ = parser.add_argument(
        "--print-make-paths",
        action="store_true",
        help=(
            "Print the Make artifacts this comparison needs, one relative path per line."
        ),
    )
    _ = parser.add_argument(
        "--jobs",
        type=int,
        default=os.cpu_count(),
        help=(
            "How many artifacts to compare at once. Each comparison may run multiple processes (e.g. readelf, abidiff...)"
        ),
    )
    _ = parser.add_argument(
        "--report",
        type=Path,
        default=Path("target/equivalence-report.json"),
        help="Where to write the run report, relative to the repo root.",
    )
    _ = parser.add_argument("--existing-oci", type=Path, help="Compare only sealed existing OCI pairs; no builds, queries or package provisioning.")
    _ = parser.add_argument("--source-checkout", type=Path, help="Clean producer source checkout for existing artifacts; may differ from the checker checkout.")
    _ = parser.add_argument("--build-all-pairs", action="store_true", help="Explicitly allow the legacy Make/Bazel/DEB comparison route and tool builds.")
    args = parser.parse_args()

    if args.existing_oci is not None:
        if args.print_sources or args.print_make_paths or args.print_all_artifacts or args.skip_build or args.build_all_pairs:
            parser.error("--existing-oci is a separate comparison-only entry")
        if args.source_checkout is None:
            parser.error("--existing-oci requires --source-checkout")
        return run_existing(args)
    if not args.build_all_pairs or args.source_checkout is not None:
        parser.error("use --existing-oci with --source-checkout, or explicitly opt into --build-all-pairs")

    build = not (args.skip_build or args.print_sources or args.print_make_paths)

    bazel = Bazel()
    with tempfile.TemporaryDirectory(prefix="equivalence-") as tmp:
        ctx = Context(
            sink=DiagnosticSink(),
            index=ArtifactIndex(),
            bazel=bazel,
            tools=Tools.resolve(bazel),
            needs_build=build,
            debian_release=args.bldenv,
            jobs=args.jobs,
            workdir=Path(tmp),
        )
        return _run(ctx, args)



def checkout_revision(path: Path) -> str:
    """Bind a clean repository root without changing its revision or contents."""
    root = subprocess.run(["git", "rev-parse", "--show-toplevel"], cwd=path, capture_output=True, text=True, check=True).stdout.strip()
    if Path(root).resolve() != path.resolve():
        raise ValueError("source/tool path must be a repository root")
    dirty = subprocess.run(["git", "status", "--porcelain"], cwd=path, capture_output=True, text=True, check=True)
    if dirty.stdout:
        raise ValueError("existing-artifact comparison requires a clean checkout")
    return subprocess.run(["git", "rev-parse", "HEAD"], cwd=path, capture_output=True, text=True, check=True).stdout.strip()


def run_existing(args) -> int:
    """Use the original extractor/comparator with a validated OCI-only manifest."""
    source_checkout = args.source_checkout.resolve()
    revision = checkout_revision(source_checkout)
    tool_revision = checkout_revision(registry_lib.REPO_ROOT)
    report = registry_lib.REPO_ROOT / args.report
    sources, required, tools, manifest_sha256 = existing_oci.load(
        args.existing_oci.absolute(), revision, report=report
    )
    with tempfile.TemporaryDirectory(prefix="equivalence-") as tmp:
        ctx = Context(sink=DiagnosticSink(), index=ArtifactIndex(), bazel=None, tools=tools, needs_build=False, debian_release=args.bldenv, jobs=args.jobs, workdir=Path(tmp))
        for artifact in sources:
            ctx.index.add(artifact)
        compared = extract_all(ctx, sources, scoped_oci=True)
        compare_artifacts(ctx, compared, scoped_oci=True)
        classified = rules_engine.classify(ctx.sink.diagnostics, rules.RULES)
        write_report(ctx, classified, report)
        result = existing_oci.scoped_result(classified, required, {a.identifier.name for a in sources})
        payload = json.loads(report.read_text())
        payload["scope"] = result
        payload["provenance_manifest"] = {"path": str(args.existing_oci.absolute()), "sha256": manifest_sha256}
        payload["source_checkout"] = {"path": str(source_checkout), "revision": revision, "clean": True}
        payload["tool_checkout"] = {"path": str(registry_lib.REPO_ROOT), "revision": tool_revision, "clean": True}
        report.write_text(json.dumps(payload, indent=2) + "\n")
        print_report(ctx, classified)
        print(f"Required OCI pairs: {result['required_status']}; all compared pairs: {result['all_compared_status']}")
        return 0 if result["required_status"] == "PASS" else 1


def _relative(path: Path) -> Path:
    """Try to resolve `path` against the repo root, but return it as-is if it falls outside it."""
    try:
        return path.relative_to(registry_lib.REPO_ROOT)
    except ValueError:
        return path


def _run(ctx: Context, args: argparse.Namespace) -> int:
    source_artifacts = collect_artifacts(ctx)
    if not source_artifacts:
        print("No comparable source_artifacts found.")
        return 1

    if args.print_make_paths:
        for artifact in source_artifacts:
            print(artifact.makeVersion.relative_to(registry_lib.REPO_ROOT))
        return 0

    if args.print_sources:
        for artifact in source_artifacts:
            print(artifact.printable)
        return 0

    artifacts_to_compare = extract_all(ctx, source_artifacts)

    if args.print_all_artifacts:
        for artifact in artifacts_to_compare:
            print(artifact.printable)
        return 0

    # Compare each artifact, looking at its id, and dispatching the appropriate comparison for each type.
    compare_artifacts(ctx, artifacts_to_compare)

    classified = rules_engine.classify(ctx.sink.diagnostics, rules.RULES)
    print_report(ctx, classified)

    report = registry_lib.REPO_ROOT / args.report
    write_report(ctx, classified, report)
    print(f"\nReport: {_relative(report)}")

    return 1 if rules_engine.unaccepted(classified) else 0


if __name__ == "__main__":
    sys.exit(main())
