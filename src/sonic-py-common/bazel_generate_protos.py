"""Run the package's existing gNOI generator into Bazel's declared directory."""

import argparse
from pathlib import Path

from generate_protos import generate


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True)
    generate(parser.parse_args().output_dir)


if __name__ == "__main__":
    main()
