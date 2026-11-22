"""Select component source suites using their existing event and path filters."""

import json
import os
from pathlib import Path
import re
import subprocess

import yaml


WORKFLOWS = ("bazel-eventd.yml", "bazel-frr.yml", "bazel-libteam.yml", "bazel-supervisor-rust.yml",
             "bazel-sysmgr.yml", "bazel-yang-models.yml")


def changed_paths(base):
    if not base or base == "0" * 40:
        return None
    if not re.fullmatch("[0-9a-f]{40}", base):
        raise ValueError("Invalid event base revision")
    if subprocess.run(["git", "cat-file", "-e", base + "^{commit}"], stderr=subprocess.DEVNULL).returncode:
        subprocess.run(["git", "fetch", "--depth=1", "--no-tags", "--no-recurse-submodules", "origin", base], check=True)
    diff = subprocess.check_output(["git", "diff", "--no-renames", "--name-only", "-z", base, "HEAD"])
    return [os.fsdecode(path) for path in diff.split(b"\0") if path]


def matches(pattern, path):
    # The owner workflows use literals, single-segment globs and directory/**.
    if pattern.endswith("/**"):
        return path.startswith(pattern[:-2])
    if any(c in pattern for c in "!?[") or "**" in pattern:
        raise ValueError("Unsupported component workflow path filter: " + pattern)
    return re.fullmatch(re.escape(pattern).replace(r"\*", "[^/]*"), path) is not None


def select(event, changes):
    result = []
    for name in WORKFLOWS:
        events = yaml.load((Path(".github/workflows") / name).read_text(), Loader=yaml.BaseLoader)["on"]
        if event not in events:
            continue
        config = events[event] or {}
        patterns = config.get("paths", [])
        if changes is None or not patterns or any(matches(p, path) for path in changes for p in patterns):
            result.append(name)
    return result


if __name__ == "__main__":
    event = os.environ["GITHUB_EVENT_NAME"]
    payload = json.loads(Path(os.environ["GITHUB_EVENT_PATH"]).read_bytes())
    base = payload["pull_request"]["base"]["sha"] if event == "pull_request" else payload.get("before")
    print(json.dumps(select(event, changed_paths(base)), separators=(",", ":")))
