#!/usr/bin/env python3

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Parse and format switch-CPU reboot-cause gNMI JSON_IETF payloads."""

from __future__ import annotations

import argparse
import json
import sys
from typing import Any


def extract_json_ietf_payload(gnmi_text: str) -> dict[str, Any]:
    """Extract and decode the json_ietf_val field from gnmi_get stdout."""
    marker = "json_ietf_val:"
    idx = gnmi_text.find(marker)
    if idx == -1:
        raise ValueError("no json_ietf_val found in gNMI response")

    line = gnmi_text[idx + len(marker):].splitlines()[0].strip()
    try:
        inner, _end = json.JSONDecoder().raw_decode(line)
        if not isinstance(inner, str):
            raise ValueError("json_ietf_val is not a JSON string")
        return json.loads(inner)
    except json.JSONDecodeError as exc:
        raise ValueError(f"failed to parse reboot-cause JSON: {exc}") from exc


def format_reboot_cause(data: dict[str, Any]) -> str:
    """Format a flat previous-reboot-cause dict as human-readable text."""
    order = ["cause", "time", "user", "comment", "gen_time"]
    labels = {
        "cause": "Cause",
        "time": "Time",
        "user": "User",
        "comment": "Comment",
        "gen_time": "Generated",
    }
    lines: list[str] = []
    seen: set[str] = set()

    for key in order:
        if key in data:
            lines.append(f"{labels.get(key, key):10}: {data[key]}")
            seen.add(key)

    for key, value in data.items():
        if key not in seen:
            lines.append(f"{key:10}: {value}")

    return "\n".join(lines)


def format_reboot_cause_history(data: dict[str, Any]) -> str:
    """Format reboot-cause history keyed by timestamp/name."""
    cols = [
        ("name", "Name"),
        ("cause", "Cause"),
        ("time", "Time"),
        ("user", "User"),
        ("comment", "Comment"),
    ]
    rows: list[dict[str, str]] = []

    for name in sorted(data.keys(), reverse=True):
        entry = data[name]
        if not isinstance(entry, dict):
            continue
        row = {"name": name}
        for key, _ in cols[1:]:
            row[key] = str(entry.get(key, ""))
        rows.append(row)

    if not rows:
        return ""

    widths = {
        key: max([len(header)] + [len(row[key]) for row in rows])
        for key, header in cols
    }
    fmt = "  ".join("%-" + str(widths[key]) + "s" for key, _ in cols)
    lines = [
        fmt % tuple(header for _, header in cols),
        fmt % tuple("-" * widths[key] for key, _ in cols),
    ]
    lines.extend(fmt % tuple(row[key] for key, _ in cols) for row in rows)
    return "\n".join(lines)


def render_reboot_cause(
    data: dict[str, Any], *, history: bool = False, raw_json: bool = False
) -> str:
    if raw_json:
        return json.dumps(data, indent=2)

    if history:
        return format_reboot_cause_history(data)

    return format_reboot_cause(data)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Parse switch-CPU reboot-cause gNMI JSON_IETF output"
    )
    parser.add_argument(
        "-f",
        "--response-file",
        required=True,
        help="File containing gnmi_get stdout",
    )
    parser.add_argument(
        "-H",
        "--history",
        action="store_true",
        help="Format reboot-cause history instead of previous cause",
    )
    parser.add_argument(
        "-j",
        "--json",
        action="store_true",
        help="Print raw JSON instead of plain text",
    )
    args = parser.parse_args(argv)

    try:
        with open(args.response_file, "r", encoding="utf-8") as handle:
            gnmi_text = handle.read()
        data = extract_json_ietf_payload(gnmi_text)
        print(render_reboot_cause(data, history=args.history, raw_json=args.json))
    except (OSError, ValueError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
