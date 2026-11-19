#!/usr/bin/env python3
"""Check the release Tag in the Docker archives exported by the native VS batch."""

import argparse
import hashlib
import json
from pathlib import Path
import sys
import tarfile

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci.artifact_validation import path_name, require
from tools.bazel.ci.vs_archive_reuse import CONFIG_NAME, MAX_JSON


def batch_archives(timings):
    # A Make retry can leave a failed invocation in the same timing directory.
    completed = []
    for path in (timings / "vs-container-images").glob("recipe-*/build.timing.json"):
        timing = json.loads(path.read_bytes())
        if timing["exit_code"] == 0:
            completed.append((timing["start_epoch_seconds"], path.parent))
    require(completed, "Missing successful VS container batch in " + str(timings))
    _, invocation = max(completed)
    targets = json.loads((invocation / "batch-targets.json").read_bytes())
    require(isinstance(targets, dict) and targets, "Empty or invalid VS container batch")
    require(all(Path(name).name == name and name.endswith(".gz") for name in targets),
            "Invalid archive name in VS container batch")
    return sorted(targets)


def archive_tag(path):
    # The pinned rules_oci Docker exporter emits the config blob before its
    # layers. Stop there to avoid decompressing or hashing their payloads.
    with tarfile.open(path, "r|gz") as stream:
        member = next((entry for entry in stream if not entry.isdir()), None)
        require(member is not None and member.isfile() and CONFIG_NAME.fullmatch(path_name(member.name))
                and 0 < member.size <= MAX_JSON, f"{path.name}: expected a leading Docker config blob")
        raw = stream.extractfile(member).read()
    require(hashlib.sha256(raw).hexdigest() == Path(member.name).name.removesuffix(".json"),
            f"{path.name}: Docker config does not match its digest")
    try:
        settings = json.loads(raw)["config"]
    except (KeyError, TypeError, ValueError) as error:
        raise ValueError(f"{path.name}: invalid leading Docker config") from error
    require(isinstance(settings, dict), f"{path.name}: invalid Docker config")
    labels = settings.get("Labels") or {}
    require(isinstance(labels, dict), f"{path.name}: invalid Docker labels")
    return labels.get("Tag")


def verify(target, version_receipt, timings):
    version = json.loads(version_receipt.read_bytes())["build_version"]
    require(isinstance(version, str) and version, "Missing installer build_version")
    names = batch_archives(timings)
    for name in names:
        value = archive_tag(target / name)
        require(value == version, f"{name}: expected Tag={version!r}, found {value!r}")
    return len(names), version


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", type=Path, required=True)
    parser.add_argument("--version", type=Path, required=True, help="Validated installer version JSON")
    parser.add_argument("--timings", type=Path, required=True)
    args = parser.parse_args()
    try:
        count, version = verify(args.target, args.version, args.timings)
    except (OSError, KeyError, ValueError, tarfile.TarError, EOFError) as error:
        parser.exit(1, f"VS container Tag check failed: {error}\n")
    print(f"Verified Tag={version} in all {count} exported VS container archives.")


if __name__ == "__main__":
    main()
