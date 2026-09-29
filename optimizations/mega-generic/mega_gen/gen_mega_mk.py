"""Generate `rules/docker-mega.mk` for a selected feature set.

Implements the `gen-docker-mega-mk` to-do from the
`generic_mega-container_merge_script` plan (section 3's module table and
section 6's "What `_INCLUDE_DOCKER` does NOT merge" bullets):

  1. `$(DOCKER_MEGA)_INCLUDE_DOCKER += $(DOCKER_<X>)` for **every** selected
     feature. `rules/functions`' `add_docker_feature`/`process_include_dockers`
     (invoked automatically by `slave.mk` for every docker in
     `SONIC_DOCKER_IMAGES`) already unions `_DEPENDS`/`_PYTHON_WHEELS`/
     `_FILES` and wires up `--build-context`/`-I` from this alone -- nothing
     else to do for those three.
  2. `_LOAD_DOCKERS`: the "heaviest common ancestor" base layer (the
     `*_SWSS_LAYER*` image if `swss` is selected, else the `*_CONFIG_ENGINE*`
     image any other feature already loads) **plus** one entry per selected
     feature that has no `Dockerfile.common.j2` (`FeatureSpec.dockerfile_common
     is None`) -- `gen_dockerfile.py` (a separate to-do) needs each such
     feature's own already-built image loaded so it can `FROM
     docker-<stem>-...:... AS <feature>-layer` and rsync its filesystem in
     (plan section 6, "hybrid Dockerfile strategy").

     NOTE (verified against this tree, per the sonic-docs-lookup rule): no
     `Dockerfile.common.j2` actually exists anywhere under `dockers/` here,
     and `docker-sonic-vs` -- the plan's cited precedent for a
     `.common.j2`-include "hybrid" split -- doesn't use one either; it's a
     hand-curated Dockerfile.j2 that only relies on `_INCLUDE_DOCKER` for
     dep/build-context union plus its own hardcoded `COPY` list. So in
     practice, *every* selected feature currently falls into the
     no-`Dockerfile.common.j2` bucket and gets its own image added to
     `_LOAD_DOCKERS` for rsync. This code still checks `dockerfile_common`
     generically (not hardcoded to "always rsync") so it automatically
     switches strategy for any future feature that does ship one.
  3. `_INSTALL_PYTHON_WHEELS` / `_INSTALL_DEBS`: unioned from whichever
     selected features define their own (most don't; `eventd` does).
  4. `_BASE_IMAGE_FILES`: unioned `name:dest` pairs from every selected
     feature's own `_BASE_IMAGE_FILES`, with the underlying host-side CLI
     wrapper / monit-config **file contents** retargeted so any literal
     reference to the feature's own container name (`docker exec bgp ...`,
     `memory_checker snmp ...`, `restart_service gnmi`) instead targets the
     merged container (`mega` by default).
  5. `_WARM_SHUTDOWN_BEFORE` / `_WARM_SHUTDOWN_AFTER` / `_FAST_SHUTDOWN_BEFORE`
     / `_FAST_SHUTDOWN_AFTER`: unioned from whichever selected features
     define their own, **filtering out self-references** -- e.g. `lldp`'s
     own `WARM_SHUTDOWN_BEFORE = swss` becomes meaningless once `lldp` and
     `swss` are both folded into the same mega container, so it's dropped;
     `swss`'s own `WARM_SHUTDOWN_BEFORE = syncd` still targets a container
     outside mega (`syncd` is always hard-excluded, see `auto_registry.py`)
     so it's kept. Best-effort: this generator could not find any actual
     runtime consumer of these fields anywhere in this tree (only the
     producer, `generate_manifest` in `rules/functions` -> `manifest.json.j2`
     -> a per-container `manifest.json`'s `warm-shutdown`/`fast-shutdown`
     keys) -- carried through for correctness/completeness, but flagged as
     unconfirmed rather than asserted as load-bearing.
  6. `_RUN_OPT` (the `docker create` flags Makefile variable, which feeds
     `docker_image_run_opt` -> `{{docker_image_run_opt}}` in
     `docker_image_ctl.j2`'s `docker create` line -- confirmed via
     `slave.mk`): unioned from every selected feature's own `_RUN_OPT`
     lines, collision-checked (see `merge_run_opt` below) rather than
     naively concatenated. This is deliberately the "plain Makefile
     variable" half of `docker create`'s flags -- **not** the ~15 more
     `{%- if docker_container_name == "swss" %}` -style blocks hardcoded
     directly into `docker_image_ctl.j2`'s template text (e.g. swss's
     `-e ASIC_VENDOR=...`, bgp's `-v /etc/sonic/frr/$DEV:/etc/frr:rw`,
     database's whole `$DB_OPT` lifecycle) -- those live in a file every
     container shares and require actual template surgery, which stays
     `ctl_parser.py`'s job (a separate to-do).

     Collision audit (verified against all 29 `docker-*.mk` files that
     define `_RUN_OPT`, not just the 10 proven ones -- see
     `merge_run_opt`'s docstring for the full policy):
       - Every shared `-v` bind-mount destination across the whole registry
         uses an identical source+mode *except one*: `p4rt` mounts
         `/etc/sonic` as `rw` while all 26 other features mount it `ro`.
         Per-project decision: on a mode conflict, `ro` always wins
         ("assume best practice") and only ONE `-v` line is ever emitted
         per destination -- never both.
       - Host-port publishing (`-p`/`--publish`, e.g. `restapi`'s `8081`,
         `sonic-redfish`'s `443`) is a genuinely scarce, unshareable
         resource: if two selected features ever claim the *same* host
         port, that's a hard, unresolvable conflict -- reported as an
         `ERROR:`-prefixed warning (per-project decision: assume the
         requested ports themselves are correct, just detect a collision
         loudly rather than silently letting one clobber the other).
       - Per-project decision, deliberately **not** handled (scope
         reduction, not oversight): `--privileged` gets no special
         treatment (unioned like any other opaque flag); `--device=`/
         `--mount` bind-mount-like flags are kept opaque (not
         cross-checked against `-v` destinations); overlapping-but-not-
         identical destinations (e.g. a directory mount vs. a file mount
         nested inside it) are not detected.

This module does **not** write anything to the real tree by itself (same
style as `supervisord_merge.py`) -- it returns a `MegaMkResult` with the
generated `.mk`/`.dep` text plus the retargeted `base_image_files` payloads,
and leaves the actual file writes to `patch_templates.py` /
`mega_gen_cli.py --apply` (separate to-dos), or to this module's own
`__main__` smoke-test block when explicitly asked via `--out-*` flags.

Explicitly **out of scope** here (owned by other to-dos, per plan section 6):
  - The `docker_image_ctl.j2` per-`docker_container_name` template blocks
    that add further `docker create` flags beyond `_RUN_OPT` -- `ctl_parser.py`'s
    job (see point 6 above).
  - `dockers/docker-mega/Dockerfile.j2` / `docker-mega-init.sh` /
    `supervisord.conf.j2` / `critical_processes` -- `gen_dockerfile.py` /
    `supervisord_merge.py`.
  - Debug (`_DBG`) image generation -- omitted entirely for this PoC (the
    plan never asks for it; see the project-purpose rule: "dirty is
    allowed", fastest path to a credible number over completeness).
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

from mega_gen.discovery import FeatureSpec

# ---------------------------------------------------------------------------
# Defaults for the generated container's identity.
# ---------------------------------------------------------------------------

MEGA_DOCKER_VAR_DEFAULT = "DOCKER_MEGA"
MEGA_STEM_DEFAULT = "docker-mega"
MEGA_CONTAINER_NAME_DEFAULT = "mega"
MEGA_VERSION_DEFAULT = "1.0.0"

# ---------------------------------------------------------------------------
# Generic .mk text scanning helpers.
#
# Same approach as `auto_registry.py` (see its module docstring): every fact
# is read straight out of the feature's own `.mk` file text at generation
# time, not hand-maintained -- so this keeps working for any feature the
# open registry accepts, not just the 10 proven ones.
# ---------------------------------------------------------------------------

_DOCKER_TOKEN_RE = re.compile(r"\$\((DOCKER_[A-Z0-9_]+)\)")

# A `name:/dest/path` token inside a `_BASE_IMAGE_FILES` RHS, e.g.
# `vtysh:/usr/bin/vtysh` or `monit_snmp:/etc/monit/conf.d`.
_BASE_IMAGE_FILE_ENTRY_RE = re.compile(r"^([^:\s]+):(\S+)$")


def _own_suffix_rhs_lines(text: str, docker_var: str, suffix: str) -> List[str]:
    """Every RHS of a `$(<docker_var>)<suffix> [+:]= <rhs>` line in `text` --
    i.e. lines where THIS feature's own docker var defines `<suffix>`
    directly (not a nested read of some other docker's field via
    `$($(DOCKER_X)_SUFFIX)`, which would never match `^\\s*\\$\\(<docker_var>\\)`
    at line start the way a real assignment does)."""
    pat = re.compile(
        rf"^\s*\$\({re.escape(docker_var)}\){re.escape(suffix)}\s*[+:]?=\s*(.+?)\s*$",
        re.MULTILINE,
    )
    return [m.group(1) for m in pat.finditer(text)]


def load_docker_tokens_of(text: str, docker_var: str) -> List[str]:
    """Docker-var tokens (e.g. `DOCKER_SWSS_LAYER_TRIXIE`) this feature's own
    `_LOAD_DOCKERS` line(s) reference, in file order, deduplicated. Also
    picks up tokens embedded in a nested reference like sysmgr's
    `$($(DOCKER_CONFIG_ENGINE_TRIXIE)_LOAD_DOCKERS)` (the regex finds the
    innermost `$(DOCKER_X)` regardless of what wraps it)."""
    tokens: List[str] = []
    for rhs in _own_suffix_rhs_lines(text, docker_var, "_LOAD_DOCKERS"):
        for m in _DOCKER_TOKEN_RE.finditer(rhs):
            tok = m.group(1)
            if tok not in tokens:
                tokens.append(tok)
    return tokens


def raw_tokens_of(text: str, docker_var: str, suffix: str) -> List[str]:
    """Whitespace-split RHS tokens (e.g. `$(SONIC_UTILITIES_PY3)`) from every
    `_<suffix>` line this feature's own docker var defines, in file order,
    deduplicated. Used for `_INSTALL_PYTHON_WHEELS` / `_INSTALL_DEBS`."""
    out: List[str] = []
    for rhs in _own_suffix_rhs_lines(text, docker_var, suffix):
        for tok in rhs.split():
            if tok not in out:
                out.append(tok)
    return out


def base_image_file_pairs_of(text: str, docker_var: str) -> List[Tuple[str, str]]:
    """`(name, dest)` pairs from every `_BASE_IMAGE_FILES` line this
    feature's own docker var defines, in file order."""
    pairs: List[Tuple[str, str]] = []
    for rhs in _own_suffix_rhs_lines(text, docker_var, "_BASE_IMAGE_FILES"):
        for tok in rhs.split():
            m = _BASE_IMAGE_FILE_ENTRY_RE.match(tok)
            if m:
                pairs.append((m.group(1), m.group(2)))
    return pairs


