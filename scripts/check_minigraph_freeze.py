#!/usr/bin/env python3
"""Check Git snapshots for minigraph-retirement policy violations.

Reject changes to minigraph.py, minigraph_ext.py, and minigraph_custom.py
under src/sonic-config-engine. For other changed files, compare dependency
counts per file and reference: increases violate the policy; existing
recognized references and removals are not violations.

Recognize supported parser imports, legacy configuration commands,
minigraph XML access, and shipped minigraph.xml artifacts. Analyze supported
Python, shell, YAML, and Jinja forms without executing source commands or
rendering templates. This is bounded static analysis, not proof of
minigraph-free runtime behavior.

Run from the repository root:
    python3 scripts/check_minigraph_freeze.py --base BASE_REV --head HEAD_REV

The CI workflow runs this guard for PRs targeting master, supplying the
verified test merge as HEAD_REV and its first parent as BASE_REV.

Exit codes:
    0: No frozen-file changes or new recognized dependencies detected.
    1: Policy violations detected.
    2: Invalid arguments, analysis failure, or Git comparison failure.

See the "Minigraph retirement gate" section of .github/workflows/README.md
for detailed coverage, limitations, and contributor guidance.
"""

import argparse
import ast
from collections import Counter
import os
from pathlib import Path, PurePosixPath
import re
import shlex
import subprocess
import sys

from jinja2 import Environment, TemplateSyntaxError
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
SHELL_CALLS = {"exec_command", "shell"}
ARGV_CALLS = {"command", "run_command"}
PIPE_CALLS = {"check_output_pipe", "getstatusoutput_noshell_pipe"}
SHELLS = {"bash", "sh", "dash"}
SHELL_PARSER = Parser(Language(tree_sitter_bash.language()))
# Tokenization only; this checker never renders templates or HTML.
# nosemgrep: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
JINJA_ENVIRONMENT = Environment(keep_trailing_newline=True)
FILE_CALLS = {
    "Path", "PurePath", "copy", "copy2", "copyfile", "exists", "is_file",
    "isfile", "move", "open", "parse", "read_bytes", "read_text", "remove",
    "unlink", "write_bytes", "write_text",
}
BYTE_FILE_CALLS = {
    "copy", "copy2", "copyfile", "exists", "isfile", "move", "open", "parse",
    "remove", "unlink",
}


class AnalysisError(Exception):
    pass


class CfggenArgumentParser(argparse.ArgumentParser):
    def error(self, message):
        raise AnalysisError("Cannot analyze sonic-cfggen arguments: " + message)


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


def static_value(node, bindings, seen=(), imports=None):
    """Resolve literal fragments while preserving argument sequence types."""
    if isinstance(node, ast.Constant) and isinstance(node.value, (str, bytes, bool)):
        return node.value
    if isinstance(node, ast.Name) and node.id not in seen:
        values = bindings.get(node.id, [])
        if len(values) == 1:
            return static_value(values[0], bindings, (*seen, node.id), imports)
    if isinstance(node, (ast.List, ast.Tuple)):
        values = [static_value(item, bindings, seen, imports) for item in node.elts]
        return tuple(values) if isinstance(node, ast.Tuple) else values
    if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Add):
        left = static_value(node.left, bindings, seen, imports)
        right = static_value(node.right, bindings, seen, imports)
        if isinstance(left, (str, bytes, list, tuple)) and type(left) is type(right):
            return left + right
        return None
    if isinstance(node, ast.JoinedStr):
        return "".join(
            static_text(value, bindings, seen, imports)
            if isinstance(value, ast.Constant) else "${dynamic}"
            for value in node.values
        )
    if isinstance(node, ast.Call):
        if isinstance(node.func, ast.Attribute) and node.func.attr == "format":
            return static_text(node.func.value, bindings, seen, imports)
        if qualified_name(node.func, imports).rsplit(".", 1)[-1] in {"Path", "PurePath"}:
            parts = [static_value(arg, bindings, seen, imports) for arg in node.args]
            if not any(isinstance(part, bytes) for part in parts):
                return str(PurePosixPath(*(
                    part if isinstance(part, str) else "${dynamic}" for part in parts
                )))
    return None


