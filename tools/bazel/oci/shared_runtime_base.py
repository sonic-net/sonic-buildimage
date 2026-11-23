"""Identify base paths that could redirect or obstruct APT layer extraction."""

import argparse
import json
from pathlib import Path, PurePosixPath
import tarfile

from tools.bazel.oci.oci_layout import validate_layout


def non_directory_paths(base):
    # rules_oci supplies the content-addressed blobs. Only their TAR headers
    # are needed here; seek over regular file contents in uncompressed layers.
    layout = validate_layout(base, "linux/amd64", verify_layer_digest=False)
    non_directories = set()
    for layer in layout.layers:
        with tarfile.open(layer, "r:*") as archive:
            for member in archive:
                path = PurePosixPath(member.name)
                if path.is_absolute() or ".." in path.parts or path.name.startswith(".wh."):
                    return None
                # A write through a link (including a hardlinked symlink) can
                # land elsewhere. A lexical inventory cannot certify that base.
                if any(parent in non_directories for parent in path.parents):
                    return None
                if not member.isdir():
                    non_directories.add(path)
    return sorted(str(path) for path in non_directories)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    args.out.write_text(json.dumps(non_directory_paths(args.base), separators=(",", ":")) + "\n")


if __name__ == "__main__":
    main()