# _RUN_OPT tokenization: most docker-create flags are self-contained single
# whitespace tokens (`-t`, `--cap-add=X`, `--pid=host`, `--device=SRC:DST:MODE`),
# but several are written as TWO separate space-delimited tokens in the
# source: `-v SRC:DST:MODE` (the dominant bind-mount form -- every single
# occurrence in the tree uses a space, never `-v=...`), `--security-opt
# VALUE`, `--mount VALUE`, and `-p`/`--publish` when NOT written with `=`
# (e.g. `-p 0.0.0.0:443:18080` vs `-p=8081:8081/tcp`, both forms seen in the
# tree) -- verified against every `_RUN_OPT` line. Treat the named ones as
# atomic (flag, value) units so dedup/collision-checking never separates a
# flag from its value.
_TWO_TOKEN_RUN_OPT_FLAG_NAMES: Tuple[str, ...] = ("-v", "--volume", "--security-opt", "--mount", "-p", "--publish")


def _tokenize_run_opt_rhs(rhs: str) -> List[str]:
    """Split one `_RUN_OPT` line's RHS into atomic flag units (see comment
    above) -- NOT deduplicated, caller unions across features."""
    raw_tokens = rhs.split()
    units: List[str] = []
    i = 0
    while i < len(raw_tokens):
        tok = raw_tokens[i]
        if tok in _TWO_TOKEN_RUN_OPT_FLAG_NAMES and i + 1 < len(raw_tokens):
            units.append(f"{tok} {raw_tokens[i + 1]}")
            i += 2
        else:
            units.append(tok)
            i += 1
    return units


