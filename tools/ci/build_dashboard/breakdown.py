"""Reduce public Make job logs to bounded timing measurements.

Publish timestamps, numeric metrics, fixed outcomes and validated build target
identifiers. Command lines, paths and free-form log messages stay out of history.
"""
from __future__ import annotations

from collections import Counter
from datetime import datetime, timedelta
import json
import math
import re

VERSION = 1
SOURCE = "github-job-log"
MAX_LOG_BYTES = 32 * 1024 * 1024
MAX_SECTIONS = 32
MAX_TASKS = 512
MAX_PHASE_SEGMENTS = 64
MAX_PHASE_JSON_BYTES = 16 * 1024
# Producer durations use a monotonic clock; parent spans use streamed GitHub log
# timestamps. Allow at most two seconds of overhead/ingestion difference and
# preserve both measured totals unchanged.
PHASE_CLOCK_TOLERANCE_SECONDS = 2
PHASE_PARTITION_TOLERANCE_SECONDS = 0.00001
IMAGE_PHASES = {"assembly", "boot-checks", "compression"}
IMAGE_CONTEXTS = {"recipe", "filesystem", "installer", "bios", "uefi", "4-asic", "6-asic"}
PHASE_REASONS = {"not-recorded", "invalid-timing", "incomplete-timing"}
FINAL_IMAGE_TARGETS = {"target/sonic-vs.bin", "target/sonic-vs.img.gz"}
REASONS = {
    "pending-collection", "step-not-successful", "logs-expired", "logs-too-large",
    "logs-temporarily-unavailable", "unsupported-log-format", "incomplete-log", "invalid-timing",
}
RETRY_REASONS = {"pending-collection", "logs-temporarily-unavailable"}
SECTION_LABELS = {
    "vs-image": "VS image, dependencies and cleanup",
    "oci-session": "OCI archives and verification",
    "oci-verification": "P4 packages and OCI verification",
    "orchagent-release": "Orchagent release",
    "orchagent-debug": "Orchagent debug",
    "syncd-release": "Syncd release",
    "syncd-debug": "Syncd debug",
    "p4rt-runtime": "P4RT runtime package",
    "p4rt-debug": "P4RT debug package",
}
OUTER_TARGETS = {"bazel-vs-oci": "oci-session", "bazel-vs-oci-verify": "oci-verification"}
TASK_LABELS = {
    "final-image": "Final VS image recipe",
    "rootfs": "Root filesystem squashfs",
    "base": "Base container",
    "config-engine": "Config-engine container",
    "orchagent-release": "Orchagent release container",
    "orchagent-debug": "Orchagent debug container",
    "orchagent-pair": "Orchagent runtime and debug containers",
    "syncd-release": "Syncd release container",
    "syncd-debug": "Syncd debug container",
    "syncd-pair": "Syncd runtime and debug containers",
    "p4rt-runtime": "P4RT runtime package",
    "p4rt-debug": "P4RT debug package",
    "bmp-watchdog": "BMP watchdog container",
    "gnmi-watchdog": "gNMI watchdog container",
    "sonic-otel": "OpenTelemetry container",
    "router-advertiser": "Router advertiser container",
    "dash-engine": "DASH engine container",
    "sonic-mgmt-framework": "Management framework container",
    "database": "Database container",
    "gnmi-sidecar": "gNMI sidecar container",
    "sonic-gnmi": "gNMI container",
    "mux": "MUX container",
    "restapi-sidecar": "REST API sidecar container",
    "sysmgr": "System manager container",
    "lldp": "LLDP container",
    "sonic-p4rt": "P4RT container",
    "gbsyncd-vs": "Gearbox syncd container",
    "platform-monitor": "Platform monitor container",
    "snmp": "SNMP container",
    "dhcp-relay": "DHCP relay container",
    "eventd": "Event daemon container",
    "sonic-bmp": "BMP container",
    "swss-layer": "SWSS layer container",
    "dash-ha": "DASH HA container",
    "fpm-frr": "FRR container",
    "nat": "NAT container",
    "sflow": "sFlow container",
    "teamd": "Team daemon container",
    "macsec": "MACsec container",
}
EXACT_TARGETS = {
    "target/sonic-vs.bin": "final-image",
    "target/sonic-vs.bin__vs__rfs.squashfs": "rootfs",
    "target/sonic-vs.img.gz": "final-image",
    "target/sonic-vs.img.gz__vs__rfs.squashfs": "rootfs",
    "target/docker-orchagent.gz": "orchagent-release",
    "target/docker-orchagent-dbg.gz": "orchagent-debug",
    "target/docker-syncd-vs.gz": "syncd-release",
    "target/docker-syncd-vs-dbg.gz": "syncd-debug",
}
for _kind in TASK_LABELS.keys() - {
    "final-image", "rootfs", "base", "config-engine", "swss-layer", "orchagent-release",
    "orchagent-debug", "syncd-release", "syncd-debug", "p4rt-runtime", "p4rt-debug",
    "orchagent-pair", "syncd-pair",
}:
    EXACT_TARGETS[f"target/docker-{_kind}.gz"] = _kind

