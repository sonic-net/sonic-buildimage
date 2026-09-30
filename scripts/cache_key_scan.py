#!/usr/bin/env python3
"""
cache_key_scan.py - comprehensive, evidence-based classifier for build-environment
variables versus the SONiC dpkg build-cache key.

This is the "source of truth" behind Check 10 of audit_dep_completeness.sh. It is a
WHOLE-TREE scan of GIT-TRACKED source (no build artifacts, no name heuristics, no
shortcuts) that assigns every exported build-environment variable to exactly one
disposition, justified by concrete code evidence.

PRODUCERS (where a build-env variable is set for child processes/recipes):
  the entire superproject make-orchestration layer - slave.mk, Makefile,
  Makefile.work, and every tracked *.mk (rules/*, platform/**). Both `export VAR`
  and inline `VAR=val ... $(MAKE)` environment passing are collected. Submodule-
  internal Makefiles are NOT producers: their own exports are part of that
  package's recipe content, already hashed into its cache key.

CONSUMERS (where a variable is dereferenced during a cached build):
  every build recipe across the whole tree INCLUDING submodules - each package's
  debian/rules, its build Makefile(s), Dockerfile*.j2 templates, and build shell
  scripts (*.sh). A "consumer" must actually dereference the variable ($(VAR),
  ${VAR} or $VAR); a bare textual mention in a comment does not count (comments
  are stripped before matching).

DISPOSITIONS:
  in-key    Variable is literally part of a cache key: in SONIC_COMMON_FLAGS_LIST
            or some package's *_DEP_FLAGS. A change flips the key. PROVABLY SAFE.
  filename  Variable's value flows (transitively) into a cached target's file NAME
            (*.deb/*.whl/*.gz). The name is part of the key. PROVABLY SAFE. Only
            the filename token itself is inspected - not prerequisites or recipe
            bodies on the same line - so this cannot false-clear.
  gap       Dereferenced by a cached build recipe but neither in-key nor filename.
            Changing it alters the artifact without flipping the key. REPORTED.
  assembly  Dereferenced only in the make/assembly layer, not in a cached recipe.
            NOT provably safe (an assembly-layer var can still reach a cached
            `docker build`). REPORTED.
  external-env  Exported but never dereferenced in-repo: read straight from the
            environment by an external tool (docker/cargo/dpkg). Static analysis
            cannot see inside those tools. REPORTED.

Only in-key and filename are machine-provably cache-safe and auto-clear. Every
other exported variable is a finding (default-deny). A human clears a specific
variable by recording it - WITH A REASON - in cache_key_export_waivers.tsv.

Usage:
  cache_key_scan.py            Write scripts/cache_key_export_registry.tsv snapshot
                               and print a distribution summary.
  cache_key_scan.py --stdout   Emit "variable<TAB>disposition<TAB>evidence" rows to
                               stdout (consumed live by the audit gate). No file I/O.

Enumeration is driven by `git ls-files`, so the scan sees committed source only and
runs in a few seconds regardless of how dirty the working tree is.
"""
import os
import re
import sys
import collections
import subprocess

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir))
REGISTRY = os.path.join(os.path.dirname(__file__), "cache_key_export_registry.tsv")