def run_opt_units_of(text: str, docker_var: str) -> List[str]:
    """Every `_RUN_OPT` flag unit this feature's own docker var defines, in
    file order, NOT deduplicated (`merge_run_opt` does the cross-feature
    union/collision-checking).

    KNOWN LIMITATION (shared by every other scanner in this module): this is
    a flat per-line regex scan with no `ifeq`/`endif` tracking, so a
    `_RUN_OPT` line wrapped in e.g. `ifeq ($(ENABLE_ASAN), y)` (several
    features' `--cap-add=SYS_PTRACE`) is picked up unconditionally,
    regardless of that build flag's actual value. This doesn't corrupt the
    *merge* (the flag still just dedupes against any other contributor
    requesting the same value), but it means mega may end up with a flag
    some contributor only wanted conditionally. Same simplification
    `auto_registry.py` explicitly avoids for install-line detection but
    every other suffix-scanner here (`_LOAD_DOCKERS`, `_INSTALL_*`, the
    shutdown-order fields) already makes -- consistent, not new."""
    units: List[str] = []
    for rhs in _own_suffix_rhs_lines(text, docker_var, "_RUN_OPT"):
        units.extend(_tokenize_run_opt_rhs(rhs))
    return units


# `-v SRC:DST[:MODE]` bind mounts (the dominant docker-create bind-mount
# syntax in this tree -- `--device=`/`--mount` are deliberately left opaque,
# see module docstring point 6).
_BIND_MOUNT_RE = re.compile(r"^(?:-v|--volume)\s+(?P<src>[^:]+):(?P<dst>[^:]+)(?::(?P<mode>[\w,]+))?$")

# `-p`/`--publish VALUE` (either token form -- see `_tokenize_run_opt_rhs`).
# VALUE is `[host_ip:]host_port:container_port[/proto]`; host_port is always
# the second-to-last colon-separated part.
_PORT_UNIT_RE = re.compile(r"^(?:-p|--publish)(?:=|\s+)(?P<value>\S+)$")


def _parse_bind_mount(unit: str) -> Optional[Tuple[str, str, str]]:
    """`(dest, source, mode)` if `unit` is a `-v SRC:DST[:MODE]` flag
    (`mode` is `""` if omitted), else None."""
    m = _BIND_MOUNT_RE.match(unit)
    if not m:
        return None
    return m.group("dst"), m.group("src"), (m.group("mode") or "")


def _parse_port_publish(unit: str) -> Optional[str]:
    """Host-side port number if `unit` is a `-p`/`--publish` flag, else
    None."""
    m = _PORT_UNIT_RE.match(unit)
    if not m:
        return None
    parts = m.group("value").split(":")
    if len(parts) >= 2:
        return parts[-2]
    return None


@dataclass
class RunOptMergeResult:
    units: List[str]
    """Final, deduplicated, collision-resolved `_RUN_OPT` flag unit list, in
    emission order (opaque flags, then resolved bind mounts, then port
    publishes)."""

    warnings: List[str] = field(default_factory=list)


