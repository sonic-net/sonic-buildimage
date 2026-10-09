"""Generic supervisord-config merge engine for the mega container.

Implements plan section 8 ("Supervisord merge algorithm"): given the
`FeatureSpec`s discovery.py already resolved (supervisord.conf(.j2) +
optional .common.j2 companion + critical_processes per feature), produce
ONE merged `supervisord.conf.j2` text + ONE merged `critical_processes`
text for `dockers/docker-mega`.

This is a **text-level** merge, not a Jinja-execution engine: every source
file is still a Jinja2 *template* (rendered later, at container-init time,
by `sonic-cfggen`), so the merge must preserve embedded `{% if %}`/`{% for
%}`/`{{ }}` constructs verbatim wherever it doesn't have a specific reason
to touch a line. Concretely this module:

1. Parses each feature's supervisord.conf(.j2) and supervisord.conf.common.j2
   (if any) into an ordered list of `Stanza`s (`[program:x]`,
   `[eventlistener:x]`, `[supervisord]`, ...) via a **generic INI-ish
   parser** that tolerates arbitrary interleaved Jinja2 control lines
   inside/around stanzas (see `parse_conf`).
2. Drops the shared-infra trio (`[eventlistener:dependent-startup]`,
   `[eventlistener:supervisor-proc-exit-listener]`, `[program:rsyslogd]`,
   plus the bare `[supervisord]` header) from every feature and emits
   exactly one merged copy of each in mega's base config (fields unioned;
   `supervisor-proc-exit-listener`'s `--container-name` and `events=` are
   specially recomputed for the merged container).
3. Resolves `[program:start]` collisions: every feature that defines its
   own `[program:start]` (teamd, lldp, snmp, gnmi, radv, eventd, in the
   default 10) gets pulled out and folded into ONE shared `[program:start]`
   whose command sequentially subshells each colliding feature's
   (isolation-dir-relocated, see plan section 1/7)
   `/opt/sonic/core-services/<feature>/start.sh`.
4. Assigns each feature a non-overlapping priority band (width 20,
   `base = 10 + i*20`, `i` = index in the canonical feature order) and
   rewrites priorities:
   - Features whose `.common.j2` already parameterizes priority via a
     `{% set X_priority_base = ... %}`-style knob (swss, bgp, teamd, lldp)
     get that knob **re-injected** with the computed band value (reusing
     the file's own Jinja arithmetic -- no literal-priority text surgery
     needed).
   - Everyone else gets literal integer `priority=N` lines rebased to
     `N + base` directly.
5. Rewrites cross-feature `dependent_startup_wait_for` edges so the whole
   mega container starts as one sequential chain (feature i's entry point
   waits on feature i-1's inferred terminal program) instead of every
   feature's independent entry point racing on the now-shared
   `rsyslogd:running`/`start:exited` events in parallel. The shared
   `[program:start]` (point 3 above) is chained the same way: it waits on
   the terminal program of the last non-starter feature that precedes the
   first starter feature in canonical order (typically swss/bgp), mirroring
   the real per-container systemd `After=swss.service` teamd/lldp both
   declare, instead of racing against swss/bgp on `rsyslogd:running`. Its
   merged command is also failure-tolerant (`a || echo ...; b || echo ...`,
   not `set -e; a; b`) so one folded feature's `start.sh` failing doesn't
   silently skip every other folded feature's `start.sh` (Finding #9).
6. Emits one `[group:<feature>]` wrapper per feature (skipped, with a
   warning, for features whose program names are Jinja-dynamic, e.g.
   `docker-database`'s `INSTANCES` loop -- supervisord's `programs=` list
   must be static).
7. Unions `critical_processes` (straight concatenation; each feature's own
   Jinja logic, if any, is preserved verbatim).

Every anomaly this module can't confidently resolve is recorded in
`MergeResult.warnings` rather than silently guessed away or crashing (per
the `sonic-docs-lookup` rule: "flag the uncertainty explicitly").
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

from mega_gen.discovery import FeatureSpec

# ---------------------------------------------------------------------------
# Canonical feature order / priority bands (plan section 8, "Priority bands").
# ---------------------------------------------------------------------------

CANONICAL_ORDER: Tuple[str, ...] = (
    "database",
    "swss",
    "bgp",
    "teamd",
    "lldp",
    "snmp",
    "gnmi",
    "radv",
    "eventd",
    "sysmgr",
    "pmon",
)

PRIORITY_BAND_START = 10
PRIORITY_BAND_WIDTH = 20

# The minimal "core" set of features whose critical_processes are included
# in conservative-mode critical_processes (Finding #8): database's redis
# instances, swss's ASIC-programming daemons, bgp's routing daemons, and
# teamd's LAG daemons -- crashing any of these genuinely warrants a full
# mega-container restart. Every other folded feature (lldp, snmp, gnmi,
# radv, eventd, sysmgr, pmon) is excluded from conservative mode even if it
# ships its own critical_processes file.
_CONSERVATIVE_CORE_FEATURES: Tuple[str, ...] = ("database", "swss", "bgp", "teamd")

# ---------------------------------------------------------------------------
# Generic INI-ish parser for supervisord stanzas.
# ---------------------------------------------------------------------------

_HEADER_RE = re.compile(r"^\[([A-Za-z_][\w.-]*)(?::([^\]]+))?\]\s*$")
_IF_OPEN_RE = re.compile(r"\{%-?\s*if\b")
_IF_CLOSE_RE = re.compile(r"\{%-?\s*endif\b")


@dataclass
class Stanza:
    """One `[kind:name]` (or `[kind]`) block: the header line plus every
    raw line up to (not including) the next header line or EOF, verbatim.

    `raw_lines[0]` is always the header line itself. Deliberately does
    *not* eagerly decompose into a field dict -- callers that need a
    specific field use `get_field`/`set_field_line` below, which operate
    directly on `raw_lines` so every untouched line (comments,
    `environment=`, embedded Jinja, whatever) round-trips byte-for-byte.
    """

    kind: str
    name: str
    raw_lines: List[str] = field(default_factory=list)
    jinja_depth_at_header: int = 0
    """Net count of unclosed `{% if %}` blocks open at this stanza's header
    line, computed over the *whole file* this stanza came from. 0 means
    "unconditionally present when rendered"; >0 flags a stanza that only
    exists in some rendered variants (used to pick safer terminal-edge
    candidates -- see `_infer_terminal`)."""

    def to_text(self) -> str:
        return "\n".join(self.raw_lines)


def parse_conf(text: str) -> Tuple[List[str], List[Stanza]]:
    """Split `text` into (preamble_lines_before_first_stanza, [Stanza, ...]).

    Any Jinja control lines or `{% set %}` assignments physically
    interleaved *between* stanzas end up attached as trailing raw_lines of
    whichever stanza precedes them (or in the returned preamble, if before
    the first header) -- this is what lets `docker-router-advertiser`'s
    `{%- if vlan_v6.count > 0 %}` guard (which wraps two stanzas, opened
    right after `[program:start]` and closed after `[program:radvd]`)
    round-trip correctly as long as callers don't reorder stanzas *within*
    one feature's own contribution (this module never does).
    """
    preamble: List[str] = []
    stanzas: List[Stanza] = []
    current: Optional[Stanza] = None
    depth = 0
    for line in text.splitlines():
        m = _HEADER_RE.match(line)
        if m:
            if current is not None:
                stanzas.append(current)
            current = Stanza(
                kind=m.group(1),
                name=m.group(2) or "",
                raw_lines=[line],
                jinja_depth_at_header=depth,
            )
        elif current is None:
            preamble.append(line)
        else:
            current.raw_lines.append(line)
        depth += len(_IF_OPEN_RE.findall(line))
        depth -= len(_IF_CLOSE_RE.findall(line))
        depth = max(depth, 0)  # defensive against a lone stray {% endif %}
    if current is not None:
        stanzas.append(current)
    return preamble, stanzas


def get_field(stanza: Stanza, key: str) -> Optional[str]:
    """First literal `key=value` line's value (stripped), or None. Only
    matches a plain assignment line -- a field hidden inside a
    `{% if %}`-guarded alternate branch that isn't the first match is not
    found (deliberate scaffold-stage simplification; see module docstring
    point 5 for why this is safe for the fields this module actually
    rewrites)."""
    pat = re.compile(rf"^\s*{re.escape(key)}\s*=\s*(.*)$")
    for line in stanza.raw_lines:
        m = pat.match(line)
        if m:
            return m.group(1).strip()
    return None


def set_field_line(stanza: Stanza, key: str, new_value: str) -> bool:
    """Replace the first literal `key=...` line's value in place,
    preserving leading whitespace. Returns False (no-op) if no such line
    exists."""
    pat = re.compile(rf"^(\s*{re.escape(key)}\s*=\s*)(.*)$")
    for i, line in enumerate(stanza.raw_lines):
        m = pat.match(line)
        if m:
            stanza.raw_lines[i] = f"{m.group(1)}{new_value}"
            return True
    return False


_PRIORITY_LITERAL_RE = re.compile(r"^(\s*priority\s*=\s*)(\d+)\s*$")


def rebase_literal_priority(stanza: Stanza, base: int) -> bool:
    """Add `base` to a stanza's `priority=` value, but only if that value
    is a bare integer (Jinja-expression priorities, e.g.
    `priority={{ swss_priority_base + 3 }}`, are intentionally left alone
    -- those are rebased by injecting the `*_priority_base` variable
    instead; see `_detect_priority_base_var`)."""
    for i, line in enumerate(stanza.raw_lines):
        m = _PRIORITY_LITERAL_RE.match(line)
        if m:
            stanza.raw_lines[i] = f"{m.group(1)}{int(m.group(2)) + base}"
            return True
    return False


# ---------------------------------------------------------------------------
# Field-name lists used when "absorbing" a stanza into a shared bucket:
# strip its recognized supervisord config fields (which get merged/
# deduped elsewhere) but keep any leftover non-field lines (Jinja `{% set
# %}`/`{% if %}` control structures interleaved in the source file) so
# nothing needed downstream in the *same* file gets silently dropped.
# ---------------------------------------------------------------------------

_KNOWN_FIELD_RE = re.compile(
    r"^\s*(command|process_name|numprocs|numprocs_start|priority|autostart|"
    r"autorestart|startsecs|startretries|exitcodes|stopsignal|stopwaitsecs|"
    r"stopasgroup|killasgroup|user|redirect_stderr|stdout_logfile|"
    r"stdout_logfile_maxbytes|stdout_logfile_backups|stdout_capture_maxbytes|"
    r"stdout_events_enabled|stdout_syslog|stderr_logfile|"
    r"stderr_logfile_maxbytes|stderr_logfile_backups|stderr_capture_maxbytes|"
    r"stderr_events_enabled|stderr_syslog|environment|directory|umask|"
    r"serverurl|dependent_startup|dependent_startup_wait_for|events|buffer_size|"
    r"logfile_maxbytes|logfile_backups|nodaemon|loglevel)\s*="
)
_INCLUDE_LINE_RE = re.compile(r"\{%-?\s*include\b")


def _clean_leftover(lines: Sequence[str]) -> List[str]:
    """Strip recognized `key=value` config-field lines and `{% include %}`
    lines (the content they would have pulled in is inlined directly by
    this module instead, see module docstring point 1) out of a stanza's
    body, keeping everything else (comments, blanks, other Jinja control
    lines) verbatim."""
    out = []
    for line in lines:
        if _KNOWN_FIELD_RE.match(line):
            continue
        if _INCLUDE_LINE_RE.search(line):
            continue
        out.append(line)
    return out


def _fields_only_copy(st: Stanza) -> Stanza:
    """The inverse of `_clean_leftover`: a copy of `st` carrying *only* its
    header + recognized `key=value` field lines. Used when a stanza is
    being folded into the cross-feature shared bucket (rsyslogd, the
    eventlisteners, the bare `[supervisord]` header) -- any interstitial
    Jinja text trailing the stanza in its source file (e.g. database's
    `{% if INSTANCES %}{% for ... %}` loop opener, which physically sits
    right after `[program:rsyslogd]`'s fields in `docker-database`'s
    source) is *not* infrastructure, it belongs to that feature's own
    contribution and must stay there (see `extra_preamble` in
    `merge_supervisord`) -- carrying it along into the single shared copy
    too would both duplicate it and leave a dangling unclosed `{% if %}`
    wrapping the rest of the merged file."""
    kept = [st.raw_lines[0]] + [
        l for l in st.raw_lines[1:] if _KNOWN_FIELD_RE.match(l)
    ]
    return Stanza(
        kind=st.kind,
        name=st.name,
        raw_lines=kept,
        jinja_depth_at_header=st.jinja_depth_at_header,
    )


# ---------------------------------------------------------------------------
# Detecting the priority-base / wait-for Jinja knobs a feature's
# `.common.j2` already exposes (swss, bgp, teamd, lldp -- plan section 6
# "hybrid Dockerfile strategy" companions). Two shapes observed in the
# tree:
#   - direct:  {% set swss_priority_base = 0 %}                  (swss)
#   - alias:   {% set _pbase = frr_priority_base | default(4) %} (bgp, lldp)
#              {% set teamd_priority_base = teamd_priority_base | default(3) %}
# Both resolve to the *outer* knob name a caller is meant to set before
# the (now-inlined) common content runs.
# ---------------------------------------------------------------------------

_ALIAS_PBASE_RE = re.compile(
    r"\{%-?\s*set\s+\w+\s*=\s*(\w+_priority_base)\s*\|\s*default\(\s*\d+\s*\)"
)
_DIRECT_PBASE_RE = re.compile(r"\{%-?\s*set\s+(\w+_priority_base)\s*=\s*\d+\s*-?%\}")
_WAITVAR_RE = re.compile(r'\{%-?\s*set\s+(\w+_wait_for)\s*=\s*"rsyslogd:running"')


def _detect_priority_base_var(text: str) -> Optional[str]:
    m = _ALIAS_PBASE_RE.search(text)
    if m:
        return m.group(1)
    m = _DIRECT_PBASE_RE.search(text)
    return m.group(1) if m else None


def _detect_wait_for_var(text: str) -> Optional[str]:
    m = _WAITVAR_RE.search(text)
    return m.group(1) if m else None


# ---------------------------------------------------------------------------
# Terminal-program inference (for cross-feature wait_for chaining).
# ---------------------------------------------------------------------------


def _infer_terminal(
    stanzas: Sequence[Stanza],
) -> Tuple[Optional[str], Optional[str], Optional[str]]:
    """Best-effort "what does this feature finish on" for chaining the next
    feature's entry point after it. Returns (program_name, state,
    warning) -- `state` is `"running"` or `"exited"`; `warning` is set
    (name/state may still be usable) when the heuristic had to guess.

    Strategy: prefer the *last* `[program:X]` stanza that is
    unconditionally present (`jinja_depth_at_header == 0`) -- conditional
    stanzas (e.g. radv's ToR-only `radvd`) aren't safe anchors since the
    downstream feature would deadlock in builds where they don't render.
    The expected state is taken from another stanza in the *same* feature
    that already waits on this program (self-consistent with how the
    feature uses it internally); failing that, a one-shot-script heuristic
    (`autostart=false` and `startsecs=0`) picks `exited`, else `running`.
    """
    programs = [s for s in stanzas if s.kind == "program"]
    if not programs:
        return None, None, "feature contributes no [program:] stanzas at all"

    unconditional = [s for s in programs if s.jinja_depth_at_header == 0]
    if not unconditional:
        return (
            None,
            None,
            "all contributed programs are conditionally Jinja-guarded; no "
            "safe unconditional terminal anchor -- next feature keeps its "
            "default rsyslogd:running dependency and may start concurrently "
            "with this feature's conditional programs",
        )
    term = unconditional[-1]

    state = None
    for other in stanzas:
        wf = get_field(other, "dependent_startup_wait_for")
        if wf and wf.strip().startswith(f"{term.name}:"):
            state = wf.strip().split(":", 1)[1]
            break
    warn = None
    if state is None:
        autostart = (get_field(term, "autostart") or "").strip()
        startsecs = (get_field(term, "startsecs") or "").strip()
        state = "exited" if autostart == "false" and startsecs == "0" else "running"
        warn = (
            f"terminal program '{term.name}' inferred as '{state}' via the "
            f"one-shot-script heuristic (nothing in-file already waits on "
            f"it to confirm) -- verify on real hardware"
        )
    return term.name, state, warn


def _find_entry_stanza(stanzas: Sequence[Stanza]) -> Optional[Stanza]:
    """The stanza whose `dependent_startup_wait_for` is literally
    `rsyslogd:running` -- i.e. this feature's natural (pre-merge) entry
    point, the one that needs to be re-pointed at the previous feature's
    terminal program instead."""
    for st in stanzas:
        wf = get_field(st, "dependent_startup_wait_for")
        if wf and wf.strip() == "rsyslogd:running":
            return st
    return None


# ---------------------------------------------------------------------------
# Shared-infra union helpers.
# ---------------------------------------------------------------------------


def _union_kv_lines(stanzas: Sequence[Stanza]) -> List[str]:
    """Base = first stanza's full body verbatim; append any `key=value`
    lines from later stanzas whose key wasn't already present in the
    base (e.g. sysmgr's `[supervisord]` adding `loglevel=warn` on top of
    everyone else's `logfile_maxbytes`/`logfile_backups`/`nodaemon`)."""
    if not stanzas:
        return []
    out = list(stanzas[0].raw_lines[1:])
    seen = set()
    for line in out:
        m = re.match(r"^\s*(\w+)\s*=", line)
        if m:
            seen.add(m.group(1))
    for st in stanzas[1:]:
        for line in st.raw_lines[1:]:
            m = re.match(r"^\s*(\w+)\s*=", line)
            if m and m.group(1) not in seen:
                out.append(line)
                seen.add(m.group(1))
    return out


def _merge_proc_exit_listener(stanzas: Sequence[Stanza], container_name: str) -> List[str]:
    """Special-cased merge for `[eventlistener:supervisor-proc-exit-listener]`:
    unlike the other shared-infra stanzas this one's `command=` genuinely
    differs per feature (`--container-name <feature>`) and its `events=`
    list varies (swss adds `PROCESS_COMMUNICATION_STDOUT`; radv omits
    `PROCESS_STATE_FATAL`) -- so this isn't "identical, take one copy", it
    needs `--container-name` retargeted to the mega container and
    `events=` unioned across every selected feature."""
    base = _union_kv_lines(stanzas)
    events: List[str] = []
    for st in stanzas:
        ev = get_field(st, "events")
        if not ev:
            continue
        for e in ev.split(","):
            e = e.strip()
            if e and e not in events:
                events.append(e)

    out: List[str] = []
    replaced_cmd = replaced_events = False
    for line in base:
        if re.match(r"^\s*command\s*=", line):
            out.append(
                f"command=/usr/bin/supervisor-proc-exit-listener-rs --container-name {container_name}"
            )
            replaced_cmd = True
        elif re.match(r"^\s*events\s*=", line):
            out.append(f"events={','.join(events)}")
            replaced_events = True
        else:
            out.append(line)
    if not replaced_cmd:
        out.append(
            f"command=/usr/bin/supervisor-proc-exit-listener-rs --container-name {container_name}"
        )
    if not replaced_events and events:
        out.append(f"events={','.join(events)}")
    return out


# ---------------------------------------------------------------------------
# Public result type.
# ---------------------------------------------------------------------------


@dataclass
class MergeResult:
    supervisord_conf_text: str
    """Full merged `supervisord.conf.j2` body (still a Jinja2 template --
    every feature's own runtime conditionals are preserved verbatim)."""

    critical_processes_text: str
    """Union of every selected feature's `critical_processes(.j2)`."""

    warnings: List[str] = field(default_factory=list)
    feature_order: List[str] = field(default_factory=list)
    starter_features: List[str] = field(default_factory=list)
    """Features whose own `[program:start]` was folded into the shared
    `[program:start]` (plan section 8, "[program:start] collisions")."""

    grouped_features: List[str] = field(default_factory=list)
    """Features that got a `[group:<feature>]` wrapper emitted (excludes
    any feature whose program names are Jinja-dynamic -- see warnings)."""

    priority_bands: Dict[str, int] = field(default_factory=dict)
    """feature -> assigned priority-band base (10 + i*20)."""


# ---------------------------------------------------------------------------
# Main entry point.
# ---------------------------------------------------------------------------


def merge_supervisord(
    specs: Sequence[FeatureSpec],
    container_name: str = "mega",
    conservative_critical: bool = True,
) -> MergeResult:
    """Merge the supervisord assets of every already-discovered
    `FeatureSpec` (see `mega_gen.discovery.discover_features`) into one
    `MergeResult`. Never raises on a per-feature anomaly -- everything
    unexpected is recorded in `MergeResult.warnings` instead (per the
    `sonic-docs-lookup` rule)."""
    warnings: List[str] = []

    order_index = {name: i for i, name in enumerate(CANONICAL_ORDER)}
    ordered = sorted(
        specs, key=lambda s: order_index.get(s.feature, len(CANONICAL_ORDER) + 1)
    )
    for s in ordered:
        if s.feature not in order_index:
            warnings.append(
                f"{s.feature}: not in the canonical priority-band order "
                f"{CANONICAL_ORDER}; appended after it (open feature "
                f"universe -- band still assigned, just unranked)"
            )

    # --- Phase 1: parse + classify each feature's stanzas -----------------
    shared_supervisord: List[Stanza] = []
    shared_dependent_startup: List[Stanza] = []
    shared_proc_exit: List[Stanza] = []
    shared_rsyslogd: List[Stanza] = []

    features_data: Dict[str, dict] = {}

    for spec in ordered:
        feat = spec.feature
        if spec.supervisord_conf is None:
            warnings.append(
                f"{feat}: discovery found no supervisord.conf(.j2) -- skipped "
                f"entirely from the merge"
            )
            continue

        main_text = spec.supervisord_conf.read_text(errors="replace")
        common_text = (
            spec.supervisord_common.read_text(errors="replace")
            if spec.supervisord_common
            else ""
        )
        main_pre, main_stanzas = parse_conf(main_text)
        common_pre, common_stanzas = parse_conf(common_text) if common_text else ([], [])
        full_text = main_text + "\n" + common_text

        starter: Optional[Stanza] = None
        remaining: List[Stanza] = []
        extra_preamble: List[str] = list(main_pre)

        for st in main_stanzas:
            key = (st.kind, st.name.strip())
            leftover = _clean_leftover(st.raw_lines[1:])
            if key == ("program", "start"):
                starter = st
                extra_preamble.extend(leftover)
            elif key == ("program", "rsyslogd"):
                shared_rsyslogd.append(_fields_only_copy(st))
                extra_preamble.extend(leftover)
            elif key == ("eventlistener", "dependent-startup"):
                shared_dependent_startup.append(_fields_only_copy(st))
                extra_preamble.extend(leftover)
            elif key == ("eventlistener", "supervisor-proc-exit-listener"):
                shared_proc_exit.append(_fields_only_copy(st))
                extra_preamble.extend(leftover)
            elif st.kind == "supervisord":
                shared_supervisord.append(_fields_only_copy(st))
                extra_preamble.extend(leftover)
            else:
                remaining.append(st)

        # A `.common.j2` companion only matters if *this* feature's own
        # main.j2 actually `{% include %}`s it (the swss/bgp/teamd/lldp
        # "hybrid" pattern). Some features (sflow, nat) ship a
        # `supervisord.conf.common.j2` that's dead weight for their own
        # standalone container -- it exists solely for `docker-sonic-vs` to
        # pull in via `_INCLUDE_DOCKER`, and their own `supervisord.conf` is
        # a fully static, self-contained file with zero `{% include %}` at
        # all. Conflating "has a .common.j2 file" with "is templated" would
        # silently drop that feature's *real* main.j2-only programs (e.g.
        # sflow's `port_index_mapper`, which has no common.j2 counterpart).
        is_templated = bool(common_stanzas) and bool(_INCLUDE_LINE_RE.search(main_text))
        if is_templated:
            real_stanzas = common_stanzas
            extra_preamble.extend(common_pre)
        else:
            real_stanzas = remaining
            if common_stanzas:
                warnings.append(
                    f"{feat}: has a supervisord.conf.common.j2 but its own "
                    f"main supervisord.conf never {{% include %}}s it (likely "
                    f"only consumed by docker-sonic-vs) -- ignoring it and "
                    f"using main.j2's own {len(remaining)} program stanza(s) "
                    f"as-is"
                )

        pbase_var = _detect_priority_base_var(full_text) if is_templated else None
        wfvar = _detect_wait_for_var(full_text) if is_templated else None

        base_preamble = extra_preamble
        if pbase_var:
            pat = re.compile(rf"set\s+{re.escape(pbase_var)}\s*=")
            base_preamble = [l for l in base_preamble if not pat.search(l)]
        if wfvar:
            pat = re.compile(rf"set\s+{re.escape(wfvar)}\s*=")
            base_preamble = [l for l in base_preamble if not pat.search(l)]

        starter_command = get_field(starter, "command") if starter else None
        starter_autostart = (
            (get_field(starter, "autostart") or "").strip() == "true" if starter else False
        )

        crit_text = None
        if spec.critical_processes is not None:
            crit_text = spec.critical_processes.read_text(errors="replace")
        else:
            warnings.append(
                f"{feat}: no critical_processes(.j2) found -- omitted from the "
                f"unioned critical_processes"
            )

        dynamic_names = any("{{" in s.name for s in real_stanzas if s.kind == "program")

        features_data[feat] = dict(
            is_templated=is_templated,
            pbase_var=pbase_var,
            wfvar=wfvar,
            starter_command=starter_command,
            starter_autostart=starter_autostart,
            base_preamble=base_preamble,
            real_stanzas=real_stanzas,
            dynamic_names=dynamic_names,
            crit_text=crit_text,
        )

    starter_features = [
        f for f, d in features_data.items() if d["starter_command"] is not None
    ]
    # keep canonical order
    starter_features = [s.feature for s in ordered if s.feature in starter_features]

    # --- Phase 2: assign bands, inject overrides, chain wait_for edges ----
    program_blocks: List[str] = []
    group_blocks: List[str] = []
    priority_bands: Dict[str, int] = {}
    grouped_features: List[str] = []
    prev_terminal: Optional[Tuple[str, str]] = None
    # Finding #9: the shared `[program:start]` (teamd/lldp/snmp/gnmi/radv/
    # eventd's start.sh scripts, folded into one program -- see Phase 3)
    # must start no earlier than the LAST non-starter feature that precedes
    # the first starter feature in canonical order (typically swss and/or
    # bgp) -- mirroring the real systemd `After=swss.service` teamd/lldp
    # both declare -- instead of racing against it on the shared
    # `rsyslogd:running` event like every other feature's own entry point.
    # Captured once, the first time a starter feature is reached, from
    # whatever `prev_terminal` already is at that point (i.e. the terminal
    # program of the last-processed non-starter feature).
    starter_wait_edge: Optional[str] = None
    seen_starter = False

    for idx, spec in enumerate(ordered):
        feat = spec.feature
        d = features_data.get(feat)
        if d is None:
            continue

        base = PRIORITY_BAND_START + idx * PRIORITY_BAND_WIDTH
        priority_bands[feat] = base
        is_starter = feat in starter_features

        if is_starter and not seen_starter:
            seen_starter = True
            if prev_terminal is not None:
                starter_wait_edge = f"{prev_terminal[0]}:{prev_terminal[1]}"
            else:
                warnings.append(
                    "no non-starter feature precedes the first starter "
                    "feature (teamd/lldp/snmp/gnmi/radv/eventd) with an "
                    "inferable terminal program -- the shared [program:start] "
                    "keeps its default rsyslogd:running dependency, meaning "
                    "it may start concurrently with swss/bgp instead of "
                    "strictly after them"
                )

        injected: List[str] = []
        if d["pbase_var"]:
            injected.append(f"{{% set {d['pbase_var']} = {base} %}}")
        else:
            for st in d["real_stanzas"]:
                rebase_literal_priority(st, base)

        if idx > 0 and not is_starter:
            if prev_terminal is None:
                warnings.append(
                    f"{feat}: previous feature in the chain has no inferable "
                    f"terminal program -- leaving {feat}'s entry point on its "
                    f"default rsyslogd:running dependency (it may start "
                    f"concurrently with the previous feature instead of "
                    f"strictly after it)"
                )
            else:
                edge = f"{prev_terminal[0]}:{prev_terminal[1]}"
                if d["wfvar"]:
                    injected.append(f'{{% set {d["wfvar"]} = "{edge}" %}}')
                else:
                    entry = _find_entry_stanza(d["real_stanzas"])
                    if entry is not None:
                        set_field_line(entry, "dependent_startup_wait_for", edge)
                    else:
                        warnings.append(
                            f"{feat}: no stanza depends directly on "
                            f"rsyslogd:running to chain after '{edge}' -- "
                            f"leaving {feat} as-is (check for a pre-existing "
                            f"dangling wait_for reference upstream, e.g. "
                            f"sysmgr's rebootbackend waits on 'start:exited' "
                            f"even though sysmgr ships no [program:start])"
                        )

        preamble_lines = injected + d["base_preamble"]
        block: List[str] = []
        if any(l.strip() for l in preamble_lines):
            block.append(f"# --- {feat}: feature preamble ---")
            block.extend(preamble_lines)
        for st in d["real_stanzas"]:
            block.extend(st.raw_lines)
            block.append("")
        program_blocks.append("\n".join(block))

        if d["real_stanzas"]:
            if d["dynamic_names"]:
                warnings.append(
                    f"{feat}: skipped [group:{feat}] wrapper -- member program "
                    f"names are Jinja-dynamic (e.g. a for-loop over INSTANCES) "
                    f"and supervisord's programs= list must be static"
                )
            else:
                # A stanza with jinja_depth_at_header > 0 (e.g. bgp's bfdd,
                # wrapped in `{% if frr_mgmt_framework_config == "true" %}`)
                # is only in the rendered output in *some* variants. Putting
                # it in this static programs= list anyway makes supervisord
                # fail outright ("unknown program ... bfdd") whenever that
                # variant renders without it -- so such names are excluded
                # here the same way a for-loop's dynamic names already are.
                conditional = [
                    s.name.strip() for s in d["real_stanzas"]
                    if s.kind == "program" and s.jinja_depth_at_header > 0
                ]
                names = [
                    s.name.strip() for s in d["real_stanzas"]
                    if s.kind == "program" and s.jinja_depth_at_header == 0
                ]
                if conditional:
                    warnings.append(
                        f"{feat}: omitted conditionally-rendered program(s) "
                        f"{','.join(conditional)} from [group:{feat}]'s "
                        f"programs= list -- they only exist in some rendered "
                        f"variants and supervisord's programs= list must "
                        f"name only unconditionally-present programs"
                    )
                if names:
                    group_blocks.append(
                        f"[group:{feat}]\nprograms={','.join(names)}"
                    )
                    grouped_features.append(feat)

        term_name, term_state, term_warn = _infer_terminal(d["real_stanzas"])
        if term_warn:
            warnings.append(f"{feat}: {term_warn}")
        prev_terminal = (term_name, term_state) if term_name else None

    # --- Phase 3: emit shared infra + the (possibly-merged) [program:start] ---
    shared: List[str] = []

    if shared_supervisord:
        shared.append("[supervisord]")
        shared.extend(_union_kv_lines(shared_supervisord))
    else:
        warnings.append(
            "no [supervisord] header found in any selected feature -- emitting "
            "a minimal default"
        )
        shared.extend(["[supervisord]", "logfile_maxbytes=1MB", "logfile_backups=2", "nodaemon=true"])
    shared.append("")

    if shared_dependent_startup:
        shared.append("[eventlistener:dependent-startup]")
        dep_lines = _union_kv_lines(shared_dependent_startup)
        # Plan section 12: depstartup pending-start guard.  Replace the
        # stock ``python3 -m supervisord_dependent_startup`` command with the
        # guard wrapper generated by gen_dockerfile.py, which monkey-patches
        # xmlrpc.client to skip startProcess calls for processes already in a
        # transitional state (prevents the respawn storm in the merged mega
        # container's supervisord).
        _guard_path = "/usr/local/bin/depstartup_guard.py"
        for i, line in enumerate(dep_lines):
            if re.match(r"^\s*command\s*=", line):
                dep_lines[i] = f"command=python3 {_guard_path}"
                break
        else:
            dep_lines.insert(0, f"command=python3 {_guard_path}")
        shared.extend(dep_lines)
        shared.append("")
    else:
        warnings.append(
            "no [eventlistener:dependent-startup] found in any selected feature "
            "-- dependent_startup_wait_for chaining will not actually run"
        )

    if shared_proc_exit:
        shared.append("[eventlistener:supervisor-proc-exit-listener]")
        shared.extend(_merge_proc_exit_listener(shared_proc_exit, container_name))
        shared.append("")

    if shared_rsyslogd:
        shared.append("[program:rsyslogd]")
        shared.extend(_union_kv_lines(shared_rsyslogd))
        shared.append("")
    else:
        warnings.append("no [program:rsyslogd] found in any selected feature")

    if starter_features:
        # Finding #9 (failure tolerance): each folded feature's start.sh
        # originally ran as its OWN supervisord program in its OWN
        # container -- one feature's start.sh failing never affected any
        # other. Now that they're sequential commands in ONE shared bash
        # invocation, a plain `set -e; a; b; c` chain would let feature 1's
        # failure silently skip features 2..N entirely. Wrap each command so
        # its own failure is logged but doesn't abort the rest.
        commands = [
            f'/opt/sonic/core-services/{f}/start.sh || '
            f'echo "mega: start.sh for {f} failed (rc=$?)" >&2'
            for f in starter_features
        ]
        any_autostart = any(features_data[f]["starter_autostart"] for f in starter_features)
        shared.append("[program:start]")
        shared.append(f"command=/bin/bash -c '{'; '.join(commands)}'")
        shared.append(f"priority={min(priority_bands[f] for f in starter_features)}")
        shared.append(f"autostart={'true' if any_autostart else 'false'}")
        shared.append("autorestart=false")
        shared.append("startsecs=0")
        shared.append("stdout_logfile=NONE")
        shared.append("stdout_syslog=true")
        shared.append("stderr_logfile=NONE")
        shared.append("stderr_syslog=true")
        shared.append("dependent_startup=true")
        # Finding #9 (ordering): wait for the last non-starter feature's
        # (typically swss/bgp's) terminal program instead of racing on
        # rsyslogd:running -- mirrors the real per-container systemd
        # `After=swss.service` teamd/lldp both declare. See `starter_wait_edge`
        # computation in Phase 2 above.
        shared.append(f"dependent_startup_wait_for={starter_wait_edge or 'rsyslogd:running'}")
        shared.append("")

    body = (
        "\n".join(shared)
        + "\n\n"
        + "\n\n".join(program_blocks)
        + "\n\n"
        + "\n\n".join(group_blocks)
        + "\n"
    )

    # F12 fix (Finding #8): conservative critical_processes.  In a
    # single-supervisord mega container, a "critical" process exit tears
    # down the WHOLE container (all 30+ programs), so conservative mode
    # only marks processes whose crash genuinely warrants a full restart
    # of the data/control plane as critical -- not every folded feature's
    # own critical_processes (e.g. losing lldpd or snmp-subagent shouldn't
    # restart orchagent/bgpd/redis too).
    #
    # An earlier version of this used ONLY swss's own critical_processes
    # entry (and a hardcoded "program:orchagent" fallback when swss wasn't
    # selected), which meant a redis, bgpd/zebra or teamsyncd crash would
    # never trigger a restart even though those are just as core to the
    # data/control plane as orchagent. _CONSERVATIVE_CORE_FEATURES below
    # is the documented, minimal "core" set (database's redis instances,
    # swss's ASIC-programming daemons, bgp's routing daemons, teamd's
    # LAG daemons) -- each one present and selected contributes its own
    # critical_processes entries; conservative mode is still much smaller
    # than the full union (lldp/snmp/gnmi/radv/eventd/sysmgr/pmon are
    # excluded even when they ship their own critical_processes).
    #
    # `conservative_critical` remains a caller-configurable bool (see
    # `merge_supervisord`'s signature) for anyone who wants the full
    # union instead (debugging / completeness audits).
    if conservative_critical:
        crit_parts = []
        for feat in _CONSERVATIVE_CORE_FEATURES:
            d = features_data.get(feat)
            if d and d["crit_text"]:
                crit_parts.append(f"; --- {feat} ---\n{d['crit_text'].rstrip()}")
        if crit_parts:
            critical_text = "\n\n".join(crit_parts) + "\n"
        else:
            critical_text = "program:orchagent\n"
            warnings.append(
                "conservative_critical: none of the core features "
                f"{_CONSERVATIVE_CORE_FEATURES} are selected or have a "
                "critical_processes file -- falling back to a hardcoded "
                "'program:orchagent' entry"
            )
    else:
        crit_parts = []
        for spec in ordered:
            d = features_data.get(spec.feature)
            if d and d["crit_text"]:
                crit_parts.append(f"; --- {spec.feature} ---\n{d['crit_text'].rstrip()}")
        critical_text = "\n\n".join(crit_parts) + "\n"

    return MergeResult(
        supervisord_conf_text=body,
        critical_processes_text=critical_text,
        warnings=warnings,
        feature_order=[s.feature for s in ordered if s.feature in features_data],
        starter_features=starter_features,
        grouped_features=grouped_features,
        priority_bands=priority_bands,
    )


# ---------------------------------------------------------------------------
# Standalone smoke test (not part of the pipeline wiring -- that's
# patch_templates.py's job, a separate to-do). Lets this module be
# exercised directly against a real sonic-buildimage tree:
#
#   python3 -m mega_gen.supervisord_merge --sonic-root /path/to/sonic-buildimage
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
    ap.add_argument("--out-conf", type=Path, default=None)
    ap.add_argument("--out-critical", type=Path, default=None)
    args = ap.parse_args()

    registry = discover_registry(args.sonic_root)
    resolved = resolve_features(args.features.split(","), registry)
    if resolved.unknown or resolved.excluded:
        print(f"ERROR: unknown={resolved.unknown} excluded={resolved.excluded}", file=sys.stderr)
        sys.exit(1)

    specs = _discover_features(args.sonic_root, resolved.resolved, registry)
    result = merge_supervisord(specs)

    print(f"=== feature order: {result.feature_order} ===")
    print(f"=== priority bands: {result.priority_bands} ===")
    print(f"=== starter (start-collapsed) features: {result.starter_features} ===")
    print(f"=== grouped features: {result.grouped_features} ===")
    print(f"=== warnings ({len(result.warnings)}) ===")
    for w in result.warnings:
        print(f"  WARNING: {w}")

    if args.out_conf:
        args.out_conf.write_text(result.supervisord_conf_text)
        print(f"wrote {args.out_conf}")
    if args.out_critical:
        args.out_critical.write_text(result.critical_processes_text)
        print(f"wrote {args.out_critical}")
    if not args.out_conf:
        print("\n--- merged supervisord.conf.j2 (not written, pass --out-conf) ---")
        print(result.supervisord_conf_text)
