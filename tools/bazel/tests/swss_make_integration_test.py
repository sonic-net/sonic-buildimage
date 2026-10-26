#!/usr/bin/env python3
"""Check SWSS selection without starting a slave, Docker or Bazel."""

import os
import pathlib
import subprocess
import sys
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))

from tools.bazel.tests.docker_test import ArchiveRecipeFixture


class MakeIntegrationTest(unittest.TestCase):
    def select(self, **overrides):
        settings = {
            "BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n",
            "BLDENV": "trixie",
            "CONFIGURED_PLATFORM": "vs",
            "CONFIGURED_ARCH": "amd64",
            "ENABLE_ASAN": "n",
        }
        settings.update(overrides)
        makefile = """
DBG_IMAGE_MARK = dbg
SWSS = swss.deb
SWSS_DBG = swss-dbgsym.deb
DOCKER_SWSS_LAYER_TRIXIE = docker-swss-layer-trixie.gz
LIB_SONIC_DASH_API = dash.deb
SCAPY = scapy.whl
DOCKERS_PATH = dockers
TARGET_PATH = target
DOCKER_CONFIG_ENGINE_TRIXIE = docker-config-engine-trixie.gz
PYTHON_WHEELS_PATH = target/python-wheels/trixie
SONIC_DOCKER_IMAGES = docker-sysmgr.gz docker-swss-layer-trixie.gz
include rules/docker-orchagent.mk
.PHONY: selected
selected:
	@echo bazel=$(SONIC_BAZEL_DOCKER_IMAGES)
	@echo bazel_debug=$(SONIC_BAZEL_DBG_DOCKER_IMAGES)
	@echo legacy=$(filter-out $(SONIC_BAZEL_DOCKER_IMAGES),$(SONIC_DOCKER_IMAGES))
	@echo debug=$(filter-out $(SONIC_BAZEL_DBG_DOCKER_IMAGES),$(SONIC_DOCKER_DBG_IMAGES))
	@echo install=$(SONIC_INSTALL_DOCKER_IMAGES)
	@echo swss_packages=$($(DOCKER_ORCHAGENT)_DEPENDS)
	@echo swss_wheels=$($(DOCKER_ORCHAGENT)_PYTHON_WHEELS)
	@echo swss_base=$($(DOCKER_ORCHAGENT)_LOAD_DOCKERS)
	@echo manifests=$(SONIC_BAZEL_MANIFESTS)
	@echo runtime_label=$($(DOCKER_ORCHAGENT)_BAZEL_TARGET)
	@echo debug_label=$($(DOCKER_ORCHAGENT_DBG)_BAZEL_TARGET)
	@echo runtime_args=$($(DOCKER_ORCHAGENT)_BAZEL_ARGS)
	@echo debug_args=$($(DOCKER_ORCHAGENT_DBG)_BAZEL_ARGS)
	@echo runtime_inputs=$($(DOCKER_ORCHAGENT)_BAZEL_DEPENDS)
	@echo debug_inputs=$($(DOCKER_ORCHAGENT_DBG)_BAZEL_DEPENDS)
	@echo debug_path=$($(DOCKER_ORCHAGENT_DBG)_PATH)
	@echo switchable=$(SONIC_BAZEL_SWITCHABLE_IMAGES)
	@echo oci_bases=$(SONIC_BAZEL_OCI_BASES)
	@echo oci_archive=$(docker-config-engine-trixie.oci_OCI_ARCHIVE)
	@echo oci_platform=$(docker-config-engine-trixie.oci_OCI_PLATFORM)
"""
        return subprocess.run(
            ["make", "--no-print-directory", "-f", "-", "selected"]
            + [f"{key}={value}" for key, value in settings.items()],
            input=makefile,
            text=True,
            cwd=ROOT,
            capture_output=True,
            check=False,
        )

    def values(self, result):
        self.assertEqual(result.returncode, 0, result.stderr)
        return dict(line.split("=", 1) for line in result.stdout.splitlines())

    def test_default_preserves_make(self):
        """Keep the default SWSS builder on Make while registering both archives for later builder
        switches.
        """
        values = self.values(self.select())
        self.assertEqual(values["bazel"], "")
        self.assertEqual(values["oci_bases"], "")
        self.assertEqual(values["switchable"].split(), ["docker-orchagent.gz", "docker-orchagent-dbg.gz"])
        self.assertIn("docker-orchagent.gz", values["legacy"].split())
        self.assertEqual(values["debug"], "docker-orchagent-dbg.gz")

    def test_opt_in_selects_supported_swss_and_keeps_installer_contract(self):
        """Select the available SWSS Bazel targets while preserving legacy image metadata."""
        values = self.values(self.select(BUILD_WITH_BAZEL_WHEN_AVAILABLE="y"))
        self.assertEqual(values["bazel"], "docker-orchagent.gz")
        self.assertEqual(values["bazel_debug"], "docker-orchagent-dbg.gz")
        self.assertEqual(values["switchable"].split(), ["docker-orchagent.gz", "docker-orchagent-dbg.gz"])
        self.assertEqual(values["runtime_label"], "//dockers/docker-orchagent:docker-orchagent.gz")
        self.assertEqual(values["debug_label"], "//dockers/docker-orchagent:docker-orchagent-dbg.gz")
        self.assertEqual(values["runtime_inputs"].split(),
                         ["target/docker-config-engine-trixie.oci", "target/python-wheels/trixie/scapy.whl",
                          "target/bazel-manifests/docker-orchagent/manifest.json"])
        self.assertEqual(values["debug_inputs"].split(), values["runtime_inputs"].split() +
                         ["target/bazel-manifests/docker-orchagent-dbg/manifest.json"])
        self.assertEqual(values["debug_path"], "dockers/docker-orchagent")
        self.assertEqual(values["oci_bases"], "docker-config-engine-trixie.oci")
        self.assertEqual(values["oci_archive"], "target/docker-config-engine-trixie.gz")
        self.assertEqual(values["oci_platform"], "linux/amd64")
        self.assertEqual(
            values["legacy"].split(),
            ["docker-sysmgr.gz", "docker-swss-layer-trixie.gz"],
        )
        self.assertEqual(values["debug"], "")
        self.assertEqual(values["install"], "docker-orchagent.gz")
        # The combined docker-sonic-vs Make build imports this metadata.
        self.assertEqual(values["swss_packages"], "swss.deb dash.deb")
        self.assertEqual(values["swss_wheels"], "scapy.whl")

    def test_other_slave_phases_stay_on_make(self):
        """Prevent the SWSS opt-in from introducing Trixie Bazel work into other distribution
        phases.
        """
        for distro in ("bookworm", "bullseye"):
            with self.subTest(distro=distro):
                values = self.values(self.select(BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", BLDENV=distro))
                self.assertEqual(values["bazel"], "")
                self.assertEqual(values["oci_bases"], "")
                self.assertEqual(values["switchable"], "")

    def test_fips_consumes_source_base_without_extracting_its_docker_archive(self):
        values = self.values(self.select(BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", INCLUDE_FIPS="y"))
        self.assertEqual(values["oci_bases"], "")
        self.assertEqual(values["oci_archive"], "")
        self.assertEqual(values["runtime_args"], "--//dockers/docker-orchagent:source_config_engine=true")
        self.assertEqual(values["debug_args"], values["runtime_args"])
        self.assertEqual(values["runtime_inputs"].split(), [
            "target/bazel-manifests/docker-base-trixie/manifest.json",
            "target/bazel-manifests/docker-config-engine-trixie/manifest.json",
            "target/python-wheels/trixie/scapy.whl",
            "target/bazel-manifests/docker-orchagent/manifest.json",
        ])
        self.assertEqual(values["debug_inputs"].split(), values["runtime_inputs"].split() +
                         ["target/bazel-manifests/docker-orchagent-dbg/manifest.json"])
        legacy = self.values(self.select(BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", INCLUDE_FIPS="n"))
        self.assertEqual(legacy["runtime_args"], "")
        self.assertEqual(legacy["debug_args"], "")
        self.assertEqual(legacy["oci_bases"], "docker-config-engine-trixie.oci")

    def test_unavailable_bazel_configurations_preserve_make_inputs(self):
        """The global opt-in keeps the original Make build when SWSS has no supported target."""
        for setting in (
            {"CONFIGURED_PLATFORM": "mellanox"},
            {"CONFIGURED_PLATFORM": ""},
            {"CONFIGURED_ARCH": "arm64"},
            {"CONFIGURED_ARCH": "armhf"},
            {"CONFIGURED_ARCH": ""},
            {"ENABLE_ASAN": "y"},
            {"CROSS_BUILD_ENVIRON": "y"},
            {"MULTIARCH_QEMU_ENVIRON": "y"},
        ):
            with self.subTest(setting=setting):
                values = self.values(self.select(BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", **setting))
                self.assertEqual(values, self.values(self.select(**setting)))
                for name in ("bazel", "bazel_debug", "runtime_label", "debug_label",
                             "runtime_inputs", "debug_inputs", "manifests", "oci_bases",
                             "oci_archive", "oci_platform"):
                    self.assertEqual(values[name], "", name)
                self.assertIn("docker-orchagent.gz", values["legacy"].split())
                self.assertEqual(values["debug"], "docker-orchagent-dbg.gz")
                self.assertEqual(values["install"], "docker-orchagent.gz")
                packages = ["swss.deb", "dash.deb"]
                if setting.get("ENABLE_ASAN") == "y":
                    packages.append("swss-dbgsym.deb")
                self.assertEqual(values["swss_packages"].split(), packages)
                self.assertEqual(values["swss_wheels"], "scapy.whl")
                self.assertEqual(values["swss_base"], "docker-swss-layer-trixie.gz")

    def test_make_remains_available_for_other_platforms(self):
        """Keep Bazel support restrictions from narrowing the platforms supported by the existing
        Make path.
        """
        values = self.values(self.select(CONFIGURED_PLATFORM="mellanox", CONFIGURED_ARCH="arm64"))
        self.assertEqual(values["bazel"], "")

    def test_invalid_selector_fails(self):
        """Reject ambiguous or mistyped selector values before they can choose an unintended
        builder.
        """
        for selector in ("yes", "y n", ""):
            with self.subTest(selector=selector):
                result = self.select(BUILD_WITH_BAZEL_WHEN_AVAILABLE=selector)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("must be y or n", result.stderr)



class MakeArchiveRecipeTest(ArchiveRecipeFixture, unittest.TestCase):
    """Check SWSS-owned prerequisites through the actual generic bridge."""

    archives = ("docker-orchagent.gz", "docker-orchagent-dbg.gz")
    labels = {archive: "//dockers/docker-orchagent:" + archive for archive in archives}

    def setUp(self):
        from tools.bazel.tests.oci_base_fixture import docker_save, image_fixture

        super().setUp()
        for directory in ("dockers", "rules", "files"):
            (self.root / directory).symlink_to(ROOT / directory, target_is_directory=True)
        (self.root / "scripts/j2cli").symlink_to(ROOT / "scripts/j2cli", target_is_directory=True)
        # The production slave loads this generator before the shared bridge.
        generator = (ROOT / "rules/functions").read_text().split("define generate_manifest\n", 1)[1].split("endef", 1)[0]
        self.makefile += "\ndefine generate_manifest\n" + generator + "endef\n"
        self.base = self.target / "docker-config-engine-trixie.gz"
        docker_save(self.base, *image_fixture())
        self.wheel = self.target / "python-wheels/trixie/scapy.whl"
        self.wheel.parent.mkdir(parents=True)
        self.wheel.write_bytes(b"cached Make wheel")
        self.prerequisites = {archive: ["target/docker-config-engine-trixie.oci/index.json",
                                        "target/python-wheels/trixie/scapy.whl",
                                        "target/bazel-manifests/docker-orchagent/manifest.json"]
                              for archive in self.archives}
        self.prerequisites[self.archives[1]].append("target/bazel-manifests/docker-orchagent-dbg/manifest.json")
        self.makefile += """
BUILD_WITH_BAZEL_WHEN_AVAILABLE = y
BLDENV = trixie
CONFIGURED_PLATFORM = vs
CONFIGURED_ARCH = amd64
ENABLE_ASAN = n
DBG_IMAGE_MARK = dbg
DOCKERS_PATH = dockers
DOCKER_CONFIG_ENGINE_TRIXIE = docker-config-engine-trixie.gz
PYTHON_WHEELS_PATH = target/python-wheels/trixie
SCAPY = scapy.whl
include rules/docker-orchagent.mk
include tools/bazel/docker.mk
"""

    def test_source_base_prepares_both_variants_without_a_make_base_archive(self):
        self.makefile = self.makefile.replace("include rules/docker-orchagent.mk",
                                              "INCLUDE_FIPS = y\ninclude rules/docker-orchagent.mk")
        self.base.unlink()
        source_manifests = ["target/bazel-manifests/" + name + "/manifest.json"
                            for name in ("docker-base-trixie", "docker-config-engine-trixie")]
        for name in source_manifests:
            path = self.root / name
            path.parent.mkdir(parents=True)
            path.write_text("{}\n")
        for name in self.archives:
            self.prerequisites[name] = source_manifests + self.prerequisites[name][1:]
        result = self.run_make(*self.archives, parallel=True)
        self.assert_success(result)
        calls = self.recorded_calls()
        self.assertEqual(len(calls), 4)
        self.assertTrue(all(call['prerequisites_ready'] for call in calls))
        self.assertTrue(all("--//dockers/docker-orchagent:source_config_engine=true" in call['arguments']
                            for call in calls))
        self.assertNotIn("prepare_oci_base.py --archive", result.stdout)
        self.assertFalse((self.target / "docker-config-engine-trixie.oci").exists())

    def test_make_prerequisite_failures_stop_before_bazel(self):
        """Keep the previous SWSS archive when its Make-provided base or wheel is missing, or the
        base is invalid.
        """
        for missing in (self.base, self.wheel):
            with self.subTest(missing=missing.name):
                original = missing.read_bytes()
                missing.unlink()
                result = self.run_make()
                self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(self.recorded_calls(), [])
                self.assertEqual((self.target / self.archives[0]).read_bytes(), b'previous image')
                missing.write_bytes(original)
        self.base.write_bytes(b'invalid Make base archive')
        result = self.run_make()
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(self.recorded_calls(), [])
        self.assertEqual((self.target / self.archives[0]).read_bytes(), b'previous image')

    def test_debug_alone_prepares_both_manifests(self):
        """Supply both JSON inputs without building the runtime archive for a debug-only request."""
        self.assert_success(self.run_make(self.archives[1]))
        calls = self.recorded_calls()
        self.assertEqual(len(calls), 2)
        self.assertTrue(all(call["prerequisites_ready"] for call in calls))
        self.assertEqual((self.target / self.archives[0]).read_bytes(), b"previous image")

    def test_manifest_failure_stops_before_bazel(self):
        """Stop archive publication when the existing Make manifest renderer fails."""
        commands = self.root / "commands"
        commands.mkdir()
        renderer = commands / "j2"
        renderer.write_text("#!/bin/sh\nexit 32\n")
        renderer.chmod(0o755)
        result = self.run_make(environment={"PATH": str(commands) + os.pathsep + os.environ["PATH"]})
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(self.recorded_calls(), [])
        self.assertEqual((self.target / self.archives[0]).read_bytes(), b"previous image")

    def test_swss_declarations_prepare_shared_base_before_both_consumers(self):
        """Verify the actual SWSS declarations prepare one shared base before either runtime or
        debug invokes Bazel.
        """
        result = self.run_make(*self.archives, parallel=True)
        self.assert_success(result)
        self.assertEqual(result.stdout.count('python3 tools/bazel/oci/prepare_oci_base.py'), 1)
        self.assertTrue((self.target / 'docker-config-engine-trixie.oci/index.json').is_file())
        calls = self.recorded_calls()
        self.assertEqual(len(calls), 4)
        self.assertTrue(all(call['prerequisites_ready'] for call in calls))
        for archive in self.archives:
            self.assertEqual((self.target / archive).read_bytes(), ('new ' + archive).encode())
            self.assertEqual([call['arguments'][0] for call in calls
                              if call['arguments'][-1] == self.labels[archive]], ['build', 'cquery'])


if __name__ == "__main__":
    unittest.main()
