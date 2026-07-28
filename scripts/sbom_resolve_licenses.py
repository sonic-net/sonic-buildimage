#!/usr/bin/env python3
"""
sbom_resolve_licenses.py — extract SPDX license expressions from
/usr/share/doc/<pkg>/copyright files captured during the build.

Inputs:
    --copyrights TARBALL    A copyrights.tar.gz produced by either
                            build_debian.sh (host rootfs) or by the
                            per-container collect_version_files hook.
                            May be passed multiple times.
    --output FILE           Write a JSON map { pkg: spdx_expr } here.
    --license-map FILE      Override default scripts/sbom_license_map.json
    --quiet                 Suppress per-package warnings.

The tarball entries are expected to look like
    usr/share/doc/<pkg>/copyright
We extract the package name from the directory and parse the file:

  1. If it has 'Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/'
     header, it is DEP-5. Parse the Files: stanzas, take the union of
     License: identifiers, translate to SPDX via the license map.

  2. Otherwise, attempt a heuristic pass via `licensecheck` (from the
     devscripts Debian package) if available. licensecheck output is
     name-or-NONE per file; we map names through the same table.

  3. Otherwise emit 'NOASSERTION' and surface a warning so the operator
     can decide whether to add a per-recipe LICENSE override.

The resolver is read-only and stateless — runs once per build inside
the aggregator (scripts/build_sbom.py).
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
from collections import OrderedDict
from typing import Optional


# ---------------------------------------------------------------------------
# License-map loading + lookup
# ---------------------------------------------------------------------------

_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_LICENSE_MAP = os.path.join(_SCRIPT_DIR, "sbom_license_map.json")


def normalize_license_key(s: str) -> str:
    """Collapse whitespace and trim. Case-sensitive lookup first, then
    case-insensitive fallback is done by the caller via two map copies."""
    return re.sub(r"\s+", " ", s.strip())


class LicenseMap:
    def __init__(self, path: str):
        with open(path) as f:
            raw = json.load(f)
        # Skip the _comment key.
        self._exact = {
            normalize_license_key(k): v
            for k, v in raw.items() if not k.startswith("_")
        }
        self._lower = {k.lower(): v for k, v in self._exact.items()}

    def lookup(self, raw_id: str) -> Optional[str]:
        k = normalize_license_key(raw_id)
        if k in self._exact:
            return self._exact[k]
        kl = k.lower()
        if kl in self._lower:
            return self._lower[kl]
        return None


# ---------------------------------------------------------------------------
# DEP-5 parser (machine-readable copyright format 1.0)
# ---------------------------------------------------------------------------

_DEP5_HEADER_RE = re.compile(
    r"^Format:\s*https?://(?:www\.)?debian\.org/doc/"
    r"packaging-manuals/copyright-format/1\.0/?",
    re.IGNORECASE | re.MULTILINE,
)


def _split_paragraphs(text: str) -> list:
    """Split RFC822-ish copyright file into paragraphs."""
    paragraphs = []
    current: list = []
    for line in text.splitlines():
        if not line.strip():
            if current:
                paragraphs.append("\n".join(current))
                current = []
        else:
            current.append(line)
    if current:
        paragraphs.append("\n".join(current))
    return paragraphs


def _paragraph_fields(paragraph: str) -> dict:
    """Parse a DEP-5 paragraph into a {field: value} dict (multi-line
    values joined; leading-space continuations are folded)."""
    fields: dict = {}
    current_key: Optional[str] = None
    current_val: list = []
    for line in paragraph.splitlines():
        if line.startswith((" ", "\t")):
            if current_key is not None:
                current_val.append(line.strip())
        else:
            if current_key is not None:
                fields[current_key] = "\n".join(current_val).strip()
            if ":" in line:
                k, _, v = line.partition(":")
                current_key = k.strip()
                current_val = [v.strip()]
    if current_key is not None:
        fields[current_key] = "\n".join(current_val).strip()
    return fields


def parse_dep5(text: str, lmap: LicenseMap) -> Optional[str]:
    """Returns an SPDX expression, or None if DEP-5 parse failed."""
    if not _DEP5_HEADER_RE.search(text[:500]):
        return None
    paragraphs = _split_paragraphs(text)
    seen_licenses: "OrderedDict[str, None]" = OrderedDict()
    for p in paragraphs:
        fields = _paragraph_fields(p)
        if "License" not in fields or "Files" not in fields:
            continue
        # Take just the first line of the License: field — it's the
        # short identifier. The rest is the license text body.
        first_line = fields["License"].splitlines()[0].strip()
        for tok in re.split(r"\s+(?:or|and|OR|AND)\s+", first_line):
            tok = tok.strip()
            if not tok:
                continue
            mapped = lmap.lookup(tok)
            seen_licenses[mapped or f"LicenseRef-{tok}"] = None
    if not seen_licenses:
        return None
    if len(seen_licenses) == 1:
        return next(iter(seen_licenses))
    return " AND ".join(seen_licenses.keys())


# ---------------------------------------------------------------------------
# licensecheck fallback
# ---------------------------------------------------------------------------


def _have_licensecheck() -> bool:
    return shutil.which("licensecheck") is not None


def _run_licensecheck(path: str) -> Optional[str]:
    """Returns the licensecheck verdict (raw string, like 'GPL-2+') or None."""
    try:
        r = subprocess.run(
            ["licensecheck", "--copyright", "--no-conf", path],
            capture_output=True, text=True, timeout=15, check=False,
        )
    except Exception:
        return None
    if r.returncode != 0:
        return None
    for line in r.stdout.splitlines():
        # Format: "<path>: <license>"
        if ":" in line:
            _, _, lic = line.partition(":")
            lic = lic.strip()
            if lic and lic.upper() != "UNKNOWN":
                return lic
    return None


def parse_licensecheck(path: str, lmap: LicenseMap) -> Optional[str]:
    raw = _run_licensecheck(path)
    if not raw:
        return None
    mapped = lmap.lookup(raw)
    return mapped or f"LicenseRef-{raw}"


# ---------------------------------------------------------------------------
# Tarball iteration
# ---------------------------------------------------------------------------


_PKG_PATH_RE = re.compile(r"(?:^|/)usr/share/doc/([^/]+)/copyright$")


def resolve_from_tarball(
    tarball: str, lmap: LicenseMap, results: dict,
    noassertions: list, quiet: bool = False,
) -> None:
    """Walk a copyrights.tar.gz and populate results[pkg] = spdx_expr."""
    if not os.path.isfile(tarball):
        if not quiet:
            sys.stderr.write(
                f"[sbom_resolve_licenses.py] tarball not found: {tarball}\n"
            )
        return
    try:
        tf = tarfile.open(tarball, "r:*")
    except Exception as e:
        sys.stderr.write(
            f"[sbom_resolve_licenses.py] could not open {tarball}: {e}\n"
        )
        return

    have_lc = _have_licensecheck()

    with tf:
        for member in tf:
            if not member.isfile():
                continue
            m = _PKG_PATH_RE.search(member.name)
            if not m:
                continue
            pkg = m.group(1)
            if pkg in results:
                # Already resolved from an earlier tarball; keep the first.
                continue
            try:
                f = tf.extractfile(member)
                if f is None:
                    continue
                text = f.read().decode("utf-8", errors="replace")
            except Exception:
                continue

            spdx = parse_dep5(text, lmap)
            if not spdx and have_lc:
                # Stage to disk and let licensecheck have a go.
                tmp = f"/tmp/sbom-cp-{pkg}.txt"
                try:
                    with open(tmp, "w") as fw:
                        fw.write(text)
                    spdx = parse_licensecheck(tmp, lmap)
                finally:
                    try:
                        os.unlink(tmp)
                    except Exception:
                        pass
            if spdx:
                results[pkg] = spdx
            else:
                results[pkg] = "NOASSERTION"
                noassertions.append(pkg)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--copyrights", action="append", default=[],
                    help="Path to a copyrights.tar.gz; repeatable.")
    ap.add_argument("--output", required=True,
                    help="Where to write the resolved map (JSON).")
    ap.add_argument("--license-map", default=DEFAULT_LICENSE_MAP,
                    help="Path to the SPDX translation table.")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args()

    try:
        lmap = LicenseMap(args.license_map)
    except Exception as e:
        sys.stderr.write(
            f"[sbom_resolve_licenses.py] could not load license map: {e}\n"
        )
        return 1

    results: dict = {}
    noassertions: list = []
    for tarball in args.copyrights:
        resolve_from_tarball(tarball, lmap, results, noassertions,
                             quiet=args.quiet)

    try:
        with open(args.output, "w") as f:
            json.dump({
                "license_map_version": "1",
                "resolved": dict(sorted(results.items())),
                "noassertion_packages": sorted(noassertions),
            }, f, indent=2)
    except Exception as e:
        sys.stderr.write(
            f"[sbom_resolve_licenses.py] could not write {args.output}: {e}\n"
        )
        return 1

    resolved_count = len(results) - len(noassertions)
    total = len(results)
    pct = (100.0 * resolved_count / total) if total else 0.0
    sys.stderr.write(
        f"[sbom_resolve_licenses.py] resolved {resolved_count}/{total} "
        f"packages ({pct:.1f}%); {len(noassertions)} NOASSERTION\n"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
