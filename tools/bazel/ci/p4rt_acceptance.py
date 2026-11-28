#!/usr/bin/env python3
"""Check the downloaded P4RT evidence and installed probes against this checkout."""

import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import p4rt_receipt_semantics
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


def verify_selected_modules(graph, required):
    """Check required modules; Bazel resolves versions from MODULE.bazel."""
    modules = resolved_modules(graph)
    for name in required:
        found = modules.get(name, set())
        require(len(found) == 1 and all(found),
                f"P4RT requires one resolved version of {name}, resolved {sorted(found)}")


def verify(directory, contract, source, owner):
    require(__debug__, "P4RT acceptance requires assertions enabled")
    require(owner == contract["owner"], "Selected PINS gitlink changed")

    def read(name):
        return json.loads((directory / name).read_bytes())

    targets = contract["test_targets"]
    require(len(targets) == len(set(targets)) == 1, "Expected unified P4RT runtime package test")
    p4rt_receipt_semantics.verify(
        read, source=source, owner=owner, outputs=contract["outputs"], test_targets=targets,
    )
    audit = read("action-audit.json")
    require(audit["deb_outputs"] == audit["packaging_wrappers"] == [] and
            len(audit["targets"]) == len(set(audit["targets"])) == 26 and
            set(audit["targets"]) == set(contract["audit_labels"]), "Image no-DEB scope changed")
    summary = read("build-summary.json")
    build = summary["build-events.jsonl"]
    require(build["finished"] == {"success": True, "exit_code": 0} and build["bazel_version"] == "8.5.1"
            and build["truncated"] is False and build["tests"] == {}, "Image build summary differs")
    names = {label.replace("@@sonic-sairedis+//", "@sonic_sairedis//", 1).replace("@@sonic-pins+//", "@sonic_pins//", 1)
             for label in build["targets"]}
    require(len(build["targets"]) == 23 and set(build["targets"].values()) == {True}
            and names == set(contract["outputs"].values()), "Image build output inventory changed")
    owner_tests = summary["owner-test-events.jsonl"]
    require(owner_tests["finished"] == {"success": True, "exit_code": 0}
            and owner_tests["bazel_version"] == "8.5.1" and owner_tests["truncated"] is False
            and set(owner_tests["tests"]) == set(targets)
            and all(entry.get("status") == "PASSED" for entry in owner_tests["tests"].values()),
            "Owner test build-event summary differs")
    verify_selected_modules(read("module-graph.json"), contract["required_modules"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    inputs = parser.add_mutually_exclusive_group(required=True)
    inputs.add_argument("--directory", type=Path, help="Directory with the full P4RT acceptance evidence")
    inputs.add_argument("--module-graph", type=Path, help="Check required P4RT modules in the resolved Bazel graph")
    args = parser.parse_args()
    contract = json.loads(Path(__file__).with_suffix(".json").read_text())
    if args.module_graph:
        verify_selected_modules(json.loads(args.module_graph.read_bytes()), contract["required_modules"])
    else:
        revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
        owner = subprocess.check_output(["git", "rev-parse", "HEAD:src/sonic-p4rt/sonic-pins"], cwd=ROOT, text=True).strip()
        verify(args.directory, contract, revision, owner)
