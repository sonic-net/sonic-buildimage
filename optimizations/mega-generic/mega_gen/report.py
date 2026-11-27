"""``--dry-run`` validation, dependency graph (JSON + mermaid), CHANGES.md
manifest, and the idempotent anchor-match-or-abort file-editing primitive.

Implements the ``report-dryrun`` to-do from the
``generic_mega-container_merge_script`` plan (section 5, module table):

  - **Dry-run mode** (``validate_dry_run``): run the full pipeline read-only
    — discovery + feature resolution + per-feature asset scanning — then
    report what *would* be generated, every error/warning, per-feature
    statistics, and a dependency graph (JSON + Mermaid) showing runtime
    ordering, supervisord wait-for edges, build strategy per feature, and
    external dependencies.  Nothing is written to the tree.
  - **Normal mode** (``build_changes_manifest``): after the rest of the
    pipeline has produced its outputs (``MegaMkResult``, ``DockerfileResult``,
    ``MergeResult``, ``PatchResult``), compose a ``CHANGES.md`` manifest
    listing every file created or modified, with action/size/description.
  - **Idempotent anchor-match-or-abort** (``anchor_replace``,
    ``anchor_insert_before``, ``anchor_insert_after``): the file-editing
    primitive every patching module should use.  On first run the anchor
    (a known string in the original file) is located and wrapped with
    ``# --- mega-gen: begin <label> ---`` / ``# --- mega-gen: end <label>
    ---`` markers.  On re-run the markers are found and the region between
    them is replaced in place (idempotent).  If neither the original anchor
    nor the markers are found, the edit is refused with a clear error (the
    file has been unexpectedly modified since the generator last ran).

This module does **not** write files itself (same pattern as every other
``gen_*``/``*_merge`` module).  ``mega_gen_cli.py --apply`` is responsible
for the actual writes; ``validate_dry_run`` and ``format_dry_run_report``
are used by ``mega_gen_cli.py --dry-run`` (and the ``--list-assets``
shortcut that already exists).
"""

from __future__ import annotations

import json
import re
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

from mega_gen.auto_registry import (
    PROVEN_FEATURES,
    FeatureRegistryEntry,
    ResolvedFeatures,
    discover_registry,
    resolve_features,
)
from mega_gen.discovery import FeatureSpec, discover_features
from mega_gen.supervisord_merge import CANONICAL_ORDER, PRIORITY_BAND_START, PRIORITY_BAND_WIDTH

# ---------------------------------------------------------------------------
# 1.  Idempotent anchor-match-or-abort file editing.
#
# Every file edit the generator applies to the sonic-buildimage tree goes
# through one of these three functions.  The contract:
#
#   • First run:  the *original* anchor (a substring the stock file is known
#     to contain, verified against the tree per the sonic-docs-lookup rule)
#     is located.  The edit is applied and wrapped in a pair of mega-gen
#     markers so a future re-run can recognise its own work.
#   • Re-run:  the mega-gen markers for this ``label`` are found.  The
#     region between them is replaced in place — fully idempotent, the
#     output of N re-runs is identical to the output of one.
#   • Neither found:  ``AnchorNotFoundError`` — the file has been modified
#     in a way the generator doesn't understand; refuse loudly rather than
#     silently producing wrong output.
# ---------------------------------------------------------------------------

MEGA_GEN_MARKER_PREFIX = "# --- mega-gen:"
MEGA_GEN_MARKER_SUFFIX = "---"


class AnchorNotFoundError(Exception):
    """The expected anchor AND the generator's own markers are both absent."""


def _marker_begin(label: str) -> str:
    return f"{MEGA_GEN_MARKER_PREFIX} begin {label} {MEGA_GEN_MARKER_SUFFIX}"


def _marker_end(label: str) -> str:
    return f"{MEGA_GEN_MARKER_PREFIX} end {label} {MEGA_GEN_MARKER_SUFFIX}"


