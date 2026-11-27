"""Per-feature asset discovery: docker-dir -> FeatureSpec.

`auto_registry.py` answers "which `.mk` / docker dir does feature X live
in?". This module answers the next question: "what does that docker dir
*contain*?" -- generically, by scanning the directory tree, not by hand-
maintaining a table of filenames per feature (plan section 1: "true
generator").

Every fact this module asserts about file-naming conventions was verified
directly against the sonic-buildimage tree (per the `sonic-docs-lookup`
rule), specifically:

  - `supervisord.conf.j2` is the standard per-container entry-point template.
    It typically does `{% include "supervisord.conf.common.j2" %}` to pull
    in the reusable per-process program stanzas (see
    `docker-teamd/supervisord.conf.j2`, which includes
    `docker-teamd/supervisord.conf.common.j2`). Some
    containers name the entry file `<prefix>.supervisord.conf.j2` instead
    (`docker-pmon.supervisord.conf.j2`,
    `docker-router-advertiser.supervisord.conf.j2`).
  - A few containers (`docker-sonic-gnmi`, `docker-eventd`, `docker-sysmgr`)
    skip the split entirely and ship a single static `supervisord.conf`
    (no `.j2`, no `.common.j2` companion) -- ENTRYPOINT is
    `/usr/local/bin/supervisord` directly, no wrapper init script.
  - `bgp` (`docker-fpm-frr`) nests its supervisord assets one level down at
    `frr/supervisord/{supervisord.conf.j2,supervisord.conf.common.j2,
    critical_processes.j2}` rather than at the docker-dir top level --
    confirming discovery must recurse (`rglob`), not just look at the top
    level.
  - `critical_processes` is either a static file (most containers) or a
    Jinja2 template `critical_processes.j2` (`docker-database`,
    `docker-orchagent`, and bgp's nested copy) rendered at container-init
    time by `sonic-cfggen`.
  - "Init scripts" are whatever the Dockerfile.j2's final-stage `ENTRYPOINT`
    points at. Two shapes were observed:
      1. A literal `.sh` file `COPY`'d verbatim (`docker_init.sh`,
         `docker-teamd-init.sh`, `docker-lldp-init.sh`,
         `docker-snmp-init.sh`, `docker-database-init.sh`, router-advertiser's
         `docker-init.sh`).
      2. A `.j2` template (`docker-init.j2` for swss/orchagent,
         `docker_init.j2` for pmon) rendered by a `RUN sonic-cfggen ... -t
         .../<name>.j2 > /usr/bin/<name>.sh` line at build time -- the
         literal `.sh` therefore never exists in the source tree.
      3. No init script at all: ENTRYPOINT is `/usr/local/bin/supervisord`
         directly (gnmi, eventd, sysmgr).

See the `generic_mega-container_merge_script` plan, section 2 (module list)
and section 4 (`docker_image_ctl.j2` parsing feeds off the same containers).
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Sequence

from mega_gen.auto_registry import (
    FeatureRegistryEntry,
    discover_registry,
)

# ---------------------------------------------------------------------------
# Naming-convention constants (see module docstring for how each was
# verified against the tree).
# ---------------------------------------------------------------------------

_SUPERVISORD_MAIN_GLOB = "*supervisord.conf.j2"
_SUPERVISORD_COMMON_GLOB = "*supervisord.conf.common.j2"
_SUPERVISORD_STATIC_NAME = "supervisord.conf"

_CRITICAL_PROCESSES_TEMPLATE_NAME = "critical_processes.j2"
_CRITICAL_PROCESSES_STATIC_NAME = "critical_processes"

_DOCKERFILE_NAME = "Dockerfile.j2"
_DOCKERFILE_COMMON_NAME = "Dockerfile.common.j2"
_COMMON_J2_GLOB = "*.common.j2"

_BASE_IMAGE_FILES_DIRNAME = "base_image_files"

# `ENTRYPOINT ["/usr/bin/docker-init.sh"]` (possibly with extra args after
# the first quoted element, e.g. docker-ptf's `-c` `/etc/.../supervisord.conf`
# -- we only need the first element).
_ENTRYPOINT_RE = re.compile(r'^\s*ENTRYPOINT\s*\[\s*"([^"]+)"', re.MULTILINE)


# ---------------------------------------------------------------------------
# Output dataclass
# ---------------------------------------------------------------------------


@dataclass
class FeatureSpec:
    """Everything the generation stages (`supervisord_merge.py`,
    `gen_mega_mk.py`, `gen_dockerfile.py`, `ctl_parser.py`/`patch_templates.py`)
    need to know about one feature's on-disk assets, discovered generically
    (no hand-maintained per-feature table)."""

    # --- identity / registry passthrough (see auto_registry.FeatureRegistryEntry) ---
    feature: str
    registry_entry: FeatureRegistryEntry
    docker_dir: Optional[Path]
    """Absolute path to `dockers/<stem>`, or None if the registry couldn't
    resolve one (discovery is then a no-op and `warnings` explains why)."""

    # --- supervisord assets ---
    supervisord_conf: Optional[Path] = None
    """The container's supervisord entry-point config: either a rendered
    `*supervisord.conf.j2` template or, for a few containers, a static
    `supervisord.conf` with no templating at all (see
    `supervisord_is_template`)."""

    supervisord_is_template: bool = False

    supervisord_common: Optional[Path] = None
    """The reusable `*supervisord.conf.common.j2` companion that
    `supervisord_conf` `{% include %}`s, if this feature has one (only the
    containers already proven against `docker-sonic-vs` -- swss, bgp,
    teamd, lldp -- currently do). None otherwise; `supervisord_conf` alone
    then has everything."""

    # --- critical_processes ---
    critical_processes: Optional[Path] = None
    critical_processes_is_template: bool = False

    # --- base_image_files/ ---
    base_image_files_dir: Optional[Path] = None
    """Absolute path to `<docker_dir>/base_image_files/` if present. Holds
    the host-side CLI wrapper scripts (`vtysh`, `lldpctl`, `teamdctl`,
    `redis-cli`, etc.) plan section 9 says mega must retarget."""

    # --- init script (Dockerfile.j2 ENTRYPOINT) ---
    dockerfile_j2: Optional[Path] = None
    entrypoint: Optional[str] = None
    """Raw `ENTRYPOINT` argv[0] string from `dockerfile_j2`, e.g.
    `/usr/bin/docker-init.sh`, or `/usr/local/bin/supervisord` when the
    container has no wrapper init script at all."""

    uses_supervisord_directly: bool = False
    """True when `entrypoint`'s basename is `supervisord` -- no init
    wrapper; `docker create`/`start()` just launches supervisord."""

    init_script: Optional[Path] = None
    """The literal init `.sh` file copied verbatim into the image, if one
    exists in the source tree under this name."""

    init_script_template: Optional[Path] = None
    """The `.j2` template that gets rendered into the init script at build
    time (`RUN sonic-cfggen ... -t <this>.j2 > /usr/bin/<name>.sh`), when
    the init script itself is generated rather than copied verbatim (e.g.
    swss's `docker-init.j2`, pmon's `docker_init.j2`). Mutually exclusive
    with `init_script` in every container observed so far, but both are
    exposed in case some future container ships both.
    """

    # --- Dockerfile.common.j2 (structural, not supervisord) ---
    dockerfile_common: Optional[Path] = None
    """`Dockerfile.common.j2` if this docker dir has one -- the fragment
    `Dockerfile.j2` pulls in via `{% include "Dockerfile.common.j2" %}` to
    COPY the feature's static assets (frr/, lldpd.conf.j2, etc.). Consumed
    by `gen_dockerfile.py`."""

    # --- generic catch-all ---
    common_j2_files: List[Path] = field(default_factory=list)
    """Every `*.common.j2` file found anywhere under `docker_dir`
    (superset covering `dockerfile_common` + `supervisord_common` + any
    other `.common.j2` fragment this feature happens to have -- kept as a
    flat list so later stages don't need to special-case unknown ones)."""

    warnings: List[str] = field(default_factory=list)
    """Discovery-time anomalies (missing docker dir, no supervisord conf
    found, more than one candidate found, etc.) -- surfaced, never
    silently swallowed, per the `sonic-docs-lookup` rule ("flag the
    uncertainty explicitly rather than filling the gap with a plausible
    guess")."""

    # --- convenience passthroughs from FeatureRegistryEntry ---
    @property
    def container_name(self) -> str:
        return self.registry_entry.container_name

    @property
    def stem(self) -> str:
        return self.registry_entry.stem

    @property
    def mk_file(self) -> str:
        return self.registry_entry.mk_file

    @property
    def include_flag(self) -> Optional[str]:
        return self.registry_entry.include_flag

    @property
    def core(self) -> bool:
        return self.registry_entry.core

    @property
    def proven(self) -> bool:
        return self.registry_entry.proven


# ---------------------------------------------------------------------------
# Internal scanning helpers
# ---------------------------------------------------------------------------


def _rglob_files(root: Path, pattern: str) -> List[Path]:
    return sorted(p for p in root.rglob(pattern) if p.is_file())


def _pick_one(
    candidates: List[Path], docker_dir: Path, what: str, warnings: List[str]
) -> Optional[Path]:
    """Pick the best candidate out of a (possibly empty/multi) list,
    preferring the one closest to `docker_dir`'s top level (fewest path
    parts) since nested copies are typically feature-internal (e.g. FRR's
    isolate/unisolate templates) rather than the "the" file. Warns (does
    not raise) on zero or multiple candidates."""
    if not candidates:
        return None
    if len(candidates) == 1:
        return candidates[0]
    candidates_sorted = sorted(
        candidates, key=lambda p: len(p.relative_to(docker_dir).parts)
    )
    warnings.append(
        f"multiple {what} candidates found under {docker_dir}: "
        f"{[str(p.relative_to(docker_dir)) for p in candidates_sorted]} "
        f"-- picked {candidates_sorted[0].relative_to(docker_dir)}"
    )
    return candidates_sorted[0]


def _discover_supervisord(
    docker_dir: Path, warnings: List[str]
) -> tuple[Optional[Path], bool, Optional[Path]]:
    """Returns (supervisord_conf, is_template, supervisord_common)."""
    main_candidates = _rglob_files(docker_dir, _SUPERVISORD_MAIN_GLOB)
    common_candidates = _rglob_files(docker_dir, _SUPERVISORD_COMMON_GLOB)
    static_candidates = [
        p for p in _rglob_files(docker_dir, _SUPERVISORD_STATIC_NAME)
    ]

    supervisord_common = _pick_one(
        common_candidates, docker_dir, "supervisord.conf.common.j2", warnings
    )

    if main_candidates:
        conf = _pick_one(
            main_candidates, docker_dir, "supervisord.conf.j2", warnings
        )
        return conf, True, supervisord_common
    if static_candidates:
        conf = _pick_one(
            static_candidates, docker_dir, "supervisord.conf", warnings
        )
        return conf, False, supervisord_common

    warnings.append(f"no supervisord.conf(.j2) found under {docker_dir}")
    return None, False, supervisord_common


def _discover_critical_processes(
    docker_dir: Path, warnings: List[str]
) -> tuple[Optional[Path], bool]:
    template_candidates = _rglob_files(
        docker_dir, _CRITICAL_PROCESSES_TEMPLATE_NAME
    )
    static_candidates = _rglob_files(
        docker_dir, _CRITICAL_PROCESSES_STATIC_NAME
    )
    if template_candidates:
        return (
            _pick_one(
                template_candidates,
                docker_dir,
                "critical_processes.j2",
                warnings,
            ),
            True,
        )
    if static_candidates:
        return (
            _pick_one(
                static_candidates, docker_dir, "critical_processes", warnings
            ),
            False,
        )
    warnings.append(f"no critical_processes(.j2) found under {docker_dir}")
    return None, False


def _discover_entrypoint(
    docker_dir: Path, warnings: List[str]
) -> tuple[Optional[Path], Optional[str]]:
    """Returns (dockerfile_j2 path, raw ENTRYPOINT argv[0]) for the
    feature's own `Dockerfile.j2`. Multi-stage Dockerfiles may have more
    than one `ENTRYPOINT` (only the final stage's counts, e.g.
    docker-orchagent's `base` stage has none -- only the final `FROM $BASE`
    stage does) -- the *last* match in the file wins."""
    dockerfile = docker_dir / _DOCKERFILE_NAME
    if not dockerfile.is_file():
        warnings.append(f"no {_DOCKERFILE_NAME} found under {docker_dir}")
        return None, None
    text = dockerfile.read_text(errors="replace")
    matches = _ENTRYPOINT_RE.findall(text)
    if not matches:
        warnings.append(f"no ENTRYPOINT found in {dockerfile}")
        return dockerfile, None
    return dockerfile, matches[-1]


def _discover_init_script(
    docker_dir: Path, entrypoint: Optional[str], warnings: List[str]
) -> tuple[bool, Optional[Path], Optional[Path]]:
    """Returns (uses_supervisord_directly, init_script, init_script_template)."""
    if entrypoint is None:
        return False, None, None

    entry_path = Path(entrypoint)
    if entry_path.name == "supervisord":
        return True, None, None

    base_name = entry_path.name  # e.g. "docker-init.sh"
    stem = entry_path.stem  # e.g. "docker-init"

    direct_candidates = _rglob_files(docker_dir, base_name)
    template_candidates = _rglob_files(docker_dir, f"{stem}.j2")

    init_script = _pick_one(
        direct_candidates, docker_dir, f"init script '{base_name}'", warnings
    )
    init_script_template = _pick_one(
        template_candidates,
        docker_dir,
        f"init script template '{stem}.j2'",
        warnings,
    )

    if init_script is None and init_script_template is None:
        warnings.append(
            f"ENTRYPOINT '{entrypoint}' but neither a literal '{base_name}' "
            f"nor a generated '{stem}.j2' template was found under {docker_dir}"
        )

    return False, init_script, init_script_template


def _discover_dockerfile_common(docker_dir: Path) -> Optional[Path]:
    candidate = docker_dir / _DOCKERFILE_COMMON_NAME
    return candidate if candidate.is_file() else None


def _discover_base_image_files(docker_dir: Path) -> Optional[Path]:
    candidate = docker_dir / _BASE_IMAGE_FILES_DIRNAME
    return candidate if candidate.is_dir() else None


def _discover_common_j2_files(docker_dir: Path) -> List[Path]:
    return _rglob_files(docker_dir, _COMMON_J2_GLOB)


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------


def build_feature_spec(entry: FeatureRegistryEntry) -> FeatureSpec:
    """Discover on-disk assets for one already-resolved
    `FeatureRegistryEntry` (see `auto_registry.discover_registry`)."""
    warnings: List[str] = []
    docker_dir = entry.docker_dir

    if docker_dir is None:
        warnings.append(
            f"registry has no docker_dir for feature '{entry.feature}' "
            f"(stem={entry.stem!r} does not exist under dockers/) -- "
            f"discovery skipped"
        )
        return FeatureSpec(
            feature=entry.feature,
            registry_entry=entry,
            docker_dir=None,
            warnings=warnings,
        )

    supervisord_conf, supervisord_is_template, supervisord_common = (
        _discover_supervisord(docker_dir, warnings)
    )
    critical_processes, critical_processes_is_template = (
        _discover_critical_processes(docker_dir, warnings)
    )
    dockerfile_j2, entrypoint = _discover_entrypoint(docker_dir, warnings)
    uses_supervisord_directly, init_script, init_script_template = (
        _discover_init_script(docker_dir, entrypoint, warnings)
    )

    return FeatureSpec(
        feature=entry.feature,
        registry_entry=entry,
        docker_dir=docker_dir,
        supervisord_conf=supervisord_conf,
        supervisord_is_template=supervisord_is_template,
        supervisord_common=supervisord_common,
        critical_processes=critical_processes,
        critical_processes_is_template=critical_processes_is_template,
        base_image_files_dir=_discover_base_image_files(docker_dir),
        dockerfile_j2=dockerfile_j2,
        entrypoint=entrypoint,
        uses_supervisord_directly=uses_supervisord_directly,
        init_script=init_script,
        init_script_template=init_script_template,
        dockerfile_common=_discover_dockerfile_common(docker_dir),
        common_j2_files=_discover_common_j2_files(docker_dir),
        warnings=warnings,
    )


def discover_features(
    sonic_root: Path,
    resolved_features: Sequence[str],
    registry: Optional[Dict[str, FeatureRegistryEntry]] = None,
) -> List[FeatureSpec]:
    """Discover `FeatureSpec`s for `resolved_features` (feature names
    already validated against the registry, e.g.
    `auto_registry.resolve_features(...).resolved` -- see
    `mega_gen_cli.py`).

    `registry` is accepted so callers who already parsed it (the CLI
    always does, to report unknown/unproven/excluded features) don't pay
    to re-parse `rules/docker-*.mk`; if omitted, this calls
    `discover_registry(sonic_root)` itself.

    Features not found in `registry` are silently skipped here -- that
    classification (unknown/unproven/excluded) is `auto_registry`'s job and
    should have already happened before this is called. Defensive only.
    """
    if registry is None:
        registry = discover_registry(sonic_root)

    specs: List[FeatureSpec] = []
    for name in resolved_features:
        entry = registry.get(name)
        if entry is None:
            continue
        specs.append(build_feature_spec(entry))
    return specs


def describe(spec: FeatureSpec) -> str:
    """One-line human-readable summary, e.g. for `--list-registry`-style
    debug output (full tabular/JSON reporting is `report.py`'s job)."""
    if spec.docker_dir is None:
        return f"{spec.feature}: NO DOCKER DIR"

    def _rel(p: Optional[Path]) -> str:
        if p is None:
            return "-"
        try:
            return str(p.relative_to(spec.docker_dir))
        except ValueError:
            return str(p)

    bits = [
        f"supervisord={_rel(spec.supervisord_conf)}"
        f"({'j2' if spec.supervisord_is_template else 'static'})",
        f"common={_rel(spec.supervisord_common)}",
        f"critical_processes={_rel(spec.critical_processes)}"
        f"({'j2' if spec.critical_processes_is_template else 'static'})",
        f"base_image_files={'y' if spec.base_image_files_dir else 'n'}",
    ]
    if spec.uses_supervisord_directly:
        bits.append("init=<direct supervisord, no wrapper>")
    elif spec.init_script is not None:
        bits.append(f"init_script={_rel(spec.init_script)}")
    elif spec.init_script_template is not None:
        bits.append(f"init_script_template={_rel(spec.init_script_template)}")
    else:
        bits.append("init_script=<none found>")
    if spec.warnings:
        bits.append(f"WARNINGS={len(spec.warnings)}")
    return f"{spec.feature}: " + " ".join(bits)
