import argparse
import os
import sys
import tempfile
from pathlib import Path

import registry_lib
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
    args = parser.parse_args()

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
