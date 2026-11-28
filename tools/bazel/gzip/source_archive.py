#!/usr/bin/env python3
"""Carry the pinned GitLab source downloads from source CI to the VS job."""

import argparse
import base64
import hashlib
import json
from pathlib import Path
import tempfile


# BCR source.json integrity values for the GitLab-hosted modules selected by
# rules_gzip and its dependencies. Each archive is named src.tar.gz upstream.
# Metadata: https://bcr.bazel.build/modules/<module>/<version>/source.json
SOURCES = {
    "ape": ("1.0.1", "sha512-YHlfxYHiCxzrifn5ywADS59XhmTk5u0rW/1fPepfk1C6/bXn3uwSADixLkvee6rAP9u2sulmHX0igt5F5+eqTA=="),
    "download_utils": ("1.0.1", "sha512-nsUJrJJjUWKzTQA187uWHhnBSqPA7t8Q6Rqry83Sj63Kf09L6xEFCR7thlnRUtbRbzabIKGr7JreouFdjjdarw=="),
    "rules_coreutils": ("1.0.1", "sha512-ViXfJnpIYJ/rhVAcq/WIc/SwekMKubAFS/1cVrziq9NoHkLBtXbf977l/fEknU1ElXtteVDtxOfgi8vcM+34Qg=="),
    "rules_gzip": ("1.0.0", "sha512-KK1A/NSvOaixXftdIRSBHt+QSPEk1ZorO2HtKY6o5KRP/i8KQ1hsDQPVwbCCi0++6pE529idMytTL/3SmhNmCQ=="),
    "toolchain_utils": ("1.0.2", "sha512-AXqtA9CzczVUdS5dw0dX332eaXZQhFcEZmf1iKALwsoKoT9Mboah7/j1JN9hZpzq6B5X7gpe83TPpya6yJZ6Lw=="),
}


def check_versions(module_graph):
    """Check transitive selections as well as the root's direct dependencies."""
    selected = {}

    def visit(module):
        selected.setdefault(module["name"], set()).add(module["version"])
        for dependency in module.get("dependencies", []):
            visit(dependency)

    visit(json.loads(module_graph.read_text()))
    for name, (version, _) in SOURCES.items():
        if selected.get(name) != {version}:
            raise ValueError(f"{name} selection changed; refresh its source-download integrity")


def publish(data, destination):
    """Atomically install an already verified archive."""
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=destination.parent, delete=False) as output:
            temporary = Path(output.name)
            output.write(data)
        temporary.chmod(0o644)
        temporary.replace(destination)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--repository-cache", type=Path)
    source.add_argument("--archives-dir", type=Path)
    parser.add_argument("--module-graph", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if args.repository_cache is not None:
        if args.module_graph is None:
            parser.error("--repository-cache requires --module-graph")
        check_versions(args.module_graph)
    verified = []
    for name, (version, integrity) in SOURCES.items():
        digest = base64.b64decode(integrity.split("-", 1)[1]).hex()
        relative = Path(f"{name}-{version}") / "src.tar.gz"
        archive = (args.repository_cache / "content_addressable" / "sha512" / digest / "file"
                   if args.repository_cache is not None else args.archives_dir / relative)
        data = archive.read_bytes()
        if hashlib.sha512(data).hexdigest() != digest:
            raise ValueError(f"{name} source archive does not match its BCR SHA512")
        verified.append((data, args.output / relative, {
            "module": name, "version": version, "integrity": integrity,
            "bytes": len(data), "archive": str(args.output / relative),
        }))
    # Verify the entire set before replacing any existing archive.
    for data, destination, _ in verified:
        publish(data, destination)
    print(json.dumps([receipt for _, _, receipt in verified], indent=2))


if __name__ == "__main__":
    main()
