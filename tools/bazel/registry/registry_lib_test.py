"""Data tests for registry_lib.py's pure text-manipulation helpers.

Fixtures should live in memory.
"""

import sys
import tempfile
from pathlib import Path

import registry_lib

MODULE_BAZEL_SAMPLE = """\
module(
    name = "sonic-swss-common",
    version = "0.0.0",
)

bazel_dep(name = "libnl3", version = "3.7.0.sonic-buildimage")
bazel_dep(name = "sonic-build-infra", version = "0.0.0", repo_name = "sonic_build_infra")
"""

REPO_RULE_SAMPLE = """\
module(
    name = "libnl3",
    version = "3.7.0",
)

libnl3_src = use_repo_rule("//:libnl3_src.bzl", "libnl3_src")

libnl3_src(
    name = "libnl3_src",
    sha256 = "9fe43ccbeeea72c653bdcf8c93332583135cda46a79507bfd0a483bb57f65939",
    strip_prefix = "libnl-3.7.0",
    urls = ["http://example.com/libnl3.tar.gz"],
)
"""


def check(condition: bool, description: str) -> bool:
    print(f"{'PASS' if condition else 'FAIL'}: {description}")
    return condition


def test_extract_module_call_finds_declaration() -> bool:
    call = registry_lib._extract_module_call(MODULE_BAZEL_SAMPLE)
    return check(
        call is not None and 'name = "sonic-swss-common"' in call and 'version = "0.0.0"' in call,
        "_extract_module_call finds the module() declaration",
    )


def test_extract_module_call_returns_none_without_module() -> bool:
    call = registry_lib._extract_module_call("bazel_dep(name = \"foo\", version = \"1.0\")\n")
    return check(call is None, "_extract_module_call returns None with no module() call")


def test_parse_module_declaration_reads_name_and_version() -> bool:
    with tempfile.TemporaryDirectory() as tmp:
        module_bazel = Path(tmp) / "MODULE.bazel"
        module_bazel.write_text(MODULE_BAZEL_SAMPLE)
        result = registry_lib.parse_module_declaration(module_bazel)
    return check(
        result == ("sonic-swss-common", "0.0.0"),
        "parse_module_declaration reads (name, version)",
    )


def test_parse_module_declaration_defaults_version() -> bool:
    with tempfile.TemporaryDirectory() as tmp:
        module_bazel = Path(tmp) / "MODULE.bazel"
        module_bazel.write_text('module(\n    name = "foo",\n)\n')
        result = registry_lib.parse_module_declaration(module_bazel)
    return check(result == ("foo", "0.0.0"), "parse_module_declaration defaults version to 0.0.0")


def test_parse_module_declaration_none_without_module() -> bool:
    with tempfile.TemporaryDirectory() as tmp:
        module_bazel = Path(tmp) / "MODULE.bazel"
        module_bazel.write_text('bazel_dep(name = "foo", version = "1.0")\n')
        result = registry_lib.parse_module_declaration(module_bazel)
    return check(result is None, "parse_module_declaration returns None with no module() call")


def test_rewrite_module_version_replaces_existing() -> bool:
    rewritten = registry_lib.rewrite_module_version(MODULE_BAZEL_SAMPLE, "0.0.0-abc123")
    module_call = registry_lib._extract_module_call(rewritten)
    return check(
        module_call is not None and 'version = "0.0.0-abc123"' in module_call and "0.0.0\"," not in module_call,
        "rewrite_module_version replaces an existing version field",
    )


def test_rewrite_module_version_preserves_rest_of_file() -> bool:
    rewritten = registry_lib.rewrite_module_version(MODULE_BAZEL_SAMPLE, "0.0.0-abc123")
    return check(
        'bazel_dep(name = "libnl3", version = "3.7.0.sonic-buildimage")' in rewritten,
        "rewrite_module_version leaves content outside module() untouched",
    )


def test_rewrite_module_version_adds_missing_version() -> bool:
    rewritten = registry_lib.rewrite_module_version('module(\n    name = "foo",\n)\n', "1.2.3")
    call = registry_lib._extract_module_call(rewritten)
    return check(
        call is not None and 'version = "1.2.3"' in call,
        "rewrite_module_version adds a version field when one is absent",
    )


def test_extract_module_call_handles_nested_parens() -> bool:
    # module() calls elsewhere in the codebase never contain nested parens,
    # but the extraction uses real balanced-paren matching rather than a
    # naive non-greedy regex, so it stays correct if that ever changes.
    text = 'module(name = "foo", x = bar(1), version = "1.0")\n'
    call = registry_lib._extract_module_call(text)
    return check(
        call == 'name = "foo", x = bar(1), version = "1.0"',
        "_extract_module_call handles a nested paren inside module() correctly",
    )