def static_text(node, bindings, seen=(), imports=None):
    value = static_value(node, bindings, seen, imports)
    return value if isinstance(value, str) else ""


def decode_arguments(value):
    """Normalize bytes only at APIs accepting POSIX byte arguments."""
    if isinstance(value, bytes):
        try:
            return value.decode("utf-8")
        except UnicodeDecodeError as error:
            raise AnalysisError("Cannot decode literal byte argument as UTF-8") from error
    if isinstance(value, (list, tuple)):
        return type(value)(decode_arguments(item) for item in value)
    return value


def path_instance(node, bindings, imports=None, seen=()):
    if isinstance(node, ast.Name) and node.id not in seen:
        values = bindings.get(node.id, [])
        if len(values) == 1:
            return path_instance(values[0], bindings, imports, (*seen, node.id))
    return isinstance(node, ast.Call) and qualified_name(
        node.func, imports
    ).rsplit(".", 1)[-1] in {"Path", "PurePath"}


def file_arguments(node, name, bindings, imports):
    """Select path operands, not file contents, encodings or other options."""
    if name in {"ast.parse", "copy.copy"}:
        return []
    owner, _, call = name.rpartition(".")
    if call in {"Path", "PurePath"}:
        return [node]
    receiver = node.func.value if isinstance(node.func, ast.Attribute) else None
    path_class = owner.rsplit(".", 1)[-1] in {"Path", "PurePath"}
    if call in {"read_text", "read_bytes", "write_text", "write_bytes", "is_file"} or (
        call in {"open", "exists", "unlink"} and (
            path_class or path_instance(receiver, bindings, imports)
        )
    ):
        if path_class:
            return list(node.args[:1]) + [
                keyword.value for keyword in node.keywords if keyword.arg == "self"
            ]
        return [receiver] if path_instance(receiver, bindings, imports) else []
    count = 2 if call in {"copy", "copy2", "copyfile", "move"} else 1
    keywords = {"src", "dst", "dest"} if count == 2 else {
        "open": {"file", "path"}, "parse": {"source", "file", "filename"},
    }.get(call, {"path"})
    return list(node.args[:count]) + [
        keyword.value for keyword in node.keywords if keyword.arg in keywords
    ]


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
            if re.search(r"minigraph|sonic-cfggen", str(words)):
                raise AnalysisError("Cannot determine wrapper options")
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


def shell_payload(words, mode="command"):
    stdin = mode != "command"
    index = 1
    command = False
    execute = True
    read_stdin = False
    while index < len(words):
        word = words[index]
        if word is None:
            if stdin or re.search(r"minigraph|sonic-cfggen", str(words)):
                raise AnalysisError("Cannot determine shell options")
            return None, False
        if word == "--" or (stdin and word == "-"):
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
            if stdin and index > len(words):
                raise AnalysisError("Missing shell option argument")
            continue
        if len(word) < 2 or word[0] not in "-+":
            break
        index += 1
        for char in word[1:]:
            if char in "oO":
                option = words[index] if index < len(words) else None
                if option is None:
                    if stdin or re.search(r"minigraph|sonic-cfggen", str(words)):
                        raise AnalysisError("Cannot determine named shell option")
                    return None, False
                index += 1
                if char == "o" and option == "noexec":
                    execute = word[0] == "+"
                continue
            if char not in "abcefhiklmnprstuvxBCEHPT":
                if not stdin and not re.search(r"minigraph|sonic-cfggen", str(words)):
                    return None, False
                raise AnalysisError("Unsupported shell option: " + word)
            if char == "c":
                command = True
            if char == "n":
                execute = word[0] == "+"
            if char == "s":
                read_stdin = word[0] == "-"
            if stdin and char == "i":
                raise AnalysisError("Interactive shell stdin is not supported")
    if mode == "stdin":
        return (index, execute) if not command and (
            read_stdin or index == len(words)
        ) else (None, execute)
    if mode == "script":
        return (index, execute) if not command and not read_stdin and index < len(words) else (None, execute)
    return (index, execute) if command and index < len(words) else (None, False)


