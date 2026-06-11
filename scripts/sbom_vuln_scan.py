#!/usr/bin/env python3
"""
sbom_vuln_scan.py — generate a vulnerability report from a SONiC SBOM.

This is a **standalone post-build tool**, intentionally not part of the
build. It runs anywhere a CycloneDX SBOM and Python 3 are available and
produces a CycloneDX VEX-annotated vulnerability report.

Why standalone:
- The SBOM is reproducible; a vulnerability report cannot be (new CVEs
  are disclosed daily). Coupling them would break SBOM reproducibility.
- A six-month-old SBOM can be re-scanned against today's CVE data
  without rebuilding the .bin.
- Release engineering picks its own cadence for CVE scans separate from
  build cadence.

Usage:
    sbom_vuln_scan.py SBOM.cdx.json
    sbom_vuln_scan.py --vex vex/ --output target/sonic-broadcom.bin.vuln.json SBOM.cdx.json
    sbom_vuln_scan.py --min-severity high --format table SBOM.cdx.json
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
from typing import Any, Optional


_SEVERITY_RANK = {
    "negligible": 0, "low": 1, "medium": 2, "high": 3, "critical": 4,
    "unknown": -1,
}


def warn(msg: str) -> None:
    sys.stderr.write(f"[sbom_vuln_scan.py] WARNING: {msg}\n")


def info(msg: str) -> None:
    sys.stderr.write(f"[sbom_vuln_scan.py] {msg}\n")


# ---------------------------------------------------------------------------
# Grype install / invocation
# ---------------------------------------------------------------------------


def resolve_grype() -> Optional[str]:
    """Find grype on PATH, then in the build cache, then auto-install via
    scripts/install_sbom_tool.sh."""
    p = shutil.which("grype")
    if p:
        return p
    # Build-tree cache.
    for d in ("target/sbom-tools", os.path.expanduser("~/.cache/sonic-sbom")):
        if not os.path.isdir(d):
            continue
        for sub in os.listdir(d):
            cand = os.path.join(d, sub, "grype")
            if os.path.isfile(cand) and os.access(cand, os.X_OK):
                return cand
    # Auto-fetch via the install helper.
    install_script = os.path.join(
        os.path.dirname(__file__), "install_sbom_tool.sh"
    )
    if not os.path.isfile(install_script):
        return None
    try:
        r = subprocess.run(
            [install_script, "grype"],
            capture_output=True, text=True, timeout=300,
        )
        out = r.stdout.strip()
        if r.returncode == 0 and out:
            return out
    except Exception as e:
        warn(f"install_sbom_tool.sh grype failed: {e}")
    return None


def run_grype(grype: str, sbom_path: str, vex_files: list) -> Optional[dict]:
    """Run grype against an SBOM and return the parsed JSON output."""
    cmd = [grype, f"sbom:{sbom_path}", "-o", "json", "-q"]
    for v in vex_files:
        cmd.extend(["--vex", v])
    info(f"running: {' '.join(cmd)}")
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=900)
    except subprocess.TimeoutExpired:
        warn("grype timed out")
        return None
    except Exception as e:
        warn(f"grype failed to start: {e}")
        return None
    if r.returncode != 0:
        warn(f"grype exited {r.returncode}: {r.stderr.strip()[:400]}")
        return None
    try:
        return json.loads(r.stdout)
    except Exception as e:
        warn(f"could not parse grype JSON: {e}")
        return None


# ---------------------------------------------------------------------------
# VEX loading (project-local YAML files)
# ---------------------------------------------------------------------------


def collect_vex_documents(vex_dirs: list) -> list:
    """Return absolute paths to every .yaml/.yml/.json file under the
    given VEX directories. Files are passed directly to grype which
    handles OpenVEX, CycloneDX-VEX, and CSAF formats."""
    out = []
    for d in vex_dirs:
        if not os.path.isdir(d):
            warn(f"vex dir not found: {d}")
            continue
        for root, _, files in os.walk(d):
            for fn in files:
                if fn.startswith(".") or fn.endswith(".md"):
                    continue
                if fn.endswith((".yaml", ".yml", ".json")):
                    out.append(os.path.abspath(os.path.join(root, fn)))
    return sorted(out)


# ---------------------------------------------------------------------------
# Report shaping
# ---------------------------------------------------------------------------


def severity_rank(s: str) -> int:
    return _SEVERITY_RANK.get((s or "").lower(), -1)


def filter_by_severity(matches: list, min_sev: Optional[str]) -> list:
    if not min_sev:
        return matches
    threshold = severity_rank(min_sev)
    if threshold < 0:
        warn(f"unknown severity: {min_sev}; not filtering")
        return matches
    return [
        m for m in matches
        if severity_rank(m.get("vulnerability", {}).get("severity", ""))
        >= threshold
    ]


def summarize(matches: list) -> dict:
    by_sev: dict = {}
    suppressed = 0
    for m in matches:
        sev = (m.get("vulnerability") or {}).get("severity") or "Unknown"
        by_sev[sev] = by_sev.get(sev, 0) + 1
    return {"total": len(matches), "by_severity": by_sev,
            "suppressed_via_vex": suppressed}


def to_cyclonedx_vex(grype_doc: dict, source_sbom_path: str) -> dict:
    """Wrap grype's matches into a CycloneDX 1.6 VEX document."""
    sbom_meta = {}
    try:
        with open(source_sbom_path) as f:
            src = json.load(f)
            sbom_meta = src.get("metadata", {})
    except Exception:
        pass

    vulns = []
    for m in grype_doc.get("matches", []):
        v = m.get("vulnerability", {})
        art = m.get("artifact", {})
        purl = art.get("purl") or f"{art.get('name','?')}@{art.get('version','?')}"
        primary, _ = vuln_id_pair(m)
        entry: dict[str, Any] = {
            "id": primary,
            "source": {"name": v.get("dataSource", "").split("/")[2]
                       if v.get("dataSource", "").startswith(("http://", "https://"))
                       else "unknown",
                       "url": v.get("dataSource", "")},
            "ratings": [{
                "severity": (v.get("severity") or "unknown").lower(),
                "method": "other",
            }],
            "description": v.get("description") or "",
            "affects": [{"ref": purl}],
        }
        # Surface aliases via references[] (CycloneDX 1.6) so downstream
        # consumers can join across CVE / GHSA / vendor-VDB id spaces.
        # Add the grype-canonical id when it differs from primary (it
        # will when we swapped CVE→primary, GHSA→here), then add every
        # CVE alias from relatedVulnerabilities[] that isn't already
        # primary.
        refs = []
        if v.get("id") and v.get("id") != primary:
            refs.append({
                "id": v["id"],
                "source": {"name": "GitHub Advisory Database",
                           "url": v.get("dataSource", "")},
            })
        for rel in m.get("relatedVulnerabilities", []) or []:
            rid = rel.get("id", "")
            if not rid or rid == primary:
                continue
            if rid.startswith("CVE-"):
                refs.append({
                    "id": rid,
                    "source": {
                        "name": "NVD",
                        "url": f"https://nvd.nist.gov/vuln/detail/{rid}",
                    },
                })
        if refs:
            entry["references"] = refs
        # Preserve grype's exact fix.state ('fixed', 'not-fixed',
        # 'wont-fix', 'unknown') as a CycloneDX property so downstream
        # consumers can distinguish "no fix yet" from "upstream
        # declined to fix" without having to parse the recommendation
        # field. Only 'fixed' findings get a recommendation; the rest
        # carry only the state property.
        fix = v.get("fix") or {}
        state = fix.get("state", "unknown")
        entry.setdefault("properties", []).append({
            "name": "grype:fix-state",
            "value": state,
        })
        if state == "fixed" and fix.get("versions"):
            entry["recommendation"] = (
                f"Upgrade to {', '.join(fix['versions'])}"
            )
        vulns.append(entry)

    return {
        "bomFormat": "CycloneDX",
        "specVersion": "1.6",
        "version": 1,
        "metadata": {
            "tools": [{
                "vendor": "Anchore",
                "name": "grype",
                "version": grype_doc.get("descriptor", {})
                                    .get("version", "unknown"),
            }, {
                "vendor": "SONiC",
                "name": "sbom_vuln_scan.py",
                "version": "1.0",
            }],
            "properties": [
                {"name": "sonic:source_sbom", "value": source_sbom_path},
                {"name": "sonic:source_sbom_serial",
                 "value": sbom_meta.get("component", {})
                                   .get("bom-ref", "") or ""},
            ],
        },
        "vulnerabilities": vulns,
    }


