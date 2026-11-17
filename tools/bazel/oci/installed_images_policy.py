"""Bind declared installed-image checks to the exact OCI and installer outputs."""

import argparse
import json
from pathlib import Path

from tools.bazel.oci.image_validation import installed_images_policy


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--policy", type=Path, action="append", required=True)
    parser.add_argument("--layout", nargs=2, action="append", required=True)
    parser.add_argument("--archive", nargs=2, action="append", required=True)
    parser.add_argument("--architecture", required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    policy = installed_images_policy([json.loads(path.read_bytes()) for path in args.policy],
                                     dict(args.layout), dict(args.archive), args.architecture)
    args.out.write_text(json.dumps(policy, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
