#!/usr/bin/env python3
"""Publish git-submodule-backed Bazel modules to the remote sonic-bazel-registry.

Scans src/ for MODULE.bazel files belonging to git submodules,
and publishes any not-yet-published (module, version+commit) pairs to
https://github.com/blorente/sonic-bazel-registry as a PR.

TODO: Migrate to sonic-net when we have a repository available.

Usage:
    python3 tools/bazel/registry/publish_to_remote_registry.py
    python3 tools/bazel/registry/publish_to_remote_registry.py <path>  # publish a single module, e.g. src/libnl3

Or, via Bazel (note the `--` separating Bazel's own flags from this script's):
    bazel run //tools/bazel/registry:publish_to_remote_registry
    bazel run //tools/bazel/registry:publish_to_remote_registry -- <path>
"""

import base64
import difflib
import hashlib
import json
import os
import re
import requests
import shutil
import subprocess
import tempfile
from collections.abc import Callable
from dataclasses import dataclass
from datetime import datetime
from functools import partial
from pathlib import Path

import click

from registry_lib import (
    REPO_ROOT,
    extract_repo_rule_call,
    extract_single_item_list_kwarg,
    extract_str_kwarg,
    is_git_submodule,
    load_submodule_paths,
    load_submodule_urls,
    parse_module_declaration,
    rewrite_module_version,
    submodule_root_for,
)

REGISTRY_REPO_URL = "https://github.com/blorente/sonic-bazel-registry"


def sha256_hex_to_integrity(sha256_hex: str) -> str:
    """Convert a plain hex sha256 digest to Bazel's "sha256-<base64>" integrity format."""
    return "sha256-" + base64.b64encode(bytes.fromhex(sha256_hex)).decode("ascii")


@dataclass(frozen=True)
class PublishCandidate:
    """A module found by one of the three discover_*_modules(), normalized so
    main() only needs one skip-if-published/write/report loop. write(registry_dir)
    does all its own (possibly expensive, e.g. a network fetch) work lazily,
    only once actually called."""

    name: str
    version: str
    # REPO_ROOT-relative directory this candidate is read from.
    # This path is required to be clean before publishing.
    path: str
    write: Callable[[Path], None]


@dataclass(frozen=True)
class RuleSource:
    """A resolved upstream archive: url/strip_prefix/integrity, ready to write
    into a registry entry's source.json."""

    url: str
    strip_prefix: str
    integrity: str


def repo_rule_source(wrapper_dir: str, repo_rule_name: str) -> RuleSource:
    """Resolve a RuleSource by parsing a `<repo_rule_name>(...)` call out of
    <wrapper_dir>/MODULE.bazel (e.g. libnl3's `libnl3_src(...)`)."""
    module_bazel_text = (REPO_ROOT / wrapper_dir / "MODULE.bazel").read_text()
    repo_rule_call = extract_repo_rule_call(module_bazel_text, repo_rule_name)
    return RuleSource(
        url=extract_single_item_list_kwarg(repo_rule_call, "urls"),
        strip_prefix=extract_str_kwarg(repo_rule_call, "strip_prefix"),
        integrity=sha256_hex_to_integrity(extract_str_kwarg(repo_rule_call, "sha256")),
    )


def archive_source(url: str, sha256: str, strip_prefix: str) -> RuleSource:
    """A hand-specified RuleSource: url/sha256/strip_prefix given directly,
    rather than parsed or resolved from anything else."""
    return RuleSource(url=url, strip_prefix=strip_prefix, integrity=sha256_hex_to_integrity(sha256))


@dataclass(frozen=True)
class OverlayModule:
    name: str
    version: str
    wrapper_dir: str
    source: RuleSource
    overlay_files: list[str]


# Patched external dependencies, where we fetch the source from somewhere else,
# patch it, and overlay a Bazel build on top.
#
# We assume these change rarely, and hence are okay with hardcoding values like the version.
OVERLAY_MODULES = [
    OverlayModule(
        name="libnl3",
        version="3.7.0.sonic-buildimage",
        wrapper_dir="src/libnl3",
        source=repo_rule_source(wrapper_dir="src/libnl3", repo_rule_name="libnl3_src"),
        overlay_files=[
            "MODULE.bazel",
            "BUILD.bazel",
            "libnl3_src.bzl",
            "libnl3.BUILD",
            "patch/0003-Adding-support-for-RTA_NH_ID-attribute.patch",
        ],
    ),
    OverlayModule(
        name="com_github_openconfig_gnoi",
        version="0.6.1.sonic-buildimage",
        wrapper_dir="src/sonic-sysmgr/gnoi_overlay",
        # gnoi ships no repo_rule of its own to fetch its source
        # (it hasn't migrated to bzlmod yet), so we pin its archive by hand.
        # Update this by hand when bumping the pinned submodule commit.
        source=archive_source(
            url="https://github.com/openconfig/gnoi/archive/2b6ff72de5769839fc68bd019f345a184e3b0bf1.tar.gz",
            sha256="0f71e9452ec8c50f5a87f54d59f709501a2cb4770a4633d773c443379ca4d4e0",
            strip_prefix="gnoi-2b6ff72de5769839fc68bd019f345a184e3b0bf1",
        ),
        overlay_files=["MODULE.bazel"],
    ),
]


