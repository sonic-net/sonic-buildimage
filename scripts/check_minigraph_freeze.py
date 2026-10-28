#!/usr/bin/env python3
"""Compare Git snapshots for frozen parser edits and new static dependencies."""

import argparse
import ast
from collections import Counter
import json
import os
from pathlib import Path, PurePosixPath
import re
import shlex
import subprocess
import sys


GUIDE_URL = (
    "https://github.com/sonic-net/sonic-buildimage/blob/master/"
    ".github/workflows/README.md#minigraph-retirement-gate"
)
PARSER_MODULES = {"minigraph", "minigraph_ext", "minigraph_custom"}
FROZEN_FILES = {
    "src/sonic-config-engine/{}.py".format(module)
    for module in PARSER_MODULES
}
XML_PATH = re.compile(
    r"(?<![\w.])(?:[\w$./{}-]+/)?minigraph\.xml(?:[.\w-]*)"
)
COMMAND_CALLS = {
    "Popen", "call", "check_call", "check_output", "check_output_pipe",
    "command", "exec_command", "getoutput", "getstatusoutput",
    "getstatusoutput_noshell", "popen", "run", "run_command", "shell",
    "system",
}
FILE_CALLS = {
    "Path", "PurePath", "copy", "copy2", "copyfile", "exists", "is_file",
    "isfile", "move", "open", "parse", "read_bytes", "read_text", "remove",
    "unlink", "write_bytes", "write_text",
}


class AnalysisError(Exception):
    pass


def git(repo, *args):
    result = subprocess.run(
        ["git", "-C", str(repo), *args], capture_output=True, check=False
    )
    if result.returncode:
        raise AnalysisError(result.stderr.decode("utf-8", errors="replace").strip())
    return result.stdout


def qualified_name(node, imports=None):
    if isinstance(node, ast.Name):
        if imports is not None:
            return imports.get(node.id, node.id)
        return node.id
    if isinstance(node, ast.Attribute):
        return "{}.{}".format(qualified_name(node.value, imports), node.attr)
    return ""


def static_value(node, bindings, seen=()):
    """Resolve literal fragments while preserving argument sequence types."""
    if isinstance(node, ast.Constant) and isinstance(node.value, str):
        return node.value
    if isinstance(node, ast.Name) and node.id not in seen:
        values = bindings.get(node.id, [])
        if len(values) == 1:
            return static_value(values[0], bindings, (*seen, node.id))
    if isinstance(node, (ast.List, ast.Tuple)):
        values = [static_text(item, bindings, seen) for item in node.elts]
        return tuple(values) if isinstance(node, ast.Tuple) else values
    if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Add):
        left = static_value(node.left, bindings, seen)
        right = static_value(node.right, bindings, seen)
        if type(left) is type(right):
            return left + right
        return ""
    if isinstance(node, ast.JoinedStr):
        return "".join(
            static_text(value, bindings, seen)
            if isinstance(value, ast.Constant) else "${dynamic}"
            for value in node.values
        )
    if isinstance(node, ast.Call):
        if isinstance(node.func, ast.Attribute) and node.func.attr == "format":
            return static_text(node.func.value, bindings, seen)
        if qualified_name(node.func).rsplit(".", 1)[-1] in {"Path", "PurePath"}:
            return "/".join(static_text(arg, bindings, seen) for arg in node.args)
    return ""


def static_text(node, bindings, seen=()):
    value = static_value(node, bindings, seen)
    return shlex.join(value) if isinstance(value, (list, tuple)) else value


def shell_substitutions(line):
    """Extract executable substitutions without interpreting quoted literals."""
    pattern = re.compile(r"\$\(([^()]*)\)|`([^`]*)`")
    text = []
    commands = []
    quote = None
    index = 0
    while index < len(line):
        char = line[index]
        if char == "\\" and quote != "'":
            text.append(line[index:index + 2])
            index += 2
            continue
        if char in "'\"" and (quote is None or quote == char):
            quote = char if quote is None else None
        if char == "#" and quote is None and (
            index == 0 or line[index - 1].isspace() or line[index - 1] in ";&|()<>"
        ):
            text.append(line[index:])
            break
        match = pattern.match(line, index) if quote != "'" else None
        if match:
            commands.append(match[1] or match[2])
            text.append("${substitution}")
            index = match.end()
        else:
            text.append(char)
            index += 1
    return "".join(text), commands


