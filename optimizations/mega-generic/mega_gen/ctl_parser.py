"""Parse docker_image_ctl.j2 → per-container blocks by hook point.

This module reads the stock `docker_image_ctl.j2` template and extracts the
per-container conditional blocks (keyed on ``docker_container_name``) organised
by hook point.  The output is consumed by ``patch_templates.py`` to
mechanically compose mega's branch from the selected features' stock blocks
(plan section 4, "docker_image_ctl.j2 hooks are parsed, not pre-configured").

See the ``generic_mega-container_merge_script`` plan, section 4 for the full
design and the six hook points this parser covers:

  | Hook point              | How mega composes it                               |
  |-------------------------|----------------------------------------------------|
  | preStartAction()        | Concatenate selected features' blocks               |
  | postStartAction()       | Concatenate selected features' blocks               |
  | start() pre-create      | Concatenate selected features' standalone blocks    |
  | docker create flags     | Generic flags + union of additive flag blocks       |
  | start/wait/stop/kill    | Database path when database selected; else generic  |
  | Top-level helpers       | Include any selected feature's functions             |

Three Jinja2 conditional patterns on ``docker_container_name`` are parsed:

  1. ``{%- if docker_container_name == "X" %}``     (equality)
  2. ``{%- if docker_container_name != "X" %}``     (inequality/negation)
  3. ``{%- if docker_container_name in ["X", …] %}`` (membership)

Each may be followed by ``{%- elif …%}`` branches and an ``{%- else %}``
fallback, terminated by ``{%- endif %}``.

Parsing approach (verified against the stock template in this tree, per the
``sonic-docs-lookup`` rule -- every structural assumption about function
layout, brace placement, and conditional patterns was confirmed directly
against ``files/build_templates/docker_image_ctl.j2``):

  - The six key functions (``preStartAction``, ``postStartAction``, ``start``,
    ``wait``, ``stop``, ``kill``) are located by name; their bodies extend to
    the next key function definition (sufficient for chain extraction -- see
    ``_find_section_ranges``).
  - Within each section, all ``docker_container_name`` conditional chains are
    found and their branches extracted with nesting-depth tracking (nested
    ``{%- if … %}``/``{%- endif %}`` blocks increment/decrement depth so the
    chain's own ``endif`` is correctly matched).
  - The ``docker create {{docker_image_run_opt}}`` command within ``start()``
    is identified by its distinctive literal text; its extent runs to the
    ``|| {`` error handler -- chains inside are reported under the dedicated
    ``docker_create_flags`` hook point.
  - Stop timeouts are extracted from ``stop()`` by scanning for
    ``container stop -t <N>`` adjacent to ``docker_container_name`` branches.
  - Chains wrapped in non-``docker_container_name`` Jinja conditionals (e.g.
    ``{%- if sonic_asic_platform == "mellanox" %}``) have their enclosing
    conditions recorded so the consumer can reconstruct the wrapper.

This module does **not** write anything to the tree (same style as every other
``gen_*``/``*_merge`` module) -- it returns a ``CtlParseResult``; actual
template surgery is ``patch_templates.py``'s job.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, FrozenSet, List, Optional, Set, Tuple


# ---------------------------------------------------------------------------
# Jinja2 conditional-line regexes.
#
# The ``docker_container_name`` patterns are the three documented in the
# module docstring.  The ``_JINJA_*`` predicates cover ANY Jinja
# if/elif/else/endif (for nesting-depth tracking inside chain parsing).
# ---------------------------------------------------------------------------

# docker_container_name == "X"  [and <extra>]
_DCN_EQ_RE = re.compile(
    r"\{%-?\s*(?:if|elif)\s+docker_container_name\s*==\s*"
    r"""["\']([^"\']+)["\']"""
    r"(?:\s+and\s+(.+?))?\s*%\}"
)
# docker_container_name != "X"  [and <extra>]
_DCN_NE_RE = re.compile(
    r"\{%-?\s*(?:if|elif)\s+docker_container_name\s*!=\s*"
    r"""["\']([^"\']+)["\']"""
    r"(?:\s+and\s+(.+?))?\s*%\}"
)
# docker_container_name in ["X", "Y", …]  [and <extra>]
_DCN_IN_RE = re.compile(
    r"\{%-?\s*(?:if|elif)\s+docker_container_name\s+in\s+\[([^\]]+)\]"
    r"(?:\s+and\s+(.+?))?\s*%\}"
)