def check_repo_is_clean(path: str | None = None) -> None:
    """Abort if there are uncommitted changes (including in submodules).

    If path is given (REPO_ROOT-relative), only that path is checked.
    """
    cmd = ["git", "status", "--porcelain", "--ignore-submodules=none"]
    if path is not None:
        cmd += ["--", path]

    result = subprocess.run(
        cmd,
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        check=True,
    )
    if result.stdout.strip():
        scope = f"{path!r}" if path is not None else "Repo"
        raise click.ClickException(
            f"{scope} has uncommitted changes; commit or stash them before publishing:\n"
            + result.stdout
        )


def resolve_module_path(path: str) -> str:
    """Normalize a user-provided path to a REPO_ROOT-relative string, matching
    how src_path/wrapper_dir are expressed everywhere else in this script."""
    absolute = Path(path).expanduser()
    if not absolute.is_absolute():
        # Under `bazel run`, the process's own cwd is inside the execroot, not
        # wherever the user actually invoked `bazel run` from -- Bazel sets
        # BUILD_WORKING_DIRECTORY for scripts to recover that. Plain
        # `python3 ...` invocations don't set it, so fall back to Path.cwd().
        cwd = Path(os.environ.get("BUILD_WORKING_DIRECTORY", Path.cwd()))
        absolute = cwd / absolute
    absolute = absolute.resolve()

    try:
        return str(absolute.relative_to(REPO_ROOT))
    except ValueError:
        raise click.ClickException(f"{path!r} is not inside the repo ({REPO_ROOT}).") from None


@dataclass(frozen=True)
class SubmoduleModule:
    """A module discovered from a git submodule's own MODULE.bazel."""

    name: str
    version: str
    # REPO_ROOT-relative path to the submodule, e.g. "src/sonic-sysmgr".
    src_path: str


def discover_submodule_modules() -> list[SubmoduleModule]:
    """Find MODULE.bazel files that live inside git submodules."""
    submodule_paths = load_submodule_paths()
    modules = []

    for module_bazel in sorted((REPO_ROOT / "src").rglob("MODULE.bazel")):
        src_path = str(module_bazel.parent.relative_to(REPO_ROOT))
        if not is_git_submodule(src_path, submodule_paths):
            continue
        result = parse_module_declaration(module_bazel)
        if result is None:
            continue
        name, version = result
        modules.append(SubmoduleModule(name=name, version=version, src_path=src_path))

    return modules


def resolve_commit(src_path: str) -> str:
    """Return the full 40-char SHA of the submodule's currently checked-out commit."""
    result = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=REPO_ROOT / src_path,
        capture_output=True,
        text=True,
        check=True,
    )
    return result.stdout.strip()


def parse_github_org_repo(url: str) -> tuple[str, str]:
    """Extract (org, repo) from a github.com submodule URL.

    Fails fast if the URL isn't a github.com URL,
    since archive-based publishing only knows how to build github.com archive URLs.
    """
    match = re.match(r"^https://github\.com/([^/]+)/([^/]+?)(?:\.git)?/?$", url)
    if match is None:
        raise click.ClickException(f"Unsupported submodule remote (not a github.com URL): {url}")
    return match.group(1), match.group(2)


def integrity(b: bytes) -> str:
    """Return the Bazel-style sha256 integrity string for a stream of bytes."""
    digest = hashlib.sha256(b).digest()
    return "sha256-" + base64.b64encode(digest).decode("ascii")


def compute_archive_integrity(archive_url: str) -> str:
    """Download the archive at archive_url and return its Bazel-style sha256 integrity string."""
    response = requests.get(archive_url)
    if not response.ok:
        raise click.ClickException(
            f"Could not download {archive_url} ({e.code} {e.reason}). "
            "The pinned commit is likely missing from the submodule's registered remote "
            "(check .gitmodules) — push it there before publishing."
        )
    # TODO(bazel-ready): Decide if we need to stream the response.
    return integrity(response.content)


def clone_registry() -> Path:
    """Clone the remote registry repo into a fresh temp directory.

    Works even if the remote repo has no commits yet (git clone of an empty
    repo succeeds; it just leaves the checkout with no branch/HEAD).
    """
    tmp_dir = Path(tempfile.mkdtemp(prefix="sonic-bazel-registry-"))
    subprocess.run(
        ["git", "clone", REGISTRY_REPO_URL, str(tmp_dir)],
        capture_output=True,
        text=True,
        check=True,
    )
    return tmp_dir


