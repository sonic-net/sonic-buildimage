"""Compare the published VS build selection with the release packaged in the installer."""

import argparse
import json
import re
from pathlib import Path

from tools.bazel.ci.vs_archive_reuse import installer_selection


def verify(workspace, revision, debug_enabled):
    evidence = workspace / "artifacts/vs"
    release = json.loads((evidence / "sonic-version.json").read_bytes())["build_version"]
    selection = json.loads((evidence / "containers/selection.json").read_bytes())
    if selection["revision"] != revision or selection["build_version"] != release:
        raise ValueError("VS container batch and installer release do not match")
    if selection["architecture"] != "amd64" or selection["debug_enabled"] is not debug_enabled:
        raise ValueError("VS container batch and requested build profile do not match")
    if not isinstance(release, str) or not re.fullmatch(r"[a-zA-Z0-9_][a-zA-Z0-9_.-]*", release):
        raise ValueError("Invalid installer release label")
    # The VS installer selects runtime images even when the build exports
    # additional debug companions. Make records its installation choices here.
    installed = installer_selection(workspace)
    batch = set(selection["installer_archives"])
    if not all(Path(image).name in batch for image in installed["paired_images"]):
        raise ValueError("The VS batch did not export the selected Orchagent/Syncd runtime archives")
    return release


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workspace", required=True, type=Path)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--debug-layer", required=True, choices=("y", "n"))
    parser.add_argument("--github-output", required=True, type=Path)
    args = parser.parse_args()
    release = verify(args.workspace, args.revision, args.debug_layer == "y")
    with args.github_output.open("a") as output:
        output.write(f"build_version={release}\n")


if __name__ == "__main__":
    main()