DISTRO_CONTAINER = re.compile(r"target/docker-(base|config-engine|swss-layer)-(bookworm|trixie)\.gz\Z")
P4RT = re.compile(r"target/files/(?:bookworm|trixie)/sonic-p4rt(-dbgsym)?_[0-9][A-Za-z0-9.+~:-]{0,63}_amd64\.tar\.bz2\Z")
P4RT_DEB = re.compile(r"target/debs/(?:bookworm|trixie)/sonic-p4rt(-dbgsym)?_[0-9][A-Za-z0-9.+~:-]{0,63}_amd64\.deb\Z")
STAMP = re.compile(r"^(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d(?:\.\d{1,9})?Z) (.*)$")
ANSI = re.compile(r"\x1b\[[0-9;]*m")
OUTER = re.compile(r"\+\+\+ --- Making (\S{1,256}) --- \+\+\+\s*\Z")
TARGET = re.compile(r"\[ (building|finished|cached) \] \[ (\S{1,256}) \]\s*\Z")
IMAGE_PHASE_MARKER = re.compile(r"\[ image phases \] \[ (\S{1,256}) \] (.*)\Z")
IMAGE_PAIR_MARKER = re.compile(r"\[ bazel image pair \] \[ (\S{1,256}) \] \[ (\S{1,256}) \]\Z")
IMAGE_PAIR_KINDS = {"target/docker-orchagent.gz": "orchagent-pair", "target/docker-syncd-vs.gz": "syncd-pair"}
# Published v1 snapshots used exactly these four image owners.
RECIPE_OWNERS = {"docker-teamd.gz", "docker-syncd-vs.gz", "docker-fpm-frr.gz", "docker-mux.gz"}
RECIPE_METRICS = {
    "bazel_phases": (3, ("bazel_seconds",)),
    "profiles": (3, ("lock_seconds", "compile_seconds", "foreign_seconds", "fetch_seconds")),
    "exports": (2, ("export_seconds",)),
}
RECIPE_EXPECTED = {"bazel_phases": "bazel_expected", "profiles": "profiles_expected", "exports": "exports_expected"}
RECIPE_FIELDS = ({"owner", "failed"} | RECIPE_METRICS.keys() |
                 {key for _, metrics in RECIPE_METRICS.values() for key in metrics})
RECIPE_REASONS = {"pending-collection", "step-not-successful", "logs-expired", "logs-too-large",
                  "logs-temporarily-unavailable", "not-recorded", "invalid-timing"}
