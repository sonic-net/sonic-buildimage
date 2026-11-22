"""Reduce public Make job logs to bounded timing measurements.

Publish timestamps, numeric metrics, fixed outcomes and validated build target
identifiers. Command lines, paths and free-form log messages stay out of history.
"""
from __future__ import annotations

from collections import Counter
from datetime import datetime, timedelta
from itertools import islice
import json
import math
import re

VERSION = 1
PARSER_REVISION = 3
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
IMAGE_CONTEXTS = {"recipe", "filesystem", "installer", "bios", "uefi", "4-asic", "6-asic",
                  "docker-load", "docker-store", "payload"}
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
    "vs-container-images": "Bazel container images",
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
VS_ROOTFS_TARGET = "target/sonic-vs.bin__vs__rfs.squashfs"
EXACT_TARGETS = {
    "target/vs-container-images": "vs-container-images",
    "target/sonic-vs.bin": "final-image",
    VS_ROOTFS_TARGET: "rootfs",
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
    "orchagent-pair", "syncd-pair", "vs-container-images",
}:
    EXACT_TARGETS[f"target/docker-{_kind}.gz"] = _kind

# Explicit pipeline spans emitted by the VS Make build. Only the stable IDs and
# their measured boundaries are retained, never arbitrary command output.
VS_TIMING = re.compile(r"\[ vs timing \] \[ ([a-z0-9-]+) \] \[ (start|end) \]\Z")
VS_ROOTFS_TIMING_LABELS = {
    "rootfs-prepare": "Prepare the stage-one root filesystem",
    "rootfs-compress": "Compress the intermediate squashfs",
    "rootfs-cache-save": "Save the intermediate squashfs to cache",
}
VS_TIMING_LABELS = {
    "kernel-prep": "Kernel package preparation",
    "p4rt-prep": "P4RT source and SDK preparation",
    "p4rt-audit": "P4RT dependency graph audit",
    "p4rt-sdk": "P4RT SDK inputs",
    "p4rt-owner": "P4RT source build",
    "p4rt-handoff": "P4RT artifact handoff",
    **VS_ROOTFS_TIMING_LABELS,
    "build-version-collection": "Collect build-version metadata",
    "builder-exit": "Exit the builder and return to the host",
    "trixie-image-cleanup": "Trixie image cleanup",
    "bookworm-cleanup": "Bookworm cleanup invocation"
}
TASK_LABELS.update(VS_TIMING_LABELS)
TASK_LABELS.update({
    "bookworm-make": "Bookworm Make invocation",
    "prep-deb": "Debian package build or cache lookup",
    "prep-deb-install": "Debian package install",
    "prep-wheel": "Python wheel build or cache lookup",
    "prep-wheel-install": "Python wheel install",
    "trixie-cache-eval": "Evaluate Trixie package cache",
    "container-manifest": "Generate container manifest",
    "device-data": "SONiC device data package",
    "config-engine-load": "Load the config-engine base image",
    "swss-layer-load": "Load the SWSS base image",
})

