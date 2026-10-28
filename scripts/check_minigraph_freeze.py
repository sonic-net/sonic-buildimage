#!/usr/bin/env python3
"""Compare Git snapshots for frozen parser edits and new static dependencies."""

import argparse
import ast
from collections import Counter
import os
from pathlib import Path, PurePosixPath
import re
import shlex
import subprocess
import sys

import tree_sitter_bash
from tree_sitter import Language, Parser
import yaml


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
PROCESS_CALLS = {"Popen", "call", "check_call", "check_output", "run"}
SHELL_CALLS = {"check_output_pipe", "exec_command", "shell"}
ARGV_CALLS = {"command", "getstatusoutput_noshell", "run_command"}
SHELL_PARSER = Parser(Language(tree_sitter_bash.language()))
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
    if isinstance(node, ast.Constant) and isinstance(node.value, (str, bool)):
        return node.value
    if isinstance(node, ast.Name) and node.id not in seen:
        values = bindings.get(node.id, [])
        if len(values) == 1:
            return static_value(values[0], bindings, (*seen, node.id))
    if isinstance(node, (ast.List, ast.Tuple)):
        values = [static_value(item, bindings, seen) for item in node.elts]
        return tuple(values) if isinstance(node, ast.Tuple) else values
    if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Add):
        left = static_value(node.left, bindings, seen)
        right = static_value(node.right, bindings, seen)
        if isinstance(left, (str, list, tuple)) and type(left) is type(right):
            return left + right
        return None
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
    return None


def static_text(node, bindings, seen=()):
    value = static_value(node, bindings, seen)
    return value if isinstance(value, str) else ""


def syntax_nodes(root):
    pending = [root]
    while pending:
        node = pending.pop()
        yield node
        pending.extend(reversed(node.named_children))


def shell_tree(source):
    data = source.encode()
    root = SHELL_PARSER.parse(data).root_node
    # The grammar separates tokens across continuations. Normalize only
    # executable/expandable text, retaining quoted and here-document data.
    protected = bytearray(len(data))
    for node in syntax_nodes(root):
        spans = []
        if node.type in {"raw_string", "comment", "heredoc_start", "heredoc_end",
                         "heredoc_content"}:
            spans.append((node.start_byte, node.end_byte))
        elif node.type == "heredoc_body":
            start = node.start_byte
            for child in node.named_children:
                spans.append((start, child.start_byte))
                start = child.end_byte
            spans.append((start, node.end_byte))
        for start, end in spans:
            protected[start:end] = b"\1" * (end - start)
    normalized = bytearray()
    index = 0
    while index < len(data):
        if data[index] == 92 and not protected[index]:
            if data[index + 1:index + 3] == b"\r\n":
                index += 3
                continue
            if data[index + 1:index + 2] == b"\n":
                index += 2
                continue
            normalized.extend(data[index:index + 2])
            index += 2
        else:
            normalized.append(data[index])
            index += 1
    root = SHELL_PARSER.parse(bytes(normalized)).root_node
    if root.has_error:
        raise AnalysisError("Cannot parse shell syntax")
    return root


def shell_value(node, bindings, seen=()):
    if node is None:
        return None
    text = node.text.decode()
    if node.type == "command_name":
        return shell_value(node.named_children[0], bindings, seen)
    if node.type == "raw_string":
        return text[1:-1]
    if node.type in {"word", "number"}:
        return re.sub(r"\\(.)", r"\1", text, flags=re.DOTALL)
    if node.type == "string_content":
        return re.sub(r'\\([$`"\\])', r"\1", text)
    if node.type in {"string", "concatenation"}:
        values = [shell_value(child, bindings, seen)
                  for child in node.named_children]
        return "".join(value if value is not None else "${dynamic}"
                       for value in values)
    if node.type in {"simple_expansion", "expansion"}:
        match = re.fullmatch(r"\$(?:([A-Za-z_]\w*)|\{([A-Za-z_]\w*)\})", text)
        if match:
            name = match[1] or match[2]
            values = bindings.get(name, [])
            if name not in seen and len(values) == 1:
                value, end, scope = values[0]
                ancestor = node
                while ancestor is not None and ancestor.id != scope:
                    ancestor = ancestor.parent
                if ancestor is not None and end <= node.start_byte:
                    return shell_value(value, bindings, (*seen, name))
    return None


