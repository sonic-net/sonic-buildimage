"""Generate the mega container's per-feature "preinit" scripts + the
`docker-mega-init.sh` orchestrator (plan section 7, "Preinit pattern for
init scripts").

Implements the `gen-dockerfile-init` to-do's second half: *"Implement
gen_docker_init.py: generate docker-mega-init.sh using the preinit pattern --
strip 'exec supervisord' from each service's init script, run them
sequentially to generate runtime configs, then exec supervisord."*

Every non-static selected feature (i.e. one whose own `Dockerfile.j2`
`ENTRYPOINT` is a wrapper init script, not `/usr/local/bin/supervisord`
directly -- `FeatureSpec.uses_supervisord_directly is False`) has its own
init script responsible for three things, verified directly against every
default feature's real init script in this tree (per the
`sonic-docs-lookup` rule):

  1. Rendering that feature's OWN runtime config file(s) from Jinja2
     templates via `sonic-cfggen -d -t ...` (device-metadata-driven, reads
     CONFIG_DB) -- e.g. teamd/snmp/lldp each render exactly their own
     `/etc/supervisor/conf.d/supervisord.conf`; `bgp` additionally renders
     FRR's `bgpd.conf`/`zebra.conf`/etc.; `radv` additionally renders
     `/etc/radvd.conf` + `/usr/bin/wait_for_link.sh`; `database` is the most
     involved (chassis/multi-instance redis config).
  2. A handful of directory-creation / permission-setting side effects
     (`mkdir -p`, `chmod +x`).
  3. `exec /usr/local/bin/supervisord` as the very last line -- this is
     what turns the init script into the container's actual PID 1.

For mega, step 3 must happen exactly ONCE, for the whole merged container
(started by `docker-mega-init.sh` itself, after every selected feature's
own step-1/2 work has run) -- so this module's job is: for every such
feature, take its own init script (or `.j2` template, in swss's case --
see below) **verbatim**, strip out every `exec /usr/local/bin/supervisord`
line (there can be more than one -- `database`'s own init script has an
early-return `chassisdb` branch with its own `exec`), and leave everything
else untouched. The result is written out as `dockers/docker-mega/
preinit-<feature>.sh(.j2)` (one per feature) plus one combined
`docker-mega-init.sh` orchestrator that runs them all in the same
canonical order `supervisord_merge.py` uses, then renders mega's own
*merged* `supervisord.conf.j2` (that module's own output -- not this
module's concern) and finally `exec`s the single shared supervisord.

**Deliberate deviation from the plan's illustrative section-6 rsync
snippet** (documented here since the plan file itself must not be edited,
per the assignment): that snippet's `install -m644 /svc/etc/supervisor/
conf.d/supervisord.conf /opt/sonic/core-services/teamd/supervisord.conf`
assumes a *pre-rendered* `/etc/supervisor/conf.d/supervisord.conf` already
exists in teamd's *built* image filesystem. Verified against every
dynamic-init feature's own init script in this tree: that file is **not**
pre-rendered at build time at all for teamd/snmp/radv/lldp/bgp/database --
it is generated at *container startup* by their own init script reading
live CONFIG_DB (`sonic-cfggen -d ...`), so the built image never contains
it pre-runtime and that `install` would simply fail (source doesn't
exist). This module strips the dead `sonic-cfggen -t` render arguments for
supervisord.conf.j2 and critical_processes.j2 from each feature's own
init script (see `_strip_dead_renders` below), rather than retargeting
their output paths: in the mega container these templates are either
replaced by the merged version or not installed at all, and the intended
outputs are handled by mega's own orchestrator -- rendering them into
per-feature isolation dirs would produce wrong content (merged template
where a feature-specific one was expected) or crash (missing template
file entirely).

**swss is a template, not a literal script.** `docker-orchagent`'s
`ENTRYPOINT` is `/usr/bin/docker-init.sh`, but that file is *generated at
image build time* from `docker-init.j2` via `RUN sonic-cfggen -a
"{\"ENABLE_ASAN\":\"{{ENABLE_ASAN}}\"}" -t docker-init.j2 > /usr/bin/
docker-init.sh` (verified in `dockers/docker-orchagent/Dockerfile.j2`) --
the template has exactly one build-time-resolved Jinja conditional
(`{% if ENABLE_ASAN == "y" %}`). This module treats `init_script_template`
features (currently only swss) as raw Jinja2 *text* -- exactly like
`supervisord_merge.py` treats supervisord `.j2` sources -- preserving any
`{{ }}`/`{% %}` constructs verbatim rather than trying to render them
itself (`ENABLE_ASAN` is only known at the real sonic-buildimage build).
`gen_dockerfile.py` is responsible for re-running the equivalent
`sonic-cfggen -a "{\"ENABLE_ASAN\":\"{{ENABLE_ASAN}}\"}" -t ...` build-time
render for any such preinit before mega's container ever starts.

This module does **not** write anything to the real tree itself (same
style as `gen_mega_mk.py`/`supervisord_merge.py`) -- `generate_docker_init`
returns a `DockerInitResult`; `gen_dockerfile.py` / `patch_templates.py`
own the actual file writes, or this module's own `__main__` smoke-test
block when explicitly asked via `--out-dir`.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional, Sequence, Tuple

from mega_gen.discovery import FeatureSpec
from mega_gen.supervisord_merge import CANONICAL_ORDER

# ---------------------------------------------------------------------------
# Per-service isolation dirs (plan section 1: "Colliding files ... relocated
# to /opt/sonic/core-services/<svc>/ so multiple services coexist without
# naming conflicts"). Shared with gen_dockerfile.py (start.sh relocation) --
# duplicated as a tiny format-string constant rather than imported, to keep
# the two modules' only real coupling the DockerInitResult/PreinitScript
# data returned here (gen_dockerfile.py imports those, not this constant).
# ---------------------------------------------------------------------------

ISOLATION_DIR_FMT = "/opt/sonic/core-services/{feature}"

# Convention this module establishes for where each stripped preinit script
# ends up at container RUNTIME (gen_dockerfile.py is responsible for making
# this true at build time, whether the source was a literal .sh (plain COPY)
# or a .j2 template (COPY + a build-time sonic-cfggen render, swss's case)).
PREINIT_RUNTIME_PATH_FMT = "/usr/bin/docker-mega-{feature}-preinit.sh"

# ---------------------------------------------------------------------------
# Text surgery: strip `exec .../supervisord`, retarget shared output paths.
# ---------------------------------------------------------------------------

# Matches a whole line that `exec`s supervisord, in either of the two forms
# observed across every default feature's init script (`exec /usr/local/bin/
# supervisord` -- the only form actually present in this tree, but the
# bare-name form is included defensively for any future feature that omits
# the absolute path).
_EXEC_SUPERVISORD_RE = re.compile(
    r"^[ \t]*exec[ \t]+(?:/usr/local/bin/)?supervisord\b.*$", re.MULTILINE
)

# The two shared OUTPUT paths whose `sonic-cfggen -t` renders (or shell
# redirects) must be stripped from each preinit.  All features render
# their own supervisord / critical_processes template to these same two
# well-known paths, regardless of each feature's own template filename
# (e.g. bgp uses `.../supervisord/supervisord.conf.j2`, radv uses
# `.../docker-router-advertiser.supervisord.conf.j2`).  Matching on
# the output path is therefore the robust, feature-agnostic approach.
_DEAD_OUTPUT_SUPERVISORD = "/etc/supervisor/conf.d/supervisord.conf"
_DEAD_OUTPUT_CRITICAL = "/etc/supervisor/critical_processes"


def _strip_exec_supervisord(text: str) -> Tuple[str, int]:
    """Comment out every `exec .../supervisord` line (there can be more
    than one -- e.g. `database`'s own init script has an early-return
    `chassisdb` branch with its own `exec` before the main-path one).
    Returns `(new_text, count_stripped)`."""
    return _EXEC_SUPERVISORD_RE.subn(
        "# (docker-mega preinit: 'exec supervisord' stripped here by "
        "gen_docker_init.py -- mega's own docker-mega-init.sh execs the "
        "ONE shared supervisord instead, after every selected feature's "
        "preinit has run)",
        text,
    )


def _strip_dead_renders(text: str) -> Tuple[str, bool, bool]:
    """Strip ``sonic-cfggen -t`` render arguments whose **output** targets
    one of the two shared supervisor paths from the preinit text.

    In the mega container, mega's own orchestrator renders the merged
    supervisord.conf.j2 and the static ``critical_processes`` file is placed
    by the Dockerfile -- per-feature preinit renders to these shared paths
    are dead (no consumer reads from the isolation dirs they'd target) and
    worse, the per-feature templates may not even be installed in the mega
    image (critical_processes.j2 is not installed at all, so
    ``sonic-cfggen`` would crash).

    Matching on the **output** path (after the ``,`` in a ``-t tmpl,out``
    pair, or after ``>`` in a redirect) is robust across features that use
    different template filenames but all target the same two shared output
    paths (verified across database, swss, bgp, radv, lldp, teamd, snmp).

    Also cleans up dangling ``\\`` line continuations left by removing
    lines from a multi-line shell command.

    Returns ``(new_text, stripped_supervisord, stripped_critical)``.
    """
    stripped_sup = False
    stripped_crit = False

    dead_outputs = (
        (_DEAD_OUTPUT_SUPERVISORD, "sup"),
        (_DEAD_OUTPUT_CRITICAL, "crit"),
    )

    lines = text.split("\n")
    out: List[str] = []

    for line in lines:
        s = line.rstrip()
        should_strip = False

        for output_path, kind in dead_outputs:
            escaped = re.escape(output_path)
            # Form 1: -t TEMPLATE,OUTPUT (sonic-cfggen pair arg)
            if re.search(rf"(?:^|\s)-t\s+\S+,{escaped}(?:\s|$)", s):
                should_strip = True
            # Form 2: > OUTPUT (shell redirect — whole command line)
            elif re.search(rf">\s*{escaped}(?:\s|$)", s):
                should_strip = True

            if should_strip:
                if kind == "sup":
                    stripped_sup = True
                else:
                    stripped_crit = True
                break

        if should_strip:
            # If this is the terminal line of a `\`-continued block (does
            # not itself end with `\`), the previous emitted line's trailing
            # `\` is now dangling -- strip it.
            if not s.endswith("\\") and out:
                prev = out[-1].rstrip()
                if prev.endswith("\\"):
                    out[-1] = prev[:-1].rstrip()
            continue

        out.append(line)

    return "\n".join(out), stripped_sup, stripped_crit


# ---------------------------------------------------------------------------
# Output types.
# ---------------------------------------------------------------------------


@dataclass
class PreinitScript:
    feature: str

    generated_filename: str
    """Basename this module wants written under `dockers/docker-mega/`,
    e.g. `preinit-teamd.sh` or `preinit-swss.sh.j2`."""

    content: str
    """Stripped + retargeted script text, ready to write to
    `dockers/docker-mega/<generated_filename>`."""

    is_template: bool
    """True if the source was a `.j2` template (`init_script_template`,
    currently only swss) -- `gen_dockerfile.py` must render this at build
    time (e.g. via the same `sonic-cfggen -a "{\\"ENABLE_ASAN\\":...}"`
    pattern swss's own Dockerfile.j2 uses) rather than a plain `COPY`."""

    source_relpath: str
    """The real source file this was derived from, for auditability."""

    runtime_path: str
    """Where this ends up at container runtime, once `gen_dockerfile.py`
    has placed it (see `PREINIT_RUNTIME_PATH_FMT`) -- what
    `docker-mega-init.sh` actually invokes."""

    exec_supervisord_stripped: int
    """How many `exec .../supervisord` lines were removed (0 is suspicious
    -- see the warning `build_preinit_scripts` emits for that case)."""

    stripped_supervisord_render: bool
    stripped_critical_processes_render: bool


@dataclass
class DockerInitResult:
    preinit_scripts: List[PreinitScript] = field(default_factory=list)
    """One per non-static selected feature, in the same canonical order
    `docker-mega-init.sh` runs them in."""

    orchestrator_filename: str = "docker-mega-init.sh"
    orchestrator_text: str = ""

    static_features: List[str] = field(default_factory=list)
    """Selected features with `uses_supervisord_directly` (gnmi/eventd/
    sysmgr in the default 10) -- no preinit needed for these; their static
    supervisord.conf's `[program:X]` stanzas are already folded into mega's
    own merged conf by `supervisord_merge.py`."""

    critical_processes_is_template: bool = False
    """True if any selected feature's own `critical_processes` is itself a
    Jinja2 template (e.g. database's `{% for redis_inst, ... in INSTANCES %}`
    loop, or swss's `{% if DEVICE_METADATA... %}`) -- when true, the
    orchestrator itself renders the merged `critical_processes.j2`
    (`supervisord_merge.py`'s output) via `sonic-cfggen` right alongside the
    supervisord.conf.j2 render (Finding #8 fix), and `gen_dockerfile.py` must
    ship the merged file as `critical_processes.j2` (to
    `/usr/share/sonic/templates/`) instead of a plain, static `COPY` to
    `/etc/supervisor/critical_processes`."""

    warnings: List[str] = field(default_factory=list)


# ---------------------------------------------------------------------------
# Main entry points.
# ---------------------------------------------------------------------------


def build_preinit_scripts(
    specs: Sequence[FeatureSpec],
) -> Tuple[List[PreinitScript], List[str], List[str]]:
    """Returns `(preinit_scripts, static_features, warnings)` -- see
    `DockerInitResult`'s fields of the same names."""
    order_index = {name: i for i, name in enumerate(CANONICAL_ORDER)}
    ordered = sorted(
        specs, key=lambda s: order_index.get(s.feature, len(CANONICAL_ORDER) + 1)
    )

    scripts: List[PreinitScript] = []
    static_features: List[str] = []
    warnings: List[str] = []

    for spec in ordered:
        feat = spec.feature

        if spec.uses_supervisord_directly:
            static_features.append(feat)
            continue

        is_template = spec.init_script is None and spec.init_script_template is not None
        source_path: Optional[Path] = spec.init_script or spec.init_script_template
        if source_path is None:
            warnings.append(
                f"{feat}: neither uses_supervisord_directly nor an "
                f"init_script/init_script_template was discovered for it -- "
                f"no preinit generated; its ENTRYPOINT convention doesn't "
                f"match any of the three shapes discovery.py knows about "
                f"(verify manually, e.g. a brand-new unproven feature)"
            )
            continue

        raw = source_path.read_text(errors="replace")
        stripped, n_exec = _strip_exec_supervisord(raw)
        if n_exec == 0:
            warnings.append(
                f"{feat}: init script {source_path} has no literal "
                f"'exec .../supervisord' line to strip -- copied through "
                f"as a preinit unmodified; verify it doesn't start "
                f"supervisord some other way that would now run a second, "
                f"competing supervisord instance inside mega"
            )
        rendered, sup, crit = _strip_dead_renders(stripped)

        ext = ".sh.j2" if is_template else ".sh"
        filename = f"preinit-{feat}{ext}"
        scripts.append(
            PreinitScript(
                feature=feat,
                generated_filename=filename,
                content=rendered,
                is_template=is_template,
                source_relpath=str(source_path),
                runtime_path=PREINIT_RUNTIME_PATH_FMT.format(feature=feat),
                exec_supervisord_stripped=n_exec,
                stripped_supervisord_render=sup,
                stripped_critical_processes_render=crit,
            )
        )

    return scripts, static_features, warnings


def build_orchestrator(
    preinit_scripts: Sequence[PreinitScript],
    critical_processes_is_template: bool = False,
) -> str:
    """The single `docker-mega-init.sh` that becomes mega's own ENTRYPOINT:
    create every selected feature's isolation dir, run each stripped
    preinit in canonical order (each is its own bash process -- deliberate,
    so two features' scripts can't collide on shell function/variable names
    even though the *source* scripts were never written with that in mind),
    render mega's own *merged* supervisord.conf.j2 (supervisord_merge.py's
    output -- this module doesn't produce that file, just assumes
    `gen_dockerfile.py` COPYs it to the same conventional
    `/usr/share/sonic/templates/supervisord.conf.j2` path every other
    feature's own entry-point config uses), then `exec`s the one shared
    supervisord -- exactly mirroring the tail of every individual feature's
    own (now-stripped) init script, just done once for the whole container.

    **F8/F11 fix:** renders supervisord.conf.j2 via ``-j`` (file-based
    cfggen) instead of ``-d`` (redis-based). At ENTRYPOINT time redis has
    not started yet -- it is started BY supervisord. The reference PoC
    reads from JSON config files for the same reason. ``-j init_cfg.json``
    provides the baseline; ``-j config_db.json`` overlays device-specific
    config when present (first boot to a new image may not have it yet).

    Preinit scripts are invoked with ``|| true`` so a failure in one
    feature's non-critical init (e.g. a ``sonic-cfggen -d`` for a
    feature-specific template that can't reach redis) does not abort the
    whole ENTRYPOINT. The critical work (supervisord.conf rendering) is
    done by the orchestrator itself using file-based cfggen, not by any
    preinit.
    """
    lines: List[str] = [
        "#!/usr/bin/env bash",
        "# Auto-generated by optimizations/mega-generic/mega_gen/gen_docker_init.py",
        "# -- DO NOT EDIT BY HAND. Re-run the generator instead.",
        "#",
        "# Preinit pattern (generic_mega-container_merge_script plan, section 7):",
        "# every selected feature's own init-script logic (device-metadata-driven",
        "# config/template rendering + mkdir/chmod side effects -- NOT starting",
        "# supervisord itself, that line was stripped at generation time) runs",
        "# sequentially here, each in its own subshell process (isolates shell",
        "# function/variable names across features), THEN this script renders",
        "# mega's own merged supervisord.conf.j2 (mega_gen.supervisord_merge's",
        "# output) exactly once and execs the single shared supervisord.",
        "#",
        "# NOTE: preinit scripts may contain sonic-cfggen -d calls for",
        "# feature-specific configs (bgp/frr, radv, snmp, etc.).  Since redis",
        "# is not running yet at this point (it starts under supervisord),",
        "# those -d calls may fail.  We tolerate failures (|| true) because:",
        "#   1. The supervisord.conf render (the critical step) uses -j below",
        "#   2. Feature daemons re-read CONFIG_DB at runtime once redis is up",
        "#   3. The host postStartAction loads config and sets up the DB",
        "set -e",
        "",
    ]

    if preinit_scripts:
        lines.append("# --- per-service isolation dirs (plan section 1) ---")
        for ps in preinit_scripts:
            lines.append(f"mkdir -p {ISOLATION_DIR_FMT.format(feature=ps.feature)}")
        lines.append("")

        lines.append("# --- run every selected feature's own (stripped) preinit ---")
        lines.append("# Tolerate failures: some preinits have sonic-cfggen -d calls for")
        lines.append("# feature-specific configs, but redis is not up yet.  The critical")
        lines.append("# supervisord.conf render below uses -j (file-based), not -d.")
        for ps in preinit_scripts:
            lines.append(f'echo "[docker-mega-init] running {ps.feature} preinit" >&2')
            lines.append(f"bash {ps.runtime_path} || echo '[docker-mega-init] {ps.feature} preinit returned non-zero (tolerated)' >&2")
        lines.append("")
    else:
        lines.append("# (no selected feature needed a preinit -- e.g. an all-static subset)")
        lines.append("")

    lines.append("# --- mega's own merged supervisord config (supervisord_merge.py) ---")
    lines.append("# F8/F11 fix: use -j (file-based cfggen) NOT -d (redis-based).")
    lines.append("# Redis has not started yet -- it starts UNDER supervisord.")
    lines.append("mkdir -p /etc/supervisor/conf.d")
    lines.append("CFGGEN_ARGS=''")
    lines.append("if [ -r /etc/sonic/init_cfg.json ]; then")
    lines.append("    CFGGEN_ARGS=\"-j /etc/sonic/init_cfg.json\"")
    lines.append("fi")
    lines.append("if [ -r /etc/sonic/config_db.json ]; then")
    lines.append("    CFGGEN_ARGS=\"$CFGGEN_ARGS -j /etc/sonic/config_db.json\"")
    lines.append("fi")
    if any(ps.feature == "database" for ps in preinit_scripts):
        lines.append("")
        lines.append(
            "# Finding #3 fix: the merged supervisord.conf.j2 template still"
        )
        lines.append(
            "# contains database's own '{% for redis_inst, ... in INSTANCES %}'"
        )
        lines.append(
            "# loop (see dockers/docker-database/supervisord.conf.j2) -- feed it"
        )
        lines.append(
            "# the SAME database_config.json + additional_data_json (is_protected_mode"
        )
        lines.append(
            "# overlay) the database preinit above itself uses to render its own"
        )
        lines.append(
            "# supervisord.conf, or INSTANCES stays undefined and no redis-server"
        )
        lines.append("# programs get rendered at all.")
        lines.append('DB_CFG_FILE="/var/run/redis/sonic-db/database_config.json"')
        lines.append('if [ -r "$DB_CFG_FILE" ]; then')
        lines.append('    CFGGEN_ARGS="$CFGGEN_ARGS -j $DB_CFG_FILE"')
        lines.append(
            "    DB_ADDITIONAL_DATA_JSON=$(jq -c "
            "'{INSTANCES: .INSTANCES | map_values({is_protected_mode: "
            "(.hostname == \"127.0.0.1\")})}' \"$DB_CFG_FILE\" 2>/dev/null || echo '{}')"
        )
        lines.append('    CFGGEN_ARGS="$CFGGEN_ARGS -a $DB_ADDITIONAL_DATA_JSON"')
        lines.append("else")
        lines.append(
            "    echo '[docker-mega-init] WARNING: $DB_CFG_FILE not found "
            "after database preinit -- supervisord.conf will render with no "
            "redis-server programs' >&2"
        )
        lines.append("fi")
        lines.append("")
    lines.append("if [ -z \"$CFGGEN_ARGS\" ]; then")
    lines.append("    echo '[docker-mega-init] WARNING: neither init_cfg.json nor config_db.json found, rendering supervisord.conf with empty config' >&2")
    lines.append("fi")
    lines.append(
        "sonic-cfggen $CFGGEN_ARGS -t /usr/share/sonic/templates/supervisord.conf.j2 "
        "> /etc/supervisor/conf.d/supervisord.conf"
    )
    if critical_processes_is_template:
        lines.append("")
        lines.append(
            "# Finding #8 fix: at least one folded feature's own"
        )
        lines.append(
            "# critical_processes is itself a Jinja2 template (e.g. database's"
        )
        lines.append(
            "# '{% for redis_inst, ... in INSTANCES %}' loop) -- the merged"
        )
        lines.append(
            "# critical_processes union (supervisord_merge.py's output) was"
        )
        lines.append(
            "# therefore shipped as critical_processes.j2 (not the plain,"
        )
        lines.append(
            "# static 'critical_processes' name) so it gets rendered here with"
        )
        lines.append(
            "# the SAME $CFGGEN_ARGS as supervisord.conf.j2 above, instead of"
        )
        lines.append(
            "# copying raw unrendered Jinja text into /etc/supervisor/critical_processes"
        )
        lines.append(
            "# (which system-health's service_checker.py would flag as invalid syntax)."
        )
        lines.append(
            "sonic-cfggen $CFGGEN_ARGS -t /usr/share/sonic/templates/critical_processes.j2 "
            "> /etc/supervisor/critical_processes"
        )
    lines.append("")
    lines.append("exec /usr/local/bin/supervisord")
    lines.append("")
    return "\n".join(lines)


def generate_docker_init(specs: Sequence[FeatureSpec]) -> DockerInitResult:
    """Generate the preinit scripts + orchestrator for the already-discovered
    `specs` (see `mega_gen.discovery.discover_features`). Read-only against
    the tree (only reads each feature's own init script/template) -- writes
    nothing itself; see module docstring."""
    scripts, static_features, warnings = build_preinit_scripts(specs)
    critical_is_template = any(
        s.critical_processes_is_template for s in specs if s.critical_processes is not None
    )
    orchestrator_text = build_orchestrator(scripts, critical_is_template)
    return DockerInitResult(
        preinit_scripts=scripts,
        orchestrator_text=orchestrator_text,
        static_features=static_features,
        critical_processes_is_template=critical_is_template,
        warnings=warnings,
    )


# ---------------------------------------------------------------------------
# Standalone smoke test (not part of the pipeline wiring -- that's
# patch_templates.py's job, a separate to-do). Lets this module be exercised
# directly against a real sonic-buildimage tree:
#
#   python3 -m mega_gen.gen_docker_init --sonic-root /path/to/sonic-buildimage \
#       --features database,swss,bgp,teamd,lldp,snmp,gnmi,radv,eventd,sysmgr \
#       --out-dir /tmp/mega-init
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    import argparse
    import sys

    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
    from mega_gen.auto_registry import discover_registry, resolve_features
    from mega_gen.discovery import discover_features as _discover_features

    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--sonic-root", required=True, type=Path)
    ap.add_argument(
        "--features",
        default="database,swss,bgp,teamd,lldp,snmp,gnmi,radv,eventd,sysmgr",
    )
    ap.add_argument("--out-dir", type=Path, default=None)
    args = ap.parse_args()

    registry = discover_registry(args.sonic_root)
    resolved = resolve_features(args.features.split(","), registry)
    if resolved.unknown or resolved.excluded:
        print(f"ERROR: unknown={resolved.unknown} excluded={resolved.excluded}", file=sys.stderr)
        sys.exit(1)

    specs = _discover_features(args.sonic_root, resolved.resolved, registry)
    result = generate_docker_init(specs)

    print(f"=== static (no preinit) features: {result.static_features} ===")
    print(f"=== preinit scripts ({len(result.preinit_scripts)}) ===")
    for ps in result.preinit_scripts:
        print(
            f"  {ps.feature}: {ps.generated_filename} <- {ps.source_relpath} "
            f"(template={ps.is_template}, exec_stripped={ps.exec_supervisord_stripped}, "
            f"stripped_supervisord={ps.stripped_supervisord_render}, "
            f"stripped_critical={ps.stripped_critical_processes_render}) "
            f"-> runtime {ps.runtime_path}"
        )
    print(f"=== warnings ({len(result.warnings)}) ===")
    for w in result.warnings:
        print(f"  WARNING: {w}")

    if args.out_dir:
        args.out_dir.mkdir(parents=True, exist_ok=True)
        for ps in result.preinit_scripts:
            out_path = args.out_dir / ps.generated_filename
            out_path.write_text(ps.content)
            print(f"wrote {out_path}")
        out_path = args.out_dir / result.orchestrator_filename
        out_path.write_text(result.orchestrator_text)
        print(f"wrote {out_path}")
    else:
        print(f"\n--- {result.orchestrator_filename} (not written, pass --out-dir) ---")
        print(result.orchestrator_text)
