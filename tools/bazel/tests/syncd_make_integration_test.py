#!/usr/bin/env python3
"""Check syncd-vs selection and package prerequisites without running Bazel."""

import json
from pathlib import Path
import re
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[3]

REAL_PACKAGE_RULES = """
DBG_IMAGE_MARK = dbg
PLATFORM_PATH = platform/vs
TARGET_PATH = target
DEBS_PATH = target/debs/trixie
include rules/functions
include rules/libnl3.mk
include rules/libyang3.mk
include rules/swss-common.mk
include rules/p4lang.mk
include rules/dash-sai.mk
include rules/sairedis.mk
include rules/eventd.mk
include rules/sonic-fips.mk
include rules/docker-base-trixie.mk
include rules/docker-config-engine-trixie.mk
include platform/vs/syncd-vs.mk
include platform/vs/docker-syncd-vs.mk
"""


class MakeIntegrationTest(unittest.TestCase):
    def makefile(self):
        expansion = next(line for line in (ROOT / "rules/functions").read_text().splitlines()
                         if line.startswith("expand ="))
        return expansion + "\n" + """
DBG_IMAGE_MARK = dbg
PLATFORM_PATH = platform/vs
TARGET_PATH = target
DEBS_PATH = target/debs/trixie
DOCKER_CONFIG_ENGINE_TRIXIE = docker-config-engine-trixie.gz
docker-config-engine-trixie.gz_DBG_DEPENDS = base-dbgsym.deb $(FIPS_OPENSSH_CLIENT)
docker-config-engine-trixie.gz_DBG_IMAGE_PACKAGES = gdb gdbserver vim sshpass strace
P4LANG_PI = p4lang-pi.deb
P4LANG_BMV2 = p4lang-bmv2.deb
P4LANG_P4C = p4lang-p4c.deb
DASH_SAI = libsai.deb
SYNCD_VS = syncd-vs.deb
SYNCD_VS_DBG = syncd-vs-dbgsym.deb
LIBNL3_DEV = libnl-3-dev.deb
LIBNL_ROUTE3_DEV = libnl-route-3-dev.deb
LIBNL3 = libnl-3-200.deb
LIBSWSSCOMMON = libswsscommon.deb
LIBSAIMETADATA = libsaimetadata.deb
LIBSAIREDIS = libsairedis.deb
LIBSWSSCOMMON_DBG = libswsscommon-dbgsym.deb
LIBSAIMETADATA_DBG = libsaimetadata-dbgsym.deb
LIBSAIREDIS_DBG = libsairedis-dbgsym.deb
LIBSAIVS_DBG = libsaivs-dbgsym.deb
FIPS_OPENSSH_CLIENT = openssh-client_10.0p1-7+fips_amd64.deb
syncd-vs.deb_RDEPENDS = libsairedis.deb libsaimetadata.deb libsaivs.deb libsai.deb
libsairedis.deb_RDEPENDS = libswsscommon.deb
libswsscommon.deb_RDEPENDS = libyang3.deb
libswsscommon-dbgsym.deb_RDEPENDS = libswsscommon.deb
libsairedis-dbgsym.deb_RDEPENDS = libsairedis.deb
libsaimetadata-dbgsym.deb_RDEPENDS = libsaimetadata.deb
libsai.deb_RDEPENDS = p4lang-pi.deb p4lang-bmv2.deb p4lang-p4c.deb
syncd-vs-dbgsym.deb_RDEPENDS = syncd-vs.deb
SONIC_DOCKER_IMAGES = docker-other.gz
include platform/vs/docker-syncd-vs.mk
.PHONY: selected
selected:
\t@echo bazel=$(SONIC_BAZEL_DOCKER_IMAGES)
\t@echo bazel_debug=$(SONIC_BAZEL_DBG_DOCKER_IMAGES)
\t@echo legacy=$(filter-out $(SONIC_BAZEL_DOCKER_IMAGES),$(SONIC_DOCKER_IMAGES))
\t@echo debug=$(filter-out $(SONIC_BAZEL_DBG_DOCKER_IMAGES),$(SONIC_DOCKER_DBG_IMAGES))
\t@echo install=$(SONIC_INSTALL_DOCKER_IMAGES)
\t@echo install_debug=$(SONIC_INSTALL_DOCKER_DBG_IMAGES)
\t@echo switchable=$(SONIC_BAZEL_SWITCHABLE_IMAGES)
\t@echo oci_bases=$(SONIC_BAZEL_OCI_BASES)
\t@echo oci_archive=$(docker-config-engine-trixie.oci_OCI_ARCHIVE)
\t@echo oci_platform=$(docker-config-engine-trixie.oci_OCI_PLATFORM)
\t@echo manifests=$(SONIC_BAZEL_MANIFESTS)
\t@echo runtime_label=$($(DOCKER_SYNCD_BASE)_BAZEL_TARGET)
\t@echo debug_label=$($(DOCKER_SYNCD_BASE_DBG)_BAZEL_TARGET)
\t@echo runtime_inputs=$($(DOCKER_SYNCD_BASE)_BAZEL_DEPENDS)
\t@echo debug_inputs=$($(DOCKER_SYNCD_BASE_DBG)_BAZEL_DEPENDS)
\t@echo runtime_debs=$(SYNCD_VS_BAZEL_RUNTIME_DEBS)
\t@echo debug_debs=$(SYNCD_VS_BAZEL_DEBUG_DEBS)
\t@echo base_debug_debs=$(SYNCD_VS_BAZEL_BASE_DEBUG_DEBS)
\t@echo runtime_packages=$($(DOCKER_SYNCD_BASE)_DEPENDS)
\t@echo runtime_path=$($(DOCKER_SYNCD_BASE)_PATH)
\t@echo debug_path=$($(DOCKER_SYNCD_BASE_DBG)_PATH)
\t@echo run_opt=$($(DOCKER_SYNCD_BASE)_RUN_OPT)
.PHONY: debug_dependencies
debug_dependencies: $($(DOCKER_SYNCD_BASE_DBG)_BAZEL_DEPENDS)
\t@echo complete-debug-inputs
target/debs/trixie/%.deb:
\t@echo fixture-deb $@
target/docker-config-engine-trixie.oci:
\t@echo fixture-base $@
target/bazel-manifests/%/manifest.json:
\t@echo fixture-manifest $@
"""

    def run_make(self, target="selected", *, dry_run=False, make_source=None, directory=ROOT, **overrides):
        settings = {
            "BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n", "BLDENV": "trixie",
            "CONFIGURED_PLATFORM": "vs", "CONFIGURED_ARCH": "amd64", "DBG_IMAGE_MARK": "dbg",
            "INCLUDE_VS_DASH_SAI": "y", "INCLUDE_FIPS": "y", "ENABLE_ASAN": "n", "ENABLE_SYNCD_RPC": "n",
        }
        settings.update(overrides)
        arguments = ["make", "--no-print-directory", "-f", "-"]
        if dry_run:
            arguments.extend(["--dry-run", "--always-make"])
        return subprocess.run(arguments + [target] + [f"{key}={value}" for key, value in settings.items()],
                              input=make_source if make_source is not None else self.makefile(),
                              text=True, cwd=directory, capture_output=True, check=False)

    def values(self, result):
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return dict(line.split("=", 1) for line in result.stdout.splitlines())

    def test_opt_in_uses_oci_and_keeps_the_installer_and_runtime_metadata(self):
        """Selecting Bazel changes the image producer while retaining installer names and service settings."""
        values = self.values(self.run_make(BUILD_WITH_BAZEL_WHEN_AVAILABLE="y"))
        self.assertEqual(values["bazel"], "docker-syncd-vs.gz")
        self.assertEqual(values["bazel_debug"], "docker-syncd-vs-dbg.gz")
        self.assertEqual(values["runtime_label"], "//platform/vs/docker-syncd-vs:docker-syncd-vs.gz")
        self.assertEqual(values["debug_label"], "//platform/vs/docker-syncd-vs:docker-syncd-vs-dbg.gz")
        self.assertEqual(values["runtime_inputs"].split(), [
            *["target/debs/trixie/" + name for name in values["runtime_debs"].split()],
            "target/bazel-manifests/docker-base-trixie/manifest.json",
            "target/bazel-manifests/docker-config-engine-trixie/manifest.json",
            "target/bazel-manifests/docker-syncd-vs/manifest.json"])
        self.assertEqual(values["debug_inputs"].split(), values["runtime_inputs"].split() + [
            *["target/debs/trixie/" + name for name in values["debug_debs"].split()],
            "target/bazel-manifests/docker-syncd-vs-dbg/manifest.json"])
        self.assertEqual(values["manifests"].split(), ["docker-syncd-vs", "docker-syncd-vs-dbg"])
        self.assertEqual(values["oci_bases"], "")
        self.assertEqual(values["oci_archive"], "")
        self.assertEqual(values["oci_platform"], "")
        self.assertEqual(values["legacy"], "docker-other.gz")
        self.assertEqual(values["debug"], "")
        self.assertEqual(values["install"], "docker-syncd-vs.gz")
        self.assertEqual(values["install_debug"], "docker-syncd-vs-dbg.gz")
        self.assertEqual(values["runtime_packages"].split(), ["syncd-vs.deb", "libnl-3-dev.deb", "libnl-3-200.deb"])
        self.assertEqual(values["runtime_path"], "platform/vs/docker-syncd-vs")
        self.assertEqual(values["debug_path"], values["runtime_path"])
        self.assertIn("--privileged", values["run_opt"])
        self.assertIn("--cap-add=SYS_RAWIO", values["run_opt"])
        self.assertIn("-v /etc/sonic:/etc/sonic:ro", values["run_opt"])

    def test_platform_architecture_and_fips_follow_the_global_selector(self):
        """The same registered Syncd targets are selected across these Make settings."""
        for platform in ("vs", "mellanox"):
            for architecture in ("amd64", "arm64", "armhf"):
                for fips in ("y", "n"):
                    with self.subTest(platform=platform, architecture=architecture, fips=fips):
                        settings = {"CONFIGURED_PLATFORM": platform, "CONFIGURED_ARCH": architecture,
                                    "INCLUDE_FIPS": fips}
                        selected = self.values(self.run_make(BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", **settings))
                        self.assertEqual(selected["bazel"], "docker-syncd-vs.gz")
                        self.assertEqual(selected["bazel_debug"], "docker-syncd-vs-dbg.gz")
                        self.assertEqual(selected["runtime_label"], "//platform/vs/docker-syncd-vs:docker-syncd-vs.gz")
                        self.assertEqual(selected["debug_label"], "//platform/vs/docker-syncd-vs:docker-syncd-vs-dbg.gz")
                        legacy = self.values(self.run_make(BUILD_WITH_BAZEL_WHEN_AVAILABLE="n", **settings))
                        self.assertEqual(legacy["bazel"], "")
                        self.assertEqual(legacy["bazel_debug"], "")
                        self.assertIn("docker-syncd-vs.gz", legacy["legacy"].split())
                        self.assertIn("docker-syncd-vs-dbg.gz", legacy["debug"].split())

    def test_default_and_remaining_guarded_configurations_keep_make(self):
        """Disabling Bazel or selecting another image mode keeps the legacy producer."""
        default = self.values(self.run_make())
        self.assertEqual(default["bazel"], "")
        self.assertEqual(default["switchable"].split(), ["docker-syncd-vs.gz", "docker-syncd-vs-dbg.gz"])
        for setting in (
            {"BLDENV": "bookworm"}, {"DBG_IMAGE_MARK": "debug"}, {"INCLUDE_VS_DASH_SAI": "n"},
            {"ENABLE_ASAN": "y"}, {"ENABLE_SYNCD_RPC": "y"},
            {"CROSS_BUILD_ENVIRON": "y"}, {"MULTIARCH_QEMU_ENVIRON": "y"},
            {"SONIC_BUILD_TARGET": "target/docker-sonic-vs.gz"}, {"EXTRA_DOCKER_TARGETS": "docker-sonic-vs"},
        ):
            with self.subTest(setting=setting):
                selected = self.values(self.run_make(BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", **setting))
                legacy = self.values(self.run_make(**setting))
                self.assertEqual(selected, legacy)
                self.assertEqual(selected["bazel"], "")
                self.assertEqual(selected["bazel_debug"], "")
                self.assertEqual(selected["oci_bases"], "")

    def test_invalid_selector_is_rejected(self):
        """Reject misspelled selectors instead of silently choosing a different build producer."""
        for value in ("", "yes", "y n"):
            with self.subTest(value=value):
                result = self.run_make(BUILD_WITH_BAZEL_WHEN_AVAILABLE=value)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("must be y or n", result.stderr)

    def test_source_images_have_no_make_deb_prerequisites(self):
        """Runtime and symbols come from source TARs, with no P4 DEB build prerequisites."""
        values = self.values(self.run_make(BUILD_WITH_BAZEL_WHEN_AVAILABLE="y"))
        self.assertEqual(values["runtime_debs"], "")
        self.assertEqual(values["debug_debs"], "")
        self.assertEqual(values["base_debug_debs"], "")
        for variant in ("runtime", "debug"):
            inputs = values[variant + "_inputs"].split()
            self.assertFalse(any(path.endswith((".deb", ".oci")) for path in inputs))
        result = self.run_make("debug_dependencies", dry_run=True, BUILD_WITH_BAZEL_WHEN_AVAILABLE="y")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("fixture-deb", result.stdout)
        self.assertNotIn("fixture-base", result.stdout)
        self.assertIn("fixture-manifest target/bazel-manifests/docker-base-trixie/manifest.json", result.stdout)
        self.assertIn("fixture-manifest target/bazel-manifests/docker-config-engine-trixie/manifest.json", result.stdout)

    def test_custom_make_package_directories_do_not_change_source_inputs(self):
        """Source OCI builds are independent of where the legacy producer writes DEBs."""
        expected = self.values(self.run_make(BUILD_WITH_BAZEL_WHEN_AVAILABLE="y"))
        for setting in ({"docker-syncd-vs.gz_DEBS_PATH": "custom-debs/trixie"},
                        {"DEBS_PATH": "custom-debs/trixie"}):
            with self.subTest(setting=setting):
                selected = self.values(self.run_make(BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", **setting))
                self.assertEqual(selected, expected)
                self.assertEqual(self.values(self.run_make(**setting))["bazel"], "")

    def test_legacy_p4_producers_remain_declared_after_source_oci_selection(self):
        """The opt-in image changes do not intercept or remove legacy P4 package targets."""
        source = REAL_PACKAGE_RULES + """
P4_CANDIDATES = $(P4LANG_PI) $(P4LANG_BMV2) $(P4LANG_P4C) $(DASH_SAI) $(DASH_SAI_DEV)
.PHONY: selected
selected:
\t@echo candidates=$(P4_CANDIDATES)
\t@echo native=$(filter $(P4_CANDIDATES),$(SONIC_MAKE_DEBS))
\t@echo derived=$(filter $(P4_CANDIDATES),$(SONIC_DERIVED_DEBS))
\t@echo development=$(DASH_SAI_DEV)
\t@echo development_requires=$($(DASH_SAI_DEV)_DEPENDS)
"""
        expected = self.values(self.run_make(make_source=source))
        self.assertEqual(set(expected["native"].split() + expected["derived"].split()),
                         set(expected["candidates"].split()))
        self.assertEqual(expected["derived"], expected["development"])
        self.assertTrue(expected["development_requires"])
        for settings in ({}, {"SONIC_DEBUGGING_ON": "y"}, {"ENABLE_ASAN": "y"}):
            with self.subTest(settings=settings):
                selected = self.values(self.run_make(make_source=source,
                    BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", **settings))
                self.assertEqual(selected, expected)

    def test_dockerfile_direct_apt_packages_remain_declared(self):
        """The reviewed OCI APT declarations and lock must cover the legacy Dockerfile requests."""
        dockerfile = (ROOT / "platform/vs/docker-syncd-vs/Dockerfile.j2").read_text()
        packages = set()
        for line in dockerfile.splitlines():
            if line.startswith("RUN apt-get install -f -y "):
                packages.update(line.split()[5:])
        concrete = {"libgrpc++1.51": "libgrpc++1.51t64", "libgrpc29": "libgrpc29t64",
                    "libprotobuf32": "libprotobuf32t64", "libpcap0.8": "libpcap0.8t64"}
        packages = {concrete.get(name, name) for name in packages}
        lock = json.loads((ROOT / "platform/vs/docker-syncd-vs/bazel/apt.lock.json").read_text())
        declared = {item["name"] for item in lock["packages"].values()
                    if item["architecture"] in ("amd64", "all")}
        providers = {"libc-ares2": "libcares2", "pkg-config": "pkgconf"}
        packages = {providers.get(name, name) for name in packages}
        self.assertTrue(packages.issubset(declared), "Dockerfile APT packages missing from OCI inputs: " +
                        repr(sorted(packages - declared)))
        lock = json.loads((ROOT / "platform/vs/docker-syncd-vs/bazel/apt.lock.json").read_text())
        roots = lock["dependency_sets"]["syncd_vs_debian"]["sets"]["amd64"]
        locked = {key.rsplit("/", 1)[1].split(":", 1)[0] for key in roots}
        providers = {"libc-ares2": "libcares2", "pkg-config": "pkgconf"}
        concrete_packages = {providers.get(name, name) for name in packages}
        self.assertTrue(concrete_packages.issubset(locked), "Dockerfile APT packages missing from canonical lock: " +
                        repr(sorted(concrete_packages - locked)))


if __name__ == "__main__":
    unittest.main()