# Distinguish ``if`` from ``elif`` for the above:
_IS_IF_TAG = re.compile(r"\{%-?\s*if\s+")
_IS_ELIF_TAG = re.compile(r"\{%-?\s*elif\s+")

# Generic Jinja control-flow predicates (any condition, not just dcn):
_JINJA_IF_RE = re.compile(r"\{%-?\s*if\s+")
_JINJA_ELIF_RE = re.compile(r"\{%-?\s*elif\s+")
_JINJA_ELSE_RE = re.compile(r"\{%-?\s*else\s*-?%\}")
_JINJA_ENDIF_RE = re.compile(r"\{%-?\s*endif\s*-?%\}")

# ``docker create`` landmark inside ``start()``:
_DOCKER_CREATE_RE = re.compile(
    r"docker\s+create\s+\{\{docker_image_run_opt\}\}"
)

# Stop-timeout extraction:
_STOP_TIMEOUT_RE = re.compile(r"container\s+stop\s+-t\s+(\d+)")


def _parse_names_from_list(s: str) -> List[str]:
    """``'"swss", "syncd"'`` → ``['swss', 'syncd']``."""
    return re.findall(r"""["\']([^"\']+)["\']""", s)


# ---------------------------------------------------------------------------
# Output dataclasses.
# ---------------------------------------------------------------------------


@dataclass
class ContainerBranch:
    """One branch (``if`` or ``elif``) of a ``docker_container_name`` chain.

    ``negated`` is True for ``!=`` patterns (the branch matches containers
    whose name is *not* in ``container_names``).

    ``extra_condition`` captures an optional trailing ``and …`` clause (e.g.
    ``enable_asan == "y"`` for the swss/syncd asan stop timeout).
    """

    container_names: FrozenSet[str]
    negated: bool = False
    extra_condition: Optional[str] = None
    text: str = ""
    header_line: str = ""


@dataclass
class ConditionalChain:
    """A complete ``if``/``elif``/``else``/``endif`` chain whose ``if`` or at
    least one ``elif`` references ``docker_container_name``.

    ``non_container_branches`` captures branches whose condition is *not* on
    ``docker_container_name`` (e.g. the ``stop_time is defined`` first branch
    in ``stop()``'s timeout chain, which is followed by dcn ``elif``
    branches).
    """

    branches: List[ContainerBranch] = field(default_factory=list)
    else_text: Optional[str] = None
    line_start: int = 0
    line_end: int = 0

    non_container_branches: List[Tuple[str, str]] = field(default_factory=list)
    """``(header_line, body_text)`` for branches not on
    ``docker_container_name``."""

    enclosing_conditions: List[str] = field(default_factory=list)
    """Jinja conditional lines that wrap this chain but are not part of the
    ``docker_container_name`` chain itself (e.g.
    ``{%- if sonic_asic_platform == "mellanox" %}``).  Populated by
    ``_extract_chains``'s context tracker."""


@dataclass
class HookPoint:
    """All conditional chains plus shared (non-conditional) code for one hook
    point in ``docker_image_ctl.j2``."""

    chains: List[ConditionalChain] = field(default_factory=list)
    shared_code_before: str = ""
    shared_code_after: str = ""
    raw_text: str = ""

    # -- convenience accessors for ``patch_templates.py`` --

    def blocks_for(self, container_name: str) -> List[str]:
        """Every text block across all chains that applies to
        *container_name*.

        Equality branches match when *container_name* is in the branch's
        ``container_names`` set.  Negated branches match when it is *not*.
        If no branch in a chain matches the name, the chain's ``else_text``
        (if any) is returned.  The result is a list (one entry per chain),
        in chain order.
        """
        result: List[str] = []
        for chain in self.chains:
            matched = False
            for branch in chain.branches:
                if branch.negated:
                    if container_name not in branch.container_names:
                        result.append(branch.text)
                        matched = True
                        break
                else:
                    if container_name in branch.container_names:
                        result.append(branch.text)
                        matched = True
                        break
            if not matched and chain.else_text is not None:
                result.append(chain.else_text)
        return result

    def all_container_names(self) -> FrozenSet[str]:
        """All container names referenced across all chains (both equality
        and negated branches)."""
        names: Set[str] = set()
        for chain in self.chains:
            for branch in chain.branches:
                names.update(branch.container_names)
        return frozenset(names)


