#!/usr/bin/env python3
"""Recognize optional RFS file inputs with both content and presence in the key.

This deliberately supports a small, structural pattern, not arbitrary Make or
shell evaluation. Unknown expressions and additional content uses remain findings.
"""

from pathlib import Path
import re
import sys


NAME = r"[A-Za-z_]\w*"


def file_key_inputs(cache, consumer, keyed_flags, recipe, environment=""):
    """Return file-source variables proven covered by the staged-file pattern."""
    cache = re.sub(r"\\\n[ \t]*", " ", cache)
    lines = cache.splitlines()
    assignments = {}
    conditions = []
    for position, line in enumerate(lines):
        if line.startswith("\t") or line.lstrip().startswith("#"):
            continue
        line = line.split("#", 1)[0].strip()
        if re.match(r"if(?:n?eq|n?def)\b", line):
            conditions.append(line)
        elif line == "endif":
            if conditions:
                conditions.pop()
        elif line.startswith("else"):
            if conditions:
                conditions[-1] = "else"
        else:
            match = re.fullmatch(r"(" + NAME + r")\s*(:=|\+=|=|\?=)\s*(.*)", line)
            if match:
                name, operator, value = match.groups()
                assignments.setdefault(name, []).append(
                    (operator, value, tuple(conditions), position)
                )

    prefix = "$(foreach pkg, $(SONIC_RFS_TARGETS), "
    rfs_blocks = [
        (position, line[len(prefix):])
        for position, line in enumerate(lines) if line.startswith(prefix)
    ]
    if len(rfs_blocks) != 1:
        return set()
    wiring_position, wiring = rfs_blocks[0]
    if ("$(eval $(pkg)_DEP_FILES := $(SONIC_COMMON_BASE_FILES_LIST) $(RFS_DEP_FILES))"
            not in wiring or
            "$(eval $(pkg)_CACHE_MODE := GIT_CONTENT_SHA)" not in wiring):
        return set()

    # A presence bit alone cannot track file bytes. Require the exact optional
    # mode + nonempty-source predicate AND a staged content dependency.
    enabled_re = re.compile(
        r'\$\(if \$\(and \$\(filter ([\w -]+),'
        r'\$\(subst ",,\$\((' + NAME + r')\)\)\),'
        r'\$\((' + NAME + r')\)\),y,n\)'
    )
    covered = set()
    for enabled in keyed_flags:
        flags = assignments.get("RFS_INLINE_ENV_FLAGS", [])
        if (len(flags) != 1 or flags[0][0] != ":=" or flags[0][2] or
                flags[0][3] >= wiring_position or
                not re.search(
                    r"(?:^|\s)(?:" + re.escape(enabled) + r"=)?"
                    + re.escape(f"$({enabled})") + r"(?:\s|$)", flags[0][1]
                )):
            continue
        definitions = assignments.get(enabled, [])
        if len(definitions) != 1:
            continue
        operator, value, guards, enabled_position = definitions[0]
        match = enabled_re.fullmatch(value)
        if (operator != ":=" or guards or not match or
                enabled_position >= flags[0][3]):
            continue
        modes, mode, source = match.groups()
        modes = modes.split()
        if (not modes or assignments.get(mode) or
                any(position > enabled_position for _, _, _, position
                    in assignments.get(source, [])) or
                not _same_recipe_source(recipe, source) or
                not _same_recipe_mode(recipe, source, mode, environment) or
                not _copy_only_consumer(consumer, source, mode, modes)):
            continue
        guard = (f"ifeq ($({enabled}),y)",)
        for staged, definitions in assignments.items():
            if len(definitions) != 1:
                continue
            operator, path, guards, staged_position = definitions[0]
            if (operator != ":=" or guards != guard or
                    enabled_position >= staged_position or
                    not re.fullmatch(r"\$\(TARGET_PATH\)/[\w./-]+", path)):
                continue
            deps = assignments.get("RFS_DEP_FILES", [])
            additions = [
                i for i, (op, val, cond, _) in enumerate(deps)
                if op == "+=" and val == f"$({staged})" and cond == guard
            ]
            if not additions or any(
                op != "+=" for op, _, _, _ in deps[additions[-1] + 1:]
            ):
                continue
            # Both += on an immediate list and the RFS eval snapshot capture
            # values in file order, as does the enabled-state conditional.
            if (deps[0][0] != ":=" or deps[0][2] or
                    any(not staged_position < deps[i][3] < wiring_position
                        for i in additions)):
                continue
            if _staging_rule(cache, staged, source, guard[0], staged_position):
                covered.add(source)
    return covered


def _staging_rule(cache, staged, source, guard, staged_position):
    quoted_source = """'$(subst ','"'"',$(%s))'""" % source
    if len(re.findall(r"^" + re.escape(f"$({staged})") + r"\s*:", cache, re.M)) != 1:
        return False
    # Match the entire conditional so extra recipes, a different source, or
    # failure-swallowing commands cannot turn a mere mention into evidence.
    rule = (
        "^" + re.escape(guard) + r"\n"
        r"(?:#[^\n]*\n)*"
        r"\.PHONY: ([\w.-]+)\n"
        r"\1:\n\n"
        + re.escape(f"$({staged})") + r": \1\n"
        r"\t@mkdir -p '\$\(@D\)'\n"
        r"\tcmp -s -- " + re.escape(quoted_source) + r" '\$@' \|\|[ \t]+"
        r"cp -- " + re.escape(quoted_source) + r" '\$@'\n"
        r"endif"
    )
    for match in re.finditer(rule, cache, re.M):
        preceding = cache[:match.start()].splitlines()
        if len(preceding) <= staged_position:
            continue
        depth = 0
        for line in preceding:
            if line.startswith("\t"):
                continue
            line = line.strip()
            if re.match(r"if(?:n?eq|n?def)\b", line):
                depth += 1
            elif line == "endif":
                depth -= 1
        if depth == 0:
            return True
    return False


