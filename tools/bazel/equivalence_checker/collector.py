from collections.abc import Callable
from pathlib import Path

import progress
import registry_lib
from context import Context
from diagnostics import (
    ArtifactIdentifier,
    ArtifactType,
    Codes,
    ComparableArtifact,
    Modifier,
)
from tools import BazelLabel, BazelOutput

# Where Make will write each type of artifact, relative to the root of the repository.
MAKE_DEBS_DIR = "target/debs"
MAKE_IMAGE_DIR = "target"

# slave.mk's DBG_IMAGE_MARK.
DEBUG_MARK = "-dbg"


def _modifiers_for(name: str) -> frozenset[Modifier]:
    """Whether `name` is the debug variant of an artifact, or the shipped one."""
    return frozenset({Modifier.DEBUG if DEBUG_MARK in name else Modifier.RUNTIME})


def _bazel_label_identifier(
    label: BazelLabel, module: str | None = None
) -> ArtifactIdentifier:
    """The identity of the artifact `label` produces, including a module if adequate"""
    return ArtifactIdentifier(
        name=f"@{module}{label}" if module else label,
        source=None,
        modifiers=_modifiers_for(label),
    )


def _record_sources(
    ctx: Context,
    labels: list[BazelLabel],
    excluded: set[BazelLabel],
    identifier_of: Callable[[BazelLabel], ArtifactIdentifier],
    make_dir: Path,
    artifact_type: ArtifactType,
) -> list[ComparableArtifact]:
    """Pair every label in `labels` with the Make artifact of the same filename.

    A source is a top-level thing to compare: a deb, or a container image.

    Args:
        ctx: the run's context.
        labels: the Bazel labels to pair (including those marked `excluded`).
        excluded: `labels` from targets excluded by a tag.
        identifier_of: how to name the artifact a label produces.
        make_dir: where Make writes this kind of artifact.
        artifact_type: what kind of artifact these labels build.

    Returns:
        One ComparableArtifact per label that was not excluded, also added to the index.
    """
    artifacts = []
    for label in labels:
        identifier = identifier_of(label)

        if label in excluded:
            ctx.sink.record(
                identifier,
                Codes.COLLECTION_EXCLUDED_BY_TAG,
                f"tagged {ctx.bazel.EXCLUDE_TAG}",
            )
            continue

        built = ctx.bazel.output_artifact(
            label, BazelOutput.FILE, build=ctx.needs_build
        )
        artifact = ComparableArtifact(
            identifier=identifier,
            bazelVersion=built,
            makeVersion=make_dir / built.name,
            type=artifact_type,
        )
        ctx.index.add(artifact)
        artifacts.append(artifact)

    return artifacts


def _collect_debs(ctx: Context) -> list[ComparableArtifact]:
    """Every deb a top-level Bazel module declares, paired with its Make counterpart.

    Debs pair by filename: `sonic_deb` emits `<package>_<version>_<arch>.deb`, which is
    the Debian convention Make already follows.
    """
    make_dir = registry_lib.REPO_ROOT / MAKE_DEBS_DIR / ctx.debian_release
    repo_names = ctx.bazel.root_repo_names()
    artifacts = []

    for module, _ in registry_lib.discover_top_level_bazel_modules():
        # If a module is not in repo_names, it cannot contribute to any image by definition.
        # So we skip it.
        if module not in repo_names:
            ctx.sink.record(
                _bazel_label_identifier("//...", module),
                Codes.COLLECTION_MODULE_UNREACHABLE,
                f"{module} is not a bazel_dep of the root MODULE.bazel",
            )
            continue

        progress.start(f"LISTING DEBS IN {module}")
        repo_name = repo_names[module]
        repo_prefix = f"@{repo_name}"

        compared, excluded = ctx.bazel.deb_targets(repo_name)
        progress.finish()

        artifacts += _record_sources(
            ctx,
            labels=excluded + compared,
            excluded=set(excluded),
            # Queried from the root, a label names the repo it came from. The
            # identifier spells the module instead, so that it reads the same
            # whichever workspace the query ran in.
            identifier_of=lambda label: _bazel_label_identifier(
                label.removeprefix(repo_prefix), module
            ),
            make_dir=make_dir,
            artifact_type=ArtifactType.DEB,
        )

    return artifacts


def _collect_images(ctx: Context) -> list[ComparableArtifact]:
    """Every oci image the root module declares, paired by name with its Make equivalent."""
    progress.start("LISTING OCI IMAGES")
    compared, excluded = ctx.bazel.image_targets()
    progress.finish()

    return _record_sources(
        ctx,
        labels=excluded + compared,
        excluded=set(excluded),
        identifier_of=_bazel_label_identifier,
        make_dir=registry_lib.REPO_ROOT / MAKE_IMAGE_DIR,
        artifact_type=ArtifactType.OCI_IMAGE,
    )


def collect_artifacts(ctx: Context) -> list[ComparableArtifact]:
    """Run Bazel queries to find out which top-level artifacts we need to compare (debs and OCI images).

    Collects diagnostics in the diagnostics sink.
    Returns a merged list of artifacts, containing deb packages and oci images.
    """
    return _collect_debs(ctx) + _collect_images(ctx)
