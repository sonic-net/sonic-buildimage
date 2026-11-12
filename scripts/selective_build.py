#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path
from typing import Iterable


# Every platform build job group of azure-pipelines.yml, in the order they
# are reported.
ALL_PLATFORMS = (
    "vs",
    "vpp",
    "alpinevs",
    "broadcom",
    "mellanox",
    "marvell-prestera-arm64",
    "marvell-prestera-armhf",
    "nvidia-bluefield",
    "aspeed-arm64",
)

# Job groups of the BuildVS stage. They are selected when a path cannot be
# mapped to a platform (docs, a newly added platform directory, ...) or when
# no path changed, as the baseline coverage of a pull request.
DEFAULT_PLATFORMS = (
    "vs",
    "vpp",
    "alpinevs",
)

# Files every platform build depends on. A change to any of them builds all
# platforms. A path ending with "/" matches the whole directory.
GLOBAL_PATHS = (
    "Makefile",
    "Makefile.work",
    "slave.mk",
    "azure-pipelines.yml",
    ".azure-pipelines/",
    ".github/",
    "rules/",
)

# Source directories of each job group. marvell-prestera is shared by the
# ARM64 and the ARMHF job groups, so a change there builds both.
PLATFORM_PATHS = {
    "vs": ("platform/vs/",),
    "vpp": ("platform/vpp/",),
    "alpinevs": ("platform/alpinevs/",),
    "broadcom": ("platform/broadcom/",),
    "mellanox": ("platform/mellanox/",),
    "marvell-prestera-arm64": ("platform/marvell-prestera/",),
    "marvell-prestera-armhf": ("platform/marvell-prestera/",),
    "nvidia-bluefield": ("platform/nvidia-bluefield/",),
    "aspeed-arm64": ("platform/aspeed/",),
}


def path_matches(path: str, pattern: str) -> bool:
    # A pattern ending with "/" is a directory, anything else is a file name.
    if pattern.endswith("/"):
        directory = pattern.rstrip("/")
        return path == directory or path.startswith(pattern)

    return path == pattern


def all_platforms_result(
    changed_paths: list[str],
    mode: str,
    reason: str,
) -> dict:
    return {
        "mode": mode,
        "run_all": True,
        "run_vs_tests": True,
        "platforms": list(ALL_PLATFORMS),
        "changed_paths": changed_paths,
        "reasons": [reason],
    }


# Result for changes that cannot be mapped to a platform: build the BuildVS
# stage and run the VS tests instead of skipping the pull request.
def default_platforms_result(
    changed_paths: list[str],
    reason: str,
) -> dict:
    return {
        "mode": "path",
        "run_all": False,
        "run_vs_tests": True,
        "platforms": list(DEFAULT_PLATFORMS),
        "changed_paths": changed_paths,
        "reasons": [reason],
    }


def select_platforms(
    changed_paths: Iterable[str],
    mode: str = "path",
) -> dict:
    if mode not in {"path", "all"}:
        raise ValueError(f"Unsupported selection mode: {mode}")

    paths = sorted(
        {
            path.strip()
            for path in changed_paths
            if path and path.strip()
        }
    )

    if mode == "all":
        # Used for push, scheduled and manual builds, and for a full rerun.
        return all_platforms_result(
            paths,
            mode="all",
            reason="all-platform mode requested",
        )

    if not paths:
        return default_platforms_result(
            paths,
            reason="no changed paths detected",
        )

    selected: set[str] = set()
    reasons: list[str] = []
    run_vs_tests = False

    for path in paths:
        global_match = next(
            (
                pattern
                for pattern in GLOBAL_PATHS
                if path_matches(path, pattern)
            ),
            None,
        )

        # A global path decides the result at once, whatever the other paths
        # are.
        if global_match:
            reasons.append(
                f"{path} matched global path {global_match}"
            )
            return all_platforms_result(
                paths,
                mode="path",
                reason="global path detected: " + global_match,
            ) | {"reasons": reasons}

        matched_platforms = [
            platform
            for platform, patterns in PLATFORM_PATHS.items()
            if any(
                path_matches(path, pattern)
                for pattern in patterns
            )
        ]

        # Keep looking at the remaining paths: a later global path still
        # selects every platform.
        if not matched_platforms:
            reasons.append(
                f"{path} did not match a known platform path, "
                "BuildVS default selected"
            )
            selected.update(DEFAULT_PLATFORMS)
            run_vs_tests = True
            continue

        selected.update(matched_platforms)

        if "vs" in matched_platforms:
            run_vs_tests = True

        reasons.append(
            f"{path} selected "
            f"{', '.join(sorted(matched_platforms))}"
        )

    return {
        "mode": "path",
        "run_all": False,
        "run_vs_tests": run_vs_tests,
        "platforms": [
            platform
            for platform in ALL_PLATFORMS
            if platform in selected
        ],
        "changed_paths": paths,
        "reasons": reasons,
    }


def get_changed_paths(base: str, head: str) -> list[str]:
    # Rename detection is off so that both the old and the new path of a
    # moved file are reported: the platform losing the file needs a build too.
    result = subprocess.run(
        [
            "git",
            "diff",
            "--name-only",
            "--no-renames",
            "--diff-filter=ACMRD",
            f"{base}...{head}",
        ],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    return sorted(
        {
            line.strip()
            for line in result.stdout.splitlines()
            if line.strip()
        }
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Select SONiC platform jobs from changed paths."
    )
    parser.add_argument("--base", required=True)
    parser.add_argument("--head", required=True)
    parser.add_argument(
        "--mode",
        choices=("path", "all"),
        default="path",
    )
    parser.add_argument("--output", type=Path)

    args = parser.parse_args()

    changed_paths = get_changed_paths(
        args.base,
        args.head,
    )

    decision = select_platforms(
        changed_paths,
        mode=args.mode,
    )

    rendered = json.dumps(decision, indent=2)
    print(rendered)

    if args.output:
        args.output.write_text(
            rendered + "\n",
            encoding="utf-8",
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
