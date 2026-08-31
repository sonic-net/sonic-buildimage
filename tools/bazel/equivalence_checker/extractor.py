"""Unpacks a top-level artifact (OCI image or deb archive) into the individual files worth comparing."""

import gzip
import json
import re
import shutil
import tarfile
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path, PurePosixPath

import progress
from context import Context
from diagnostics import (
    ArtifactIdentifier,
    ArtifactType,
    Codes,
    ComparableArtifact,
    Codes,
    Modifier,
)
from tools import Tool


def _is_elf(path: Path) -> bool:
    """True if `path` starts with the ELF magic.

    Cheaper than running `file`.
    """
    try:
        with path.open("rb") as handle:
            return handle.read(4) == b"\x7fELF"
    except OSError:
        # An unreadable file is not something we can compare either way.
        return False


# Debug files live at /usr/lib/debug/.build-id/NN/REST.debug.
DEBUG_PREFIX = "/usr/lib/debug/.build-id/"

# What is worth pairing between the two sides.
COMPARABLE_TYPES = (
    ArtifactType.ELF_EXECUTABLE,
    ArtifactType.FILE,
    ArtifactType.LINK,
)


BUILD_ID_RE = re.compile(r"Build ID:\s*([0-9a-f]+)")


def _build_id_of_debug_file(install_path: str) -> str:
    """The build-id a debug file's own path encodes.

    Debug files are content-addressed: /usr/lib/debug/.build-id/93/e50c...debug
    belongs to the binary whose build-id is 93e50c...
    """
    return install_path[len(DEBUG_PREFIX) :].removesuffix(".debug").replace("/", "")


def _build_id_of_binary(path: Path, readelf: Tool) -> str | None:
    """The GNU build-id recorded in `path`, or None if it carries none."""
    match = BUILD_ID_RE.search(readelf.run("-n", str(path)).stdout)
    return match.group(1) if match else None


def _entry_identifier(
    install_path: str, source: ArtifactIdentifier
) -> ArtifactIdentifier:
    """The identity of one entry, as found inside `source`."""
    return ArtifactIdentifier(
        name=install_path,
        source=source,
        modifiers=frozenset({Modifier.RUNTIME}),
    )


@dataclass(frozen=True)
class UnpackedTree:
    """What one side unpacked.

    For each ArtifactType, store a map of install path -> real path.
    """

    entries: dict[ArtifactType, dict[str, Path]] = field(
        default_factory=lambda: defaultdict(dict)
    )

    def of(self, kind: ArtifactType) -> dict[str, Path]:
        """The entries of one kind, keyed by install path."""
        return self.entries[kind]

    def all_entries(self) -> set[str]:
        return {path for bucket in self.entries.values() for path in bucket}


@dataclass(frozen=True)
class DebugFiles:
    """Every .debug file each side ships, keyed by the build-id that owns it.

    Debug files are named after build-ids, which diverge from build to build.
    This mapping allows us to tie debug files back to the binaries they originate from.
    """

    make: dict[str, Path] = field(default_factory=dict)
    bazel: dict[str, Path] = field(default_factory=dict)


def _index_tree(root: Path) -> UnpackedTree:
    """Walk `root` once, bucketing every entry by what kind of artifact it is."""

    def _type_of(path: Path) -> ArtifactType | None:
        if path.is_symlink():
            return ArtifactType.LINK
        if path.is_dir():
            return ArtifactType.DIRECTORY
        if not path.is_file():
            # A device, fifo or socket has no contents to compare.
            return None
        return ArtifactType.ELF_EXECUTABLE if _is_elf(path) else ArtifactType.FILE

    tree = UnpackedTree()
    for path in sorted(root.rglob("*")):
        kind = _type_of(path)
        if kind is not None:
            tree.entries[kind]["/" + str(path.relative_to(root))] = path
    return tree


def _harvest_debug(tree: UnpackedTree, into: dict[str, Path]) -> None:
    """Move `tree`'s debug files into the build-id index.

    Note that this modifies the UnpackedTree.
    """
    elfs = tree.of(ArtifactType.ELF_EXECUTABLE)
    for install_path in [p for p in elfs if p.startswith(DEBUG_PREFIX)]:
        into[_build_id_of_debug_file(install_path)] = elfs.pop(install_path)