def anchor_replace(
    text: str,
    anchor: str,
    replacement: str,
    *,
    label: str,
) -> Tuple[str, bool]:
    """Replace ``anchor`` with ``replacement``, wrapped in markers.

    Returns ``(new_text, was_already_applied)`` where
    ``was_already_applied`` is True when the markers were already present
    (idempotent re-run) and the region between them was refreshed.

    Raises ``AnchorNotFoundError`` if neither the original ``anchor`` nor
    the markers for ``label`` are found.
    """
    mb = _marker_begin(label)
    me = _marker_end(label)
    marked = f"{mb}\n{replacement}\n{me}"

    # Re-run: markers already present → replace the marked region.
    if mb in text and me in text:
        begin_idx = text.index(mb)
        end_idx = text.index(me) + len(me)
        return text[:begin_idx] + marked + text[end_idx:], True

    # First run: find the original anchor.
    if anchor not in text:
        raise AnchorNotFoundError(
            f"anchor {anchor!r} not found in file (and no previous mega-gen "
            f"marker for label={label!r} either) — file may have been "
            f"modified in an unexpected way; edit not applied"
        )
    return text.replace(anchor, marked, 1), False


def anchor_insert_before(
    text: str,
    anchor: str,
    insertion: str,
    *,
    label: str,
) -> Tuple[str, bool]:
    """Insert ``insertion`` just before ``anchor``, wrapped in markers.

    Idempotent: on re-run, the existing marker region is replaced.
    """
    mb = _marker_begin(label)
    me = _marker_end(label)
    marked = f"{mb}\n{insertion}\n{me}\n"

    if mb in text and me in text:
        begin_idx = text.index(mb)
        end_idx = text.index(me) + len(me)
        if end_idx < len(text) and text[end_idx] == "\n":
            end_idx += 1
        return text[:begin_idx] + marked + text[end_idx:], True

    if anchor not in text:
        raise AnchorNotFoundError(
            f"anchor {anchor!r} not found (and no previous mega-gen marker "
            f"for label={label!r}) — insertion not applied"
        )
    idx = text.index(anchor)
    return text[:idx] + marked + text[idx:], False


def anchor_insert_after(
    text: str,
    anchor: str,
    insertion: str,
    *,
    label: str,
) -> Tuple[str, bool]:
    """Insert ``insertion`` just after ``anchor`` (+ trailing newline),
    wrapped in markers.  Idempotent."""
    mb = _marker_begin(label)
    me = _marker_end(label)
    marked = f"{mb}\n{insertion}\n{me}\n"

    if mb in text and me in text:
        begin_idx = text.index(mb)
        end_idx = text.index(me) + len(me)
        if end_idx < len(text) and text[end_idx] == "\n":
            end_idx += 1
        return text[:begin_idx] + marked + text[end_idx:], True

    if anchor not in text:
        raise AnchorNotFoundError(
            f"anchor {anchor!r} not found (and no previous mega-gen marker "
            f"for label={label!r}) — insertion not applied"
        )
    idx = text.index(anchor) + len(anchor)
    if idx < len(text) and text[idx] == "\n":
        idx += 1
    return text[:idx] + marked + text[idx:], False


# ---------------------------------------------------------------------------
# 2.  Dependency graph.
#
# Captures the inter-feature relationships mega's merged container
# embodies.  Four edge kinds:
#
#   runtime_order   – canonical supervisord priority-band sequence
#   wait_for        – supervisord dependent_startup_wait_for chain edges
#   build_strategy  – Dockerfile strategy per feature (include vs rsync)
#   external_dep    – dependencies on containers *outside* mega (syncd)
# ---------------------------------------------------------------------------


@dataclass
class DependencyEdge:
    source: str
    target: str
    kind: str
    label: str = ""