def shell_fields(node, bindings):
    def fragments(part):
        if part.type in {"command_name", "concatenation"}:
            for child in part.named_children:
                yield from fragments(child)
        else:
            yield shell_value(part, bindings), part.type in {
                "simple_expansion", "expansion", "command_substitution",
            }

    parts = list(fragments(node))
    if len(parts) == 1 and parts[0][0] is None:
        return [None]
    fields = [""]
    present = False
    for value, expand in parts:
        value = value if value is not None else "${dynamic}"
        if not expand:
            fields[-1] += value
            present = True
            continue
        # Only unquoted expansion characters undergo field splitting;
        # adjacent quoted text (including empty quotes) remains literal.
        for piece in re.split(r"([ \t\n]+)", value):
            if not piece:
                continue
            if piece[0] in " \t\n":
                if present:
                    fields.append("")
                    present = False
            else:
                fields[-1] += piece
                present = True
    return fields if present else fields[:-1]


def option_end(words, flags, values):
    """Find operands using known option arities, without scanning operands."""
    index = 1
    while index < len(words):
        word = words[index]
        if word is None:
            return None
        if word == "--":
            return index + 1
        if not word.startswith("-") or word == "-":
            break
        option, equals, _ = word.partition("=")
        if option in values:
            index += 1 if equals else 2
        elif word in flags:
            index += 1
        elif not word.startswith("--"):
            index += 1
            for offset, char in enumerate(word[1:], 2):
                if "-" + char in values:
                    if offset == len(word):
                        index += 1
                    break
                if "-" + char not in flags:
                    if not re.search(r"minigraph|sonic-cfggen", str(words)):
                        return None
                    raise AnalysisError("Unsupported wrapper option: " + word)
        else:
            if not re.search(r"minigraph|sonic-cfggen", str(words)):
                return None
            raise AnalysisError("Unsupported wrapper option: " + word)
    return index


def shell_payload(words):
    index = 1
    command = False
    execute = True
    while index < len(words):
        word = words[index]
        if word is None:
            return None, False
        if word == "--":
            index += 1
            break
        if word in {"--help", "--version"}:
            return None, False
        if word in {"--noprofile", "--norc", "--posix", "--restricted",
                    "--login", "--verbose"}:
            index += 1
            continue
        if word in {"--rcfile", "--init-file"}:
            index += 2
            continue
        if len(word) < 2 or word[0] not in "-+":
            break
        index += 1
        for offset, char in enumerate(word[1:], 2):
            if char in "oO":
                if offset == len(word):
                    index += 1
                break
            if char not in "abcefhiklmnprstuvxBCEHPT":
                if not re.search(r"minigraph|sonic-cfggen", str(words)):
                    return None, False
                raise AnalysisError("Unsupported shell option: " + word)
            if char == "c" and word[0] == "-":
                command = True
            if char == "n":
                execute = word[0] == "+"
    return (index, execute) if command and index < len(words) else (None, False)


def command_dependencies(words):
    references = Counter()
    words = [word if isinstance(word, str) else None for word in words]
    while words and isinstance(words[0], str):
        executable = words[0].rsplit("/", 1)[-1]
        index = None
        if executable in {"echo", "printf", "logger"}:
            return references
        if executable == "sudo":
            index = option_end(
                words, {"-" + flag for flag in "bEHiknPSs"},
                {"-u", "-g", "-h", "-p", "-C", "-T", "-D", "-R",
                 "--user", "--group", "--host", "--prompt", "--chdir",
                 "--chroot", "--close-from", "--command-timeout"},
            )
        elif executable == "env":
            index = option_end(
                words, {"-i", "-0", "--ignore-environment", "--null"},
                {"-u", "-C", "--unset", "--chdir"},
            )
            while index is not None and index < len(words) and isinstance(
                words[index], str
            ) and re.match(r"[A-Za-z_]\w*=", words[index]):
                index += 1
        elif executable == "command":
            if any(word in {"-v", "-V"} for word in words[1:2]):
                return references
            index = option_end(words, {"-p"}, set())
        elif executable == "exec":
            index = option_end(words, {"-c", "-l"}, {"-a"})
        elif executable == "timeout":
            index = option_end(
                words, {"--foreground", "--preserve-status", "--verbose"},
                {"-s", "-k", "--signal", "--kill-after"},
            )
            if index is not None:
                index += 1
        elif executable == "nice":
            index = option_end(words, set(), {"-n", "--adjustment"})
        elif executable == "nohup":
            index = option_end(words, set(), set())
        else:
            break
        if index is None:
            break
        words = words[index:]
    if not words:
        return references
    executable = words[0].rsplit("/", 1)[-1] if isinstance(words[0], str) else ""
    operands = words[1:]
    if executable in {"bash", "sh", "dash"}:
        payload, execute = shell_payload(words)
        if payload is not None:
            if execute and isinstance(words[payload], str):
                references.update(shell_dependencies(words[payload]))
            operands = words[payload + 1:]
    if executable == "sonic-cfggen" and any(
        isinstance(flag, str) and (
            flag == "--minigraph" or flag.startswith("--minigraph=")
            or flag.startswith("-m")
        ) for flag in operands
    ):
        references[("command", "sonic-cfggen minigraph mode")] += 1
    if executable == "config" and operands[:1] == ["load_minigraph"]:
        references[("command", "config load_minigraph")] += 1
    if executable in {"load_minigraph", "reload_minigraph"}:
        references[("legacy helper", executable)] += 1
    for word in operands:
        if isinstance(word, str):
            for path in XML_PATH.findall(word):
                references[("XML path", path)] += 1
    return references


