"""Reduce public Make job logs to bounded, static-label timing measurements.

Log contents are untrusted input. Nothing copied from a log becomes public text:
only timestamps, known target identities, and fixed outcomes survive parsing.
"""
from __future__ import annotations

from collections import Counter
from datetime import datetime, timedelta
import math
import re

VERSION = 1
SOURCE = "github-job-log"
MAX_LOG_BYTES = 32 * 1024 * 1024
MAX_SECTIONS = 32
MAX_TASKS = 512
REASONS = {
    "pending-collection", "step-not-successful", "logs-expired", "logs-too-large",
    "logs-temporarily-unavailable", "unsupported-log-format", "incomplete-log", "invalid-timing",
}
RETRY_REASONS = {"pending-collection", "logs-temporarily-unavailable"}
SECTION_LABELS = {
    "vs-image": "VS image, dependencies and cleanup",
    "orchagent-release": "Orchagent release",
    "orchagent-debug": "Orchagent debug",
    "syncd-release": "Syncd release",
    "syncd-debug": "Syncd debug",
    "p4rt-runtime": "P4RT runtime package",
    "p4rt-debug": "P4RT debug package",
}
TASK_LABELS = {
    "final-image": "Final VS image recipe",
    "rootfs": "Root filesystem squashfs",
    "base": "Base container",
    "config-engine": "Config-engine container",
    "orchagent-release": "Orchagent release container",
    "orchagent-debug": "Orchagent debug container",
    "syncd-release": "Syncd release container",
    "syncd-debug": "Syncd debug container",
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
}:
    EXACT_TARGETS[f"target/docker-{_kind}.gz"] = _kind

DISTRO_CONTAINER = re.compile(r"target/docker-(base|config-engine|swss-layer)-(bookworm|trixie)\.gz\Z")
P4RT = re.compile(r"target/files/(?:bookworm|trixie)/sonic-p4rt(-dbgsym)?_[0-9][A-Za-z0-9.+~:-]{0,63}_amd64\.tar\.bz2\Z")
STAMP = re.compile(r"^(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d(?:\.\d{1,9})?Z) (.*)$")
ANSI = re.compile(r"\x1b\[[0-9;]*m")
OUTER = re.compile(r"\+\+\+ --- Making (\S{1,256}) --- \+\+\+\s*\Z")
TARGET = re.compile(r"\[ (building|finished|cached) \] \[ (\S{1,256}) \]\s*\Z")


def target_kind(target):
    if target in EXACT_TARGETS:
        return EXACT_TARGETS[target]
    match = DISTRO_CONTAINER.fullmatch(target)
    if match:
        return match[1]
    match = P4RT.fullmatch(target)
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
        lines = log.decode("utf-8-sig").splitlines()
    except UnicodeError:
        return unavailable("unsupported-log-format")
    rows = []
    for line in lines:
        match = STAMP.fullmatch(line)
        if match:
            when = moment(match[1])
            if when is not None:
                rows.append((when, ANSI.sub("", match[2])))
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
    current = None
    task_count = 0
    for when, text in rows[first:last]:
        if when < previous or when > log_end:
            return unavailable("invalid-timing")
        previous = when
        outer = OUTER.fullmatch(text)
        if outer:
            kind = target_kind(outer[1])
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
        else:
            began = active.pop(identity, None)
            if began is None:
                return unavailable("incomplete-log")
            task_count += 1
            if task_count > MAX_TASKS:
                return unavailable("unsupported-log-format")
            count = sum(task["id"].rsplit("-", 1)[0] == kind for task in current["tasks"]) + 1
            current["tasks"].append({"id": f"{kind}-{count}", "label": TASK_LABELS[kind],
                                     "offset_seconds": round((began - current["start"]).total_seconds(), 6),
                                     "duration_seconds": round((when - began).total_seconds(), 6), "outcome": outcome})
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
            if not isinstance(task, dict) or set(task) != {"id", "label", "offset_seconds", "duration_seconds", "outcome"}:
                return False
            kind = known_identity(task["id"], TASK_LABELS)
            if (not kind or task["id"] in task_ids or task["label"] != TASK_LABELS[kind]
                    or task["outcome"] not in ("finished", "cached") or not finite_seconds(task["offset_seconds"])
                    or not finite_seconds(task["duration_seconds"]) or task["offset_seconds"] < previous
                    or task["offset_seconds"] + task["duration_seconds"] > section["duration_seconds"] + 0.00001):
                return False
            task_ids.add(task["id"])
            previous = task["offset_seconds"]
    return abs(cursor - value["duration_seconds"]) <= 0.00001