RECIPE_MARKER = "SONIC_VS_IMAGE_TIMINGS_V1"
BAZEL_RECIPE_MARKER = "SONIC_VS_BAZEL_TIMINGS_V2"
RECIPE_STEP_VERSIONS = {"Summarize VS image build timings": 1, "Summarize VS Bazel build timings": 2}
RECIPE_LISTS = {1: "images", 2: "recipes"}
RECIPE_OWNER = re.compile(r"[A-Za-z0-9][A-Za-z0-9_.+-]{0,191}\Z")
MAX_RECIPES = 1024
MAX_RECIPE_COMMANDS = 65535
RECIPE_COMMAND = "python3 -B tools/bazel/ci/summarize_vs_image_timings.py "


def recipe_unavailable(reason, version=1):
    return {"version": version, "status": "unavailable", "reason": reason, RECIPE_LISTS[version]: []}


def valid_recipes(value):
    if not isinstance(value, dict) or type(value.get("version")) is not int or value["version"] not in RECIPE_LISTS:
        return False
    version = value["version"]
    key = RECIPE_LISTS[version]
    if value.get("status") == "unavailable":
        return (set(value) == {"version", "status", "reason", key} and value[key] == []
                and isinstance(value["reason"], str) and value["reason"] in RECIPE_REASONS)
    if (set(value) != {"version", "status", key} or value["status"] != "available"
            or not isinstance(value[key], list) or len(value[key]) > MAX_RECIPES):
        return False
    owners = set()
    fields = RECIPE_FIELDS if version == 1 else RECIPE_FIELDS | set(RECIPE_EXPECTED.values())
    for row in value[key]:
        if (not isinstance(row, dict) or set(row) != fields or type(row["failed"]) is not bool
                or not isinstance(row["owner"], str) or not RECIPE_OWNER.fullmatch(row["owner"])
                or row["owner"] in owners):
            return False
        owners.add(row["owner"])
        for count, (limit, metrics) in RECIPE_METRICS.items():
            if version == 2:
                limit = row[RECIPE_EXPECTED[count]]
                if type(limit) is not int or not 0 <= limit <= MAX_RECIPE_COMMANDS:
                    return False
            if type(row[count]) is not int or not 0 <= row[count] <= limit:
                return False
            if row[count]:
                if not all(finite_seconds(row[metric]) for metric in metrics):
                    return False
            elif any(row[metric] is not None for metric in metrics):
                return False
    return version == 2 or owners == RECIPE_OWNERS


def recipe_markers(log, start, end):
    """Yield only machine records in the identified Actions command's stdout."""
    groups = 0
    header = command = output = False
    second = timedelta(seconds=1)
    markers = {RECIPE_MARKER: 1, BAZEL_RECIPE_MARKER: 2}
    for when, line in log_rows(log):
        if not start <= when < end + second:
            continue
        if line.startswith("##[group]Run "):
            # Adjacent steps can start within the same API timestamp second.
            header = when < start + second
            command = output = False
        elif header:
            if line == "##[endgroup]":
                header = False
                output = command
                groups += int(command)
            elif ANSI.sub("", line).lstrip().startswith(RECIPE_COMMAND):
                command = True
        elif output and line.startswith((RECIPE_MARKER, BAZEL_RECIPE_MARKER)):
            prefix, _, payload = line.partition(" ")
            if (prefix not in markers or not payload.startswith("{") or not payload.endswith("}")
                    or len(line.encode()) > 4096):
                raise ValueError("invalid recipe marker")
            yield markers[prefix], json.loads(payload, object_pairs_hook=unique_json_object,
                                               parse_constant=reject_json_constant)
    if groups != 1:
        raise ValueError("ambiguous recipe summary step")


