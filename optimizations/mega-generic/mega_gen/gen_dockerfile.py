"""Generate `dockers/docker-mega/Dockerfile.j2` (plan section 6, "Hybrid
Dockerfile strategy" + section 1/7's per-service isolation dirs & preinit
pattern).

Implements the `gen-dockerfile-init` to-do's first half: *"Implement
gen_dockerfile.py: generate dockers/docker-mega/Dockerfile.j2 using a hybrid
strategy -- {% include Dockerfile.common.j2 %} for features that have one
(database, swss, bgp, lldp), rsync from built image stages for those that
don't (teamd, snmp, gnmi, radv, eventd, sysmgr). Relocate colliding files
(start.sh, supervisord.conf) to per-service isolation dirs
(/opt/sonic/core-services/<svc>/)."*

Two-stage build, mirroring **every** feature's own `Dockerfile.j2` in this
tree (`FROM $BASE AS base` does all the work; `FROM $BASE` + the shared
`rsync_from_builder_stage()` macro copies the whole result into a clean
final stage -- verified against `docker-teamd`/`docker-snmp`/`docker-lldp`/
`docker-database`/etc., all identical in shape) -- **not** invented for
mega, just applied once for the union of every selected feature's own work:

  1. `ARG BASE=<base-layer>-{{DOCKER_USERNAME}}:{{DOCKER_USERTAG}}` -- the
     same "heaviest common ancestor" `gen_mega_mk.py` already derives
     (`*_SWSS_LAYER*` if `swss` selected, else `*_CONFIG_ENGINE*`) --
     re-derived here from the same `FeatureSpec`s rather than importing
     `MegaMkResult` (plan section 5's pipeline diagram draws `gen_mega_mk.py`
     and `gen_dockerfile.py` as parallel, independent consumers of
     `discovery.py`, not of each other), reusing `gen_mega_mk.py`'s own
     `load_docker_tokens_of` text-scanning primitive to avoid re-inventing
     that regex.
  2. One `FROM docker-<stem>-{{DOCKER_USERNAME}}:{{DOCKER_USERTAG}} AS
     <feat>-layer` build stage per feature with **no** `Dockerfile.common.j2`
     (`FeatureSpec.dockerfile_common is None` -- teamd/snmp/gnmi/radv/
     eventd/sysmgr in the default 10) -- these are the images
     `gen_mega_mk.py` already arranges to have `_LOAD_DOCKERS`ed (and
     therefore locally `docker load`ed + tagged, per `slave.mk`'s
     `docker-image-load` macro) before mega's own build runs.
  3. Inside `base`:
     a. The union of every selected feature's own `_DEPENDS`/
        `_PYTHON_WHEELS` (already merged into mega's own `_DEPENDS` by
        `_INCLUDE_DOCKER`, exported by `slave.mk` as `docker_mega_debs`/
        `docker_mega_whls` -- mega's *own* docker-dir-basename-derived
        variable names, not each feature's own).
     b. `bgp`-only: FRR user/group creation (plan section 12) -- `bgp`'s own
        `Dockerfile.j2` does this itself (`groupadd`/`useradd`/`chown
        /etc/frr`), but that happens OUTSIDE its `Dockerfile.common.j2`
        (verified against the source), so it doesn't come along for free
        via the include below and must be replicated explicitly.
     c. One block per **common.j2-bucket** feature: `{% set copy_from =
        "<build-context-name>" %} {% include "<stem>/Dockerfile.common.j2"
        %}` (the `docker-sonic-vs` precedent, confirmed still live in this
        tree at `platform/vs/docker-sonic-vs/Dockerfile.j2`) -- pulls in
        that feature's shared static assets from its own **source**
        directory via the `--build-context`/`-I` machinery `_INCLUDE_DOCKER`
        already wires up (`rules/functions`), needing neither that
        feature's own built image nor `_LOAD_DOCKERS`.
     d. Best-effort supplement for (c): any top-level `*.j2` file under that
        feature's own docker dir that `Dockerfile.common.j2` did *not*
        already cover and isn't one of the supervisord/critical_processes/
        Dockerfile basenames handled elsewhere -- glob-copied the same way
        that feature's *own* `Dockerfile.j2` does it (verified: `swss`
        (`docker-orchagent`) literally has `COPY ["*.j2", "/usr/share/
        sonic/templates/"]` in its own Dockerfile.j2, right after its own
        `{% include "Dockerfile.common.j2" %}`, to pick up
        `switch.json.j2`/`ports.json.j2`/etc. that its own `docker-init.j2`
        needs at runtime). Flagged explicitly as best-effort/non-exhaustive
        (see `_extra_j2_templates_of`'s docstring) -- this is exactly the
        "fragile Dockerfile parsing" the plan's own rationale for the rsync
        bucket says to avoid, kept minimal and well-scoped rather than a
        general Dockerfile.j2 parser.
     e. One rsync block per **no-common.j2-bucket** feature: `RUN
        --mount=type=bind,from=<feat>-layer,target=/svc rsync -axAX
        --omit-dir-times --no-D --exclude=/sys --exclude=/proc
        --exclude=/dev --exclude=resolv.conf --exclude=/etc/supervisor/
        conf.d/ --exclude=/usr/bin/start.sh /svc/ /` -- captures that
        feature's *entire* already-built-and-processed filesystem (apt
        packages actually installed, e.g. `radv`'s `radvd` binary or
        `snmp`'s `sonic_ax_impl install`-touched files -- things no plain
        source-file COPY could reproduce without re-executing that
        feature's own Dockerfile.j2 RUN commands, which is exactly what
        this strategy is chosen to avoid). `/etc/supervisor/conf.d/` is
        excluded wholesale so no feature's own static/rendered supervisord
        config can clobber mega's shared merged one.

        **Caveat, flagged rather than silently assumed away**: each
        rsync-bucket feature's own image may be based on a *different*
        `*_CONFIG_ENGINE*`/`*_SWSS_LAYER*` base than mega's own chosen
        `ARG BASE` (e.g. `gnmi`/`eventd`/`sysmgr` are config-engine-based
        while mega is swss-layer-based whenever `swss` is selected) --
        rsyncing the whole filesystem in *could* in principle overwrite a
        shared base file with a divergent version from the other base.
        Not observed as an actual problem against this tree (both
        `*-trixie` base images are built from the same Debian release), but
        called out here rather than asserted safe.
     f. Every selected feature's own `start.sh`, if it has one (regardless
        of bucket -- `supervisord_merge.py`'s own shared `[program:start]`
        subshells `/opt/sonic/core-services/<feat>/start.sh` for every
        "starter" feature it found, so this relocation is a hard
        requirement of that module's contract, not optional): `COPY
        --from=<ctx> ["start.sh", "/opt/sonic/core-services/<feat>/
        start.sh"]` + a `chmod`, sourced from that feature's own **build
        context** (available for every selected feature via
        `_INCLUDE_DOCKER`, common.j2 bucket or not) rather than rsync --
        `start.sh` is always a static source file, never build-time
        generated, so the simpler mechanism suffices and sidesteps the
        no-such-file problem in (e)'s bullet above entirely.
     g. Every selected feature's own preinit script
        (`gen_docker_init.generate_docker_init`'s output) placed at its
        conventional runtime path (plain `COPY` + `chmod` for a literal
        `.sh`; `COPY` + a build-time `sonic-cfggen` render + `chmod` for a
        `.j2` template, `swss`'s case -- mirrors that feature's *own*
        Dockerfile.j2 build-time render of `docker-init.j2` verbatim,
        including the one Jinja knob it actually depends on,
        `ENABLE_ASAN`).
     h. Mega's own generated files (`supervisord_merge.py`'s output +
        `gen_docker_init.py`'s orchestrator): `COPY`d in last, after (d)'s
        best-effort glob so mega's own merged `supervisord.conf.j2`/
        `critical_processes` always wins any basename collision with a
        raw per-feature copy.
  4. Final stage: `{{ rsync_from_builder_stage() }}` (same macro every
     feature already uses) + `ENTRYPOINT ["/usr/bin/docker-mega-init.sh"]`
     (`gen_docker_init.py`'s orchestrator, not any single feature's own
     entry point).

**Deliberate deviation from the plan's illustrative section-6 snippet**,
same one `gen_docker_init.py`'s docstring documents in more depth (repeated
briefly here since it directly shapes this module's rsync block, (e)
above): that snippet's `install ... /svc/etc/supervisor/conf.d/
supervisord.conf ...` assumes a file that, verified against every dynamic-
init feature's own init script in this tree, does not exist pre-runtime
(it's rendered by `sonic-cfggen -d ...` at container *startup*, not baked
in at build time) -- so this module excludes `/etc/supervisor/conf.d/`
from the rsync entirely instead of trying to `install` a nonexistent file,
and relies on `gen_docker_init.py`'s preinit retargeting (runtime, not
build-time) to get each feature's own rendered output into its isolation
dir instead.

This module does **not** write anything to the real tree itself (same
style as every other `gen_*`/`*_merge` module here) -- `generate_dockerfile`
returns a `DockerfileResult`; `patch_templates.py` (a separate to-do) owns
the actual file writes, or this module's own `__main__` smoke-test block.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Set, Tuple

from mega_gen.discovery import FeatureSpec
from mega_gen.gen_docker_init import DockerInitResult, PreinitScript, generate_docker_init
from mega_gen.gen_mega_mk import load_docker_tokens_of
from mega_gen.supervisord_merge import CANONICAL_ORDER

ISOLATION_DIR_FMT = "/opt/sonic/core-services/{feature}"

_RSYNC_EXCLUDES: Tuple[str, ...] = (
    "--exclude=/sys",
    "--exclude=/proc",
    "--exclude=/dev",
    "--exclude=resolv.conf",
    "--exclude=/etc/supervisor/conf.d/",
    "--exclude=/usr/bin/start.sh",
    # --- identity/dpkg files: excluded from the wholesale rsync below and
    # merged additively instead (see _IDENTITY_MERGE_FILES / the merge
    # shell snippet appended to each rsync RUN block). Confirmed finding:
    # sequentially rsyncing full per-feature rootfs's on top of each other
    # would otherwise let the LAST feature's /etc/passwd,/etc/group,
    # /etc/shadow,/etc/gshadow and /var/lib/dpkg/status* silently win,
    # losing earlier features' special users (redis, frr, _lldpd, ...)
    # and dpkg package records.
    "--exclude=/etc/passwd",
    "--exclude=/etc/group",
    "--exclude=/etc/shadow",
    "--exclude=/etc/gshadow",
    "--exclude=/var/lib/dpkg/status",
    "--exclude=/var/lib/dpkg/status-old",
    "--exclude=/var/lib/dpkg/available",
)

# Identity files merged additively (by first ':'-delimited field, i.e. user/
# group name) after each rsync-bucket feature's rootfs is copied in, instead
# of being overwritten wholesale. New entries from that feature's image are
# appended only if that name isn't already known (keeps whichever feature
# defined it first -- order doesn't matter for correctness since UIDs/GIDs
# for shared system accounts like redis/frr are fixed by the same Debian
# packages/build args across every feature image).
_IDENTITY_MERGE_FILES: Tuple[str, ...] = (
    "/etc/passwd",
    "/etc/group",
    "/etc/shadow",
    "/etc/gshadow",
)

_DOCKERFILE_MACROS_IMPORT = (
    '{% from "dockers/dockerfile-macros.j2" import install_debian_packages, '
    "install_python_wheels, copy_files, rsync_from_builder_stage %}"
)

# ---------------------------------------------------------------------------
# Per-feature Dockerfile commands that live in a feature's own Dockerfile.j2
# but NOT in its Dockerfile.common.j2, and are NOT handled by _INCLUDE_DOCKER
# (which only merges _DEPENDS/.deb and _PYTHON_WHEELS/.whl packages, not
# direct apt-get/pip installs from Debian or PyPI repos).
#
# Hardcoded for the known common.j2-bucket features rather than parsed from
# Dockerfiles -- this is a PoC (per the project-purpose rule: "dirty is
# allowed, fastest path to a credible number over a production-quality
# implementation"). If a new common.j2-bucket feature is added with its own
# direct installs, a corresponding entry must be added here manually.
#
# Verified against the original Dockerfile.j2 for each feature and the
# docker-swss-layer-trixie / docker-config-engine-trixie base images to
# confirm these packages are NOT already provided by the base layer.
# ---------------------------------------------------------------------------

# Lines emitted AFTER `RUN apt-get update` but BEFORE the unioned _DEPENDS/
# _PYTHON_WHEELS install blocks. Matches each feature's own Dockerfile.j2
# ordering (packages must be installed before Dockerfile.common.j2, e.g.
# database's common.j2 runs sed on /etc/redis/redis.conf which redis-server
# creates; swss needs conntrack/ndppd/etc. for its runtime programs).
_FEATURE_PRE_INSTALL_LINES: Dict[str, List[str]] = {
    "database": [
        "# --- database: apt/pip packages (not in Dockerfile.common.j2) ---",
        "RUN apt-get install -y redis-tools redis-server",
        "RUN pip3 install click",
        "",
    ],
    "swss": [
        "# --- swss: apt/pip packages (not in Dockerfile.common.j2) ---",
        "RUN apt-get install -f -y \\",
        "        ifupdown \\",
        "        arping \\",
        "        iproute2 \\",
        "        ndisc6 \\",
        "        tcpdump \\",
        "        bridge-utils \\",
        "        conntrack \\",
        "        ndppd \\",
        "        python3-protobuf \\",
        "        pciutils \\",
        "        python3-netifaces",
        "",
        '{% if ( CONFIGURED_ARCH == "armhf" or CONFIGURED_ARCH == "arm64" ) %}',
        "# Fix for gcc/python/iputils-ping not found in arm docker",
        "RUN apt-get install -y \\",
        "        gcc \\",
        "        iputils-ping",
        "{% endif %}",
        "",
        "# Dependencies of restore_neighbors.py",
        "RUN pip3 install pyroute2==0.5.14",
        "",
        '{% if ( CONFIGURED_ARCH == "armhf" or CONFIGURED_ARCH == "arm64" ) %}',
        "# Remove installed gcc",
        "RUN apt-get remove -y gcc",
        "{% endif %}",
        "",
    ],
}

# Lines emitted AFTER the unioned _DEPENDS/_PYTHON_WHEELS install blocks but
# BEFORE the Dockerfile.common.j2 includes. Matches swss's own cleanup
# ordering (purge build deps after all installs are done).
_FEATURE_POST_INSTALL_LINES: Dict[str, List[str]] = {
    "swss": [
        "# Clean up",
        "RUN apt-get purge -y build-essential python3-dev",
        "",
    ],
}

# Lines emitted right after a feature's own Dockerfile.common.j2 include
# (and its extra-j2 supplement), for common.j2-bucket features only.
# database's sysctl config lives in the build context via _FILES (unioned
# by _INCLUDE_DOCKER) but is COPYed in its own Dockerfile.j2 outside the
# common.j2 fragment -- this requires that feature's own build context
# (`COPY --from=<ctx>`), which is only wired up for common-bucket features
# here. For rsync-bucket features (the only bucket populated anywhere in
# this tree today) this file is already brought in wholesale by the rsync
# of that feature's own already-built image, so re-copying it would be both
# redundant and (without --from=<ctx>) a build error (no such path in
# mega's own build context). See _FEATURE_POST_COMMON_LINES_ANY below for
# the bucket-independent counterpart.
_FEATURE_POST_COMMON_LINES: Dict[str, List[str]] = {
    "database": [
        'COPY --from={ctx} ["files/90-sonic.conf", "/usr/lib/sysctl.d/"]',
    ],
}

# Lines that must run once a feature's own files are present on disk,
# regardless of *how* they got there (Dockerfile.common.j2 include or
# rsync from the built image) -- applied to every selected feature after
# both the common.j2-include loop and the rsync loop have run (finding #1:
# gating this on common_bucket alone means it silently never fires, since
# no Dockerfile.common.j2 exists anywhere in this tree today).
_FEATURE_POST_COMMON_LINES_ANY: Dict[str, List[str]] = {
    "bgp": [
        "# --- FRR template symlinks (plan section 12) ---",
        "# Defensive guard, not currently expected to fire in the rsync",
        "# bucket: bgp's own Dockerfile.j2 COPYs its 'frr' source dir",
        "# straight onto /usr/share/sonic/templates/ (bgpd/, zebra/, etc.",
        "# already at the top level, verified against dockers/docker-fpm-frr/",
        "# Dockerfile.j2), so bgp's own rsynced image needs no symlinks. Kept",
        "# in case a future Dockerfile.common.j2 nests them under frr/.",
        "RUN for d in bgpd zebra staticd common; do \\",
        "    if [ ! -e /usr/share/sonic/templates/$d ] && \\",
        "       [ -d /usr/share/sonic/templates/frr/$d ]; then \\",
        "        ln -sf frr/$d /usr/share/sonic/templates/$d; \\",
        "    fi; \\",
        "  done",
    ],
}

# Lines emitted in the final stage (after rsync_from_builder_stage). The
# database container exposes INCLUDE_SYSTEM_EVENTD to its init script, which
# controls the database_config.json template selection at runtime.
_FEATURE_FINAL_STAGE_LINES: Dict[str, List[str]] = {
    "database": [
        'ENV INCLUDE_SYSTEM_EVENTD={{ include_system_eventd | default("n") }}',
    ],
}

MEGA_DOCKERFILE_NAME = "Dockerfile.j2"
MEGA_INIT_TEMPLATES_DIR = "/usr/share/sonic/templates"

# ---------------------------------------------------------------------------
# Depstartup pending-start guard (plan section 12).
# ---------------------------------------------------------------------------

DEPSTARTUP_GUARD_FILENAME = "depstartup_guard.py"
DEPSTARTUP_GUARD_RUNTIME_PATH = "/usr/local/bin/depstartup_guard.py"


def generate_depstartup_guard() -> str:
    """Return the content of the pending-start guard wrapper script.

    This script replaces the stock ``python3 -m supervisord_dependent_startup``
    command in the merged ``[eventlistener:dependent-startup]`` stanza.  It
    monkey-patches ``xmlrpc.client._Method.__call__`` (the single chokepoint
    through which every XML-RPC call to supervisor passes) to intercept
    ``supervisor.startProcess`` calls and skip processes already in a
    transitional state (STARTING, RUNNING, BACKOFF).

    This prevents the respawn storm that occurs in the mega container's
    merged supervisord when a crashed process triggers both ``autorestart``
    and ``dependent-startup`` simultaneously: without the guard, both paths
    try to ``startProcess`` for the same (or dependent) process, the second
    call collides with the first, and the resulting failure cascade escalates
    into a container-wide restart loop.

    Robust by design: the patch targets the Python stdlib ``xmlrpc.client``
    internals (stable across Python 3.x) rather than any private API of
    ``supervisord_dependent_startup`` itself, so it works with any version of
    that package without needing to parse or modify its source code.
    """
    return '''\
#!/usr/bin/env python3
"""depstartup pending-start guard (plan section 12, auto-generated).