DISTRO_CONTAINER = re.compile(r"target/docker-(base|config-engine|swss-layer)-(bookworm|trixie)\.gz\Z")
BASE_IMAGE_LOAD = re.compile(r"target/docker-(config-engine|swss-layer)-(?:bookworm|trixie)\.gz-load\Z")
DEVICE_DATA = re.compile(r"target/debs/(?:trixie|bookworm)/sonic-device-data_[0-9][A-Za-z0-9.+~:-]{0,63}_all\.deb\Z")
P4RT = re.compile(r"target/files/(?:bookworm|trixie)/sonic-p4rt(-dbgsym)?_[0-9][A-Za-z0-9.+~:-]{0,63}_amd64\.tar\.bz2\Z")
P4RT_DEB = re.compile(r"target/debs/(?:bookworm|trixie)/sonic-p4rt(-dbgsym)?_[0-9][A-Za-z0-9.+~:-]{0,63}_amd64\.deb\Z")
STAMP = re.compile(r"^(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d(?:\.\d{1,9})?Z) (.*)$")
ANSI = re.compile(r"\x1b\[[0-9;]*m")
OUTER = re.compile(r"\+\+\+ --- Making (\S{1,256}) --- \+\+\+\s*\Z")
TARGET = re.compile(r"\[ (building|finished|cached) \] \[ (\S{1,256}) \]\s*\Z")
BOOKWORM_COMMAND = "./scripts/run_with_retry make EXTRA_DOCKER_TARGETS=sonic-vs.bin BLDENV=bookworm -f Makefile.work bookworm"
MAKE_DIRECTORY = re.compile(r"make\[1\]: (Entering|Leaving) directory '(/[^'\n]+)'\Z")
PREP_DEB = re.compile(r"target/debs/trixie/[a-z0-9][a-z0-9+.-]*_[A-Za-z0-9.+~:-]+_[a-z0-9-]+\.deb(-install)?\Z")
PREP_WHEEL = re.compile(r"target/python-wheels/trixie/[A-Za-z0-9][A-Za-z0-9_.+-]*\.whl(-install)?\Z")
MANIFEST_TIMING = re.compile(r"\[ vs timing \] \[ container-manifest \] \[ ([a-z0-9][a-z0-9_.-]{0,95}) \] \[ (start|end) \]\Z")
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
CONTAINER_MARKER = "SONIC_VS_CONTAINER_TIMINGS_V1"
CONTAINER_GROUP = "vs-container-images"
CONTAINER_METRICS = {"action_seconds", "fetch_seconds", "archive_seconds", "compress_seconds"}
CONTAINER_TOTALS = {"other_action_seconds", "no_action_seconds", "parallel_seconds"}
RECIPE_STEP_VERSIONS = {"Summarize VS image build timings": 1, "Summarize VS Bazel build timings": 2}
RECIPE_LISTS = {1: "images", 2: "recipes"}
RECIPE_OWNER = re.compile(r"[A-Za-z0-9][A-Za-z0-9_.+-]{0,191}\Z")
MAX_RECIPES = 1024
MAX_RECIPE_COMMANDS = 65535
RECIPE_COMMAND = "python3 -B tools/bazel/ci/summarize_vs_image_timings.py "
BUILD_PROFILE_METRICS = {
    "lock_seconds", "compile_seconds", "foreign_seconds", "fetch_seconds", "repository_seconds",
    "repository_read_seconds", "packaging_seconds", "cache_write_seconds",
    "package_selection_seconds", "validation_seconds",
    "arm64_repository_seconds", "armhf_repository_seconds",
}
EVIDENCE_GROUPS = {"repositories", "critical_path", "phases"}
REPOSITORY_NAME = re.compile(r"//:[A-Za-z0-9_.+~-]{1,188}\Z")
PUBLIC_TARGET = re.compile(
    r"(?:(?:@{1,2}[A-Za-z0-9_.+~-]+)?//[A-Za-z0-9_+./~-]*:[A-Za-z0-9_+./~-]+"
    r"|(?:platform|dockers|tools|src|external)/[A-Za-z0-9_+./~-]+)\Z")
CRITICAL_KINDS = {
    "OCIImage", "OCITarball", "OCITarballManifest", "Gzip", "APTDebug", "APTRuntime",
    "ValidatePackages", "ValidateSource", "GoCompilePkg", "GoLink", "GoStdlib",
    "GoToolchainBinaryBuild", "PackageControls", "Unpack", "KernelBuild", "KernelTools", "Genrule",
}


def valid_target(value):
    return (value is None or isinstance(value, str) and len(value) <= 240 and PUBLIC_TARGET.fullmatch(value)
            and not any(part in {".", ".."} for part in re.split(r"[/:]", value)))


