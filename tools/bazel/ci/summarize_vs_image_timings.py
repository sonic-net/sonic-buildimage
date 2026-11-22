"""Summarize Bazel recipes in the VS installer build using JSON trace profiles."""

import argparse
from bisect import bisect_right
from collections import defaultdict
from datetime import datetime, timezone
import gzip
import html
import json
import math
import re
import sys
from operator import itemgetter
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci.container_profile import (
    COMPILERS, batch_profile, cache_fetch, combine_batches, subprocess_mnemonic, union_seconds,
)


PUBLIC_NAME = re.compile(r"[A-Za-z0-9][A-Za-z0-9_.+-]{0,191}\Z")
MAX_BATCH_ARCHIVES = 1024
CATEGORIES = ("lock", "compile", "foreign", "fetch")
LOCK_EVENTS = {
    "Blocking on busy Bazel server (in client)",
    "Blocking on busy Bazel server (in server)",
}
BUILD_EVIDENCE = {
    "repository_seconds": "Repository preparation",
    "repository_read_seconds": "Repository input reads/waits",
    "compile_seconds": "Compiler subprocesses",
    "foreign_seconds": "Nested build subprocesses",
    "packaging_seconds": "Image/tar/gzip critical path",
    "package_selection_seconds": "APT selection critical path",
    "validation_seconds": "Identity/source validation critical path",
    "fetch_seconds": "Cache input/output fetch",
    "cache_write_seconds": "Cache write spans (not proof of remote upload)",
    "arm64_repository_seconds": "ARM64 package repository preparation",
    "armhf_repository_seconds": "ARMHF package repository preparation",
}
MAX_SPANS = 8
REPOSITORY_NAME = re.compile(r"//:[A-Za-z0-9_.+~-]{1,188}\Z")
PUBLIC_TARGET = re.compile(
    r"(?:(?:@{1,2}[A-Za-z0-9_.+~-]+)?//[A-Za-z0-9_+./~-]*:[A-Za-z0-9_+./~-]+"
    r"|(?:platform|dockers|tools|src|external)/[A-Za-z0-9_+./~-]+)\Z")
CRITICAL_KINDS = {
    "OCIImage": "OCI image", "OCITarball": "OCI tarball",
    "OCITarballManifest": "OCI tar manifest", "Gzip": "Gzip compression",
    "APTDebug": "Select debug APT additions", "APTRuntime": "Select runtime APT additions",
    "ValidatePackages": "Validate installed package identities",
    "ValidateSource": "Validate source/runtime identities",
    "GoCompilePkg": "Go package compilation", "GoLink": "Go linking",
    "GoStdlib": "Go standard library", "GoToolchainBinaryBuild": "Go toolchain builder",
    "PackageControls": "Derive Debian package controls", "Unpack": "Unpack package",
    "KernelBuild": "Kernel package action", "KernelTools": "Prepare kernel build tools",
    "Genrule": "Genrule",
}


def public_target(value):
    return (isinstance(value, str) and len(value) <= 240 and PUBLIC_TARGET.fullmatch(value)
            and not any(part in {".", ".."} for part in re.split(r"[/:]", value)))


def critical_span(event):
    name = event["name"]
    if not name.startswith("action '") or not name.endswith("'"):
        return None
    name = name[8:-1].removesuffix(" [for tool]")
    target = None
    if name.startswith("Compressing ") and " to " in name:
        kind, target = "Gzip", name.rsplit(" to ", 1)[1]
    elif name == "Selecting debug APT additions for the SONiC base image":
        kind = "APTDebug"
    elif name == "Selecting runtime APT additions for the SONiC base image":
        kind = "APTRuntime"
    elif name.startswith("Validate installed source and Debian package identities for "):
        kind, target = "ValidatePackages", name.split(" for ", 1)[1]
    elif name.startswith(("ValidateSyncdSourceRuntime ", "ValidateGearboxSourceRuntime ")):
        kind, target = "ValidateSource", name.split(" ", 1)[1]
    elif name.startswith("Derive package controls from Debian recipes and target binaries for "):
        kind, target = "PackageControls", name.split(" for ", 1)[1]
    elif re.fullmatch(r"Compiling SONiC [0-9.]+ AMD64 VS kernel Debian image/debug/headers/kbuild packages", name):
        kind = "KernelBuild"
    elif name == "Preparing declared Debian kernel build tools":
        kind = "KernelTools"
    else:
        prefix, _, target = name.partition(" ")
        kind = {"OCI": "OCIImage"}.get(prefix, prefix)
        if prefix == "OCI" and target.startswith("Image "):
            target = target[6:]
        if name.startswith("Executing genrule "):
            kind, target = "Genrule", name.removeprefix("Executing genrule ")
        if kind not in CRITICAL_KINDS:
            return None
    if target is not None and not public_target(target):
        return None
    return {"kind": kind, "target": target, "seconds": event["dur"] / 1e6}