def shell_segments(line):
    """Split operators before tokenization removes shell quotes and escapes."""
    quote = None
    start = 0
    index = 0
    while index < len(line):
        char = line[index]
        if char == "\\" and quote != "'":
            index += 2
            continue
        if char in "'\"" and (quote is None or quote == char):
            quote = char if quote is None else None
        elif quote is None:
            if char == "#" and (
                index == 0 or line[index - 1].isspace()
                or line[index - 1] in ";&|()<>"
            ):
                yield line[start:index]
                return
            if char in ";&|()<>":
                yield line[start:index]
                start = index + 1
        index += 1
    yield line[start:]


def yaml_command(value):
    """Remove an enclosing YAML scalar quote, not quotes within the shell."""
    if value.startswith("'"):
        match = re.fullmatch(r"'((?:[^']|'')*)'\s*(?:#.*)?", value)
        if match:
            return match[1].replace("''", "'")
        raise AnalysisError("Cannot decode a single-quoted YAML command")
    if value.startswith('"'):
        try:
            command, end = json.JSONDecoder().raw_decode(value)
        except ValueError as error:
            raise AnalysisError(
                "Cannot decode a double-quoted YAML command: {}".format(error)
            ) from error
        if value[end:].strip() and not value[end:].lstrip().startswith("#"):
            raise AnalysisError("Unexpected text after a quoted YAML command")
        return command
    return value


def shell_dependencies(source):
    references = Counter()
    source = re.sub(r"\{#.*?#\}", "", source, flags=re.DOTALL)
    source = source.replace("\\\r\n", " ").replace("\\\n", " ")
    assignments = {}
    for match in re.finditer(
        r"""^\s*(?:export\s+)?(\w+)=(?:"([^"]*)"|'([^']*)'|([^\s;#]+))\s*(?:#.*)?$""",
        source, flags=re.MULTILINE,
    ):
        value = next(value for value in match.groups()[1:] if value is not None)
        if "$" not in value and "`" not in value:
            assignments.setdefault(match[1], []).append(value)

    def expand_literal(match):
        values = assignments.get(match[1] or match[2], [])
        return values[0] if len(values) == 1 else match[0]

    source = re.sub(r"\$\{(\w+)\}|\$(\w+)", expand_literal, source)
    lines = source.splitlines()
    index = 0
    while index < len(lines):
        line = lines[index]
        index += 1
        block = re.match(
            r"^(\s*)(?:-\s*)?(?:shell|command|run):\s*([>|])[-+]?\s*$",
            line,
        )
        if block:
            body = []
            while index < len(lines):
                following = lines[index]
                if following.strip() and (
                    len(following) - len(following.lstrip()) <= len(block[1])
                ):
                    break
                body.append(following)
                index += 1
            separator = " " if block[2] == ">" else "\n"
            references.update(shell_dependencies(separator.join(body)))
            continue
        if line.lstrip().startswith("#"):
            continue
        if not re.search(r"minigraph|sonic-cfggen", line):
            continue

        inline = re.match(
            r"^\s*(?:-\s*)?(?:[\w.]+\.)?(?:shell|command|run):\s*(.*)$",
            line,
        )
        if inline:
            references.update(shell_dependencies(yaml_command(inline[1])))
            continue

        # Double-quoted diagnostics can execute substitutions; literal text cannot.
        line, substitutions = shell_substitutions(line)
        for command in substitutions:
            references.update(shell_dependencies(command))
        try:
            segments = [
                shlex.split(segment) for segment in shell_segments(line)
            ]
        except ValueError as error:
            raise AnalysisError("Cannot tokenize a minigraph-related command: {}".format(error)) from error
        for words in segments:
            if not words or words[0] in {"echo", "printf", "logger"}:
                continue
            if words[0] == "-":
                words = words[1:]
            for word in words:
                for path in XML_PATH.findall(word):
                    references[("XML path", path)] += 1
            for position, word in enumerate(words):
                executable = word.rsplit("=", 1)[-1].rsplit("/", 1)[-1]
                if executable == "sonic-cfggen" and any(
                    flag == "--minigraph" or flag.startswith("--minigraph=")
                    or flag.startswith("-m")
                    for flag in words[position + 1:]
                ):
                    references[("command", "sonic-cfggen minigraph mode")] += 1
                if executable == "config" and words[position + 1:position + 2] == ["load_minigraph"]:
                    references[("command", "config load_minigraph")] += 1
                if executable in {"load_minigraph", "reload_minigraph"} and not re.match(
                    r"^\s*(?:function\s+)?{}\s*\(\s*\)".format(executable), line
                ):
                    references[("legacy helper", executable)] += 1
                if executable in {"bash", "sh", "dash"} and words[position + 1:position + 2] == ["-c"]:
                    if len(words) > position + 2:
                        references.update(
                            shell_dependencies(words[position + 2])
                        )
                    # The remaining words are positional arguments, not code.
                    break
    return references


