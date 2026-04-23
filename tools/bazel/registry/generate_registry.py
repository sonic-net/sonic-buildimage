#!/usr/bin/env python3
"""Generate a local Bazel registry from sonic-buildimage submodules.

Scans src/ for MODULE.bazel files, extracts each module's name and version,
then creates the registry directory structure under tools/bazel/registry/modules/.

Usage:
    python3 tools/bazel/registry/generate_registry.py
"""

import json
import re
import shutil
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = (SCRIPT_DIR / "../../..").resolve()
MODULES_DIR = SCRIPT_DIR / "modules"


def parse_module_declaration(module_bazel: Path) -> tuple[str, str] | None:
    """Extract (name, version) from a MODULE.bazel's module() declaration.

    Returns None if the file has no module() call or no name field.
    """
    text = module_bazel.read_text()
    m = re.search(
        r'module\s*\('
        r'[^)]*?name\s*=\s*"([^"]+)"'
        r'(?:[^)]*?version\s*=\s*"([^"]+)")?',
        text,
        re.DOTALL,
    )
    if not m:
        return None
    name = m.group(1)
    version = m.group(2) or "0.0.0"
    return name, version


def discover_modules() -> list[tuple[str, str, str]]:
    """Find all MODULE.bazel files under src/ and return (name, version, path).

    The path is relative to the repo root (e.g. "src/sonic-swss-common").
    """
    modules = []
    src_dir = REPO_ROOT / "src"

    for module_bazel in sorted(src_dir.rglob("MODULE.bazel")):
        result = parse_module_declaration(module_bazel)
        if result is None:
            continue
        name, version = result
        src_path = str(module_bazel.parent.relative_to(REPO_ROOT))
        modules.append((name, version, src_path))

    return modules


def generate_module_entry(name: str, version: str, src_path: str) -> None:
    """Create registry files for one module."""
    version_dir = MODULES_DIR / name / version
    version_dir.mkdir(parents=True, exist_ok=True)

    # metadata.json
    metadata = MODULES_DIR / name / "metadata.json"
    metadata.write_text(json.dumps({"versions": [version]}, indent=2) + "\n")

    # source.json
    source = version_dir / "source.json"
    source.write_text(
        json.dumps({"type": "local_path", "path": src_path}, indent=2) + "\n"
    )

    # MODULE.bazel — symlink to source
    src_module = REPO_ROOT / src_path / "MODULE.bazel"
    dst_module = version_dir / "MODULE.bazel"
    dst_module.symlink_to(src_module.resolve())


def main() -> None:
    modules = discover_modules()

    if not modules:
        print("No modules found under src/", file=sys.stderr)
        sys.exit(1)

    # Clear and regenerate modules directory.
    if MODULES_DIR.exists():
        shutil.rmtree(MODULES_DIR)

    for name, version, path in modules:
        generate_module_entry(name, version, path)

    print(f"Generated registry for {len(modules)} modules in {MODULES_DIR}")
    for name, version, path in modules:
        print(f"  {name} {version} -> {path}")


if __name__ == "__main__":
    main()
