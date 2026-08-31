from dataclasses import dataclass, field
import enum
from pathlib import Path
from typing import TypeAlias

import registry_lib


ArtifactName: TypeAlias = str


class ArtifactType(enum.StrEnum):
    DEB = "DEB"
    OCI_IMAGE = "OCI_IMAGE"
    ELF_EXECUTABLE = "ELF_EXECUTABLE"
    # The .debug file carrying a binary's symbols.
    # They are ELFs themselves, but it's a useful differentiation for reporting.
    ELF_DEBUG_INFO = "ELF_DEBUG_INFO"
    FILE = "FILE"
    DIRECTORY = "DIRECTORY"
    LINK = "LINK"


class Modifier(enum.StrEnum):
    """Distinguishers for artifacts that share a name."""

    # The binary as shipped.
    RUNTIME = "runtime"
    # The debug information belonging to that binary.
    DEBUG = "debug"


@dataclass(frozen=True)
class ArtifactIdentifier:
    name: ArtifactName
    source: "ArtifactIdentifier | None"
    modifiers: frozenset[Modifier]

    def __str__(self) -> str:
        text = self.name
        if self.modifiers:
            text += f" ({', '.join(sorted(self.modifiers))})"
        if self.source is not None:
            text += f" in {self.source}"
        return text


@dataclass()
class ComparableArtifact:
    identifier: ArtifactIdentifier
    bazelVersion: Path
    makeVersion: Path
    type: ArtifactType

    @staticmethod
    def _relative(path: Path) -> str:
        """`path` against the repo root, or as it stands when it lies outside."""
        try:
            return str(path.relative_to(registry_lib.REPO_ROOT))
        except ValueError:
            return str(path)

    @property
    def printable(self) -> str:
        """This artifact and both of its paths, as three lines for a terminal."""
        return f"""{self.type} {self.identifier}
    bazel: {self._relative(self.bazelVersion)}
    make:  {self._relative(self.makeVersion)}"""


@dataclass(frozen=True)
class ArtifactIndex:
    """Every artifact to compare, keyed by identity.

    Insertion fails if element is already there, so an identifier names exactly one artifact.
    """

    artifacts: dict[ArtifactIdentifier, ComparableArtifact] = field(
        default_factory=dict
    )

    def add(self, artifact: ComparableArtifact) -> None:
        existing = self.artifacts.get(artifact.identifier)
        if existing is not None:
            raise ValueError(
                f"{artifact.identifier} already names {existing.bazelVersion}, "
                f"so it cannot also name {artifact.bazelVersion}"
            )
        self.artifacts[artifact.identifier] = artifact

    def __getitem__(self, identifier: ArtifactIdentifier) -> ComparableArtifact:
        return self.artifacts[identifier]

    def __len__(self) -> int:
        return len(self.artifacts)


class Codes(enum.StrEnum):
    """Every way the two builds can differ, and every reason a pair went uncompared."""

    # Public findings from compareELF.
    ELFCOMPARE_ABI = "abi"
    ELFCOMPARE_DEPENDENCY = "dependency"
    ELFCOMPARE_ELF = "elf"
    ELFCOMPARE_EXPORT_ADDED = "export-added"
    ELFCOMPARE_EXPORT_CHANGED = "export-changed"
    ELFCOMPARE_EXPORT_REMOVED = "export-removed"
    ELFCOMPARE_FUNCTION_ADDED = "function-added"
    ELFCOMPARE_FUNCTION_REMOVED = "function-removed"
    ELFCOMPARE_IMPORT_ADDED = "import-added"
    ELFCOMPARE_IMPORT_CHANGED = "import-changed"
    ELFCOMPARE_IMPORT_REMOVED = "import-removed"
    ELFCOMPARE_RUNTIME = "runtime"
    ELFCOMPARE_RUNTIME_VERSION = "runtime-version"
    ELFCOMPARE_SECURITY = "security"
    ELFCOMPARE_STARTUP_CALLBACK = "startup-callback"

    # Outcomes of a whole ELF comparison.
    ELFCOMPARE_DIFFERENT_STRIP_LEVELS = "DIFFERENT_STRIP_LEVELS"
    ELFCOMPARE_INCOMPLETE = "INCOMPLETE"
    ELFCOMPARE_UNPARSEABLE = "UNPARSEABLE"
    ELFCOMPARE_ERROR = "ERROR"

    # Things that go wrong while working out what there is to compare.
    COLLECTION_EXCLUDED_BY_TAG = "EXCLUDED_BY_TAG"
    COLLECTION_MODULE_UNREACHABLE = "MODULE_UNREACHABLE"
    COLLECTION_NO_MAKE_ARTIFACT = "NO_MAKE_ARTIFACT"
    COLLECTION_NO_BAZEL_ARTIFACT = "NO_BAZEL_ARTIFACT"

    # Things that go wrong while unpacking an artifact.
    EXTRACTION_MAKE_ONLY = "MAKE_ONLY"
    EXTRACTION_BAZEL_ONLY = "BAZEL_ONLY"
    EXTRACTION_UNREADABLE = "UNREADABLE"
    EXTRACTION_NO_DEBUG = "NO_DEBUG"

    # How a pair that is not an ELF came out.
    FILE_CONTENT_MISMATCH = "CONTENT_MISMATCH"
    FILE_TARGET_MISMATCH = "TARGET_MISMATCH"


@dataclass(frozen=True)
class Diagnostic:
    artifact: ArtifactIdentifier
    code: Codes
    msg: str


@dataclass(frozen=True)
class DiagnosticSink:
    diagnostics: list[Diagnostic] = field(default_factory=list)

    def record(self, artifact: ArtifactIdentifier, code: Codes, msg: str) -> None:
        """Note one way the two builds differ, or one reason a pair went uncompared."""
        self.diagnostics.append(Diagnostic(artifact, code, msg))

    def absorb(self, other: "DiagnosticSink") -> None:
        """Take everything `other` collected.

        Comparisons run in a pool, each into a sink of its own, and the results are merged here.
        """
        self.diagnostics.extend(other.diagnostics)