Wraps supervisord_dependent_startup to prevent the process respawn storm in
the mega container.  Before any ``supervisor.startProcess`` XML-RPC call
reaches the supervisor daemon, this wrapper checks the target process's
current state and silently skips the call if the process is already in
STARTING, RUNNING, or BACKOFF — the three transitional states in which a
redundant ``startProcess`` would either fail or be a no-op that confuses
the dependent-startup state machine.
"""
import sys
import xmlrpc.client

_orig_Method_call = xmlrpc.client._Method.__call__


def _guarded_Method_call(self, *args):
    # _Method stores the dotted RPC method name in the name-mangled
    # ``__name`` attribute (e.g. "supervisor.startProcess").
    # ``__send`` is the bound ``ServerProxy.__request`` method that
    # actually makes the HTTP call — we reuse it for getProcessInfo
    # so the guard uses the same connection/auth as the original call.
    try:
        method_name = self._Method__name
    except AttributeError:
        return _orig_Method_call(self, *args)

    if method_name == "supervisor.startProcess" and args:
        process_name = args[0]
        try:
            send = self._Method__send
            info = send("supervisor.getProcessInfo", (process_name,))
            state = info.get("statename", "")
            if state in ("STARTING", "RUNNING", "BACKOFF"):
                return True  # skip — already transitioning
        except Exception:
            pass  # if the check itself fails, let the start proceed

    return _orig_Method_call(self, *args)


xmlrpc.client._Method.__call__ = _guarded_Method_call

# Run the original supervisord_dependent_startup module as __main__.
import runpy
runpy.run_module("supervisord_dependent_startup", run_name="__main__",
                 alter_sys=True)
'''


# ---------------------------------------------------------------------------
# Generic naming helpers.
# ---------------------------------------------------------------------------


def build_context_name(stem: str) -> str:
    """`docker-teamd` -> `teamd`; `docker-fpm-frr` -> `fpm-frr`;
    `docker-sonic-gnmi` -> `sonic-gnmi` -- exactly `rules/functions`'
    `process_include_dockers`: `$(patsubst docker-%.gz,%,$(docker))`."""
    return stem[len("docker-") :] if stem.startswith("docker-") else stem


def image_layer_ref(stem: str) -> str:
    """The local docker tag `slave.mk`'s `docker-image-load` macro tags a
    `_LOAD_DOCKERS`ed image with once loaded: `$(1)-$(DOCKER_USERNAME):
    $(DOCKER_USERTAG)` where `$(1)` is the docker's own stem (e.g.
    `docker-teamd`) -- verified against `slave.mk`'s `docker-image-load`/
    `DOCKER_IMAGE_REF` definitions."""
    return f"{stem}-{{{{DOCKER_USERNAME}}}}:{{{{DOCKER_USERTAG}}}}"


_BASE_STEM_PATH_RE_FMT = r"^\$\({token}\)_PATH\s*=\s*\$\(DOCKERS_PATH\)/(\S+)"


def base_layer_stem(sonic_root: Path, docker_var_token: str) -> Optional[str]:
    """Resolve a *base* (non-container) docker-var token like
    `DOCKER_SWSS_LAYER_TRIXIE` to its own docker dir stem (e.g.
    `docker-swss-layer-trixie`), generically: verified against both base
    images actually used as mega's own `ARG BASE` candidate
    (`docker-swss-layer-trixie.mk`, `docker-config-engine-trixie.mk`) that
    each names itself `rules/docker-<suffix>.mk` where `<suffix>` is the
    token with `DOCKER_` stripped, lowercased, underscores->dashes -- and
    that stem equals that same suffix. Returns None (with the caller
    responsible for warning) if the convention doesn't hold for some future
    base token."""
    if not docker_var_token.startswith("DOCKER_"):
        return None
    suffix = docker_var_token[len("DOCKER_") :].lower().replace("_", "-")
    candidate_mk = sonic_root / "rules" / f"docker-{suffix}.mk"
    if not candidate_mk.is_file():
        return None
    text = candidate_mk.read_text(errors="replace")
    m = re.search(
        _BASE_STEM_PATH_RE_FMT.format(token=re.escape(docker_var_token)),
        text,
        re.MULTILINE,
    )
    return m.group(1) if m else None


def determine_base_layer(
    specs: Sequence[FeatureSpec], sonic_root: Path
) -> Tuple[Optional[str], List[str]]:
    """Mirrors `gen_mega_mk.generate_mega_mk`'s own base-layer selection
    (kept independent rather than importing `MegaMkResult` -- see module
    docstring point 1): `*_SWSS_LAYER*` if `swss` is selected and any
    selected feature's own `_LOAD_DOCKERS` names one, else the first
    `*_CONFIG_ENGINE*` referenced. Returns `(base_stem, warnings)`."""
    warnings: List[str] = []
    swss_layer_tokens: Dict[str, str] = {}
    config_engine_tokens: Dict[str, str] = {}

    for spec in specs:
        text = (Path(spec.mk_file) if Path(spec.mk_file).is_absolute() else sonic_root / spec.mk_file).read_text(
            errors="replace"
        )
        for tok in load_docker_tokens_of(text, spec.registry_entry.docker_var):
            if "SWSS_LAYER" in tok:
                swss_layer_tokens[spec.feature] = tok
            elif "CONFIG_ENGINE" in tok:
                config_engine_tokens[spec.feature] = tok

    selected_names = {s.feature for s in specs}
    token: Optional[str] = None
    if "swss" in selected_names and swss_layer_tokens:
        token = sorted(set(swss_layer_tokens.values()))[0]
    elif config_engine_tokens:
        token = sorted(set(config_engine_tokens.values()))[0]

    if token is None:
        warnings.append(
            "could not determine a swss-layer/config-engine base image from "
            "any selected feature's own _LOAD_DOCKERS -- mega's Dockerfile.j2 "
            "will have no ARG BASE at all; add one manually"
        )
        return None, warnings

    stem = base_layer_stem(sonic_root, token)
    if stem is None:
        warnings.append(
            f"resolved base token {token} but could not find its own docker "
            f"dir stem via the rules/docker-<suffix>.mk convention -- "
            f"mega's Dockerfile.j2 will have no ARG BASE at all; add one "
            f"manually (base token: {token})"
        )
        return None, warnings

    return stem, warnings


# ---------------------------------------------------------------------------
# Best-effort "extra top-level *.j2 not already covered by Dockerfile.common.j2"
# supplement for the common.j2 bucket (see module docstring point 3.d).
# ---------------------------------------------------------------------------


def _extra_j2_templates_of(spec: FeatureSpec) -> List[str]:
    """Top-level (non-recursive, matching how every feature's own
    `Dockerfile.j2` does `COPY ["*.j2", ...]`) `*.j2` basenames under
    `spec.docker_dir` NOT already handled elsewhere by this generator:
    `Dockerfile(.common).j2` (never copied), the supervisord/
    critical_processes templates (`supervisord_merge.py`'s job), and this
    feature's own `init_script_template` if it has one (`gen_docker_init.py`'s
    job). Best-effort, not exhaustive -- verified against `swss`
    (`docker-orchagent`), the one default feature where this actually
    matters (`switch.json.j2`/`ports.json.j2`/etc., needed by its own
    `docker-init.j2` at runtime) -- but this is exactly the kind of
    per-Dockerfile-content guessing the plan's own rsync-bucket rationale
    warns is fragile, so treat anything picked up here as a bonus, not a
    completeness guarantee for arbitrary future features."""
    if spec.docker_dir is None:
        return []

    skip: Set[str] = {"Dockerfile.j2", "Dockerfile.common.j2"}
    for p in (
        spec.supervisord_conf,
        spec.supervisord_common,
        spec.critical_processes if spec.critical_processes_is_template else None,
        spec.init_script_template,
    ):
        if p is not None:
            skip.add(p.name)

    out = sorted(
        p.name
        for p in spec.docker_dir.glob("*.j2")
        if p.is_file() and p.name not in skip
    )
    return out


# ---------------------------------------------------------------------------
# Output type.
# ---------------------------------------------------------------------------


@dataclass
class DockerfileResult:
    dockerfile_text: str
    """Full generated `dockers/docker-mega/Dockerfile.j2` body."""

    base_stem: Optional[str] = None
    common_bucket_features: List[str] = field(default_factory=list)
    """Selected features with `Dockerfile.common.j2` (`{% include %}`
    strategy) -- database/swss/bgp/lldp in the default 10."""

    rsync_bucket_features: List[str] = field(default_factory=list)
    """Selected features with no `Dockerfile.common.j2` (rsync-from-built-
    image strategy) -- teamd/snmp/gnmi/radv/eventd/sysmgr in the default 10."""

    start_sh_features: List[str] = field(default_factory=list)
    """Selected features that had their own `start.sh` relocated to
    `/opt/sonic/core-services/<feat>/start.sh`."""

    extra_j2_by_feature: Dict[str, List[str]] = field(default_factory=dict)
    """Best-effort extra `*.j2` basenames copied per common.j2-bucket
    feature (see `_extra_j2_templates_of`) -- present (possibly empty) only
    for features in that bucket."""

    docker_init: Optional[DockerInitResult] = None
    """`gen_docker_init.generate_docker_init`'s own result, for callers
    that also need to write out the preinit scripts/orchestrator this
    Dockerfile references (`patch_templates.py`'s job)."""

    depstartup_guard_text: str = ""
    """Content of `depstartup_guard.py` -- the pending-start guard wrapper
    for `supervisord_dependent_startup` (plan section 12).  Always generated
    (the guard is unconditional).  Callers that write the mega docker dir
    must write this file alongside the Dockerfile."""

    warnings: List[str] = field(default_factory=list)


# ---------------------------------------------------------------------------
# Dockerfile emission.
# ---------------------------------------------------------------------------


def _preinit_copy_lines(ps: PreinitScript) -> List[str]:
    if ps.is_template:
        rendered_path = ps.runtime_path
        return [
            f'COPY ["{ps.generated_filename}", "{MEGA_INIT_TEMPLATES_DIR}/"]',
            (
                'RUN sonic-cfggen -a "{\\"ENABLE_ASAN\\":\\"{{ENABLE_ASAN}}\\"}" '
                f"-t {MEGA_INIT_TEMPLATES_DIR}/{ps.generated_filename} > {rendered_path} && \\"
            ),
            f"    rm -f {MEGA_INIT_TEMPLATES_DIR}/{ps.generated_filename} && \\",
            f"    chmod 755 {rendered_path}",
        ]
    return [
        f'COPY ["{ps.generated_filename}", "{ps.runtime_path}"]',
        f"RUN chmod 755 {ps.runtime_path}",
    ]


def generate_dockerfile(
    specs: Sequence[FeatureSpec],
    sonic_root: Path,
    container_name: str = "mega",
    docker_init_result: Optional[DockerInitResult] = None,
) -> DockerfileResult:
    """Generate `dockers/docker-mega/Dockerfile.j2` for the already-
    discovered `specs` (see `mega_gen.discovery.discover_features`).
    Read-only against `sonic_root`. `docker_init_result` may be passed in
    if the caller already computed it (e.g. `patch_templates.py`, to avoid
    re-deriving it); computed internally otherwise."""
    warnings: List[str] = []

    order_index = {name: i for i, name in enumerate(CANONICAL_ORDER)}
    ordered = sorted(
        specs, key=lambda s: order_index.get(s.feature, len(CANONICAL_ORDER) + 1)
    )
    selected_names = {s.feature for s in ordered}

    if docker_init_result is None:
        docker_init_result = generate_docker_init(ordered)
    warnings.extend(f"gen_docker_init: {w}" for w in docker_init_result.warnings)
    preinit_by_feature = {ps.feature: ps for ps in docker_init_result.preinit_scripts}

    base_stem, base_warnings = determine_base_layer(ordered, sonic_root)
    warnings.extend(base_warnings)

    common_bucket = [s for s in ordered if s.dockerfile_common is not None]
    rsync_bucket = [s for s in ordered if s.dockerfile_common is None]

    start_sh_features: List[str] = []
    for spec in ordered:
        if spec.docker_dir is not None and (spec.docker_dir / "start.sh").is_file():
            start_sh_features.append(spec.feature)

    extra_j2_by_feature: Dict[str, List[str]] = {
        s.feature: _extra_j2_templates_of(s) for s in common_bucket
    }

    lines: List[str] = [
        "# dockers/docker-mega/Dockerfile.j2 (auto-generated by",
        "# optimizations/mega-generic/mega_gen_cli.py --apply -- DO NOT EDIT BY HAND)",
        f"# Composed from: {', '.join(s.feature for s in ordered)}",
        "# Hybrid strategy (generic_mega-container_merge_script plan, section 6):",
        f"#   Dockerfile.common.j2 include: {', '.join(s.feature for s in common_bucket) or '(none selected)'}",
        f"#   rsync from built image:       {', '.join(s.feature for s in rsync_bucket) or '(none selected)'}",
        "# See optimizations/mega-generic/README.md for the full design.",
        "",
        _DOCKERFILE_MACROS_IMPORT,
    ]

    if base_stem is not None:
        lines.append(f"ARG BASE={image_layer_ref(base_stem)}")
    else:
        lines.append(
            "# WARNING: no base layer could be determined -- ARG BASE is "
            "missing, this Dockerfile will not build as-is. See warnings."
        )
    lines.append("")

    for spec in rsync_bucket:
        lines.append(f"FROM {image_layer_ref(spec.stem)} AS {spec.feature}-layer")
    if rsync_bucket:
        lines.append("")

    lines.append("FROM $BASE AS base")
    lines.append("")
    lines.append("ARG docker_container_name")
    lines.append("ARG image_version")
    if "bgp" in selected_names:
        lines.append("ARG frr_user_uid")
        lines.append("ARG frr_user_gid")
    lines.append("")
    lines.append("ENV DEBIAN_FRONTEND=noninteractive")
    lines.append("ENV IMAGE_VERSION=$image_version")
    lines.append("")
    lines.append("RUN apt-get update")
    lines.append("")

    # Per-feature direct apt-get/pip installs (not in Dockerfile.common.j2,
    # not handled by _INCLUDE_DOCKER -- see _FEATURE_PRE_INSTALL_LINES).
    # Applied to every selected feature regardless of bucket: since no
    # Dockerfile.common.j2 exists anywhere in this tree today, gating this
    # on common_bucket alone means it silently never fires (confirmed
    # finding -- see README "Known limitations").
    for spec in ordered:
        pre = _FEATURE_PRE_INSTALL_LINES.get(spec.feature)
        if pre:
            lines.extend(pre)

    mega_debs_var = f"docker_{container_name}_debs"
    mega_whls_var = f"docker_{container_name}_whls"
    lines.append(f"{{% if {mega_debs_var}.strip() -%}}")
    lines.append(
        "# Union of every selected feature's own _DEPENDS (via _INCLUDE_DOCKER)"
    )
    lines.append(f'{{{{ copy_files("debs/", {mega_debs_var}.split(\' \'), "/debs/") }}}}')
    lines.append("")
    lines.append(f"{{{{ install_debian_packages({mega_debs_var}.split(' ')) }}}}")
    lines.append("{%- endif %}")
    lines.append("")
    lines.append(f"{{% if {mega_whls_var}.strip() -%}}")
    lines.append(
        "# Union of every selected feature's own _PYTHON_WHEELS (via _INCLUDE_DOCKER)"
    )
    lines.append(
        f'{{{{ copy_files("python-wheels/", {mega_whls_var}.split(\' \'), "/python-wheels/") }}}}'
    )
    lines.append("")
    lines.append(f"{{{{ install_python_wheels({mega_whls_var}.split(' ')) }}}}")
    lines.append("{% endif %}")
    lines.append("")

    # F7 fix: pip crypto workaround.  In the merged rootfs, rsyncing
    # multiple feature images can leave conflicting .dist-info metadata
    # for crypto packages (cffi, cryptography, bcrypt, pynacl), causing
    # pip's uninstall-no-record-file errors when the wheel installer
    # tries to upgrade them.  Pre-installing with --ignore-installed
    # --no-deps forces clean metadata, matching the reference PoC.
    if rsync_bucket:
        lines.append(
            "# F7 fix: force clean crypto package metadata before rsync"
        )
        lines.append(
            "# overlays from other feature images (prevents pip"
        )
        lines.append("# uninstall-no-record-file errors in the merged rootfs)")
        lines.append(
            "RUN pip3 install --ignore-installed --no-deps "
            "cffi cryptography bcrypt pynacl 2>/dev/null || true"
        )
        lines.append("")

    # Per-feature post-install cleanup (see _FEATURE_POST_INSTALL_LINES).
    # Applied regardless of bucket -- see the pre-install loop above.
    for spec in ordered:
        post = _FEATURE_POST_INSTALL_LINES.get(spec.feature)
        if post:
            lines.extend(post)

    for spec in common_bucket:
        ctx = build_context_name(spec.stem)
        lines.append(
            f"# --- {spec.feature}: shared Dockerfile.common.j2 fragment "
            f"(source dir via --build-context, docker-sonic-vs precedent) ---"
        )
        lines.append(f'{{% set copy_from = "{ctx}" %}}')
        lines.append(f'{{% include "{spec.stem}/Dockerfile.common.j2" %}}')
        extra = extra_j2_by_feature.get(spec.feature) or []
        if extra:
            quoted = ", ".join(f'"{name}"' for name in extra)
            lines.append(
                "# best-effort supplement: top-level *.j2 templates not covered "
                "by Dockerfile.common.j2 (see gen_dockerfile._extra_j2_templates_of)"
            )
            lines.append(
                f'COPY --from={ctx} [{quoted}, "{MEGA_INIT_TEMPLATES_DIR}/"]'
            )
        post_common = _FEATURE_POST_COMMON_LINES.get(spec.feature)
        if post_common:
            lines.extend(line.format(ctx=ctx) for line in post_common)
        lines.append("")

    for spec in ordered:
        if spec.feature not in start_sh_features:
            continue
        ctx = build_context_name(spec.stem)
        iso = ISOLATION_DIR_FMT.format(feature=spec.feature)
        lines.append(
            f'COPY --from={ctx} ["start.sh", "{iso}/start.sh"]'
        )
        lines.append(f"RUN chmod 755 {iso}/start.sh")
    if start_sh_features:
        lines.append("")

    for spec in ordered:
        ps = preinit_by_feature.get(spec.feature)
        if ps is None:
            continue
        lines.extend(_preinit_copy_lines(ps))
    if preinit_by_feature:
        lines.append("")

    for spec in rsync_bucket:
        exclude_args = " ".join(_RSYNC_EXCLUDES)
        lines.append(
            f"# --- {spec.feature}: rsync from its own already-built image "
            f"(no Dockerfile.common.j2 available here -- avoids fragile "
            f"per-Dockerfile parsing, plan section 6) ---"
        )
        lines.append(
            f"RUN --mount=type=bind,from={spec.feature}-layer,target=/svc \\"
        )
        lines.append(
            f"    rsync -axAX --omit-dir-times --no-D {exclude_args} /svc/ / && \\"
        )
        lines.append(
            "    for f in " + " ".join(_IDENTITY_MERGE_FILES) + "; do \\"
        )
        lines.append(
            "        awk -F: 'NR==FNR{seen[$1];next} !($1 in seen)' "
            "\"$f\" \"/svc$f\" >> \"$f\" 2>/dev/null || true; \\"
        )
        lines.append("    done")
        lines.append("")

    # Per-feature lines that must run once the feature's own files are
    # actually present on disk -- via the rsync just above (the only bucket
    # populated in this tree today) or a future Dockerfile.common.j2
    # include. Applied regardless of bucket (see _FEATURE_POST_COMMON_LINES_ANY
    # / finding #1).
    for spec in ordered:
        post_common = _FEATURE_POST_COMMON_LINES_ANY.get(spec.feature)
        if post_common:
            lines.extend(post_common)
            lines.append("")

    if "bgp" in selected_names:
        lines.append(
            "# --- bgp (FRR): user/group creation + ownership fix-up, "
            "required for FRR daemons' privs_init() (plan section 12). Run "
            "AFTER every rsync above so /etc/frr already exists (bgp's own "
            "rsynced image already ships it) and so this never races the "
            "identity-file merge step. Guarded so it's a no-op if the merge "
            "already brought 'frr' in from bgp's own /etc/passwd,/etc/group. ---"
        )
        lines.append(
            "RUN getent group frr >/dev/null 2>&1 || groupadd -g ${frr_user_gid} frr"
        )
        lines.append(
            "RUN id frr >/dev/null 2>&1 || "
            "useradd -u ${frr_user_uid} -g ${frr_user_gid} -M -s /bin/false frr"
        )
        lines.append("RUN mkdir -p /etc/frr && chown -R ${frr_user_uid}:${frr_user_gid} /etc/frr/")
        lines.append("")

    lines.append(
        "# --- mega's own generated files (supervisord_merge.py + "
        "gen_docker_init.py) ---"
    )
    lines.append(f'COPY ["{docker_init_result.orchestrator_filename}", "/usr/bin/"]')
    lines.append(f"RUN chmod 755 /usr/bin/{docker_init_result.orchestrator_filename}")
    lines.append('COPY ["supervisord.conf.j2", "/usr/share/sonic/templates/"]')
    lines.append('COPY ["critical_processes", "/etc/supervisor"]')
    lines.append("")

    lines.append(
        "# --- depstartup pending-start guard (plan section 12, always) ---"
    )
    lines.append(
        "# Prevents respawn storm: wraps supervisord_dependent_startup to"
    )
    lines.append(
        "# skip startProcess calls for processes already transitioning."
    )
    lines.append(f'COPY ["{DEPSTARTUP_GUARD_FILENAME}", "{DEPSTARTUP_GUARD_RUNTIME_PATH}"]')
    lines.append(f"RUN chmod 755 {DEPSTARTUP_GUARD_RUNTIME_PATH}")
    lines.append("")

    lines.append("FROM $BASE")
    lines.append("")
    lines.append("ARG image_version")
    lines.append("")
    lines.append("{{ rsync_from_builder_stage() }}")
    lines.append("")
    lines.append("ENV DEBIAN_FRONTEND=noninteractive")
    lines.append("ENV IMAGE_VERSION=$image_version")

    # Per-feature final-stage ENV vars (see _FEATURE_FINAL_STAGE_LINES).
    # Applied regardless of bucket -- ENV instructions are per-stage and
    # are NOT carried over by rsync_from_builder_stage() (it copies files,
    # not the Dockerfile ENV table), so this must be set explicitly here
    # even for rsync-bucket features whose built image already sets it.
    for spec in ordered:
        final = _FEATURE_FINAL_STAGE_LINES.get(spec.feature)
        if final:
            lines.extend(final)

    lines.append("")
    lines.append(f'ENTRYPOINT ["/usr/bin/{docker_init_result.orchestrator_filename}"]')
    lines.append("")

    dockerfile_text = "\n".join(lines)

    return DockerfileResult(
        dockerfile_text=dockerfile_text,
        base_stem=base_stem,
        common_bucket_features=[s.feature for s in common_bucket],
        rsync_bucket_features=[s.feature for s in rsync_bucket],
        start_sh_features=start_sh_features,
        extra_j2_by_feature=extra_j2_by_feature,
        docker_init=docker_init_result,
        depstartup_guard_text=generate_depstartup_guard(),
        warnings=warnings,
    )


# ---------------------------------------------------------------------------
# Standalone smoke test (not part of the pipeline wiring -- that's
# patch_templates.py's job, a separate to-do). Lets this module be exercised
# directly against a real sonic-buildimage tree:
#
#   python3 -m mega_gen.gen_dockerfile --sonic-root /path/to/sonic-buildimage \
#       --features database,swss,bgp,teamd,lldp,snmp,gnmi,radv,eventd,sysmgr \
#       --out-dir /tmp/mega-dockerfile
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
    ap.add_argument("--container-name", default="mega")
    ap.add_argument("--out-dir", type=Path, default=None)
    args = ap.parse_args()

    registry = discover_registry(args.sonic_root)
    resolved = resolve_features(args.features.split(","), registry)
    if resolved.unknown or resolved.excluded:
        print(f"ERROR: unknown={resolved.unknown} excluded={resolved.excluded}", file=sys.stderr)
        sys.exit(1)

    specs = _discover_features(args.sonic_root, resolved.resolved, registry)
    result = generate_dockerfile(specs, args.sonic_root, container_name=args.container_name)

    print(f"=== base_stem: {result.base_stem} ===")
    print(f"=== common_bucket_features: {result.common_bucket_features} ===")
    print(f"=== rsync_bucket_features: {result.rsync_bucket_features} ===")
    print(f"=== start_sh_features: {result.start_sh_features} ===")
    print(f"=== extra_j2_by_feature: {result.extra_j2_by_feature} ===")
    print(f"=== warnings ({len(result.warnings)}) ===")
    for w in result.warnings:
        print(f"  WARNING: {w}")

    if args.out_dir:
        args.out_dir.mkdir(parents=True, exist_ok=True)
        out_path = args.out_dir / MEGA_DOCKERFILE_NAME
        out_path.write_text(result.dockerfile_text)
        print(f"wrote {out_path}")
        assert result.docker_init is not None
        for ps in result.docker_init.preinit_scripts:
            p = args.out_dir / ps.generated_filename
            p.write_text(ps.content)
            print(f"wrote {p}")
        p = args.out_dir / result.docker_init.orchestrator_filename
        p.write_text(result.docker_init.orchestrator_text)
        print(f"wrote {p}")
        p = args.out_dir / DEPSTARTUP_GUARD_FILENAME
        p.write_text(result.depstartup_guard_text)
        print(f"wrote {p}")
    else:
        print(f"\n--- {MEGA_DOCKERFILE_NAME} (not written, pass --out-dir) ---")
        print(result.dockerfile_text)
