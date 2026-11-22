"""Analyze sanitized VS BEP events without Bazel or profile processing."""

import argparse
import json
from pathlib import Path
import re
import sys
import tempfile
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import public_artifacts as public


MAX_FILE_BYTES = 32 * 1024 * 1024
MAX_RECORD_CHARS = 16 * 1024 * 1024
TIMINGS = {
    "wallTimeInMs": "wall",
    "cpuTimeInMs": "cpu",
    "analysisPhaseTimeInMs": "analysis",
    "executionPhaseTimeInMs": "execution",
}


def count(value):
    if isinstance(value, str) and re.fullmatch(r"0|[1-9][0-9]*", value):
        value = int(value)
    return public.integer(value)


def action_metrics(value):
    result = {
        key: count(value[field])
        for field, key in (("actionsCreated", "created"),
                           ("actionsExecuted", "executed"))
        if field in value
    }
    runners = {}
    for item in value.get("runnerCount", []):
        name = item.get("name")
        if name is None:
            continue
        public.require(name not in runners, "duplicate-bep-runner")
        runners[name] = count(item.get("count", 0))
    result["runners"] = runners
    statistics = value.get("actionCacheStatistics", {})
    cache = {
        field: count(statistics[field])
        for field in ("hits", "misses") if field in statistics
    }
    if {"hits", "misses"} <= cache.keys():
        attempts = cache["hits"] + cache["misses"]
        cache["hit_rate"] = cache["hits"] / attempts if attempts else None
    result["local_action_cache"] = cache
    mnemonics = []
    for item in value.get("actionData", []):
        if "mnemonic" not in item:
            continue
        mnemonics.append({
            "name": item["mnemonic"],
            **{key: count(item[field])
               for field, key in (("actionsCreated", "created"),
                                  ("actionsExecuted", "executed"))
               if field in item},
        })
    result["mnemonics"] = sorted(
        mnemonics, key=lambda item: (-item.get("executed", 0), item["name"]))
    return result


def analyze(path):
    public.require(not path.is_symlink() and path.is_file(),
                   "invalid-bep-file")
    public.require(path.stat().st_size <= MAX_FILE_BYTES, "oversized-bep")
    metrics = {}
    metrics_seen = False
    terminal = False

    def lines():
        nonlocal metrics, metrics_seen, terminal
        with path.open(encoding="utf-8") as stream:
            while raw := stream.readline(MAX_RECORD_CHARS + 1):
                public.require(len(raw) <= MAX_RECORD_CHARS,
                               "oversized-bep-event")
                public.require(not terminal, "event-after-bep-terminal")
                event = public.load_json(raw)
                public.require(isinstance(event, dict), "invalid-bep-event")
                selected = public.public_bep_event(event)
                if selected is None:
                    continue
                if "buildMetrics" in selected["id"]:
                    public.require(not metrics_seen, "duplicate-bep-metrics")
                    metrics = selected["buildMetrics"]
                    metrics_seen = True
                terminal = selected.get("lastMessage", False)
                yield json.dumps(selected)

    summary = public.bep_summary(lines())
    finished = summary["finished"]
    complete = terminal and finished is not None
    timing = metrics.get("timingMetrics", {})
    result = {
        "status": ("passed" if finished["success"] else "failed")
        if complete else "incomplete",
        "stream_complete": complete,
        "terminal_marker_seen": terminal,
        "event_count": summary["event_count"],
        "command": summary["command"],
        "bazel_version": summary["bazel_version"],
        "build_result": finished,
        "timing_seconds": {
            name: count(timing[field]) / 1000
            for field, name in TIMINGS.items() if field in timing
        },
        "actions": action_metrics(metrics.get("actionSummary", {})),
        "targets": summary["targets"],
        "tests": summary["tests"],
    }
    public.safe_json(result)
    return result


def display(value):
    return "n/a" if value is None else str(value)


