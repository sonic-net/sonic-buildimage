"""Generate ignored protobuf modules before importing package tests."""

import os
import platform
import sys


target_architecture = os.environ.get("CONFIGURED_ARCH", platform.machine())
is_armhf = target_architecture == "armhf" or target_architecture.startswith(
    ("armv6", "armv7")
)

if sys.version_info < (3, 9) or is_armhf:
    collect_ignore = [
        "test_generate_protos.py",
        "test_generated_modules.py",
        "test_gnoi_client.py",
        "test_gnoi_testing.py",
    ]
else:
    from generate_protos import generate

    generate()