def named_spans(events):
    reads = defaultdict(list)
    for event in events:
        if (event.get("ph") == "X" and event.get("cat") == "Starlark builtin function call"
                and event.get("name") == "read"):
            reads[(event["pid"], event["tid"])].append(event)
    repositories, critical = [], []
    for event in events:
        if event.get("ph") != "X" or event.get("dur", 0) <= 0:
            continue
        if event.get("cat") == "Starlark repository function call" and REPOSITORY_NAME.fullmatch(event["name"]):
            children = [
                read for read in reads[(event["pid"], event["tid"])]
                if event["ts"] <= read["ts"] and read["ts"] + read["dur"] <= event["ts"] + event["dur"]
            ]
            repositories.append({
                "name": event["name"], "seconds": event["dur"] / 1e6, "read_count": len(children),
                "read_seconds": union_seconds((e["ts"], e["ts"] + e["dur"]) for e in children),
            })
        elif event.get("cat") == "critical path component":
            span = critical_span(event)
            if span:
                critical.append(span)
    return {
        key: sorted(values, key=lambda span: span["seconds"], reverse=True)[:MAX_SPANS]
        for key, values in (("repositories", repositories), ("critical_path", critical))
    }


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
        elif (process := subprocess_mnemonic(event)) is not None:
            if process:
                add_process(spans, process, interval)
            else:
                sandbox_processes[thread].append(interval)
        elif category == "build phase marker" and name in LOCK_EVENTS:
            spans["lock"].append(interval)
        elif cache_fetch(event):
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


def build_evidence(events, durations):
    spans = {key: [] for key in BUILD_EVIDENCE}
    repositories = defaultdict(list)
    reads = defaultdict(list)
    for event in events:
        if event.get("ph") != "X":
            continue
        start, end = event["ts"], event["ts"] + event["dur"]
        category, name = event.get("cat"), event["name"]
        thread = (event["pid"], event["tid"])
        if category == "Starlark repository function call" and name.startswith("//:"):
            spans["repository_seconds"].append((start, end))
            repositories[thread].append((start, end))
            for architecture in ("arm64", "armhf"):
                if name.startswith("//:rules_distroless++apt+") and "-" + architecture + "_" in name:
                    spans[architecture + "_repository_seconds"].append((start, end))
        elif category == "Starlark builtin function call" and name == "read":
            reads[thread].append((start, end))
        elif category == "Remote execution upload time":
            spans["cache_write_seconds"].append((start, end))
        elif category == "critical path component" and name.startswith((
                "action 'OCI Image ", "action 'OCITarball", "action 'Compressing ")):
            spans["packaging_seconds"].append((start, end))
        elif category == "critical path component" and name.startswith("action 'Selecting ") and "APT" in name:
            spans["package_selection_seconds"].append((start, end))
        elif category == "critical path component" and name.startswith("action 'Validate"):
            spans["validation_seconds"].append((start, end))
    for thread, children in reads.items():
        parents = sorted(repositories[thread])
        for start, end in children:
            index = bisect_right(parents, start, key=itemgetter(0)) - 1
            if index >= 0 and end <= parents[index][1]:
                spans["repository_read_seconds"].append((start, end))
    return {
        key: durations[key.removesuffix("_seconds")] if key in {
            "compile_seconds", "foreign_seconds", "fetch_seconds",
        } else union_seconds(intervals)
        for key, intervals in spans.items()
    }


def measured(seconds, count=1, expected=1):
    if expected == 0:
        return "n/a"
    if seconds is None:
        return "unavailable"
    display = "<0.01" if 0 < seconds < 0.01 else f"{seconds:.2f}"
    return display if count == expected else display + " (partial)"


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
        profile = json.load(stream)
    events = profile["traceEvents"]
    if not isinstance(events, list) or not events:
        raise ValueError("empty trace")
    metadata = profile.get("otherData")
    return events, metadata.get("profile_start_ts") if isinstance(metadata, dict) else None


