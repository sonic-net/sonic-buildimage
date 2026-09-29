#!/usr/bin/env python3
"""Validation harness for `mega_gen.supervisord_merge` across several
feature-group scenarios.

This is a *generator-correctness* check (does the merge engine produce a
structurally sound, Jinja2-valid, supervisord-valid config for a given
feature subset?), not a hardware measurement -- it belongs in
`optimizations/mega-generic/` next to the module it exercises, and is meant
to be re-run (per the evaluation-methodology rule: "write a script, reuse
it") every time `supervisord_merge.py` changes, or a new feature-group
combination needs checking before a real build.

Two layers of checks, per scenario:

1. **Structural** (always runs, no rendering needed): merged text is valid
   Jinja2 syntax; `{% if %}/{% endif %}` and `{% for %}/{% endfor %}` are
   balanced; each shared section (`[supervisord]`,
   `[eventlistener:dependent-startup]`,
   `[eventlistener:supervisor-proc-exit-listener]`, `[program:rsyslogd]`,
   `[program:start]`) appears at most once; static `[program:X]` names are
   unique across the whole file; every `[group:X]` only lists programs that
   actually exist; priority bands are assigned with the documented 20-wide
   spacing.
2. **Best-effort full render** (via a real `jinja2.Environment`, with a
   permissive `Undefined` and a representative stub context covering
   `DEVICE_METADATA`/`INSTANCES`/`VLAN_INTERFACE`/etc., plus a `radv`-active
   second pass): if it renders, the *rendered* text is parsed with
   `configparser` (which is stricter about real supervisord-INI validity
   than "valid Jinja2") and, for every program with a known feature owner
   (from the merge's own `[group:X]` listings), its resolved `priority=`
   is checked to fall inside that feature's assigned band.

Every `MergeResult.warning` is also classified INFO / ATTENTION / CRITICAL
(see `classify_warning`) so a human doesn't have to re-derive, on every
run, which warnings are expected (e.g. the one-shot-terminal heuristic)
versus which indicate something is actually broken (e.g. a whole feature
silently dropped).

Usage:
    python3 test_supervisord_merge.py --sonic-root /path/to/sonic-buildimage
    python3 test_supervisord_merge.py --sonic-root ... --scenario mixed_proven_unproven
"""

from __future__ import annotations

import argparse
import configparser
import re
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

import jinja2

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from mega_gen.auto_registry import discover_registry, resolve_features
from mega_gen.discovery import discover_features
from mega_gen.supervisord_merge import PRIORITY_BAND_WIDTH, MergeResult, merge_supervisord

# ---------------------------------------------------------------------------
# Scenarios: (name, feature list). Chosen to exercise distinct code paths:
# ---------------------------------------------------------------------------

SCENARIOS: List[Tuple[str, List[str]]] = [
    # The plan's actual default set: full chain, 6/10 collapse into
    # [program:start], 4/10 use the templated-common.j2 path.
    ("full_default_10", ["database", "swss", "bgp", "teamd", "lldp", "snmp", "gnmi", "radv", "eventd", "sysmgr"]),
    # No feature here defines [program:start] at all -> shared
    # [program:start] must be entirely absent from the output.
    ("core_only_no_starters", ["database", "swss", "bgp"]),
    # Every selected feature is itself a starter -> nobody should get an
    # entry-edge rewrite (idx>0 and not is_starter is never true).
    ("starters_only_no_core", ["teamd", "lldp", "snmp", "gnmi", "radv", "eventd"]),
    # Smallest possible input: one starter feature, alone (idx==0).
    ("single_feature_gnmi", ["gnmi"]),
    # The 4 "true templated" features with no database/rsyslogd-anchor
    # feature before swss -> swss itself is idx==0.
    ("templated_quartet_no_database", ["swss", "bgp", "teamd", "lldp"]),
    # sflow/nat: open/unproven features whose common.j2 is dead weight for
    # their own standalone container (only docker-sonic-vs uses it) --
    # regression scenario for the is_templated-detection fix. pmon: proven
    # but heavily Jinja-guarded (every program individually wrapped).
    ("mixed_proven_unproven", ["sflow", "nat", "pmon"]),
    # All 11 proven features together (default 10 + pmon at the tail).
    ("all_11_proven", ["database", "swss", "bgp", "teamd", "lldp", "snmp", "gnmi", "radv", "eventd", "sysmgr", "pmon"]),
]