def cfggen_minigraph(words):
    # Match option spellings/arities in src/sonic-config-engine/sonic-cfggen.
    # argparse handles bundles and unique abbreviations without scanning values.
    parser = CfggenArgumentParser(add_help=False)
    for options in [
        ("-h", "--help"), ("-d", "--from-db"), ("-H", "--platform-info"),
        ("--print-data",), ("-w", "--write-to-db"),
    ]:
        parser.add_argument(*options, action="store_true")
    for options in [
        ("-m", "--minigraph"), ("-Y", "--yang"), ("-n", "--namespace"),
        ("-p", "--port-config"), ("-S", "--hwsku-config"),
    ]:
        parser.add_argument(*options, nargs="?", const=True)
    for options in [
        ("-M", "--device-description"), ("-k", "--hwsku"),
        ("-y", "--yaml"), ("-j", "--json"), ("-a", "--additional-data"),
        ("-s", "--redis-unix-sock-file"), ("-t", "--template"),
        ("-T", "--template_dir"), ("-v", "--var"), ("--var-json",),
        ("--preset",), ("-K", "--key"),
    ]:
        parser.add_argument(*options)
    args, unknown = parser.parse_known_args([
        word if word is not None else "${dynamic}" for word in words
    ])
    if args.help:
        return False
    if unknown:
        raise AnalysisError("Unsupported sonic-cfggen arguments: " + " ".join(unknown))
    return args.minigraph is not None


def unwrap_command(words, strict=False):
    words = [word if isinstance(word, str) else None for word in words]
    while words and isinstance(words[0], str):
        executable = words[0].rsplit("/", 1)[-1]
        index = None
        if executable in {"echo", "printf", "logger"}:
            return []
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
                return []
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
            if strict:
                raise AnalysisError("Cannot resolve shell stdin wrapper")
            break
        words = words[index:]
    return words


def command_dependencies(words):
    references = Counter()
    words = unwrap_command(words)
    if not words:
        return references
    executable = words[0].rsplit("/", 1)[-1] if isinstance(words[0], str) else ""
    operands = words[1:]
    if executable in SHELLS:
        payload, execute = shell_payload(words)
        if payload is not None:
            if execute and isinstance(words[payload], str):
                references.update(shell_dependencies(words[payload]))
            operands = words[payload + 1:]
    if executable == "sonic-cfggen" and cfggen_minigraph(operands):
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


def redirection_descriptor(node):
    descriptor = node.child_by_field_name("descriptor")
    # After here-strings, the grammar can expose descriptors as argv numbers.
    previous = node.prev_named_sibling
    if descriptor is None and node.type in {"herestring_redirect", "file_redirect"} and previous is not None:
        if previous.type == "command":
            previous = previous.named_children[-1]
        if previous.type == "number" and previous.end_byte == node.start_byte:
            descriptor = previous
    return descriptor


def shell_words(node, bindings):
    parts = [node.child_by_field_name("name")]
    parts.extend(node.children_by_field_name("argument"))
    redirects = list(node.children_by_field_name("redirect"))
    if node.parent.type == "redirected_statement" and node.parent.child_by_field_name("body") == node:
        redirects.extend(node.parent.children_by_field_name("redirect"))
    descriptors = {descriptor.id for redirect in redirects
                   if (descriptor := redirection_descriptor(redirect)) is not None}
    return [word for part in parts if part is not None and part.id not in descriptors
            for word in shell_fields(part, bindings)]


