"""Identify the Git revisions behind buildimage's local Bazel modules."""

import argparse
import ast
import hashlib
import json
from pathlib import Path, PurePosixPath
import subprocess


def git(root, *args):
    return subprocess.check_output(
        ["git", "-c", f"safe.directory={root}", "-C", str(root), *args],
        text=True,
    ).strip()


def overrides(root):
    paths = {}
    for item in ast.parse((root / "MODULE.bazel").read_text()).body:
        if not isinstance(item, ast.Expr) or not isinstance(item.value, ast.Call):
            continue
        call = item.value
        if isinstance(call.func, ast.Name) and call.func.id == "local_path_override":
            fields = {kw.arg: ast.literal_eval(kw.value) for kw in call.keywords}
            paths[fields["path"]] = fields["module_name"]
    return paths


def gitlinks(root):
    paths = overrides(root)
    # A local override can point to a Bazel module inside a submodule.
    declarations = git(root, "config", "--file", ".gitmodules", "--get-regexp", r"^submodule\..*\.path$")
    submodules = [line.split(maxsplit=1)[1] for line in declarations.splitlines()]
    owners = {name: (owner, module) for module, name in paths.items() for owner in submodules
              if PurePosixPath(module).is_relative_to(owner)}
    if not owners:
        return {}
    revisions = {}
    for record in git(root, "ls-tree", "-z", "HEAD", "--", *(owner for owner, _ in owners.values())).split("\0"):
        if record:
            fields, owner = record.split("\t", 1)
            mode, kind, oid = fields.split()
            if (mode, kind) == ("160000", "commit"):
                revisions[owner] = oid
    result = {}
    for name, (owner, module) in owners.items():
        if owner not in revisions:
            raise ValueError(f"{owner} is declared as a submodule but has no committed gitlink")
        result[name] = (owner, module, revisions[owner])
    return result


def verify(root, path, oid):
    child = root / path
    if not (child / ".git").exists() or git(child, "rev-parse", "HEAD") != oid:
        raise ValueError(f"{path} must be checked out at the buildimage gitlink {oid}. "
                         f"Run git submodule update --init -- {path}")
    return child


def ref_inputs(root):
    # HEAD can be detached, a loose branch, or a branch stored in packed-refs.
    # --git-path also handles submodules and linked Git worktrees.
    names = ["HEAD", "packed-refs"]
    head = git(root, "rev-parse", "--symbolic-full-name", "HEAD")
    if head.startswith("refs/"):
        names.append(head)
    paths = [git(root, "rev-parse", "--path-format=absolute", "--git-path", name) for name in names]
    if (root / ".git").is_file():
        paths.append(str(root / ".git"))
    return paths


def identity(root, name, selected):
    records = gitlinks(root)
    if name not in records:
        raise ValueError(f"No local override backed by a buildimage gitlink for {name}")
    path, module_path, oid = records[name]
    child = verify(root, path, oid)
    module = root / module_path / "MODULE.bazel"
    if not module.samefile(selected):
        raise ValueError(f"Bazel selected a different checkout for {name}")
    contents = module.read_bytes()
    tracked_path = str(module.relative_to(child))
    if git(child, "hash-object", tracked_path) != git(child, "rev-parse", f"HEAD:{tracked_path}"):
        raise ValueError(f"Modified producer module declaration in {path}")
    # Existing package contracts read name/version declarations. Give the
    # receipt an explicit gitlink identity and bind the producer's real MODULE
    # bytes as well. This file is never used to resolve Bazel dependencies.
    digest = hashlib.sha256(contents).hexdigest()
    manifest = (f"# Buildimage gitlink {path}; producer MODULE.bazel sha256={digest}\n"
                f'module(name = "{name}", version = "gitlink-{oid}")\n')
    return {"manifest": manifest, "watch": ref_inputs(root) + ref_inputs(child)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path.cwd())
    parser.add_argument("name")
    parser.add_argument("selected", type=Path)
    args = parser.parse_args()
    print(json.dumps(identity(args.root.resolve(), args.name, args.selected)))


if __name__ == "__main__":
    main()
