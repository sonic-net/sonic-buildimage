#!/usr/bin/env python3
"""Render Dockerfile.j2 templates for the static-analysis checkout artifact.

The artifact contains selected checkout/submodule trees (subject to the CI
stage exclusions). It does not fetch or patch external package sources. For
patched sources obtained by a build, use the existing ENABLE_SOURCE_ARCHIVE=y
build-time archive instead.
"""

import argparse
import json
from pathlib import Path

import jinja2


class _EmptyPackageVariables(jinja2.Undefined):
    """Treat unavailable per-docker build package lists as empty for analysis."""

    def __str__(self):
        return ""

    def __iter__(self):
        return iter(())

    def __len__(self):
        return 0

    def __bool__(self):
        return False

    def __call__(self, *args, **kwargs):
        return type(self)()

    def __getitem__(self, key):
        return type(self)()

    def __getattr__(self, name):
        if name.startswith("_"):
            raise AttributeError(name)
        return type(self)()


class _DockerfileLoader(jinja2.BaseLoader):
    """Resolve root, dockers/, and local includes, in build search order."""

    def __init__(self, root: Path, template_dir: Path):
        self.paths = (root, root / "dockers", template_dir)

    def get_source(self, environment, template):
        for directory in self.paths:
            candidate = directory / template
            if candidate.is_file():
                return candidate.read_text(encoding="utf-8"), str(candidate), lambda: False
        raise jinja2.TemplateNotFound(template)


def render_dockerfiles(root: Path) -> int:
    context_path = root / ".j2_context.json"
    context = json.loads(context_path.read_text(encoding="utf-8"))
    templates = sorted(
        path for path in root.rglob("Dockerfile.j2")
        if not any(part.startswith("fsroot") or part == ".git"
                   for part in path.relative_to(root).parts)
    )
    if not templates:
        raise RuntimeError("No Dockerfile.j2 templates found; check the checkout")

    errors = []
    for path in templates:
        relative = path.relative_to(root)
        env = jinja2.Environment(
            loader=_DockerfileLoader(root, path.parent),
            undefined=_EmptyPackageVariables,
        )  # nosemgrep: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
        try:
            rendered = env.get_template(str(relative)).render(**context)  # nosemgrep: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
            (path.parent / "Dockerfile").write_text(rendered, encoding="utf-8")
        except Exception as exc:
            errors.append(f"{relative}: {exc}")

    print(f"Dockerfile.j2: {len(templates) - len(errors)}/{len(templates)} rendered")
    if errors:
        raise RuntimeError("Rendering failed:\n" + "\n".join(errors))
    return len(templates)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path("."))
    args = parser.parse_args()
    render_dockerfiles(args.repo_root.resolve())


if __name__ == "__main__":
    main()