def contains_shell(node, bindings):
    for child in syntax_nodes(node):
        if child.type == "command":
            words = unwrap_command(shell_words(child, bindings), strict=True)
            if words and isinstance(words[0], str) and words[0].rsplit("/", 1)[-1] in SHELLS:
                return True
    return False


def stdin_dependencies(redirect, bindings):
    owner = redirect.parent
    if owner.type == "command" and owner.parent.type == "redirected_statement":
        owner = owner.parent
    body = owner.child_by_field_name("body") if owner.type == "redirected_statement" else owner
    if body is None:
        return Counter()
    words = unwrap_command(shell_words(body, bindings), strict=True) if body.type == "command" else []
    executable = words[0].rsplit("/", 1)[-1] if words and isinstance(words[0], str) else ""
    downstream = [child for child in redirect.named_children if child.type == "pipeline"]
    if owner.parent.type == "pipeline":
        downstream.append(owner.parent)
    indirect = (body.type != "command" and contains_shell(body, bindings)) or (
        executable not in SHELLS and any(contains_shell(node, bindings) for node in downstream)
    )
    if executable not in SHELLS and not indirect:
        return Counter()
    selected = None
    script_descriptor = False
    if executable in SHELLS:
        selected, execute = shell_payload(words, mode="stdin")
        if not execute:
            return Counter()
        if selected is None:
            script, _ = shell_payload(words, mode="script")
            if script is not None and (
                not isinstance(words[script], str) or "${dynamic}" in words[script]
            ):
                raise AnalysisError("Cannot resolve shell script input")
            script_descriptor = script is not None and re.fullmatch(
                r"/(?:dev/(?:stdin|fd/\d+)|proc/self/fd/\d+)", words[script]
            )
            payload, _ = shell_payload(words)
            nested = payload is not None and (
                not isinstance(words[payload], str)
                or "${dynamic}" in words[payload]
                or contains_shell(shell_tree(words[payload]), bindings)
            )
            if not nested and not script_descriptor:
                return Counter()

    # Follow direct descriptor assignments in source order. Nested commands
    # in heredoc expansions have their own descriptors and are not included.
    pending = list(owner.children_by_field_name("redirect"))
    if body != owner:
        pending.extend(body.children_by_field_name("redirect"))
    redirects = {}
    while pending:
        item = pending.pop()
        if item.id not in redirects:
            redirects[item.id] = item
            pending.extend(item.children_by_field_name("redirect"))
    descriptors = {}
    unknown = object()
    for item in sorted(redirects.values(), key=lambda node: node.start_byte):
        operator = next(child.type for child in item.children if not child.is_named)
        descriptor = redirection_descriptor(item)
        if descriptor is not None and not descriptor.text.isdigit():
            raise AnalysisError("Cannot resolve shell file descriptor")
        fd = int(descriptor.text) if descriptor is not None else (0 if operator.startswith("<") else 1)
        if item.type in {"heredoc_redirect", "herestring_redirect"}:
            descriptors[fd] = item.id
        elif operator in {"<&-", ">&-"}:
            descriptors[fd] = None
        else:
            destination = shell_value(item.child_by_field_name("destination"), bindings)
            if destination is None or "${dynamic}" in destination:
                descriptors[fd] = unknown
                continue
            alias = re.fullmatch(r"/(?:dev/fd|proc/self/fd)/(\d+)", destination)
            if operator in {"<&", ">&"}:
                duplicate = re.fullmatch(r"(\d+)(-?)", destination)
                if duplicate:
                    source_fd = int(duplicate[1])
                    descriptors[fd] = descriptors.get(source_fd)
                    if duplicate[2]:
                        descriptors[source_fd] = None
                else:
                    descriptors[fd] = unknown
            elif operator == "<" and (alias or destination == "/dev/stdin"):
                descriptors[fd] = descriptors.get(int(alias[1]) if alias else 0)
            elif operator == "<>" and (alias or destination == "/dev/stdin"):
                descriptors[fd] = unknown
            else:
                descriptors[fd] = None
    if descriptors.get(0) is unknown:
        raise AnalysisError("Cannot resolve redirected shell input")
    if descriptors.get(0) != redirect.id:
        if script_descriptor:
            raise AnalysisError("Cannot resolve shell script descriptor input")
        return Counter()
    if indirect:
        raise AnalysisError("Cannot resolve pipeline or compound shell stdin")

    if selected is None:
        raise AnalysisError("Cannot resolve nested shell or script descriptor input")

    if redirect.type == "herestring_redirect":
        value = next(child for child in redirect.named_children if child.type != "file_descriptor")
        source = shell_value(value, bindings)
        if source is None or "${dynamic}" in source:
            raise AnalysisError("Cannot resolve shell here-string")
        source += "\n"
    else:
        delimiter = next(child for child in redirect.named_children if child.type == "heredoc_start")
        content = next(child for child in redirect.named_children if child.type == "heredoc_body")
        source = content.text.decode()
        if any(child.type == "<<-" for child in redirect.children):
            source = re.sub(r"(?m)^\t+", "", source)
        if not any(char in delimiter.text.decode() for char in "'\"\\"):
            def expand(match):
                text = match[0]
                if not text.startswith("\\"):
                    raise AnalysisError("Cannot resolve expansions in executable heredoc")
                return "" if text.endswith("\n") else text[1:]
            source = re.sub(r"\\(?:\r?\n|[$`\\])|[$`]", expand, source)
    return shell_dependencies(source)


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
            references.update(command_dependencies(shell_words(node, bindings)))
        elif node.type in {"heredoc_redirect", "herestring_redirect"}:
            references.update(stdin_dependencies(node, bindings))
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

    def fields(node):
        if isinstance(node, yaml.MappingNode) and any(
            key.tag == "tag:yaml.org,2002:merge" for key, _ in node.value
        ):
            raise AnalysisError("YAML merge keys in command structures require manual analysis")
        return {
            key.value: value for key, value in node.value
            if isinstance(key, yaml.ScalarNode)
        } if isinstance(node, yaml.MappingNode) else {}

    def wrapper(key):
        return isinstance(key, yaml.ScalarNode) and re.fullmatch(
            r"\$\{\{\s*(?:(?:if|elseif|each)\s+.+?|else)\s*\}\}",
            key.value, flags=re.DOTALL,
        )

    def run(source, interpreter):
        selector = interpreter.value.strip() if isinstance(interpreter, yaml.ScalarNode) else None
        if interpreter is None or selector in {
            "bash", "sh", "bash {0}", "sh {0}", "dash {0}",
        }:
            return shell_dependencies(source)
        if selector in {"python", "python {0}", "python3 {0}"}:
            return python_dependencies(source)
        if re.search(r"minigraph|sonic-cfggen", source + "\n" + (selector or "")):
            raise AnalysisError("Unsupported or unresolved YAML run interpreter")
        return Counter()

    def command(value, shell, interpreter=None):
        if isinstance(value, yaml.ScalarNode):
            references.update(
                run(value.value, interpreter) if shell
                else argument_dependencies(value.value)
            )
        elif isinstance(value, yaml.MappingNode):
            fields(value)
            for key, child in value.value:
                if key.value == "cmd":
                    if not isinstance(child, yaml.ScalarNode):
                        raise AnalysisError("YAML cmd arguments must be scalar strings")
                    command(child, shell, interpreter)
                elif key.value == "argv" and not shell and isinstance(
                    child, yaml.SequenceNode
                ):
                    references.update(command_dependencies([
                        item.value if isinstance(item, yaml.ScalarNode) else None
                        for item in child.value
                    ]))

    def walk(node, ancestors=(), inherited_shell=None, named_jobs=False,
             inherited_task=None, input_mode=None):
        nonlocal visits
        if node is None:
            return
        visits += 1
        if visits > 10000:
            raise AnalysisError("YAML command structure exceeds analysis limit")
        if id(node) in ancestors:
            raise AnalysisError("Recursive YAML aliases in command structure")
        ancestors = (*ancestors, id(node))
        if input_mode is not None and not isinstance(node, yaml.MappingNode):
            raise AnalysisError("Azure task inputs must be a mapping")
        if isinstance(node, yaml.SequenceNode):
            for child in node.value:
                walk(child, ancestors, inherited_shell)
        elif isinstance(node, yaml.MappingNode):
            values = fields(node)
            if input_mode is not None:
                task_name = inherited_task.value
                if task_name not in {"Bash@3", "CmdLine@2"}:
                    raise AnalysisError("Unsupported Azure command task version: " + task_name)
                if task_name == "CmdLine@2":
                    mode = "inline"
                else:
                    if input_mode and "targetType" in values:
                        raise AnalysisError("Conditional Azure Bash targetType requires manual analysis")
                    selector = values.get("targetType")
                    mode = input_mode or (
                        selector.value.strip().lower() if isinstance(selector, yaml.ScalarNode)
                        else "filepath" if selector is None else ""
                    )
                if mode not in {"inline", "filepath"}:
                    raise AnalysisError("Unsupported or unresolved Azure Bash targetType")
                if mode == "inline":
                    script = values.get("script")
                    if script is not None and not isinstance(script, yaml.ScalarNode):
                        raise AnalysisError("Azure inline script must be a scalar string")
                    if script is not None:
                        references.update(shell_dependencies(script.value))
                elif any(key in values for key in ("filePath", "arguments")):
                    path = values.get("filePath")
                    arguments = values.get("arguments")
                    if any(value is not None and not isinstance(value, yaml.ScalarNode)
                           for value in (path, arguments)):
                        raise AnalysisError("Azure script path and arguments must be scalar strings")
                    references.update(shell_dependencies("exec bash {} {}".format(
                        shlex.quote(path.value if path is not None else "${dynamic}"),
                        arguments.value if arguments is not None else "",
                    )))
                for key, child in node.value:
                    if wrapper(key):
                        walk(child, ancestors, inherited_shell,
                             inherited_task=inherited_task, input_mode=mode)
                return
            if named_jobs:
                for key, child in node.value:
                    walk(child, ancestors, inherited_shell, named_jobs=bool(wrapper(key)))
                return
            task = values.get("task", inherited_task)
            default_shell = fields(fields(values.get("defaults")).get("run")).get(
                "shell", inherited_shell
            )
            for key, value in node.value:
                if not isinstance(key, yaml.ScalarNode):
                    continue
                if wrapper(key):
                    walk(value, ancestors, default_shell, inherited_task=task)
                    continue
                name = key.value.rsplit(".", 1)[-1]
                if key.value == "shell" and "run" in values:
                    continue
                if name in {"run", "shell", "command", "script", "bash"}:
                    if isinstance(value, yaml.ScalarNode) and not value.value:
                        value = values.get("args", value)
                    interpreter = values.get("shell", default_shell) if name == "run" else None
                    command(value, name != "command", interpreter)
                elif key.value == "inputs" and isinstance(
                    task, yaml.ScalarNode
                ) and re.fullmatch(r"(?:Bash|CmdLine)@\d+", task.value):
                    walk(value, ancestors, default_shell, inherited_task=task, input_mode="")
                elif key.value in {"jobs", "stages"} and isinstance(
                    value, yaml.MappingNode
                ):
                    walk(value, ancestors, default_shell, named_jobs=True)
                elif key.value in {
                    "jobs", "stages", "steps", "tasks", "handlers",
                    "pre_tasks", "post_tasks", "block", "rescue", "always",
                }:
                    walk(value, ancestors, default_shell)

    try:
        for document in yaml.compose_all(source, Loader=yaml.SafeLoader):
            walk(document)
    except yaml.YAMLError as error:
        raise AnalysisError("Cannot parse YAML: " + str(error)) from error
    return references


