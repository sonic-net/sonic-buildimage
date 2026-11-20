"""Check the runtime, debug, and disabled NAT selections in the real Make rule."""

import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
VARIABLES = ("SONIC_BAZEL_DOCKER_IMAGES", "SONIC_BAZEL_DBG_DOCKER_IMAGES",
             "SONIC_INSTALL_DOCKER_IMAGES", "SONIC_INSTALL_DOCKER_DBG_IMAGES",
             "docker-nat.gz_BASE_IMAGE_FILES", "docker-nat.gz_BAZEL_TARGET",
             "docker-nat-dbg.gz_BAZEL_TARGET")


def select(**overrides):
    options = dict(BLDENV="trixie", CONFIGURED_PLATFORM="vs", CONFIGURED_ARCH="amd64",
                   INCLUDE_FIPS="y", DBG_IMAGE_MARK="dbg", BUILD_WITH_BAZEL_WHEN_AVAILABLE="y",
                   INCLUDE_NAT="y")
    options.update(overrides)
    recipe = "include rules/docker-nat.mk\nshow:\n\t@printf '%s\\n' "
    recipe += " ".join("'$(" + name + ")'" for name in VARIABLES) + "\n"
    process = subprocess.run(["make", "-sf", "-", "show", *[f"{k}={v}" for k, v in options.items()]],
                             input=recipe, text=True, cwd=ROOT, capture_output=True, check=True)
    return process.stdout.splitlines()


class NatMakeTest(unittest.TestCase):
    def test_native_installer_and_archive_targets(self):
        runtime, debug = "docker-nat.gz", "docker-nat-dbg.gz"
        self.assertEqual(select(), [runtime, debug, runtime, debug, "natctl:/usr/bin/natctl",
                                    "//dockers/docker-nat:" + runtime, "//dockers/docker-nat:" + debug])

    def test_disabled_feature(self):
        self.assertEqual(select(INCLUDE_NAT="n")[:4], [""] * 4)

    def test_legacy_profiles(self):
        for key, value in (("BUILD_WITH_BAZEL_WHEN_AVAILABLE", "n"), ("ENABLE_ASAN", "y"),
                           ("CONFIGURED_ARCH", "arm64"), ("CONFIGURED_PLATFORM", "broadcom"),
                           ("BLDENV", "bookworm"), ("INCLUDE_FIPS", "n"), ("CROSS_BUILD_ENVIRON", "y"),
                           ("MULTIARCH_QEMU_ENVIRON", "y")):
            with self.subTest(key=key):
                self.assertEqual(select(**{key: value})[:4], ["", "", "docker-nat.gz", "docker-nat-dbg.gz"])