@dataclass
class StopTimeout:
    """An extracted stop timeout for one container or container group."""

    container_names: FrozenSet[str]
    timeout_seconds: int
    extra_condition: Optional[str] = None


@dataclass
class CtlParseResult:
    """Complete parse result from ``docker_image_ctl.j2``.

    Each hook-point field is a :class:`HookPoint` whose chains and shared code
    the consumer can query by container name (see
    :meth:`HookPoint.blocks_for`).  ``stop_timeouts`` is a separate flat list
    of per-container timeout values extracted from ``stop()``'s timeout chain.
    """

    top_level_helpers: HookPoint = field(default_factory=HookPoint)
    pre_start_action: HookPoint = field(default_factory=HookPoint)
    inter_function_helpers: HookPoint = field(default_factory=HookPoint)
    post_start_action: HookPoint = field(default_factory=HookPoint)
    start_pre_create: HookPoint = field(default_factory=HookPoint)
    docker_create_flags: HookPoint = field(default_factory=HookPoint)
    start_post_create: HookPoint = field(default_factory=HookPoint)
    wait_body: HookPoint = field(default_factory=HookPoint)
    stop_body: HookPoint = field(default_factory=HookPoint)
    kill_body: HookPoint = field(default_factory=HookPoint)

    stop_timeouts: List[StopTimeout] = field(default_factory=list)

    all_container_names: FrozenSet[str] = frozenset()
    """Union of every container name referenced anywhere in the template."""

    warnings: List[str] = field(default_factory=list)

    # -- convenience --

    @property
    def max_stop_timeout(self) -> int:
        """Longest explicit stop timeout found (0 if none)."""
        return max((t.timeout_seconds for t in self.stop_timeouts), default=0)


# ---------------------------------------------------------------------------
# Internal line-classification helpers.
# ---------------------------------------------------------------------------


def _parse_dcn_branch(line: str) -> Optional[ContainerBranch]:
    """Try to parse *line* as a ``docker_container_name`` ``if`` or ``elif``
    directive.  Returns a partially-populated :class:`ContainerBranch` if
    it matches, else ``None``."""

    # == equality or in [...] membership
    for pattern, negated in [(_DCN_EQ_RE, False), (_DCN_NE_RE, True)]:
        m = pattern.search(line)
        if m:
            return ContainerBranch(
                container_names=frozenset([m.group(1)]),
                negated=negated,
                extra_condition=m.group(2),
                header_line=line.rstrip("\n"),
            )
    m = _DCN_IN_RE.search(line)
    if m:
        names = _parse_names_from_list(m.group(1))
        return ContainerBranch(
            container_names=frozenset(names),
            extra_condition=m.group(2),
            header_line=line.rstrip("\n"),
        )
    return None


def _is_dcn_if(line: str) -> bool:
    """True if *line* is a ``{%- if docker_container_name …%}`` (not elif)."""
    return _parse_dcn_branch(line) is not None and bool(_IS_IF_TAG.search(line))


def _is_dcn_elif(line: str) -> bool:
    """True if *line* is a ``{%- elif docker_container_name …%}``."""
    return _parse_dcn_branch(line) is not None and bool(_IS_ELIF_TAG.search(line))


def _has_jinja_if(line: str) -> bool:
    return bool(_JINJA_IF_RE.search(line))


def _has_jinja_elif(line: str) -> bool:
    return bool(_JINJA_ELIF_RE.search(line))


def _has_jinja_else(line: str) -> bool:
    return bool(_JINJA_ELSE_RE.search(line))


def _has_jinja_endif(line: str) -> bool:
    return bool(_JINJA_ENDIF_RE.search(line))


# ---------------------------------------------------------------------------
# Single-chain parser.
# ---------------------------------------------------------------------------