def batch_targets(instance, legacy_archives):
    targets = instance / "batch-targets.json"
    if not targets.exists():
        return legacy_archives, True
    archives = json.loads(targets.read_text())
    if not isinstance(archives, dict) or len(archives) > MAX_BATCH_ARCHIVES:
        raise ValueError("invalid batch targets")
    for name, value in archives.items():
        if (not PUBLIC_NAME.fullmatch(name) or not name.endswith(".gz")
                or not isinstance(value, list) or len(value) != 2
                or not all(isinstance(v, str) for v in value)
                or not PUBLIC_NAME.fullmatch(value[0]) or not value[0].endswith(".gz")
                or not value[1].startswith(("//", "@"))):
            raise ValueError("invalid batch archive or target")
    return archives, False


def batch_image_starts(epoch_ms, bounds, archives, build_record):
    """Match Bazel's relative action timestamps to the launcher's wall clock."""
    if (type(epoch_ms) is not int or not 0 < epoch_ms <= 253402300799000 or
            not build_record or build_record["start_epoch_seconds"] is None or build_record["exit_code"]):
        return {}
    build_start = build_record["start_epoch_seconds"]
    build_end = build_start + build_record["seconds"]
    phases = defaultdict(set)
    for archive, (owner, label) in archives.items():
        phases[owner].add(("layout-" if label.endswith(".boot-oci") else "export-") + archive)
    starts = {}
    for owner, (first_us, last_us) in bounds.items():
        first = epoch_ms / 1000 + first_us / 1e6
        last = epoch_ms / 1000 + last_us / 1e6
        # The profile epoch has millisecond precision. Reject clocks that would
        # place the observed actions outside the command that generated them.
        if build_start - .001 <= first <= last <= build_end + .001:
            starts[owner] = (first, max(last, build_end), phases[owner])
    return starts


