#!/usr/bin/env python3
"""Exercise gearbox producer guards and the unchanged legacy package contract."""

from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[3]

MAKE = """
PLATFORM_PATH = platform/vs
TARGET_PATH = target
DBG_IMAGE_MARK = dbg
DOCKER_CONFIG_ENGINE_TRIXIE = docker-config-engine-trixie.gz
SYNCD_VS = syncd-vs.deb
LIBNL3_DEV = libnl-3-dev.deb
LIBNL3 = libnl-3-200.deb
SYNCD_VS_DBG = syncd-vs-dbgsym.deb
LIBSWSSCOMMON_DBG = libswsscommon-dbgsym.deb
LIBSAIMETADATA_DBG = libsaimetadata-dbgsym.deb
LIBSAIREDIS_DBG = libsairedis-dbgsym.deb
LIBSAIVS_DBG = libsaivs-dbgsym.deb
include platform/vs/docker-gbsyncd-vs.mk
.PHONY: selected
selected:
\t@echo runtime=$(SONIC_BAZEL_DOCKER_IMAGES)
\t@echo debug=$(SONIC_BAZEL_DBG_DOCKER_IMAGES)
\t@echo target=$($(DOCKER_GBSYNCD_BASE)_BAZEL_TARGET)
\t@echo args=$($(DOCKER_GBSYNCD_BASE)_BAZEL_ARGS)
\t@echo depends=$($(DOCKER_GBSYNCD_BASE)_BAZEL_DEPENDS)
\t@echo debug_depends=$($(DOCKER_GBSYNCD_BASE_DBG)_BAZEL_DEPENDS)
\t@echo packages=$($(DOCKER_GBSYNCD_BASE)_DEPENDS)
\t@echo options=$($(DOCKER_GBSYNCD_BASE)_RUN_OPT)
\t@echo container=$($(DOCKER_GBSYNCD_BASE)_CONTAINER_NAME)
\t@echo install=$(SONIC_INSTALL_DOCKER_IMAGES)
\t@echo install_debug=$(SONIC_INSTALL_DOCKER_DBG_IMAGES)
"""


class GearboxContract(unittest.TestCase):
    def selected(self, **overrides):
        values = dict(BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", BLDENV="trixie", CONFIGURED_PLATFORM="vs",
                      CONFIGURED_ARCH="amd64", INCLUDE_FIPS="y", INCLUDE_VS_DASH_SAI="y", DBG_IMAGE_MARK="dbg")
        values.update(overrides)
        result = subprocess.run(["make", "--no-print-directory", "-s", "-f", "-", "selected",
                                 *[key + "=" + value for key, value in values.items()]],
                                input=MAKE, cwd=ROOT, text=True, capture_output=True, check=True)
        return dict(line.split("=", 1) for line in result.stdout.splitlines())

    def test_selected_archives_and_manifest_only_handoff(self):
        got = self.selected()
        self.assertEqual(got["runtime"], "docker-gbsyncd-vs.gz")
        self.assertEqual(got["debug"], "docker-gbsyncd-vs-dbg.gz")
        self.assertEqual(got["target"], "//platform/vs/docker-gbsyncd-vs:docker-gbsyncd-vs.gz")
        self.assertEqual(got["args"], "--config=syncd-dash")
        self.assertEqual(got["depends"].split(), [
            "target/bazel-manifests/docker-base-trixie/manifest.json",
            "target/bazel-manifests/docker-config-engine-trixie/manifest.json",
            "target/bazel-manifests/docker-gbsyncd-vs/manifest.json",
            "target/bazel-manifests/docker-gbsyncd-vs/image.env"])
        self.assertEqual(got["debug_depends"].split(), got["depends"].split() +
                         ["target/bazel-manifests/docker-gbsyncd-vs-dbg/manifest.json"])
        self.assertEqual(got["packages"].split(), ["syncd-vs.deb", "libnl-3-dev.deb", "libnl-3-200.deb"])
        self.assertEqual(got["options"], "--privileged -t -v /host/machine.conf:/etc/machine.conf -v /etc/sonic:/etc/sonic:ro")
        self.assertEqual(got["container"], "gbsyncd")
        self.assertEqual(got["install"], "docker-gbsyncd-vs.gz")
        self.assertEqual(got["install_debug"], "docker-gbsyncd-vs-dbg.gz")

    def test_other_profiles_preserve_make(self):
        for key, value in [("BUILD_WITH_BAZEL_WHEN_AVAILABLE", "n"), ("BLDENV", "bookworm"),
                           ("CONFIGURED_ARCH", "arm64"), ("CONFIGURED_PLATFORM", "broadcom"),
                           ("INCLUDE_FIPS", "n"), ("INCLUDE_VS_DASH_SAI", "n"), ("DBG_IMAGE_MARK", "debug"),
                           ("ENABLE_ASAN", "y"), ("ENABLE_SYNCD_RPC", "y"), ("CROSS_BUILD_ENVIRON", "y"),
                           ("MULTIARCH_QEMU_ENVIRON", "y"), ("SONIC_BUILD_TARGET", "docker-sonic-vs.gz")]:
            with self.subTest(key=key, value=value):
                got = self.selected(**{key: value})
                self.assertEqual(got["runtime"], "")
                self.assertEqual(got["debug"], "")
                self.assertEqual(got["packages"], "syncd-vs.deb libnl-3-dev.deb libnl-3-200.deb")

    def test_shared_dockerfile_package_contract_has_not_drifted(self):
        syncd = (ROOT / "platform/vs/docker-syncd-vs/Dockerfile.j2").read_text()
        gearbox = (ROOT / "platform/vs/docker-gbsyncd-vs/Dockerfile.j2").read_text()
        self.assertEqual(syncd.replace("docker_syncd_vs_debs", "docker_gbsyncd_vs_debs"), gearbox,
                         "Package contracts diverged: reassess sharing the Syncd lock and source package map")


if __name__ == "__main__":
    unittest.main()
