#!/usr/bin/env python3
"""Validation harness for `mega_gen.patch_templates`'s
`sonic_debian_extension.j2` patch (Finding #5).

Renders the patched `installer_services` registration loop with Jinja2 for
a realistic `installer_services` string (a mix of folded features' own
services, syncd, mega, pmon, and several NON-folded services that ship in
every stock build regardless of what's folded into mega) and checks:

  1. The patched template still parses as valid Jinja2.
  2. Every folded feature's OWN `.service`/`@.service`/`-chassis.service`
     unit is dropped from registration (it gets an alias symlink to
     mega.service instead, verified separately).
  3. Every service that is NOT one of the folded features (dhcp_relay,
     telemetry, mux, nat, gbsyncd, ...) is still registered -- this is the
     regression the original *allow*-list version of this patch caused:
     it kept only syncd/mega/(pmon if not folded) and silently dropped
     every other non-folded service.
  4. `syncd.service`/`syncd@.service` (never folded) and `mega.service`
     itself are always registered.
  5. `mega.service.j2` is generated and `docker-mega.mk` adds
     `$(DOCKER_MEGA)` to `SONIC_INSTALL_DOCKER_IMAGES`, so `mega.service`
     flows into `installer_services` via the same generic per-docker
     mechanism as any other container (slave.mk), confirming the second
     half of Finding #5 ("ensure mega.service is added to
     installer_services") needs no separate patch.

Usage:
    python3 test_sonic_debian_extension.py --sonic-root /path/to/sonic-buildimage
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
]

# Services that ship in every stock build regardless of what's folded into
# mega -- these must NEVER be dropped by the mega-gen service filter.
_NON_FOLDED_SERVICES = [
    "dhcp_relay.service",
    "telemetry.service",
    "mux.service",
    "nat.service",
    "gbsyncd.service",
    "pmon.service",
]

_LOOP_RE = re.compile(
    r"\{%- set mega_drop_services.*?\{% endfor %\}\n", re.DOTALL
)


def _extract_loop(text: str) -> str:
    m = _LOOP_RE.search(text)
    return m.group(0) if m else ""


def _registered_services(rendered_loop_output: str) -> List[str]:
    return re.findall(r'echo\s+""([^"]+)""\s+\|\s+sudo tee', rendered_loop_output)


def check_scenario(sonic_root: Path, name: str, features: List[str]) -> bool:
    print(f"\n=== scenario: {name}  (features={features}) ===")
    registry = discover_registry(sonic_root)
    resolved = resolve_features(features, registry)
    if resolved.unknown or resolved.excluded:
        print(f"  SETUP FAILURE: unknown={resolved.unknown} excluded={resolved.excluded}")
        return False

    specs = discover_features(sonic_root, resolved.resolved, registry)
    folded = sorted(s.container_name for s in specs)

    patches = generate_patches(specs, sonic_root)
    ext_path = "files/build_templates/sonic_debian_extension.j2"
    if ext_path not in patches.file_patches:
        print(f"  SETUP FAILURE: {ext_path} not patched")
        return False
    patched_text = patches.file_patches[ext_path]

    problems: List[str] = []

    # --- 1. still parses ---
    try:
        jinja2.Environment().parse(patched_text)
    except jinja2.TemplateSyntaxError as exc:
        problems.append(f"patched template has a Jinja2 syntax error: {exc}")
        print("  FAIL:")
        for p in problems:
            print(f"    - {p}")
        return False

    loop_snippet = _extract_loop(patched_text)
    if not loop_snippet:
        problems.append("could not locate the mega_drop_services loop in the patched template")
        print("  FAIL:")
        for p in problems:
            print(f"    - {p}")
        return False

    # Build a realistic installer_services string: folded features' own
    # services + syncd + mega + the always-present non-folded services.
    all_services = (
        ["syncd.service", "syncd@.service", "mega.service"]
        + [f"{f}.service" for f in folded]
        + list(_NON_FOLDED_SERVICES)
    )
    installer_services = " ".join(f'"{s}"' for s in all_services)

    env = jinja2.Environment(loader=jinja2.BaseLoader())
    tmpl = env.from_string(loop_snippet)
    out = tmpl.render(
        installer_services=installer_services,
        GENERATED_SERVICE_FILE="/tmp/x",
        FILESYSTEM_ROOT_USR_LIB_SYSTEMD_SYSTEM="/x",
    )
    registered = set(_registered_services(out))

    # --- 2. folded features' own services dropped ---
    for feat in folded:
        if f"{feat}.service" in registered:
            problems.append(f"folded feature '{feat}.service' was NOT dropped (should be aliased to mega instead)")

    # --- 3. non-folded services still registered ---
    for svc in _NON_FOLDED_SERVICES:
        if svc.split(".")[0] in folded:
            continue  # this scenario happens to fold this one in; skip
        if svc not in registered:
            problems.append(f"non-folded service '{svc}' was dropped -- regression to allow-list behavior")

    # --- 4. syncd/mega always registered ---
    for svc in ("syncd.service", "syncd@.service", "mega.service"):
        if svc not in registered:
            problems.append(f"'{svc}' was dropped but must always be registered")

    if problems:
        print("  FAIL:")
        for p in problems:
            print(f"    - {p}")
        return False

    print(f"  OK -- folded={folded} dropped correctly, {len(_NON_FOLDED_SERVICES)} non-folded services preserved")
    return True


def check_mega_service_wiring(sonic_root: Path) -> bool:
    print("\n=== check: mega.service.j2 generated + docker-mega.mk installs it ===")
    registry = discover_registry(sonic_root)
    resolved = resolve_features(
        ["database", "swss", "bgp", "teamd", "lldp", "snmp", "gnmi", "radv", "eventd", "sysmgr"],
        registry,
    )
    specs = discover_features(sonic_root, resolved.resolved, registry)
    patches = generate_patches(specs, sonic_root)

    problems: List[str] = []
    if "files/build_templates/mega.service.j2" not in patches.file_patches:
        problems.append("files/build_templates/mega.service.j2 was not generated")

    mega_mk = generate_mega_mk(specs, sonic_root)
    mk_text = mega_mk.mk_text
    if "SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_MEGA)" not in mk_text:
        problems.append(
            "docker-mega.mk does not add $(DOCKER_MEGA) to SONIC_INSTALL_DOCKER_IMAGES "
            "-- mega.service would never flow into installer_services"
        )

    if problems:
        print("  FAIL:")
        for p in problems:
            print(f"    - {p}")
        return False

    print("  OK -- mega.service.j2 generated and installed via SONIC_INSTALL_DOCKER_IMAGES")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sonic-root", required=True)
    parser.add_argument("--scenario", default=None)
    args = parser.parse_args()

    if jinja2 is None:
        print("SKIP: jinja2 is not installed -- cannot render templates for verification")
        return 0

    sonic_root = Path(args.sonic_root).resolve()

    scenarios = SCENARIOS
    if args.scenario:
        scenarios = [s for s in SCENARIOS if s[0] == args.scenario]
        if not scenarios:
            print(f"Unknown scenario: {args.scenario}")
            return 2

    all_ok = True
    for name, features in scenarios:
        if not check_scenario(sonic_root, name, features):
            all_ok = False

    if not check_mega_service_wiring(sonic_root):
        all_ok = False

    print(f"\n=== {'all' if all_ok else 'NOT all'} scenarios passed ===")
    return 0 if all_ok else 1


if __name__ == "__main__":
    sys.exit(main())