def merge_run_opt(specs_units: Dict[str, List[str]]) -> RunOptMergeResult:
    """Merge `{feature: [unit, ...]}` (see `run_opt_units_of`, one entry per
    selected feature, each already in that feature's own file order) into
    one `_RUN_OPT` unit list for mega. Three unit classes, each merged
    differently (see the module docstring's point 6 for the full policy and
    the collision audit that justifies it):

    - **`-v SRC:DST[:MODE]` bind mounts**, grouped by `DST`:
        - identical `(SRC, MODE)` everywhere -> included once.
        - same `SRC`, different `MODE` (`ro` vs `rw`) -> best-practice
          policy: `ro` always wins if requested by *any* contributor
          (this is exactly the real `p4rt` (`rw`) vs. 26-other-features
          (`ro`) `/etc/sonic` case) -- only ONE `-v` line is ever emitted
          for that destination, never both; a warning names the losing
          feature(s).
        - different `SRC` for the same `DST` (no current case in this
          tree, handled defensively) -> first-seen contributor wins, loud
          warning naming everyone dropped (no automatic resolution is
          possible when the sources themselves differ).
    - **`-p`/`--publish VALUE` port publishes**, grouped by host port:
        more than one *feature* claiming the same host port -> hard,
        unmistakable `ERROR:`-prefixed warning (ports are a genuinely
        scarce, unshareable resource -- unlike everything else here, this
        is not auto-resolvable, just flagged). Distinct ports are assumed
        correct and kept as-is.
    - **everything else** (`-t`, `--cap-add=X`, `--security-opt VALUE`,
      `--privileged`, any other opaque unit): plain exact-string dedup,
      first-seen order across features. `--privileged` gets no special
      treatment by design (per-project scope decision, not an oversight).
    """
    warnings: List[str] = []
    opaque_seen: List[str] = []
    mounts: Dict[str, List[Tuple[str, str, str]]] = {}  # dst -> [(src, mode, feature)]
    ports: Dict[str, List[Tuple[str, str]]] = {}  # host_port -> [(feature, unit)]

    for feature, units in specs_units.items():
        for unit in units:
            mount = _parse_bind_mount(unit)
            if mount is not None:
                dst, src, mode = mount
                mounts.setdefault(dst, []).append((src, mode, feature))
                continue
            port = _parse_port_publish(unit)
            if port is not None:
                ports.setdefault(port, []).append((feature, unit))
                continue
            if unit not in opaque_seen:
                opaque_seen.append(unit)

    mount_units: List[str] = []
    for dst, contributors in mounts.items():
        sources = list(dict.fromkeys(src for src, _mode, _feat in contributors))
        if len(sources) > 1:
            winner_src = contributors[0][0]
            winner_feat = contributors[0][2]
            warnings.append(
                f"_RUN_OPT bind-mount collision at destination {dst!r}: "
                f"different SOURCE paths requested across features -- "
                f"{[(f, s) for s, _m, f in contributors]} -- keeping "
                f"{winner_feat!r}'s source {winner_src!r}, dropping the "
                f"rest (no automatic resolution possible when sources "
                f"themselves differ)"
            )
            chosen = [c for c in contributors if c[0] == winner_src]
        else:
            chosen = contributors
            winner_src = sources[0]

        modes = list(dict.fromkeys(m for _s, m, _f in chosen))
        if len(modes) > 1:
            winner_mode = "ro" if "ro" in modes else chosen[0][1]
            losers = sorted({f for _s, m, f in chosen if m != winner_mode})
            warnings.append(
                f"_RUN_OPT bind-mount mode conflict at destination {dst!r} "
                f"(source {winner_src!r}): modes {sorted(modes)} requested "
                f"across features -- defaulting to the more restrictive "
                f"{winner_mode!r} (best practice) and emitting only ONE "
                f"-v line, not copying both; {losers} requested a "
                f"different mode and were overridden"
            )
        else:
            winner_mode = modes[0]

        mount_units.append(f"-v {winner_src}:{dst}" + (f":{winner_mode}" if winner_mode else ""))

    port_units: List[str] = []
    for port, contributors in ports.items():
        contributing_features = sorted({f for f, _u in contributors})
        if len(contributing_features) > 1:
            warnings.append(
                f"ERROR: host port {port!r} requested by more than one "
                f"selected feature ({contributing_features}) -- only one "
                f"container can bind a given host port; resolve by "
                f"deselecting one of them or changing the port in its "
                f"source .mk before building"
            )
        for _f, unit in contributors:
            if unit not in port_units:
                port_units.append(unit)

    return RunOptMergeResult(units=opaque_seen + mount_units + port_units, warnings=warnings)


# The four warm/fast-reboot shutdown-ordering fields (see `generate_manifest`
# in `rules/functions` and `manifest.json.j2`'s `warm-shutdown`/
# `fast-shutdown` keys). Values are container names (e.g. `syncd`, `swss`),
# space-separated if more than one -- `manifest.json.j2` calls `.split()` on
# them, so accumulating with `+=` across contributing features is safe.
_SHUTDOWN_ORDER_SUFFIXES: Tuple[str, ...] = (
    "_WARM_SHUTDOWN_BEFORE",
    "_WARM_SHUTDOWN_AFTER",
    "_FAST_SHUTDOWN_BEFORE",
    "_FAST_SHUTDOWN_AFTER",
)


def distro_docker_lists_of(text: str, docker_var: str) -> List[str]:
    """`SONIC_<DISTRO>_DOCKERS` list name(s) (e.g. `SONIC_TRIXIE_DOCKERS`)
    this feature's own docker var is unconditionally appended to in `text`.
    Used to generically detect which distro-suffixed image list mega must
    join too, instead of hardcoding a distro name (BLDENV varies across
    trees/builds -- see the `sonic-code-changes-on-vm` rule)."""
    pat = re.compile(
        rf"^SONIC_([A-Z0-9]+)_DOCKERS\s*\+=\s*\$\({re.escape(docker_var)}\)\s*$",
        re.MULTILINE,
    )
    return [f"SONIC_{m.group(1)}_DOCKERS" for m in pat.finditer(text)]


# ---------------------------------------------------------------------------
# base_image_files content retargeting: rewrite literal `<old_container>`
# references to point at the merged container instead.
#
# Idioms below were verified directly against the tree (per the
# sonic-docs-lookup rule) across every proven feature's base_image_files/:
#   - `docker exec [-<flags>] <name>[$DEV| ]...`      (vtysh, rvtysh's callee
#     vtysh itself needs no rewrite, redis-cli, teamdctl, lldpctl/lldpcli,
#     swssloglevel, TS/TSA/TSB's chassis rexec path)
#   - `/usr/bin/memory_checker <name> <bytes>`         (monit_<feature> watchdogs)
#   - `/usr/bin/restart_service <name>`                (monit_<feature> auto-restart)
# Anything else that still contains the old name as a whole word after these
# substitutions is left untouched but reported as a warning -- per
# sonic-docs-lookup, flag uncertainty rather than guess (e.g. a monit
# `check program container_memory_<feature>` identifier is intentionally
# NOT renamed here since it's just a label, not a container reference; it
# may still show up as a false-positive warning line for a human to confirm).
# ---------------------------------------------------------------------------

_RETARGET_COMMANDS: Tuple[str, ...] = ("docker exec", "memory_checker", "restart_service")