def read_owner(directory, *, legacy_archives=None, batches=None, exports_by_owner=None):
    row = {"owner": directory.name, "bazel_seconds": None, "export_seconds": None,
           **{key + "_seconds": None for key in CATEGORIES}, "failed": False,
           "bazel_phases": 0, "bazel_expected": 0, "profiles": 0, "profiles_expected": 0,
           "exports": 0, "exports_expected": 0}
    problems = []
    build = {"expected": 0, "timings": 0, "profiles": 0, "seconds": None, "lock_seconds": None,
             **{key: None for key in BUILD_EVIDENCE}}
    details = {"scope": "unavailable", "expected": 0, "profiles": 0, "phase_count": 0,
               "repositories": [], "critical_path": [], "phases": []}
    plans = [(path.parent, read_plan(path)) for path in sorted(directory.glob("*/recipe.json"))]
    if not plans and directory.name.endswith(".gz") and any(directory.glob("*.timing.json")):
        plans.append((directory, legacy_plan(directory.name)))
    if not plans:
        return None, []

    kernel_launcher = directory.name == "linux-kernel" and not any("build" in plan["bazel"] for _, plan in plans)
    for instance, plan in plans:
        has_build = "build" in plan["bazel"]
        build["expected"] += int(has_build)
        has_named = has_build or kernel_launcher
        details["expected"] += int(has_named)
        phase_records = []
        build_seconds = None
        build_record = None
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
                started = timing.get("start_epoch_seconds")
                if (type(started) not in (int, float) or not math.isfinite(started)
                        or not 0 <= started <= 253402300799):
                    started = None
                phase_records.append({
                    "phase": phase, "seconds": seconds, "lock_seconds": None,
                    "start_epoch_seconds": started, "exit_code": code,
                })
                if group == "bazel" and phase == "build":
                    build_record = phase_records[-1]
                    build_seconds = seconds
                    build["timings"] += 1
                    build["seconds"] = (build["seconds"] or 0) + seconds
                if code:
                    row["failed"] = True
                    problems.append(f"{phase} exited {code}")
        row["profiles_expected"] += len(plan["profiles"])
        for phase in plan["profiles"]:
            try:
                events, epoch_ms = read_profile(instance / f"{phase}.profile.json.gz")
                durations = summarize_profile(events)
                evidence = named = None
                if phase == "build" and has_named:
                    evidence = build_evidence(events, durations)
                    named = named_spans(events)
            except (OSError, EOFError, ValueError, KeyError, TypeError, AttributeError):
                problems.append(f"{phase} profile missing or unreadable")
                continue
            if batches is not None and phase == "build" and build_seconds is not None:
                try:
                    archives, legacy = batch_targets(instance, legacy_archives)
                    bounds = {}
                    attribution = batch_profile(events, build_seconds, archives, legacy=legacy, owner_bounds=bounds)
                    if attribution is not None:
                        starts = batch_image_starts(epoch_ms, bounds, archives, build_record) if len(plans) == 1 else {}
                        batches.append((attribution, starts))
                except (OSError, ValueError, KeyError, TypeError, AttributeError):
                    problems.append("per-container profile ownership unavailable")
            del events
            row["profiles"] += 1
            for key, seconds in durations.items():
                metric = key + "_seconds"
                row[metric] = (row[metric] or 0) + seconds
            for record in phase_records:
                if record["phase"] == phase:
                    record["lock_seconds"] = durations["lock"]
            if evidence is not None and has_build:
                build["profiles"] += 1
                for key, seconds in {"lock_seconds": durations["lock"], **evidence}.items():
                    build[key] = (build[key] or 0) + seconds
            if named is not None:
                details["profiles"] += 1
                for key in ("repositories", "critical_path"):
                    details[key] = sorted(
                        details[key] + named[key], key=lambda span: span["seconds"], reverse=True)[:MAX_SPANS]
        if (exports_by_owner is not None and len(plans) == 1 and plan["exports"] and
                not plan["bazel"] and not plan["profiles"] and len(phase_records) == len(plan["exports"]) and
                all(p["start_epoch_seconds"] is not None and not p["exit_code"] for p in phase_records)):
            exports_by_owner[directory.name] = (
                {p["phase"] for p in phase_records},
                min(p["start_epoch_seconds"] for p in phase_records),
                max(p["start_epoch_seconds"] + p["seconds"] for p in phase_records),
            )
        details["phase_count"] += len(phase_records)
        details["phases"] = sorted(
            details["phases"] + phase_records,
            key=lambda phase: (phase["start_epoch_seconds"] is None, phase["start_epoch_seconds"] or 0, phase["phase"]),
        )[:MAX_SPANS]
    row["build"] = build if build["expected"] else None
    details["scope"] = "build" if build["expected"] else (
        "kernel-build" if directory.name == "linux-kernel" else "unavailable")
    row["details"] = details
    return row, problems


def build_nonlock(row):
    build = row.get("build")
    if (build and build["expected"] == build["timings"] == build["profiles"]
            and build["seconds"] >= build["lock_seconds"]):
        return build["seconds"] - build["lock_seconds"]
    return None


def build_explanation(row):
    build = row.get("build")
    if build is None:
        return ("Separate kernel launcher: bootstrap, build, output query and module resolution; "
                "not a directly comparable image build." if row["owner"] == "linux-kernel"
                else "Detailed build-only evidence was not recorded.")
    if not build["profiles"]:
        return "Build profile unavailable; no cause inferred from elapsed time."
    complete = build["profiles"] == build["expected"]
    evidence = sorted(((value, BUILD_EVIDENCE[key]) for key, value in build.items()
                       if key in BUILD_EVIDENCE and value), reverse=True)
    if evidence:
        text = (f"Largest recorded work category: {evidence[0][1]}: {measured(evidence[0][0])}s"
                + (" (partial)." if not complete else "."))
    else:
        text = "No matching work spans recorded; remaining analysis/setup work is not attributed."
    if build_nonlock(row) is not None:
        if build["lock_seconds"] > build["seconds"] / 2:
            text = "Most build-command elapsed time was server lock waiting. " + text
        if row["profiles"] == row["profiles_expected"]:
            other_lock = row["lock_seconds"] - build["lock_seconds"]
            if other_lock > 0:
                text += f" Other-command lock wait: {measured(other_lock)}s."
    return text


def readable(seconds):
    if seconds is None:
        return "unavailable"
    if 0 < seconds < 0.01:
        return "<0.01s"
    if 0 < seconds < 1:
        return f"{seconds:.2f}s"
    seconds = math.floor(seconds + 0.5)
    hours, seconds = divmod(seconds, 3600)
    minutes, seconds = divmod(seconds, 60)
    if hours:
        return f"{hours}h{minutes:02}m{seconds:02}s"
    return f"{minutes}m{seconds:02}s" if minutes else f"{seconds}s"


