#!/usr/bin/env python3
"""
sbom_extract_vex_from_patches.py — sweep the SONiC source tree for
patches that mention CVEs in their filename or header, and emit one
OpenVEX YAML file per (CVE, source-component) pair.

The goal: when a SONiC patch fixes an upstream CVE, that CVE should
**not** appear as an active finding in the vulnerability report for
the corresponding component, even though the underlying upstream
version in the SBOM ancestor pedigree is still vulnerable. OpenVEX
status `fixed` encodes that: the patch changed the vulnerable code, so
the artifact we ship carries the fix even though the version string
does not say so.

Where it looks — the directories SONiC keeps its own patches in, and
only the entries their `series` (or their recipe) actually applies:

    src/*.patch/                   (sidecar patch directories)
    src/*/patch/
    src/*/patches/
    src/sonic-linux-kernel/patches-sonic/

The same set sbom_fragment.py records in the SBOM's pedigree, via
sbom_cve_refs.patch_dirs(). Patches carried by upstream sources the
build unpacks — Debian's own debian/patches under src/<pkg>/<pkg>-<ver>/
— are deliberately not ours to make claims about, and are skipped.

What it looks for, in order of confidence:

    Filename:  contains 'cve-NNNN-NNNNN' (case-insensitive)
    Header:    'Fixes: CVE-NNNN-NNNNN'
    Header:    'Subject: ... CVE-NNNN-NNNNN ...'
    Anywhere:  'CVE-NNNN-NNNNN' (loose match, lower confidence)

Output:

    vex/auto/<source-tree-name>/<patch-basename>.json

OpenVEX JSON (not YAML — grype's --vex flag rejects YAML). One file
per patch, capturing every CVE it mentions. The product list is
derived from the source-tree directory name (i.e. `thrift` for
src/thrift/patch/*). Concrete PURLs are not emitted because the
extractor does not know which downstream debs/wheels each patch
ultimately ships in — grype's product matching handles this via PURL
substring matching.

Re-run is idempotent — files are overwritten with consistent ordering.

Usage:
    sbom_extract_vex_from_patches.py
    sbom_extract_vex_from_patches.py --output-dir vex/auto --dry-run
"""

import argparse
import datetime
import hashlib
import json
import os
import re
import sys

# Invoked with an arbitrary working directory, so locate the sibling
# module relative to this file rather than relying on cwd.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import sbom_cve_refs  # noqa: E402  (needs the path set above)


def warn(msg: str) -> None:
    sys.stderr.write(f"[sbom_extract_vex_from_patches.py] WARN: {msg}\n")


def info(msg: str) -> None:
    sys.stderr.write(f"[sbom_extract_vex_from_patches.py] {msg}\n")


# ---------------------------------------------------------------------------
# Patch discovery
# ---------------------------------------------------------------------------


def find_patches(root: str = "src") -> list:
    """Every patch SONiC maintains and applies, under ``root``.

    Both the directories and the patches within them come from
    sbom_cve_refs — the same answers sbom_fragment.py records in the
    pedigree — so the two describe one patch set by construction.

    Taking every directory that held a *.patch instead reached the
    Debian sources the build unpacks under src/<pkg>/, whose own
    debian/patches then produced statements reading "Fixed by SONiC
    local patch" about fixes that are Debian's and are already in the
    versions we ship. On one broadcom build that was 13 of the 17
    statements written, none of them ours to make.
    """
    found = []
    for d in sbom_cve_refs.patch_dirs_under(root):
        for fname in sbom_cve_refs.applied_patches(d):
            found.append(os.path.join(d, fname))
    return sorted(found)


# ---------------------------------------------------------------------------
# CVE extraction
# ---------------------------------------------------------------------------


def cves_in(path: str) -> tuple:
    """High- and low-confidence CVE ids claimed by a patch.

    Shared with sbom_fragment.py, which records the high-confidence ones
    in the SBOM's pedigree, so the VEX statements and the SBOM cannot
    disagree about what a patch claims to fix.
    """
    return sbom_cve_refs.cves_in(path)


# ---------------------------------------------------------------------------
# Source-component derivation
# ---------------------------------------------------------------------------


_SRC_COMPONENT_RE = re.compile(r"^src/([^/]+?)(?:\.patch)?/")


def source_component(path: str) -> str:
    """Derive a short label for the source tree this patch lives in.
    'src/thrift/patch/0002-cve-2017-1000487.patch' -> 'thrift'
    'src/sonic-linux-kernel/patches-sonic/...'     -> 'sonic-linux-kernel'
    'src/scapy.patch/0001-...'                     -> 'scapy'
    """
    m = _SRC_COMPONENT_RE.match(path)
    if not m:
        return "unknown"
    return m.group(1)


# ---------------------------------------------------------------------------
# OpenVEX document construction
# ---------------------------------------------------------------------------


def now_iso() -> str:
    epoch = os.environ.get("SOURCE_DATE_EPOCH")
    if epoch:
        try:
            return datetime.datetime.fromtimestamp(
                int(epoch), tz=datetime.timezone.utc
            ).strftime("%Y-%m-%dT%H:%M:%SZ")
        except Exception:
            pass
    return datetime.datetime.now(datetime.timezone.utc).strftime(
        "%Y-%m-%dT%H:%M:%SZ"
    )