def retarget_container_refs(text: str, old_name: str, new_name: str) -> Tuple[str, List[str]]:
    """Rewrite literal `old_name` container references in `text` (a
    base_image_files script/config's raw content) to `new_name`. Returns
    `(new_text, warnings)`; `new_text is text` (no-op) is fine and expected
    for e.g. `rvtysh`, which never mentions a container directly."""
    if old_name == new_name:
        return text, []

    new_text = text
    for cmd in _RETARGET_COMMANDS:
        pat = re.compile(rf"({re.escape(cmd)}\s+(?:-\S+\s+)*){re.escape(old_name)}\b")
        new_text = pat.sub(rf"\g<1>{new_name}", new_text)

    warnings: List[str] = []
    leftover_re = re.compile(rf"\b{re.escape(old_name)}\b")
    for lineno, line in enumerate(new_text.splitlines(), 1):
        if leftover_re.search(line):
            warnings.append(
                f"still contains literal '{old_name}' after retargeting the "
                f"known idioms ({', '.join(_RETARGET_COMMANDS)}), line "
                f"{lineno}: {line.strip()!r} -- verify manually (may be an "
                f"unrelated identifier, e.g. a monit label, or a container "
                f"reference this generator doesn't know how to rewrite yet)"
            )
    return new_text, warnings


# ---------------------------------------------------------------------------
# Output types.
# ---------------------------------------------------------------------------


@dataclass
class BaseImageFileOutput:
    name: str
    """Source basename under `base_image_files/`, e.g. `vtysh`."""

    dest: str
    """Destination path (or directory) inside the base image, e.g.
    `/usr/bin/vtysh` or `/etc/monit/conf.d`."""

    feature: str
    """Owning feature (whose own `.mk` declared this `_BASE_IMAGE_FILES`
    entry) -- the one whose copy survives a name collision."""

    source_path: Optional[Path]
    """Absolute path to the real source file, or None if it couldn't be
    resolved (missing docker dir)."""

    content: Optional[str]
    """Retargeted file content, ready to write to
    `dockers/docker-mega/base_image_files/<name>`. None if `source_path`
    doesn't exist -- caller must not silently skip this (see `warnings` on
    the parent `MegaMkResult`)."""

    retargeted: bool = False
    """True if `content != ` the original source text (i.e. a container
    reference was actually rewritten)."""

    warnings: List[str] = field(default_factory=list)


@dataclass
class MegaMkResult:
    mk_text: str
    """Full generated `rules/docker-mega.mk` body."""

    dep_text: str
    """Companion `rules/docker-mega.dep` body (only consulted by the build
    when DPKG caching is enabled -- see `Makefile.cache`'s `dpkg_depend`)."""

    base_image_files: List[BaseImageFileOutput] = field(default_factory=list)
    """Physical files to place under `dockers/docker-mega/base_image_files/`
    -- not written by this module itself."""

    include_docker_features: List[str] = field(default_factory=list)
    """Every feature emitted as an `_INCLUDE_DOCKER +=` line, in order."""

    base_load_docker: Optional[str] = None
    """The chosen base-layer docker-var token (e.g.
    `DOCKER_SWSS_LAYER_TRIXIE` or `DOCKER_CONFIG_ENGINE_TRIXIE`), or None if
    undeterminable (see `warnings`)."""

    rsync_load_docker_features: List[str] = field(default_factory=list)
    """Selected features with no `Dockerfile.common.j2` -- these also get
    their own docker-var token added to `_LOAD_DOCKERS` so `gen_dockerfile.py`
    can FROM their built image as an rsync source stage."""

    load_dockers: List[str] = field(default_factory=list)
    """Final, deduplicated `_LOAD_DOCKERS` token list in emit order (base
    layer first, then the rsync-strategy features' own images)."""

    install_python_wheels: List[str] = field(default_factory=list)
    install_debs: List[str] = field(default_factory=list)

    distro_docker_list: Optional[str] = None
    """The `SONIC_<DISTRO>_DOCKERS` list mega was joined to (e.g.
    `SONIC_TRIXIE_DOCKERS`), or None if undeterminable."""

    shutdown_order: Dict[str, List[str]] = field(default_factory=dict)
    """`{"_WARM_SHUTDOWN_BEFORE": ["syncd"], ...}` -- final, deduplicated,
    self-reference-filtered values per suffix (only non-empty suffixes are
    present). See `_SHUTDOWN_ORDER_SUFFIXES`."""

    run_opt: List[str] = field(default_factory=list)
    """Final, deduplicated, collision-resolved `_RUN_OPT` flag units. See
    `merge_run_opt`."""

    warnings: List[str] = field(default_factory=list)


# ---------------------------------------------------------------------------
# Main entry point.
# ---------------------------------------------------------------------------