def parse_recipe_log(log, step):
    """Require a complete receipt; the v2 count trailer also covers empty builds."""
    expected = RECIPE_STEP_VERSIONS.get(step.get("name"))
    version = expected or 1
    if not isinstance(log, bytes) or len(log) > MAX_LOG_BYTES:
        return recipe_unavailable("logs-too-large", version)
    start, end = moment(step.get("started_at")), moment(step.get("completed_at"))
    if not start or not end or end < start:
        return recipe_unavailable("invalid-timing", version)
    rows = []
    result = None
    try:
        for marker_version, value in recipe_markers(log, start, end):
            if (expected is not None and marker_version != expected) or result is not None:
                return recipe_unavailable("invalid-timing", version)
            expected = version = marker_version
            if marker_version == 1 and set(value) == {"version", "images"}:
                result = dict(value, status="available")
            elif marker_version == 2 and set(value) == {"recipe"} and len(rows) < MAX_RECIPES:
                rows.append(value["recipe"])
            elif (marker_version == 2 and set(value) == {"count"} and type(value["count"]) is int
                  and value["count"] == len(rows)):
                result = {"version": 2, "status": "available", "recipes": rows}
            else:
                return recipe_unavailable("invalid-timing", version)
    except (UnicodeError, ValueError, RecursionError):
        return recipe_unavailable("invalid-timing", version)
    if result is None and not rows:
        return recipe_unavailable("not-recorded", version)
    return result if valid_recipes(result) else recipe_unavailable("invalid-timing", version)


def phase_unavailable(reason):
    if reason not in PHASE_REASONS:
        raise ValueError("Unknown image phase failure")
    return {"version": 1, "status": "unavailable", "reason": reason, "source": "build-phase-markers",
            "duration_seconds": None, "segments": []}


def phase_payload_error(value):
    """Validate a complete producer partition without accepting display text."""
    if (not isinstance(value, dict) or set(value) != {"version", "duration_seconds", "segments"}
            or type(value["version"]) is not int or value["version"] != 1
            or not finite_seconds(value["duration_seconds"]) or not isinstance(value["segments"], list)
            or len(value["segments"]) > MAX_PHASE_SEGMENTS):
        return "invalid-timing"
    if not value["segments"]:
        return "incomplete-timing"
    cursor, seen = 0, set()
    for segment in value["segments"]:
        if (not isinstance(segment, dict) or set(segment) != {"phase", "context", "offset_seconds", "duration_seconds"}
                or not isinstance(segment["phase"], str) or segment["phase"] not in IMAGE_PHASES
                or not isinstance(segment["context"], str) or segment["context"] not in IMAGE_CONTEXTS
                or not finite_seconds(segment["offset_seconds"]) or not finite_seconds(segment["duration_seconds"])):
            return "invalid-timing"
        key = (segment["phase"], segment["context"], segment["offset_seconds"], segment["duration_seconds"])
        if key in seen:
            return "invalid-timing"
        seen.add(key)
        if abs(segment["offset_seconds"] - cursor) > PHASE_PARTITION_TOLERANCE_SECONDS:
            return "incomplete-timing"
        cursor = segment["offset_seconds"] + segment["duration_seconds"]
    total = math.fsum(segment["duration_seconds"] for segment in value["segments"])
    if (abs(cursor - value["duration_seconds"]) > PHASE_PARTITION_TOLERANCE_SECONDS
            or abs(total - value["duration_seconds"]) > PHASE_PARTITION_TOLERANCE_SECONDS):
        return "incomplete-timing"
    return None


def unique_json_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError("Duplicate JSON field")
        result[key] = value
    return result


def reject_json_constant(_):
    raise ValueError("Non-finite JSON value")


def read_phase_payload(text):
    if len(text) > MAX_PHASE_JSON_BYTES or len(text.encode("utf-8")) > MAX_PHASE_JSON_BYTES:
        return None, "invalid-timing"
    try:
        value = json.loads(text, object_pairs_hook=unique_json_object, parse_constant=reject_json_constant)
    except (ValueError, RecursionError):
        return None, "invalid-timing"
    reason = phase_payload_error(value)
    return (None, reason) if reason else (value, None)


