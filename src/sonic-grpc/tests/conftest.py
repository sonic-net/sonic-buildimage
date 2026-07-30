"""Generate ignored protobuf modules before importing package tests."""

import sys

if sys.version_info[0] < 3:
    collect_ignore_glob = ["test_*.py"]
else:
    from generate_protos import generate

    generate()
