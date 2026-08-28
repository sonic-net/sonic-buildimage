#!/usr/bin/env python3
"""
sbom_cve_refs.py — find the CVEs a patch says it fixes.

Shared by sbom_fragment.py, which records them in the SBOM's pedigree,
and sbom_extract_vex_from_patches.py, which turns them into VEX
statements. Both used to carry their own copy of these patterns; one
copy means the two cannot disagree about what a patch claims.

Two confidence levels, because they are not the same claim:

  high — the patch declares the CVE in its filename, or in a 'Fixes:'
         or 'Subject:' header. This is the patch saying what it fixes.
  low  — the CVE appears somewhere else in the header. Often a passing
         reference ("similar to CVE-...", "see also"), so it is not
         evidence that this patch fixes it.

Only high-confidence identifiers belong anywhere that reads as an
assertion.
"""

import os
import re
import sys

_CVE_RE = re.compile(r"CVE-(\d{4})-(\d{4,7})", re.IGNORECASE)
_FIXES_RE = re.compile(r"^\s*Fixes:\s*(CVE-\d{4}-\d{4,7})",
                       re.I | re.M)
_SUBJECT_RE = re.compile(r"^Subject:.*?(CVE-\d{4}-\d{4,7})",
                         re.I | re.M)

# How much of a patch to read when no header boundary is found.
_HEADER_FALLBACK_BYTES = 4000


def cves_in_text(filename: str, text: str) -> tuple:
    """Return (high_confidence, low_confidence) CVE id sets.

    filename is the patch's basename; text is its contents. Callers that
    have already read the file should use this and avoid a second read.
    """
    high: set = set()
    low: set = set()

    for m in _CVE_RE.finditer(os.path.basename(filename)):
        high.add(f"CVE-{m.group(1)}-{m.group(2)}".upper())

    # The header ends at the first diff boundary; beyond that we are
    # reading the change itself, where a CVE mention says nothing about
    # what the patch claims to fix.
    header_end = re.search(r"^(?:diff --git|---|\+\+\+) ", text, re.M)
    header = text[:header_end.start()] if header_end else \
        text[:_HEADER_FALLBACK_BYTES]

    for m in _FIXES_RE.finditer(header):
        high.add(m.group(1).upper())
    for m in _SUBJECT_RE.finditer(header):
        high.add(m.group(1).upper())
    for m in _CVE_RE.finditer(header):
        cve = f"CVE-{m.group(1)}-{m.group(2)}".upper()
        if cve not in high:
            low.add(cve)

    return high, low


def cves_in(path: str) -> tuple:
    """Same, reading the patch from disk.

    An unreadable patch yields nothing rather than failing the caller,
    but it is reported: silence here would mean a patch's CVE claims
    quietly missing from the SBOM, which is the exact gap this data
    exists to close.
    """
    try:
        with open(path, "rb") as f:
            data = f.read()
    except OSError as e:
        sys.stderr.write(
            f"[sbom_cve_refs.py] WARNING: could not read {path}: {e}\n"
        )
        return set(), set()
    return cves_in_text(path, data.decode("utf-8", errors="replace"))