def python_dependencies(source):
    try:
        tree = ast.parse(source)
    except SyntaxError as error:
        raise AnalysisError(
            "Cannot analyze Python syntax at line {}: {}".format(error.lineno, error.msg)
        ) from error
    references = Counter()
    aliases = {}
    imports = {}
    bindings = {}
    shadowed = set()
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            for name in node.names:
                root = name.name.split(".", 1)[0]
                imports[name.asname or root] = (
                    name.name if name.asname else root
                )
                if any(part in PARSER_MODULES for part in name.name.split(".")):
                    aliases[name.asname or name.name] = name.name
                    references[("parser import", name.name)] += 1
        elif isinstance(node, ast.ImportFrom):
            for name in node.names:
                target = (
                    "{}.{}".format(node.module, name.name)
                    if node.module else name.name
                )
                imports[name.asname or name.name] = "." * node.level + target
                if any(
                    part in PARSER_MODULES for part in target.split(".")
                ):
                    aliases[name.asname or name.name] = target
                    references[("parser import", target)] += 1
        elif isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name):
                    bindings.setdefault(target.id, []).append(node.value)
        elif isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name):
            bindings.setdefault(node.target.id, []).append(node.value)
        elif isinstance(node, ast.Name) and isinstance(node.ctx, ast.Store):
            shadowed.add(node.id)
        elif isinstance(node, ast.arg):
            shadowed.add(node.arg)
        elif isinstance(
            node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)
        ):
            shadowed.add(node.name)

    # Do not resolve API aliases whose names have other bindings in the file.
    for name in imports.keys() & shadowed:
        imports[name] = ""

    def literal_parser_import(node):
        if not isinstance(node, ast.Call) or (
            qualified_name(node.func, imports) != "importlib.import_module"
        ):
            return None
        arguments = list(node.args) + [
            keyword.value for keyword in node.keywords if keyword.arg == "name"
        ]
        module = static_text(arguments[0], bindings) if arguments else ""
        if any(part in PARSER_MODULES for part in module.split(".")):
            return module
        return None

    for name, values in bindings.items():
        if len(values) == 1:
            module = literal_parser_import(values[0])
            if module:
                aliases[name] = module

    for node in ast.walk(tree):
        if isinstance(node, (ast.Name, ast.Attribute)) and isinstance(node.ctx, ast.Load):
            name = qualified_name(node)
            for alias in sorted(aliases, key=len, reverse=True):
                if name == alias or name.startswith(alias + "."):
                    references[("parser reference", aliases[alias] + name[len(alias):])] += 1
                    break
        if not isinstance(node, ast.Call):
            continue
        module = literal_parser_import(node)
        if module:
            references[("parser import", module)] += 1
        call = qualified_name(node.func).rsplit(".", 1)[-1]
        owner, _, imported_call = qualified_name(
            node.func, imports
        ).rpartition(".")
        if owner in {"os", "subprocess"} and imported_call in COMMAND_CALLS:
            call = imported_call
        arguments = list(node.args) + [
            keyword.value for keyword in node.keywords if keyword.arg in {"args", "command", "cmd"}
        ]
        if call in COMMAND_CALLS and arguments:
            references.update(shell_dependencies(static_text(arguments[0], bindings)))
        if call in {"load_minigraph", "reload_minigraph"}:
            references[("legacy helper", call)] += 1
        if call in FILE_CALLS:
            arguments = list(node.args) + [keyword.value for keyword in node.keywords]
            if isinstance(node.func, ast.Attribute):
                arguments.append(node.func.value)
            for argument in arguments:
                for path in XML_PATH.findall(static_text(argument, bindings)):
                    references[("XML path", path)] += 1
    return references


