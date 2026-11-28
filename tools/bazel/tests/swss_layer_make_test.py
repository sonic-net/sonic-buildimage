"""Check the real shared-layer Make selector without running a build."""

from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[3]


class SwssLayerMakeTest(unittest.TestCase):
    def evaluate(self, **changes):
        values = dict(BLDENV="trixie", CONFIGURED_ARCH="amd64", CONFIGURED_PLATFORM="vs",
                      BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", DOCKERS_PATH="dockers",
                      TARGET_PATH="target", SWSS="swss.deb", SWSS_DBG="swss-dbgsym.deb",
                      DOCKER_CONFIG_ENGINE_TRIXIE="docker-config-engine-trixie.gz")
        values.update(changes)
        recipe = """include rules/docker-swss-layer-trixie.mk
.PHONY: result
result:
	@printf '%s\\n' '$(SONIC_BAZEL_DOCKER_IMAGES)' '$($(DOCKER_SWSS_LAYER_TRIXIE)_BAZEL_TARGET)' '$($(DOCKER_SWSS_LAYER_TRIXIE)_BAZEL_DEPENDS)' '$($(DOCKER_SWSS_LAYER_TRIXIE)_DEPENDS)' '$($(DOCKER_SWSS_LAYER_TRIXIE)_LOAD_DOCKERS)'
"""
        return subprocess.check_output(["make", "--no-print-directory", "-f", "-", "result",
                                        *[key + "=" + value for key, value in values.items()]],
                                       input=recipe, text=True, cwd=ROOT).splitlines()

    def test_native_vs_registers_archive_without_project_package_prerequisites(self):
        selected, target, prerequisites, legacy, base = self.evaluate()
        self.assertEqual(selected, "docker-swss-layer-trixie.gz")
        self.assertEqual(target, "//dockers/docker-swss-layer-trixie:docker-swss-layer-trixie.gz")
        self.assertEqual(set(prerequisites.split()), {
            "target/bazel-manifests/" + image + "/manifest.json" for image in
            ("docker-base-trixie", "docker-config-engine-trixie", "docker-swss-layer-trixie")})
        self.assertEqual(legacy, "swss.deb")
        self.assertEqual(base, "docker-config-engine-trixie.gz")

    def test_excluded_profiles_keep_the_existing_make_recipe(self):
        for settings in ({"CONFIGURED_ARCH": "arm64"}, {"CONFIGURED_ARCH": "armhf"},
                         {"CONFIGURED_PLATFORM": "broadcom"}, {"BLDENV": "bookworm"},
                         {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"}, {"ENABLE_ASAN": "y"},
                         {"CROSS_BUILD_ENVIRON": "y"}, {"MULTIARCH_QEMU_ENVIRON": "y"}):
            with self.subTest(settings=settings):
                selected, target, prerequisites, legacy, base = self.evaluate(**settings)
                self.assertEqual((selected, target, prerequisites), ("", "", ""))
                self.assertEqual(legacy, "swss.deb")
                self.assertEqual(base, "docker-config-engine-trixie.gz")


if __name__ == "__main__":
    unittest.main()
