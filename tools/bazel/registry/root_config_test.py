"""Verifies that tools/bazel/root-unpinned-modules-config.bazelrc is complete and up to date.

sonic-buildimage always builds these src/ modules from the local source tree,
regardless of what version any consumer's bazel_dep declares.
This test suite makes sure that the list of modules is always complete.

Assumes that the module name (declared in MODULE.bazel) is the same as the subdirectory name.

Run with --fix (note `bazel run`, not `bazel test`) to regenerate the file from scratch:
    bazel run //tools/bazel/registry:root_config_test -- --fix
"""

import argparse
import sys

import registry_lib

ROOT_CONFIG = registry_lib.REPO_ROOT / "tools/bazel/root-unpinned-modules-config.bazelrc"

CONFIG_NAME = "local-modules"

HEADER = """\
# ==============================================================================
# THIS FILE IS AUTO-GENERATED. Do not hand-edit it. To re-generate, run:
#
#     bazel run //tools/bazel/registry:root_config_test -- --fix
#
# ==============================================================================
#
# Imported unconditionally by sonic-buildimage's own .bazelrc.
# Always builds these src/ modules from the checked-out tree,
# regardless of what version any consumer's bazel_dep declares.
#
# This bypasses registry lookup and version-string resolution entirely,
# regardless of what any of the submodule's MODULE.bazel declare.
#
# Assumes that the module name (declared in MODULE.bazel) is the same as the subdirectory name.
"""


def render_entry(name: str, src_path: str) -> str:
    """Render the override_module entry for one module."""
    return f"common:{CONFIG_NAME} --override_module={name}=%workspace%/{src_path}\n"

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--fix",
        action="store_true",
        help="Regenerate root-unpinned-modules-config.bazelrc from scratch.",
    )
    args = parser.parse_args()

    modules = registry_lib.discover_top_level_bazel_modules()
    updated = HEADER + "".join(render_entry(name, src_path) for name, src_path in modules)

    if ROOT_CONFIG.exists() and ROOT_CONFIG.read_text() == updated:
        print(f"Nothing to do: {ROOT_CONFIG} is already up to date.")
        return

    if args.fix:
        ROOT_CONFIG.write_text(updated)
        print(f"Regenerated {ROOT_CONFIG}")
        return

    # Else, the file was out of date (or didn't exist).
    print(f"FAIL: {ROOT_CONFIG.relative_to(registry_lib.REPO_ROOT)} is incomplete or out of date.")
    print()
    print("To fix, run:")
    print("    bazel run //tools/bazel/registry:root_config_test -- --fix")
    sys.exit(1)


if __name__ == "__main__":
    main()