def complete_phase_breakdown(state, began, ended, outcome):
    if outcome != "finished" or state is None or not state["seen"]:
        return phase_unavailable("not-recorded")
    if state["error"]:
        return phase_unavailable(state["error"])
    value, marker_time = state["value"], state["when"]
    elapsed = (ended - began).total_seconds()
    if (not began <= marker_time <= ended
            or abs(elapsed - value["duration_seconds"]) > PHASE_CLOCK_TOLERANCE_SECONDS
            or abs((marker_time - began).total_seconds() - value["duration_seconds"]) > PHASE_CLOCK_TOLERANCE_SECONDS):
        return phase_unavailable("invalid-timing")
    return {"version": 1, "status": "available", "reason": None, "source": "build-phase-markers",
            "duration_seconds": value["duration_seconds"], "segments": value["segments"]}


def valid_phase_breakdown(value, parent_duration, outcome):
    if (not isinstance(value, dict)
            or set(value) != {"version", "status", "reason", "source", "duration_seconds", "segments"}
            or type(value["version"]) is not int or value["version"] != 1 or value["source"] != "build-phase-markers"):
        return False
    if value["status"] == "unavailable":
        return (isinstance(value["reason"], str) and value["reason"] in PHASE_REASONS
                and value["duration_seconds"] is None and value["segments"] == [])
    if value["status"] != "available" or value["reason"] is not None or outcome != "finished":
        return False
    payload = {key: value[key] for key in ("version", "duration_seconds", "segments")}
    return (phase_payload_error(payload) is None and finite_seconds(parent_duration)
            and abs(value["duration_seconds"] - parent_duration) <= PHASE_CLOCK_TOLERANCE_SECONDS)


def target_kind(target):
    if target in EXACT_TARGETS:
        return EXACT_TARGETS[target]
    match = DISTRO_CONTAINER.fullmatch(target)
    if match:
        return match[1]
    match = P4RT.fullmatch(target) or P4RT_DEB.fullmatch(target)
    if match:
        return "p4rt-debug" if match[1] else "p4rt-runtime"
    return None


def unavailable(reason):
    if reason not in REASONS:
        raise ValueError("Unknown breakdown reason")
    return {"version": VERSION, "status": "unavailable", "reason": reason, "source": SOURCE,
            "duration_seconds": None, "sections": []}


def retry_needed(value):
    return value is None or value.get("reason") in RETRY_REASONS


def moment(value):
    if not isinstance(value, str):
        return None
    try:
        result = datetime.fromisoformat(value.replace("Z", "+00:00"))
        return result if result.tzinfo and result.year >= 2000 else None
    except ValueError:
        return None


def log_rows(log):
    for line in log.decode("utf-8-sig").splitlines():
        match = STAMP.fullmatch(line)
        if match:
            when = moment(match[1])
            if when is not None:
                yield when, match[2]


