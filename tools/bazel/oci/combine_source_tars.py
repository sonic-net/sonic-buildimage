"""Assemble self-contained public runtime payloads from source module owners."""

import argparse
from pathlib import Path

from tools.bazel.oci.package_contract import bindings, combine_source_payloads


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--architecture", required=True, choices=("amd64", "arm64"))
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("packages", nargs="+", metavar="OWNER=TAR")
    args = parser.parse_args()
    combine_source_payloads(bindings(args.packages), args.output, args.architecture, base_files={}, required_paths={})


if __name__ == "__main__":
    main()
