"""Generate ignored protobuf modules before importing package tests."""

import os
import platform
import stat
import sys

import pytest


target_architecture = os.environ.get("CONFIGURED_ARCH", platform.machine())
is_armhf = target_architecture == "armhf" or target_architecture.startswith(
    ("armv6", "armv7", "armv8l")
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


# These unit fixtures exercise real modes and symlinks without requiring chown.
# The explicit root acceptance files use actual ownership and UID transitions.
@pytest.fixture
def root_owned_auth_files(monkeypatch):
    from sonic_py_common import redis_auth

    original_os = redis_auth.os
    owners = []

    class FixtureOS:
        def __getattr__(self, name):
            return getattr(original_os, name)

        def fstat(self, fd):
            info = original_os.fstat(fd)
            path = original_os.readlink('/proc/self/fd/' + str(fd))
            for directory, owner in reversed(owners):
                if path == directory or path.startswith(directory + '/'):
                    values = list(info)
                    values[stat.ST_UID] = owner
                    return original_os.stat_result(values)
            return info

    def register(directory, owner=0):
        if original_os.geteuid() == 0 and owner == 0:
            return
        owners.append((original_os.path.realpath(directory), owner))
        monkeypatch.setattr(redis_auth, 'os', FixtureOS())

    return register