def _pair(
    ctx: Context,
    source: ComparableArtifact,
    make: UnpackedTree,
    bazel: UnpackedTree,
    debug: DebugFiles,
    identical: frozenset[str] = frozenset(),
) -> list[ComparableArtifact]:
    """Pair what the two sides unpacked, by kind and install path.

    Debug files are paired by build-id rather than by path.

    Make and bazel entries that are known to be identical (e.g. because they come from identical layers)
    are skipped.

    If we can't find something to pair against, we report it and move on.
    """
    _harvest_debug(make, debug.make)
    _harvest_debug(bazel, debug.bazel)

    paired = []
    for kind in COMPARABLE_TYPES:
        make_entries = {
            path: real for path, real in make.of(kind).items() if path not in identical
        }
        bazel_entries = {
            path: real for path, real in bazel.of(kind).items() if path not in identical
        }

        for code, install_paths, detail in (
            (
                Codes.EXTRACTION_MAKE_ONLY,
                make_entries.keys() - bazel_entries.keys(),
                "Bazel ships no such entry",
            ),
            (
                Codes.EXTRACTION_BAZEL_ONLY,
                bazel_entries.keys() - make_entries.keys(),
                "Make ships no such entry",
            ),
        ):
            for install_path in sorted(install_paths):
                ctx.sink.record(
                    _entry_identifier(install_path, source.identifier), code, detail
                )

        for install_path in sorted(make_entries.keys() & bazel_entries.keys()):
            extracted = ComparableArtifact(
                identifier=_entry_identifier(install_path, source.identifier),
                bazelVersion=bazel_entries[install_path],
                makeVersion=make_entries[install_path],
                type=kind,
            )
            ctx.index.add(extracted)
            paired.append(extracted)

    return paired


def _debug_for(binary: Path, side: dict[str, Path], readelf: Tool) -> Path | None:
    """The debug file holding `binary`'s symbols, or None if that side ships none."""
    build_id = _build_id_of_binary(binary, readelf)
    return side.get(build_id) if build_id else None


def pair_debug_info(
    ctx: Context, artifacts: list[ComparableArtifact], debug: DebugFiles
) -> list[ComparableArtifact]:
    """Pair the debug information belonging to each runtime binary, by build-id.

    The pair takes the name of the binary that owns it, because the debug files
    themselves are named after their own contents and so never agree across builds.
    """
    paired = []
    for artifact in artifacts:
        if artifact.type is not ArtifactType.ELF_EXECUTABLE:
            continue

        make_debug = _debug_for(artifact.makeVersion, debug.make, ctx.tools.readelf)
        bazel_debug = _debug_for(artifact.bazelVersion, debug.bazel, ctx.tools.readelf)
        if make_debug is None and bazel_debug is None:
            continue

        identifier = ArtifactIdentifier(
            name=artifact.identifier.name,
            source=artifact.identifier.source,
            modifiers=frozenset({Modifier.DEBUG}),
        )

        if make_debug is None or bazel_debug is None:
            missing = "Make" if make_debug is None else "Bazel"
            ctx.sink.record(
                identifier,
                Codes.EXTRACTION_NO_DEBUG,
                f"{missing} ships no debug information for this binary",
            )
            continue

        extracted = ComparableArtifact(
            identifier=identifier,
            bazelVersion=bazel_debug,
            makeVersion=make_debug,
            type=ArtifactType.ELF_DEBUG_INFO,
        )
        ctx.index.add(extracted)
        paired.append(extracted)

    return paired


def _roots_for(ctx: Context, artifact: ComparableArtifact) -> tuple[Path, Path]:
    """Where each side of `artifact` should unpack its contents to, as (make, bazel)."""
    # A label is not a file name: `/` is the one character a component cannot hold.
    base = ctx.workdir / artifact.type / artifact.identifier.name.replace("/", "_")
    base.mkdir(parents=True)
    return base / "make", base / "bazel"


def _extract_deb(
    ctx: Context, artifact: ComparableArtifact, debug: DebugFiles
) -> list[ComparableArtifact]:
    """Unpack both sides of a deb and pair what is inside them."""
    make_root, bazel_root = _roots_for(ctx, artifact)
    for root, deb in (
        (make_root, artifact.makeVersion),
        (bazel_root, artifact.bazelVersion),
    ):
        root.mkdir(parents=True, exist_ok=True)
        ctx.tools.dpkg_deb.run("-x", str(deb), str(root))

    return _pair(ctx, artifact, _index_tree(make_root), _index_tree(bazel_root), debug)


# A layer entry whose basename carries this prefix deletes what the rest of it names.
WHITEOUT_PREFIX = ".wh."

# The one whiteout that names no file: it hides everything already in its directory.
OPAQUE_MARKER = ".wh..wh..opq"


def _remove(path: Path) -> None:
    """Remove `path`, whatever kind of thing it is.

    A whiteout for something no lower layer shipped is legal, and removes nothing.
    """
    if path.is_symlink() or path.is_file():
        path.unlink(missing_ok=True)
    elif path.is_dir():
        shutil.rmtree(path)


def _apply_whiteout(root: Path, name: str) -> None:
    """Delete from `root` whatever the whiteout entry `name` removes."""
    entry = PurePosixPath(name.removeprefix("./"))
    directory = root / entry.parent

    if entry.name == OPAQUE_MARKER:
        if directory.is_dir():
            for child in directory.iterdir():
                _remove(child)
        return

    _remove(directory / entry.name.removeprefix(WHITEOUT_PREFIX))


