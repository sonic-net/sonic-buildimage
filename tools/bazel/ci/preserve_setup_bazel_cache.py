#!/usr/bin/env python3
"""Forward selected local caches and explicitly enabled CI remote services."""

import argparse
from pathlib import Path
import shlex


PRIVATE_CACHE_ENDPOINT = "grpc://172.16.0.4:9092"
PRIVATE_REMOTE_SETTINGS = (
    "common --remote_cache=" + PRIVATE_CACHE_ENDPOINT,
    "common --experimental_remote_downloader=" + PRIVATE_CACHE_ENDPOINT,
    "common --experimental_remote_downloader_local_fallback",
)


def setup_cache_settings(home, repository_only=False):
    prefixes = {"common --repository_cache=": "repository"}
    if not repository_only:
        prefixes["build --disk_cache="] = "disk"
    settings = {}
    for line in (home / ".bazelrc").read_text().splitlines():
        prefix = next((value for value in prefixes if line.startswith(value)), None)
        if prefix is None:
            continue
        kind = prefixes[prefix]
        tokens = shlex.split(line, comments=True)
        if (kind in settings or len(tokens) != 2 or
                not tokens[1].split("=", 1)[1]):
            raise ValueError("setup-bazel cache setting is duplicate or invalid")
        settings[kind] = line
    if set(settings) != set(prefixes.values()):
        raise ValueError("setup-bazel cache settings are missing")
    return list(settings.values())


def preserve(workspace, home, repository_only=False, private_remote_cache=False):
    selected = setup_cache_settings(home, repository_only)
    if private_remote_cache:
        selected.extend(PRIVATE_REMOTE_SETTINGS)
        selected.extend([
            # Local actions and their BEP files must survive a cache outage.
            "build --remote_local_fallback",
            "build --incompatible_remote_local_fallback_for_remote_cache",
            "build --experimental_build_event_upload_strategy=local",
        ])
    user = workspace / ".bazelrc.user"
    current = user.read_text() if user.exists() else ""
    separator = "\n" if current and not current.endswith("\n") else ""
    user.write_text(current + separator + "\n".join(selected) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository-only", action="store_true",
                        help="Keep disk caching disabled for repository-only CI jobs")
    parser.add_argument("--private-remote-cache", action="store_true",
                        help="Enable the private gRPC cache and downloader on cache-reachable agents")
    args = parser.parse_args()
    try:
        preserve(Path.cwd(), Path.home(), args.repository_only, args.private_remote_cache)
    except (OSError, ValueError) as error:
        parser.exit(1, "Cannot preserve setup-bazel caches: " + str(error) + "\n")


if __name__ == "__main__":
    main()
