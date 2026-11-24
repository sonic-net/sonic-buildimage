#!/usr/bin/env python3
"""Remove stopped VS cache users only after their GitHub attempt has completed."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import urllib.request


LABEL = "io.sonic-buildimage.ci-job"
STOPPED = {"exited", "dead"}


class NoRedirects(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, response, code, message, headers, url):
        return None


def completed(repository, run_id, attempt):
    request = urllib.request.Request(
        f"https://api.github.com/repos/{repository}/actions/runs/{run_id}/attempts/{attempt}",
        headers={"Authorization": "Bearer " + os.environ["GITHUB_TOKEN"],
                 "Accept": "application/vnd.github+json"})
    with urllib.request.build_opener(NoRedirects()).open(request, timeout=30) as response:
        record = json.load(response)
    return (record.get("id") == int(run_id) and record.get("run_attempt") == int(attempt)
            and record.get("status") == "completed")


def inspect(container):
    records = json.loads(subprocess.check_output(
        ["docker", "inspect", container], text=True, timeout=30))
    if len(records) != 1 or records[0].get("Id") != container:
        raise ValueError("Docker returned an unexpected container identity")
    return records[0]


def cache_user(record, cache, repository):
    state = record.get("State", {})
    if state.get("Status") not in STOPPED or state.get("Running") is not False:
        return None
    label = record.get("Config", {}).get("Labels", {}).get(LABEL, "")
    match = re.fullmatch(re.escape(repository) + r":([1-9][0-9]*):([1-9][0-9]*):vs", label)
    if not match:
        return None
    for mount in record.get("Mounts", []):
        source = mount.get("Source")
        if mount.get("Type") != "bind" or not source:
            continue
        source = Path(source)
        if source.is_absolute() and source.resolve().is_relative_to(cache):
            return match.groups()
    return None


def cleanup(workspace, repository, run_id, attempt):
    if (not re.fullmatch(r"[A-Za-z0-9._-]+/[A-Za-z0-9._-]+", repository)
            or not all(re.fullmatch(r"[1-9][0-9]*", value) for value in (run_id, attempt))
            or not workspace.is_absolute() or not workspace.is_dir()):
        raise ValueError("Cleanup requires a repository, numeric run identity and absolute workspace")
    cache = workspace.resolve(strict=True) / "vs-cache"
    if cache.is_symlink() or cache.resolve() != cache:
        raise ValueError("Refusing redirected VS cache")
    containers = subprocess.check_output(
        ["docker", "ps", "--all", "--quiet", "--no-trunc", "--filter", f"label={LABEL}"],
        text=True, timeout=30).split()
    verified = {}
    removed = 0
    for container in containers:
        if not re.fullmatch(r"[0-9a-f]{64}", container):
            raise ValueError("Docker returned an invalid container ID")
        record = inspect(container)
        owner = cache_user(record, cache, repository)
        if owner is None or owner == (run_id, attempt):
            continue
        if owner not in verified:
            verified[owner] = completed(repository, *owner)
        if not verified[owner]:
            print(f"Preserving container {container}: owning attempt is not completed")
            continue
        # Recheck after the API request; non-force removal also rejects a restarted container.
        current = inspect(container)
        if cache_user(current, cache, repository) != owner:
            raise ValueError(f"Container changed while verifying its owning attempt: {container}")
        print(f"Removing stopped VS container {container} from completed attempt {owner[0]}/{owner[1]}",
              flush=True)
        subprocess.run(["docker", "rm", container], check=True, timeout=30)
        removed += 1
    print(f"Removed {removed} verified stale VS containers")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner-workspace", type=Path, required=True)
    args = parser.parse_args()
    try:
        cleanup(args.runner_workspace, os.environ["GITHUB_REPOSITORY"],
                os.environ["GITHUB_RUN_ID"], os.environ["GITHUB_RUN_ATTEMPT"])
    except (KeyError, OSError, ValueError, subprocess.SubprocessError) as error:
        parser.exit(1, f"Stale VS container cleanup failed: {error}\n")


if __name__ == "__main__":
    main()