def test_extract_repo_rule_call_raises_on_unbalanced_parens() -> bool:
    try:
        registry_lib.extract_repo_rule_call('libnl3_src(\n    name = "x",\n', "libnl3_src")
    except ValueError:
        return check(True, "extract_repo_rule_call raises ValueError on unbalanced parens")
    return check(False, "extract_repo_rule_call raises ValueError on unbalanced parens")


def test_extract_repo_rule_call_raises_when_rule_absent() -> bool:
    try:
        registry_lib.extract_repo_rule_call(MODULE_BAZEL_SAMPLE, "does_not_exist")
    except ValueError:
        return check(True, "extract_repo_rule_call raises ValueError when the rule call is absent")
    return check(False, "extract_repo_rule_call raises ValueError when the rule call is absent")


def test_extract_single_item_list_kwarg_raises_on_multiple_items() -> bool:
    try:
        registry_lib.extract_single_item_list_kwarg('urls = ["a", "b"]', "urls")
    except ValueError:
        return check(True, "extract_single_item_list_kwarg raises ValueError on a multi-item list")
    return check(False, "extract_single_item_list_kwarg raises ValueError on a multi-item list")


def test_extract_single_item_list_kwarg_raises_on_empty_list() -> bool:
    try:
        registry_lib.extract_single_item_list_kwarg("urls = []", "urls")
    except ValueError:
        return check(True, "extract_single_item_list_kwarg raises ValueError on an empty list")
    return check(False, "extract_single_item_list_kwarg raises ValueError on an empty list")


def test_is_git_submodule_matches_exact_and_nested_paths() -> bool:
    paths = {"src/sonic-build-infra"}
    return check(
        registry_lib.is_git_submodule("src/sonic-build-infra", paths)
        and registry_lib.is_git_submodule("src/sonic-build-infra/tests/foo", paths)
        and not registry_lib.is_git_submodule("src/sonic-build-infra-other", paths)
        and not registry_lib.is_git_submodule("src/unrelated", paths),
        "is_git_submodule matches exact and nested paths, not lookalike prefixes",
    )


def test_submodule_root_for_returns_matching_root_or_none() -> bool:
    paths = {"src/sonic-build-infra"}
    return check(
        registry_lib.submodule_root_for("src/sonic-build-infra/tests/foo", paths) == "src/sonic-build-infra"
        and registry_lib.submodule_root_for("src/unrelated", paths) is None,
        "submodule_root_for returns the covering submodule path, or None",
    )


def test_extract_repo_rule_call_finds_invocation_not_assignment() -> bool:
    call = registry_lib.extract_repo_rule_call(REPO_RULE_SAMPLE, "libnl3_src")
    return check(
        'sha256 = "9fe43ccb' in call and "use_repo_rule" not in call,
        "extract_repo_rule_call finds the invocation, not the use_repo_rule assignment",
    )


def test_extract_str_kwarg_extracts_value() -> bool:
    call = registry_lib.extract_repo_rule_call(REPO_RULE_SAMPLE, "libnl3_src")
    value = registry_lib.extract_str_kwarg(call, "strip_prefix")
    return check(value == "libnl-3.7.0", "extract_str_kwarg extracts a plain string kwarg")


def test_extract_str_kwarg_raises_when_missing() -> bool:
    call = registry_lib.extract_repo_rule_call(REPO_RULE_SAMPLE, "libnl3_src")
    try:
        registry_lib.extract_str_kwarg(call, "does_not_exist")
    except ValueError:
        return check(True, "extract_str_kwarg raises ValueError for a missing kwarg")
    return check(False, "extract_str_kwarg raises ValueError for a missing kwarg")


def test_extract_str_kwarg_raises_on_non_string_value() -> bool:
    try:
        registry_lib.extract_str_kwarg("sha256 = SOME_CONSTANT", "sha256")
    except ValueError:
        return check(True, "extract_str_kwarg raises ValueError for a non-string value")
    return check(False, "extract_str_kwarg raises ValueError for a non-string value")


def test_extract_single_item_list_kwarg_extracts_value() -> bool:
    call = registry_lib.extract_repo_rule_call(REPO_RULE_SAMPLE, "libnl3_src")
    value = registry_lib.extract_single_item_list_kwarg(call, "urls")
    return check(value == "http://example.com/libnl3.tar.gz", "extract_single_item_list_kwarg extracts the one element")