def _parse_one_chain(
    lines: List[str],
    start: int,
    max_end: int,
) -> Tuple[ConditionalChain, int, List[str]]:
    """Parse a conditional chain starting at line *start* through at most
    line *max_end*.

    The chain may start with either a ``docker_container_name`` ``if`` or a
    non-dcn ``if`` (the latter occurs in ``stop()``'s ``stop_time is
    defined`` chain, which has dcn ``elif`` branches).

    Returns ``(chain, end_line_inclusive, warnings)``.
    """
    warnings: List[str] = []
    branches: List[ContainerBranch] = []
    non_container_branches: List[Tuple[str, str]] = []
    else_text: Optional[str] = None

    # Classify the opening ``if`` line.
    dcn_branch = _parse_dcn_branch(lines[start])
    if dcn_branch is not None:
        current_branch: Optional[ContainerBranch] = dcn_branch
        current_is_non_dcn = False
    else:
        current_branch = None
        current_is_non_dcn = True
        non_container_branches.append((lines[start].rstrip("\n"), ""))

    current_text_lines: List[str] = []
    depth = 0
    in_else = False
    i = start + 1

    while i <= max_end:
        line = lines[i]

        # --- nested ``{%- if %}`` (any condition) increases depth ----------
        if _has_jinja_if(line):
            depth += 1
            current_text_lines.append(line)
            i += 1
            continue

        # --- ``{%- endif %}`` ---------------------------------------------
        if _has_jinja_endif(line):
            if depth > 0:
                depth -= 1
                current_text_lines.append(line)
                i += 1
                continue
            # depth == 0 → this endif closes the chain.
            text = "\n".join(current_text_lines)
            if in_else:
                else_text = text
            elif current_branch is not None:
                current_branch = ContainerBranch(
                    container_names=current_branch.container_names,
                    negated=current_branch.negated,
                    extra_condition=current_branch.extra_condition,
                    text=text,
                    header_line=current_branch.header_line,
                )
                branches.append(current_branch)
            elif current_is_non_dcn and non_container_branches:
                hdr, _ = non_container_branches[-1]
                non_container_branches[-1] = (hdr, text)
            return (
                ConditionalChain(
                    branches=branches,
                    else_text=else_text,
                    non_container_branches=non_container_branches,
                    line_start=start,
                    line_end=i,
                ),
                i,
                warnings,
            )

        # Below this point, depth > 0 is just content.
        if depth > 0:
            current_text_lines.append(line)
            i += 1
            continue

        # --- ``{%- elif %}`` at chain level (depth == 0) -------------------
        if _has_jinja_elif(line):
            text = "\n".join(current_text_lines)
            # Save previous branch/segment.
            if current_branch is not None:
                current_branch = ContainerBranch(
                    container_names=current_branch.container_names,
                    negated=current_branch.negated,
                    extra_condition=current_branch.extra_condition,
                    text=text,
                    header_line=current_branch.header_line,
                )
                branches.append(current_branch)
            elif current_is_non_dcn and non_container_branches:
                hdr, _ = non_container_branches[-1]
                non_container_branches[-1] = (hdr, text)

            current_text_lines = []
            in_else = False

            # Classify this elif.
            dcn_branch = _parse_dcn_branch(line)
            if dcn_branch is not None:
                current_branch = dcn_branch
                current_is_non_dcn = False
            else:
                current_branch = None
                current_is_non_dcn = True
                non_container_branches.append((line.rstrip("\n"), ""))
            i += 1
            continue

        # --- ``{%- else %}`` at chain level --------------------------------
        if _has_jinja_else(line):
            text = "\n".join(current_text_lines)
            if current_branch is not None:
                current_branch = ContainerBranch(
                    container_names=current_branch.container_names,
                    negated=current_branch.negated,
                    extra_condition=current_branch.extra_condition,
                    text=text,
                    header_line=current_branch.header_line,
                )
                branches.append(current_branch)
            elif current_is_non_dcn and non_container_branches:
                hdr, _ = non_container_branches[-1]
                non_container_branches[-1] = (hdr, text)
            current_text_lines = []
            current_branch = None
            current_is_non_dcn = False
            in_else = True
            i += 1
            continue

        # --- ordinary content line -----------------------------------------
        current_text_lines.append(line)
        i += 1

    # Reached max_end without a matching endif -- malformed, but don't crash.
    warnings.append(
        f"conditional chain starting at line {start + 1} reached end of "
        f"section (line {max_end + 1}) without a matching endif"
    )
    text = "\n".join(current_text_lines)
    if in_else:
        else_text = text
    elif current_branch is not None:
        current_branch = ContainerBranch(
            container_names=current_branch.container_names,
            negated=current_branch.negated,
            extra_condition=current_branch.extra_condition,
            text=text,
            header_line=current_branch.header_line,
        )
        branches.append(current_branch)
    elif current_is_non_dcn and non_container_branches:
        hdr, _ = non_container_branches[-1]
        non_container_branches[-1] = (hdr, text)

    return (
        ConditionalChain(
            branches=branches,
            else_text=else_text,
            non_container_branches=non_container_branches,
            line_start=start,
            line_end=max_end,
        ),
        max_end,
        warnings,
    )