def shell_dependencies(source):
    references = Counter()
    root = shell_tree(source)
    bindings = {}
    for node in syntax_nodes(root):
        if node.type == "variable_assignment":
            name = node.child_by_field_name("name").text.decode()
            parent = node.parent
            if parent.type == "declaration_command":
                parent = parent.parent
            value = node.child_by_field_name("value")
            direct = parent.type == "program" or (
                parent.type == "compound_statement"
                and parent.parent.type == "function_definition"
            )
            bindings.setdefault(name, []).append((
                value if direct else None, node.end_byte, parent.id
            ))
    for node in syntax_nodes(root):
        if node.type == "command":
            parts = [node.child_by_field_name("name")]
            parts.extend(node.children_by_field_name("argument"))
            words = [word for part in parts if part is not None
                     for word in shell_fields(part, bindings)]
            references.update(command_dependencies(words))
        elif node.type == "file_redirect":
            value = shell_value(node.child_by_field_name("destination"), bindings)
            for path in XML_PATH.findall(value or ""):
                references[("XML path", path)] += 1
        elif node.type in {"unary_expression", "binary_expression"}:
            operator = node.child_by_field_name("operator")
            operators = {"-ef", "-nt", "-ot"} if node.type == "binary_expression" else {
                "-a", "-b", "-c", "-d", "-e", "-f", "-g", "-h", "-k",
                "-L", "-N", "-O", "-G", "-p", "-r", "-S", "-s", "-u",
                "-w", "-x",
            }
            if operator is not None and operator.text.decode() in operators:
                for operand in node.named_children:
                    if operand != operator:
                        value = shell_value(operand, bindings)
                        for path in XML_PATH.findall(value or ""):
                            references[("XML path", path)] += 1
    return references


def argument_dependencies(value):
    if isinstance(value, str):
        try:
            value = shlex.split(value)
        except ValueError as error:
            raise AnalysisError("Cannot split command arguments: " + str(error)) from error
    return command_dependencies(value) if isinstance(value, (list, tuple)) else Counter()


def yaml_dependencies(source):
    references = Counter()
    visits = 0

    def command(value, shell):
        if isinstance(value, yaml.ScalarNode):
            references.update(
                shell_dependencies(value.value) if shell
                else argument_dependencies(value.value)
            )
        elif isinstance(value, yaml.MappingNode):
            for key, child in value.value:
                if key.value == "cmd":
                    if not isinstance(child, yaml.ScalarNode):
                        raise AnalysisError("YAML cmd arguments must be scalar strings")
                    command(child, shell)
                elif key.value == "argv" and not shell and isinstance(
                    child, yaml.SequenceNode
                ):
                    references.update(command_dependencies([
                        item.value if isinstance(item, yaml.ScalarNode) else None
                        for item in child.value
                    ]))

    def walk(node, ancestors=()):
        nonlocal visits
        if node is None:
            return
        visits += 1
        if visits > 10000:
            raise AnalysisError("YAML command structure exceeds analysis limit")
        if id(node) in ancestors:
            raise AnalysisError("Recursive YAML aliases in command structure")
        ancestors = (*ancestors, id(node))
        if isinstance(node, yaml.SequenceNode):
            for child in node.value:
                walk(child, ancestors)
        elif isinstance(node, yaml.MappingNode):
            fields = {key.value: value for key, value in node.value
                      if isinstance(key, yaml.ScalarNode)}
            for key, value in node.value:
                if not isinstance(key, yaml.ScalarNode):
                    continue
                name = key.value.rsplit(".", 1)[-1]
                if name in {"run", "shell", "command", "script", "bash"}:
                    if isinstance(value, yaml.ScalarNode) and not value.value:
                        value = fields.get("args", value)
                    command(value, name != "command")
                elif key.value == "inputs" and isinstance(
                    fields.get("task"), yaml.ScalarNode
                ) and re.fullmatch(r"(?:Bash|CmdLine)@\d+", fields["task"].value):
                    walk(value, ancestors)
                elif key.value in {"jobs", "stages"} and isinstance(
                    value, yaml.MappingNode
                ):
                    for _, child in value.value:
                        walk(child, ancestors)
                elif key.value in {
                    "jobs", "stages", "steps", "tasks", "handlers",
                    "pre_tasks", "post_tasks", "block", "rescue", "always",
                }:
                    walk(value, ancestors)

    try:
        for document in yaml.compose_all(source, Loader=yaml.BaseLoader):
            walk(document)
    except yaml.YAMLError as error:
        raise AnalysisError("Cannot parse YAML: " + str(error)) from error
    return references