@dataclass
class DependencyGraph:
    """Directed graph of inter-feature (and external) relationships."""

    nodes: List[str] = field(default_factory=list)
    """Feature names, in canonical order."""

    edges: List[DependencyEdge] = field(default_factory=list)

    node_attrs: Dict[str, Dict[str, str]] = field(default_factory=dict)
    """Per-node attributes for rendering (e.g. Dockerfile strategy)."""

    def to_json(self) -> str:
        """Compact JSON representation (machine-readable store)."""
        return json.dumps(
            {
                "nodes": [
                    {"name": n, **(self.node_attrs.get(n, {}))}
                    for n in self.nodes
                ],
                "edges": [
                    {
                        "source": e.source,
                        "target": e.target,
                        "kind": e.kind,
                        "label": e.label,
                    }
                    for e in self.edges
                ],
            },
            indent=2,
        )

    def to_mermaid(self) -> str:
        """Mermaid ``graph LR`` text (for CHANGES.md / terminal output)."""
        lines = ["graph LR"]

        for n in self.nodes:
            attrs = self.node_attrs.get(n, {})
            strategy = attrs.get("strategy", "")
            suffix = f"<br/>{strategy}" if strategy else ""
            lines.append(f"    {n}[{n}{suffix}]")
        lines.append("")

        _STYLE = {
            "runtime_order": "-->",
            "wait_for": "-.->",
            "build_strategy": "==>",
            "external_dep": "--x",
        }
        for kind, arrow in _STYLE.items():
            kind_edges = [e for e in self.edges if e.kind == kind]
            if not kind_edges:
                continue
            lines.append(f"    %% {kind}")
            for e in kind_edges:
                label_part = f"|{e.label}|" if e.label else ""
                lines.append(f"    {e.source} {arrow} {label_part}{e.target}")
        return "\n".join(lines)


def build_dependency_graph(
    specs: Sequence[FeatureSpec],
) -> DependencyGraph:
    """Build a dependency graph for the resolved feature set.

    Does NOT require ``sonic_root`` or running any pipeline modules —
    only the already-discovered ``FeatureSpec``\\ s (cheap, always
    available in both dry-run and normal modes).
    """
    order_index = {name: i for i, name in enumerate(CANONICAL_ORDER)}
    ordered = sorted(
        specs,
        key=lambda s: order_index.get(s.feature, len(CANONICAL_ORDER) + 1),
    )

    graph = DependencyGraph(nodes=[s.feature for s in ordered])

    # Per-node attributes
    for spec in ordered:
        strategy = (
            "include (Dockerfile.common.j2)"
            if spec.dockerfile_common is not None
            else "rsync (built image)"
        )
        init_kind = (
            "direct-supervisord"
            if spec.uses_supervisord_directly
            else (
                "template (.j2)"
                if spec.init_script_template
                else ("script (.sh)" if spec.init_script else "none")
            )
        )
        graph.node_attrs[spec.feature] = {
            "strategy": strategy,
            "init": init_kind,
            "proven": "y" if spec.proven else "n",
            "core": "y" if spec.core else "n",
        }

    # Runtime ordering (supervisord priority bands)
    for i in range(1, len(ordered)):
        prev_band = PRIORITY_BAND_START + (i - 1) * PRIORITY_BAND_WIDTH
        curr_band = PRIORITY_BAND_START + i * PRIORITY_BAND_WIDTH
        graph.edges.append(
            DependencyEdge(
                source=ordered[i - 1].feature,
                target=ordered[i].feature,
                kind="runtime_order",
                label=f"band {prev_band}→{curr_band}",
            )
        )

    # Build strategy edges (mega → each feature)
    for spec in ordered:
        strategy = (
            "include" if spec.dockerfile_common is not None else "rsync"
        )
        graph.edges.append(
            DependencyEdge(
                source="mega",
                target=spec.feature,
                kind="build_strategy",
                label=strategy,
            )
        )
    if ordered:
        graph.nodes.append("mega")

    # External dependency: swss → syncd (always outside mega)
    selected_names = {s.feature for s in ordered}
    if "swss" in selected_names:
        graph.nodes.append("syncd")
        graph.edges.append(
            DependencyEdge(
                source="swss",
                target="syncd",
                kind="external_dep",
                label="lockstep",
            )
        )

    return graph