# ---------------------------------------------------------------------------
# Multi-chain extractor with enclosing-context tracking.
# ---------------------------------------------------------------------------


def _extract_chains(
    lines: List[str],
    start: int,
    end: int,
) -> Tuple[List[ConditionalChain], List[str]]:
    """Extract all ``docker_container_name`` conditional chains from
    ``lines[start]`` through ``lines[end]`` (inclusive).

    A chain is detected when:
      - A ``{%- if docker_container_name …%}`` line is found (primary), **or**
      - A ``{%- if <non-dcn> %}`` line whose chain later contains a
        ``{%- elif docker_container_name …%}`` (secondary -- covers the
        ``stop_time is defined`` case in ``stop()``).

    Each chain's ``enclosing_conditions`` is populated with any non-dcn Jinja
    ``{%- if %}`` blocks that were open at the point the chain started.
    """
    chains: List[ConditionalChain] = []
    warnings: List[str] = []
    non_dcn_stack: List[str] = []
    i = start

    while i <= end:
        line = lines[i]

        # --- dcn ``if`` → start a primary chain ---------------------------
        if _is_dcn_if(line):
            chain, chain_end, w = _parse_one_chain(lines, i, end)
            chain.enclosing_conditions = list(non_dcn_stack)
            chains.append(chain)
            warnings.extend(w)
            i = chain_end + 1
            continue

        # --- non-dcn ``if`` → might be a secondary chain (has dcn elif) ---
        if _has_jinja_if(line) and not _is_dcn_if(line):
            if _chain_has_dcn_elif(lines, i, end):
                chain, chain_end, w = _parse_one_chain(lines, i, end)
                chain.enclosing_conditions = list(non_dcn_stack)
                chains.append(chain)
                warnings.extend(w)
                i = chain_end + 1
                continue
            # Plain non-dcn if -- push onto the enclosing-context stack.
            non_dcn_stack.append(line.rstrip("\n").strip())
            i += 1
            continue

        # --- endif for a non-dcn wrapping if → pop context ----------------
        if _has_jinja_endif(line) and non_dcn_stack:
            non_dcn_stack.pop()
            i += 1
            continue

        # --- elif / else for a non-dcn wrapping if → just skip ------------
        i += 1

    return chains, warnings


def _chain_has_dcn_elif(
    lines: List[str], start: int, max_end: int
) -> bool:
    """Peek ahead from a non-dcn ``{%- if %}`` at *start* to check whether
    any ``elif`` at depth 0 references ``docker_container_name``.  Used to
    detect the ``stop_time is defined`` secondary-chain pattern without
    consuming lines (the real parse is done by ``_parse_one_chain``)."""
    depth = 0
    for i in range(start + 1, max_end + 1):
        line = lines[i]
        if _has_jinja_if(line):
            depth += 1
        elif _has_jinja_endif(line):
            if depth > 0:
                depth -= 1
            else:
                return False  # chain closed without dcn elif
        elif depth == 0 and _is_dcn_elif(line):
            return True
    return False


# ---------------------------------------------------------------------------
# Shared (non-conditional) code extractor.
# ---------------------------------------------------------------------------


def _non_chain_text(
    lines: List[str],
    start: int,
    end: int,
    chains: List[ConditionalChain],
    position: str,
) -> str:
    """Return the concatenated non-chain lines in a section, either
    ``"before"`` (lines before the first chain) or ``"after"`` (lines after
    the last chain)."""
    if not chains:
        return "\n".join(lines[start : end + 1])
    if position == "before":
        return "\n".join(lines[start : chains[0].line_start])
    # "after"
    return "\n".join(lines[chains[-1].line_end + 1 : end + 1])


# ---------------------------------------------------------------------------
# Section/region finder.
# ---------------------------------------------------------------------------

# Key function names whose definitions we use as section boundaries.
_KEY_FUNC_NAMES = frozenset(
    {"preStartAction", "postStartAction", "start", "wait", "stop", "kill"}
)


