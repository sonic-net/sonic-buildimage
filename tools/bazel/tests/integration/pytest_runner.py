"""Run explicitly declared tests without source-tree conftest hooks or coverage."""

import sys

import pytest


if __name__ == "__main__":
    if len(sys.argv) == 1:
        sys.exit("Pass the declared test files; repository-wide discovery is disabled.")
    result = pytest.main([
        "--noconftest",
        "--import-mode=importlib",
        "-o", "addopts=",
        "-p", "no:cacheprovider",
    ] + sys.argv[1:])
    sys.exit(result)
