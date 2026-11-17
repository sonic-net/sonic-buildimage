from dataclasses import dataclass
from pathlib import Path

from diagnostics import ArtifactIndex, DiagnosticSink
from tools import Bazel, Tools


@dataclass(frozen=True)
class Context:
    sink: DiagnosticSink
    index: ArtifactIndex
    bazel: Bazel
    tools: Tools
    needs_build: bool
    debian_release: str
    # How many comparisons may run at once.
    jobs: int
    workdir: Path
