"""Summarize Bazel recipes in the VS installer build using JSON trace profiles."""

import argparse
from bisect import bisect_right
from collections import defaultdict
import gzip
import json
import math
import re
from operator import itemgetter
from pathlib import Path


PUBLIC_NAME = re.compile(r"[A-Za-z0-9][A-Za-z0-9_.+-]{0,191}\Z")
COMPILERS = {"CppCompile", "CppModuleCompile", "Rustc", "GoCompilePkg", "GoCompile"}
CATEGORIES = ("lock", "compile", "foreign", "fetch")
LOCK_EVENTS = {
    "Blocking on busy Bazel server (in client)",
    "Blocking on busy Bazel server (in server)",
}


def union_seconds(intervals):
    """Return the observed wall time, counting active threads only once."""
    total = 0
    previous_end = -math.inf
    for start, end in sorted(intervals):
        total += max(0, end - max(previous_end, start))
        previous_end = max(previous_end, end)
    return total / 1_000_000  # Bazel trace timestamps are in microseconds.


def summarize_profile(events):
    spans = {key: [] for key in CATEGORIES}
    actions = defaultdict(list)
    sandbox_processes = defaultdict(list)
    for event in events:
        if event.get("ph") != "X":
            continue
        start = event["ts"]
        interval = (start, start + event["dur"])
        name = event["name"]
        category = event.get("cat")
        thread = (event["pid"], event["tid"])
        if category == "action processing":
            actions[thread].append((*interval, event.get("args", {}).get("mnemonic", "")))
        elif category == "general information" and name == "subprocess.run":
            sandbox_processes[thread].append(interval)
        elif category == "Local execution process wall time":
            add_process(spans, name, interval)
        elif category == "build phase marker" and name in LOCK_EVENTS:
            spans["lock"].append(interval)
        elif category == "remote output download" and name.lower() in {
            "download outputs", "stage remote inputs",
        }:
            spans["fetch"].append(interval)

    # Sandboxed subprocess.run events carry no mnemonic. Their enclosing action on
    # the same thread supplies it. A cache hit has no compiler subprocess.
    for thread, processes in sandbox_processes.items():
        parents = sorted(actions[thread], key=itemgetter(0))
        for start, end in processes:
            index = bisect_right(parents, start, key=itemgetter(0)) - 1
            if index >= 0 and end <= parents[index][1]:
                add_process(spans, parents[index][2], (start, end))
    return {key: union_seconds(intervals) for key, intervals in spans.items()}


def add_process(spans, mnemonic, interval):
    if mnemonic in COMPILERS:
        spans["compile"].append(interval)
    # rules_foreign_cc constructs Cc<configure_name.capitalize()>MakeRule.
    elif mnemonic == "SonicKernelBuild" or (mnemonic.startswith("Cc") and mnemonic.endswith("MakeRule")):
        spans["foreign"].append(interval)


def measured(seconds, complete):
    if seconds is None:
        return "unavailable"
    display = "<0.01" if 0 < seconds < 0.01 else f"{seconds:.2f}"
    return display if complete else display + " (partial)"


def legacy_plan(owner):
    # Earlier image artifacts recorded one runtime/debug pair directly under the owner.
    archives = (owner, owner.removesuffix(".gz") + "-dbg.gz")
    commands = ["build", *("cquery-" + archive for archive in archives)]
    return {"bazel": commands, "profiles": commands, "exports": ["export-" + archive for archive in archives]}


def read_plan(path):
    plan = json.loads(path.read_text())
    if (not isinstance(plan, dict) or set(plan) != {"bazel", "profiles", "exports"}
            or any(not isinstance(names, list)
                   or any(not isinstance(name, str) or not PUBLIC_NAME.fullmatch(name) for name in names)
                   or len(set(names)) != len(names) for names in plan.values())
            or set(plan["bazel"]).intersection(plan["exports"])):
        raise ValueError(f"Invalid phase list in {path}")
    return plan


def read_profile(path):
    with gzip.open(path, "rt") as stream:
        events = json.load(stream)["traceEvents"]
    if not isinstance(events, list) or not events:
        raise ValueError("empty trace")
    return summarize_profile(events)


