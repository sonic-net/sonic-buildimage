#!/usr/bin/env python3

import argparse
import re
import sys
from pathlib import Path


PIPELINE_PATHS = (
    Path("azure-pipelines.yml"),
    Path(".azure-pipelines/azure-pipelines-build-vs-and-test.yml"),
    Path(".azure-pipelines/official-build-vs-with-test.yml"),
    Path(".azure-pipelines/baseline_test/baseline.test.buildimage.yml"),
)

REPOSITORIES = {
    "public": {
        "sonic-mgmt": "sonic-net/sonic-mgmt",
        "buildimage": "sonic-net/sonic-buildimage",
    },
    "internal": {
        "sonic-mgmt": "Azure/sonic-mgmt.msft",
        "buildimage": "Azure/sonic-buildimage-msft",
    },
}


class AlignmentError(RuntimeError):
    pass


def _line_ending(line):
    return "\r\n" if line.endswith("\r\n") else "\n"


def _replace_property(line, key, value):
    match = re.match(rf"^(?P<indent>\s*){re.escape(key)}\s*:", line)
    if not match:
        raise AlignmentError(f"Unable to replace {key!r} in {line!r}")
    return f"{match.group('indent')}{key}: {value}{_line_ending(line)}"


def _align_repository(text, alias, repository_name, branch, required):
    lines = text.splitlines(keepends=True)
    resource_pattern = re.compile(
        rf"^(?P<indent>\s*)-\s+repository:\s*{re.escape(alias)}\s*$"
    )
    start = next(
        (index for index, line in enumerate(lines) if resource_pattern.match(line.rstrip("\r\n"))),
        None,
    )
    if start is None:
        if required:
            raise AlignmentError(f"Missing repository resource {alias!r}")
        return text

    resource_indent = len(resource_pattern.match(lines[start].rstrip("\r\n")).group("indent"))
    end = len(lines)
    for index in range(start + 1, len(lines)):
        content = lines[index].rstrip("\r\n")
        stripped = content.lstrip()
        indent = len(content) - len(stripped)
        if re.match(r"^-\s+repository\s*:", stripped) and indent <= resource_indent:
            end = index
            break
        if stripped and not stripped.startswith("#") and indent < resource_indent:
            end = index
            break

    name_index = next(
        (index for index in range(start + 1, end) if re.match(r"^\s*name\s*:", lines[index])),
        None,
    )
    if name_index is None:
        raise AlignmentError(f"Repository resource {alias!r} has no name")
    lines[name_index] = _replace_property(lines[name_index], "name", repository_name)

    ref_index = next(
        (index for index in range(start + 1, end) if re.match(r"^\s*ref\s*:", lines[index])),
        None,
    )
    if ref_index is None:
        indent = re.match(r"^(?P<indent>\s*)name\s*:", lines[name_index]).group("indent")
        lines.insert(name_index + 1, f"{indent}ref: {branch}{_line_ending(lines[name_index])}")
    else:
        lines[ref_index] = _replace_property(lines[ref_index], "ref", branch)

    return "".join(lines)


def _align_mgmt_branch_default(text):
    lines = text.splitlines(keepends=True)
    parameter_pattern = re.compile(r"^(?P<indent>\s*)-\s+name:\s*MGMT_BRANCH\s*$")
    start = next(
        (index for index, line in enumerate(lines) if parameter_pattern.match(line.rstrip("\r\n"))),
        None,
    )
    if start is None:
        return text

    parameter_indent = len(parameter_pattern.match(lines[start].rstrip("\r\n")).group("indent"))
    for index in range(start + 1, len(lines)):
        content = lines[index].rstrip("\r\n")
        stripped = content.lstrip()
        indent = len(content) - len(stripped)
        if re.match(r"^-\s+name\s*:", stripped) and indent <= parameter_indent:
            break
        if re.match(r"^\s*default\s*:", lines[index]):
            lines[index] = _replace_property(lines[index], "default", "'$(BUILD_BRANCH)'")
            return "".join(lines)

    raise AlignmentError("MGMT_BRANCH parameter has no default")


def align_file(path, branch, scope):
    text = path.read_text(encoding="utf-8")
    repositories = REPOSITORIES[scope]
    updated = _align_repository(
        text,
        "sonic-mgmt",
        repositories["sonic-mgmt"],
        branch,
        required=True,
    )
    updated = _align_repository(
        updated,
        "buildimage",
        repositories["buildimage"],
        branch,
        required=False,
    )
    if path.name == "azure-pipelines-build-vs-and-test.yml":
        updated = _align_mgmt_branch_default(updated)
    return text, updated


def parse_args():
    parser = argparse.ArgumentParser(
        description="Align PR and baseline pipeline resources with a release branch."
    )
    parser.add_argument("--branch", required=True)
    parser.add_argument("--scope", choices=sorted(REPOSITORIES), required=True)
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument(
        "--check",
        action="store_true",
        help="Report mismatches without modifying files.",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    branch = args.branch.removeprefix("refs/heads/")
    if not re.fullmatch(r"20\d{4}", branch):
        raise AlignmentError(f"Unsupported release branch {args.branch!r}")

    root = args.root.resolve()
    if not (root / PIPELINE_PATHS[0]).is_file():
        raise AlignmentError(f"Missing required pipeline file {PIPELINE_PATHS[0]}")

    mismatches = []
    for relative_path in PIPELINE_PATHS:
        path = root / relative_path
        if not path.is_file():
            continue
        original, updated = align_file(path, branch, args.scope)
        if original == updated:
            continue
        mismatches.append(str(relative_path))
        if not args.check:
            path.write_text(updated, encoding="utf-8")

    if args.check and mismatches:
        print("Release pipeline references are not branch-aligned:", file=sys.stderr)
        for path in mismatches:
            print(f"  {path}", file=sys.stderr)
        return 1

    action = "Checked" if args.check else "Aligned"
    print(f"{action} release pipeline references for {branch} ({args.scope})")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except AlignmentError as error:
        print(f"error: {error}", file=sys.stderr)
        sys.exit(2)
