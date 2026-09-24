#!/usr/bin/env python3
"""
sbom_vuln_diff.py — compare two vulnerability reports (produced by
sbom_vuln_scan.py) and report which CVEs were added, removed, or
changed status.

Usage:
    sbom_vuln_diff.py old.vuln.json new.vuln.json
    sbom_vuln_diff.py --json out.json --quiet old.vuln.json new.vuln.json

Intended for release-cycle drift analysis: "what new CVEs apply to my
SBOM since the last release?" without rebuilding the image. Also used
to verify VEX suppressions: same SBOM, before vs after adding new VEX
documents.
"""

import argparse
import json
import sys


_SEVERITY_RANK = {
    "negligible": 0, "low": 1, "medium": 2, "high": 3, "critical": 4,
    "unknown": -1,
}


def load_vulns(path: str) -> dict:
    """Return { (cve_id, affected_ref): vuln-dict } for one report."""
    with open(path) as f:
        doc = json.load(f)
    out = {}
    for v in doc.get("vulnerabilities", []) or []:
        cve = v.get("id", "?")
        for affects in v.get("affects", []) or [{}]:
            ref = affects.get("ref", "?")
            out[(cve, ref)] = v
    return out


def severity(v: dict) -> str:
    ratings = v.get("ratings") or []
    if not ratings:
        return "unknown"
    return (ratings[0].get("severity") or "unknown").lower()


def rank(s: str) -> int:
    return _SEVERITY_RANK.get(s, -1)


def vuln_status(v: dict) -> str:
    """Detect VEX-applied status (CycloneDX 1.6 analysis.state)."""
    a = v.get("analysis") or {}
    return a.get("state", "")


def diff(old: dict, new: dict) -> dict:
    added = []
    removed = []
    status_changed = []
    severity_changed = []
    for key in sorted(new.keys() - old.keys()):
        added.append({"key": list(key), "vuln": new[key]})
    for key in sorted(old.keys() - new.keys()):
        removed.append({"key": list(key), "vuln": old[key]})
    for key in sorted(new.keys() & old.keys()):
        o, n = old[key], new[key]
        if severity(o) != severity(n):
            severity_changed.append({
                "key": list(key),
                "old": severity(o),
                "new": severity(n),
            })
        if vuln_status(o) != vuln_status(n):
            status_changed.append({
                "key": list(key),
                "old": vuln_status(o) or "(unsuppressed)",
                "new": vuln_status(n) or "(unsuppressed)",
            })
    return {
        "added": added,
        "removed": removed,
        "severity_changed": severity_changed,
        "status_changed": status_changed,
    }


def print_report(d: dict, old_path: str, new_path: str) -> None:
    print(f"Comparing vuln reports:")
    print(f"  old: {old_path}")
    print(f"  new: {new_path}")
    print()
    print(f"Added:            {len(d['added'])}")
    print(f"Removed:          {len(d['removed'])}")
    print(f"Severity changed: {len(d['severity_changed'])}")
    print(f"Status changed:   {len(d['status_changed'])}")
    if d["added"]:
        print()
        print("--- Newly present in NEW (regressions or newly disclosed) ---")
        # Sort by severity desc.
        items = sorted(
            d["added"],
            key=lambda x: -rank(severity(x["vuln"])),
        )
        for x in items[:50]:
            sev = severity(x["vuln"])
            cve, ref = x["key"]
            print(f"  + [{sev:8}] {cve:22} {ref}")
        if len(d["added"]) > 50:
            print(f"  ... and {len(d['added']) - 50} more")
    if d["removed"]:
        print()
        print("--- No longer present in NEW (fixed, removed, or VEX'd) ---")
        for x in d["removed"][:30]:
            cve, ref = x["key"]
            print(f"  - {cve:22} {ref}")
        if len(d["removed"]) > 30:
            print(f"  ... and {len(d['removed']) - 30} more")
    if d["status_changed"]:
        print()
        print("--- VEX status flips ---")
        for x in d["status_changed"][:30]:
            cve, ref = x["key"]
            print(f"  ~ {cve}  ({x['old']} -> {x['new']})  {ref}")
    if d["severity_changed"]:
        print()
        print("--- Severity reclassified ---")
        for x in d["severity_changed"][:30]:
            cve, ref = x["key"]
            print(f"  ~ {cve}  ({x['old']} -> {x['new']})  {ref}")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("old", help="Older / baseline .vuln.json")
    ap.add_argument("new", help="Newer / candidate .vuln.json")
    ap.add_argument("--quiet", action="store_true")
    ap.add_argument("--json",
                    help="Write the diff as JSON to this file")
    args = ap.parse_args()

    old = load_vulns(args.old)
    new = load_vulns(args.new)
    d = diff(old, new)

    if args.json:
        with open(args.json, "w") as f:
            json.dump(d, f, indent=2, sort_keys=True)

    if args.quiet:
        print(f"+{len(d['added'])} "
              f"-{len(d['removed'])} "
              f"~sev:{len(d['severity_changed'])} "
              f"~status:{len(d['status_changed'])}")
    else:
        print_report(d, args.old, args.new)

    if d["added"] or d["removed"] or d["severity_changed"] or d["status_changed"]:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