# ---------------------------------------------------------------------------
# Warning severity classifier.
# ---------------------------------------------------------------------------

_INFO_PATTERNS = [
    r"terminal program '.*' inferred as '.*' via the one-shot-script heuristic",
    r"skipped \[group:.*\] wrapper -- member program names are Jinja-dynamic",
    r"not in the canonical priority-band order .* appended after it",
    r"has a supervisord\.conf\.common\.j2 but its own main supervisord\.conf never",
    r"all contributed programs are conditionally Jinja-guarded",
    r"no critical_processes\(\.j2\) found -- omitted",
]
_CRITICAL_PATTERNS = [
    r"no supervisord\.conf\(\.j2\) found -- skipped entirely",
    r"no \[program:rsyslogd\] found in any selected feature$",
    r"no \[eventlistener:dependent-startup\] found in any selected feature",
]


def classify_warning(w: str) -> str:
    for pat in _CRITICAL_PATTERNS:
        if re.search(pat, w):
            return "CRITICAL"
    for pat in _INFO_PATTERNS:
        if re.search(pat, w):
            return "INFO"
    return "ATTENTION"


# ---------------------------------------------------------------------------
# Layer 1: structural checks (no rendering required).
# ---------------------------------------------------------------------------

_SHARED_SECTION_HEADERS = [
    "[supervisord]",
    "[eventlistener:dependent-startup]",
    "[eventlistener:supervisor-proc-exit-listener]",
    "[program:rsyslogd]",
    "[program:start]",
]


def structural_checks(result: MergeResult) -> List[str]:
    problems: List[str] = []
    text = result.supervisord_conf_text

    env = jinja2.Environment()
    try:
        env.parse(text)
    except jinja2.exceptions.TemplateSyntaxError as e:
        problems.append(f"INVALID JINJA2 SYNTAX: {e}")

    for open_pat, close_pat, label in (
        (r"\{%-?\s*if\b", r"\{%-?\s*endif\b", "if/endif"),
        (r"\{%-?\s*for\b", r"\{%-?\s*endfor\b", "for/endfor"),
    ):
        o = len(re.findall(open_pat, text))
        c = len(re.findall(close_pat, text))
        if o != c:
            problems.append(f"UNBALANCED {label}: {o} opens vs {c} closes")

    for header in _SHARED_SECTION_HEADERS:
        count = len(re.findall(r"^" + re.escape(header) + r"\s*$", text, re.MULTILINE))
        if count > 1:
            problems.append(f"DUPLICATE SHARED SECTION {header}: found {count} times")

    names = re.findall(r"^\[program:([^\]]+)\]\s*$", text, re.MULTILINE)
    static_names = [n for n in names if "{{" not in n]
    dupes = sorted({n for n in static_names if static_names.count(n) > 1})
    if dupes:
        problems.append(f"DUPLICATE [program:] NAMES ACROSS FEATURES: {dupes}")

    all_static = set(static_names)
    group_re = re.compile(r"^\[group:([^\]]+)\]\s*\nprograms=(.*)$", re.MULTILINE)
    for gm in group_re.finditer(text):
        gname, members_raw = gm.group(1), gm.group(2)
        members = [m.strip() for m in members_raw.split(",") if m.strip()]
        missing = [m for m in members if m not in all_static]
        if missing:
            problems.append(f"[group:{gname}] references undefined program(s): {missing}")

    bands = result.priority_bands
    by_base = sorted(bands.items(), key=lambda kv: kv[1])
    for i in range(1, len(by_base)):
        (prev_feat, prev_base), (feat, base) = by_base[i - 1], by_base[i]
        if base - prev_base != PRIORITY_BAND_WIDTH:
            problems.append(
                f"PRIORITY BAND SPACING: {prev_feat}({prev_base}) -> {feat}({base}) "
                f"delta={base - prev_base}, expected {PRIORITY_BAND_WIDTH}"
            )
    if len(bands) != len(result.feature_order):
        problems.append(
            f"priority_bands has {len(bands)} entries but feature_order has "
            f"{len(result.feature_order)}"
        )

    # [program:start] presence must match whether any feature is a starter.
    has_start_section = bool(re.search(r"^\[program:start\]\s*$", text, re.MULTILINE))
    if bool(result.starter_features) != has_start_section:
        problems.append(
            f"[program:start] section presence ({has_start_section}) doesn't match "
            f"starter_features ({result.starter_features})"
        )

    return problems