# ---------------------------------------------------------------------------
# 3.  Dry-run validation.
# ---------------------------------------------------------------------------


@dataclass
class ValidationResult:
    """Complete dry-run validation output — everything a human or the CLI
    needs to decide whether the requested feature set is sound."""

    ok: bool
    """True if no hard errors (unknown/excluded features).  Warnings alone
    do not flip this to False."""

    feature_count: int
    resolved_features: List[str] = field(default_factory=list)

    errors: List[str] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)
    discovery_warnings: List[str] = field(default_factory=list)

    specs: List[FeatureSpec] = field(default_factory=list)
    graph: Optional[DependencyGraph] = None

    stats: Dict[str, int] = field(default_factory=dict)
    """Asset-level counters (total_features, proven, core, has_common_j2,
    rsync_bucket, has_init_script, has_supervisord_conf, …)."""


def validate_dry_run(
    sonic_root: Path,
    requested_features: Sequence[str],
    registry: Optional[Dict[str, FeatureRegistryEntry]] = None,
) -> ValidationResult:
    """Run the full pipeline validation without touching any files.

    Performs auto-registry discovery, feature resolution, per-feature asset
    scanning, builds the dependency graph, and computes summary statistics.
    All anomalies are captured in ``errors``/``warnings``/
    ``discovery_warnings`` rather than raised.
    """
    result = ValidationResult(ok=True, feature_count=0)

    # --- registry ---
    if registry is None:
        registry = discover_registry(sonic_root)

    # --- feature resolution ---
    resolved = resolve_features(requested_features, registry)
    result.resolved_features = list(resolved.resolved)
    result.feature_count = len(resolved.resolved)

    if resolved.excluded:
        result.errors.append(
            f"hard-excluded features requested: {', '.join(resolved.excluded)}"
        )
        result.ok = False
    if resolved.unknown:
        result.errors.append(
            f"unknown features not in auto-discovered registry: "
            f"{', '.join(resolved.unknown)}"
        )
        result.ok = False
    if resolved.unproven:
        result.warnings.append(
            f"unproven features (not in the validated 11): "
            f"{', '.join(resolved.unproven)}"
        )

    if not result.ok:
        return result

    # --- per-feature discovery ---
    specs = discover_features(sonic_root, resolved.resolved, registry)
    result.specs = specs

    for spec in specs:
        for w in spec.warnings:
            result.discovery_warnings.append(f"{spec.feature}: {w}")

    # --- statistics ---
    has_common_j2 = sum(
        1 for s in specs if s.dockerfile_common is not None
    )
    has_init_script = sum(
        1
        for s in specs
        if s.init_script is not None or s.init_script_template is not None
    )
    uses_supervisord_directly = sum(
        1 for s in specs if s.uses_supervisord_directly
    )
    has_base_image_files = sum(
        1 for s in specs if s.base_image_files_dir is not None
    )
    has_supervisord = sum(
        1 for s in specs if s.supervisord_conf is not None
    )
    has_critical_processes = sum(
        1 for s in specs if s.critical_processes is not None
    )
    proven_count = sum(1 for s in specs if s.proven)
    core_count = sum(1 for s in specs if s.core)

    result.stats = {
        "total_features": len(specs),
        "proven": proven_count,
        "unproven": len(specs) - proven_count,
        "core": core_count,
        "toggled": len(specs) - core_count,
        "has_dockerfile_common_j2": has_common_j2,
        "rsync_bucket": len(specs) - has_common_j2,
        "has_init_script": has_init_script,
        "uses_supervisord_directly": uses_supervisord_directly,
        "has_base_image_files": has_base_image_files,
        "has_supervisord_conf": has_supervisord,
        "has_critical_processes": has_critical_processes,
    }

    # --- dependency graph ---
    result.graph = build_dependency_graph(specs)

    return result