def valid_named_evidence(value, row):
    if (not isinstance(value, dict)
            or set(value) != EVIDENCE_GROUPS | {"scope", "expected", "profiles", "phase_count"}
            or value["scope"] not in ("build", "kernel-build", "unavailable")
            or any(type(value[key]) is not int or not 0 <= value[key] <= MAX_RECIPE_COMMANDS * (
                2 if key == "phase_count" else 1)
                   for key in ("expected", "profiles", "phase_count"))
            or value["profiles"] > min(value["expected"], row["profiles"])
            or value["phase_count"] != row["bazel_phases"] + row["exports"]
            or any(not isinstance(value[key], list) or len(value[key]) > 8 for key in EVIDENCE_GROUPS)
            or len(value["phases"]) != min(8, value["phase_count"])):
        return False
    if value["scope"] == "build":
        build = row.get("build")
        if not build or any(value[key] != build[key] for key in ("expected", "profiles")):
            return False
    elif value["scope"] == "kernel-build":
        if row["owner"] != "linux-kernel" or row.get("build") is not None:
            return False
    elif value["expected"] or value["profiles"]:
        return False
    if not value["profiles"] and (value["repositories"] or value["critical_path"]):
        return False
    for span in value["repositories"]:
        if (not isinstance(span, dict) or set(span) != {"name", "seconds", "read_count", "read_seconds"}
                or not isinstance(span["name"], str) or not REPOSITORY_NAME.fullmatch(span["name"])
                or not finite_seconds(span["seconds"]) or not finite_seconds(span["read_seconds"])
                or span["read_seconds"] > span["seconds"] + 1e-6
                or type(span["read_count"]) is not int or not 0 <= span["read_count"] <= 1000000):
            return False
    for span in value["critical_path"]:
        if (not isinstance(span, dict) or set(span) != {"kind", "target", "seconds"}
                or not isinstance(span["kind"], str) or span["kind"] not in CRITICAL_KINDS
                or not valid_target(span["target"]) or not finite_seconds(span["seconds"])):
            return False
    for phase in value["phases"]:
        if (not isinstance(phase, dict)
                or set(phase) != {"phase", "seconds", "lock_seconds", "start_epoch_seconds", "exit_code"}
                or not isinstance(phase["phase"], str) or not RECIPE_OWNER.fullmatch(phase["phase"])
                or not finite_seconds(phase["seconds"])
                or phase["lock_seconds"] is not None and not finite_seconds(phase["lock_seconds"])
                or phase["start_epoch_seconds"] is not None and (
                    not finite_seconds(phase["start_epoch_seconds"]) or phase["start_epoch_seconds"] > 253402300799)
                or type(phase["exit_code"]) is not int or not 0 <= phase["exit_code"] <= 255):
            return False
    return True


def valid_build_evidence(value, row):
    if value is None:
        return True
    if (not isinstance(value, dict)
            or set(value) != {"expected", "timings", "profiles", "seconds"} | BUILD_PROFILE_METRICS):
        return False
    if (type(value["expected"]) is not int or not 1 <= value["expected"] <= row["bazel_expected"]
            or any(type(value[key]) is not int or not 0 <= value[key] <= min(value["expected"], row[parent])
                   for key, parent in (("timings", "bazel_phases"), ("profiles", "profiles")))):
        return False
    for count, metrics in (("timings", ("seconds",)), ("profiles", BUILD_PROFILE_METRICS)):
        if value[count]:
            if not all(finite_seconds(value[key]) for key in metrics):
                return False
        elif any(value[key] is not None for key in metrics):
            return False
    return (value["timings"] != value["expected"] or value["profiles"] != value["expected"]
            or value["lock_seconds"] <= value["seconds"])


def recipe_unavailable(reason, version=1):
    return {"version": version, "status": "unavailable", "reason": reason, RECIPE_LISTS[version]: []}


def container_unavailable(reason):
    return {"status": "unavailable", "reason": reason}


def valid_container_breakdown(value, recipes):
    if not isinstance(value, dict) or not any(row["owner"] == CONTAINER_GROUP for row in recipes):
        return False
    if set(value) == {"status", "reason"}:
        return (value["status"] == "unavailable" and isinstance(value["reason"], str)
                and value["reason"] in RECIPE_REASONS)
    if (set(value) != {"rows"} | CONTAINER_TOTALS
            or not all(finite_seconds(value[key]) for key in CONTAINER_TOTALS)
            or not isinstance(value["rows"], list) or not 0 < len(value["rows"]) <= MAX_RECIPES):
        return False
    members = {row["owner"] for row in recipes if row["owner"].endswith(".gz") and row["exports_expected"] > 0}
    seen = set()
    for row in value["rows"]:
        if (not isinstance(row, dict)
                or set(row) - {"compile_seconds", "total_seconds"} != {"owner", "actions"} | CONTAINER_METRICS
                or not isinstance(row["owner"], str) or row["owner"] not in members or row["owner"] in seen
                or type(row["actions"]) is not int or not finite_seconds(row["actions"])
                or not all(finite_seconds(row[key]) for key in CONTAINER_METRICS)):
            return False
        if "compile_seconds" in row and (not finite_seconds(row["compile_seconds"])
                                         or row["compile_seconds"] > row["action_seconds"] + 1e-6):
            return False
        if "total_seconds" in row and (not finite_seconds(row["total_seconds"])
                                       or row["total_seconds"] + 1e-6 < row["action_seconds"]):
            return False
        seen.add(row["owner"])
    return True


