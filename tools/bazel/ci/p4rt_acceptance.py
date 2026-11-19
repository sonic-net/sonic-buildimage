#!/usr/bin/env python3
"""Check the downloaded P4RT evidence and installed probes against this checkout."""

import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import p4rt_receipt_semantics, p4rt_sdk_receipt_semantics
from tools.bazel.ci.artifact_validation import require


def resolved_modules(graph):
    selected = {}

    def walk(node):
        if isinstance(node, dict):
            if isinstance(node.get("name"), str) and isinstance(node.get("version"), str):
                selected.setdefault(node["name"], set()).add(node["version"])
            for child in node.values():
                walk(child)
        elif isinstance(node, list):
            for child in node:
                walk(child)

    walk(graph)
    return selected


def verify(directory, contract, source, owner):
    require(__debug__, "P4RT acceptance requires assertions enabled")
    require(owner == contract["owner"], "Selected PINS gitlink changed")

    def raw(name):
        return (directory / name).read_bytes()

    def read(name):
        return json.loads(raw(name))

    targets = contract["test_targets"]
    require(len(targets) == len(set(targets)) == 59, "The original 59 owner test targets changed")
    p4rt_receipt_semantics.verify(read, raw, source=source, owner=owner, outputs=contract["outputs"],
                                  test_targets=targets, owner_test_helper_sha256=contract["owner_test_helper_sha256"])
    p4rt_sdk_receipt_semantics.verify(read, source=source, owner=owner,
                                      preparation_targets=contract["preparation_outputs"],
                                      common_revision=contract["common_revision"])
    audit = read("action-audit.json")
    require(audit["deb_outputs"] == audit["packaging_wrappers"] == [] and
            len(audit["targets"]) == len(set(audit["targets"])) == 31 and
            set(audit["targets"]) == set(contract["audit_labels"]), "Image no-DEB scope changed")
    build = read("build-summary.json")["build-events.jsonl"]
    require(build["finished"] == {"success": True, "exit_code": 0} and build["bazel_version"] == "8.5.1"
            and build["truncated"] is False and build["tests"] == {}, "Image build summary differs")
    names = {label.replace("@@sonic-sairedis+//", "@sonic_sairedis//", 1) for label in build["targets"]}
    require(len(build["targets"]) == 27 and set(build["targets"].values()) == {True}
            and names == set(contract["outputs"].values()), "Image build output inventory changed")
    modules = resolved_modules(read("module-graph.json"))
    require(all(modules.get(name) == {version} for name, version in contract["selected_modules"].items()),
            "Resolved dependency selection changed")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=Path, required=True)
    args = parser.parse_args()
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    owner = subprocess.check_output(["git", "rev-parse", "HEAD:src/sonic-p4rt/sonic-pins"], cwd=ROOT, text=True).strip()
    verify(args.directory, json.loads(Path(__file__).with_suffix(".json").read_text()), revision, owner)