# ---------------------------------------------------------------------------
# 4.  CHANGES.md manifest.
# ---------------------------------------------------------------------------


@dataclass
class FileChange:
    relpath: str
    action: str  # "created" | "patched"
    description: str
    size_bytes: int = 0
    line_count: int = 0


@dataclass
class ChangesManifest:
    timestamp: str
    features: List[str]
    container_name: str
    changes: List[FileChange] = field(default_factory=list)
    warnings_count: int = 0
    graph: Optional[DependencyGraph] = None

    def to_markdown(self) -> str:
        """Render the full CHANGES.md body."""
        lines = [
            "# CHANGES.md — Mega Container Generator Manifest",
            "",
            f"**Generated:** {self.timestamp}  ",
            f"**Container:** `{self.container_name}`  ",
            f"**Features ({len(self.features)}):** "
            f"{', '.join(self.features)}  ",
            f"**Warnings:** {self.warnings_count}",
            "",
        ]

        # Summary table
        created = sorted(
            (c for c in self.changes if c.action == "created"),
            key=lambda c: c.relpath,
        )
        patched = sorted(
            (c for c in self.changes if c.action == "patched"),
            key=lambda c: c.relpath,
        )

        lines.append("## Files overview")
        lines.append("")
        lines.append("| Action | Path | Lines | Description |")
        lines.append("|--------|------|------:|-------------|")
        for c in created + patched:
            lines.append(
                f"| {c.action} | `{c.relpath}` "
                f"| {c.line_count} | {c.description} |"
            )
        lines.append("")

        if created:
            lines.append(f"### Generated new ({len(created)} files)")
            lines.append("")
            for c in created:
                lines.append(f"- `{c.relpath}` — {c.description}")
            lines.append("")

        if patched:
            lines.append(f"### Patched in place ({len(patched)} files)")
            lines.append("")
            for c in patched:
                lines.append(f"- `{c.relpath}` — {c.description}")
            lines.append("")

        # Dependency graph (Mermaid)
        if self.graph:
            lines.append("## Dependency graph")
            lines.append("")
            lines.append("```mermaid")
            lines.append(self.graph.to_mermaid())
            lines.append("```")
            lines.append("")

        return "\n".join(lines)


# Description lookup for well-known paths.
_STATIC_DESCRIPTIONS: Dict[str, str] = {}  # populated by _describe_file


def _describe_file(relpath: str, container_name: str) -> str:
    """Human-readable one-line description of what this file is/does."""
    # Exact-match table (built once per container_name).
    _exact: Dict[str, str] = {
        f"rules/docker-{container_name}.mk": (
            "Build rules (INCLUDE_DOCKER, LOAD_DOCKERS, RUN_OPT, …)"
        ),
        f"rules/docker-{container_name}.dep": (
            "DPKG dependency-cache metadata"
        ),
        f"dockers/docker-{container_name}/Dockerfile.j2": (
            "Multi-stage Dockerfile (hybrid include/rsync strategy)"
        ),
        f"dockers/docker-{container_name}/docker-{container_name}-init.sh": (
            "Container ENTRYPOINT orchestrator (preinit pattern)"
        ),
        f"dockers/docker-{container_name}/docker-mega-init.sh": (
            "Container ENTRYPOINT orchestrator (preinit pattern)"
        ),
        f"dockers/docker-{container_name}/supervisord.conf.j2": (
            "Merged supervisord config template"
        ),
        f"dockers/docker-{container_name}/critical_processes": (
            "Union of critical process lists"
        ),
        f"dockers/docker-{container_name}/docker_image_ctl.j2": (
            "Container lifecycle script (start/stop/wait/kill)"
        ),
        f"dockers/docker-{container_name}/depstartup_guard.py": (
            "Pending-start guard (respawn storm prevention)"
        ),
        f"files/build_templates/{container_name}.service.j2": (
            "Systemd service unit for mega"
        ),
        "files/build_templates/init_cfg.json.j2": (
            "FEATURE table (folded features deleted, mega added)"
        ),
        "files/build_templates/sonic_debian_extension.j2": (
            "Service alias symlinks for folded features"
        ),
        "rules/config": (
            "INCLUDE_* flags set to n for folded features"
        ),
    }

    if relpath in _exact:
        return _exact[relpath]

    bif_prefix = f"dockers/docker-{container_name}/base_image_files/"
    if relpath.startswith(bif_prefix):
        name = relpath[len(bif_prefix):]
        return (
            f"Host CLI wrapper / monit config ({name}), "
            f"retargeted to {container_name}"
        )

    preinit_prefix = f"dockers/docker-{container_name}/preinit-"
    if relpath.startswith(preinit_prefix):
        feat = (
            relpath[len(preinit_prefix):]
            .replace(".sh.j2", "")
            .replace(".sh", "")
        )
        return f"Stripped preinit script for {feat}"

    if relpath.endswith((".service.j2", ".service")):
        return "Systemd unit (fan-in: folded feature refs → mega)"

    if re.match(r"^rules/docker-.*\.mk$", relpath):
        return "Folded feature's .mk (SONIC_INSTALL commented out)"

    if "syncd" in relpath:
        return "syncd service (swss dependency repointed to mega)"

    return "Modified by mega-gen"