def fix_state_of(match: dict) -> str:
    """grype's fix.state for a match — 'fixed', 'not-fixed', 'wont-fix',
    'unknown'."""
    return (match.get("vulnerability", {}).get("fix") or {}).get(
        "state", "unknown"
    )


def is_actionable(match: dict) -> bool:
    """A finding is actionable when an upstream fix is available."""
    return fix_state_of(match) == "fixed"


def print_table(matches: list, total_by_sev: dict) -> None:
    # Bucket findings by fix state. The human-readable table only
    # lists actionable rows (the ones a release manager can do
    # something about); not-fixed and wont-fix are summarised in the
    # header so the reader knows they exist without being buried in
    # un-fixable noise. The CycloneDX JSON sidecar contains the full
    # set for machine consumption.
    by_fix_state: dict = {"fixed": [], "not-fixed": [], "wont-fix": [],
                          "unknown": []}
    for m in matches:
        state = fix_state_of(m)
        by_fix_state.setdefault(state, []).append(m)

    fixed = by_fix_state.get("fixed", [])
    not_fixed = by_fix_state.get("not-fixed", [])
    wont_fix = by_fix_state.get("wont-fix", [])
    unknown = by_fix_state.get("unknown", [])

    print(f"Total findings: {sum(total_by_sev.values())}")
    print("By fix state:")
    print(f"  Actionable (upstream fix available):  {len(fixed)}")
    print(f"  Not-fixed (no upstream fix yet):      {len(not_fixed)}")
    print(f"  Won't-fix (upstream declined):        {len(wont_fix)}")
    if unknown:
        print(f"  Unknown fix state:                    {len(unknown)}")

    # Severity breakdown for actionable findings only — that's what
    # the row listing below covers and what release planning targets.
    actionable_by_sev: dict = {}
    for m in fixed:
        sev = (m.get("vulnerability", {}).get("severity") or "unknown").title()
        actionable_by_sev[sev] = actionable_by_sev.get(sev, 0) + 1
    print("By severity (actionable only):")
    for sev in ("Critical", "High", "Medium", "Low", "Negligible", "Unknown"):
        n = actionable_by_sev.get(sev, 0)
        if n:
            print(f"  {sev:11} {n}")

    if not fixed:
        return
    print()
    print(f"{'SEVERITY':9} {'ID':18} {'ECOSYS':7} "
          f"{'PKG':26} {'VERSION':24} FIX")
    print("-" * 110)
    # Sort by severity desc, then by preferred display id
    matches_sorted = sorted(
        fixed,
        key=lambda m: (-severity_rank(
            m.get("vulnerability", {}).get("severity", "")
        ), preferred_vuln_id(m))
    )
    for m in matches_sorted:
        v = m.get("vulnerability", {})
        a = m.get("artifact", {})
        sev = (v.get("severity") or "?")
        primary, _ = vuln_id_pair(m)
        ecosys = ecosystem_from_purl(a.get("purl") or "") or a.get("type", "")
        pkg = a.get("name") or "?"
        ver = a.get("version") or "?"
        fix = v.get("fix") or {}
        fix_versions = ", ".join(fix.get("versions") or []) or "-"
        print(f"{sev:9} {primary:18} {ecosys:7} "
              f"{pkg:26} {ver:24} {fix_versions}")