def dependencies(path, content, mode):
    name = PurePosixPath(path)
    references = Counter()
    if name.name == "minigraph.xml" and "tests" not in name.parts:
        references[("shipped XML artifact", path)] += 1
    if mode == "120000":
        target = content.decode("utf-8")
        if PurePosixPath(target).name in {module + ".py" for module in PARSER_MODULES}:
            references[("parser symlink", target)] += 1
        return references
    if name.suffix in {".md", ".rst", ".xml"}:
        return references
    if b"\0" in content:
        if name.suffix == ".py":
            raise AnalysisError("Python source contains a NUL byte")
        return Counter()
    source = content.decode("utf-8")
    if name.suffix == ".py" or re.match(r"^#!.*python", source):
        return python_dependencies(source)
    return shell_dependencies(source)


def read_blob(repo, tree, path):
    entry = git(repo, "ls-tree", "-z", tree, "--", ":(literal)" + path)
    if not entry:
        return None
    metadata = entry.split(b"\t", 1)[0].decode("ascii").split()
    mode, kind, object_id = metadata
    if kind != "blob":
        return None
    return git(repo, "cat-file", "blob", object_id), mode


def check_changes(repo, base, head):
    base = git(repo, "rev-parse", "--verify", "--end-of-options", base + "^{tree}").decode().strip()
    head = git(repo, "rev-parse", "--verify", "--end-of-options", head + "^{tree}").decode().strip()
    paths = git(repo, "diff", "--no-renames", "--name-only", "-z", base, head, "--")
    violations = []
    for raw_path in paths.split(b"\0"):
        if not raw_path:
            continue
        path = raw_path.decode("utf-8")
        if path in FROZEN_FILES:
            violations.append((path, "The legacy parser or its extension is frozen."))
            continue
        current = read_blob(repo, head, path)
        if current is None:
            continue
        previous = read_blob(repo, base, path)
        try:
            new = dependencies(path, *current)
            old = dependencies(path, *previous) if previous else Counter()
        except (AnalysisError, UnicodeError) as error:
            raise AnalysisError("{}: {}".format(path, error)) from error
        for (kind, target), count in sorted((new - old).items()):
            violations.append((
                path,
                "Added {} static {} reference(s): {}.".format(count, kind, target),
            ))
    return violations


def annotation(message, path=None):
    def escape(text):
        return text.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")

    location = ""
    if path:
        location = " file=" + escape(path).replace(":", "%3A").replace(",", "%2C")
    print("::error{}::{}".format(location, escape(message)))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", required=True)
    parser.add_argument("--head", required=True)
    args = parser.parse_args(argv)
    guide_url = os.environ.get("MINIGRAPH_GUIDE_URL", GUIDE_URL)
    try:
        violations = check_changes(Path.cwd(), args.base, args.head)
    except (AnalysisError, OSError) as error:
        annotation("Minigraph comparison could not be completed: {}".format(error))
        print("This is an analysis failure, not a successful freeze check.")
        print("Guide: " + guide_url)
        return 2
    if not violations:
        print(
            "No frozen parser edits or new recognized minigraph references "
            "detected."
        )
        return 0
    for path, reason in violations:
        annotation(reason, path)
    print(
        "Minigraph is being retired. New dependencies on its legacy configuration "
        "path prolong retirement and create competing configuration sources."
    )
    print(
        "Use authoritative golden configuration for desired settings, and sonic-mgmt "
        "for lab-specific generation. Do not replace one minigraph dependency with another."
    )
    print(
        "Remove the dependency or request a reviewed compatibility/retirement exception "
        "with qualification evidence. Do not weaken the checker to make the PR pass."
    )
    print("Guide: " + guide_url)
    return 1


if __name__ == "__main__":
    sys.exit(main())