def generate_mega_mk(
    specs: Sequence[FeatureSpec],
    sonic_root: Path,
    container_name: str = MEGA_CONTAINER_NAME_DEFAULT,
    docker_var: str = MEGA_DOCKER_VAR_DEFAULT,
    stem: str = MEGA_STEM_DEFAULT,
    version: str = MEGA_VERSION_DEFAULT,
) -> MegaMkResult:
    """Generate `rules/docker-mega.mk` (+ its `.dep` companion) for the
    already-discovered `specs` (see `mega_gen.discovery.discover_features`).
    Read-only against `sonic_root` (only reads each feature's own `.mk` text
    and `base_image_files/*`) -- writes nothing itself."""
    warnings: List[str] = []
    mk_texts: Dict[str, str] = {}

    def text_of(spec: FeatureSpec) -> str:
        if spec.feature not in mk_texts:
            path = sonic_root / spec.mk_file
            mk_texts[spec.feature] = path.read_text(errors="replace")
        return mk_texts[spec.feature]

    include_docker_features: List[str] = [s.feature for s in specs]
    swss_layer_by_feature: Dict[str, str] = {}
    config_engine_by_feature: Dict[str, str] = {}
    other_own_load_tokens: List[str] = []
    rsync_load_docker_features: List[str] = []
    install_python_wheels: List[str] = []
    install_debs: List[str] = []
    base_image_entries: List[Tuple[str, str, FeatureSpec]] = []
    distro_lists_seen: Dict[str, int] = {}
    shutdown_order_raw: Dict[str, List[Tuple[str, str]]] = {
        suffix: [] for suffix in _SHUTDOWN_ORDER_SUFFIXES
    }
    run_opt_units_by_feature: Dict[str, List[str]] = {}

    for spec in specs:
        entry = spec.registry_entry
        text = text_of(spec)

        for tok in load_docker_tokens_of(text, entry.docker_var):
            if "SWSS_LAYER" in tok:
                swss_layer_by_feature[spec.feature] = tok
            elif "CONFIG_ENGINE" in tok:
                config_engine_by_feature[spec.feature] = tok
            elif tok not in other_own_load_tokens:
                other_own_load_tokens.append(tok)

        if spec.dockerfile_common is None:
            rsync_load_docker_features.append(spec.feature)

        for tok in raw_tokens_of(text, entry.docker_var, "_INSTALL_PYTHON_WHEELS"):
            if tok not in install_python_wheels:
                install_python_wheels.append(tok)
        for tok in raw_tokens_of(text, entry.docker_var, "_INSTALL_DEBS"):
            if tok not in install_debs:
                install_debs.append(tok)

        for name, dest in base_image_file_pairs_of(text, entry.docker_var):
            base_image_entries.append((name, dest, spec))

        for dl in distro_docker_lists_of(text, entry.docker_var):
            distro_lists_seen[dl] = distro_lists_seen.get(dl, 0) + 1

        for suffix in _SHUTDOWN_ORDER_SUFFIXES:
            for tok in raw_tokens_of(text, entry.docker_var, suffix):
                shutdown_order_raw[suffix].append((tok, spec.feature))

        run_opt_units_by_feature[spec.feature] = run_opt_units_of(text, entry.docker_var)

    # --- base layer selection: swss-layer if swss selected, else config-engine ---
    base_load_docker: Optional[str] = None
    selected_feature_names = {s.feature for s in specs}
    if "swss" in selected_feature_names:
        if swss_layer_by_feature:
            values = sorted(set(swss_layer_by_feature.values()))
            base_load_docker = values[0]
            if len(values) > 1:
                warnings.append(
                    f"multiple distinct swss-layer base images referenced "
                    f"across selected features ({swss_layer_by_feature}) -- "
                    f"using {base_load_docker}"
                )
        elif config_engine_by_feature:
            values = sorted(set(config_engine_by_feature.values()))
            base_load_docker = values[0]
            warnings.append(
                f"'swss' selected but its own .mk declares no *_SWSS_LAYER* "
                f"_LOAD_DOCKERS -- falling back to the config-engine base "
                f"{base_load_docker} referenced by {sorted(config_engine_by_feature)}"
            )
        else:
            warnings.append(
                "'swss' selected but no swss-layer/config-engine base image "
                "could be determined from any selected feature's own "
                "_LOAD_DOCKERS -- rules/docker-mega.mk will have no base "
                "_LOAD_DOCKERS at all; add one manually"
            )
    elif config_engine_by_feature:
        values = sorted(set(config_engine_by_feature.values()))
        base_load_docker = values[0]
        if len(values) > 1:
            warnings.append(
                f"multiple distinct config-engine base images referenced "
                f"across selected features ({config_engine_by_feature}) -- "
                f"using {base_load_docker}"
            )
    else:
        warnings.append(
            "no config-engine (or swss-layer) base image could be determined "
            "from any selected feature's own _LOAD_DOCKERS -- rules/"
            "docker-mega.mk will have no base _LOAD_DOCKERS at all; add one "
            "manually"
        )

    # --- final _LOAD_DOCKERS: base layer + rsync-strategy features' own images ---
    load_dockers: List[str] = []
    if base_load_docker:
        load_dockers.append(base_load_docker)
    specs_by_feature = {s.feature: s for s in specs}
    for feat in rsync_load_docker_features:
        tok = specs_by_feature[feat].registry_entry.docker_var
        if tok not in load_dockers:
            load_dockers.append(tok)
    for tok in other_own_load_tokens:
        if tok not in load_dockers:
            load_dockers.append(tok)
            warnings.append(
                f"a selected feature's own _LOAD_DOCKERS references {tok}, "
                f"which is neither a *_SWSS_LAYER* nor *_CONFIG_ENGINE* base "
                f"image -- carried through into mega's own _LOAD_DOCKERS "
                f"verbatim; verify it's still needed/correct for mega"
            )

    # --- SONIC_<DISTRO>_DOCKERS: majority vote across selected features ---
    distro_docker_list: Optional[str] = None
    if distro_lists_seen:
        distro_docker_list = max(distro_lists_seen, key=lambda k: distro_lists_seen[k])
        if len(distro_lists_seen) > 1:
            warnings.append(
                f"selected features disagree on their SONIC_<DISTRO>_DOCKERS "
                f"list ({distro_lists_seen}) -- mega joins the majority one, "
                f"{distro_docker_list}; this usually means mixed-distro "
                f"features were selected, which one mega base image cannot "
                f"actually satisfy for the minority feature(s)"
            )
    else:
        warnings.append(
            "could not determine a SONIC_<DISTRO>_DOCKERS list from any "
            "selected feature's own .mk -- rules/docker-mega.mk will not "
            "join one; add `SONIC_<DISTRO>_DOCKERS += $(DOCKER_MEGA)` "
            "manually"
        )

    # --- _RUN_OPT: merge with collision resolution (see merge_run_opt) ---
    run_opt_result = merge_run_opt(run_opt_units_by_feature)
    warnings.extend(run_opt_result.warnings)

    # --- shutdown ordering: keep only references outside the selected set ---
    selected_container_names = {s.container_name for s in specs} | {container_name}
    shutdown_order: Dict[str, List[str]] = {}
    for suffix, raw_values in shutdown_order_raw.items():
        kept: List[str] = []
        for value, feat in raw_values:
            if value in selected_container_names:
                warnings.append(
                    f"{feat}'s own {suffix}={value!r} became a self-reference "
                    f"once merged into '{container_name}' (both are now the "
                    f"same container) -- dropped"
                )
                continue
            if value not in kept:
                kept.append(value)
        if kept:
            shutdown_order[suffix] = kept
            warnings.append(
                f"carrying {suffix}={' '.join(kept)!r} through to "
                f"'{container_name}' -- this generator could not find any "
                f"actual consumer of this field anywhere in the tree (only "
                f"the manifest.json producer in rules/functions), so treat "
                f"this as best-effort/unconfirmed, not a verified fix"
            )

    # --- _BASE_IMAGE_FILES: collision detection + content retargeting ---
    base_image_outputs: List[BaseImageFileOutput] = []
    seen_names: Dict[str, str] = {}
    mk_base_image_lines: List[str] = []
    for name, dest, spec in base_image_entries:
        if name in seen_names:
            warnings.append(
                f"_BASE_IMAGE_FILES name collision: '{name}' claimed by both "
                f"{seen_names[name]!r} and {spec.feature!r} -- keeping "
                f"{seen_names[name]!r}'s copy, dropping {spec.feature!r}'s "
                f"(source would have been "
                f"{spec.docker_dir}/base_image_files/{name})"
            )
            continue
        seen_names[name] = spec.feature

        source_path = (
            (spec.docker_dir / "base_image_files" / name) if spec.docker_dir else None
        )
        content: Optional[str] = None
        retargeted = False
        if source_path is not None and source_path.is_file():
            raw = source_path.read_text(errors="replace")
            content, retarget_warnings = retarget_container_refs(
                raw, spec.container_name, container_name
            )
            retargeted = content != raw
            for w in retarget_warnings:
                warnings.append(f"base_image_files/{name} ({spec.feature}): {w}")
        else:
            retarget_warnings = []
            warnings.append(
                f"_BASE_IMAGE_FILES entry '{name}:{dest}' declared by "
                f"{spec.feature!r} but no source file found at "
                f"{source_path} -- entry still emitted into "
                f"rules/docker-mega.mk (build will fail until the file "
                f"exists under dockers/docker-mega/base_image_files/)"
            )

        base_image_outputs.append(
            BaseImageFileOutput(
                name=name,
                dest=dest,
                feature=spec.feature,
                source_path=source_path,
                content=content,
                retargeted=retargeted,
                warnings=retarget_warnings,
            )
        )
        mk_base_image_lines.append(f"{name}:{dest}")

    # --- emit rules/docker-mega.mk -----------------------------------------
    lines: List[str] = [
        f"# docker image for {container_name} (auto-generated by",
        "# optimizations/mega-generic/mega_gen_cli.py --apply -- DO NOT EDIT BY HAND)",
        f"# Composed from: {', '.join(include_docker_features)}",
        "# See optimizations/mega-generic/README.md and the",
        "# generic_mega-container_merge_script plan for the full design.",
        "",
        f"{docker_var}_STEM = {stem}",
        f"{docker_var} = $({docker_var}_STEM).gz",
        "",
        f"$({docker_var})_PATH = $(DOCKERS_PATH)/$({docker_var}_STEM)",
        "",
        "# --- _INCLUDE_DOCKER: dep union (_DEPENDS/_PYTHON_WHEELS/_FILES) + build",
        "# contexts for every selected feature, handled automatically by",
        "# rules/functions' add_docker_feature/process_include_dockers (plan section 1).",
    ]
    for spec in specs:
        lines.append(f"$({docker_var})_INCLUDE_DOCKER += $({spec.registry_entry.docker_var})")
    lines.append("")

    if load_dockers:
        lines.append("# --- _LOAD_DOCKERS: base layer" +
                      (" + per-feature rsync source stage images" if rsync_load_docker_features else ""))
        if rsync_load_docker_features:
            lines.append(
                f"# ({', '.join(rsync_load_docker_features)} -- none of the selected"
            )
            lines.append(
                "# features have a Dockerfile.common.j2 in this tree, verified at"
            )
            lines.append(
                "# generation time, so all of them use the rsync-from-built-image"
            )
            lines.append(
                "# strategy here, not just a subset; see gen_mega_mk.py's module"
            )
            lines.append(
                "# docstring). gen_dockerfile.py FROMs each as a build stage and"
            )
            lines.append("# rsyncs its filesystem in (plan section 6).")
        for tok in load_dockers:
            lines.append(f"$({docker_var})_LOAD_DOCKERS += $({tok})")
        lines.append("")

    if install_python_wheels:
        lines.append("# --- union of selected features' own _INSTALL_PYTHON_WHEELS ---")
        lines.append(f"$({docker_var})_INSTALL_PYTHON_WHEELS += {' '.join(install_python_wheels)}")
        lines.append("")

    if install_debs:
        lines.append("# --- union of selected features' own _INSTALL_DEBS ---")
        lines.append(f"$({docker_var})_INSTALL_DEBS += {' '.join(install_debs)}")
        lines.append("")

    lines.append(f"$({docker_var})_VERSION = {version}")
    lines.append(f"$({docker_var})_PACKAGE_NAME = {container_name}")
    lines.append("")
    lines.append(f"SONIC_DOCKER_IMAGES += $({docker_var})")
    if distro_docker_list:
        lines.append(f"{distro_docker_list} += $({docker_var})")
    lines.append(f"SONIC_INSTALL_DOCKER_IMAGES += $({docker_var})")
    lines.append("")
    lines.append(f"$({docker_var})_CONTAINER_NAME = {container_name}")
    lines.append("")

    if run_opt_result.units:
        lines.append(
            "# --- _RUN_OPT: union of selected features' own _RUN_OPT, collision-resolved"
        )
        lines.append(
            "# (mode conflicts on a shared -v destination default to the more restrictive"
        )
        lines.append(
            "# 'ro'; colliding host ports are flagged, not silently merged -- see"
        )
        lines.append("# mega_gen.gen_mega_mk.merge_run_opt). NOTE: this covers only the")
        lines.append(
            "# Makefile-variable half of docker create's flags -- the per-container-name"
        )
        lines.append(
            "# {%- if docker_container_name == \"X\" %} blocks hardcoded directly into"
        )
        lines.append(
            "# docker_image_ctl.j2 (e.g. swss's -e ASIC_VENDOR=..., bgp's frr mount,"
        )
        lines.append(
            "# database's $DB_OPT) still need a new 'mega' branch from ctl_parser.py."
        )
        for unit in run_opt_result.units:
            lines.append(f"$({docker_var})_RUN_OPT += {unit}")
        lines.append("")

    if shutdown_order:
        lines.append(
            "# --- warm/fast-reboot shutdown ordering: unioned from selected features'"
        )
        lines.append(
            "# own fields, filtering out now-self-referential entries (both endpoints"
        )
        lines.append(
            "# merged into this container). Best-effort: no confirmed consumer of"
        )
        lines.append(
            "# these fields was found anywhere in this tree; see gen_mega_mk.py."
        )
        for suffix, values in shutdown_order.items():
            lines.append(f"$({docker_var}){suffix} += {' '.join(values)}")
        lines.append("")

    if mk_base_image_lines:
        lines.append(
            "# --- _BASE_IMAGE_FILES: union of selected features' host CLI wrappers /"
        )
        lines.append(
            f"# monit configs, retargeted from their own container name to '{container_name}'"
        )
        lines.append(
            "# (mega_gen.gen_mega_mk.retarget_container_refs). Physical files live under"
        )
        lines.append(f"# $({docker_var})_PATH/base_image_files/.")
        for entry_line in mk_base_image_lines:
            lines.append(f"$({docker_var})_BASE_IMAGE_FILES += {entry_line}")
        lines.append("")

    mk_text = "\n".join(lines).rstrip("\n") + "\n"

    dep_lines = [f"DPATH       := $($({docker_var})_PATH)"]
    dep_lines += [
        f"DEP_FILES   := $(SONIC_COMMON_FILES_LIST) rules/{stem}.mk rules/{stem}.dep",
        "DEP_FILES   += $(SONIC_COMMON_BASE_FILES_LIST)",
        "DEP_FILES   += $(shell git ls-files $(DPATH))",
        "",
        f"$({docker_var})_CACHE_MODE  := GIT_CONTENT_SHA",
        f"$({docker_var})_DEP_FLAGS   := $(SONIC_COMMON_FLAGS_LIST)",
        f"$({docker_var})_DEP_FILES   := $(DEP_FILES)",
    ]
    dep_text = "\n".join(dep_lines) + "\n"

    return MegaMkResult(
        mk_text=mk_text,
        dep_text=dep_text,
        base_image_files=base_image_outputs,
        include_docker_features=include_docker_features,
        base_load_docker=base_load_docker,
        rsync_load_docker_features=rsync_load_docker_features,
        load_dockers=load_dockers,
        install_python_wheels=install_python_wheels,
        install_debs=install_debs,
        distro_docker_list=distro_docker_list,
        shutdown_order=shutdown_order,
        run_opt=run_opt_result.units,
        warnings=warnings,
    )