def read_owner(directory):
    row = {"owner": directory.name, "bazel_seconds": None, "export_seconds": None,
           **{key + "_seconds": None for key in CATEGORIES}, "failed": False,
           "bazel_phases": 0, "bazel_expected": 0, "profiles": 0, "profiles_expected": 0,
           "exports": 0, "exports_expected": 0}
    problems = []
    plans = [(path.parent, read_plan(path)) for path in sorted(directory.glob("*/recipe.json"))]
    if not plans and directory.name.endswith(".gz") and any(directory.glob("*.timing.json")):
        plans.append((directory, legacy_plan(directory.name)))
    if not plans:
        return None, []

    for instance, plan in plans:
        for group, count, expected, metric in (
                ("bazel", "bazel_phases", "bazel_expected", "bazel_seconds"),
                ("exports", "exports", "exports_expected", "export_seconds")):
            row[expected] += len(plan[group])
            for phase in plan[group]:
                try:
                    timing = json.loads((instance / f"{phase}.timing.json").read_text())
                    seconds, code = timing["elapsed_seconds"], timing["exit_code"]
                    if (timing["phase"] != phase or type(seconds) not in (float, int)
                            or not math.isfinite(seconds) or seconds < 0
                            or type(code) is not int or not 0 <= code <= 255):
                        raise ValueError("invalid timing")
                except (OSError, ValueError, KeyError, TypeError):
                    problems.append(f"{phase} timing missing or unreadable")
                    continue
                row[count] += 1
                row[metric] = (row[metric] or 0) + seconds
                if code:
                    row["failed"] = True
                    problems.append(f"{phase} exited {code}")
        row["profiles_expected"] += len(plan["profiles"])
        for phase in plan["profiles"]:
            try:
                durations = read_profile(instance / f"{phase}.profile.json.gz")
            except (OSError, EOFError, ValueError, KeyError, TypeError, AttributeError):
                problems.append(f"{phase} profile missing or unreadable")
                continue
            row["profiles"] += 1
            for key, seconds in durations.items():
                metric = key + "_seconds"
                row[metric] = (row[metric] or 0) + seconds
    return row, problems


def markdown_row(row):
    return [row["owner"], measured(row["bazel_seconds"], row["bazel_phases"] == row["bazel_expected"]),
            *(measured(row[key + "_seconds"], row["profiles"] == row["profiles_expected"]) for key in CATEGORIES),
            measured(row["export_seconds"], row["exports"] == row["exports_expected"])
            if row["exports_expected"] else "n/a"]


def report(root):
    recipes = []
    notes = []
    for directory in sorted(root.iterdir()):
        if directory.is_dir() and PUBLIC_NAME.fullmatch(directory.name):
            row, problems = read_owner(directory)
            if row is not None:
                recipes.append(row)
            if problems:
                notes.append(f"- `{directory.name}` — " + ", ".join(problems))
    recipes.sort(key=lambda row: (
        row["bazel_phases"] == row["exports"] == 0,
        -((row["bazel_seconds"] or 0) + (row["export_seconds"] or 0)),
        row["owner"],
    ))
    lines = ["### VS Bazel build timings (wall seconds)", ""]
    if recipes:
        lines.extend(["| Make target or package | Bazel / launcher | Bazel lock wait | Compiler subprocesses | "
                      "Nested build subprocesses | Cache input/output fetch | Export |", "|---|---:|---:|---:|---:|---:|---:|"])
        lines.extend("| " + " | ".join(markdown_row(row)) + " |" for row in recipes)
        lines.extend(["", "Command times and exports are summed per target. The kernel launcher includes bootstrap, "
                      "build, output query and module resolution. Nested build time covers an entire Make or "
                      "foreign build subprocess. Cache fetch includes both disk and remote reads. "
                      "Parallel activities can overlap. The Bazel total also includes work such as analysis and linking. "
                      "A Make cache hit can skip Bazel entirely. "
                      "Zero means no matching span was recorded."])
    else:
        lines.append("No Bazel recipes were recorded for this build.")
    if notes:
        lines.extend(["", "Incomplete measurements", "", *notes])
    return {"version": 2, "recipes": recipes}, "\n".join(lines) + "\n"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--dashboard-json", type=Path, help="Write numeric timings and coverage counts")
    parser.add_argument("--github-actions", action="store_true",
                        help="Save summary.md and summary.json and emit bounded dashboard records to the job log")
    args = parser.parse_args()
    receipt, markdown = report(args.directory)
    output = args.dashboard_json or (args.directory / "summary.json" if args.github_actions else None)
    if output is not None:
        output.write_text(json.dumps(receipt, allow_nan=False, separators=(",", ":")) + "\n")
    if args.github_actions:
        (args.directory / "summary.md").write_text(markdown)
        for recipe in receipt["recipes"]:
            print("SONIC_VS_BAZEL_TIMINGS_V2", json.dumps({"recipe": recipe}, allow_nan=False, separators=(",", ":")))
        print("SONIC_VS_BAZEL_TIMINGS_V2", json.dumps({"count": len(receipt["recipes"])}, separators=(",", ":")))
    else:
        print(markdown, end="")