# A dereference of a make/shell/docker variable: $(VAR), ${VAR} or $VAR.
REF_RE = re.compile(r"\$\((\w+)\)|\$\{(\w+)\}|\$(\w+)")
# A cached-target filename token: a contiguous run ending in .deb/.whl/.gz that may
# embed $(VAR)/${VAR} references. It stops at whitespace and rule/recipe separators
# (: ; = | & < > quotes) so it captures ONLY the artifact name - never a
# prerequisite or a recipe command on the same line - while still spanning the
# parentheses of an embedded $(VAR).
FN_TOKEN_RE = re.compile(r"[^\s:;=|&<>'\"]*\.(?:deb|whl|gz)\b")
# A make variable reference in EITHER syntax: $(VAR) or ${VAR}. Package .mk files
# freely mix both (e.g. `foo_${VER}_${ARCH}.deb`), so var-ref extraction must not
# be paren-only or it silently misses curly-brace refs and reports false gaps.
VARREF_RE = re.compile(r"\$[({]\s*(\w+)\s*[})]")
# An `export` directive. Make supports two forms:
#   list form       `export A B C`      -> exports EVERY listed name
#   assignment form `export A := v`      -> exports the single assigned name
# (shell `export A=v` inside a recipe is the assignment form too). Capture the
# whole tail after `export`; exported_vars() splits list vs assignment. A single
# capture group + findall previously kept only the FIRST name, silently dropping
# the rest of a list like `export FRR FRR_DBG FRR_SNMP`.
EXPORT_RE = re.compile(r"^[ \t]*export[ \t]+(.+)$", re.M)
# Assignment operators. `_ASSIGN_OP` is any operator (used only to detect that a
# tail is an assignment vs a bare `export A B C` list). `_MAKE_OP` are the
# make-ONLY operators: for these GNU Make takes the ENTIRE rest of the line as the
# value (`export A := v B=2` exports only A), so they are always single-var. Only
# a plain `=` is ambiguous: `NAME = value` (space before `=`) is a make assignment
# (single var, rest-of-line value), while `NAME=value` (no space) is shell and may
# be a multi-assign (`export A=1 B=2`), so only that form is walked. A VALUE is one
# shell word: quoted spans (`"..."`/`'...'`), `$(...)`/`${...}`, and backslash
# escapes (`\ `) are consumed whole so their internal spaces do not end the word
# and their internal text never leaks as a name; the walk resumes at the next
# `NAME=` after unescaped/unquoted whitespace.
_ASSIGN_OP = r"(?:::=|:=|\?=|\+=|!=|=)"
_MAKE_OP = r"(?:::=|:=|\?=|\+=|!=)"
_VALUE = r'(?:"[^"]*"|' + r"'[^']*'" + r"|\$\([^)]*\)|\$\{[^}]*\}|\\.|\S)*"
ASSIGN_HEAD_RE = re.compile(r"^[A-Za-z_]\w*[ \t]*" + _ASSIGN_OP)
ASSIGN_SINGLE_RE = re.compile(r"^([A-Za-z_]\w*)(?:[ \t]*" + _MAKE_OP + r"|[ \t]+=)")
ASSIGN_STEP_RE = re.compile(r"[ \t]*([A-Za-z_]\w*)=" + _VALUE)
ASSIGN_RE = re.compile(r"^\s*([A-Za-z_]\w*)\s*[:?+]?=\s*(.*)$", re.M)
# Unconditional (re)assignment of a variable inside a recipe: `VAR = x` / `VAR := x`
# (make), `export VAR=x` / `override VAR := x` (make), or `VAR=x` (shell). Such a
# variable is SHADOWED - the recipe forces its own value (a plain make assignment
# overrides the inherited environment), so the recipe is not a consumer of the
# export. `?=` (use-env-if-set) and `+=` (append to the inherited value) still
# consume, so they are excluded from this pattern.
OVERRIDE_RE = re.compile(r"^\s*(?:export\s+|override\s+)*([A-Za-z_]\w*)\s*:?=", re.M)


def read(path):
    try:
        with open(os.path.join(REPO_ROOT, path), encoding="utf-8", errors="ignore") as fh:
            return fh.read()
    except OSError:
        return ""


def git_files(recurse=False):
    """Git-tracked files. A git failure here silently yielding [] would let the
    scan run on incomplete inputs and emit misleading classifications, so a
    failure is surfaced instead of swallowed:
      - base `git ls-files` failing is fatal (broken repo / not a work tree).
      - `--recurse-submodules` failing (uninitialized submodules, older git)
        falls back to the non-recursive listing with a warning, so the
        superproject is still scanned rather than the whole tree vanishing."""
    def _run(cmd):
        return subprocess.run(cmd, cwd=REPO_ROOT, capture_output=True,
                              text=True, check=True).stdout.splitlines()
    base = ["git", "ls-files"]
    if not recurse:
        try:
            return _run(base)
        except (OSError, subprocess.CalledProcessError) as e:
            sys.stderr.write("cache_key_scan: FATAL: `git ls-files` failed "
                             "(not a git work tree?): %s\n" % e)
            raise
    try:
        return _run(base + ["--recurse-submodules"])
    except (OSError, subprocess.CalledProcessError) as e:
        sys.stderr.write("cache_key_scan: WARNING: `git ls-files "
                         "--recurse-submodules` failed (%s); falling back to a "
                         "non-recursive scan — submodule recipes will be "
                         "MISSED, classifications may be incomplete.\n" % e)
        return git_files(recurse=False)