def parse_log(log, step):
    """Measure a successful step; reject partial, ambiguous or inconsistent logs.

API timestamps identify the step at second precision. Run-group timestamps give
the finer boundaries, including final cleanup up to the following step. Children
are observed recipe/cache intervals and are deliberately not summed.
"""
    if step.get("status") != "completed" or step.get("conclusion") != "success":
        return unavailable("step-not-successful")
    if not isinstance(log, bytes) or len(log) > MAX_LOG_BYTES:
        return unavailable("logs-too-large")
    start, end = moment(step.get("started_at")), moment(step.get("completed_at"))
    if not start or not end or end < start:
        return unavailable("invalid-timing")
    try:
        rows = [(when, ANSI.sub("", line)) for when, line in log_rows(log)]
    except UnicodeError:
        return unavailable("unsupported-log-format")
    groups = [(i, when) for i, (when, text) in enumerate(rows) if text.startswith("##[group]Run ")]
    starts = [(i, when) for i, when in groups if start <= when < start + timedelta(seconds=1)]
    if len(starts) != 1:
        return unavailable("unsupported-log-format")
    first, log_start = starts[0]
    following = [(i, when) for i, when in groups if i > first]
    if not following:
        return unavailable("incomplete-log")
    last, log_end = following[0]
    if not end <= log_end < end + timedelta(seconds=1):
        return unavailable("incomplete-log")
    previous = log_start
    sections, active, counts = [], {}, Counter()
    active_pairs = {}
    current = None
    task_count = 0
    active_phases = {}
    for when, text in rows[first:last]:
        # Phase marker failures affect only the optional inner measurement. A
        # marker is associated with the exact currently building image target;
        # stale markers outside that interval cannot be reused by later work.
        if text.startswith("[ image phases ]"):
            marker = IMAGE_PHASE_MARKER.fullmatch(text)
            identity = marker[1] if marker else next(
                (target for target in FINAL_IMAGE_TARGETS if text.startswith(f"[ image phases ] [ {target} ]")), None)
            active_phase = active_phases.get(identity)
            if active_phase is not None and identity in active:
                if marker:
                    active_phase["seen"] += 1
                    if active_phase["seen"] > 1 or when < previous or when > log_end:
                        active_phase["error"] = "invalid-timing"
                    else:
                        value, error = read_phase_payload(marker[2])
                        active_phase.update(value=value, error=error, when=when)
                else:
                    active_phase.update(seen=1, error="invalid-timing")
            continue
        for active_phase in active_phases.values():
            if active_phase["when"] is not None and when < active_phase["when"]:
                active_phase["error"] = "invalid-timing"
        if when < previous or when > log_end:
            return unavailable("invalid-timing")
        previous = when
        pair = IMAGE_PAIR_MARKER.fullmatch(text)
        if pair:
            identity, runtime = pair.groups()
            if (identity in active and runtime in IMAGE_PAIR_KINDS and
                    identity in (runtime, runtime.removesuffix(".gz") + "-dbg.gz")):
                active_pairs[identity] = IMAGE_PAIR_KINDS[runtime]
            continue
        outer = OUTER.fullmatch(text)
        if outer:
            kind = OUTER_TARGETS.get(outer[1]) or target_kind(outer[1])
            kind = "vs-image" if kind == "final-image" else kind
            if kind not in SECTION_LABELS:
                return unavailable("unsupported-log-format")
            if active:
                return unavailable("incomplete-log")
            if not sections and kind != "vs-image":
                return unavailable("incomplete-log")
            if current is not None:
                current["duration_seconds"] = round((when - current.pop("start")).total_seconds(), 6)
            counts[kind] += 1
            section_start = log_start if not sections else when
            current = {"id": f"{kind}-{counts[kind]}", "label": SECTION_LABELS[kind],
                       "start": section_start, "offset_seconds": round((section_start - log_start).total_seconds(), 6),
                       "duration_seconds": None, "tasks": []}
            sections.append(current)
            if len(sections) > MAX_SECTIONS:
                return unavailable("unsupported-log-format")
            continue
        target = TARGET.fullmatch(text)
        if not target:
            continue
        outcome, identity = target.groups()
        kind = target_kind(identity)
        if kind is None:
            continue
        if current is None:
            return unavailable("incomplete-log")
        if outcome == "building":
            if identity in active:
                return unavailable("incomplete-log")
            active[identity] = when
            if identity in FINAL_IMAGE_TARGETS:
                active_phases[identity] = {"seen": 0, "error": None, "value": None, "when": None}
        else:
            began = active.pop(identity, None)
            if began is None:
                return unavailable("incomplete-log")
            task_count += 1
            if task_count > MAX_TASKS:
                return unavailable("unsupported-log-format")
            kind = active_pairs.pop(identity, kind)
            count = sum(task["id"].rsplit("-", 1)[0] == kind for task in current["tasks"]) + 1
            task = {"id": f"{kind}-{count}", "label": TASK_LABELS[kind],
                    "offset_seconds": round((began - current["start"]).total_seconds(), 6),
                    "duration_seconds": round((when - began).total_seconds(), 6), "outcome": outcome}
            if identity in FINAL_IMAGE_TARGETS:
                task["phase_breakdown"] = complete_phase_breakdown(active_phases.pop(identity), began, when, outcome)
            current["tasks"].append(task)
    if not current or active:
        return unavailable("incomplete-log")
    if not any(task["id"].startswith("final-image-") for section in sections for task in section["tasks"]):
        return unavailable("incomplete-log")
    current["duration_seconds"] = round((log_end - current.pop("start")).total_seconds(), 6)
    for section in sections:
        section["tasks"].sort(key=lambda task: (task["offset_seconds"], task["id"]))
    result = {"version": VERSION, "status": "available", "reason": None, "source": SOURCE,
              "duration_seconds": round((log_end - log_start).total_seconds(), 6), "sections": sections}
    return result if valid(result) else unavailable("invalid-timing")