def _find_section_ranges(
    lines: List[str],
) -> Tuple[Dict[str, Tuple[int, int]], Optional[Tuple[int, int]], List[str]]:
    """Locate the line ranges for each template section.

    Returns ``(sections, docker_create_range, warnings)`` where:
      - ``sections`` maps section name → ``(start_line, end_line)`` inclusive
      - ``docker_create_range`` is ``(start_line, end_line)`` for the
        ``docker create`` command within ``start()`` (or ``None``)
    """
    warnings: List[str] = []
    func_lines: Dict[str, int] = {}

    for i, line in enumerate(lines):
        s = line.strip()
        m = re.match(r"^function\s+(\w+)\s*\(\)", s)
        if m and m.group(1) in _KEY_FUNC_NAMES:
            func_lines[m.group(1)] = i
            continue
        m = re.match(r"^(\w+)\s*\(\)\s*\{", s)
        if m and m.group(1) in _KEY_FUNC_NAMES:
            func_lines[m.group(1)] = i
            continue

    # Find DOCKERNAME= (marks end of function definitions, start of main body)
    main_body_line: Optional[int] = None
    for i, line in enumerate(lines):
        if "DOCKERNAME={{docker_container_name}}" in line:
            main_body_line = i
            break

    # Ordered key functions as they appear in the template
    ordered = sorted(func_lines.items(), key=lambda kv: kv[1])

    sections: Dict[str, Tuple[int, int]] = {}

    # top_level: line 0 to just before first key function
    if ordered:
        sections["top_level"] = (0, ordered[0][1] - 1)

    # Each key function's section extends to just before the next one
    for idx, (name, start_line) in enumerate(ordered):
        if idx + 1 < len(ordered):
            end_line = ordered[idx + 1][1] - 1
        elif main_body_line is not None:
            end_line = main_body_line - 1
        else:
            end_line = len(lines) - 1
        sections[name] = (start_line, end_line)

    # Split preStartAction section into body + inter-helpers.
    # preStartAction's body closes at the first ``}`` on its own line after
    # the function's opening ``{``.
    if "preStartAction" in sections:
        ps_start, ps_end = sections["preStartAction"]
        func_body_start = None
        func_body_end = None
        for j in range(ps_start, ps_end + 1):
            if lines[j].strip() == "{" and func_body_start is None:
                func_body_start = j
            elif (
                lines[j].rstrip() == "}"
                and func_body_start is not None
                and func_body_end is None
            ):
                func_body_end = j
                break
        if func_body_start is not None and func_body_end is not None:
            sections["preStartAction"] = (func_body_start + 1, func_body_end - 1)
            if func_body_end + 1 <= ps_end:
                sections["inter_helpers"] = (func_body_end + 1, ps_end)
        else:
            warnings.append(
                "could not locate preStartAction's function body braces"
            )

    # Similarly extract postStartAction body.
    if "postStartAction" in sections:
        pa_start, pa_end = sections["postStartAction"]
        func_body_start = None
        func_body_end = None
        for j in range(pa_start, pa_end + 1):
            if lines[j].strip() == "{" and func_body_start is None:
                func_body_start = j
            elif (
                lines[j].rstrip() == "}"
                and func_body_start is not None
                and func_body_end is None
            ):
                func_body_end = j
                break
        if func_body_start is not None and func_body_end is not None:
            sections["postStartAction"] = (func_body_start + 1, func_body_end - 1)
        else:
            warnings.append(
                "could not locate postStartAction's function body braces"
            )

    # Locate ``docker create`` within start() to split that section.
    docker_create_range: Optional[Tuple[int, int]] = None
    if "start" in sections:
        s_start, s_end = sections["start"]
        dc_line: Optional[int] = None
        dc_end_line: Optional[int] = None
        for j in range(s_start, s_end + 1):
            if _DOCKER_CREATE_RE.search(lines[j]) and dc_line is None:
                dc_line = j
            if dc_line is not None and "|| {" in lines[j]:
                # Find the ``}`` closing the error handler.
                for k in range(j + 1, min(j + 10, s_end + 1)):
                    if lines[k].strip() == "}":
                        dc_end_line = k
                        break
                if dc_end_line is None:
                    dc_end_line = j
                break
        if dc_line is not None and dc_end_line is not None:
            docker_create_range = (dc_line, dc_end_line)

    return sections, docker_create_range, warnings


# ---------------------------------------------------------------------------
# Stop-timeout extractor.
# ---------------------------------------------------------------------------