def registry_version_dir(registry_dir: Path, name: str, version: str) -> Path:
    """The modules/<name>/<version>/ path within a registry checkout."""
    return registry_dir / "modules" / name / version


def is_already_published(registry_dir: Path, name: str, target_version: str) -> bool:
    """Check whether modules/<name>/<target_version>/ already exists in the registry clone."""
    return registry_version_dir(registry_dir, name, target_version).is_dir()


def update_metadata(registry_dir: Path, name: str, target_version: str) -> None:
    """Add target_version to modules/<name>/metadata.json, preserving existing versions."""
    metadata_path = registry_dir / "modules" / name / "metadata.json"
    if metadata_path.exists():
        metadata = json.loads(metadata_path.read_text())
    else:
        metadata = {"versions": []}

    if target_version not in metadata["versions"]:
        metadata["versions"] = sorted(metadata["versions"] + [target_version])

    metadata_path.parent.mkdir(parents=True, exist_ok=True)
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n")


def write_source_json(version_dir: Path, source_json: dict) -> None:
    """Write source.json into version_dir."""
    (version_dir / "source.json").write_text(json.dumps(source_json, indent=2) + "\n")


def write_module_bazel(version_dir: Path, module_bazel_text: str) -> None:
    """Write the top-level MODULE.bazel into version_dir."""
    (version_dir / "MODULE.bazel").write_text(module_bazel_text)


def build_version_patch(original_text: str, new_version: str) -> tuple[str, str]:
    """Return (patch_text, patched_text) that bumps the module()'s version field.

    Follows the BCR convention, where the checked-in MODULE.bazel
    must match the result of applying the registry patches to a given module archive.
    """
    patched_text = rewrite_module_version(original_text, new_version)
    patch_text = "".join(
        difflib.unified_diff(
            original_text.splitlines(keepends=True),
            patched_text.splitlines(keepends=True),
            fromfile="a/MODULE.bazel",
            tofile="b/MODULE.bazel",
        )
    )
    return patch_text, patched_text


def write_module_entry(
    registry_dir: Path, name: str, target_version: str, src_path: str, org: str, repo: str, commit: str
) -> None:
    """Write source.json, a version-bumped MODULE.bazel, its patch, and metadata.json.

    Fetches the archive's integrity itself (rather than taking a pre-built
    source.json) so that, like the other two writers, nothing expensive
    happens until this is actually called for a not-yet-published module.
    """
    archive_url = f"https://github.com/{org}/{repo}/archive/{commit}.tar.gz"
    source_json = {
        "url": archive_url,
        "strip_prefix": f"{repo}-{commit}",
        "integrity": compute_archive_integrity(archive_url),
    }

    version_dir = registry_version_dir(registry_dir, name, target_version)
    version_dir.mkdir(parents=True)

    original_text = (REPO_ROOT / src_path / "MODULE.bazel").read_text()
    patch_text, patched_text = build_version_patch(original_text, target_version)

    patch_name = "bump-version.patch"
    (version_dir / "patches").mkdir()
    (version_dir / "patches" / patch_name).write_text(patch_text)
    patch_integrity = "sha256-" + base64.b64encode(hashlib.sha256(patch_text.encode()).digest()).decode("ascii")

    source_json = {**source_json, "patches": {patch_name: patch_integrity}, "patch_strip": 1}
    write_source_json(version_dir, source_json)
    write_module_bazel(version_dir, patched_text)
    update_metadata(registry_dir, name, target_version)


def write_overlay_module_entry(registry_dir: Path, entry: OverlayModule) -> None:
    """Publish an OVERLAY_MODULES entry: an upstream archive carrying the
    wrapper's own files as an overlay, unmodified except a version bump."""
    version_dir = registry_version_dir(registry_dir, entry.name, entry.version)
    version_dir.mkdir(parents=True)

    wrapper_dir = REPO_ROOT / entry.wrapper_dir
    module_bazel_text = (wrapper_dir / "MODULE.bazel").read_text()

    source_json = {
        "url": entry.source.url,
        "strip_prefix": entry.source.strip_prefix,
        "integrity": entry.source.integrity,
    }

    overlay_dir = version_dir / "overlay"
    overlay_dir.mkdir()

    final_module_bazel_text = None
    for rel_path in entry.overlay_files:
        src = wrapper_dir / rel_path
        dst = overlay_dir / rel_path
        dst.parent.mkdir(parents=True, exist_ok=True)
        if rel_path == "MODULE.bazel":
            final_module_bazel_text = rewrite_module_version(module_bazel_text, entry.version)
            dst.write_text(final_module_bazel_text)
        else:
            shutil.copy(src, dst)

    overlay_integrities = {
        str(f.relative_to(overlay_dir)): integrity(f.read_bytes())
        for f in sorted(overlay_dir.rglob("*"))
        if f.is_file()
    }
    source_json["overlay"] = overlay_integrities

    write_source_json(version_dir, source_json)
    write_module_bazel(version_dir, final_module_bazel_text)
    update_metadata(registry_dir, entry.name, entry.version)


