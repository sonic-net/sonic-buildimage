"""Shared helpers for generating and publishing sonic-buildimage Bazel modules."""

import configparser
import re
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = (SCRIPT_DIR / "../../..").resolve()


def _find_matching_close_paren(text: str, open_paren_index: int) -> int:
    """Return the index of the ')' matching the '(' at open_paren_index."""
    depth = 0
    for i in range(open_paren_index, len(text)):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                return i
    raise ValueError("Unbalanced parentheses")


def _extract_module_call_span(text: str) -> tuple[int, int] | None:
    """Return (start, end) indices of the module(...) call's argument text, or None if absent."""
    match = re.search(r"module\(", text)
    if match is None:
        return None
    open_paren = text.index("(", match.start())
    close_paren = _find_matching_close_paren(text, open_paren)
    return open_paren + 1, close_paren


def _extract_module_call(text: str) -> str | None:
    """Return the argument text of the module(...) call, or None if absent."""
    span = _extract_module_call_span(text)
    return text[span[0] : span[1]] if span else None


def parse_module_declaration(module_bazel: Path) -> tuple[str, str] | None:
    """Extract (name, version) from a MODULE.bazel's module() declaration.

    Returns None if the file has no module() call or no name field.
    """
    module_call = _extract_module_call(module_bazel.read_text())
    if module_call is None:
        return None

    name_match = re.search(r'name\s*=\s*"([^"]+)"', module_call)
    if name_match is None:
        return None

    version_match = re.search(r'version\s*=\s*"([^"]+)"', module_call)
    version = version_match.group(1) if version_match else "0.0.0"

    return name_match.group(1), version


def rewrite_module_version(text: str, new_version: str) -> str:
    """Return MODULE.bazel text with the module() call's version field set to new_version."""
    span = _extract_module_call_span(text)
    if span is None:
        raise ValueError("No module() call found")
    start, end = span

    module_call = text[start:end]
    if re.search(r'version\s*=\s*"[^"]*"', module_call):
        new_module_call = re.sub(r'version\s*=\s*"[^"]*"', f'version = "{new_version}"', module_call, count=1)
    else:
        new_module_call = module_call.rstrip() + f',\n    version = "{new_version}",\n'

    return text[:start] + new_module_call + text[end:]


def extract_repo_rule_call(text: str, rule_name: str) -> str:
    """Return the argument text of a `<rule_name>(...)` invocation."""
    match = re.search(rf"\n{re.escape(rule_name)}\s*\(", text)
    if match is None:
        raise ValueError(f"No {rule_name}(...) call found")

    open_paren = text.index("(", match.start())
    close_paren = _find_matching_close_paren(text, open_paren)
    return text[open_paren + 1 : close_paren]


def extract_str_kwarg(call_text: str, kwarg: str) -> str:
    """Extract a plain string-literal kwarg's value from a call's argument text."""
    match = re.search(rf'{re.escape(kwarg)}\s*=\s*"([^"]*)"', call_text)
    if match is None:
        raise ValueError(f"No string kwarg {kwarg!r} found")
    return match.group(1)


def extract_single_item_list_kwarg(call_text: str, kwarg: str) -> str:
    """Extract the one string-literal element of a single-item list kwarg (e.g. urls = ["..."])."""
    match = re.search(rf'{re.escape(kwarg)}\s*=\s*\[\s*"([^"]*)"\s*,?\s*\]', call_text)
    if match is None:
        raise ValueError(f"No single-element list kwarg {kwarg!r} found")
    return match.group(1)


def load_submodule_paths() -> set[str]:
    """Return the set of git submodule paths declared in .gitmodules, relative to repo root."""
    gitmodules = REPO_ROOT / ".gitmodules"
    if not gitmodules.exists():
        return set()

    parser = configparser.ConfigParser()
    parser.read(gitmodules)
    return {
        parser.get(section, "path")
        for section in parser.sections()
        if parser.has_option(section, "path")
    }


def load_submodule_urls() -> dict[str, str]:
    """Return a mapping of git submodule path -> remote URL, from .gitmodules."""
    gitmodules = REPO_ROOT / ".gitmodules"
    if not gitmodules.exists():
        return {}

    parser = configparser.ConfigParser()
    parser.read(gitmodules)
    return {
        parser.get(section, "path"): parser.get(section, "url")
        for section in parser.sections()
        if parser.has_option(section, "path") and parser.has_option(section, "url")
    }


def is_git_submodule(src_path: str, submodule_paths: set[str]) -> bool:
    """Check whether src_path is a git submodule (or lives inside one)."""
    return any(
        src_path == submodule_path or src_path.startswith(submodule_path + "/")
        for submodule_path in submodule_paths
    )


def submodule_root_for(src_path: str, submodule_paths: set[str]) -> str | None:
    """Return the submodule path that src_path is (or lives inside), or None."""
    for submodule_path in submodule_paths:
        if src_path == submodule_path or src_path.startswith(submodule_path + "/"):
            return submodule_path
    return None


def discover_top_level_bazel_modules() -> list[tuple[str, str]]:
    """Return (module_name, src_path) for every directory directly under src/
    whose own MODULE.bazel has a real module() declaration.

    Includes both git submodules (e.g. sonic-swss-common)
    and plain vendored directories (e.g. sonic-sysmgr, libnl3)
    """
    modules = []
    for module_bazel in sorted((REPO_ROOT / "src").glob("*/MODULE.bazel")):
        result = parse_module_declaration(module_bazel)
        if result is None:
            continue
        name, _ = result
        src_path = str(module_bazel.parent.relative_to(REPO_ROOT))
        modules.append((name, src_path))
    return sorted(modules)