def test_extract_single_item_list_kwarg_raises_on_non_string_value() -> bool:
    try:
        registry_lib.extract_single_item_list_kwarg("urls = [SOME_CONSTANT]", "urls")
    except ValueError:
        return check(True, "extract_single_item_list_kwarg raises ValueError for a non-string value")
    return check(False, "extract_single_item_list_kwarg raises ValueError for a non-string value")


def test_load_submodule_paths_and_urls_read_gitmodules() -> bool:
    gitmodules_text = (
        '[submodule "foo"]\n'
        "\tpath = src/foo\n"
        "\turl = https://example.com/foo\n"
    )
    with tempfile.TemporaryDirectory() as tmp:
        repo_root = Path(tmp)
        (repo_root / ".gitmodules").write_text(gitmodules_text)
        original_repo_root = registry_lib.REPO_ROOT
        registry_lib.REPO_ROOT = repo_root
        try:
            paths = registry_lib.load_submodule_paths()
            urls = registry_lib.load_submodule_urls()
        finally:
            registry_lib.REPO_ROOT = original_repo_root
    return check(
        paths == {"src/foo"} and urls == {"src/foo": "https://example.com/foo"},
        "load_submodule_paths/load_submodule_urls read .gitmodules correctly",
    )


def test_load_submodule_paths_empty_without_gitmodules() -> bool:
    with tempfile.TemporaryDirectory() as tmp:
        original_repo_root = registry_lib.REPO_ROOT
        registry_lib.REPO_ROOT = Path(tmp)
        try:
            paths = registry_lib.load_submodule_paths()
            urls = registry_lib.load_submodule_urls()
        finally:
            registry_lib.REPO_ROOT = original_repo_root
    return check(paths == set() and urls == {}, "load_submodule_paths/urls return empty without a .gitmodules file")


def test_load_submodule_paths_and_urls_skip_malformed_sections() -> bool:
    # A section missing `path` is meaningless to us and should be droppe entirely.
    # A section with `path` but no `url` is fine for paths (used by is_git_submodule) but excluded from urls (which needs both).
    gitmodules_text = (
        '[submodule "has-url-no-path"]\n'
        "\turl = https://example.com/no-path\n"
        "\n"
        '[submodule "has-path-no-url"]\n'
        "\tpath = src/no-url\n"
    )
    with tempfile.TemporaryDirectory() as tmp:
        repo_root = Path(tmp)
        (repo_root / ".gitmodules").write_text(gitmodules_text)
        original_repo_root = registry_lib.REPO_ROOT
        registry_lib.REPO_ROOT = repo_root
        try:
            paths = registry_lib.load_submodule_paths()
            urls = registry_lib.load_submodule_urls()
        finally:
            registry_lib.REPO_ROOT = original_repo_root
    return check(
        paths == {"src/no-url"} and urls == {},
        "load_submodule_paths/urls gracefully skip sections missing path/url instead of crashing",
    )


TESTS = [
    test_extract_module_call_finds_declaration,
    test_extract_module_call_returns_none_without_module,
    test_extract_module_call_handles_nested_parens,
    test_parse_module_declaration_reads_name_and_version,
    test_parse_module_declaration_defaults_version,
    test_parse_module_declaration_none_without_module,
    test_rewrite_module_version_replaces_existing,
    test_rewrite_module_version_preserves_rest_of_file,
    test_rewrite_module_version_adds_missing_version,
    test_is_git_submodule_matches_exact_and_nested_paths,
    test_submodule_root_for_returns_matching_root_or_none,
    test_extract_repo_rule_call_finds_invocation_not_assignment,
    test_extract_repo_rule_call_raises_on_unbalanced_parens,
    test_extract_repo_rule_call_raises_when_rule_absent,
    test_extract_str_kwarg_extracts_value,
    test_extract_str_kwarg_raises_when_missing,
    test_extract_str_kwarg_raises_on_non_string_value,
    test_extract_single_item_list_kwarg_extracts_value,
    test_extract_single_item_list_kwarg_raises_on_multiple_items,
    test_extract_single_item_list_kwarg_raises_on_empty_list,
    test_extract_single_item_list_kwarg_raises_on_non_string_value,
    test_load_submodule_paths_and_urls_read_gitmodules,
    test_load_submodule_paths_empty_without_gitmodules,
    test_load_submodule_paths_and_urls_skip_malformed_sections,
]


def main() -> None:
    results = [test() for test in TESTS]
    if not all(results):
        sys.exit(1)
    print(f"All {len(results)} tests passed.")


if __name__ == "__main__":
    main()