def card_time(row):
    value = build_nonlock(row)
    if value is None and row["owner"] == "linux-kernel" and row["bazel_phases"] == row["bazel_expected"]:
        return row["bazel_seconds"]
    return value


def diagnosis(row):
    build = row.get("build")
    if not build:
        return "Isolated kernel launcher" if row["owner"] == "linux-kernel" else "Detailed profiling unavailable"
    if not build["profiles"]:
        return "Build profile unavailable"
    details = row.get("details", {})
    critical = details.get("critical_path", [])
    debug = any(span["kind"] in {"Gzip", "OCIImage", "OCITarball"}
                and "-dbg" in (span["target"] or "") for span in critical)
    primary = max(BUILD_EVIDENCE, key=lambda key: build[key] or 0)
    if not build[primary]:
        return "Analysis/setup not fully attributed"
    if primary in {"repository_seconds", "repository_read_seconds"}:
        if any("++gcc+" in span["name"] or "++sysroots+" in span["name"]
               for span in details.get("repositories", [])):
            return "Toolchain and repository preparation"
        return ("Repository preparation + " + ("debug packaging" if debug else "image packaging")
                if build["packaging_seconds"] else "Repository preparation")
    if primary == "compile_seconds" and {"GoCompilePkg", "GoLink"} <= {span["kind"] for span in critical}:
        return "Go compilation and linking"
    return {
        "packaging_seconds": ("Debug packaging and validation" if debug else "Image packaging and validation")
        if build["validation_seconds"] else "Image/archive processing",
        "compile_seconds": "Source compilation", "foreign_seconds": "Nested build subprocesses",
        "package_selection_seconds": "APT selection and package assembly",
        "validation_seconds": "Source and package identity validation",
        "cache_write_seconds": "Cache I/O and output processing", "fetch_seconds": "Cache input/output fetch",
    }.get(primary, BUILD_EVIDENCE[primary])


def measured_duration(seconds):
    return readable(seconds) + (f" ({seconds:.2f}s)" if seconds is not None and seconds >= 1 else "")


def target_markdown(row):
    build, details = row.get("build"), row.get("details")
    label = "LAUNCHER ELAPSED" if row["owner"] == "linux-kernel" and not build else "BUILD EXCLUDING LOCK"
    lines = ["<details>", f"<summary><strong>{html.escape(row['owner'])}</strong> - "
             f"{readable(card_time(row))} {label} - {diagnosis(row)}</summary>", "", build_explanation(row), "",
             "| Measurement | Duration / coverage |", "|---|---:|",
             f"| {label.title()} | {measured_duration(card_time(row))} |"]
    if build:
        lines.extend([
            f"| Build elapsed | {measured_duration(build['seconds'])} ({build['timings']}/{build['expected']} receipts) |",
            f"| Build lock | {measured_duration(build['lock_seconds'])} ({build['profiles']}/{build['expected']} profiles) |",
        ])
        lines.extend(f"| {label} | {measured_duration(build[key])} |"
                     for key, label in BUILD_EVIDENCE.items() if build[key])
    if not details:
        return lines + ["", "Named spans were not recorded for this historical run.", "", "</details>"]
    lines.extend(["", f"Named evidence: {details['profiles']}/{details['expected']} build profiles. "
                  "Up to eight longest recognized repository/action spans are retained; they overlap and are not additive."])
    if details["scope"] == "kernel-build":
        lines.extend(["", "Kernel spans describe its build profile only, not the entire launcher elapsed time."])
    lines.extend(["", "#### Longest repository spans", ""])
    if details["repositories"]:
        lines.extend(["| Repository | Span | Nested reads/waits |", "|---|---:|---:|"])
        lines.extend(f"| `{span['name']}` | {measured_duration(span['seconds'])} | "
                     f"{span['read_count']} reads; {measured_duration(span['read_seconds'])} |"
                     for span in details["repositories"])
    else:
        lines.append("No recognized repository spans recorded." if details["profiles"] else "Build profile unavailable.")
    lines.extend(["", "#### Longest critical-path actions", ""])
    if details["critical_path"]:
        lines.extend(["| Action | Target / output | Span |", "|---|---|---:|"])
        lines.extend(f"| {CRITICAL_KINDS[span['kind']]} | "
                     f"{'`' + span['target'] + '`' if span['target'] else '-'} | {measured_duration(span['seconds'])} |"
                     for span in details["critical_path"])
    else:
        lines.append("No recognized critical-path actions recorded." if details["profiles"] else "Build profile unavailable.")
    lines.extend(["", f"#### Command receipts ({len(details['phases'])}/{details['phase_count']} shown)", "",
                  "| Phase | Requested (UTC) | Elapsed | Lock | Exit |", "|---|---|---:|---:|---:|"])
    for phase in details["phases"]:
        started = (datetime.fromtimestamp(phase["start_epoch_seconds"], timezone.utc).strftime("%Y-%m-%d %H:%M:%S")
                   if phase["start_epoch_seconds"] is not None else "unavailable")
        lines.append(f"| `{phase['phase']}` | {started} | {measured_duration(phase['seconds'])} | "
                     f"{measured_duration(phase['lock_seconds'])} | {phase['exit_code']} |")
    return lines + ["", "Receipts show elapsed wall time including lock waiting. Query/export waits are excluded "
                    "from the build-only ranking. Missing lock profiles are not treated as zero.", "", "</details>"]