def ecosystem_from_purl(purl: str) -> str:
    """Extract the package ecosystem label from a PURL.

    Maps long PURL types to short, fixed-width labels suitable for a
    table column. 'pkg:deb/...' → 'deb', 'pkg:golang/...' → 'go',
    'pkg:cargo/...' → 'rust', 'pkg:pypi/...' → 'py', etc.
    """
    if not purl.startswith("pkg:"):
        return ""
    after = purl[4:]
    head = after.split("/", 1)[0]
    return {
        "deb": "deb",
        "rpm": "rpm",
        "apk": "apk",
        "cargo": "rust",
        "golang": "go",
        "npm": "npm",
        "pypi": "py",
        "gem": "gem",
        "maven": "java",
        "nuget": "nuget",
        "oci": "oci",
        "github": "github",
        "generic": "generic",
    }.get(head, head[:7])


def vuln_id_pair(m: dict) -> tuple[str, str]:
    """Return (primary_id, alias_id) for display.

    Grype puts the canonical advisory id (often GHSA-* for findings
    sourced from the GitHub Advisory Database) in `vulnerability.id`,
    and any CVE alias in `relatedVulnerabilities[].id`. We display the
    CVE as the *primary* id when one exists, since CVE-NNNN-NNNN is the
    identifier humans recognize; the GHSA goes in the alias column.
    """
    v = m.get("vulnerability", {})
    primary = v.get("id") or "?"
    cve = ""
    for rel in m.get("relatedVulnerabilities", []) or []:
        rid = rel.get("id", "")
        if rid.startswith("CVE-"):
            cve = rid
            break
    if cve and not primary.startswith("CVE-"):
        return cve, primary
    return primary, cve or ""


