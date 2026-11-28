#!/usr/bin/env python3
"""Bind receipts to GitHub's immutable checkout and actual merge parents.

The pull_request event contract defines GITHUB_SHA as the tested merge commit:
https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows#pull_request
The event base snapshot remains in the log; receipts use the pinned merge's base.
"""

import argparse
import json
from pathlib import Path
import subprocess


def identity(expected_revision, event_name, head_sha="", event_base_sha=""):
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()
    if revision != expected_revision:
        raise ValueError("checkout differs from the requested GitHub revision")
    headers = subprocess.check_output(["git", "cat-file", "-p", "HEAD"], text=True).split("\n\n", 1)[0]
    parents = [line.split()[1] for line in headers.splitlines() if line.startswith("parent ")]
    base_sha = ""
    if event_name == "pull_request":
        if not head_sha or not event_base_sha:
            raise ValueError("pull request identity requires its event head and base")
        if len(parents) != 2 or parents[1] != head_sha:
            raise ValueError("GitHub merge checkout does not contain the requested PR head")
        # GITHUB_SHA identifies the tested merge. Its first parent is the tested
        # base; pull_request.base.sha can still describe an earlier PR snapshot.
        base_sha = parents[0]
    elif head_sha or event_base_sha:
        raise ValueError("PR identity supplied for a non-PR checkout")
    return {"revision": revision, "parents": parents, "head_sha": head_sha,
            "base_sha": base_sha, "event_base_sha": event_base_sha,
            "event_name": event_name}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--expected-revision", required=True)
    parser.add_argument("--event-name", required=True)
    parser.add_argument("--head-sha", default="")
    parser.add_argument("--event-base-sha", default="")
    parser.add_argument("--github-output", required=True, type=Path)
    args = parser.parse_args()
    result = identity(args.expected_revision, args.event_name, args.head_sha, args.event_base_sha)
    # Only publish outputs after the immutable revision and PR parent checks pass.
    with args.github_output.open("a") as output:
        for key in ("revision", "head_sha", "base_sha"):
            output.write(key + "=" + result[key] + "\n")
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
