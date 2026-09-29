#!/usr/bin/env python3
"""CLI entry point for the generic mega-container merge generator.

    mega_gen_cli.py --sonic-root /build-sonic-arm/sonic-buildimage \
        --features bgp,swss,lldp,...

`--features` overrides `features.default.yaml`; omit it to read that file.

Modes:
  - Default (no flag): dry run — resolve features and report what would be generated.
  - ``--list-registry``: print the full auto-discovered feature registry.
  - ``--list-assets``: resolve features + print per-feature asset discovery.
  - ``--apply --out-dir DIR``: run the full pipeline and write all generated
    files into ``DIR`` (staged output for inspection/diff, never writes
    directly into the sonic-buildimage tree).
  - ``--apply`` (no ``--out-dir``): print what would be generated, but write nothing.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Dict, List

from mega_gen.auto_registry import (
    HARD_EXCLUDED_FEATURES,
    FeatureRegistryEntry,
    ResolvedFeatures,
    discover_registry,
    resolve_features,
)
from mega_gen.discovery import describe, discover_features
from mega_gen.gen_mega_mk import generate_mega_mk, MEGA_CONTAINER_NAME_DEFAULT
from mega_gen.gen_dockerfile import generate_dockerfile, MEGA_DOCKERFILE_NAME, DEPSTARTUP_GUARD_FILENAME, generate_depstartup_guard
from mega_gen.supervisord_merge import merge_supervisord
from mega_gen.patch_templates import generate_patches
from mega_gen.report import validate_dry_run, format_dry_run_report, build_changes_manifest, build_dependency_graph

HERE = Path(__file__).resolve().parent
DEFAULT_FEATURES_YAML = HERE / "features.default.yaml"


def _load_default_features(yaml_path: Path) -> List[str]:
    """Read the `features:` list out of `features.default.yaml`.

    Tries real PyYAML first (not guaranteed present on the ARM build VM);
    falls back to a tiny hand-rolled parser tailored to this file's
    deliberately simple, flat shape (`key: value` + `  - item` lists,
    `#`-comments) so this CLI has zero hard dependency on PyYAML.
    """
    text = yaml_path.read_text()
    try:
        import yaml  # type: ignore

        data = yaml.safe_load(text) or {}
        return list(data.get("features", []))
    except ImportError:
        pass

    features: List[str] = []
    in_features = False
    for raw_line in text.splitlines():
        line = raw_line.split("#", 1)[0].rstrip()
        if not line.strip():
            continue
        if not raw_line.startswith((" ", "\t")):
            in_features = line.strip().rstrip(":") == "features"
            continue
        if in_features:
            stripped = line.strip()
            if stripped.startswith("- "):
                features.append(stripped[2:].strip())
    return features


def _print_registry_table(entries: List[FeatureRegistryEntry]) -> None:
    headers = ("feature", "docker dir", "mk file", "include flag", "core", "proven")
    rows = [headers]
    for e in sorted(entries, key=lambda x: x.feature):
        rows.append(
            (
                e.feature,
                e.stem or "?",
                e.mk_file,
                e.include_flag or ("-" if not e.installed_via else e.installed_via),
                "y" if e.core else "n",
                "y" if e.proven else "n",
            )
        )
    widths = [max(len(row[i]) for row in rows) for i in range(len(headers))]
    for i, row in enumerate(rows):
        line = "  ".join(cell.ljust(widths[j]) for j, cell in enumerate(row))
        print(line)
        if i == 0:
            print("  ".join("-" * w for w in widths))


def _report_resolution(resolved: ResolvedFeatures) -> bool:
    """Print the resolution report. Returns True if generation may proceed."""
    ok = True
    if resolved.excluded:
        print(
            f"ERROR: hard-excluded feature(s) requested: {', '.join(resolved.excluded)} "
            f"(always excluded: {', '.join(sorted(HARD_EXCLUDED_FEATURES))})",
            file=sys.stderr,
        )
        ok = False
    if resolved.unknown:
        print(
            f"ERROR: unknown feature(s) not found in the auto-discovered registry: "
            f"{', '.join(resolved.unknown)} (run --list-registry to see what's available)",
            file=sys.stderr,
        )
        ok = False
    if resolved.unproven:
        print(
            f"WARNING: unproven feature(s) requested (not in the validated 11): "
            f"{', '.join(resolved.unproven)} -- proceeding, but expect rough edges.",
            file=sys.stderr,
        )
    print(f"Resolved features ({len(resolved.resolved)}): {', '.join(resolved.resolved)}")
    return ok


def main(argv: List[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--sonic-root",
        required=True,
        type=Path,
        help="Path to the sonic-buildimage tree to discover features from "
        "and (eventually) patch, e.g. /build-sonic-arm/sonic-buildimage.",
    )
    parser.add_argument(
        "--features",
        default=None,
        help="Comma-separated feature list, e.g. bgp,swss,lldp. Overrides "
        "--config entirely when given.",
    )
    parser.add_argument(
        "--config",
        default=DEFAULT_FEATURES_YAML,
        type=Path,
        help=f"Feature-list YAML, read when --features is omitted "
        f"(default: {DEFAULT_FEATURES_YAML.name}).",
    )
    parser.add_argument(
        "--list-registry",
        action="store_true",
        help="Print the full auto-discovered feature registry and exit "
        "(ignores --features/--config).",
    )
    parser.add_argument(
        "--list-assets",
        action="store_true",
        help="Resolve --features/--config as usual, then print the "
        "per-feature asset discovery (mega_gen.discovery) for each "
        "resolved feature and exit -- no generation.",
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        help="Run the full generation pipeline. When combined with "
        "--out-dir, writes all generated files into that directory "
        "(staged output for inspection/diff). Without --out-dir, "
        "prints a summary but writes nothing.",
    )
    parser.add_argument(
        "--out-dir",
        default=None,
        type=Path,
        help="Directory to write all generated files into (only used "
        "with --apply). Files are laid out as they would appear "
        "relative to sonic-root.",
    )
    parser.add_argument(
        "--container-name",
        default=MEGA_CONTAINER_NAME_DEFAULT,
        help=f"Container name for the mega container "
        f"(default: {MEGA_CONTAINER_NAME_DEFAULT}).",
    )
    args = parser.parse_args(argv)

    sonic_root: Path = args.sonic_root
    if not (sonic_root / "rules").is_dir():
        print(
            f"ERROR: {sonic_root} does not look like a sonic-buildimage tree "
            f"(no rules/ subdir).",
            file=sys.stderr,
        )
        return 2

    registry = discover_registry(sonic_root)

    if args.list_registry:
        _print_registry_table(list(registry.values()))
        return 0

    if args.features is not None:
        requested = [f for f in args.features.split(",") if f.strip()]
    else:
        requested = _load_default_features(args.config)

    resolved = resolve_features(requested, registry)
    if not _report_resolution(resolved):
        return 1

    if args.list_assets:
        specs = discover_features(sonic_root, resolved.resolved, registry)
        for spec in specs:
            print(describe(spec))
            for warning in spec.warnings:
                print(f"   WARNING: {warning}")
        return 0

    if args.apply:
        return _run_apply(sonic_root, resolved.resolved, registry, args)

    print(
        "\n(dry run: nothing written. Pass --apply to run the full "
        "generation pipeline, or --list-registry to inspect discovery output.)"
    )
    return 0


def _run_apply(
    sonic_root: Path,
    resolved_features: List[str],
    registry: dict,
    args: argparse.Namespace,
) -> int:
    """Run the full generation pipeline and optionally write output."""
    container_name = args.container_name
    out_dir: Path | None = args.out_dir
    all_warnings: List[str] = []

    print(f"=== Running full pipeline for container '{container_name}' ===")
    print(f"Features: {', '.join(resolved_features)}")
    print()

    # 1. Feature discovery
    print("[1/5] Feature discovery...")
    specs = discover_features(sonic_root, resolved_features, registry)
    for spec in specs:
        for w in spec.warnings:
            all_warnings.append(f"discovery/{spec.feature}: {w}")
    print(f"  Discovered {len(specs)} features")

    # 2. Supervisord merge
    print("[2/5] Supervisord merge...")
    sup_result = merge_supervisord(specs, container_name=container_name)
    all_warnings.extend(f"supervisord: {w}" for w in sup_result.warnings)
    print(f"  Feature order: {sup_result.feature_order}")
    print(f"  Starter features: {sup_result.starter_features}")
    print(f"  Priority bands: {sup_result.priority_bands}")

    # 3. Generate docker-mega.mk
    print("[3/5] Generate rules/docker-mega.mk...")
    mk_result = generate_mega_mk(specs, sonic_root, container_name=container_name)
    all_warnings.extend(f"gen_mega_mk: {w}" for w in mk_result.warnings)
    print(f"  Include features: {mk_result.include_docker_features}")
    print(f"  Base load docker: {mk_result.base_load_docker}")
    print(f"  Load dockers: {mk_result.load_dockers}")
    print(f"  Base image files: {len(mk_result.base_image_files)}")

    # 4. Generate Dockerfile.j2 + preinit + depstartup guard
    print("[4/5] Generate Dockerfile.j2 + preinit scripts...")
    df_result = generate_dockerfile(specs, sonic_root, container_name=container_name)
    all_warnings.extend(f"gen_dockerfile: {w}" for w in df_result.warnings)
    print(f"  Base stem: {df_result.base_stem}")
    print(f"  Common bucket: {df_result.common_bucket_features}")
    print(f"  Rsync bucket: {df_result.rsync_bucket_features}")
    print(f"  Start.sh features: {df_result.start_sh_features}")

    # 5. Patch templates (docker_image_ctl.j2, init_cfg.json.j2, etc.)
    print("[5/5] Patch templates...")
    patch_result = generate_patches(specs, sonic_root, container_name=container_name)
    all_warnings.extend(f"patch_templates: {w}" for w in patch_result.warnings)
    print(f"  File patches: {len(patch_result.file_patches)}")

    # --- Collect all generated files ---
    mega_docker_dir = f"dockers/docker-{container_name}"
    generated_files: Dict[str, str] = {}

    # rules/docker-mega.mk + .dep
    generated_files[f"rules/docker-{container_name}.mk"] = mk_result.mk_text
    generated_files[f"rules/docker-{container_name}.dep"] = mk_result.dep_text

    # dockers/docker-mega/Dockerfile.j2
    generated_files[f"{mega_docker_dir}/Dockerfile.j2"] = df_result.dockerfile_text

    # supervisord.conf.j2 + critical_processes
    generated_files[f"{mega_docker_dir}/supervisord.conf.j2"] = sup_result.supervisord_conf_text
    critical_is_template = bool(
        df_result.docker_init and df_result.docker_init.critical_processes_is_template
    )
    critical_filename = "critical_processes.j2" if critical_is_template else "critical_processes"
    generated_files[f"{mega_docker_dir}/{critical_filename}"] = sup_result.critical_processes_text

    # docker-mega-init.sh + preinit scripts
    if df_result.docker_init:
        generated_files[f"{mega_docker_dir}/{df_result.docker_init.orchestrator_filename}"] = \
            df_result.docker_init.orchestrator_text
        for ps in df_result.docker_init.preinit_scripts:
            generated_files[f"{mega_docker_dir}/{ps.generated_filename}"] = ps.content

    # depstartup guard
    generated_files[f"{mega_docker_dir}/{DEPSTARTUP_GUARD_FILENAME}"] = df_result.depstartup_guard_text

    # base_image_files
    for bif in mk_result.base_image_files:
        if bif.content is not None:
            generated_files[f"{mega_docker_dir}/base_image_files/{bif.name}"] = bif.content

    # All remaining patch_result.file_patches are patched-in-place files
    patched_files: Dict[str, str] = dict(patch_result.file_patches)

    # --- Summary ---
    print()
    print(f"=== GENERATED FILES ({len(generated_files)}) ===")
    for relpath in sorted(generated_files):
        text = generated_files[relpath]
        print(f"  {relpath} ({len(text.splitlines())} lines, {len(text)} bytes)")

    print(f"\n=== PATCHED FILES ({len(patched_files)}) ===")
    for relpath in sorted(patched_files):
        text = patched_files[relpath]
        print(f"  {relpath} ({len(text.splitlines())} lines, {len(text)} bytes)")

    print(f"\n=== WARNINGS ({len(all_warnings)}) ===")
    for w in all_warnings:
        print(f"  WARNING: {w}")

    # --- Write output ---
    if out_dir is not None:
        out_dir.mkdir(parents=True, exist_ok=True)
        written = 0
        for relpath, content in {**generated_files, **patched_files}.items():
            out_path = out_dir / relpath
            out_path.parent.mkdir(parents=True, exist_ok=True)
            out_path.write_text(content)
            written += 1
        print(f"\nWrote {written} files to {out_dir}/")

        # Also write a CHANGES.md manifest
        graph = build_dependency_graph(specs)
        manifest = build_changes_manifest(
            file_patches=patched_files,
            generated_files=generated_files,
            features=resolved_features,
            container_name=container_name,
            warnings_count=len(all_warnings),
            graph=graph,
        )
        changes_path = out_dir / "CHANGES.md"
        changes_path.write_text(manifest.to_markdown())
        print(f"Wrote {changes_path}")
    else:
        print(
            "\n(--out-dir not specified: nothing written. Pass --out-dir to "
            "write all generated files to a staging directory.)"
        )

    return 0


if __name__ == "__main__":
    sys.exit(main())