def preferred_vuln_id(m: dict) -> str:
    """CVE if present, else the grype-canonical id. Used for sorting."""
    return vuln_id_pair(m)[0]


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("sbom",
                    help="Input CycloneDX SBOM (.cdx.json)")
    ap.add_argument("--vex", action="append", default=[],
                    help="Directory containing VEX yaml/json documents. "
                         "Repeatable.")
    ap.add_argument("--output", "-o",
                    help="Write CycloneDX VEX-annotated report here. "
                         "Default: <sbom>.vuln.json")
    ap.add_argument("--format", choices=("json", "table", "both"),
                    default="both",
                    help="Output style. 'json' writes only the file; "
                         "'table' writes only stdout; 'both' (default).")
    ap.add_argument("--min-severity",
                    help="Filter findings below this severity in the "
                         "report. (negligible/low/medium/high/critical)")
    ap.add_argument("--fail-on",
                    help="Exit non-zero if any finding at or above this "
                         "severity remains after VEX/filter. Useful for "
                         "CI gating.")
    ap.add_argument("--grype",
                    help="Path to grype binary. Auto-installed if absent.")
    args = ap.parse_args()

    if not os.path.isfile(args.sbom):
        warn(f"SBOM not found: {args.sbom}")
        return 2

    grype = args.grype or resolve_grype()
    if not grype:
        warn("grype is not available; install via scripts/install_sbom_tool.sh")
        return 2
    info(f"using grype: {grype}")

    vex_files = collect_vex_documents(args.vex)
    if vex_files:
        info(f"loaded {len(vex_files)} VEX document(s)")

    grype_doc = run_grype(grype, args.sbom, vex_files)
    if grype_doc is None:
        return 2

    matches = grype_doc.get("matches", []) or []
    if args.min_severity:
        before = len(matches)
        matches = filter_by_severity(matches, args.min_severity)
        info(f"min-severity filter: {len(matches)}/{before} remain")

    by_sev: dict = {}
    for m in matches:
        sev = (m.get("vulnerability") or {}).get("severity") or "Unknown"
        by_sev[sev] = by_sev.get(sev, 0) + 1

    out_path = args.output or (args.sbom.removesuffix(".cdx.json") + ".vuln.json")
    if args.format in ("json", "both"):
        out_doc = to_cyclonedx_vex({**grype_doc, "matches": matches},
                                   args.sbom)
        try:
            with open(out_path, "w") as f:
                json.dump(out_doc, f, indent=2, sort_keys=True)
            info(f"wrote {out_path}")
        except Exception as e:
            warn(f"could not write {out_path}: {e}")

    if args.format in ("table", "both"):
        print_table(matches, by_sev)

    if args.fail_on:
        # Restrict the failure gate to *actionable* findings — i.e. those
        # with an upstream fix available. Failing on not-fixed / wont-fix
        # entries gates the build on things the team can't act on; those
        # belong on a separate VEX triage workflow, not CI gating.
        threshold = severity_rank(args.fail_on)
        high_findings = [
            m for m in matches
            if is_actionable(m)
            and severity_rank(m.get("vulnerability", {}).get("severity", ""))
            >= threshold
        ]
        if high_findings:
            warn(f"{len(high_findings)} actionable finding(s) at or above "
                 f"'{args.fail_on}' — failing")
            return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
