#!/usr/bin/env python3

from __future__ import annotations

import argparse
import fnmatch
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Iterator

DEFAULT_ALLOWLIST = ".github/binary-artifacts-allowlist.txt"

# Bytes inspected at the start of every blob (matches git's binary heuristic).
HEAD_BYTES = 8000

MODE_SYMLINK = "120000"
MODE_SUBMODULE = "160000"

EXECUTABLE_KIND = "executable, library or object code"
ARCHIVE_KIND = "archive or package"
DATA_KIND = "binary data"
EXTENSION_KIND = "build artifact"

SIGNATURES = (
    (b"\x7fELF", EXECUTABLE_KIND),
    (b"\xfe\xed\xfa\xce", EXECUTABLE_KIND),
    (b"\xfe\xed\xfa\xcf", EXECUTABLE_KIND),
    (b"\xce\xfa\xed\xfe", EXECUTABLE_KIND),
    (b"\xcf\xfa\xed\xfe", EXECUTABLE_KIND),
    (b"!<arch>\n", EXECUTABLE_KIND),
    (b"\x1f\x8b", ARCHIVE_KIND),
    (b"BZh", ARCHIVE_KIND),
    (b"\xfd7zXZ\x00", ARCHIVE_KIND),
    (b"\x28\xb5\x2f\xfd", ARCHIVE_KIND),
    (b"PK\x03\x04", ARCHIVE_KIND),
    (b"PK\x05\x06", ARCHIVE_KIND),
    (b"7z\xbc\xaf\x27\x1c", ARCHIVE_KIND),
    (b"Rar!\x1a\x07", ARCHIVE_KIND),
    (b"hsqs", ARCHIVE_KIND),
    (b"\xed\xab\xee\xdb", ARCHIVE_KIND),
)

TAR_MAGIC_OFFSET = 257
TAR_MAGIC = b"ustar"

BINARY_EXTENSIONS = (
    ".a",
    ".bz2",
    ".class",
    ".deb",
    ".dll",
    ".dylib",
    ".exe",
    ".gz",
    ".img",
    ".iso",
    ".jar",
    ".ko",
    ".o",
    ".pyc",
    ".pyo",
    ".so",
    ".tar",
    ".tgz",
    ".whl",
    ".xz",
    ".zip",
)


@dataclass(frozen=True)
class Change:
    status: str
    path: str
    old_sha: str
    new_sha: str
    new_mode: str


@dataclass(frozen=True)
class Finding:
    path: str
    kind: str


def git(*args: str) -> bytes:
    return subprocess.run(
        ["git", *args],
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    ).stdout


def parse_raw_diff(raw: bytes) -> Iterator[Change]:
    """Parse the output of `git diff --raw -z`."""
    tokens = raw.decode("utf-8", errors="surrogateescape").split("\0")
    index = 0

    while index < len(tokens) and tokens[index]:
        # :<old mode> <new mode> <old sha> <new sha> <status>
        fields = tokens[index].lstrip(":").split(" ")
        _old_mode, new_mode, old_sha, new_sha, status = fields[:5]
        index += 1

        # Renames and copies carry the old and the new path.
        if status[0] in "RC":
            index += 1

        yield Change(status[0], tokens[index], old_sha, new_sha, new_mode)
        index += 1


def get_changes(base: str, head: str) -> list[Change]:
    raw = git(
        "diff",
        "--raw",
        "-z",
        "--no-abbrev",
        "-M",
        "--diff-filter=ACMR",
        f"{base}...{head}",
    )
    return list(parse_raw_diff(raw))