# ---------------------------------------------------------------------------
# Layer 2: best-effort full Jinja2 render + configparser/priority-band check.
# ---------------------------------------------------------------------------


# NOTE: variables a template checks with `is defined` (e.g. lldp's
# `namespace_id is defined and namespace_id|length`) must be *omitted*
# here, not set to `None` -- `is defined` only checks context membership,
# so `namespace_id=None` would make that check pass and then crash on
# `None|length`. Leaving a key out entirely makes it a real
# `jinja2.Undefined` (falsy, `is defined` is False, short-circuits safely).
_RENDER_CONTEXTS = {
    "default": dict(
        DEVICE_METADATA={"localhost": {"type": "ToRRouter", "switch_type": "switch"}},
        FEATURE={},
        INSTANCES={
            "redis": {
                "hostname": "127.0.0.1",
                "port": 6379,
                "unix_socket_path": "/var/run/redis/redis.sock",
                "is_protected_mode": True,
            }
        },
        VLAN_INTERFACE=None,
        WARM_RESTART={},
        SYSTEM_DEFAULTS={},
        ENABLE_ASAN="n",
        constants={},
    ),
    "radv_active": dict(
        DEVICE_METADATA={"localhost": {"type": "ToRRouter", "switch_type": "switch"}},
        FEATURE={},
        INSTANCES={},
        VLAN_INTERFACE={"Vlan1000|2001:db8::1/64": {}},
        WARM_RESTART={},
        SYSTEM_DEFAULTS={},
        ENABLE_ASAN="n",
        constants={},
    ),
}


def _stub_pfx_filter(vlan_interface):
    # VLAN_INTERFACE|pfx_filter -> iterable of (name, prefix) pairs. Real
    # filter parses SONiC's "Vlan1000|2001:db8::1/64" ConfigDB-style keys;
    # this stub just does the same for our synthetic single-entry dict.
    out = []
    for key in (vlan_interface or {}):
        if "|" in key:
            name, prefix = key.split("|", 1)
            out.append((name, prefix))
    return out


def _stub_ipv6_filter(prefix):
    return ":" in str(prefix)


def try_render(text: str, context_name: str) -> Tuple[Optional[str], Optional[Exception]]:
    env = jinja2.Environment(undefined=jinja2.Undefined)
    env.filters["pfx_filter"] = _stub_pfx_filter
    env.filters["ipv6"] = _stub_ipv6_filter
    try:
        tmpl = env.from_string(text)
        return tmpl.render(**_RENDER_CONTEXTS[context_name]), None
    except Exception as e:  # noqa: BLE001 - best-effort, report and move on
        return None, e