def python_command(node, name, bindings, imports=None):
    owner, _, call = name.rpartition(".")
    if call in PIPE_CALLS or call == "getstatusoutput_noshell":
        keyword_name = "cmd0" if call in PIPE_CALLS else "cmd"
        stages = list(node.args if call in PIPE_CALLS else node.args[:1])
        stages.extend(keyword.value for keyword in node.keywords
                      if keyword.arg == keyword_name)
        references = Counter()
        for stage in stages:
            value = decode_arguments(static_value(stage, bindings, imports=imports))
            words = value if isinstance(value, (list, tuple)) else [value]
            references.update(command_dependencies(words))
        return references
    arguments = list(node.args) + [
        keyword.value for keyword in node.keywords
        if keyword.arg in {"args", "command", "cmd"}
    ]
    if not arguments:
        return Counter()
    value = static_value(arguments[0], bindings, imports=imports)
    if (owner == "subprocess" and call in PROCESS_CALLS | {"getoutput", "getstatusoutput"}) or (
        owner == "os" and call == "system"
    ):
        value = decode_arguments(value)
    elif call in SHELL_CALLS | ARGV_CALLS:
        values = value if isinstance(value, (list, tuple)) else [value]
        if any(isinstance(item, bytes) for item in values):
            raise AnalysisError("Unsupported byte-valued command helper arguments")
    if owner == "subprocess" and call in {"getoutput", "getstatusoutput"}:
        words = list(value) if isinstance(value, (list, tuple)) else [value]
        return command_dependencies(["/bin/sh", "-c", *words])
    if (owner == "os" and call in {"system", "popen"}) or call in SHELL_CALLS:
        return shell_dependencies(value) if isinstance(value, str) else Counter()
    if call in ARGV_CALLS:
        unsafe = next((keyword.value for keyword in node.keywords
                       if keyword.arg == "use_unsafe_shell"), None)
        if unsafe is not None:
            mode = static_value(unsafe, bindings, imports=imports)
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
    executable = options.get("executable", node.args[2] if len(node.args) > 2 else None)
    if isinstance(executable, ast.Constant) and executable.value is None:
        executable = None
    program = decode_arguments(static_value(executable, bindings, imports=imports))
    relevant = re.search(r"minigraph|sonic-cfggen", str((value, program)))
    if None in options and relevant:
        raise AnalysisError("Cannot determine expanded subprocess options")
    shell = static_value(options.get(
        "shell", node.args[8] if len(node.args) > 8 else ast.Constant(False)
    ), bindings, imports=imports)
    if not isinstance(shell, bool):
        if relevant:
            raise AnalysisError("Cannot determine subprocess shell mode")
        return Counter()
    if executable and not isinstance(program, str):
        if relevant:
            raise AnalysisError("Cannot determine subprocess executable")
        return Counter()
    if shell:
        words = list(value) if isinstance(value, (list, tuple)) else [value]
        return command_dependencies([program if executable else "/bin/sh", "-c", *words])
    words = list(value) if isinstance(value, (list, tuple)) else [value]
    if executable and words:
        words[0] = program
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
        module = static_text(arguments[0], bindings, imports=imports) if arguments else ""
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
        name = qualified_name(node.func, imports)
        call = name.rsplit(".", 1)[-1]
        references.update(python_command(
            node, name, bindings, imports
        ))
        if call in {"load_minigraph", "reload_minigraph"}:
            references[("legacy helper", call)] += 1
        if call in FILE_CALLS:
            arguments = file_arguments(node, name, bindings, imports)
            values = [static_value(argument, bindings, imports=imports) for argument in arguments]
            for value in values:
                if call in BYTE_FILE_CALLS:
                    value = decode_arguments(value)
                for path in XML_PATH.findall(value if isinstance(value, str) else ""):
                    references[("XML path", path)] += 1
    return references