class BlobReader:
    """Read the leading bytes of git blobs through one `git cat-file` process."""

    def __init__(self) -> None:
        self._process = subprocess.Popen(
            ["git", "cat-file", "--batch"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
        )

    def head(self, sha: str) -> bytes:
        assert self._process.stdin and self._process.stdout

        self._process.stdin.write(sha.encode("ascii") + b"\n")
        self._process.stdin.flush()

        header = self._process.stdout.readline().split()
        if len(header) != 3:
            raise RuntimeError(f"cannot read blob {sha}")

        size = int(header[2])
        data = self._process.stdout.read(size)
        self._process.stdout.read(1)

        return data[:HEAD_BYTES]

    def close(self) -> None:
        if self._process.stdin:
            self._process.stdin.close()
        self._process.wait()
        if self._process.stdout:
            self._process.stdout.close()

    def __enter__(self) -> "BlobReader":
        return self

    def __exit__(self, *_exc: object) -> None:
        self.close()


def classify(path: str, content: bytes) -> str | None:
    """Return a description when the file looks binary, otherwise None."""
    for signature, kind in SIGNATURES:
        if content.startswith(signature):
            return kind

    if (
        content[TAR_MAGIC_OFFSET:TAR_MAGIC_OFFSET + len(TAR_MAGIC)]
        == TAR_MAGIC
    ):
        return ARCHIVE_KIND

    if b"\0" in content:
        if content.startswith(b"MZ"):
            return EXECUTABLE_KIND
        return DATA_KIND

    if path.lower().endswith(BINARY_EXTENSIONS):
        return EXTENSION_KIND

    return None


def load_allowlist(path: Path) -> list[str]:
    if not path.is_file():
        return []

    patterns = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            patterns.append(line)

    return patterns


def is_allowed(path: str, patterns: Iterable[str]) -> bool:
    return any(fnmatch.fnmatchcase(path, pattern) for pattern in patterns)


def find_binary_artifacts(
    changes: Iterable[Change],
    reader: BlobReader,
    allowlist: list[str],
) -> list[Finding]:
    findings = []

    for change in changes:
        if change.new_mode in (MODE_SYMLINK, MODE_SUBMODULE):
            continue

        # A rename that keeps the content is not a new binary.
        if change.status == "R" and change.old_sha == change.new_sha:
            continue

        if is_allowed(change.path, allowlist):
            continue

        kind = classify(change.path, reader.head(change.new_sha))
        if kind is None:
            continue

        # Already binary before this changeset: it is not introduced here.
        if change.status in "MR" and (
            classify(change.path, reader.head(change.old_sha)) is not None
        ):
            continue

        findings.append(Finding(change.path, kind))

    return findings


def escape_data(value: str) -> str:
    return (
        value.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
    )


def escape_property(value: str) -> str:
    return escape_data(value).replace(":", "%3A").replace(",", "%2C")


def report(
    findings: list[Finding],
    allowlist_path: str,
    level: str,
) -> None:
    for finding in findings:
        message = (
            f"Binary artifact detected ({finding.kind}). Binary files should "
            "be built from source or fetched at build time instead of being "
            "committed. If this file must be committed, add it to "
            f"{allowlist_path} in this pull request and explain why."
        )
        print(
            f"::{level} file={escape_property(finding.path)},"
            f"title=Binary artifact::{escape_data(message)}"
        )

    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    if not summary_path:
        return

    lines = ["### Binary artifacts detected", "", "| File | Type |", "|---|---|"]
    lines += [f"| `{item.path}` | {item.kind} |" for item in findings]
    with open(summary_path, "a", encoding="utf-8") as summary:
        summary.write("\n".join(lines) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Report binary artifacts introduced by a changeset."
    )
    parser.add_argument("--base", required=True)
    parser.add_argument("--head", default="HEAD")
    parser.add_argument("--allowlist", default=DEFAULT_ALLOWLIST)
    parser.add_argument(
        "--warn-only",
        action="store_true",
        help="report findings as warnings and exit successfully",
    )

    args = parser.parse_args()

    try:
        changes = get_changes(args.base, args.head)
        allowlist = load_allowlist(Path(args.allowlist))

        with BlobReader() as reader:
            findings = find_binary_artifacts(changes, reader, allowlist)
    except (subprocess.CalledProcessError, RuntimeError) as error:
        detail = getattr(error, "stderr", b"") or b""
        print(f"::error::Unable to inspect the changeset: {error} "
              f"{detail.decode(errors='replace').strip()}")
        return 2

    print(f"Checked {len(changes)} changed file(s) against "
          f"{len(allowlist)} allowlist pattern(s).")

    if not findings:
        print("No binary artifacts detected.")
        return 0

    report(findings, args.allowlist, "warning" if args.warn_only else "error")
    print(f"{len(findings)} binary artifact(s) detected.")

    return 0 if args.warn_only else 1


if __name__ == "__main__":
    sys.exit(main())