def build_changes_manifest(
    file_patches: Dict[str, str],
    generated_files: Dict[str, str],
    features: Sequence[str],
    container_name: str = "mega",
    warnings_count: int = 0,
    graph: Optional[DependencyGraph] = None,
) -> ChangesManifest:
    """Build a CHANGES.md manifest from the generator's combined outputs.

    Parameters
    ----------
    file_patches
        ``{relpath: new_text}`` for files patched in place (the union of
        ``PatchResult.file_patches`` and any other modified files).
    generated_files
        ``{relpath: content}`` for brand-new files the generator creates
        (``dockers/docker-mega/*``, ``rules/docker-mega.mk``, etc.).
    features
        Resolved feature list (in canonical order).
    container_name
        Mega container's name (default ``mega``).
    warnings_count
        Total warnings across all pipeline modules.
    graph
        Pre-computed dependency graph (embedded in the markdown output).
    """
    manifest = ChangesManifest(
        timestamp=datetime.now(timezone.utc).strftime(
            "%Y-%m-%d %H:%M:%S UTC"
        ),
        features=list(features),
        container_name=container_name,
        warnings_count=warnings_count,
        graph=graph,
    )

    for relpath, content in generated_files.items():
        manifest.changes.append(
            FileChange(
                relpath=relpath,
                action="created",
                description=_describe_file(relpath, container_name),
                size_bytes=len(content.encode("utf-8")),
                line_count=len(content.splitlines()),
            )
        )

    for relpath, content in file_patches.items():
        if relpath in generated_files:
            continue  # already counted as "created"
        manifest.changes.append(
            FileChange(
                relpath=relpath,
                action="patched",
                description=_describe_file(relpath, container_name),
                size_bytes=len(content.encode("utf-8")),
                line_count=len(content.splitlines()),
            )
        )

    return manifest


# ---------------------------------------------------------------------------
# 5.  Human-readable dry-run report (terminal / console output).
# ---------------------------------------------------------------------------


