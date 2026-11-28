"""Use the real base-image owner rules to check Make's producer selection."""

from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]


class SourceBaseMakeTest(unittest.TestCase):
    def evaluate(self, **settings):
        configuration = {
            "BLDENV": "trixie", "CONFIGURED_PLATFORM": "vs", "CONFIGURED_ARCH": "amd64",
            "INCLUDE_FIPS": "y", "BUILD_WITH_BAZEL_WHEN_AVAILABLE": "y",
            "DOCKERS_PATH": "dockers", "TARGET_PATH": "target", "SOCAT": "socat.deb",
        }
        configuration.update(settings)
        with tempfile.TemporaryDirectory() as directory:
            recipe = Path(directory) / "Makefile"
            recipe.write_text(
                "include rules/docker-base-trixie.mk\n"
                "include rules/docker-config-engine-trixie.mk\n"
                ".PHONY: result\nresult:\n"
                "\t@printf '%s\\n' '$(sort $(SONIC_BAZEL_DOCKER_IMAGES))' "
                "'$(sort $(SONIC_BAZEL_SWITCHABLE_IMAGES))' "
                "'$($(DOCKER_BASE_TRIXIE)_DEPENDS)' "
                "'$($(DOCKER_CONFIG_ENGINE_TRIXIE)_BAZEL_DEPENDS)'\n")
            return subprocess.check_output(
                ["make", "--no-print-directory", "-f", str(recipe), "result",
                 *[key + "=" + value for key, value in configuration.items()]],
                cwd=ROOT, text=True).splitlines()

    def test_enabled_flag_selects_both_source_images_and_base_manifest(self):
        selected, switchable, legacy, prerequisites = self.evaluate()
        self.assertEqual(selected.split(), ["docker-base-trixie.gz", "docker-config-engine-trixie.gz"])
        self.assertEqual(switchable.split(), selected.split())
        self.assertIn("socat.deb", legacy)
        self.assertEqual(set(prerequisites.split()), {
            "target/bazel-manifests/docker-base-trixie/manifest.json",
            "target/bazel-manifests/docker-config-engine-trixie/manifest.json",
        })

    def test_disabled_flag_retains_switch_tracking_and_legacy_inputs(self):
        selected, switchable, legacy, _ = self.evaluate(BUILD_WITH_BAZEL_WHEN_AVAILABLE="n")
        self.assertEqual(selected, "")
        self.assertEqual(len(switchable.split()), 2)
        self.assertIn("socat.deb", legacy)

    def test_platform_architecture_and_fips_do_not_override_the_global_selector(self):
        for platform in ("vs", "broadcom", "mellanox"):
            for architecture in ("amd64", "arm64", "armhf"):
                for fips in ("y", "n"):
                    with self.subTest(platform=platform, architecture=architecture, fips=fips):
                        selected = self.evaluate(CONFIGURED_PLATFORM=platform,
                                                 CONFIGURED_ARCH=architecture,
                                                 INCLUDE_FIPS=fips)[0]
                        self.assertEqual(selected.split(), [
                            "docker-base-trixie.gz", "docker-config-engine-trixie.gz"])

    def test_other_execution_modes_keep_legacy_producer(self):
        for settings in ({"ENABLE_ASAN": "y"}, {"CROSS_BUILD_ENVIRON": "y"},
                         {"MULTIARCH_QEMU_ENVIRON": "y"}):
            with self.subTest(settings=settings):
                self.assertEqual(self.evaluate(**settings)[0], "")


if __name__ == "__main__":
    unittest.main()