def render_checks(result: MergeResult) -> Tuple[List[str], List[str]]:
    """Returns (problems, notes). `notes` records render attempts that
    failed -- not necessarily a bug in the merge (custom sonic-cfggen
    Jinja filters/globals this harness doesn't stub could be the cause),
    but worth surfacing rather than silently skipping."""
    problems: List[str] = []
    notes: List[str] = []

    # feature -> [program names], parsed straight from this result's own
    # [group:X] output (skips dynamic-named features like database, which
    # are already flagged separately).
    group_re = re.compile(r"^\[group:([^\]]+)\]\s*\nprograms=(.*)$", re.MULTILINE)
    members_by_feature: Dict[str, List[str]] = {}
    for gm in group_re.finditer(result.supervisord_conf_text):
        members_by_feature[gm.group(1)] = [
            m.strip() for m in gm.group(2).split(",") if m.strip()
        ]

    rendered_any = False
    for context_name in _RENDER_CONTEXTS:
        rendered, err = try_render(result.supervisord_conf_text, context_name)
        if err is not None:
            notes.append(f"render[{context_name}] failed: {err!r} (not necessarily a bug -- "
                         f"see try_render's stubbed context/filters)")
            continue
        rendered_any = True

        cp = configparser.ConfigParser(strict=True, interpolation=None)
        try:
            cp.read_string(rendered, source=f"<rendered:{context_name}>")
        except configparser.Error as e:
            problems.append(f"render[{context_name}]: rendered output is invalid INI: {e}")
            continue

        for feat, members in members_by_feature.items():
            base = result.priority_bands.get(feat)
            if base is None:
                continue
            for member in members:
                section = f"program:{member}"
                if not cp.has_section(section):
                    continue  # conditionally rendered out in this context; fine
                raw_priority = cp.get(section, "priority", fallback=None)
                if raw_priority is None:
                    continue
                try:
                    value = int(raw_priority.strip())
                except ValueError:
                    problems.append(
                        f"render[{context_name}]: [program:{member}] (feature {feat}) has "
                        f"non-integer priority after render: {raw_priority!r}"
                    )
                    continue
                if not (base <= value < base + PRIORITY_BAND_WIDTH):
                    problems.append(
                        f"render[{context_name}]: [program:{member}] (feature {feat}) "
                        f"priority={value} outside its band [{base}, {base + PRIORITY_BAND_WIDTH})"
                    )

    if not rendered_any:
        notes.append("no context rendered successfully -- Layer 2 checks skipped entirely")
    return problems, notes


# ---------------------------------------------------------------------------
# Runner.
# ---------------------------------------------------------------------------


def run_scenario(sonic_root: Path, name: str, features: List[str]) -> bool:
    print(f"\n=== scenario: {name}  (features={features}) ===")
    registry = discover_registry(sonic_root)
    resolved = resolve_features(features, registry)
    if resolved.unknown or resolved.excluded:
        print(f"  SETUP FAILURE: unknown={resolved.unknown} excluded={resolved.excluded}")
        return False
    if resolved.unproven:
        print(f"  (note: unproven features requested, as intended by this scenario: {resolved.unproven})")

    specs = discover_features(sonic_root, resolved.resolved, registry)
    result = merge_supervisord(specs)

    ok = True

    struct_problems = structural_checks(result)
    render_problems, render_notes = render_checks(result)

    print(f"  feature_order:    {result.feature_order}")
    print(f"  priority_bands:   {result.priority_bands}")
    print(f"  starter_features: {result.starter_features}")
    print(f"  grouped_features: {result.grouped_features}")

    print(f"  --- warnings ({len(result.warnings)}) ---")
    by_sev: Dict[str, int] = {"CRITICAL": 0, "ATTENTION": 0, "INFO": 0}
    for w in result.warnings:
        sev = classify_warning(w)
        by_sev[sev] += 1
        print(f"    [{sev:9s}] {w}")
    if by_sev["CRITICAL"]:
        ok = False

    if render_notes:
        print("  --- render notes (informational) ---")
        for n in render_notes:
            print(f"    {n}")

    all_problems = struct_problems + render_problems
    if all_problems:
        ok = False
        print(f"  --- STRUCTURAL/RENDER PROBLEMS ({len(all_problems)}) ---")
        for p in all_problems:
            print(f"    FAIL: {p}")

    print(
        f"  RESULT: {'PASS' if ok else 'FAIL'}  "
        f"(warnings: {by_sev['CRITICAL']} critical / {by_sev['ATTENTION']} attention / "
        f"{by_sev['INFO']} info; problems: {len(all_problems)})"
    )
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

    results = {name: run_scenario(args.sonic_root, name, feats) for name, feats in scenarios}

    print("\n=== SUMMARY ===")
    for name, ok in results.items():
        print(f"  {'PASS' if ok else 'FAIL'}  {name}")
    n_fail = sum(1 for ok in results.values() if not ok)
    print(f"\n{len(results) - n_fail}/{len(results)} scenarios passed")
    return 1 if n_fail else 0


if __name__ == "__main__":
    sys.exit(main())
