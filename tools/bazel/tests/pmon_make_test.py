"""Exercise PMON archive selection through its production Make rule."""

from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory
import unittest


ROOT = Path(__file__).resolve().parents[3]
PROFILE = {
    "BLDENV": "trixie", "CONFIGURED_PLATFORM": "vs", "CONFIGURED_ARCH": "amd64",
    "BUILD_WITH_BAZEL_WHEN_AVAILABLE": "y", "INCLUDE_FIPS": "y", "DBG_IMAGE_MARK": "dbg",
}
RECIPE = """include rules/config
PDDF_SUPPORT = $(if $(filter y,$(SONIC_USE_PDDF_FRAMEWORK)),y,n)
include rules/docker-platform-monitor.mk
TARGET_PATH := target
DOCKERS_PATH := dockers
SONIC_BUILD_TARGET := target/sonic-vs.bin
sonic-vs.bin_DOCKERS := $(DOCKER_PLATFORM_MONITOR)
include tools/bazel/docker.mk
selection:
	@echo '$(SONIC_BAZEL_DOCKER_IMAGES)' '$(SONIC_BAZEL_DBG_DOCKER_IMAGES)'
	@echo '$(SONIC_INSTALL_DOCKER_IMAGES)' '$(SONIC_INSTALL_DOCKER_DBG_IMAGES)'
	@echo '$(strip $($(DOCKER_PLATFORM_MONITOR)_PYTHON_WHEELS))'
	@echo '$($(DOCKER_PLATFORM_MONITOR)_BAZEL_ARGS)' '$($(DOCKER_PLATFORM_MONITOR_DBG)_BAZEL_ARGS)'
	@echo '$(SONIC_BAZEL_VS_BATCH_ARCHIVES)'
"""


class PmonMakeTest(unittest.TestCase):
    def test_both_archives_generate_the_selected_make_image_version(self):
        makefile = """include rules/docker-platform-monitor.mk
include tools/bazel/manifests.mk
runtime: $(filter %/image.env,$($(DOCKER_PLATFORM_MONITOR)_BAZEL_DEPENDS))
debug: $(filter %/image.env,$($(DOCKER_PLATFORM_MONITOR_DBG)_BAZEL_DEPENDS))
"""
        for variant in ("runtime", "debug"):
            with self.subTest(variant=variant), TemporaryDirectory() as output:
                subprocess.run(
                    ["make", "-sf", "-", variant, f"TARGET_PATH={output}",
                     "SONIC_IMAGE_VERSION=pmon-version-test",
                     *[f"{key}={value}" for key, value in PROFILE.items()]],
                    cwd=ROOT, input=makefile, text=True, capture_output=True, check=True,
                )
                environment = Path(output) / "bazel-manifests/docker-platform-monitor/image.env"
                self.assertEqual(environment.read_text(), "DEBIAN_FRONTEND=noninteractive\nIMAGE_VERSION=pmon-version-test\n")

    def selection(self, **overrides):
        settings = PROFILE | overrides
        result = subprocess.run(
            ["make", "-sf", "-", "selection", "PDDF_PLATFORM_API_BASE_PY3=pddf.whl",
             *[f"{key}={value}" for key, value in settings.items()]],
            cwd=ROOT, text=True, input=RECIPE, capture_output=True, check=True,
        )
        return [line.split() for line in result.stdout.splitlines()]

    def test_only_supported_profile_switches_both_producers(self):
        images = ["docker-platform-monitor.gz", "docker-platform-monitor-dbg.gz"]
        enabled = "--//dockers/docker-platform-monitor:pddf=true"
        disabled = "--//dockers/docker-platform-monitor:pddf=false"
        for pddf, flag, wheels in (("y", enabled, ["pddf.whl"]), ("n", disabled, [])):
            for debug, batch in (("n", images[:1]), ("y", sorted(images))):
                with self.subTest(pddf=pddf, debug=debug):
                    options = {"SONIC_ENABLE_DEBUG_LAYER": debug}
                    if pddf == "n":
                        options["SONIC_USE_PDDF_FRAMEWORK"] = pddf
                    self.assertEqual(self.selection(**options), [images, images, wheels, [flag, flag], batch])
        for key, value in {
            "BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n", "CONFIGURED_ARCH": "arm64",
            "CONFIGURED_PLATFORM": "mellanox", "BLDENV": "bookworm", "INCLUDE_FIPS": "n",
            "DBG_IMAGE_MARK": "custom", "CROSS_BUILD_ENVIRON": "y",
            "MULTIARCH_QEMU_ENVIRON": "y", "ENABLE_ASAN": "y",
        }.items():
            with self.subTest(key=key, value=value):
                selected, installed, wheels, options, batch = self.selection(**{key: value})
                self.assertEqual(selected, [])
                debug = f"docker-platform-monitor-{value}.gz" if key == "DBG_IMAGE_MARK" else images[1]
                self.assertEqual(installed, [images[0], debug])
                self.assertEqual(wheels, ["pddf.whl"])
                self.assertEqual(options, [])
                self.assertEqual(batch, [])


if __name__ == "__main__":
    unittest.main()
