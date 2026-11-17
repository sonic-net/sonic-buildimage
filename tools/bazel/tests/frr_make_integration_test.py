"""Exercise the FRR Make selector without building a package or container."""

from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[3]


class FrrMakeIntegrationTest(unittest.TestCase):
    def select(self, **overrides):
        settings = {
            "BUILD_WITH_BAZEL_WHEN_AVAILABLE": "y", "BLDENV": "trixie",
            "CONFIGURED_PLATFORM": "vs", "CONFIGURED_ARCH": "amd64",
            "INCLUDE_FIPS": "y", "ENABLE_ASAN": "n",
            "ENABLE_FRR_TCMALLOC": "y", "ENABLE_FRR_SNMP_AGENT": "y",
            "FRR_USER_UID": "300", "FRR_USER_GID": "300",
        }
        settings.update(overrides)
        recipe = """
DBG_IMAGE_MARK = dbg
DOCKERS_PATH = dockers
TARGET_PATH = target
FRR = frr.deb
FRR_SNMP = frr-snmp.deb
SWSS = swss.deb
LIBYANG3 = libyang3.deb
SONIC_BGPCFGD = bgpcfgd.whl
SONIC_FRR_MGMT_FRAMEWORK = frrcfgd.whl
DOCKER_SWSS_LAYER_TRIXIE = docker-swss-layer-trixie.gz
include rules/docker-fpm-frr.mk
.PHONY: selected
selected:
	@echo runtime=$(SONIC_BAZEL_DOCKER_IMAGES)
	@echo debug=$(SONIC_BAZEL_DBG_DOCKER_IMAGES)
	@echo switchable=$(SONIC_BAZEL_SWITCHABLE_IMAGES)
	@echo packages=$($(DOCKER_FPM_FRR)_DEPENDS)
	@echo wheels=$($(DOCKER_FPM_FRR)_PYTHON_WHEELS)
	@echo base=$($(DOCKER_FPM_FRR)_LOAD_DOCKERS)
	@echo wrappers=$($(DOCKER_FPM_FRR)_BASE_IMAGE_FILES)
	@echo target=$($(DOCKER_FPM_FRR)_BAZEL_TARGET)
	@echo inputs=$($(DOCKER_FPM_FRR)_BAZEL_DEPENDS)
	@echo debug_inputs=$($(DOCKER_FPM_FRR_DBG)_BAZEL_DEPENDS)
	@echo debug_path=$($(DOCKER_FPM_FRR_DBG)_PATH)
	@echo args=$($(DOCKER_FPM_FRR)_BAZEL_ARGS)
	@echo debug_args=$($(DOCKER_FPM_FRR_DBG)_BAZEL_ARGS)
"""
        output = subprocess.check_output(
            ["make", "--no-print-directory", "-f", "-", "selected",
             *[key + "=" + value for key, value in settings.items()]],
            input=recipe, text=True, cwd=ROOT,
        )
        return dict(line.split("=", 1) for line in output.splitlines())

    def test_selected_profile_preserves_legacy_metadata_and_uses_only_manifests(self):
        selected = self.select(SONIC_IMAGE_VERSION="202705.42")
        self.assertEqual(selected["runtime"], "docker-fpm-frr.gz")
        self.assertEqual(selected["debug"], "docker-fpm-frr-dbg.gz")
        self.assertEqual(selected["target"], "//dockers/docker-fpm-frr:docker-fpm-frr.gz")
        self.assertEqual(selected["packages"].split(), ["frr.deb", "swss.deb", "libyang3.deb", "frr-snmp.deb"])
        self.assertEqual(selected["wheels"].split(), ["bgpcfgd.whl", "frrcfgd.whl"])
        self.assertEqual(selected["base"], "docker-swss-layer-trixie.gz")
        self.assertEqual(len(selected["wrappers"].split()), 9)
        self.assertIn("vtysh:/usr/bin/vtysh", selected["wrappers"].split())
        self.assertEqual(selected["inputs"].split(), [
            "target/bazel-manifests/" + name + "/manifest.json"
            for name in ["docker-base-trixie", "docker-config-engine-trixie",
                         "docker-swss-layer-trixie", "docker-fpm-frr"]
        ])
        self.assertEqual(selected["debug_inputs"].split(), selected["inputs"].split() +
                         ["target/bazel-manifests/docker-fpm-frr-dbg/manifest.json"])
        self.assertEqual(selected["args"], selected["debug_args"])
        self.assertIn("--//dockers/docker-fpm-frr:image_version=202705.42", selected["args"].split())
        self.assertEqual(selected["debug_path"], "dockers/docker-fpm-frr")

    def test_other_profiles_retain_the_make_builder(self):
        for settings in [
            {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"}, {"BLDENV": "bookworm"},
            {"CONFIGURED_PLATFORM": "broadcom"}, {"CONFIGURED_ARCH": "arm64"},
            {"INCLUDE_FIPS": "n"}, {"ENABLE_ASAN": "y"},
            {"CROSS_BUILD_ENVIRON": "y"}, {"MULTIARCH_QEMU_ENVIRON": "y"},
            {"ENABLE_FRR_TCMALLOC": "n"}, {"ENABLE_FRR_SNMP_AGENT": "n"},
            {"FRR_USER_UID": "301"}, {"FRR_USER_GID": "301"},
            {"DBG_IMAGE_MARK": "debug"},
        ]:
            with self.subTest(settings=settings):
                selected = self.select(**settings)
                self.assertEqual(selected["runtime"], "")
                self.assertEqual(selected["debug"], "")
                self.assertEqual(selected["inputs"], "")
                self.assertIn("frr.deb", selected["packages"].split())
                self.assertEqual(len(selected["wrappers"].split()), 9)


if __name__ == "__main__":
    unittest.main()