def _extract_stop_timeouts(
    lines: List[str], start: int, end: int
) -> List[StopTimeout]:
    """Scan ``lines[start:end+1]`` (the ``stop()`` section) for explicit
    ``container stop -t <N>`` timeouts adjacent to
    ``docker_container_name`` branches.

    Returns a :class:`StopTimeout` for each branch that carries a timeout.
    """
    timeouts: List[StopTimeout] = []
    i = start
    while i <= end:
        line = lines[i]
        branch = _parse_dcn_branch(line)
        if branch is not None:
            for j in range(i + 1, min(i + 6, end + 1)):
                m = _STOP_TIMEOUT_RE.search(lines[j])
                if m:
                    timeouts.append(
                        StopTimeout(
                            container_names=branch.container_names,
                            timeout_seconds=int(m.group(1)),
                            extra_condition=branch.extra_condition,
                        )
                    )
                    break
                if _has_jinja_elif(lines[j]) or _has_jinja_else(lines[j]) or _has_jinja_endif(lines[j]):
                    break
        i += 1
    return timeouts


# ---------------------------------------------------------------------------
# HookPoint builder.
# ---------------------------------------------------------------------------


def _build_hook_point(
    lines: List[str], start: int, end: int
) -> Tuple[HookPoint, List[str]]:
    """Build a :class:`HookPoint` from ``lines[start:end+1]``."""
    chains, warnings = _extract_chains(lines, start, end)
    raw = "\n".join(lines[start : end + 1])
    before = _non_chain_text(lines, start, end, chains, "before")
    after = _non_chain_text(lines, start, end, chains, "after")
    return (
        HookPoint(
            chains=chains,
            shared_code_before=before,
            shared_code_after=after,
            raw_text=raw,
        ),
        warnings,
    )


# ---------------------------------------------------------------------------
# Public API.
# ---------------------------------------------------------------------------


def parse_ctl_template(sonic_root: Path) -> CtlParseResult:
    """Parse ``files/build_templates/docker_image_ctl.j2`` and return a
    structured :class:`CtlParseResult` with per-container blocks organised by
    hook point.

    ``sonic_root`` is the sonic-buildimage tree root.
    """
    ctl_path = (
        sonic_root / "files" / "build_templates" / "docker_image_ctl.j2"
    )
    text = ctl_path.read_text(errors="replace")
    return parse_ctl_text(text)


def parse_ctl_text(text: str) -> CtlParseResult:
    """Parse raw template text (testable without a real tree)."""
    lines = text.split("\n")
    warnings: List[str] = []

    sections, docker_create_range, section_warnings = _find_section_ranges(
        lines
    )
    warnings.extend(section_warnings)

    all_names: Set[str] = set()

    def _build(section_name: str) -> HookPoint:
        if section_name not in sections:
            warnings.append(
                f"section {section_name!r} not found in template"
            )
            return HookPoint()
        start, end = sections[section_name]
        hp, w = _build_hook_point(lines, start, end)
        warnings.extend(w)
        all_names.update(hp.all_container_names())
        return hp

    top_level_helpers = _build("top_level")
    pre_start_action = _build("preStartAction")
    inter_function_helpers = _build("inter_helpers")
    post_start_action = _build("postStartAction")

    # start() is split into pre-create, docker-create, post-create.
    start_pre_create = HookPoint()
    docker_create_flags = HookPoint()
    start_post_create = HookPoint()

    if "start" in sections:
        s_start, s_end = sections["start"]
        if docker_create_range is not None:
            dc_start, dc_end = docker_create_range
            if dc_start > s_start:
                start_pre_create, w = _build_hook_point(
                    lines, s_start, dc_start - 1
                )
                warnings.extend(w)
                all_names.update(start_pre_create.all_container_names())
            docker_create_flags, w = _build_hook_point(
                lines, dc_start, dc_end
            )
            warnings.extend(w)
            all_names.update(docker_create_flags.all_container_names())
            if dc_end < s_end:
                start_post_create, w = _build_hook_point(
                    lines, dc_end + 1, s_end
                )
                warnings.extend(w)
                all_names.update(start_post_create.all_container_names())
        else:
            start_pre_create = _build("start")
            warnings.append(
                "could not locate 'docker create {{docker_image_run_opt}}' "
                "within start() -- all start() chains reported under "
                "start_pre_create"
            )

    wait_body = _build("wait")
    stop_body = _build("stop")
    kill_body = _build("kill")

    # Extract stop timeouts from the stop section.
    stop_timeouts: List[StopTimeout] = []
    if "stop" in sections:
        s_start, s_end = sections["stop"]
        stop_timeouts = _extract_stop_timeouts(lines, s_start, s_end)

    return CtlParseResult(
        top_level_helpers=top_level_helpers,
        pre_start_action=pre_start_action,
        inter_function_helpers=inter_function_helpers,
        post_start_action=post_start_action,
        start_pre_create=start_pre_create,
        docker_create_flags=docker_create_flags,
        start_post_create=start_post_create,
        wait_body=wait_body,
        stop_body=stop_body,
        kill_body=kill_body,
        stop_timeouts=stop_timeouts,
        all_container_names=frozenset(all_names),
        warnings=warnings,
    )