def strip_comments(text):
    """Remove '#' comments (Makefile/shell/Dockerfile) and Jinja {# #} so that a
    variable mentioned only in a comment is never counted as a real reference.
    Errs toward removal; over-stripping can only downgrade a finding's severity,
    never falsely clear one."""
    text = re.sub(r"\{#.*?#\}", " ", text, flags=re.S)   # jinja comments
    out = []
    for line in text.splitlines():
        h = line.find("#")
        out.append(line if h < 0 else line[:h])
    return "\n".join(out)


def refs_in(text):
    """Set of variable names dereferenced ($(V)/${V}/$V) in text."""
    return {m.group(1) or m.group(2) or m.group(3) for m in REF_RE.finditer(text)}


def logical_lines(text):
    """Yield make 'logical lines' with backslash-continuations joined, so a
    multi-line assignment (e.g. SONIC_COMMON_FLAGS_LIST or a _DEP_FLAGS spanning
    several `\\`-continued lines) is treated as one line. Without this, vars on
    continuation lines are missed and falsely reported as gaps."""
    buf = ""
    for line in text.splitlines():
        if line.endswith("\\"):
            buf += line[:-1] + " "
        else:
            yield buf + line
            buf = ""
    if buf:
        yield buf


# ---- producers -------------------------------------------------------------
def _src_root_patterns(candidates):
    """Regexes matching every cached package's SOURCE root, derived from the
    `$(PKG)_SRC_PATH = ...` declarations in the make-layer. Files under a source
    root are package content (hashed by GIT_CONTENT_SHA); a variable whose
    producer AND consumer both live inside the same source tree is intra-package
    flow, NOT a build-env injection, so such files must be excluded from the
    producer set. Cross-boundary injections (a superproject .mk exporting into a
    package build) are unaffected because the superproject .mk is not under a
    source root. Roots shallower than a concrete package subdirectory (e.g. a
    bare $(PLATFORM_PATH)) are skipped so sibling build-rule .mk are not excluded."""
    SENT = "\x01"   # placeholder for an unresolved make-var path segment
    pats = []
    for f in candidates:
        for m in re.finditer(r"_SRC_PATH\s*[:+]?=\s*(\S+)", read(f)):
            val = m.group(1)
            val = val.replace("$(SRC_PATH)", "src")
            val = re.sub(r"\$\(PLATFORM(?:_PDDF|_COMMON)?_PATH\)", "platform/" + SENT, val)
            prev = None
            while prev != val:                      # collapse remaining (possibly nested) make vars
                prev = val
                val = re.sub(r"\$\([^()]*\)", SENT, val)
            segs = [s for s in val.split("/") if s]
            if len(segs) < 2 or SENT in segs[-1]:   # need a concrete package subdir at the leaf
                continue
            rx = re.escape(val).replace(re.escape(SENT), "[^/]+")
            pats.append(re.compile("^" + rx + "/"))
    return pats


def producer_candidates():
    """All make-layer files (root orchestration + every tracked .mk/Makefile),
    BEFORE narrowing out package sources. Used for filename capture: an artifact
    file NAME may be composed in a package-internal Makefile, and a variable whose
    value flows into a cached *.deb/*.whl/*.gz name is cache-safe wherever that
    name is defined."""
    out = []
    for f in git_files(recurse=False):
        base = os.path.basename(f)
        if f.endswith(".mk") or base in ("slave.mk", "Makefile", "Makefile.work"):
            out.append(f)
    return out