def _same_recipe_source(recipe, source):
    values = re.findall(
        r"^[ \t]*" + re.escape(source) + r"=(.*?)\s*\\$", recipe, re.M
    )
    allowed = {
        f'"$({source})"',
        """'$(subst ','"'"',$(%s))'""" % source,
    }
    return bool(values) and all(value in allowed for value in values)


def _same_recipe_mode(recipe, source, mode, environment):
    allowed = {
        f"$({mode})", f'"$({mode})"',
        """'$(subst ','"'"',$(%s))'""" % mode,
    }

    def forwarded(text):
        bindings = re.findall(
            r"^[ \t]*(?:export[ \t]+)?" + re.escape(mode)
            + r"\s*([:+?]?=)[ \t]*(.*?)\s*$", text, re.M,
        )
        return bool(bindings) and all(
            operator == "=" and value.removesuffix("\\").rstrip() in allowed
            for operator, value in bindings
        )

    # Check every explicit binding, not just one elsewhere in the Makefile.
    if re.search(
        r"^[ \t]*(?:export[ \t]+)?" + re.escape(mode) + r"\s*[:+?]?=",
        recipe, re.M,
    ) and not forwarded(recipe):
        return False
    if re.search(
        r"^[ \t]*unexport(?:[ \t]*$|[^\n]*\b" + re.escape(mode) + r"\b)",
        recipe, re.M,
    ):
        return False

    inherited = forwarded(environment)
    lines = recipe.splitlines()
    binding = re.compile(r"[ \t]*(" + NAME + r")=(.*?)\s*\\[ \t]*")
    for position, line in enumerate(lines):
        match = binding.fullmatch(line)
        if not match or match[1] != source:
            continue
        start = end = position
        while start and binding.fullmatch(lines[start - 1]):
            start -= 1
        while end + 1 < len(lines) and binding.fullmatch(lines[end + 1]):
            end += 1
        explicit = any(
            binding.fullmatch(line)[1] == mode for line in lines[start:end + 1]
        )
        if not explicit and not inherited:
            return False
    return True


def _copy_only_consumer(consumer, source, mode, modes):
    reference = re.compile(
        r"\$(?:" + re.escape(source) + r"\b|\{[#!]?" + re.escape(source) + r"\b)"
    )
    nonempty = f'if [ -n "${source}" ]; then'
    exists = f'if [ ! -f "${source}" ]; then'
    # Compare the shell guard's enum with the Make filter's enum. A differently
    # gated copy would invalidate the implication "disabled => no embedded file".
    mode_test = r"\$" + re.escape(mode) + r""" == ['"]([\w-]+)['"]"""
    mode_guard = re.compile(
        r"if \[\[ (" + mode_test + r"(?: \|\| " + mode_test + r")*) \]\]; then"
    )
    guards = []
    copies = 0
    for raw in consumer.splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        # A redirected or continued group closure can capture earlier echoes.
        if re.match(r"[})][ \t]*[^#\s]", line):
            return False
        if line.startswith(("if ", "elif ")):
            # Inline bodies/closures are outside the supported block syntax.
            if re.search(r"\bthen\b(?!\s*$)|\bfi\b", line):
                return False
            if line.startswith("if "):
                guards.append(line)
            elif guards:
                guards[-1] = "else"
        elif line == "fi":
            if guards:
                guards.pop()
        elif line == "else":
            if guards:
                guards[-1] = "else"
        elif re.match(r"(?:fi|else|elif)\b", line):
            return False
        if re.match(
            r"(?:export )?(?:" + re.escape(source) + "|" +
            re.escape(mode) + r")=", line
        ):
            return False
        if not reference.search(line):
            continue
        if line in (nonempty, exists):
            continue
        if (line.startswith("echo ") and not line.endswith("\\") and
                not re.search(r"[;|&<>`]", line)):
            continue
        copy = re.fullmatch(
            r'(?:sudo )?cp "\$' + re.escape(source) +
            r'" "\$FILESYSTEM_ROOT/[\w./-]+"', line
        )
        matching_mode = any(
            mode_guard.fullmatch(guard) and
            set(re.findall(mode_test, guard)) == set(modes)
            for guard in guards
        )
        if not copy or nonempty not in guards or not matching_mode:
            return False
        copies += 1
    return copies == 1


if __name__ == "__main__":
    cache = Path(sys.argv[1]).read_text(encoding="utf-8")
    consumer = Path(sys.argv[2]).read_text(encoding="utf-8")
    recipe = Path(sys.argv[4]).read_text(encoding="utf-8")
    environment = (
        Path(sys.argv[5]).read_text(encoding="utf-8")
        if len(sys.argv) > 5 and Path(sys.argv[5]).exists() else ""
    )
    for name in sorted(file_key_inputs(
            cache, consumer, set(sys.argv[3].split()), recipe, environment)):
        print(name)