def _writable_dirs(member: tarfile.TarInfo, dest: str) -> tarfile.TarInfo | None:
    """Take every member as it comes, but never leave a directory we cannot write into.

    We can't use the stock `tar_filter`, because images ship absolute symlinks.
    Because we control the build outputs, we can allow absolute symlinks.
    """
    if member.isdir():
        member.mode |= 0o700
    return member


def _install_path_of(member: tarfile.TarInfo) -> str:
    """The install path a layer member lands at, spelled as `_index_tree` spells it."""
    return "/" + member.name.removeprefix("./").rstrip("/")


def _unpack_docker_archive(archive: Path, root: Path) -> dict[str, str]:
    """Flatten a gzipped docker-archive into the filesystem it describes.

    Both Bazel and Make ship the same format.

    Returns the layer each path finally came from. Layers are content-addressed, so
    a path both sides took from the same layer holds the same bytes on both.
    """
    root.mkdir(parents=True, exist_ok=True)
    blobs = root.parent / f"{root.name}.blobs"
    blobs.mkdir(parents=True, exist_ok=True)

    # The whole archive lands first because manifest.json is written last, and a
    # stream cannot be rewound to reach the layers it names.
    with gzip.open(archive, "rb") as compressed:
        with tarfile.open(fileobj=compressed, mode="r|") as outer:
            outer.extractall(blobs, filter=_writable_dirs)

    manifest = json.loads((blobs / "manifest.json").read_text())
    came_from: dict[str, str] = {}
    for layer in manifest[0]["Layers"]:
        with tarfile.open(blobs / layer) as tar:
            members = tar.getmembers()
            whiteouts = {
                member.name
                for member in members
                if Path(member.name).name.startswith(WHITEOUT_PREFIX)
            }
            # Deletions land before entries
            for name in sorted(whiteouts):
                _apply_whiteout(root, name)

            landing = [m for m in members if m.name not in whiteouts]
            tar.extractall(root, members=landing, filter=_writable_dirs)
            # This loop may intentionally cause overwrites, if a file overwrites another previously unpacked file.
            for member in landing:
                came_from[_install_path_of(member)] = layer

    shutil.rmtree(blobs)
    return came_from


def _extract_oci_image(
    ctx: Context, artifact: ComparableArtifact, debug: DebugFiles
) -> list[ComparableArtifact]:
    """Unpack both sides of a container archive and pair what is inside them.

    Most of an image comes from layers both builds share, and those are settled
    before any comparison starts.
    """
    make_root, bazel_root = _roots_for(ctx, artifact)
    make_layers = _unpack_docker_archive(artifact.makeVersion, make_root)
    bazel_layers = _unpack_docker_archive(artifact.bazelVersion, bazel_root)

    make_tree, bazel_tree = _index_tree(make_root), _index_tree(bazel_root)
    make_present, bazel_present = make_tree.all_entries(), bazel_tree.all_entries()

    # Create a set of "artifacts that come from identical layers".
    # Note that just because two images share a base layer, it doesn't mean they'll result in identical artifacts.
    # A later layer may overwrite the base layer file.
    identical = frozenset(
        path
        for path, layer in make_layers.items()
        if path in make_present
        and path in bazel_present
        and bazel_layers.get(path) == layer
    )

    return _pair(ctx, artifact, make_tree, bazel_tree, debug, identical)


def extract_source(
    ctx: Context, artifact: ComparableArtifact, debug: DebugFiles
) -> list[ComparableArtifact]:
    """Unpack one top-level artifact into the files to compare inside it.

    A side that is not on disk is reported and skipped.
    """
    for code, path in (
        (Codes.COLLECTION_NO_MAKE_ARTIFACT, artifact.makeVersion),
        (Codes.COLLECTION_NO_BAZEL_ARTIFACT, artifact.bazelVersion),
    ):
        if not path.exists():
            ctx.sink.record(artifact.identifier, code, f"{path} does not exist")
            return []

    match artifact.type:
        case ArtifactType.DEB:
            return _extract_deb(ctx, artifact, debug)
        case ArtifactType.OCI_IMAGE:
            return _extract_oci_image(ctx, artifact, debug)
        case _:
            raise ValueError(f"nothing knows how to unpack a {artifact.type}")


def extract_all(
    ctx: Context, sources: list[ComparableArtifact]
) -> list[ComparableArtifact]:
    """Unpack every source artifact into the entries to compare inside them."""
    debug = DebugFiles()

    extracted = []
    for source in sources:
        progress.start(f"EXTRACTING {source.identifier}")
        extracted += extract_source(ctx, source, debug)
        progress.finish()

    # Collect the debug info in a second pass, because we need all the binaries in scope
    # to be able to tie them to their debug information.
    progress.start("MATCHING DEBUG INFORMATION")
    with_debug = extracted + pair_debug_info(ctx, extracted, debug)
    progress.finish()
    return with_debug
