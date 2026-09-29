#!/usr/bin/env python3
"""Validation harness for `mega_gen.gen_dockerfile` + `mega_gen.gen_docker_init`
across several feature-group scenarios.

Generator-correctness check (does `dockers/docker-mega/Dockerfile.j2` +
`docker-mega-init.sh` come out structurally sound and internally consistent
for a given feature subset?), not a hardware measurement -- belongs next to
the modules it exercises, meant to be re-run whenever either module changes
(evaluation-methodology rule: "write a script, reuse it").

Checks, per scenario:
  1. Every selected feature lands in exactly one of `common_bucket_features`/
     `rsync_bucket_features`, consistent with its own `dockerfile_common`.
  2. Generated `Dockerfile.j2` parses as valid Jinja2 syntax (doesn't need
     any variable bound -- just no `TemplateSyntaxError`).
  3. `ARG BASE=...` is present and its stem is a `*swss-layer*` dir iff
     `swss` is selected, else a `*config-engine*` dir (mirrors
     `test_gen_mega_mk.py`'s own base-layer check, cross-checking the two
     independent derivations agree).
  4. Every common-bucket feature has exactly one `{% include
     "<stem>/Dockerfile.common.j2" %}` line; every rsync-bucket feature has
     exactly one `FROM ... AS <feat>-layer` stage and exactly one
     `--mount=type=bind,from=<feat>-layer` rsync RUN block.
  5. `ARG frr_user_uid`/`ARG frr_user_gid` + the `groupadd`/`useradd`/`chown`
     block are present iff `bgp` is selected.
  6. Every selected feature with a real `start.sh` gets exactly one COPY
     into its own `/opt/sonic/core-services/<feat>/start.sh`.
  7. Every preinit script `gen_docker_init` produced is referenced by
     exactly one COPY (+ a `sonic-cfggen` render step for `.j2` ones) in the
     Dockerfile, and appears exactly once in the orchestrator script, in
     the same relative order for both.
  8. No two preinit scripts collide on `generated_filename`.
  9. Every non-static selected feature's preinit has at least one
     `exec .../supervisord` line stripped (0 is flagged as a generator
     warning already -- checked here too) and, if its own source
     mentioned the two shared paths, those no longer appear verbatim in
     the generated preinit content.
  10. Static features (`uses_supervisord_directly`) get no preinit and are
      not referenced anywhere in the orchestrator.

Usage:
    python3 test_gen_dockerfile.py --sonic-root /path/to/sonic-buildimage
    python3 test_gen_dockerfile.py --sonic-root ... --scenario single_feature_gnmi
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import List, Tuple

try:
    import jinja2
except ImportError:  # pragma: no cover -- optional dep, degrade gracefully
    jinja2 = None

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from mega_gen.auto_registry import discover_registry, resolve_features
from mega_gen.discovery import discover_features
from mega_gen.gen_dockerfile import (
    DEPSTARTUP_GUARD_FILENAME,
    DEPSTARTUP_GUARD_RUNTIME_PATH,
    DockerfileResult,
    generate_dockerfile,
)
from mega_gen.gen_docker_init import DockerInitResult

# ---------------------------------------------------------------------------
# Scenarios -- same shapes test_gen_mega_mk.py already exercises, so the two
# harnesses' results are directly comparable for the same feature subsets.
# ---------------------------------------------------------------------------

SCENARIOS: List[Tuple[str, List[str]]] = [
    ("full_default_10", ["database", "swss", "bgp", "teamd", "lldp", "snmp", "gnmi", "radv", "eventd", "sysmgr"]),
    ("no_swss_core_engine", ["database", "lldp", "snmp", "gnmi", "radv", "eventd", "sysmgr"]),
    ("swss_bgp_teamd_only", ["swss", "bgp", "teamd"]),
    ("single_feature_gnmi", ["gnmi"]),
    ("single_feature_sysmgr", ["sysmgr"]),
    # bgp without swss: exercises the FRR user/group fix independently of
    # the swss-layer base-selection path.
    ("bgp_no_swss", ["database", "bgp"]),
    # every common.j2-bucket feature, no rsync-bucket feature at all.
    ("common_bucket_only", ["database", "swss", "bgp", "lldp"]),
    # every rsync-bucket feature, no common.j2-bucket feature at all.
    ("rsync_bucket_only", ["teamd", "snmp", "gnmi", "radv", "eventd", "sysmgr"]),
]

_SWSS_LAYER_RE = re.compile(r"swss-layer", re.IGNORECASE)
_CONFIG_ENGINE_RE = re.compile(r"config-engine", re.IGNORECASE)


def check_scenario(sonic_root: Path, name: str, features: List[str]) -> bool:
    print(f"\n=== scenario: {name}  (features={features}) ===")
    registry = discover_registry(sonic_root)
    resolved = resolve_features(features, registry)
    if resolved.unknown or resolved.excluded:
        print(f"  SETUP FAILURE: unknown={resolved.unknown} excluded={resolved.excluded}")
        return False

    specs = discover_features(sonic_root, resolved.resolved, registry)
    result: DockerfileResult = generate_dockerfile(specs, sonic_root)
    docker_init: DockerInitResult = result.docker_init
    text = result.dockerfile_text

    problems: List[str] = []
    feature_names = sorted(s.feature for s in specs)

    # --- 1. every feature in exactly one bucket, consistent with discovery
    bucketed = set(result.common_bucket_features) | set(result.rsync_bucket_features)
    if bucketed != set(feature_names):
        problems.append(f"bucketed features {sorted(bucketed)} != selected {feature_names}")
    if set(result.common_bucket_features) & set(result.rsync_bucket_features):
        problems.append("a feature landed in BOTH buckets")
    for spec in specs:
        expect_common = spec.dockerfile_common is not None
        actually_common = spec.feature in result.common_bucket_features
        if expect_common != actually_common:
            problems.append(
                f"{spec.feature}: dockerfile_common is "
                f"{'set' if expect_common else 'None'} but bucketed as "
                f"{'common' if actually_common else 'rsync'}"
            )

    # --- 2. Jinja2 syntax validity ----------------------------------------
    if jinja2 is not None:
        try:
            jinja2.Environment().parse(text)
        except jinja2.TemplateSyntaxError as e:
            problems.append(f"Dockerfile.j2 failed to parse as Jinja2: {e}")

    # --- 3. base layer stem matches swss/config-engine expectation -------
    if "swss" in feature_names:
        if not result.base_stem or not _SWSS_LAYER_RE.search(result.base_stem):
            problems.append(
                f"'swss' selected but base_stem={result.base_stem!r} is not "
                f"a swss-layer dir"
            )
    elif result.base_stem is not None and not _CONFIG_ENGINE_RE.search(result.base_stem):
        problems.append(
            f"'swss' not selected but base_stem={result.base_stem!r} is not "
            f"a config-engine dir"
        )

    # --- 4. common.j2 include / rsync stage+mount, exactly once each -----
    for feat in result.common_bucket_features:
        spec = next(s for s in specs if s.feature == feat)
        pat = re.compile(rf'\{{%\s*include\s*"{re.escape(spec.stem)}/Dockerfile\.common\.j2"\s*%\}}')
        n = len(pat.findall(text))
        if n != 1:
            problems.append(f"{feat}: Dockerfile.common.j2 include appears {n} times (expected 1)")

    for feat in result.rsync_bucket_features:
        n_from = len(re.findall(rf"FROM\s+\S+\s+AS\s+{re.escape(feat)}-layer\b", text))
        n_mount = len(re.findall(rf"from={re.escape(feat)}-layer\b", text))
        if n_from != 1:
            problems.append(f"{feat}: 'FROM ... AS {feat}-layer' appears {n_from} times (expected 1)")
        if n_mount != 1:
            problems.append(f"{feat}: rsync --mount=...,from={feat}-layer appears {n_mount} times (expected 1)")

    # --- 5. FRR user/group fix iff bgp selected ---------------------------
    has_frr_fix = "groupadd -g ${frr_user_gid} frr" in text and "ARG frr_user_uid" in text
    if ("bgp" in feature_names) != has_frr_fix:
        problems.append(
            f"FRR user/group fix present={has_frr_fix} inconsistent with "
            f"'bgp' selected={('bgp' in feature_names)}"
        )

    # --- 5b. depstartup guard always present ----------------------------
    has_guard = DEPSTARTUP_GUARD_FILENAME in text and DEPSTARTUP_GUARD_RUNTIME_PATH in text
    if not has_guard:
        problems.append("depstartup guard COPY/chmod missing from Dockerfile.j2 (should be unconditional)")
    if not result.depstartup_guard_text:
        problems.append("depstartup_guard_text is empty in DockerfileResult")

    # --- 5c. FRR template symlinks iff bgp selected ----------------------
    has_frr_symlinks = "for d in bgpd zebra staticd common" in text
    if ("bgp" in feature_names) != has_frr_symlinks:
        problems.append(
            f"FRR template symlinks present={has_frr_symlinks} inconsistent "
            f"with 'bgp' selected={('bgp' in feature_names)}"
        )

    # --- 6. start.sh relocation, exactly once per feature that has one ---
    for feat in feature_names:
        spec = next(s for s in specs if s.feature == feat)
        has_start_sh = spec.docker_dir is not None and (spec.docker_dir / "start.sh").is_file()
        n = len(re.findall(
            rf'COPY --from=\S+ \["start\.sh", "/opt/sonic/core-services/{re.escape(feat)}/start\.sh"\]',
            text,
        ))
        if has_start_sh and n != 1:
            problems.append(f"{feat}: has start.sh but its relocation COPY appears {n} times (expected 1)")
        if not has_start_sh and n != 0:
            problems.append(f"{feat}: has no start.sh but a relocation COPY was emitted anyway")
        if has_start_sh != (feat in result.start_sh_features):
            problems.append(
                f"{feat}: has_start_sh={has_start_sh} inconsistent with "
                f"start_sh_features membership"
            )

    # --- 7 & 8. preinit scripts referenced exactly once, no filename clashes,
    #            same relative order in Dockerfile and orchestrator --------
    filenames = [ps.generated_filename for ps in docker_init.preinit_scripts]
    if len(filenames) != len(set(filenames)):
        problems.append(f"preinit generated_filename collision(s): {filenames}")

    dockerfile_positions = []
    orchestrator_positions = []
    for ps in docker_init.preinit_scripts:
        # Non-template: one quoted COPY source. Template: one quoted COPY
        # source + two bare-path occurrences in the build-time sonic-cfggen
        # render step (the `-t ...` source path and the `rm -f ...` cleanup).
        n_copy = len(re.findall(re.escape(ps.generated_filename), text))
        expected_copy_refs = 3 if ps.is_template else 1
        if n_copy != expected_copy_refs:
            problems.append(
                f"{ps.feature}: preinit file {ps.generated_filename!r} referenced "
                f"{n_copy} times in Dockerfile.j2 (expected {expected_copy_refs})"
            )
        m = re.search(re.escape(ps.generated_filename), text)
        dockerfile_positions.append(m.start() if m else -1)

        n_orch = len(re.findall(re.escape(ps.runtime_path), docker_init.orchestrator_text))
        if n_orch != 1:
            problems.append(
                f"{ps.feature}: runtime path {ps.runtime_path!r} appears {n_orch} "
                f"times in docker-mega-init.sh (expected 1)"
            )
        m2 = re.search(re.escape(ps.runtime_path), docker_init.orchestrator_text)
        orchestrator_positions.append(m2.start() if m2 else -1)

    if dockerfile_positions != sorted(dockerfile_positions):
        problems.append(f"preinit COPY lines in Dockerfile.j2 are out of canonical order: {list(zip(filenames, dockerfile_positions))}")
    if orchestrator_positions != sorted(orchestrator_positions):
        problems.append(f"preinit invocations in docker-mega-init.sh are out of canonical order: {list(zip(filenames, orchestrator_positions))}")

    # --- 9. exec-supervisord stripped + shared-path retargeting sanity ---
    for ps in docker_init.preinit_scripts:
        if ps.exec_supervisord_stripped == 0:
            # Already surfaced as a generator warning; still a real anomaly.
            problems.append(f"{ps.feature}: preinit has zero 'exec .../supervisord' lines stripped")
        if "/etc/supervisor/conf.d/supervisord.conf" in ps.content:
            problems.append(f"{ps.feature}: preinit content still contains the shared, un-retargeted supervisord.conf path")
        if "/etc/supervisor/critical_processes" in ps.content:
            problems.append(f"{ps.feature}: preinit content still contains the shared, un-retargeted critical_processes path")

    # --- 10. static features get no preinit, not referenced anywhere -----
    for feat in docker_init.static_features:
        if feat in filenames_by_feature(docker_init):
            problems.append(f"{feat}: uses_supervisord_directly but a preinit was generated for it")
        if f"docker-mega-{feat}-preinit.sh" in docker_init.orchestrator_text:
            problems.append(f"{feat}: static feature referenced in the orchestrator script")

    if problems:
        print(f"  FAIL ({len(problems)} problem(s)):")
        for p in problems:
            print(f"    - {p}")
        return False

    print(
        f"  OK -- base_stem={result.base_stem}, common={result.common_bucket_features}, "
        f"rsync={result.rsync_bucket_features}, start_sh={result.start_sh_features}, "
        f"preinit={filenames}, static={docker_init.static_features}"
    )
    return True


def filenames_by_feature(docker_init: DockerInitResult) -> List[str]:
    return [ps.feature for ps in docker_init.preinit_scripts]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--sonic-root", required=True, type=Path)
    ap.add_argument(
        "--scenario",
        default=None,
        help="Run only this scenario (see SCENARIOS) instead of all of them.",
    )
    args = ap.parse_args()

    scenarios = SCENARIOS
    if args.scenario:
        scenarios = [s for s in SCENARIOS if s[0] == args.scenario]
        if not scenarios:
            print(f"ERROR: unknown scenario {args.scenario!r}", file=sys.stderr)
            return 2

    if jinja2 is None:
        print("NOTE: jinja2 not importable here -- skipping syntax-validity check (#2)", file=sys.stderr)

    results = [check_scenario(args.sonic_root, name, features) for name, features in scenarios]
    passed = sum(results)
    total = len(results)
    print(f"\n=== {passed}/{total} scenarios passed ===")
    return 0 if passed == total else 1


if __name__ == "__main__":
    sys.exit(main())
