#!/usr/bin/env python3
"""Compatibility entry for the shared Make-to-OCI image-version publisher."""

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.oci.prepare_image_version import main, publish


if __name__ == "__main__":
    main()
