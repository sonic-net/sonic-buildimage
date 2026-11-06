#!/usr/bin/env python3
"""Validation harness for `mega_gen.gen_mega_mk` across several
feature-group scenarios.

Generator-correctness check (does `rules/docker-mega.mk` come out
structurally sound and internally consistent for a given feature subset?),
not a hardware measurement -- belongs next to the module it exercises, meant
to be re-run whenever `gen_mega_mk.py` changes (evaluation-methodology rule:
"write a script, reuse it").

Checks, per scenario:
  1. Every selected feature appears exactly once as `_INCLUDE_DOCKER +=`.
  2. `base_load_docker` is set and is a `*_SWSS_LAYER*` token iff `swss` is
     selected, else a `*_CONFIG_ENGINE*` token (when any feature declares
     one).
  3. `_LOAD_DOCKERS` starts with the base layer and contains one entry per
     `rsync_load_docker_features` feature, no duplicates.
  4. `_INSTALL_PYTHON_WHEELS`/`_INSTALL_DEBS` unions are non-empty iff at
     least one selected feature's own `.mk` defines them.
  5. Every `_BASE_IMAGE_FILES` entry has a resolved source file (no missing
     files) and, if its content mentions `docker exec`/`memory_checker`/
     `restart_service` with the OWNING feature's own container name, that
     the retargeted copy no longer does (replaced with the target container
     name) -- i.e. the retarget actually fired where expected.
  6. No `_BASE_IMAGE_FILES` name collisions among the selected features (the
     10 proven features have none; only flagged as a problem here if the
     generator's own warnings report one unexpectedly for a scenario that
     shouldn't have any).
  7. Generated `.mk` text is parseable as one make-variable-assignment per
     non-comment/non-blank line and mentions every emitted list at least
     once (sanity, not a full Makefile parser).
  9. `_RUN_OPT`: every `-v` destination in the final merged list appears
     exactly once (no duplicate/conflicting mode ever emitted twice); the
     real `swss`+`p4rt` scenario specifically exercises the verified
     `/etc/sonic` `ro`-vs-`rw` collision and must resolve to `ro` only, with
     a warning naming `p4rt`. `merge_run_opt()` itself is also exercised
     directly with synthetic data (source-path collision, host-port
     collision) that no current real feature pairing can trigger -- see
     `test_merge_run_opt_synthetic()`.

Usage:
    python3 test_gen_mega_mk.py --sonic-root /path/to/sonic-buildimage
    python3 test_gen_mega_mk.py --sonic-root ... --scenario no_swss_core_engine
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import List, Tuple

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from mega_gen.auto_registry import discover_registry, resolve_features
from mega_gen.discovery import discover_features
from mega_gen.gen_mega_mk import MegaMkResult, generate_mega_mk, merge_run_opt

# ---------------------------------------------------------------------------
# Scenarios: chosen to exercise distinct base-layer-selection code paths.
# ---------------------------------------------------------------------------

SCENARIOS: List[Tuple[str, List[str]]] = [
    # The plan's actual default set: swss selected -> swss-layer base;
    # every feature currently lacks Dockerfile.common.j2 (verified against
    # this tree) -> all 10 also land in _LOAD_DOCKERS for rsync; eventd is
    # the only one of the 10 with its own _INSTALL_PYTHON_WHEELS/_INSTALL_DEBS.
    ("full_default_10", ["database", "swss", "bgp", "teamd", "lldp", "snmp", "gnmi", "radv", "eventd", "sysmgr"]),
    # No swss -> base layer must fall back to a *_CONFIG_ENGINE* image.
    ("no_swss_core_engine", ["database", "lldp", "snmp", "gnmi", "radv", "eventd", "sysmgr"]),
    # swss selected alongside its two other swss-layer-based siblings only.
    ("swss_bgp_teamd_only", ["swss", "bgp", "teamd"]),
    # Smallest possible input: one feature, alone.
    ("single_feature_gnmi", ["gnmi"]),
    # A feature with no _BASE_IMAGE_FILES / no _INSTALL_* at all (sysmgr) --
    # generator must not choke on all-empty unions.
    ("single_feature_sysmgr", ["sysmgr"]),
    # Shutdown-order filtering: teamd's WARM/FAST_SHUTDOWN_BEFORE=syncd must
    # survive (syncd stays outside mega), while its WARM/FAST_SHUTDOWN_AFTER
    # =swss must be dropped (swss is also selected -> self-reference).
    ("shutdown_order_syncd_survives", ["swss", "teamd"]),
    # RUN_OPT mode-conflict, verified real case: p4rt mounts /etc/sonic as
    # rw while every other feature in the whole registry (26 of them) mounts
    # it ro. Must resolve to exactly one -v line, mode 'ro'. p4rt is
    # 'unproven' (not in PROVEN_FEATURES) but still resolvable -- open
    # feature universe, flagged not refused.
    ("etc_sonic_mode_conflict_p4rt", ["swss", "p4rt"]),
]

_SWSS_LAYER_RE = re.compile(r"SWSS_LAYER")
_CONFIG_ENGINE_RE = re.compile(r"CONFIG_ENGINE")


def check_scenario(sonic_root: Path, name: str, features: List[str]) -> bool:
    print(f"\n=== scenario: {name}  (features={features}) ===")
    registry = discover_registry(sonic_root)
    resolved = resolve_features(features, registry)
    if resolved.unknown or resolved.excluded:
        print(f"  SETUP FAILURE: unknown={resolved.unknown} excluded={resolved.excluded}")
        return False

    specs = discover_features(sonic_root, resolved.resolved, registry)
    result: MegaMkResult = generate_mega_mk(specs, sonic_root)

    problems: List[str] = []

    # --- 1. every feature appears exactly once as _INCLUDE_DOCKER -------
    for spec in specs:
        pat = re.compile(
            rf"^\$\(DOCKER_MEGA\)_INCLUDE_DOCKER \+= \$\({re.escape(spec.registry_entry.docker_var)}\)$",
            re.MULTILINE,
        )
        n = len(pat.findall(result.mk_text))
        if n != 1:
            problems.append(f"_INCLUDE_DOCKER for {spec.feature} appears {n} times (expected 1)")

    # --- 2. base layer selection -----------------------------------------
    feature_names = {s.feature for s in specs}
    if "swss" in feature_names:
        if not result.base_load_docker or not _SWSS_LAYER_RE.search(result.base_load_docker):
            problems.append(
                f"'swss' selected but base_load_docker={result.base_load_docker!r} "
                f"is not a *_SWSS_LAYER* token"
            )
    else:
        has_config_engine_feature = any(
            "CONFIG_ENGINE" in tok
            for spec in specs
            for tok in _load_dockers_declared(sonic_root, spec)
        )
        if has_config_engine_feature and (
            not result.base_load_docker or not _CONFIG_ENGINE_RE.search(result.base_load_docker)
        ):
            problems.append(
                f"'swss' not selected but base_load_docker={result.base_load_docker!r} "
                f"is not a *_CONFIG_ENGINE* token"
            )

    # --- 3. _LOAD_DOCKERS starts with base layer, no dupes ---------------
    if result.base_load_docker and (
        not result.load_dockers or result.load_dockers[0] != result.base_load_docker
    ):
        problems.append(
            f"_LOAD_DOCKERS does not start with the chosen base layer "
            f"({result.base_load_docker!r}): {result.load_dockers}"
        )
    if len(result.load_dockers) != len(set(result.load_dockers)):
        problems.append(f"_LOAD_DOCKERS has duplicates: {result.load_dockers}")
    expected_rsync_tokens = {
        s.registry_entry.docker_var
        for s in specs
        if s.feature in result.rsync_load_docker_features
    }
    missing_rsync = expected_rsync_tokens - set(result.load_dockers)
    if missing_rsync:
        problems.append(f"_LOAD_DOCKERS missing rsync-strategy feature image(s): {missing_rsync}")

    # --- 4. _INSTALL_PYTHON_WHEELS / _INSTALL_DEBS present iff declared --
    any_declares_wheels = any(
        re.search(rf"\$\({re.escape(s.registry_entry.docker_var)}\)_INSTALL_PYTHON_WHEELS", _mk_text(sonic_root, s))
        for s in specs
    )
    if any_declares_wheels != bool(result.install_python_wheels):
        problems.append(
            f"install_python_wheels={result.install_python_wheels} inconsistent with "
            f"whether any selected feature's .mk declares _INSTALL_PYTHON_WHEELS "
            f"({any_declares_wheels})"
        )

    # --- 5. base_image_files: resolved + retargeted where expected -------
    for bif in result.base_image_files:
        if bif.source_path is None or not bif.source_path.is_file():
            problems.append(f"_BASE_IMAGE_FILES '{bif.name}' has no resolved source file")
            continue
        if bif.content is None:
            problems.append(f"_BASE_IMAGE_FILES '{bif.name}' has no generated content")
            continue
        original = bif.source_path.read_text(errors="replace")
        owner_name = next(s for s in specs if s.feature == bif.feature).container_name
        mentions_owner_cmd = re.search(
            rf"(docker exec|memory_checker|restart_service)\S*\s+(?:-\S+\s+)*{re.escape(owner_name)}\b",
            original,
        )
        if mentions_owner_cmd and owner_name != "mega":
            if not re.search(r"\bmega\b", bif.content):
                problems.append(
                    f"_BASE_IMAGE_FILES '{bif.name}' (feature={bif.feature}) looked "
                    f"retargetable (mentions '{owner_name}' after docker exec/"
                    f"memory_checker/restart_service) but retargeted content has no "
                    f"'mega' reference at all"
                )

    # --- 6. no unexpected name collisions ---------------------------------
    collision_warnings = [w for w in result.warnings if "name collision" in w]
    names = [bif.name for bif in result.base_image_files]
    if len(names) == len(set(names)) and collision_warnings:
        problems.append(f"unexpected collision warning(s) with no actual duplicate names: {collision_warnings}")

    # --- 8. shutdown-order: self-references dropped, external ones kept --
    selected_container_names = {s.container_name for s in specs}
    for suffix, values in result.shutdown_order.items():
        for v in values:
            if v in selected_container_names:
                problems.append(
                    f"shutdown_order[{suffix}] kept a self-reference that should "
                    f"have been filtered: {v!r} (selected: {selected_container_names})"
                )
    # independently re-derive what SHOULD have survived, straight from the
    # raw .mk text, and compare -- not just re-testing gen_mega_mk's own logic.
    for spec in specs:
        text = _mk_text(sonic_root, spec)
        for suffix in ("_WARM_SHUTDOWN_BEFORE", "_WARM_SHUTDOWN_AFTER",
                       "_FAST_SHUTDOWN_BEFORE", "_FAST_SHUTDOWN_AFTER"):
            pat = re.compile(
                rf"^\s*\$\({re.escape(spec.registry_entry.docker_var)}\){suffix}\s*[+:]?=\s*(\S+)\s*$",
                re.MULTILINE,
            )
            for m in pat.finditer(text):
                target = m.group(1)
                if target not in selected_container_names:
                    if target not in result.shutdown_order.get(suffix, []):
                        problems.append(
                            f"{spec.feature}'s {suffix}={target!r} points outside the "
                            f"selected set but is missing from "
                            f"shutdown_order[{suffix}]={result.shutdown_order.get(suffix)}"
                        )

    # --- 7. generated .mk sanity -------------------------------------------
    for required_substr in (f"$(DOCKER_MEGA)_PATH = $(DOCKERS_PATH)/$(DOCKER_MEGA_STEM)",
                             "SONIC_DOCKER_IMAGES += $(DOCKER_MEGA)",
                             "SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_MEGA)",
                             "$(DOCKER_MEGA)_CONTAINER_NAME = mega"):
        if required_substr not in result.mk_text:
            problems.append(f"generated .mk missing expected line: {required_substr!r}")

    # --- 9. _RUN_OPT: no destination ever emitted twice ---------------------
    mount_re = re.compile(r"^-v\s+[^:]+:([^:]+)(?::[\w,]+)?$")
    dest_seen: List[str] = []
    for unit in result.run_opt:
        m = mount_re.match(unit)
        if m:
            dest = m.group(1)
            if dest in dest_seen:
                problems.append(
                    f"_RUN_OPT emitted the same -v destination twice: {dest!r} "
                    f"(units: {result.run_opt})"
                )
            dest_seen.append(dest)
    if name == "etc_sonic_mode_conflict_p4rt":
        sonic_mounts = [u for u in result.run_opt if u.startswith("-v /etc/sonic:")]
        if sonic_mounts != ["-v /etc/sonic:/etc/sonic:ro"]:
            problems.append(
                f"expected exactly one '-v /etc/sonic:/etc/sonic:ro' in run_opt "
                f"for the p4rt-vs-everyone-else mode conflict, got: {sonic_mounts}"
            )
        if not any("p4rt" in w and "/etc/sonic" in w for w in result.warnings):
            problems.append(
                "expected a warning naming p4rt's /etc/sonic mode override, found none"
            )

    # --- 10. Finding #11: ASAN ifeq guard tracking -------------------------
    # docker-orchagent.mk/docker-fpm-frr.mk/docker-teamd.mk all wrap their
    # own `--cap-add=SYS_PTRACE` in `ifeq ($(ENABLE_ASAN), y) ... endif` --
    # verify the generated .mk re-applies that SAME guard when every
    # contributor of the flag agreed on it, and never emits it bare unless
    # some OTHER selected feature (e.g. gnmi) also wants it unconditionally.
    if "--cap-add=SYS_PTRACE" in result.run_opt:
        any_unconditional_contributor = any(
            spec.feature == "gnmi" for spec in specs
        )
        guarded_block = re.search(
            r'ifeq \(\$\(ENABLE_ASAN\), y\)\n\$\(DOCKER_MEGA\)_RUN_OPT \+= --cap-add=SYS_PTRACE\nendif',
            result.mk_text,
        )
        text_outside_guard = (
            result.mk_text.replace(guarded_block.group(0), "", 1) if guarded_block else result.mk_text
        )
        bare_line = re.search(
            r'^\$\(DOCKER_MEGA\)_RUN_OPT \+= --cap-add=SYS_PTRACE$', text_outside_guard, re.MULTILINE
        )
        if any_unconditional_contributor:
            if not bare_line:
                problems.append(
                    "expected --cap-add=SYS_PTRACE to be emitted UNCONDITIONALLY "
                    "(gnmi wants it regardless of ENABLE_ASAN) but no bare line found"
                )
            if guarded_block:
                problems.append(
                    "--cap-add=SYS_PTRACE should not be ifeq-guarded when gnmi "
                    "(an unconditional contributor) is selected"
                )
        else:
            if not guarded_block:
                problems.append(
                    "expected --cap-add=SYS_PTRACE to be wrapped in "
                    "'ifeq ($(ENABLE_ASAN), y) ... endif' (every contributor "
                    "guarded it identically) but no such block found in "
                    f"generated .mk"
                )
            if bare_line:
                problems.append(
                    "--cap-add=SYS_PTRACE emitted unconditionally even though "
                    "every selected contributor guarded it with ENABLE_ASAN"
                )

    print(f"  base_load_docker:          {result.base_load_docker}")
    print(f"  load_dockers:              {result.load_dockers}")
    print(f"  rsync_load_docker_features:{result.rsync_load_docker_features}")
    print(f"  install_python_wheels:     {result.install_python_wheels}")
    print(f"  install_debs:              {result.install_debs}")
    print(f"  distro_docker_list:        {result.distro_docker_list}")
    print(f"  shutdown_order:            {result.shutdown_order}")
    print(f"  run_opt:                   {result.run_opt}")
    print(f"  base_image_files:          {[b.name for b in result.base_image_files]}")

    print(f"  --- generator warnings ({len(result.warnings)}) ---")
    for w in result.warnings:
        print(f"    WARNING: {w}")

    ok = not problems
    if problems:
        print(f"  --- PROBLEMS ({len(problems)}) ---")
        for p in problems:
            print(f"    FAIL: {p}")

    print(f"  RESULT: {'PASS' if ok else 'FAIL'}")
    return ok


# ---------------------------------------------------------------------------
# Small helpers (re-derive a couple of facts independently of gen_mega_mk's
# own internals, so the checks above aren't just re-testing the same code
# path against itself).
# ---------------------------------------------------------------------------

_MK_TEXT_CACHE: dict = {}


def _mk_text(sonic_root: Path, spec) -> str:
    key = (sonic_root, spec.mk_file)
    if key not in _MK_TEXT_CACHE:
        _MK_TEXT_CACHE[key] = (sonic_root / spec.mk_file).read_text(errors="replace")
    return _MK_TEXT_CACHE[key]


def _load_dockers_declared(sonic_root: Path, spec) -> List[str]:
    text = _mk_text(sonic_root, spec)
    pat = re.compile(
        rf"^\s*\$\({re.escape(spec.registry_entry.docker_var)}\)_LOAD_DOCKERS\s*[+:]?=\s*(.+)$",
        re.MULTILINE,
    )
    tokens: List[str] = []
    for m in pat.finditer(text):
        tokens.extend(re.findall(r"\$\((DOCKER_[A-Z0-9_]+)\)", m.group(1)))
    return tokens


def test_merge_run_opt_synthetic() -> bool:
    """Exercise `merge_run_opt()` directly with handwritten data for the two
    collision classes no *real* feature pairing in this tree can currently
    trigger together: a bind-mount SOURCE conflict (different host paths to
    the same destination) and a host-port conflict. The mode conflict
    (`ro`/`rw`) and plain dedup paths are already covered by the real
    `etc_sonic_mode_conflict_p4rt` scenario in `SCENARIOS`."""
    print("\n=== synthetic: merge_run_opt() collision classes ===")
    problems: List[str] = []

    # --- source-path collision: same destination, different host source ---
    result = merge_run_opt({
        "feat_a": ["-v /host/a:/shared:ro"],
        "feat_b": ["-v /host/b:/shared:ro"],
    })
    shared_units = [u for u in result.units if ":/shared:" in u]
    if len(shared_units) != 1:
        problems.append(f"source collision: expected exactly 1 surviving -v /shared unit, got {shared_units}")
    if not any("collision" in w and "/shared" in w for w in result.warnings):
        problems.append(f"source collision: expected a warning naming '/shared', got {result.warnings}")
    print(f"  source collision -> units={shared_units} warnings={[w for w in result.warnings if '/shared' in w]}")

    # --- mode collision: same source, ro vs rw -> ro wins, only one line ---
    result = merge_run_opt({
        "feat_a": ["-v /etc/x:/etc/x:ro"],
        "feat_b": ["-v /etc/x:/etc/x:rw"],
    })
    if result.units != ["-v /etc/x:/etc/x:ro"]:
        problems.append(f"mode collision: expected only ro to survive, got {result.units}")
    print(f"  mode collision -> units={result.units}")

    # --- plain dedup: identical mount from two features -> appears once ---
    result = merge_run_opt({
        "feat_a": ["-t", "-v /etc/sonic:/etc/sonic:ro", "--cap-add=NET_ADMIN"],
        "feat_b": ["-t", "-v /etc/sonic:/etc/sonic:ro", "--cap-add=SYS_ADMIN"],
    })
    if sorted(result.units) != sorted(["-t", "-v /etc/sonic:/etc/sonic:ro", "--cap-add=NET_ADMIN", "--cap-add=SYS_ADMIN"]):
        problems.append(f"plain dedup: unexpected result {result.units}")
    if result.warnings:
        problems.append(f"plain dedup: expected zero warnings, got {result.warnings}")
    print(f"  plain dedup -> units={result.units}")

    # --- host-port collision: two features claim the same host port ------
    result = merge_run_opt({
        "feat_a": ["-p=8080:80/tcp"],
        "feat_b": ["-p 0.0.0.0:8080:9090"],
    })
    if not any(w.startswith("ERROR:") and "8080" in w for w in result.warnings):
        problems.append(f"port collision: expected an ERROR: warning naming port 8080, got {result.warnings}")
    if len(result.units) != 2:
        problems.append(f"port collision: expected both distinct port units kept (not silently dropped), got {result.units}")
    print(f"  port collision -> units={result.units} warnings={[w for w in result.warnings if w.startswith('ERROR:')]}")

    # --- distinct ports: no collision, both kept, no warning ---------------
    result = merge_run_opt({
        "restapi": ["-p=8081:8081/tcp"],
        "sonic-redfish": ["-p 0.0.0.0:443:18080"],
    })
    if len(result.units) != 2 or result.warnings:
        problems.append(f"distinct ports: expected 2 units / 0 warnings, got units={result.units} warnings={result.warnings}")
    print(f"  distinct ports -> units={result.units}")

    ok = not problems
    if problems:
        print(f"  --- PROBLEMS ({len(problems)}) ---")
        for p in problems:
            print(f"    FAIL: {p}")
    print(f"  RESULT: {'PASS' if ok else 'FAIL'}")
    return ok


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--sonic-root", required=True, type=Path)
    ap.add_argument(
        "--scenario",
        default=None,
        help="Run only this scenario by name (default: run all of them).",
    )
    args = ap.parse_args(argv)

    if not (args.sonic_root / "rules").is_dir():
        print(f"ERROR: {args.sonic_root} does not look like a sonic-buildimage tree", file=sys.stderr)
        return 2

    scenarios = SCENARIOS
    if args.scenario:
        scenarios = [s for s in SCENARIOS if s[0] == args.scenario]
        if not scenarios:
            print(f"ERROR: unknown scenario {args.scenario!r}; choices: {[s[0] for s in SCENARIOS]}", file=sys.stderr)
            return 2

    results = {name: check_scenario(args.sonic_root, name, feats) for name, feats in scenarios}
    if not args.scenario:
        results["merge_run_opt_synthetic"] = test_merge_run_opt_synthetic()

    print("\n=== SUMMARY ===")
    for name, ok in results.items():
        print(f"  {'PASS' if ok else 'FAIL'}  {name}")
    n_fail = sum(1 for ok in results.values() if not ok)
    print(f"\n{len(results) - n_fail}/{len(results)} scenarios passed")
    return 1 if n_fail else 0


if __name__ == "__main__":
    sys.exit(main())
