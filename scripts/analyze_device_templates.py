#!/usr/bin/env python3
"""
Analyze duplication and conflict hotspots for device template files.

This script scans the SONiC device tree and reports how much template/config
content is duplicated across HWSKUs and where platform-level variant conflicts
exist.

Scanned filenames:
  - buffers.json.j2
  - qos.json.j2
  - hwsku.json
  - sai.profile
  - pg_profile_lookup.ini

The analysis is read-only. It never modifies files.

Usage examples:
  python3 scripts/analyze_device_templates.py
  python3 scripts/analyze_device_templates.py --top-groups 8
  python3 scripts/analyze_device_templates.py --json
  python3 scripts/analyze_device_templates.py --json --json-indent 2
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
from collections import defaultdict
from dataclasses import dataclass
from datetime import datetime, timezone
from typing import Dict, Iterable, List, Tuple


TARGET_FILENAMES: Tuple[str, ...] = (
    "buffers.json.j2",
    "qos.json.j2",
    "hwsku.json",
    "sai.profile",
    "pg_profile_lookup.ini",
)

DEFAULT_TOP_GROUPS = 5
DEFAULT_TOP_HOTSPOTS = 10


@dataclass(frozen=True)
class FileRecord:
    target_type: str
    rel_path: str
    platform: str
    content_hash: str


def normalize_line_endings(raw: bytes) -> bytes:
    """Normalize CRLF/CR to LF to avoid false hash differences."""
    return raw.replace(b"\r\n", b"\n").replace(b"\r", b"\n")


def content_hash_for_file(path: str) -> str:
    """Return SHA256 hash of file content with normalized line endings."""
    with open(path, "rb") as file_obj:
        normalized = normalize_line_endings(file_obj.read())
    return hashlib.sha256(normalized).hexdigest()


def infer_platform(rel_path: str) -> str:
    """
    Infer platform key from relative path.

    Expected layout: device/<vendor>/<platform>/<hwsku>/<target_file>
    Returned key:   device/<vendor>/<platform>
    """
    parts = rel_path.replace("\\", "/").split("/")
    if len(parts) >= 3 and parts[0] == "device":
        return "/".join(parts[:3])
    return "unknown"


def collect_records(device_root: str) -> List[FileRecord]:
    """Scan device tree and return records for target filenames."""
    records: List[FileRecord] = []
    target_set = set(TARGET_FILENAMES)

    for root, dirnames, filenames in os.walk(device_root):
        # Sort traversal state to keep output deterministic across filesystems.
        dirnames.sort()
        for filename in sorted(filenames):
            if filename not in target_set:
                continue

            full_path = os.path.join(root, filename)
            rel_path = os.path.relpath(full_path, start=os.getcwd()).replace("\\", "/")
            records.append(
                FileRecord(
                    target_type=filename,
                    rel_path=rel_path,
                    platform=infer_platform(rel_path),
                    content_hash=content_hash_for_file(full_path),
                )
            )

    return records


def group_records_by_type(records: Iterable[FileRecord]) -> Dict[str, List[FileRecord]]:
    by_type: Dict[str, List[FileRecord]] = {name: [] for name in TARGET_FILENAMES}
    for record in records:
        by_type[record.target_type].append(record)
    return by_type


def build_type_report(records: List[FileRecord], top_groups: int, top_hotspots: int) -> Dict[str, object]:
    hash_groups: Dict[str, List[FileRecord]] = defaultdict(list)
    platform_hashes: Dict[str, set] = defaultdict(set)
    platform_file_counts: Dict[str, int] = defaultdict(int)

    for record in records:
        hash_groups[record.content_hash].append(record)
        platform_hashes[record.platform].add(record.content_hash)
        platform_file_counts[record.platform] += 1

    total_files = len(records)
    unique_variants = len(hash_groups)
    duplicated_files = total_files - unique_variants
    duplication_ratio = (duplicated_files / total_files) if total_files else 0.0

    largest_duplicate_groups: List[Dict[str, object]] = []
    sorted_groups = sorted(hash_groups.items(), key=lambda item: len(item[1]), reverse=True)
    for content_hash, group_records in sorted_groups:
        if len(group_records) <= 1:
            continue
        largest_duplicate_groups.append(
            {
                "hash": content_hash,
                "count": len(group_records),
                "sample_paths": [r.rel_path for r in group_records[:3]],
            }
        )
        if len(largest_duplicate_groups) >= top_groups:
            break

    hotspots: List[Dict[str, object]] = []
    for platform, hashes in platform_hashes.items():
        variant_count = len(hashes)
        if variant_count <= 1:
            continue
        hotspots.append(
            {
                "platform": platform,
                "variants": variant_count,
                "files": platform_file_counts[platform],
            }
        )

    hotspots.sort(key=lambda item: (-item["variants"], -item["files"], item["platform"]))
    hotspots = hotspots[:top_hotspots]

    return {
        "total_files": total_files,
        "unique_content_variants": unique_variants,
        "duplicated_files": duplicated_files,
        "duplication_ratio": duplication_ratio,
        "largest_duplicate_groups": largest_duplicate_groups,
        "conflict_hotspots": hotspots,
    }


def build_report(records: List[FileRecord], top_groups: int, top_hotspots: int) -> Dict[str, object]:
    by_type = group_records_by_type(records)
    per_type = {
        target_type: build_type_report(type_records, top_groups=top_groups, top_hotspots=top_hotspots)
        for target_type, type_records in by_type.items()
    }

    overall_total = sum(report["total_files"] for report in per_type.values())
    overall_unique = sum(report["unique_content_variants"] for report in per_type.values())
    overall_duplication_ratio = (
        (overall_total - overall_unique) / overall_total if overall_total else 0.0
    )

    return {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "device_root": "device",
        "targets": list(TARGET_FILENAMES),
        "overall": {
            "total_files": overall_total,
            "unique_content_variants": overall_unique,
            "duplicated_files": overall_total - overall_unique,
            "duplication_ratio": overall_duplication_ratio,
        },
        "per_target": per_type,
    }


def print_console_report(report: Dict[str, object]) -> None:
    print("=== Device Template Duplication Analysis ===")
    print(f"Generated (UTC): {report['generated_at_utc']}")
    print(f"Device root: {report['device_root']}")
    print(f"Targets: {', '.join(report['targets'])}")
    print("")

    overall = report["overall"]
    print("Overall")
    print(f"  Total files: {overall['total_files']}")
    print(f"  Unique variants: {overall['unique_content_variants']}")
    print(f"  Duplicated files: {overall['duplicated_files']}")
    print(f"  Duplication ratio: {overall['duplication_ratio'] * 100:.2f}%")
    print("")

    per_target = report["per_target"]
    for target_type in TARGET_FILENAMES:
        section = per_target[target_type]
        print(f"Target: {target_type}")
        print(f"  Total files: {section['total_files']}")
        print(f"  Unique variants: {section['unique_content_variants']}")
        print(f"  Duplicated files: {section['duplicated_files']}")
        print(f"  Duplication ratio: {section['duplication_ratio'] * 100:.2f}%")

        groups = section["largest_duplicate_groups"]
        if groups:
            print("  Largest duplicate groups:")
            for group in groups:
                preview = ", ".join(group["sample_paths"]) if group["sample_paths"] else "n/a"
                print(
                    f"    - {group['count']} files share {group['hash'][:12]}... "
                    f"(examples: {preview})"
                )
        else:
            print("  Largest duplicate groups: none")

        hotspots = section["conflict_hotspots"]
        if hotspots:
            print("  Conflict hotspots (same platform, multiple variants):")
            for hotspot in hotspots:
                print(
                    f"    - {hotspot['platform']}: variants={hotspot['variants']}, files={hotspot['files']}"
                )
        else:
            print("  Conflict hotspots: none")

        print("")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Analyze duplication/conflict statistics for device template files.",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Print machine-readable JSON report instead of console summary.",
    )
    parser.add_argument(
        "--json-indent",
        type=int,
        default=2,
        help="JSON indentation level when using --json (default: 2).",
    )
    parser.add_argument(
        "--top-groups",
        type=int,
        default=DEFAULT_TOP_GROUPS,
        help=f"Number of largest duplicate groups to show (default: {DEFAULT_TOP_GROUPS}).",
    )
    parser.add_argument(
        "--top-hotspots",
        type=int,
        default=DEFAULT_TOP_HOTSPOTS,
        help=(
            "Number of per-platform conflict hotspots to show per target "
            f"(default: {DEFAULT_TOP_HOTSPOTS})."
        ),
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.top_groups < 1 or args.top_hotspots < 1:
        print("error: --top-groups and --top-hotspots must be >= 1", file=sys.stderr)
        return 2
    if args.json_indent < 0:
        print("error: --json-indent must be >= 0", file=sys.stderr)
        return 2

    device_root = os.path.join(os.getcwd(), "device")
    if not os.path.isdir(device_root):
        print("error: expected to run from repository root containing 'device/'", file=sys.stderr)
        return 2

    records = collect_records(device_root)
    report = build_report(records, top_groups=args.top_groups, top_hotspots=args.top_hotspots)

    if args.json:
        print(json.dumps(report, indent=args.json_indent, sort_keys=False))
    else:
        print_console_report(report)

    return 0


if __name__ == "__main__":
    sys.exit(main())