def commit_changes(registry_dir: Path, published: list[tuple[str, str]]) -> str:
    """Create a branch and commit all newly-written module files. Returns the branch name."""
    branch_name = "publish/" + datetime.now().strftime("%Y%m%d-%H%M%S")
    message = "Publish " + ", ".join(f"{name} {version}" for name, version in published)

    subprocess.run(
        ["git", "checkout", "-b", branch_name], cwd=registry_dir, check=True, capture_output=True, text=True
    )
    subprocess.run(["git", "add", "-A"], cwd=registry_dir, check=True, capture_output=True, text=True)
    subprocess.run(
        ["git", "commit", "-m", message], cwd=registry_dir, check=True, capture_output=True, text=True
    )
    return branch_name


def push_and_create_pr(registry_dir: Path, branch_name: str, published: list[tuple[str, str]]) -> str:
    """Push branch_name and open a PR against the registry repo. Returns the PR URL."""
    subprocess.run(
        ["git", "push", "-u", "origin", branch_name],
        cwd=registry_dir,
        check=True,
        capture_output=True,
        text=True,
    )

    title = "Publish " + ", ".join(f"{name}" for name, _ in published)
    body = "Published modules:\n" + "\n".join(f"- {name} {version}" for name, version in published)

    result = subprocess.run(
        ["gh", "pr", "create", "--title", title, "--body", body, "--head", branch_name],
        cwd=registry_dir,
        check=True,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip()


def gather_candidates(
    modules: list[SubmoduleModule],
    overlay_modules: list[OverlayModule],
    submodule_paths: set[str],
    submodule_urls: dict[str, str],
) -> list[PublishCandidate]:
    candidates: list[PublishCandidate] = []

    for module in modules:
        root = submodule_root_for(module.src_path, submodule_paths)
        org, repo = parse_github_org_repo(submodule_urls[root])
        commit = resolve_commit(module.src_path)
        target_version = f"{module.version}-{commit}"
        candidates.append(PublishCandidate(
            name=module.name,
            version=target_version,
            path=module.src_path,
            write=partial(
                write_module_entry,
                name=module.name,
                target_version=target_version,
                src_path=module.src_path,
                org=org,
                repo=repo,
                commit=commit,
            ),
        ))

    for entry in overlay_modules:
        candidates.append(PublishCandidate(
            name=entry.name,
            version=entry.version,
            path=entry.wrapper_dir,
            write=partial(write_overlay_module_entry, entry=entry),
        ))

    return candidates

def publish_candidates(registry_dir: Path, candidates: list[PublishCandidate]) -> list[tuple[str, str]]:
    published = []
    for candidate in candidates:
        if is_already_published(registry_dir, candidate.name, candidate.version):
            print(f"skip (already published): {candidate.name} {candidate.version}")
            continue

        candidate.write(registry_dir)
        published.append((candidate.name, candidate.version))
        print(f"new: {candidate.name} {candidate.version}")

    return published

@click.command()
@click.argument("path", required=False)
def main(path: str | None) -> None:
    """Publish git-submodule-backed and external (BCR-style) modules to the remote registry.

    If PATH is given (e.g. src/libnl3), only the module anchored there is
    published, and only PATH itself needs to be a clean checkout -- not the
    whole repo.
    """
    module_path = resolve_module_path(path) if path is not None else None
    check_repo_is_clean(module_path)

    modules = discover_submodule_modules()
    overlay_modules = OVERLAY_MODULES

    if module_path is not None:
        modules = [m for m in modules if m.src_path == module_path]
        overlay_modules = [e for e in overlay_modules if e.wrapper_dir == module_path]
        if not modules and not overlay_modules:
            raise click.ClickException(f"No publishable module found at path {module_path!r}.")

    if not modules and not overlay_modules:
        print("No modules found to publish.")
        return

    submodule_paths = load_submodule_paths()
    submodule_urls = load_submodule_urls()

    candidates: list[PublishCandidate] = gather_candidates(modules, overlay_modules, submodule_paths, submodule_urls)

    registry_dir = clone_registry()
    print(f"Cloned {REGISTRY_REPO_URL} to {registry_dir}")

    published: list[tuple[str, str]] = publish_candidates(registry_dir, candidates)
    if not published:
        print("Nothing new to publish.")
        return

    branch_name = commit_changes(registry_dir, published)
    pr_url = push_and_create_pr(registry_dir, branch_name, published)
    print(f"Opened PR: {pr_url}")


if __name__ == "__main__":
    main()
