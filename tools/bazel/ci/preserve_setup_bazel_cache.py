#!/usr/bin/env python3
"""Forward selected local caches and explicitly enabled CI remote services."""

import argparse
from pathlib import Path
import shlex


def preserve(workspace, home, repository_only=False, private_remote_cache=False):
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
    selected = list(settings.values())
    if private_remote_cache:
        selected.extend([
            "common --remote_cache=grpc://172.16.0.4:9092",
            "common --experimental_remote_downloader=grpc://172.16.0.4:9092",
            "common --experimental_remote_downloader_local_fallback",
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