def template_sources(source, required=True):
    output = []
    current = output
    stack = []
    region = None
    tag = None
    try:
        for _, kind, value in JINJA_ENVIRONMENT.lex(source):
            if kind == "data":
                current.append(value)
            elif kind in {"block_begin", "comment_begin", "variable_begin"}:
                region = kind
                tag = None
                if kind == "variable_begin":
                    current.append("${dynamic}")
            elif kind == "name" and region == "block_begin" and tag is None:
                tag = value
            elif kind == "block_end":
                if tag in {"if", "for"}:
                    if len(stack) >= 64:
                        raise AnalysisError("Jinja nesting exceeds analysis limit")
                    branches = [[]]
                    current.append(branches)
                    stack.append({
                        "tag": tag, "parent": current, "branches": branches, "else": False,
                    })
                    current = branches[0]
                elif tag in {"elif", "else"}:
                    if not stack or stack[-1]["else"] or (
                        tag == "elif" and stack[-1]["tag"] != "if"
                    ):
                        raise AnalysisError("Unmatched Jinja branch: " + tag)
                    frame = stack[-1]
                    frame["else"] = tag == "else"
                    current = []
                    frame["branches"].append(current)
                elif tag in {"endif", "endfor"}:
                    if not stack or tag != "end" + stack[-1]["tag"]:
                        raise AnalysisError("Unmatched Jinja control ending: " + tag)
                    frame = stack.pop()
                    if not frame["else"]:
                        frame["branches"].append([])
                    current = frame["parent"]
                region = None
            elif kind in {"comment_end", "variable_end"}:
                region = None
        if region is not None:
            raise AnalysisError("Unterminated Jinja region: " + region)
        if stack:
            raise AnalysisError("Unterminated Jinja control block")
    except TemplateSyntaxError as error:
        raise AnalysisError("Cannot tokenize Jinja: " + str(error)) from error

    markers = ("minigraph", "cfggen")
    prefixes = {marker[:length] for marker in markers for length in range(len(marker))}

    def suffixes(parts, states):
        # Track only prefixes of relevant identifiers, including across branches.
        # This avoids expanding unrelated mixed-format templates exponentially.
        for part in parts:
            following = set()
            if isinstance(part, str):
                for state in states:
                    text = state + part
                    if any(marker in text for marker in markers):
                        return None
                    following.add(max(
                        (prefix for prefix in prefixes if text.endswith(prefix)), key=len
                    ))
            else:
                for branch in part:
                    result = suffixes(branch, states)
                    if result is None:
                        return None
                    following.update(result)
            states = following
        return states

    if not required and suffixes(output, {""}) is not None:
        return []

    def expand(parts):
        results = [""]
        for part in parts:
            choices = [part] if isinstance(part, str) else []
            if not isinstance(part, str):
                for branch in part:
                    choices.extend(expand(branch))
                    if len(choices) > 64:
                        raise AnalysisError("Jinja alternatives exceed analysis limit")
            if len(results) * len(choices) > 64:
                raise AnalysisError("Jinja alternatives exceed analysis limit")
            results = [prefix + suffix for prefix in results for suffix in choices]
        return results

    return expand(output)


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
    sources = [source]
    if name.suffix == ".j2":
        name = name.with_suffix("")
        sources = template_sources(
            source, required=name.suffix in {".sh", ".bash", ".yml", ".yaml"}
        )
    for source in sources:
        if name.suffix in {".yml", ".yaml"}:
            found = yaml_dependencies(source)
        elif name.suffix in {".sh", ".bash"} or re.search(r"minigraph|cfggen", source):
            found = shell_dependencies(source)
        else:
            continue
        if found:
            if references:
                raise AnalysisError("Jinja alternatives have ambiguous static reference counts")
            references.update(found)
    return references


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
    except (AnalysisError, OSError, UnicodeError) as error:
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