# ---------------------------------------------------------------------------
# Standalone smoke test.
#
#   python3 -m mega_gen.ctl_parser \
#       --sonic-root /path/to/sonic-buildimage
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    import argparse
    import sys

    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--sonic-root", required=True, type=Path)
    args = ap.parse_args()

    result = parse_ctl_template(args.sonic_root)

    hook_names = [
        ("top_level_helpers", result.top_level_helpers),
        ("pre_start_action", result.pre_start_action),
        ("inter_function_helpers", result.inter_function_helpers),
        ("post_start_action", result.post_start_action),
        ("start_pre_create", result.start_pre_create),
        ("docker_create_flags", result.docker_create_flags),
        ("start_post_create", result.start_post_create),
        ("wait_body", result.wait_body),
        ("stop_body", result.stop_body),
        ("kill_body", result.kill_body),
    ]

    for name, hp in hook_names:
        print(f"\n=== {name} ({len(hp.chains)} chain(s)) ===")
        for ci, chain in enumerate(hp.chains):
            enclosing = (
                f"  enclosing: {chain.enclosing_conditions}"
                if chain.enclosing_conditions
                else ""
            )
            print(
                f"  chain[{ci}] lines {chain.line_start + 1}-{chain.line_end + 1}: "
                f"{len(chain.branches)} branch(es)"
                f"{', else' if chain.else_text is not None else ''}"
                f"{', ' + str(len(chain.non_container_branches)) + ' non-dcn' if chain.non_container_branches else ''}"
                f"{enclosing}"
            )
            for bi, br in enumerate(chain.branches):
                cond_note = f" and {br.extra_condition}" if br.extra_condition else ""
                neg_note = "NOT " if br.negated else ""
                print(
                    f"    branch[{bi}]: {neg_note}{sorted(br.container_names)}{cond_note}"
                    f" ({len(br.text.splitlines())} lines)"
                )
            for ni, (hdr, body) in enumerate(chain.non_container_branches):
                print(
                    f"    non-dcn[{ni}]: {hdr.strip()[:80]}"
                    f" ({len(body.splitlines())} lines)"
                )
        if hp.shared_code_after.strip():
            preview = hp.shared_code_after.strip().splitlines()[0][:80]
            print(f"  shared_after: {preview!r} ...")
        containers = hp.all_container_names()
        if containers:
            print(f"  containers: {sorted(containers)}")

    print(f"\n=== stop_timeouts ({len(result.stop_timeouts)}) ===")
    for st in result.stop_timeouts:
        cond = f" (when {st.extra_condition})" if st.extra_condition else ""
        print(
            f"  {sorted(st.container_names)}: {st.timeout_seconds}s{cond}"
        )
    print(f"  max: {result.max_stop_timeout}s")

    print(f"\n=== all_container_names ({len(result.all_container_names)}) ===")
    print(f"  {sorted(result.all_container_names)}")

    print(f"\n=== warnings ({len(result.warnings)}) ===")
    for w in result.warnings:
        print(f"  WARNING: {w}")

    # Quick check: blocks_for() on key containers
    print("\n=== blocks_for() spot check ===")
    for container in ("database", "swss", "snmp", "teamd", "eventd", "bgp"):
        for hook_name, hp in hook_names:
            blocks = hp.blocks_for(container)
            if blocks:
                total_lines = sum(len(b.splitlines()) for b in blocks)
                print(
                    f"  {container}/{hook_name}: "
                    f"{len(blocks)} block(s), {total_lines} total lines"
                )
