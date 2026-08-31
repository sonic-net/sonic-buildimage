"""Compares the two sides of every paired artifact, recording what differs."""

import dataclasses
import filecmp
import json
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import progress
from context import Context
from diagnostics import (
    ArtifactType,
    ComparableArtifact,
    DiagnosticSink,
    Codes,
)
from tools import Tool


# elfcompare's exit statuses.
EXIT_CLEAN = 0
EXIT_DIFFERENT = 1
EXIT_UNPARSEABLE = 2
EXIT_INCOMPLETE = 3

# elfcompare status that don't carry metadata, so they map clearly to diagnostic codes.
ELFCOMPARE_STATUS_CODES = {
    EXIT_UNPARSEABLE: (
        Codes.ELFCOMPARE_UNPARSEABLE,
        "elfcompare could not read an artifact",
    ),
    EXIT_INCOMPLETE: (
        Codes.ELFCOMPARE_INCOMPLETE,
        "part of the analysis did not run",
    ),
}


def _has_symtab(elf: Path, readelf: Tool) -> bool:
    """True if `elf` still carries .symtab.

    Useful to detect drift in stripping options.
    """
    return " .symtab" in readelf.run("-S", str(elf)).stdout


def _code_of(finding: dict) -> Codes:
    """Mapping from elfcompare finding code to a diagnostic code."""
    try:
        return Codes(finding.get("category"))
    except ValueError:
        return Codes.ELFCOMPARE_ERROR


def _compare_elf(ctx: Context, artifact: ComparableArtifact) -> None:
    """Compare one ELF pair with elfcompare.

    Every finding elfcompare reports becomes its own diagnostic.
    """
    tools = ctx.tools
    identifier = artifact.identifier

    # If the files are identical, just move on.
    if filecmp.cmp(artifact.makeVersion, artifact.bazelVersion, shallow=False):
        return

    # With .symtab on one side only there is no function inventory to compare,
    # so elfcompare's answer would not mean anything.
    if _has_symtab(artifact.makeVersion, tools.readelf) != _has_symtab(
        artifact.bazelVersion, tools.readelf
    ):
        ctx.sink.record(
            identifier,
            Codes.ELFCOMPARE_DIFFERENT_STRIP_LEVELS,
            "only one side retains .symtab, which usually means the two builds strip with different flags",
        )
        return

    result = tools.elfcompare.run(
        str(artifact.makeVersion),
        str(artifact.bazelVersion),
        check=False,
        env={
            "ELFCOMPARE_READELF": tools.readelf.command_line,
            "ELFCOMPARE_ABIDIFF": tools.abidiff.command_line,
        },
    )

    if result.returncode == EXIT_CLEAN:
        return

    if result.returncode == EXIT_UNPARSEABLE:
        code, detail = ELFCOMPARE_STATUS_CODES[EXIT_UNPARSEABLE]
        ctx.sink.record(identifier, code, detail)
        return

    if result.returncode not in (EXIT_DIFFERENT, EXIT_INCOMPLETE):
        ctx.sink.record(
            identifier,
            Codes.ELFCOMPARE_ERROR,
            f"unexpected elfcompare exit status {result.returncode}",
        )
        return

    try:
        findings = json.loads(result.stdout).get("findings", [])
    except json.JSONDecodeError:
        ctx.sink.record(
            identifier,
            Codes.ELFCOMPARE_ERROR,
            "elfcompare produced unreadable JSON",
        )
        return

    if result.returncode == EXIT_INCOMPLETE:
        code, detail = ELFCOMPARE_STATUS_CODES[EXIT_INCOMPLETE]
        ctx.sink.record(identifier, code, detail)

    if not findings:
        if result.returncode == EXIT_DIFFERENT:
            ctx.sink.record(
                identifier,
                Codes.ELFCOMPARE_ERROR,
                "elfcompare reported a difference with no findings",
            )
        return

    for finding in findings:
        ctx.sink.record(identifier, _code_of(finding), json.dumps(finding))


def _compare_file(ctx: Context, artifact: ComparableArtifact) -> None:
    """Compare one non-ELF file pair byte for byte."""
    if filecmp.cmp(artifact.makeVersion, artifact.bazelVersion, shallow=False):
        return

    ctx.sink.record(
        artifact.identifier,
        Codes.FILE_CONTENT_MISMATCH,
        f"make is {artifact.makeVersion.stat().st_size} bytes, "
        f"bazel is {artifact.bazelVersion.stat().st_size}",
    )


def _compare_link(ctx: Context, artifact: ComparableArtifact) -> None:
    """Compare where two symlinks point, without looking at the files the symlinks point to."""
    make_target = artifact.makeVersion.readlink()
    bazel_target = artifact.bazelVersion.readlink()
    if make_target == bazel_target:
        return

    ctx.sink.record(
        artifact.identifier,
        Codes.FILE_TARGET_MISMATCH,
        f"make points at {make_target}, bazel points at {bazel_target}",
    )


def _compare_one(ctx: Context, artifact: ComparableArtifact) -> DiagnosticSink:
    """Compare one artifact, into a sink of its own."""
    own = dataclasses.replace(ctx, sink=DiagnosticSink())
    match artifact.type:
        case ArtifactType.ELF_EXECUTABLE | ArtifactType.ELF_DEBUG_INFO:
            _compare_elf(own, artifact)
        case ArtifactType.FILE:
            _compare_file(own, artifact)
        case ArtifactType.LINK:
            _compare_link(own, artifact)
        case _:
            raise ValueError(f"nothing knows how to compare a {artifact.type}")
    return own.sink


def compare_artifacts(ctx: Context, artifacts: list[ComparableArtifact]) -> None:
    """Compare every paired artifact, recording differences in the sink."""
    with ThreadPoolExecutor(max_workers=ctx.jobs) as pool:
        sinks = pool.map(lambda artifact: _compare_one(ctx, artifact), artifacts)
        for artifact, sink in zip(artifacts, sinks):
            progress.step(f"COMPARED {artifact.identifier}")
            ctx.sink.absorb(sink)
