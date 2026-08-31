import enum
import json
import os
import shlex
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import TypeAlias

import registry_lib

# A Bazel label, in any of the forms Bazel accepts or prints: `//pkg:target`,
# `@repo//pkg:target`, or `@repo//:target`.
BazelLabel: TypeAlias = str

READELF_LABEL = "@sonic_build_infra//toolchains/binutils:readelf"
ELFCOMPARE_LABEL = "@compare_elf//:elfcompare"


class BazelOutput(enum.Enum):
    """Which of a target's outputs to ask for.

    A target's default output and the executable it runs may not the same thing,
    hence the distinction.
    """

    FILE = ("--output=files",)
    EXECUTABLE = (
        "--output=starlark",
        "--starlark:expr=target.files_to_run.executable.path",
    )


@dataclass(frozen=True)
class Bazel:
    """The `bazel` command, run from the repo root unless told otherwise."""

    EXCLUDE_TAG = "no-elf-equivalence"

    QUERY_FLAGS = (
        "--keep_going",
        "--noshow_progress",
        "--ui_event_filters=-INFO,-WARNING",
    )

    # sonic_deb is a macro, so we match on the kind of the underlying rule.
    DEB_RULE = "_sonic_deb_assemble rule"

    repo_root: Path = registry_lib.REPO_ROOT

    def run(
        self,
        *args: str,
        cwd: Path | None = None,
        ok_statuses: tuple[int, ...] = (0,),
    ) -> subprocess.CompletedProcess:
        """Run one bazel command, in the repo root unless `cwd` says otherwise."""
        result = subprocess.run(
            ["bazel", *args],
            cwd=cwd or self.repo_root,
            capture_output=True,
            text=True,
        )
        if result.returncode not in ok_statuses:
            raise RuntimeError(
                f"bazel {args[0]} failed in {result.args} (exit {result.returncode}):\n"
                f"{result.stderr}"
            )
        return result

    def query(self, expression: str) -> list[str]:
        """Run one `bazel query`, returning the labels it printed."""
        return self._lines(
            self.run("query", *self.QUERY_FLAGS, expression, ok_statuses=(0, 3))
        )

    def _is_convenience_symlink(self, label: BazelLabel) -> bool:
        package = label.partition("//")[2]
        return package.startswith("bazel-")

    def _targets(self, kind: str, scope: str) -> tuple[list[str], list[str]]:
        """Return targets of `kind` under `scope`, partitioned by EXCLUDE_TAG."""
        matching = f'kind("{kind}", {scope})'
        everything = self.query(matching)
        excluded = self.query(f'attr(tags, "{self.EXCLUDE_TAG}", {matching})')
        compared = set(everything) - set(excluded)
        return (
            sorted(label for label in compared if not self._is_convenience_symlink(label)),
            sorted(label for label in excluded if not self._is_convenience_symlink(label)),
        )

    def deb_targets(self, repo_name: str) -> tuple[list[str], list[str]]:
        """The deb targets of one module."""
        return self._targets(self.DEB_RULE, f"@{repo_name}//...")

    def image_targets(self) -> tuple[list[str], list[str]]:
        return self._targets("gzip rule", "//dockers/...")

    def root_repo_names(self) -> dict[str, str]:
        """Map Bazel module name -> the repo name it is visible as from the root workspace.

        The mapping comes from Bazel, so it is what module resolution actually
        produced: repo_name defaults, transitive visibility and all.
        """
        result = self.run("mod", "dump_repo_mapping", "")
        return {
            # A module's canonical repo name is `<module name>+`. Every other shape
            # belongs to a module extension or a repo rule, which has no module.
            #
            # TODO(bazel-ready): We tolerate an instance of canonical repo naming here,
            # because it's more stable than the alternative (parsing MODULE.bazel files directly).
            canonical.removesuffix("+"): apparent
            for apparent, canonical in json.loads(result.stdout).items()
            if canonical.endswith("+") and canonical.count("+") == 1
        }

    def _execution_root(self) -> Path | None:
        """Where Bazel actually writes its outputs, or None if it will not say."""
        try:
            lines = self._lines(self.run("info", "execution_root"))
        except RuntimeError:
            return None
        return Path(lines[0]) if lines else None

    def _resolve(self, path: str) -> Path:
        """The real location of something cquery named."""
        through_symlink = self.repo_root / path
        if through_symlink.exists():
            return through_symlink

        execution_root = self._execution_root()
        if execution_root is not None and (execution_root / path).exists():
            return execution_root / path

        raise RuntimeError(
            f"Tried to resolve path {path}, but couldn't find it under bazel-out or the execution root."
        )

    def output_artifact(
        self, label: BazelLabel, output: BazelOutput, build: bool = True
    ) -> Path:
        """The one path `label` yields as `output`, building it first unless told not to.

        `build=False` locates the path without producing it, for callers that only
        need the name.
        """
        if build:
            self.run("build", "--noshow_progress", label)
        paths = self._lines(self.run("cquery", *self.QUERY_FLAGS, *output.value, label))
        if len(paths) != 1:
            raise RuntimeError(f"{label} has {len(paths)} outputs, expected exactly 1")
        return self._resolve(paths[0])

    @staticmethod
    def _lines(result: subprocess.CompletedProcess) -> list[str]:
        """The non-empty stdout lines of a completed command."""
        return [line.strip() for line in result.stdout.splitlines() if line.strip()]