def finite_seconds(value):
    return (isinstance(value, (float, int)) and not isinstance(value, bool)
            and 0 <= value <= 2 ** 53 - 1 and math.isfinite(value))


def known_identity(value, labels):
    if not isinstance(value, str):
        return None
    kind, separator, ordinal = value.rpartition("-")
    return kind if separator and kind in labels and re.fullmatch(r"[1-9][0-9]{0,2}", ordinal) else None


def valid(value):
    """Strict retained-data allowlist, including safe static labels and bounds."""
    if (not isinstance(value, dict) or set(value) != {"version", "status", "reason", "source", "duration_seconds", "sections"}
            or type(value["version"]) is not int or value["version"] != VERSION or value["source"] != SOURCE
            or not isinstance(value["sections"], list)):
        return False
    if value["status"] == "unavailable":
        return (isinstance(value["reason"], str) and value["reason"] in REASONS
                and value["duration_seconds"] is None and value["sections"] == [])
    if (value["status"] != "available" or value["reason"] is not None
            or not finite_seconds(value["duration_seconds"]) or not 0 < len(value["sections"]) <= MAX_SECTIONS):
        return False
    section_ids, cursor, total_tasks = set(), 0, 0
    for section in value["sections"]:
        if (not isinstance(section, dict) or set(section) != {"id", "label", "offset_seconds", "duration_seconds", "tasks"}
                or not isinstance(section["tasks"], list)):
            return False
        kind = known_identity(section["id"], SECTION_LABELS)
        if (not kind or section["id"] in section_ids or section["label"] != SECTION_LABELS[kind]
                or not finite_seconds(section["offset_seconds"]) or not finite_seconds(section["duration_seconds"])
                or abs(section["offset_seconds"] - cursor) > 0.00001):
            return False
        section_ids.add(section["id"])
        cursor = section["offset_seconds"] + section["duration_seconds"]
        task_ids, previous = set(), 0
        total_tasks += len(section["tasks"])
        if total_tasks > MAX_TASKS:
            return False
        for task in section["tasks"]:
            required = {"id", "label", "offset_seconds", "duration_seconds", "outcome"}
            if not isinstance(task, dict) or set(task) - {"phase_breakdown"} != required:
                return False
            kind = known_identity(task["id"], TASK_LABELS)
            if (not kind or task["id"] in task_ids or task["label"] != TASK_LABELS[kind]
                    or task["outcome"] not in ("finished", "cached") or not finite_seconds(task["offset_seconds"])
                    or not finite_seconds(task["duration_seconds"]) or task["offset_seconds"] < previous
                    or task["offset_seconds"] + task["duration_seconds"] > section["duration_seconds"] + 0.00001):
                return False
            if "phase_breakdown" in task and (kind != "final-image" or not valid_phase_breakdown(
                    task["phase_breakdown"], task["duration_seconds"], task["outcome"])):
                return False
            task_ids.add(task["id"])
            previous = task["offset_seconds"]
    return abs(cursor - value["duration_seconds"]) <= 0.00001