def producer_files():
    candidates = producer_candidates()
    src_roots = _src_root_patterns(candidates)
    return [f for f in candidates if not any(p.match(f) for p in src_roots)]


def exported_vars(text):
    """Every exported build-env variable: `export VAR` plus inline `VAR=val $(MAKE)`
    command-line environment passed to sub-makes. Handles:
      list form        `export A B C`          -> all names
      make assignment  `export A := v`         -> the single LHS name (any of the
                       `=` `:=` `::=` `?=` `+=` `!=` operators, with or without
                       surrounding spaces, e.g. `export A+=v`)
      shell assignment `export A=1 B=2`        -> every LHS name
    A dynamic expansion `export $(fn ...)` produces its name(s) at make time; the
    literal names are not statically knowable and tokenizing the expansion body
    yields garbage (function/flag tokens), so such lines are skipped.

    Assignment values are NOT tokenized on whitespace: a naive split leaks bare
    words out of quoted/`$(...)` values (`export FOO="a b"` -> phantom `b`;
    `export FOO="$(shell basename ...)"` -> phantom `basename`) and misses the
    LHS of no-space operator forms. Instead the leading `NAME<op>VALUE` run is
    walked with a shell-word VALUE (quoted spans and `$(...)`/`${...}` consumed
    whole), so only real LHS names are captured while a later assignment in a
    shell multi-assign (`export A="a b" B=2`) is still reached."""
    names = set()
    for tail in EXPORT_RE.findall(text):
        tail = tail.strip()
        if tail.startswith("$"):
            continue  # `export $(fn ...)` / `export ${VAR}` — dynamic, no literal names
        if ASSIGN_HEAD_RE.match(tail):
            m0 = ASSIGN_SINGLE_RE.match(tail)
            if m0:
                # make-only operator (value is the rest of the line) or `NAME = v`
                # (space before plain `=`, a make assignment): a single LHS.
                names.add(m0.group(1))
            else:
                # `NAME=value` with no space before a plain `=`: shell-style, may be
                # a multi-assign (`A=1 B=2`). Walk each `NAME=VALUE`; VALUE consumes
                # quoted/`$(...)`/escaped spans whole so a later assignment is still
                # reached and no inner word leaks as a name.
                pos = 0
                while pos < len(tail):
                    m = ASSIGN_STEP_RE.match(tail, pos)
                    if not m:
                        break
                    names.add(m.group(1))
                    pos = m.end()
        else:
            for t in tail.split():             # list form `export A B C`
                if re.fullmatch(r"[A-Za-z_]\w*", t):
                    names.add(t)
    for line in text.splitlines():
        if "$(MAKE)" in line:
            head = line.split("$(MAKE)", 1)[0]
            names |= set(re.findall(r"\b([A-Z_][A-Z0-9_]*)=", head))
    return sorted(names)


# ---- in-key ----------------------------------------------------------------
def tracked_vars():
    mk = read("Makefile.cache")
    flags = set()
    for line in logical_lines(mk):
        if "SONIC_COMMON_FLAGS_LIST" in line:
            flags |= set(VARREF_RE.findall(line))
    for dep in git_files(recurse=False):
        if dep.endswith(".dep"):
            for line in logical_lines(read(dep)):
                if re.search(r"DEP_FLAGS\s*[:+?]?=", line):
                    flags |= set(VARREF_RE.findall(line))
    return flags