def python_command(node, name, bindings):
    arguments = list(node.args) + [
        keyword.value for keyword in node.keywords
        if keyword.arg in {"args", "command", "cmd"}
    ]
    if not arguments:
        return Counter()
    value = static_value(arguments[0], bindings)
    owner, _, call = name.rpartition(".")
    if owner == "subprocess" and call in {"getoutput", "getstatusoutput"}:
        words = list(value) if isinstance(value, (list, tuple)) else [value]
        return command_dependencies(["/bin/sh", "-c", *words])
    if (owner == "os" and call in {"system", "popen"}) or call in SHELL_CALLS:
        return shell_dependencies(value) if isinstance(value, str) else Counter()
    if call in ARGV_CALLS:
        unsafe = next((keyword.value for keyword in node.keywords
                       if keyword.arg == "use_unsafe_shell"), None)
        if unsafe is not None:
            mode = static_value(unsafe, bindings)
            if not isinstance(mode, bool) and re.search(
                r"minigraph|sonic-cfggen", str(value)
            ):
                raise AnalysisError("Cannot determine command helper shell mode")
            if mode is True and isinstance(value, str):
                return shell_dependencies(value)
        return argument_dependencies(value)
    if owner != "subprocess" or call not in PROCESS_CALLS:
        return Counter()
    options = {keyword.arg: keyword.value for keyword in node.keywords}
    if None in options and re.search(r"minigraph|sonic-cfggen", str(value)):
        raise AnalysisError("Cannot determine expanded subprocess options")
    shell = static_value(options.get(
        "shell", node.args[8] if len(node.args) > 8 else ast.Constant(False)
    ), bindings)
    if not isinstance(shell, bool):
        if "minigraph" in str(value) or "sonic-cfggen" in str(value):
            raise AnalysisError("Cannot determine subprocess shell mode")
        return Counter()
    executable = options.get("executable", node.args[2] if len(node.args) > 2 else None)
    if isinstance(executable, ast.Constant) and executable.value is None:
        executable = None
    if executable and not isinstance(static_value(executable, bindings), str):
        if "minigraph" in str(value) or "sonic-cfggen" in str(value):
            raise AnalysisError("Cannot determine subprocess executable")
        return Counter()
    if shell:
        words = list(value) if isinstance(value, (list, tuple)) else [value]
        program = static_text(executable, bindings) if executable else "/bin/sh"
        return command_dependencies([program, "-c", *words])
    words = list(value) if isinstance(value, (list, tuple)) else [value]
    if executable and words:
        words[0] = static_value(executable, bindings)
    return command_dependencies(words)


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
        references.update(python_command(
            node, qualified_name(node.func, imports), bindings
        ))
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
    if name.suffix == ".j2":
        source = re.sub(r"\{[#%].*?[#%]\}", "", source, flags=re.DOTALL)
        source = re.sub(r"\{\{.*?\}\}", "${dynamic}", source, flags=re.DOTALL)
        name = name.with_suffix("")
    if name.suffix in {".yml", ".yaml"}:
        return yaml_dependencies(source)
    if name.suffix not in {".sh", ".bash"} and not re.search(
        r"minigraph|cfggen", source
    ):
        return references
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
        except (AnalysisError, UnicodeError, RecursionError) as error:
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