def markdown_row(row):
    return [row["owner"], measured(row["bazel_seconds"], row["bazel_phases"], row["bazel_expected"]),
            *(measured(row[key + "_seconds"], row["profiles"], row["profiles_expected"]) for key in CATEGORIES),
            measured(row["export_seconds"], row["exports"], row["exports_expected"])]


def historical_archives(root):
    # Before batch-targets.json a single shared build used export-only recipes
    # for its members. Confirm their actual archive outputs in the trace.
    archives = {}
    if len(list((root / "vs-container-images").glob("*/recipe.json"))) == 1:
        for plan_path in root.glob("*.gz/*/recipe.json"):
            owner = plan_path.parent.parent.name
            if not PUBLIC_NAME.fullmatch(owner):
                continue
            plan = read_plan(plan_path)
            if not plan["bazel"] and not plan["profiles"]:
                for phase in plan["exports"]:
                    if phase.startswith("export-") and phase.endswith(".gz"):
                        archive = phase.removeprefix("export-")
                        if archive in archives or len(archives) >= MAX_BATCH_ARCHIVES:
                            return {}
                        archives[archive] = (owner, "")
    return archives


def report(root):
    batches = []
    exports_by_owner = {}
    breakdown = None
    recipes = []
    notes = []
    for directory in sorted(root.iterdir()):
        if directory.is_dir() and PUBLIC_NAME.fullmatch(directory.name):
            if directory.name == "vs-container-images":
                row, problems = read_owner(directory, legacy_archives=historical_archives(root), batches=batches)
                if batches and len(batches) == row["profiles_expected"]:
                    breakdown = combine_batches([detail for detail, _ in batches])
            else:
                row, problems = read_owner(directory, exports_by_owner=exports_by_owner)
            if row is not None:
                recipes.append(row)
            if problems:
                notes.append(f"- `{directory.name}` — " + ", ".join(problems))
    if breakdown and len(batches) == 1:
        _, starts = batches[0]
        for member in breakdown["rows"]:
            owner = member["owner"]
            if owner in starts and owner in exports_by_owner:
                first, build_end, expected = starts[owner]
                observed, export_start, export_end = exports_by_owner[owner]
                if expected == observed and build_end <= export_start <= export_end <= 253402300799:
                    member["total_seconds"] = export_end - first
    recipes.sort(key=lambda row: (
        row["bazel_phases"] == row["exports"] == 0,
        -((row["bazel_seconds"] or 0) + (row["export_seconds"] or 0)),
        row["owner"],
    ))
    lines = ["### VS Bazel build timings (wall seconds)", ""]
    if recipes:
        lines.extend(["| Make target or package | Bazel / launcher | Bazel lock wait | Compiler subprocesses | "
                      "Nested build subprocesses | Cache input/output fetch | Output handoff |", "|---|---:|---:|---:|---:|---:|---:|"])
        lines.extend("| " + " | ".join(markdown_row(row)) + " |" for row in recipes)
        if any(row["owner"] == "vs-container-images" for row in recipes):
            lines.extend(["", "`vs-container-images` records the shared Bazel build. "
                          "Each image records the time spent publishing its OCI layout or exporting its archive."])
        lines.extend(["", "Command and output handoff times are summed per target. The kernel launcher includes bootstrap, "
                      "build, output query and module resolution. Nested build time covers an entire Make or "
                      "foreign build subprocess. Cache fetch includes both disk and remote reads. "
                      "Parallel activities can overlap. The Bazel total also includes work such as analysis and linking. "
                      "A Make cache hit can skip Bazel entirely. "
                      "Zero means no matching span was recorded."])
        lines.extend(["", "### Why each target took its time", "",
                      "Expand a target for its repository spans, critical-path actions and command receipts. "
                      "Ordered by build elapsed excluding lock; kernel launcher elapsed is shown separately in scope."])
        for row in sorted(recipes, key=lambda row: (
                card_time(row) is None, -(card_time(row) or 0), row["owner"])):
            lines.extend(["", *target_markdown(row)])
        lines.extend(["", "Build excluding lock requires matching build receipts and profiles; output-query waits "
                      "remain in the all-command totals above. Evidence spans overlap and must not be added. "
                      "Repository reads may wait for other repositories; they do not prove CPU parsing or disk slowness. "
                      "Packaging spans may include cache materialization, not just execution. "
                      "Cache writes can include disk cache and do not establish remote publication. "
                      "ARM repository spans do not by themselves establish unnecessary work or achievable savings."])
    else:
        lines.append("No Bazel recipes were recorded for this build.")
    if notes:
        lines.extend(["", "Incomplete measurements", "", *notes])
    receipt = {"version": 2, "recipes": recipes}
    if breakdown is not None:
        receipt["container_breakdown"] = breakdown
        lines.extend(["", "### Images in the shared Bazel command", "",
                      "| Image | Actions | Total elapsed (s) | Action wall (s) | Compile (s) | OCI tar (s) | Compression (s) | Cache fetch (s) |",
                      "|---|---:|---:|---:|---:|---:|---:|---:|"])
        for row in sorted(breakdown["rows"], key=lambda r: (-r["action_seconds"], r["owner"])):
            lines.append("| {owner} | {actions} | {total} | {action_seconds:.2f} | {compile_seconds:.2f} | "
                         "{archive_seconds:.2f} | {compress_seconds:.2f} | {fetch_seconds:.2f} |".format(
                             total=measured(row.get("total_seconds")), **row))
        lines.extend(["", "Image rows can overlap. Compiler subprocesses, OCI tar, compression and cache fetch are included in "
                      "action wall time. Total elapsed runs from the first action attributed to the image through its last "
                      "output handoff, including any waits between them.",
                      f"Images ran concurrently for {breakdown['parallel_seconds']:.2f}s. "
                      f"Only shared or unattributed actions ran for {breakdown['other_action_seconds']:.2f}s. "
                      f"No action was recorded for {breakdown['no_action_seconds']:.2f}s."])
    return receipt, "\n".join(lines) + "\n"


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
            details = recipe["details"]
            header = dict(recipe, details=dict(details, repositories=[], critical_path=[], phases=[]))
            print("SONIC_VS_BAZEL_TIMINGS_V2", json.dumps({"recipe": header}, allow_nan=False, separators=(",", ":")))
            for group in ("repositories", "critical_path", "phases"):
                print("SONIC_VS_BAZEL_TIMINGS_V2", json.dumps({"evidence": {
                    "owner": recipe["owner"], "group": group, "values": details[group],
                }}, allow_nan=False, separators=(",", ":")))
        print("SONIC_VS_BAZEL_TIMINGS_V2", json.dumps({
            "count": len(receipt["recipes"]), "evidence_count": len(receipt["recipes"]) * 3,
        }, separators=(",", ":")))
        if breakdown := receipt.get("container_breakdown"):
            for row in breakdown["rows"]:
                print("SONIC_VS_CONTAINER_TIMINGS_V1", json.dumps({"row": row}, separators=(",", ":")))
            trailer = {k: v for k, v in breakdown.items() if k != "rows"}
            print("SONIC_VS_CONTAINER_TIMINGS_V1",
                  json.dumps({"count": len(breakdown["rows"]), **trailer}, separators=(",", ":")))
    else:
        print(markdown, end="")
