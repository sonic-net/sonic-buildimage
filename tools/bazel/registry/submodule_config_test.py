"""Verifies that tools/bazel/submodule-config.bazelrc is complete and up to date. 

Run with --fix (note `bazel run`, not `bazel test`) to regenerate the file from scratch:
    bazel run //tools/bazel/registry:submodule_config_test -- --fix
"""

import argparse
import sys

import registry_lib

SUBMODULE_CONFIG = registry_lib.REPO_ROOT / "tools/bazel/submodule-config.bazelrc"


def find_submodule_bazel_modules() -> list[tuple[str, str]]:
    """Return (src_path, module_name) for each src/ module."""
    return sorted(
        (src_path, name) for name, src_path in registry_lib.discover_top_level_bazel_modules()
    )


# submodule-config.bazelrc is only ever imported by a submodule directly under src/
# (e.g. src/sonic-swss-common), so %workspace% is always 2 levels below the repo root
# when these lines are evaluated.
OVERRIDE_MODULE_DOTS = "../../.."


def render_entry(submodule_path: str, name: str) -> str:
    """Render a `common:unpinned-<name>` stanza overriding `name` with `submodule_path`."""
    return f"""
# Override {name} with a local checkout of {submodule_path}.
common:unpinned-{name} --override_module={name}=%workspace%/{OVERRIDE_MODULE_DOTS}/{submodule_path}
"""

HEADER = """\
# ==============================================================================
# THIS FILE IS AUTO-GENERATED. Do not hand-edit it. To re-generate, run:
#
#     bazel run //tools/bazel/registry:submodule_config_test -- --fix
#
# ==============================================================================
#
# Config file that will be imported by submodules (e.g. src/sonic-swss-common)
# when running under a sonic-buildimage checkout.
# Not to be used by sonic-buildimage directly.
#
# All paths are relative to the root of the submodule that imports this file (%workspace%),
# NOT relative to the module being overridden
#
# For instance, if this file is imported by `sonic-swss-common`, `%workspace%` will be
# `src/sonic-swss-common`, which is 2 levels below the repo root.
#
# This file currently assumes it is only ever imported by a submodule directly under src/
# (2 levels deep, e.g. `src/sonic-swss-common`). Importing it from a more deeply nested submodule
# (e.g. `src/sonic-sysmgr/gnoi`) would need a different `..` count and is not currently supported.
#
# Each override lives under its own --config, named after the module it overrides.
# We cannot just have a big list of modules and override them all with one config,
# because Bazel rejects overriding whichever module is currently root.
#
# For instance, if we had `sonic-swss-common` in a unified list, Bazel would crash if we were building in `soinc-swss-common`.
"""

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--fix",
        action="store_true",
        help="Regenerate submodule-config.bazelrc from scratch.",
    )
    args = parser.parse_args()

    modules = find_submodule_bazel_modules()
    updated = HEADER + "".join(render_entry(path, name) for path, name in modules)

    if SUBMODULE_CONFIG.exists() and SUBMODULE_CONFIG.read_text() == updated:
        print(f"Nothing to do: {SUBMODULE_CONFIG} is already up to date.")
        return

    if args.fix:
        SUBMODULE_CONFIG.write_text(updated)
        print(f"Regenerated {SUBMODULE_CONFIG}")
        return

    # Else, the file was out of date (or didn't exist).
    print(f"FAIL: {SUBMODULE_CONFIG.relative_to(registry_lib.REPO_ROOT)} is incomplete or out of date.")
    print()
    print("To fix, run:")
    print("    bazel run //tools/bazel/registry:submodule_config_test -- --fix")
    sys.exit(1)


if __name__ == "__main__":
    main()