def make_openvex_json(
    patch_path: str, component: str,
    high_cves: set, low_cves: set,
) -> str:
    """Return the OpenVEX JSON body for a single auto-VEX file.

    Re-runs are byte-identical (apart from timestamp, which is
    SOURCE_DATE_EPOCH when set).
    """
    h = hashlib.sha256((patch_path + "\n").encode()).hexdigest()[:16]
    vex_id = f"https://github.com/sonic-net/sonic-buildimage/vex/auto/{h}"

    cves_sorted = sorted(high_cves) + sorted(low_cves - high_cves)
    statements = []
    for cve in cves_sorted:
        is_high = cve in high_cves
        stmt = {
            "vulnerability": {"name": cve},
            "products": [{"@id": f"pkg:generic/{component}"}],
        }
        if is_high:
            # `fixed`, not `not_affected`: the patch changed the
            # vulnerable code, so the shipped artifact carries the
            # fix. Saying not_affected would claim the code is
            # untouched and merely unreachable, which is a different
            # thing and not what happened.
            stmt["status"] = "fixed"
            stmt["impact_statement"] = (
                f"Fixed by SONiC local patch {patch_path}. "
                f"The product is named by source tree only, so this "
                f"statement covers any version of {component} in the "
                "document; promote it to a curated vex/ file with an "
                "exact product PURL to bound it to the version built "
                "from that patch."
            )
        else:
            stmt["status"] = "under_investigation"
            stmt["impact_statement"] = (
                f"Patch {patch_path} mentions {cve} in passing but is "
                "not declared as a fix. Manual triage required to "
                "determine whether this VEX statement applies."
            )
        statements.append(stmt)

    doc = {
        "@context": "https://openvex.dev/ns/v0.2.0",
        "@id": vex_id,
        "author": "SONiC auto-VEX",
        "role": "SONiC build tooling",
        "timestamp": now_iso(),
        "version": 1,
        "_comment": (
            "Auto-extracted from " + patch_path + ". "
            "DO NOT EDIT — regenerate via "
            "scripts/sbom_extract_vex_from_patches.py"
        ),
        "statements": statements,
    }
    return json.dumps(doc, indent=2, sort_keys=True) + "\n"


# ---------------------------------------------------------------------------
# Top-level
# ---------------------------------------------------------------------------


def _input_fingerprint(patches: list) -> str:
    """SHA-256 over the sorted (path, content-sha256) pairs of every
    discovered patch. Used as a self-cache key so re-invocations with
    identical inputs (e.g. the 3 per-variant aggregator runs that
    build_sbom.sh fires) can short-circuit and skip the rescan."""
    h = hashlib.sha256()
    for p in sorted(patches):
        try:
            with open(p, "rb") as f:
                fhash = hashlib.sha256(f.read()).hexdigest()
        except Exception:
            continue
        h.update(p.encode())
        h.update(b"\0")
        h.update(fhash.encode())
        h.update(b"\0")
    return h.hexdigest()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--src", default="src",
                    help="Source root to scan (default: src)")
    ap.add_argument("--output-dir", default="vex/auto",
                    help="Where to write auto-VEX yamls (default: vex/auto)")
    ap.add_argument("--dry-run", action="store_true",
                    help="Don't write files, just report counts")
    args = ap.parse_args()

    if not os.path.isdir(args.src):
        warn(f"source dir not found: {args.src}")
        return 1

    patches = find_patches(args.src)
    info(f"scanned {len(patches)} patch files under {args.src}/")

    # Self-cache: if a prior run with byte-identical patch inputs
    # already produced this output-dir, skip the rescan. The marker
    # records the input fingerprint and lives inside the output-dir
    # itself, so `rm -rf vex/auto/` (or `make reset`) invalidates it
    # naturally. Output is deterministic so the cached files are
    # identical to what we'd produce now.
    marker = os.path.join(args.output_dir, ".input_hash")
    fingerprint = _input_fingerprint(patches)
    if not args.dry_run and os.path.isfile(marker):
        try:
            with open(marker) as f:
                if f.read().strip() == fingerprint:
                    info("inputs unchanged since last run; "
                         "vex/auto/ is fresh, skipping rescan.")
                    return 0
        except Exception:
            pass

    written = 0
    cve_total = 0
    for p in patches:
        high, low = cves_in(p)
        if not high and not low:
            continue
        component = source_component(p)
        body = make_openvex_json(p, component, high, low)
        if args.dry_run:
            cve_total += len(high) + len(low)
            written += 1
            info(f"would write VEX for {len(high)+len(low)} CVE(s) in {p}")
            continue
        out_dir = os.path.join(args.output_dir, component)
        os.makedirs(out_dir, exist_ok=True)
        out_path = os.path.join(out_dir, os.path.basename(p) + ".json")
        try:
            with open(out_path, "w") as f:
                f.write(body)
            written += 1
            cve_total += len(high) + len(low)
        except Exception as e:
            warn(f"could not write {out_path}: {e}")

    info(f"wrote {written} VEX file(s) covering {cve_total} CVE statement(s)"
         + (" (dry-run)" if args.dry_run else ""))

    # Persist input fingerprint so subsequent invocations with the
    # same patch inputs can short-circuit. Only after a successful
    # write so a failed run can't poison the cache.
    if not args.dry_run:
        try:
            os.makedirs(args.output_dir, exist_ok=True)
            with open(marker, "w") as f:
                f.write(fingerprint + "\n")
        except Exception as e:
            warn(f"could not write input-hash marker {marker}: {e}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
