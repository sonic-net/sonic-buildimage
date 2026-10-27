"""Auto-discovered feature -> docker-dir/mk-file/include-flag registry.

No hand-maintained feature dict. Every `rules/docker-*.mk` file that defines
`_CONTAINER_NAME` is self-describing (container name, docker dir stem, and
whether/how it's gated behind an `ifeq ($(INCLUDE_*), y)` block feeding
`SONIC_INSTALL_DOCKER_IMAGES`). This module parses that convention directly
out of the sonic-buildimage tree at generation time.

See `optimizations/mega-generic/../../.cursor/plans/...mega-generic...plan.md`
section 3 for the design this implements, and the `sonic-docs-lookup` rule:
every fact below is derived straight from the `.mk` sources, not guessed.

Two passes:
  1. Primary: for each container-defining `.mk`, look for its own
     `SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_X)` line and the innermost
     `ifeq (...)` condition (if any) guarding it.
  2. Wrapper fallback: a few containers (e.g. `bgp` / `docker-fpm-frr.mk`)
     have *no* install line in their own file at all -- they're installed
     from a separate "wrapper" `.mk` (one with no `_CONTAINER_NAME` of its
     own, e.g. `docker-fpm.mk`'s `SONIC_ROUTING_STACK` switch). We scan those
     wrapper files as a fallback and record how/where the install actually
     happens.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Sequence

# ---------------------------------------------------------------------------
# Feature universe (see plan section 1: "open/extensible").
# ---------------------------------------------------------------------------

# The 11 features the generator has actually been designed/validated against.
# Anything else discovered in the tree is still returned by discover_registry
# (open universe) but flagged with proven=False by resolve_features().
PROVEN_FEATURES: frozenset[str] = frozenset(
    {
        "database",
        "swss",
        "bgp",
        "teamd",
        "lldp",
        "snmp",
        "gnmi",
        "radv",
        "eventd",
        "sysmgr",
        "pmon",
    }
)

# syncd is never a mega-container candidate, full stop (plan section 1).
# (In practice syncd's .mk files live under platform/<asic>/, not
# rules/docker-*.mk, so discover_registry() never even sees it -- this set
# is kept as an explicit, defensive guard against that assumption changing.)
HARD_EXCLUDED_FEATURES: frozenset[str] = frozenset({"syncd"})


@dataclass
class FeatureRegistryEntry:
    """Everything the rest of the generator needs to know about one feature,
    derived mechanically from the sonic-buildimage tree."""

    feature: str
    """Key used everywhere else in the generator (== container name)."""

    container_name: str
    """`$(DOCKER_X)_CONTAINER_NAME` value. Same as `feature`, kept as its
    own field for clarity at call sites."""

    docker_var: str
    """Make variable prefix, e.g. `DOCKER_LLDP` (used as `$(DOCKER_LLDP)`)."""

    stem: str
    """Docker dir stem, e.g. `docker-lldp` (== `dockers/<stem>` and the
    `_INCLUDE_DOCKER`/build-context name minus the `docker-` prefix)."""

    mk_file: str
    """Path to the defining `.mk`, relative to the sonic-buildimage root,
    e.g. `rules/docker-lldp.mk`."""

    docker_dir: Optional[Path]
    """Absolute path to `dockers/<stem>` if that directory actually exists
    in this tree, else None."""

    include_flag: Optional[str] = None
    """`INCLUDE_*` (or similar) make flag gating install, if any."""

    core: bool = False
    """True if unconditionally installed (no include flag toggles it off)."""

    installed_via: Optional[str] = None
    """Set when the install line lives in a different ("wrapper") `.mk`
    than the one defining `_CONTAINER_NAME` (e.g. `bgp` via
    `docker-fpm.mk`'s `SONIC_ROUTING_STACK` switch). Human-readable note,
    e.g. `"docker-fpm.mk (SONIC_ROUTING_STACK == frr)"`."""

    proven: bool = False
    """True if `feature` is in `PROVEN_FEATURES`."""

    undetermined: bool = False
    """True if neither the primary nor wrapper pass could find any install
    line for this container at all. Not necessarily a problem (could be a
    debug-only / never-installed-by-default image) but worth flagging."""


# ---------------------------------------------------------------------------
# Parsing helpers
# ---------------------------------------------------------------------------

# `$(DOCKER_LLDP)_CONTAINER_NAME = lldp`
_CONTAINER_NAME_RE = re.compile(
    r"^\$\(DOCKER_([A-Z0-9_]+)\)_CONTAINER_NAME\s*=\s*(\S+)", re.MULTILINE
)

_IFEQ_RE = re.compile(r"^\s*ifeq\s*\(\$\(([A-Za-z0-9_]+)\)\s*,\s*([^)]+?)\)\s*$")
_ENDIF_RE = re.compile(r"^\s*endif\s*$")

# Sentinel distinguishing "install line never found in this file" from
# "found, and it's unconditional" (None).
_NOT_FOUND = object()


def _stem_for(text: str, docker_var: str) -> Optional[str]:
    """`DOCKER_LLDP_STEM = docker-lldp` -> `docker-lldp`."""
    m = re.search(
        rf"^{re.escape(docker_var)}_STEM\s*=\s*(\S+)", text, re.MULTILINE
    )
    if m:
        return m.group(1)
    # Fallback for the rare file that hardcodes `_PATH` instead of using a
    # `_STEM` var (none observed as of this writing, but stay defensive).
    m = re.search(
        rf"\$\({re.escape(docker_var)}\)_PATH\s*=\s*\$\(DOCKERS_PATH\)/(\S+)",
        text,
    )
    return m.group(1) if m else None


def _find_install_condition(text: str, docker_var: str):
    """Scan `text` line-by-line tracking an `ifeq`/`endif` stack, and return:
      - `_NOT_FOUND` if `SONIC_INSTALL_DOCKER_IMAGES += $(<docker_var>)` never
        appears in `text` at all,
      - `None` if it appears unconditionally (no enclosing `ifeq`),
      - `(cond_var, cond_val)` for the innermost enclosing `ifeq` otherwise.

    Note: `else` branches are not tracked separately (none of the files this
    generator cares about put the install line after an `else`); this is a
    deliberate scaffold-stage simplification, not a general Makefile parser.
    """
    install_re = re.compile(
        rf"^\s*SONIC_INSTALL_DOCKER_IMAGES\s*\+=\s*\$\({re.escape(docker_var)}\)\s*$"
    )
    stack: List[tuple] = []
    for line in text.splitlines():
        m = _IFEQ_RE.match(line)
        if m:
            stack.append((m.group(1), m.group(2).strip()))
            continue
        if _ENDIF_RE.match(line):
            if stack:
                stack.pop()
            continue
        if install_re.match(line):
            return stack[-1] if stack else None
    return _NOT_FOUND


def _classify_condition(cond, wrapper_note: Optional[str] = None):
    """Turn a `_find_install_condition` result into `(core, include_flag,
    installed_via)`."""
    if cond is None:
        return True, None, wrapper_note
    if cond is _NOT_FOUND:
        return False, None, None
    cond_var, cond_val = cond
    if cond_val == "y" and cond_var.startswith("INCLUDE"):
        # Standard boolean feature toggle, e.g. ifeq ($(INCLUDE_LLDP), y).
        return False, cond_var, wrapper_note
    # Some other equality (e.g. $(SONIC_ROUTING_STACK), frr) -- not a simple
    # on/off toggle. Treat as effectively core (installed by the wrapper's
    # default branch) and record the condition for auditability.
    note = f"{cond_var} == {cond_val}"
    if wrapper_note:
        note = f"{wrapper_note} ({note})"
    return True, None, note


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------


def discover_registry(sonic_root: Path) -> Dict[str, FeatureRegistryEntry]:
    """Parse `<sonic_root>/rules/docker-*.mk` and return
    `{feature_name: FeatureRegistryEntry}` for every container-defining file.

    `sonic_root` is the sonic-buildimage tree root (e.g.
    `/build-sonic-arm/sonic-buildimage` on the ARM build VM, per the
    `sonic-code-changes-on-vm` rule; a local checkout works fine for the
    read-only discovery this function does).
    """
    rules_dir = sonic_root / "rules"
    mk_files = sorted(rules_dir.glob("docker-*.mk"))

    file_texts: Dict[Path, str] = {
        mk: mk.read_text(errors="replace") for mk in mk_files
    }

    container_files: List[tuple] = []  # (mk_path, text, var_suffix, container_name)
    wrapper_files: List[tuple] = []  # (mk_path, text) -- no _CONTAINER_NAME at all

    for mk, text in file_texts.items():
        matches = list(_CONTAINER_NAME_RE.finditer(text))
        if matches:
            for m in matches:
                container_files.append((mk, text, m.group(1), m.group(2)))
        else:
            wrapper_files.append((mk, text))

    registry: Dict[str, FeatureRegistryEntry] = {}

    for mk, text, var_suffix, container_name in container_files:
        docker_var = f"DOCKER_{var_suffix}"
        stem = _stem_for(text, docker_var)
        mk_rel = str(mk.relative_to(sonic_root))

        cond = _find_install_condition(text, docker_var)
        if cond is _NOT_FOUND:
            # Primary file has no install line -- fall back to scanning
            # wrapper files (e.g. docker-fpm.mk for bgp/docker-fpm-frr.mk).
            core, include_flag, installed_via = False, None, None
            undetermined = True
            for wmk, wtext in wrapper_files:
                if f"$({docker_var})" not in wtext:
                    continue
                wcond = _find_install_condition(wtext, docker_var)
                if wcond is _NOT_FOUND:
                    continue
                core, include_flag, installed_via = _classify_condition(
                    wcond, wrapper_note=wmk.name
                )
                undetermined = False
                break
        else:
            core, include_flag, installed_via = _classify_condition(cond)
            undetermined = False

        docker_dir = None
        if stem:
            candidate = sonic_root / "dockers" / stem
            docker_dir = candidate if candidate.is_dir() else None

        entry = FeatureRegistryEntry(
            feature=container_name,
            container_name=container_name,
            docker_var=docker_var,
            stem=stem or "",
            mk_file=mk_rel,
            docker_dir=docker_dir,
            include_flag=include_flag,
            core=core,
            installed_via=installed_via,
            proven=container_name in PROVEN_FEATURES,
            undetermined=undetermined,
        )

        if entry.feature in registry:
            # Two .mk files defining the same _CONTAINER_NAME has never been
            # observed; keep the first and let report.py surface the clash
            # rather than silently overwriting.
            continue
        registry[entry.feature] = entry

    # Defensive: never let a hard-excluded feature slip into the registry.
    for excluded in HARD_EXCLUDED_FEATURES:
        registry.pop(excluded, None)

    return registry


@dataclass
class ResolvedFeatures:
    resolved: List[str] = field(default_factory=list)
    """Requested features found in the registry (in input order)."""

    unproven: List[str] = field(default_factory=list)
    """Resolved features not in PROVEN_FEATURES (flagged, not refused)."""

    unknown: List[str] = field(default_factory=list)
    """Requested features not found in the registry at all."""

    excluded: List[str] = field(default_factory=list)
    """Requested features that are hard-excluded (e.g. syncd)."""


def resolve_features(
    requested: Sequence[str], registry: Dict[str, FeatureRegistryEntry]
) -> ResolvedFeatures:
    """Classify a requested feature list against the auto-discovered
    registry. Never raises -- unknown/unproven/excluded are reported back
    for the CLI to render, per plan section 1 ("flagged unproven, not
    refused")."""
    result = ResolvedFeatures()
    for name in requested:
        name = name.strip()
        if not name:
            continue
        if name in HARD_EXCLUDED_FEATURES:
            result.excluded.append(name)
        elif name not in registry:
            result.unknown.append(name)
        else:
            result.resolved.append(name)
            if not registry[name].proven:
                result.unproven.append(name)
    return result