# ---------------------------------------------------------------------------
# Standalone smoke test (not part of the pipeline wiring -- that's
# patch_templates.py's job, a separate to-do). Lets this module be exercised
# directly against a real sonic-buildimage tree:
#
#   python3 -m mega_gen.gen_mega_mk --sonic-root /path/to/sonic-buildimage \
#       --features database,swss,bgp,teamd,lldp,snmp,gnmi,radv,eventd,sysmgr \
#       --out-mk /tmp/docker-mega.mk --out-dep /tmp/docker-mega.dep \
#       --out-base-image-files-dir /tmp/mega-base-image-files
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
    ap.add_argument("--container-name", default=MEGA_CONTAINER_NAME_DEFAULT)
    ap.add_argument("--out-mk", type=Path, default=None)
    ap.add_argument("--out-dep", type=Path, default=None)
    ap.add_argument("--out-base-image-files-dir", type=Path, default=None)
    args = ap.parse_args()

    registry = discover_registry(args.sonic_root)
    resolved = resolve_features(args.features.split(","), registry)
    if resolved.unknown or resolved.excluded:
        print(f"ERROR: unknown={resolved.unknown} excluded={resolved.excluded}", file=sys.stderr)
        sys.exit(1)

    specs = _discover_features(args.sonic_root, resolved.resolved, registry)
    result = generate_mega_mk(specs, args.sonic_root, container_name=args.container_name)

    print(f"=== include_docker_features: {result.include_docker_features} ===")
    print(f"=== base_load_docker: {result.base_load_docker} ===")
    print(f"=== rsync_load_docker_features: {result.rsync_load_docker_features} ===")
    print(f"=== load_dockers: {result.load_dockers} ===")
    print(f"=== install_python_wheels: {result.install_python_wheels} ===")
    print(f"=== install_debs: {result.install_debs} ===")
    print(f"=== distro_docker_list: {result.distro_docker_list} ===")
    print(f"=== shutdown_order: {result.shutdown_order} ===")
    print(f"=== run_opt: {result.run_opt} ===")
    print(f"=== base_image_files ({len(result.base_image_files)}) ===")
    for bif in result.base_image_files:
        print(
            f"  {bif.name}:{bif.dest}  (feature={bif.feature}, "
            f"retargeted={bif.retargeted}, has_content={bif.content is not None})"
        )
    print(f"=== warnings ({len(result.warnings)}) ===")
    for w in result.warnings:
        print(f"  WARNING: {w}")

    if args.out_mk:
        args.out_mk.write_text(result.mk_text)
        print(f"wrote {args.out_mk}")
    if args.out_dep:
        args.out_dep.write_text(result.dep_text)
        print(f"wrote {args.out_dep}")
    if args.out_base_image_files_dir:
        args.out_base_image_files_dir.mkdir(parents=True, exist_ok=True)
        for bif in result.base_image_files:
            if bif.content is None:
                continue
            out_path = args.out_base_image_files_dir / bif.name
            out_path.write_text(bif.content)
            print(f"wrote {out_path}")

    if not args.out_mk:
        print("\n--- generated rules/docker-mega.mk (not written, pass --out-mk) ---")
        print(result.mk_text)
