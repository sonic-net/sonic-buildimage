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

It also owns the answer to "which patches does this build apply":
patch_dirs() for where a source tree keeps them, applied_patches() for
which of those it applies, and patch_dirs_under() for the whole set
across the tree. Both callers need those answers and they used to
derive them differently, so they disagreed about the patch set as well
as about what to read from it — the VEX side reaching Debian's own
patches under the sources the build unpacks, and claiming SONiC had
fixed what Debian fixed.
"""

import os
import re
import sys
from typing import Optional

_CVE_RE = re.compile(r"CVE-(\d{4})-(\d{4,7})", re.IGNORECASE)
_FIXES_RE = re.compile(r"^\s*Fixes:\s*(CVE-\d{4}-\d{4,7})",
                       re.I | re.M)
_SUBJECT_RE = re.compile(r"^Subject:.*?(CVE-\d{4}-\d{4,7})",
                         re.I | re.M)

# How much of a patch to read when no header boundary is found.
_HEADER_FALLBACK_BYTES = 4000


# The only directory layouts SONiC uses for the patches it maintains.
# "patches-sonic" belongs to sonic-linux-kernel alone, which keeps
# Debian's patches beside its own in patches-debian.
_PATCH_DIR_NAMES = ("patch", "patches")
_KERNEL_PATCH_DIR = "patches-sonic"
_KERNEL_TREE = "sonic-linux-kernel"

# A source tree is src/<pkg> or src/<group>/<pkg> — the two depths
# recipes name in SRC_PATH. Nothing deeper is one, which is what keeps
# an unpacked upstream tarball's own debian/patches out: reaching
# src/grub2/grub2/debian/patches means treating src/grub2/grub2/debian
# as a source tree SONiC maintains, and it is Debian's.
_MAX_TREE_DEPTH = 2

# Build output and VCS metadata, never sources.
_SKIP_DIRS = {".git", "build", "deb_dist", "node_modules", "target"}



def patch_dirs(src_path: str) -> list:
    """Where ``src_path`` keeps the patches it applies.

    A directory counts if it applies patches, not if it has a `series`.
    Requiring one meant src/thrift/patch and src/socat/patch — whose
    recipes run `patch -p1` directly from their Makefile — were never
    looked at, so thrift's 0002-cve-2017-1000487.patch produced an
    auto-VEX suppressing CVE-2017-1000487 with nothing in the SBOM
    recording that we fix it.

    All of them, not the first: src/p4lang builds three components out
    of three sidecar directories and src/thrift_0_14_1 keeps its
    series in thrift.patch/, so stopping at the first match left four
    directories — including the one patch in the tree that names a CVE
    in its filename — with a VEX statement and no pedigree entry.

    Only the conventional layouts count. src_path itself is
    deliberately not one of them: src/ holds 36 loose patches and
    src/p4lang another 18, and a recipe rooted at either would take
    the lot.
    """
    if not src_path:
        return []
    src_path = src_path.rstrip("/")
    candidates = []
    # The Linux kernel keeps its own series in a uniquely-named
    # subdir, beside the Debian ones it does not maintain.
    if os.path.basename(src_path) == _KERNEL_TREE:
        candidates.append(os.path.join(src_path, _KERNEL_PATCH_DIR))
    # src/scapy.patch, src/sonic-swss.patch, ...
    candidates.append(src_path + ".patch")
    candidates += [os.path.join(src_path, n) for n in _PATCH_DIR_NAMES]
    # Sidecar directories one level in, for a recipe that builds
    # several components out of one tree: src/p4lang/p4lang-bmv2.patch.
    try:
        candidates += sorted(
            os.path.join(src_path, n) for n in os.listdir(src_path)
            if n.endswith(".patch")
        )
    except OSError:
        pass
    out = []
    for c in candidates:
        if c not in out and os.path.isdir(c) and applied_patches(c):
            out.append(c)
    return out


def _implied_source_tree(path: str) -> Optional[str]:
    """The source tree a patch directory at ``path`` would belong to.

    The inverse of patch_dirs' candidate list: None where the
    directory is not one of the layouts SONiC maintains.
    """
    path = path.rstrip("/")
    parent, name = os.path.split(path)
    if name.endswith(".patch"):
        return path[: -len(".patch")]
    if name in _PATCH_DIR_NAMES:
        return parent
    if name == _KERNEL_PATCH_DIR and os.path.basename(parent) == _KERNEL_TREE:
        return parent
    return None


def patch_dirs_under(root: str = "src") -> list:
    """Every patch directory under ``root`` that a source tree applies.

    The same directories patch_dirs hands the SBOM, gathered over the
    whole tree rather than one recipe at a time — so the VEX
    statements and the pedigree describe one patch set by
    construction. Taking every directory that held a *.patch instead
    reached the patches SONiC does not maintain: the Debian sources
    the build unpacks under src/<pkg>/ carry their own debian/patches,
    and a statement derived from one read "Fixed by SONiC local patch"
    about a fix that is Debian's and is already in the version we
    ship.
    """
    found = set()
    root = root.rstrip("/") or "."
    root_depth = root.count(os.sep)
    for cur, dirs, _files in os.walk(root):
        dirs[:] = sorted(d for d in dirs if d not in _SKIP_DIRS)
        # A patch dir sits at its source tree's own level
        # (src/x.patch) or one below it (src/x/patch), so nothing
        # deeper than _MAX_TREE_DEPTH + 1 can be one.
        if cur.count(os.sep) - root_depth > _MAX_TREE_DEPTH:
            dirs[:] = []
            continue
        for d in dirs:
            cand = os.path.join(cur, d)
            tree = _implied_source_tree(cand)
            if tree is None:
                continue
            if tree.count(os.sep) - root_depth > _MAX_TREE_DEPTH:
                continue
            # Ask the same question the SBOM asks, so neither side can
            # accept a directory the other would refuse.
            if cand in patch_dirs(tree):
                found.add(cand)
    return sorted(found)



def applied_patches(patch_dir: str) -> list:
    """The patches ``patch_dir`` applies, in apply order (basenames).

    A ``series`` file, where one exists, is the authoritative list: a
    patch sitting in the directory but left out of ``series`` is not
    applied, and reading it as though it were would have the SBOM and
    the VEX statements assert a fix the build never made.

    Where there is no ``series`` the recipe applies the patches
    directly — src/thrift/Makefile names both of its patches on
    explicit `patch -p1` lines — so every ``*.patch`` in the directory
    counts, sorted, which is the order those recipes list them in.
    """
    series = os.path.join(patch_dir, "series")
    if not os.path.isfile(series):
        try:
            return sorted(
                fn for fn in os.listdir(patch_dir)
                if fn.endswith(".patch")
                and os.path.isfile(os.path.join(patch_dir, fn))
            )
        except OSError as e:
            sys.stderr.write(
                f"[sbom_cve_refs.py] WARNING: could not list "
                f"{patch_dir}: {e}\n"
            )
            return []

    out = []
    try:
        with open(series) as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                # Series lines can carry options after the filename.
                fname = line.split()[0]
                if os.path.isfile(os.path.join(patch_dir, fname)):
                    out.append(fname)
    except OSError as e:
        sys.stderr.write(
            f"[sbom_cve_refs.py] WARNING: could not read {series}: {e}\n"
        )
    return out


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