# ---- filename (token-scoped, transitive) -----------------------------------
def filename_captured(producer_text_values):
    """Vars whose value is (part of) a cached target's file NAME - so a change flips
    the cache key. Seeds are:
      (a) $(VAR) refs that appear INSIDE a *.deb/*.whl/*.gz token, and
      (b) a variable A whose assignment RHS *is* an artifact filename, e.g.
          A = foo-dbgsym_$(VER)_$(ARCH).deb  ->  A's value is a target name.
    Then it propagates through assignments: if a captured var A is defined as
    `A = ... $(V) ...`, V is a component of A's value and is captured too. A
    dangerous plain env var is never assigned a .deb/.whl/.gz name, so this cannot
    false-clear the DOCKER_DEFAULT_PLATFORM class."""
    seed = set()
    contributes = collections.defaultdict(set)  # A -> vars referenced in A's RHS
    for t in producer_text_values:
        for line in t.splitlines():
            for tok in FN_TOKEN_RE.findall(line):
                seed |= set(VARREF_RE.findall(tok))
        for m in ASSIGN_RE.finditer(t):
            lhs, rhs = m.group(1), m.group(2)
            contributes[lhs] |= set(VARREF_RE.findall(rhs))
            if FN_TOKEN_RE.search(rhs):     # RHS is itself an artifact filename
                seed.add(lhs)
    captured = set(seed)
    stack = list(seed)
    while stack:
        a = stack.pop()
        for v in contributes.get(a, ()):
            if v not in captured:
                captured.add(v)
                stack.append(v)
    return captured


# ---- consumers (recipes, whole tree incl. submodules) ----------------------
def recipe_files():
    files = []
    for f in git_files(recurse=True):
        base = os.path.basename(f)
        if base == "rules" and "/debian/" in ("/" + f):
            files.append(f)
        elif base == "Makefile" and f != "Makefile":          # exclude top orchestration
            files.append(f)
        elif re.search(r"Dockerfile.*\.j2$", base):
            files.append(f)
        elif base.endswith(".sh"):
            files.append(f)
    return files


def recipe_consumers(residual, recipes):
    """{var: example_recipe_file} for residual vars actually DEREFERENCED by a cached
    build recipe (comment-stripped; $(V)/${V}/$V only). A recipe that unconditionally
    re-assigns the variable is SHADOWED and does not count - it uses its own value,
    not the inherited export."""
    resid = set(residual)
    if not resid:
        return {}
    example = {}
    for f in recipes:
        t = strip_comments(read(f))
        refs = refs_in(t) & resid
        if not refs:
            continue
        overridden = set(OVERRIDE_RE.findall(t))
        for v in refs - overridden:
            example.setdefault(v, f)
        if len(example) == len(resid):
            break
    return example


# ---- classify --------------------------------------------------------------
def classify():
    prod = producer_files()
    text = {f: strip_comments(read(f)) for f in prod}
    values = list(text.values())
    alltext = "\n".join(values)

    exp = exported_vars(alltext)
    tracked = tracked_vars()
    captured = filename_captured([strip_comments(read(f)) for f in producer_candidates()])
    make_refs = refs_in(alltext)

    residual = [v for v in exp if v not in tracked and v not in captured]
    examples = recipe_consumers(residual, recipe_files())

    rows = []
    for v in exp:
        if v in tracked:
            rows.append((v, "in-key", ""))
        elif v in captured:
            rows.append((v, "filename", "target-name"))
        elif v in examples:
            rows.append((v, "gap", examples[v]))
        elif v in make_refs:
            rows.append((v, "assembly", "make-layer only"))
        else:
            rows.append((v, "external-env", "no in-repo consumer"))
    return rows


def main():
    rows = classify()
    if "--stdout" in sys.argv[1:]:
        for v, d, e in sorted(rows):
            print(f"{v}\t{d}\t{e}")
        return 0
    counts = collections.Counter(d for _, d, _ in rows)
    with open(REGISTRY, "w", encoding="utf-8") as out:
        out.write("# cache_key_export_registry.tsv - SNAPSHOT generated by scripts/cache_key_scan.py\n")
        out.write("# Informational audit snapshot only. The gate classifies LIVE on every run;\n")
        out.write("# human waivers live in scripts/cache_key_export_waivers.tsv.\n")
        out.write("# variable\tdisposition\tevidence\n")
        for v, d, e in sorted(rows):
            out.write(f"{v}\t{d}\t{e}\n")
    print(f"wrote {REGISTRY} ({len(rows)} exports)")
    for d, n in counts.most_common():
        print(f"  {d:14}{n}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
