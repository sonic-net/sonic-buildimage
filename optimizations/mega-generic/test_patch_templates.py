#!/usr/bin/env python3
"""Validation harness for `mega_gen.patch_templates`, specifically the
`docker_image_ctl.j2` composition (`_patch_shared_ctl`).

Static reading alone can't catch a real docker-create-time error, so this
renders the *patched* shared template with Jinja2 for several
`docker_container_name` values -- `mega` (with the real, generated
`_RUN_OPT` union substituted for `docker_image_run_opt`, exactly as the real
build's `slave.mk` loop does) plus a handful of untouched, real container
names (`swss`, `database`, `bgp`, `teamd`, `pmon`) as a no-regression check
-- and inspects the resulting `docker create ...` command text.

Checks, per scenario:
  1. The patched template still parses as valid Jinja2 (no syntax error).
  2. Rendering for `docker_container_name="mega"` (both `enable_asan` "n"
     and "y") succeeds and produces a `docker create` command with **no
     duplicate `-v`/`--volume` mount destination** (the "Duplicate mount
     point" class of docker error the static review flagged).
  3. The rendered mega command contains the `/usr/share/sonic/hwsku` mount
     iff swss is folded in (orchagent needs it) -- and, when present, is not
     malformed by an empty `$HWSKU`/`$HWSKU_MOUNT_MODE` (i.e. mega must NOT
     take the plain "database" empty-HWSKU branch).
  4. `-e ASIC_VENDOR=...` present iff swss is folded in.
  5. The bgp frr mount, correctly spelled `/etc/sonic/frr/$DEV:/etc/frr:rw`
     (no missing slash), present iff bgp is folded in.
  6. `$DB_OPT` present for mega (chassis/multi-ASIC redis wiring retained).
  7. The mega `stop()` branch uses a 60 second timeout iff teamd is folded
     in, or swss is folded in with `enable_asan == "y"`.
  8. Rendering for real, untouched container names (`swss`, `database`,
     `bgp`, `teamd`, `pmon`) with the SAME source template produces byte-
     identical `docker create` blocks to rendering the ORIGINAL (unpatched)
     template -- i.e. patching mega in doesn't change any other container's
     generated script (no regression).

Usage:
    python3 test_patch_templates.py --sonic-root /path/to/sonic-buildimage
    python3 test_patch_templates.py --sonic-root ... --scenario full_default_10
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
from mega_gen.gen_mega_mk import generate_mega_mk
from mega_gen.patch_templates import generate_patches

SCENARIOS: List[Tuple[str, List[str]]] = [
    ("full_default_10", ["database", "swss", "bgp", "teamd", "lldp", "snmp", "gnmi", "radv", "eventd", "sysmgr"]),
    ("swss_bgp_teamd_only", ["swss", "bgp", "teamd"]),
    ("db_lldp_snmp_gnmi", ["database", "lldp", "snmp", "gnmi"]),
    ("with_pmon", ["database", "swss", "bgp", "teamd", "pmon"]),
    ("bgp_no_swss", ["database", "bgp"]),
    ("no_bgp_no_teamd", ["database", "swss", "lldp", "snmp"]),
]

# Real, untouched container names to render for regression checking. Any of
# these that a scenario doesn't happen to fold in are still safe/meaningful
# to render (the shared template is rendered per-real-container regardless
# of what mega folds elsewhere).
_REGRESSION_CONTAINER_NAMES = ["swss", "database", "bgp", "teamd", "pmon"]

_BASE_CTX = {
    "docker_image_id": "abc123",
    "docker_image_name": "docker-mega",
    "docker_image_reference": "docker-mega:latest",
    "docker_image_run_opt": "",
    "install_debug_image": "n",
    "mount_default_tmpfs": "y",
    "sonic_asic_platform": "broadcom",
    "enable_asan": "n",
}

_MOUNT_RE = re.compile(r"(?:^|\s)(?:-v|--volume)\s+(?P<src>[^:\s]+):(?P<dst>[^:\s]+)(?::(?P<mode>[\w,]+))?")
_DOCKER_CREATE_BLOCK_RE = re.compile(r"\n    docker create .*?\n\s*\|\| \{", re.DOTALL)


def _docker_create_block(rendered: str) -> str:
    """The literal `docker create ... \\` argument list -- i.e. only the
    flags that are ALWAYS simultaneously present in one `docker create`
    invocation. Deliberately excludes the rest of the script: many `-v`
    mentions elsewhere are inside mutually exclusive **shell** `if`/`else`
    branches building up a variable (`$DB_OPT`, `$REDIS_MNT`) that's
    referenced exactly once here -- e.g. real "database"'s own single-ASIC
    vs. multi-ASIC branches both append to `$DB_OPT`, but only one of those
    branches ever runs for a given `$DEV`, so scanning the whole script text
    for duplicate destinations would false-positive on that pre-existing,
    correct pattern (confirmed unrelated to mega: it reproduces identically
    when rendering the ORIGINAL, unpatched template for `docker_container_name
    == "database"`)."""
    m = _DOCKER_CREATE_BLOCK_RE.search(rendered)
    return m.group(0) if m else ""


def _render(text: str, **overrides) -> str:
    env = jinja2.Environment(undefined=jinja2.Undefined)
    ctx = dict(_BASE_CTX)
    ctx.update(overrides)
    return env.from_string(text).render(**ctx)


def _mount_destinations(rendered: str) -> List[str]:
    return [m.group("dst") for m in _MOUNT_RE.finditer(rendered)]


def check_scenario(sonic_root: Path, name: str, features: List[str]) -> bool:
    print(f"\n=== scenario: {name}  (features={features}) ===")
    registry = discover_registry(sonic_root)
    resolved = resolve_features(features, registry)
    if resolved.unknown or resolved.excluded:
        print(f"  SETUP FAILURE: unknown={resolved.unknown} excluded={resolved.excluded}")
        return False

    specs = discover_features(sonic_root, resolved.resolved, registry)
    selected = {s.container_name for s in specs}
    has_swss = "swss" in selected
    has_bgp = "bgp" in selected
    has_teamd = "teamd" in selected

    patches = generate_patches(specs, sonic_root)
    ctl_path = "files/build_templates/docker_image_ctl.j2"
    if ctl_path not in patches.file_patches:
        print(f"  SETUP FAILURE: {ctl_path} not patched")
        return False
    patched_text = patches.file_patches[ctl_path]
    original_text = (sonic_root / ctl_path).read_text(errors="replace")

    problems: List[str] = []

    # --- 1. patched template still parses ---
    try:
        jinja2.Environment().parse(patched_text)
    except jinja2.TemplateSyntaxError as exc:
        problems.append(f"patched template has a Jinja2 syntax error: {exc}")
        print("  FAIL:")
        for p in problems:
            print(f"    - {p}")
        return False

    mega_mk = generate_mega_mk(specs, sonic_root)
    run_opt_text = " ".join(mega_mk.run_opt)

    for asan in ("n", "y"):
        rendered = _render(
            patched_text,
            docker_container_name="mega",
            docker_image_run_opt=run_opt_text,
            enable_asan=asan,
        )

        # --- 2. no duplicate -v destination ---
        create_block = _docker_create_block(rendered)
        dests = _mount_destinations(create_block)
        dupes = sorted({d for d in dests if dests.count(d) > 1})
        if dupes:
            problems.append(f"(enable_asan={asan}) duplicate mount destinations: {dupes}")

        # --- 3. hwsku mount present iff swss selected, and not malformed ---
        hwsku_present = "/usr/share/sonic/device/$PLATFORM/$HWSKU/$DEV:/usr/share/sonic/hwsku" in create_block
        if has_swss and not hwsku_present:
            problems.append(f"(enable_asan={asan}) swss selected but no hwsku mount in mega's docker create")
        if not has_swss and hwsku_present:
            problems.append(f"(enable_asan={asan}) hwsku mount present without swss selected")
        if hwsku_present and "/usr/share/sonic/device//$DEV" in rendered:
            problems.append(f"(enable_asan={asan}) hwsku mount malformed -- empty $PLATFORM/$HWSKU segment")

        # --- 4. ASIC_VENDOR iff swss ---
        asic_vendor_present = "ASIC_VENDOR=broadcom" in rendered
        if has_swss != asic_vendor_present:
            problems.append(
                f"(enable_asan={asan}) ASIC_VENDOR presence ({asic_vendor_present}) != swss selected ({has_swss})"
            )

        # --- 5. bgp frr mount, correctly spelled, iff bgp ---
        frr_ok = "/etc/sonic/frr/$DEV:/etc/frr:rw" in rendered
        frr_typo = "/etc/sonic/frr$DEV:/etc/frr:rw" in rendered
        if frr_typo:
            problems.append(f"(enable_asan={asan}) bgp frr mount has the missing-slash typo")
        if has_bgp != frr_ok:
            problems.append(
                f"(enable_asan={asan}) bgp frr mount presence ({frr_ok}) != bgp selected ({has_bgp})"
            )

        # --- 6. $DB_OPT present ---
        if "$DB_OPT" not in rendered:
            problems.append(f"(enable_asan={asan}) $DB_OPT missing from mega's docker create")

        # --- 7. stop() 60s timeout iff teamd or (swss and asan==y) ---
        stop_fn_match = re.search(r"\nstop\(\) \{(.*?)\n\}\n", rendered, re.DOTALL)
        stop_body = stop_fn_match.group(1) if stop_fn_match else ""
        expect_60 = has_teamd or (has_swss and asan == "y")
        has_60 = "docker stop -t 60 $DOCKERNAME" in stop_body
        if expect_60 != has_60:
            problems.append(
                f"(enable_asan={asan}) mega stop() 60s-timeout presence ({has_60}) != expected ({expect_60})"
            )

    # --- 8. real container names unaffected (no regression) ---
    for real_name in _REGRESSION_CONTAINER_NAMES:
        for asan in ("n", "y"):
            before = _render(original_text, docker_container_name=real_name, enable_asan=asan)
            after = _render(patched_text, docker_container_name=real_name, enable_asan=asan)
            if before != after:
                problems.append(
                    f"patching changed rendering for docker_container_name={real_name!r} (enable_asan={asan})"
                )

    if problems:
        print("  FAIL:")
        for p in problems:
            print(f"    - {p}")
        return False

    print(f"  OK -- has_swss={has_swss} has_bgp={has_bgp} has_teamd={has_teamd}")
    return True


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--sonic-root", required=True, type=Path)
    ap.add_argument("--scenario", default=None)
    args = ap.parse_args()

    if jinja2 is None:
        print("ERROR: jinja2 is required for this harness", file=sys.stderr)
        return 2

    scenarios = SCENARIOS
    if args.scenario:
        scenarios = [s for s in SCENARIOS if s[0] == args.scenario]
        if not scenarios:
            print(f"ERROR: unknown scenario {args.scenario!r}", file=sys.stderr)
            return 2

    results = [check_scenario(args.sonic_root, name, features) for name, features in scenarios]
    passed = sum(results)
    total = len(results)
    print(f"\n=== {passed}/{total} scenarios passed ===")
    return 0 if passed == total else 1


if __name__ == "__main__":
    sys.exit(main())