@dataclass(frozen=True)
class Tool:
    """An external program this script shells out to."""

    name: str
    argv: tuple[str, ...]

    def run(
        self,
        *args: str,
        check: bool = True,
        env: dict[str, str] | None = None,
    ) -> subprocess.CompletedProcess:
        """Run the tool. `check` is off for tools whose exit status is a verdict."""
        return subprocess.run(
            [*self.argv, *args],
            cwd=registry_lib.REPO_ROOT,
            capture_output=True,
            text=True,
            check=check,
            env=os.environ | {"LC_ALL": "C"} | (env or {}),
        )

    @property
    def command_line(self) -> str:
        """The command, quoted for passing through the environment."""
        return shlex.join(self.argv)

    @classmethod
    def from_command(cls, name: str, argv: list[str]) -> "Tool":
        """A tool invoked as `argv`, with its first word resolved through PATH.

        Panics if the tool is not available.
        """
        executable = shutil.which(argv[0])
        if executable is None:
            raise SystemExit(
                f"{name}: '{argv[0]}' is not on PATH. Install it, or put it on "
                "PATH, and re-run."
            )
        return cls(name, (executable, *argv[1:]))

    @classmethod
    def from_bazel(cls, name: str, label: str, bazel: Bazel) -> "Tool":
        """The executable that building `label` produces.

        Because we use some tools with Bazel (e.g. readelf),
        using actual `bazel run` would create a Bazel-in-Bazel problem if elfcompare calls Bazel.
        This would create a deadlock when the second `bazel run` tries to acquire the workspace lock.
        """
        return cls(name, (str(bazel.output_artifact(label, BazelOutput.EXECUTABLE)),))


@dataclass(frozen=True)
class Tools:
    """Every external program the comparison needs."""

    readelf: Tool
    abidiff: Tool
    elfcompare: Tool
    dpkg_deb: Tool

    @classmethod
    def resolve(cls, bazel: Bazel) -> "Tools":
        return cls(
            readelf=Tool.from_bazel("readelf", READELF_LABEL, bazel),
            abidiff=Tool.from_command("abidiff", ["abidiff"]),
            elfcompare=Tool.from_bazel("elfcompare", ELFCOMPARE_LABEL, bazel),
            dpkg_deb=Tool.from_command("dpkg-deb", ["dpkg-deb"]),
        )