def valid_recipes(value):
    if not isinstance(value, dict) or type(value.get("version")) is not int or value["version"] not in RECIPE_LISTS:
        return False
    version = value["version"]
    key = RECIPE_LISTS[version]
    if value.get("status") == "unavailable":
        return (set(value) == {"version", "status", "reason", key} and value[key] == []
                and isinstance(value["reason"], str) and value["reason"] in RECIPE_REASONS)
    fields = set(value) - ({"container_breakdown"} if version == 2 else set())
    if (fields != {"version", "status", key} or value["status"] != "available"
            or not isinstance(value[key], list) or len(value[key]) > MAX_RECIPES):
        return False
    owners = set()
    fields = RECIPE_FIELDS if version == 1 else RECIPE_FIELDS | set(RECIPE_EXPECTED.values())
    for row in value[key]:
        if (not isinstance(row, dict) or set(row) - ({"build", "details"} if version == 2 else set()) != fields
                or type(row["failed"]) is not bool
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
        if "build" in row and not valid_build_evidence(row["build"], row):
            return False
        if "details" in row and not valid_named_evidence(row["details"], row):
            return False
    if "container_breakdown" in value:
        return valid_container_breakdown(value["container_breakdown"], value[key])
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
        elif output and line.startswith(CONTAINER_MARKER + " "):
            # Optional detail must never discard trustworthy outer recipe timings.
            try:
                if len(line.encode()) > 4096:
                    raise ValueError("oversized container marker")
                value = json.loads(line[len(CONTAINER_MARKER) + 1:], object_pairs_hook=unique_json_object,
                                   parse_constant=reject_json_constant)
            except (ValueError, RecursionError):
                value = None
            yield "container", value
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
    owners, evidence_seen = {}, set()
    members = []
    container_totals = None
    container_error = False
    result = None
    try:
        for marker_version, value in recipe_markers(log, start, end):
            if marker_version == "container":
                if not isinstance(value, dict) or container_error or container_totals is not None:
                    container_error = True
                elif set(value) == {"row"} and len(members) < MAX_RECIPES:
                    members.append(value["row"])
                elif (set(value) == {"count"} | CONTAINER_TOTALS and type(value["count"]) is int
                      and value["count"] == len(members)):
                    container_totals = {key: value[key] for key in CONTAINER_TOTALS}
                else:
                    container_error = True
                continue
            if (expected is not None and marker_version != expected) or result is not None:
                return recipe_unavailable("invalid-timing", version)
            expected = version = marker_version
            if marker_version == 1 and set(value) == {"version", "images"}:
                result = dict(value, status="available")
            elif marker_version == 2 and set(value) == {"recipe"} and len(rows) < MAX_RECIPES:
                row = value["recipe"]
                if not isinstance(row, dict) or not isinstance(row.get("owner"), str) or row["owner"] in owners:
                    return recipe_unavailable("invalid-timing", version)
                if "details" in row and (not isinstance(row["details"], dict)
                                        or any(row["details"].get(key) != [] for key in EVIDENCE_GROUPS)):
                    return recipe_unavailable("invalid-timing", version)
                rows.append(row)
                owners[row["owner"]] = row
            elif marker_version == 2 and set(value) == {"evidence"}:
                evidence = value["evidence"]
                if (not isinstance(evidence, dict) or set(evidence) != {"owner", "group", "values"}
                        or not isinstance(evidence["owner"], str) or not isinstance(evidence["group"], str)
                        or evidence["owner"] not in owners or evidence["group"] not in EVIDENCE_GROUPS
                        or "details" not in owners[evidence["owner"]]
                        or (evidence["owner"], evidence["group"]) in evidence_seen
                        or not isinstance(evidence["values"], list) or len(evidence["values"]) > 8):
                    return recipe_unavailable("invalid-timing", version)
                owners[evidence["owner"]]["details"][evidence["group"]] = evidence["values"]
                evidence_seen.add((evidence["owner"], evidence["group"]))
            elif (marker_version == 2 and set(value) in ({"count"}, {"count", "evidence_count"})
                  and type(value["count"]) is int and value["count"] == len(rows)
                  and (type(value.get("evidence_count", 0)) is int
                       and value.get("evidence_count", 0) == len(evidence_seen))
                  and evidence_seen == {(row["owner"], key) for row in rows if "details" in row
                                        for key in EVIDENCE_GROUPS}):
                result = {"version": 2, "status": "available", "recipes": rows}
            else:
                return recipe_unavailable("invalid-timing", version)
    except (UnicodeError, ValueError, RecursionError):
        return recipe_unavailable("invalid-timing", version)
    if result is None and not rows:
        return recipe_unavailable("not-recorded", version)
    if not valid_recipes(result):
        return recipe_unavailable("invalid-timing", version)
    if version == 2 and any(row["owner"] == CONTAINER_GROUP for row in rows):
        detail = None if container_totals is None else {"rows": members, **container_totals}
        if not container_error and valid_container_breakdown(detail, rows):
            result["container_breakdown"] = detail
        else:
            reason = "invalid-timing" if container_error or members or container_totals is not None else "not-recorded"
            result["container_breakdown"] = container_unavailable(reason)
    return result


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
    match = BASE_IMAGE_LOAD.fullmatch(target)
    if match:
        return f"{match[1]}-load"
    if DEVICE_DATA.fullmatch(target):
        return "device-data"
    match = P4RT.fullmatch(target) or P4RT_DEB.fullmatch(target)
    if match:
        return "p4rt-debug" if match[1] else "p4rt-runtime"
    return None


def unavailable(reason):
    if reason not in REASONS:
        raise ValueError("Unknown breakdown reason")
    return {"version": VERSION, "parser_revision": PARSER_REVISION,
            "status": "unavailable", "reason": reason, "source": SOURCE,
            "duration_seconds": None, "sections": []}


def retry_needed(value):
    # Revision 2 added support for previously unrecognized step boundaries.
    # Revision 3 adds optional tasks inside an already recognized installer.
    return (value is None or value.get("reason") in RETRY_REASONS
            or (value.get("reason") in {"unsupported-log-format", "incomplete-log"}
                and value.get("parser_revision", 1) < 2))


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
        match = STAMP.fullmatch(line.removeprefix("\ufeff"))
        if match:
            when = moment(match[1])
            if when is not None:
                yield when, match[2]


def installer_preparation_tasks(rows, section_start, tasks, budget):
    """Pair optional installer work while preserving space for all existing task measurements."""
    in_preparation = any(task["id"] == "vs-container-images-1" for task in tasks)
    result, active, counts = [], {}, Counter()
    previous_line = bookworm_directory = None
    for when, text in rows:
        if len(result) >= budget:
            break
        manifest = MANIFEST_TIMING.fullmatch(text)
        timer = VS_TIMING.fullmatch(text)
        directory = MAKE_DIRECTORY.fullmatch(text) if in_preparation else None
        preceded_by_bookworm = previous_line == BOOKWORM_COMMAND
        previous_line = text
        if manifest:
            kind, event = "container-manifest", manifest[2]
            key = (kind, manifest[1])
        elif timer and timer[1] == "trixie-cache-eval":
            kind, key, event = timer[1], timer[1], timer[2]
        elif directory:
            kind = key = "bookworm-make"
            if directory[1] == "Entering":
                # Only the top-level submake immediately after this specific
                # recipe identifies Bookworm. An unrelated entry invalidates it.
                active.pop(key, None)
                bookworm_directory = directory[2] if preceded_by_bookworm else None
                if bookworm_directory is None:
                    continue
                event = "start"
            else:
                matches = directory[2] == bookworm_directory
                bookworm_directory = None
                if not matches:
                    active.pop(key, None)
                    continue
                event = "end"
        elif in_preparation and (target := TARGET.fullmatch(text)):
            event, key = target.groups()
            if event == "building" and key == "target/vs-container-images":
                in_preparation = False
                continue
            if target_kind(key) is not None:
                continue
            if deb := PREP_DEB.fullmatch(key):
                kind = "prep-deb-install" if deb[1] else "prep-deb"
            elif wheel := PREP_WHEEL.fullmatch(key):
                kind = "prep-wheel-install" if wheel[1] else "prep-wheel"
            else:
                continue
        else:
            continue

        if event in ("start", "building"):
            if len(active) >= MAX_TASKS and key not in active:
                break
            active[key] = when if key not in active else None
            continue
        began = active.pop(key, None)
        if began is None:
            continue
        counts[kind] += 1
        result.append({"id": f"{kind}-{counts[kind]}", "label": TASK_LABELS[kind],
                       "offset_seconds": (began - section_start).total_seconds(),
                       "duration_seconds": (when - began).total_seconds(),
                       "outcome": "finished" if event == "end" else event})
    return result


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
    starts = [index for index, (_, when) in enumerate(groups)
              if start <= when < start + timedelta(seconds=1)]
    if not starts:
        return unavailable("unsupported-log-format")
    # Adjacent short steps can start in the same API timestamp second. Only
    # the group whose next boundary also matches this step's end is eligible.
    intervals = [(groups[index], groups[index + 1]) for index in starts
                 if index + 1 < len(groups)
                 and end <= groups[index + 1][1] < end + timedelta(seconds=1)]
    if not intervals:
        return unavailable("incomplete-log")
    if len(intervals) != 1:
        return unavailable("unsupported-log-format")
    (first, log_start), (last, log_end) = intervals[0]
    previous = log_start
    sections, active, counts = [], {}, Counter()
    active_pairs = {}
    current = None
    task_count = 0
    active_phases = {}
    timed_starts = {}
    installer_first, installer_last = None, last
    for index in range(first, last):
        when, text = rows[index]
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
            if not sections and outer[1] == "target/sonic-vs.bin":
                installer_first = index + 1
            elif len(sections) == 1:
                installer_last = index
            timed_starts.clear()
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
        timing = VS_TIMING.fullmatch(text)
        if timing:
            kind, boundary = timing.groups()
            if not current or current["label"] != SECTION_LABELS["vs-image"] or kind not in VS_TIMING_LABELS:
                continue
            if kind in VS_ROOTFS_TIMING_LABELS and VS_ROOTFS_TARGET not in active:
                continue
            if boundary == "start":
                timed_starts[kind] = when if kind not in timed_starts else None
                continue
            began = timed_starts.pop(kind, None)
            if began is None:
                continue
            identity, outcome = kind, "finished"
        else:
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
                continue
            if identity == VS_ROOTFS_TARGET:
                for name in VS_ROOTFS_TIMING_LABELS:
                    timed_starts.pop(name, None)
            began = active.pop(identity, None)
            if began is None:
                return unavailable("incomplete-log")
            kind = active_pairs.pop(identity, kind)
        task_count += 1
        if task_count > MAX_TASKS:
            return unavailable("unsupported-log-format")
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
    if installer_first is not None:
        tasks = sections[0]["tasks"]
        tasks.extend(installer_preparation_tasks(islice(rows, installer_first, installer_last),
                                                log_start, tasks, MAX_TASKS - task_count))
    for section in sections:
        section["tasks"].sort(key=lambda task: (task["offset_seconds"], task["id"]))
    result = {"version": VERSION, "parser_revision": PARSER_REVISION,
              "status": "available", "reason": None, "source": SOURCE,
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
    if (not isinstance(value, dict)
            or set(value) - {"parser_revision", "refresh_attempt_revision"} != {"version", "status", "reason", "source", "duration_seconds", "sections"}
            or type(value["version"]) is not int or value["version"] != VERSION or value["source"] != SOURCE
            or type(value.get("parser_revision", 1)) is not int
            or not 1 <= value.get("parser_revision", 1) <= PARSER_REVISION
            or not isinstance(value["sections"], list)):
        return False
    if "refresh_attempt_revision" in value:
        attempted = value["refresh_attempt_revision"]
        if (value["status"] != "available" or type(attempted) is not int
                or not value.get("parser_revision", 1) < attempted <= PARSER_REVISION):
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
