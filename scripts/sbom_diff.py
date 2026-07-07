#!/usr/bin/env python3
"""
sbom_diff.py — compare two SBOM files (CycloneDX JSON) and report
component-level differences. Used as a reproducibility cross-check
between independent build hosts of the same source tree, and as a
release-drift report between two SBOMs over time.

Usage:
    sbom_diff.py [--quiet] [--json out.json] OLD.cdx.json NEW.cdx.json

Default output is human-readable. With --json, emit a machine-readable
report covering added/removed/changed components and an exit code
that's non-zero when any difference exists (useful for CI gating).
"""

import argparse
import json
import sys


def load_components(path: str) -> dict:
    """Return {bom-ref or purl or (name, version, arch): component}."""
    with open(path) as f:
        doc = json.load(f)
    out: dict = {}
    for c in doc.get("components", []) or []:
        key = c.get("bom-ref") or c.get("purl") or c.get("name", "?")
        out[key] = c
    return out


def component_signature(c: dict) -> dict:
    """The fields we compare for "did this component change?".

    We deliberately ignore timestamps, scan-tool internal metadata, and
    the SBOM aggregator's own properties so a 'no real change' rebuild
    doesn't flag drift. Things that matter:
      - version
      - hashes
      - licenses
      - pedigree.ancestors[].version
      - pedigree.patches.length and per-patch hashes
    """
    sig: dict = {
        "name": c.get("name"),
        "version": c.get("version"),
        "type": c.get("type"),
    }
    hashes = c.get("hashes") or []
    if hashes:
        sig["hashes"] = sorted(
            (h.get("alg"), h.get("content")) for h in hashes
        )
    licenses = c.get("licenses") or []
    if licenses:
        lic_strs = []
        for l in licenses:
            if "expression" in l:
                lic_strs.append(("expr", l["expression"]))
            elif "license" in l:
                lic = l["license"]
                lic_strs.append((
                    "id" if "id" in lic else "name",
                    lic.get("id") or lic.get("name") or "",
                ))
        sig["licenses"] = sorted(lic_strs)
    ped = c.get("pedigree") or {}
    if ped.get("ancestors"):
        sig["ancestors"] = sorted(
            (a.get("name"), a.get("version")) for a in ped["ancestors"]
        )
    if ped.get("patches"):
        sig["patches"] = sorted(
            tuple(
                h.get("content", "")
                for h in (p.get("diff") or {}).get("hashes") or []
            )
            for p in ped["patches"]
        )
    return sig


def diff(old: dict, new: dict) -> dict:
    added = sorted(k for k in new if k not in old)
    removed = sorted(k for k in old if k not in new)
    changed = []
    for k in sorted(old.keys() & new.keys()):
        s_old = component_signature(old[k])
        s_new = component_signature(new[k])
        if s_old != s_new:
            field_diff = {}
            for f in set(s_old) | set(s_new):
                if s_old.get(f) != s_new.get(f):
                    field_diff[f] = {
                        "old": s_old.get(f),
                        "new": s_new.get(f),
                    }
            changed.append({"key": k, "changes": field_diff})
    return {
        "added": added,
        "removed": removed,
        "changed": changed,
    }


def print_report(d: dict, old_path: str, new_path: str) -> None:
    print(f"Comparing:")
    print(f"  old: {old_path}")
    print(f"  new: {new_path}")
    print()
    print(f"Added:   {len(d['added'])}")
    print(f"Removed: {len(d['removed'])}")
    print(f"Changed: {len(d['changed'])}")
    if d["added"]:
        print()
        print("--- Added ---")
        for k in d["added"][:50]:
            print(f"  + {k}")
        if len(d["added"]) > 50:
            print(f"  ... and {len(d['added']) - 50} more")
    if d["removed"]:
        print()
        print("--- Removed ---")
        for k in d["removed"][:50]:
            print(f"  - {k}")
        if len(d["removed"]) > 50:
            print(f"  ... and {len(d['removed']) - 50} more")
    if d["changed"]:
        print()
        print("--- Changed ---")
        for entry in d["changed"][:30]:
            print(f"  ~ {entry['key']}")
            for field, vals in sorted(entry["changes"].items()):
                old_v = vals.get("old")
                new_v = vals.get("new")
                if isinstance(old_v, list):
                    old_v = f"[{len(old_v)} items]"
                if isinstance(new_v, list):
                    new_v = f"[{len(new_v)} items]"
                print(f"      {field}: {old_v} -> {new_v}")
        if len(d["changed"]) > 30:
            print(f"  ... and {len(d['changed']) - 30} more")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("old", help="Older / baseline SBOM (CycloneDX JSON)")
    ap.add_argument("new", help="Newer / candidate SBOM (CycloneDX JSON)")
    ap.add_argument("--quiet", action="store_true",
                    help="Suppress per-component output; just report counts")
    ap.add_argument("--json",
                    help="Write the full diff as JSON to this file")
    args = ap.parse_args()

    old = load_components(args.old)
    new = load_components(args.new)
    d = diff(old, new)

    if args.json:
        with open(args.json, "w") as f:
            json.dump(d, f, indent=2, sort_keys=True)

    if args.quiet:
        print(
            f"+{len(d['added'])} -{len(d['removed'])} ~{len(d['changed'])}"
        )
    else:
        print_report(d, args.old, args.new)

    # Non-zero exit when any difference; useful for `make sbom-verify`.
    if d["added"] or d["removed"] or d["changed"]:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