def format_dry_run_report(result: ValidationResult) -> str:
    """Format a complete, human-readable dry-run report.

    Meant for ``mega_gen_cli.py --dry-run``'s stdout — includes the
    feature table, statistics, warnings, and the full dependency graph
    in both Mermaid and JSON.
    """
    lines: List[str] = []
    hr = "=" * 72

    lines.append(hr)
    lines.append("  MEGA CONTAINER GENERATOR — DRY-RUN VALIDATION REPORT")
    lines.append(hr)
    lines.append("")

    # --- feature resolution ---
    status = "PASS" if result.ok else "FAIL"
    lines.append(f"Feature resolution: {status}")
    lines.append(
        f"  Resolved ({result.feature_count}): "
        f"{', '.join(result.resolved_features)}"
    )
    lines.append("")

    # --- errors ---
    if result.errors:
        lines.append(f"ERRORS ({len(result.errors)}):")
        for e in result.errors:
            lines.append(f"  ERROR: {e}")
        lines.append("")

    # --- per-feature summary table ---
    if result.specs:
        lines.append("Per-feature summary:")
        headers = (
            "feature",
            "stem",
            "common.j2",
            "init",
            "supervisord",
            "proven",
            "core",
        )
        rows: List[Tuple[str, ...]] = [headers]
        for spec in result.specs:
            init_kind = (
                "direct-svd"
                if spec.uses_supervisord_directly
                else (
                    "template"
                    if spec.init_script_template
                    else ("script" if spec.init_script else "none")
                )
            )
            sup_kind = (
                "j2"
                if spec.supervisord_is_template
                else ("static" if spec.supervisord_conf else "none")
            )
            rows.append(
                (
                    spec.feature,
                    spec.stem,
                    "y" if spec.dockerfile_common else "n",
                    init_kind,
                    sup_kind,
                    "y" if spec.proven else "n",
                    "y" if spec.core else "n",
                )
            )
        widths = [
            max(len(str(row[i])) for row in rows) for i in range(len(headers))
        ]
        for i, row in enumerate(rows):
            line = "  " + "  ".join(
                str(cell).ljust(widths[j]) for j, cell in enumerate(row)
            )
            lines.append(line)
            if i == 0:
                lines.append("  " + "  ".join("-" * w for w in widths))
        lines.append("")

    # --- statistics ---
    if result.stats:
        lines.append("Asset statistics:")
        for key, val in result.stats.items():
            lines.append(f"  {key}: {val}")
        lines.append("")

    # --- all warnings ---
    all_warnings = result.warnings + result.discovery_warnings
    if all_warnings:
        lines.append(f"WARNINGS ({len(all_warnings)}):")
        for w in all_warnings:
            lines.append(f"  WARNING: {w}")
        lines.append("")

    # --- dependency graph ---
    if result.graph:
        lines.append("Dependency graph (Mermaid):")
        lines.append("```mermaid")
        lines.append(result.graph.to_mermaid())
        lines.append("```")
        lines.append("")

        lines.append("Dependency graph (JSON):")
        lines.append(result.graph.to_json())
        lines.append("")

    lines.append(hr)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Standalone entry point.
#
#   python3 -m mega_gen.report --sonic-root /path/to/sonic-buildimage \
#       --features database,swss,bgp,teamd,lldp,snmp,gnmi,radv,eventd,sysmgr
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    import argparse
    import sys

    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--sonic-root", required=True, type=Path)
    ap.add_argument(
        "--features",
        default="database,swss,bgp,teamd,lldp,snmp,gnmi,radv,eventd,sysmgr",
    )
    ap.add_argument(
        "--out-graph-json",
        type=Path,
        default=None,
        help="Write the dependency graph as JSON to this path.",
    )
    ap.add_argument(
        "--out-graph-mermaid",
        type=Path,
        default=None,
        help="Write the dependency graph as Mermaid to this path.",
    )
    args = ap.parse_args()

    vr = validate_dry_run(
        args.sonic_root, args.features.split(",")
    )
    print(format_dry_run_report(vr))

    if vr.graph:
        if args.out_graph_json:
            args.out_graph_json.write_text(vr.graph.to_json())
            print(f"wrote {args.out_graph_json}")
        if args.out_graph_mermaid:
            args.out_graph_mermaid.write_text(vr.graph.to_mermaid())
            print(f"wrote {args.out_graph_mermaid}")

    sys.exit(0 if vr.ok else 1)
