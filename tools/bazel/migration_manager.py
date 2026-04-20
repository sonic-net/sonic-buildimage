#!/usr/bin/env python3
"""Cross-Repository Commit Table Generator.

Recursively traverses Bazel module dependencies via local_path_override
directives and produces a Markdown table showing compatible commit SHAs
across all linked repositories.
"""

import argparse
import json
import os
import re
import subprocess
import sys


def red(text):
    return f"\033[1;31m{text}\033[0m"


def bold_purple(text):
    return f"\033[1;35m{text}\033[0m"


def bold(text):
    return f"\033[1m{text}\033[0m"


def dim(text):
    return f"\033[2m{text}\033[0m"


def format_grep_hit(hit):
    """Style a 'git grep -n' hit line: bold coords, dim content."""
    parts = hit.split(":", 2)
    if len(parts) == 3:
        filename, lineno, content = parts
        return f"    {bold(filename + ':' + lineno)}: {dim(content)}"
    return f"    {hit}"


def git_rev_parse(repo_path, short=False):
    cmd = ["git", "rev-parse"]
    if short:
        cmd.append("--short")
    cmd.append("HEAD")
    try:
        result = subprocess.run(
            cmd, cwd=repo_path, capture_output=True, text=True, check=True
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return None


def git_commit_message(repo_path):
    """Return the subject line of the HEAD commit, or '' if unavailable."""
    try:
        result = subprocess.run(
            ["git", "log", "-1", "--format=%s"],
            cwd=repo_path, capture_output=True, text=True, check=True
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return ""


def git_grep_todo_bl(repo_path):
    """Return list of 'file:line' hits for 'BL:' in the repo, or [] if none."""
    try:
        result = subprocess.run(
            ["git", "grep", "-n", "--fixed-strings", " BL:"],
            cwd=repo_path, capture_output=True, text=True
        )
        # exit code 0 = matches found, 1 = no matches, anything else = error
        if result.returncode == 0:
            hits = result.stdout.strip().splitlines()
            # Ignore binary file notifications ("Binary file X matches")
            # and hits in migration_manager.py itself.
            hits = [
                h for h in hits
                if not h.startswith("Binary file ")
                and not h.split(":")[0].endswith("migration_manager.py")
            ]
            return hits
        return []
    except OSError:
        return []


def git_grep_todo_bazel_ready(repo_path):
    """Return list of 'file:line' hits for 'TODO(bazel-ready)' in the repo, or [] if none."""
    try:
        result = subprocess.run(
            ["git", "grep", "-n", "--fixed-strings", "TODO(bazel-ready)"],
            cwd=repo_path, capture_output=True, text=True
        )
        if result.returncode == 0:
            return result.stdout.strip().splitlines()
        return []
    except OSError:
        return []


def git_has_uncommitted_changes(repo_path):
    try:
        result = subprocess.run(
            ["git", "status", "--porcelain"],
            cwd=repo_path, capture_output=True, text=True, check=True
        )
        return bool(result.stdout.strip())
    except subprocess.CalledProcessError:
        return False


def git_branch(repo_path):
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            cwd=repo_path, capture_output=True, text=True, check=True
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return None


def _parse_single_gitmodules(gitmodules_path):
    """Parse a single .gitmodules file and return a dict mapping submodule path -> url."""
    result = {}
    try:
        with open(gitmodules_path, "r") as f:
            content = f.read()
    except OSError:
        return result
    for block in re.finditer(
        r'\[submodule\s+"[^"]*"\]\s*'
        r'(?:(?!\[submodule).)*',
        content, re.DOTALL,
    ):
        text = block.group(0)
        path_m = re.search(r'path\s*=\s*(.+)', text)
        url_m = re.search(r'url\s*=\s*(.+)', text)
        if path_m and url_m:
            result[path_m.group(1).strip()] = url_m.group(1).strip()
    return result


def parse_gitmodules(root_path):
    """Parse .gitmodules in root_path and recursively in submodules.

    Returns a dict mapping paths (relative to root_path) -> url.
    For nested submodules (e.g. SAI inside sonic-sairedis), the path
    is relative to root_path (e.g. "src/sonic-sairedis/SAI") and the
    url comes from the parent submodule's .gitmodules.
    """
    result = {}
    top_level = _parse_single_gitmodules(os.path.join(root_path, ".gitmodules"))
    result.update(top_level)

    # Check each submodule for its own .gitmodules (nested submodules).
    for sub_path in list(top_level.keys()):
        sub_abs = os.path.join(root_path, sub_path)
        nested = _parse_single_gitmodules(os.path.join(sub_abs, ".gitmodules"))
        for nested_rel, url in nested.items():
            # Translate the nested-relative path to root-relative.
            result[os.path.join(sub_path, nested_rel)] = url
    return result


def git_create_tag(repo_path, tag_name):
    """Create a lightweight tag in repo_path. Returns (True, '') on success or (False, err) on failure."""
    try:
        result = subprocess.run(
            ["git", "tag", tag_name],
            cwd=repo_path, capture_output=True, text=True
        )
        if result.returncode == 0:
            return True, ""
        return False, result.stderr.strip()
    except OSError as e:
        return False, str(e)


def run_tests(repo_path):
    """Run test_working_targets.sh from repo_path. Returns True on success."""
    script = os.path.join(os.path.dirname(os.path.realpath(__file__)), "test_working_targets.sh")
    if not os.path.isfile(script):
        print(f"Error: could not find test script at {script}", file=sys.stderr)
        return False
    print("Running test_working_targets.sh ...", file=sys.stderr)
    result = subprocess.run([script], cwd=repo_path)
    return result.returncode == 0


def git_push_tag(repo_path, remote, tag_name):
    """Push tag_name to remote in repo_path. Returns (True, '') on success or (False, err) on failure."""
    try:
        result = subprocess.run(
            ["git", "push", remote, tag_name],
            cwd=repo_path, capture_output=True, text=True
        )
        if result.returncode == 0:
            return True, ""
        return False, result.stderr.strip()
    except OSError as e:
        return False, str(e)


def git_has_unpushed_commits(repo_path):
    """Return True if the current branch has commits not pushed to the 'thesayyn' remote."""
    try:
        branch = git_branch(repo_path)
        if not branch:
            return False
        # Check if thesayyn remote has this branch
        result = subprocess.run(
            ["git", "rev-parse", "--verify", f"thesayyn/{branch}"],
            cwd=repo_path, capture_output=True, text=True
        )
        if result.returncode != 0:
            # Remote branch doesn't exist — all local commits are unpushed
            return True
        # Count commits ahead of the remote branch
        result = subprocess.run(
            ["git", "rev-list", "--count", f"thesayyn/{branch}..HEAD"],
            cwd=repo_path, capture_output=True, text=True, check=True
        )
        return int(result.stdout.strip()) > 0
    except (subprocess.CalledProcessError, ValueError):
        return False


BAZEL_GITIGNORE_PATTERNS = [
    "MODULE.bazel",
    "MODULE.bazel.lock",
    "BUILD",
    "BUILD.bazel",
    ".bazelversion",
    ".bazelrc",
    "WORKSPACE",
    "WORKSPACE.bazel",
]


def git_check_ignored_bazel_files(repo_path):
    """Return list of Bazel-related file patterns that are gitignored in this repo."""
    ignored = []
    try:
        result = subprocess.run(
            ["git", "check-ignore", "--"] + BAZEL_GITIGNORE_PATTERNS,
            cwd=repo_path, capture_output=True, text=True
        )
        if result.returncode == 0 and result.stdout.strip():
            ignored = result.stdout.strip().splitlines()
    except OSError:
        pass
    return ignored


REPO_NAME_OVERRIDES = {
    "sonic-sairedis/SAI": "opencompute/SAI",
}


def repo_name(abs_path):
    parent = os.path.basename(os.path.dirname(abs_path))
    name = os.path.basename(abs_path)
    default = f"{parent}/{name}"
    # For submodules under src/, use "sonic-net/<name>" as the display name
    # to match the upstream repository naming convention.
    if parent == "src" and name.startswith("sonic-"):
        default = f"sonic-net/{name}"
    return REPO_NAME_OVERRIDES.get(default, default)


def parse_local_path_overrides(module_bazel_path):
    """Extract path values from local_path_override directives."""
    try:
        with open(module_bazel_path, "r") as f:
            content = f.read()
    except OSError:
        return []

    paths = []
    pattern = re.compile(
        r'local_path_override\s*\([^)]*?path\s*=\s*"([^"]+)"', re.DOTALL
    )
    for match in pattern.finditer(content):
        paths.append(match.group(1))
    return paths


def process_repo(repo_path, visited, results, dirty_repos, unpushed_repos,
                 ignored_bazel_files, todo_hits, bazel_ready_hits,
                 short=False, include_branch=False, include_message=False,
                 parent_path=None, root_path=None, submodule_urls=None):
    abs_path = os.path.realpath(repo_path)

    if abs_path in visited:
        return
    visited.add(abs_path)

    # Skip subdirectories that live under the root repo but share its
    # git history (i.e. are not their own git repository / submodule).
    if root_path:
        root_abs = os.path.realpath(root_path)
        if abs_path.startswith(root_abs + os.sep):
            # It's inside the root tree. Only process it if it has its
            # own .git (a submodule), otherwise it shares the root's
            # git history and should be skipped.
            git_marker = os.path.join(abs_path, ".git")
            if not os.path.exists(git_marker):
                return

    if not os.path.isdir(abs_path):
        print(f"Warning: path does not exist: {abs_path}", file=sys.stderr)
        return

    sha = git_rev_parse(abs_path, short=short)
    if sha is None:
        print(f"Warning: not a git repository: {abs_path}", file=sys.stderr)
        return

    if git_has_uncommitted_changes(abs_path):
        dirty_repos.append(abs_path)

    if git_has_unpushed_commits(abs_path):
        unpushed_repos.append(abs_path)

    ignored = git_check_ignored_bazel_files(abs_path)
    if ignored:
        ignored_bazel_files[abs_path] = ignored

    bl_hits = git_grep_todo_bl(abs_path)
    if bl_hits:
        todo_hits[abs_path] = bl_hits

    br_hits = git_grep_todo_bazel_ready(abs_path)
    if br_hits:
        bazel_ready_hits[abs_path] = br_hits

    # For submodules under src/, show on-disk location and remote from .gitmodules.
    # For repos outside src/, these columns stay empty.
    effective_root = root_path or abs_path
    root_abs = os.path.realpath(effective_root)
    rel_path = os.path.relpath(abs_path, root_abs)
    is_src_submodule = rel_path.startswith("src" + os.sep)

    location = ""
    remote = ""
    if is_src_submodule:
        location = rel_path
        if submodule_urls:
            remote = submodule_urls.get(rel_path, "")

    entry = {
        "repository": repo_name(abs_path),
        "path": abs_path,
        "location": location,
        "remote": remote,
        "commit": sha,
        "is_working": not bool(bl_hits),
        "is_bazel_ready": not bool(br_hits),
    }
    if include_branch:
        entry["branch"] = git_branch(abs_path) or "unknown"
    if include_message:
        entry["message"] = git_commit_message(abs_path)
    results.append(entry)

    module_bazel = os.path.join(abs_path, "MODULE.bazel")
    if not os.path.isfile(module_bazel):
        print(f"Warning: no MODULE.bazel in {abs_path}", file=sys.stderr)
        return

    # The first repo processed is the root.
    effective_root = root_path or abs_path

    # Parse .gitmodules once when processing the root repo.
    if submodule_urls is None and root_path is None:
        submodule_urls = parse_gitmodules(abs_path)

    for rel_path in parse_local_path_overrides(module_bazel):
        dep_path = os.path.join(abs_path, rel_path)
        process_repo(dep_path, visited, results, dirty_repos, unpushed_repos,
                     ignored_bazel_files, todo_hits, bazel_ready_hits,
                     short=short, include_branch=include_branch,
                     include_message=include_message,
                     parent_path=abs_path, root_path=effective_root,
                     submodule_urls=submodule_urls)


def format_markdown(results, include_branch=False, include_message=False):
    headers = ["Repository", "Commit"]
    keys = ["repository", "commit"]
    if include_branch:
        headers.append("Branch")
        keys.append("branch")
    if include_message:
        headers.append("Message")
        keys.append("message")
    headers += ["Location", "Remote"]
    keys += ["location", "remote"]

    # Compute column widths from headers and data.
    widths = [len(h) for h in headers]
    for r in results:
        for i, k in enumerate(keys):
            widths[i] = max(widths[i], len(r.get(k, "")))

    def row(cells):
        padded = [c.ljust(w) for c, w in zip(cells, widths)]
        return "| " + " | ".join(padded) + " |"

    lines = [
        row(headers),
        "|" + "|".join("-" * (w + 2) for w in widths) + "|",
    ]
    for r in results:
        lines.append(row([r.get(k, "") for k in keys]))
    return "\n".join(lines)


def format_checkpoint_markdown(results, todo_bl_hits, todo_bazel_ready_hits,
                                include_branch=False, include_message=False):
    import datetime
    date_str = datetime.date.today().isoformat()
    lines = [f"# Checkpoint -- {date_str}", ""]

    headers = ["Repository", "Commit"]
    keys = ["repository", "commit"]
    if include_branch:
        headers.append("Branch")
        keys.append("branch")
    if include_message:
        headers.append("Message")
        keys.append("message")
    headers += ["Location", "Remote", "Working", "Bazel-Ready"]

    # Variable-width columns: compute from data
    extra_keys = ["location", "remote"]
    all_keys = keys + extra_keys
    widths = [len(h) for h in headers[:len(all_keys)]]
    for r in results:
        for i, k in enumerate(all_keys):
            widths[i] = max(widths[i], len(r.get(k, "")))
    # Status columns: fixed to header width
    status_widths = [len("Working"), len("Bazel-Ready")]
    all_widths = widths + status_widths

    CHECK, CROSS = "✓", "✗"

    def row(cells):
        padded = [c.ljust(w) for c, w in zip(cells, all_widths)]
        return "| " + " | ".join(padded) + " |"

    lines.append(row(headers))
    lines.append("|" + "|".join("-" * (w + 2) for w in all_widths) + "|")
    for r in results:
        cells = [r.get(k, "") for k in all_keys] + [
            CHECK if r["is_working"] else CROSS,
            CHECK if r["is_bazel_ready"] else CROSS,
        ]
        lines.append(row(cells))

    if todo_bl_hits or todo_bazel_ready_hits:
        lines += ["", "## Issues", ""]
        for section_title, hits_dict in [
            ("### TODO BL: markers", todo_bl_hits),
            ("### TODO(bazel-ready) markers", todo_bazel_ready_hits),
        ]:
            if hits_dict:
                lines.append(section_title)
                lines.append("")
                for repo_path, hits in hits_dict.items():
                    lines.append(f"**{repo_name(repo_path)}** (`{repo_path}`)")
                    lines.append("")
                    lines.append("```")
                    lines.extend(hits)
                    lines.append("```")
                    lines.append("")

    return "\n".join(lines)


def report_errors(dirty_repos, unpushed_repos, ignored_bazel_files, todo_hits):
    """Print all error diagnostics to stderr. Returns True if any errors were found."""
    errors = False

    if dirty_repos:
        print(red("Error: the following repositories have uncommitted changes:"),
              file=sys.stderr)
        for path in dirty_repos:
            print(f"  {bold_purple(path)}", file=sys.stderr)
        errors = True

    if unpushed_repos:
        print(red("Error: the following repositories have unpushed commits (remote: thesayyn):"),
              file=sys.stderr)
        for path in unpushed_repos:
            print(f"  {bold_purple(path)}", file=sys.stderr)
        errors = True

    if ignored_bazel_files:
        print(red("Error: the following repositories have gitignored Bazel files:"),
              file=sys.stderr)
        for path, files in ignored_bazel_files.items():
            print(f"  {bold_purple(path)}", file=sys.stderr)
            for f in files:
                print(f"    {dim(f)}", file=sys.stderr)
        errors = True

    if todo_hits:
        print(red("Error: the following repositories contain 'BL:' markers:"),
              file=sys.stderr)
        for path, hits in todo_hits.items():
            print(f"  {bold_purple(path)}", file=sys.stderr)
            for hit in hits:
                print(format_grep_hit(hit), file=sys.stderr)
        errors = True

    return errors


def main():
    parser = argparse.ArgumentParser(
        description="Generate a commit table from Bazel local_path_override dependencies"
    )
    parser.add_argument("repo", help="Path to the central repository")
    parser.add_argument("--short", action="store_true",
                        help="Use short commit SHAs")
    parser.add_argument("--include-branch", action="store_true",
                        help="Also show current branch name")
    parser.add_argument("--json", action="store_true", dest="output_json",
                        help="Output as JSON instead of Markdown")
    parser.add_argument("--checkpoint", action="store_true",
                        help="Output a checkpoint Markdown document with status columns "
                             "(does not fail on BL: or bazel-ready markers)")
    parser.add_argument("--tag", action="store_true",
                        help="When used with --checkpoint, create a 'checkpoint-<date>' git tag "
                             "in every repository (only if all repos are clean)")
    parser.add_argument("--push-remote", metavar="REMOTE",
                        help="When used with --tag, push the created tags to this remote "
                             "(e.g. 'origin')")
    parser.add_argument("--run-tests", action="store_true",
                        help="Run test_working_targets.sh before creating a checkpoint "
                             "(tests always run before tagging)")
    parser.add_argument("--include-message", action="store_true",
                        help="Also show the subject line of the HEAD commit")
    args = parser.parse_args()

    results = []
    visited = set()
    dirty_repos = []
    unpushed_repos = []
    ignored_bazel_files = {}
    todo_hits = {}
    bazel_ready_hits = {}
    process_repo(args.repo, visited, results, dirty_repos, unpushed_repos,
                 ignored_bazel_files, todo_hits, bazel_ready_hits,
                 short=args.short, include_branch=args.include_branch,
                 include_message=args.include_message)

    if args.checkpoint:
        if args.run_tests and not run_tests(args.repo):
            print("Aborting checkpoint: tests failed.", file=sys.stderr)
            sys.exit(1)

        if args.output_json:
            print(json.dumps(results, indent=2))
        else:
            print(format_checkpoint_markdown(
                results, todo_hits, bazel_ready_hits,
                include_branch=args.include_branch,
                include_message=args.include_message
            ))

        if args.tag:
            if dirty_repos:
                print("Skipping tags: the following repositories have uncommitted changes:",
                      file=sys.stderr)
                for path in dirty_repos:
                    print(f"  {bold_purple(path)}", file=sys.stderr)
            elif not run_tests(args.repo):
                print("Skipping tags: tests failed.", file=sys.stderr)
            else:
                import datetime
                tag_name = f"checkpoint-{datetime.date.today().isoformat()}"
                for r in results:
                    ok, err = git_create_tag(r["path"], tag_name)
                    if ok:
                        print(f"Tagged {r['repository']} with {tag_name}", file=sys.stderr)
                    else:
                        print(f"Warning: could not tag {r['repository']}: {err}", file=sys.stderr)
                        continue
                    if args.push_remote:
                        ok, err = git_push_tag(r["path"], args.push_remote, tag_name)
                        if ok:
                            print(f"Pushed {tag_name} in {r['repository']} to {args.push_remote}",
                                  file=sys.stderr)
                        else:
                            print(f"Warning: could not push tag in {r['repository']}: {err}",
                                  file=sys.stderr)

        report_errors(dirty_repos, unpushed_repos, ignored_bazel_files,
                      todo_hits)
        return

    errors = report_errors(dirty_repos, unpushed_repos, ignored_bazel_files,
                           todo_hits)

    if not errors:
        if args.output_json:
            print(json.dumps(results, indent=2))
        else:
            print(format_markdown(results, include_branch=args.include_branch,
                                  include_message=args.include_message))
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