def markdown(report):
    lines = [
        "# Bazel BEP analysis", "",
        f"Analysis: {report['analysis_seconds']:.6f} seconds; "
        f"{len(report['streams'])} event streams.", "",
        "Each stream is one Bazel invocation. CPU, analysis and execution "
        "times can overlap; do not add them to wall time or across streams.",
        "Target completion events report success, not per-target duration.",
        "`actionsExecuted` includes cache-handled and internal actions. "
        "Local action-cache hits/misses and runner counts are separate "
        "counter domains; a local cache miss is not a remote cache miss.",
        "Missing metrics are n/a, not zero. Incomplete streams are not "
        "reported as passed.", "",
    ]
    for stream in report["streams"]:
        lines.extend([f"## `{stream['source']}`", "",
                      f"Status: **{stream['status']}**.", ""])
        if stream["status"] == "unavailable":
            lines.append(
                "Missing, oversized or invalid sanitized event stream.")
            lines.append("")
            continue
        lines.extend([
            f"Bazel: {display(stream['bazel_version'])}; "
            f"command: {display(stream['command'])}; "
            "exit code: "
            f"{display((stream['build_result'] or {}).get('exit_code'))}.",
            "", "| Timing | Seconds |", "| --- | ---: |",
        ])
        for name in TIMINGS.values():
            value = stream["timing_seconds"].get(name)
            lines.append(f"| {name} | "
                         f"{'n/a' if value is None else f'{value:.3f}'} |")
        actions = stream["actions"]
        cache = actions["local_action_cache"]
        rate = cache.get("hit_rate")
        lines.extend([
            "", f"Actions created: {display(actions.get('created'))}; "
            f"Bazel actionsExecuted: {display(actions.get('executed'))}.",
            f"Local action-cache hits: {display(cache.get('hits'))}; "
            f"misses: {display(cache.get('misses'))}; "
            f"hit rate: {'n/a' if rate is None else f'{rate:.1%}'}.",
            "", "| Runner | Count |", "| --- | ---: |",
        ])
        for name, value in sorted(actions["runners"].items()):
            lines.append(f"| {name} | {value} |")
        lines.extend(["", "| Mnemonic | Created | Bazel actionsExecuted |",
                      "| --- | ---: | ---: |"])
        for item in actions["mnemonics"]:
            lines.append(f"| {item['name']} | "
                         f"{display(item.get('created'))} | "
                         f"{display(item.get('executed'))} |")
        lines.extend(["", "| Target | Result |", "| --- | --- |"])
        for target, success in sorted(stream["targets"].items()):
            result = "passed" if success else "failed"
            lines.append(f"| `{target}` | {result} |")
        lines.extend(["", "| Test target | Status |", "| --- | --- |"])
        for target, test in sorted(stream["tests"].items()):
            lines.append(f"| `{target}` | {test['status']} |")
        lines.append("")
    text = "\n".join(lines) + "\n"
    public.safe_text(text)
    return text


def report(root):
    started = time.monotonic()
    root = Path(root)
    public.require(root.is_dir() and not root.is_symlink(), "invalid-bep-root")
    streams = []
    rejected = 0
    for path in sorted(root.glob("*/recipe-*/*.bep.json")):
        try:
            source = public.relative_name(path.relative_to(root).as_posix())
            public.safe_text(source)
        except ValueError:
            rejected += 1
            continue
        try:
            public.require(not path.parent.is_symlink()
                           and not path.parent.parent.is_symlink(),
                           "invalid-bep-parent")
            value = analyze(path)
        except (OSError, ValueError, TypeError, AttributeError,
                RecursionError):
            value = {"status": "unavailable"}
        streams.append({"source": source, **value})
    result = {
        "schema_version": 1,
        "analysis_seconds": time.monotonic() - started,
        "rejected_source_names": rejected,
        "streams": streams,
    }
    public.safe_json(result)
    return result


def write_report(path, text):
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(
                mode="w", encoding="utf-8", dir=path.parent,
                delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(text)
        temporary.replace(path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    args = parser.parse_args()
    try:
        value = report(args.root)
        write_report(args.root / "bep-summary.json",
                     json.dumps(value, indent=2, allow_nan=False) + "\n")
        write_report(args.root / "bep-summary.md", markdown(value))
    except (OSError, ValueError, TypeError, AttributeError, RecursionError):
        print("Public BEP analysis unavailable: missing or invalid evidence.",
              file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
