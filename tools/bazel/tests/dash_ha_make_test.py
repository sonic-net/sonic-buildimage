"""Exercise the real HA Make selectors and preserve the legacy install contract."""
from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[3]


class DashHaMakeTest(unittest.TestCase):
    def evaluate(self, **changes):
        values = dict(BLDENV="trixie", CONFIGURED_PLATFORM="vs", CONFIGURED_ARCH="amd64",
                      INCLUDE_FIPS="y", INCLUDE_DASH_HA="y", DBG_IMAGE_MARK="dbg",
                      BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", DOCKERS_PATH="dockers", TARGET_PATH="target",
                      DASH_HA="dash-ha_1.0.0_amd64.deb", DASH_HA_DBG="dash-ha-dbgsym.deb",
                      DOCKER_SWSS_LAYER_TRIXIE="docker-swss-layer-trixie.gz")
        values.update(changes)
        variables = ["SONIC_BAZEL_DOCKER_IMAGES", "SONIC_BAZEL_DBG_DOCKER_IMAGES",
                     "SONIC_BAZEL_SWITCHABLE_IMAGES", "docker-dash-ha.gz_BAZEL_TARGET",
                     "docker-dash-ha-$(DBG_IMAGE_MARK).gz_BAZEL_TARGET",
                     "docker-dash-ha-$(DBG_IMAGE_MARK).gz_BAZEL_DEPENDS",
                     "docker-dash-ha.gz_DEPENDS", "docker-dash-ha.gz_DBG_DEPENDS",
                     "docker-dash-ha.gz_LOAD_DOCKERS", "docker-dash-ha.gz_CONTAINER_NAME",
                     "docker-dash-ha.gz_RUN_OPT", "docker-dash-ha.gz_WARM_SHUTDOWN_BEFORE",
                     "docker-dash-ha.gz_WARM_SHUTDOWN_AFTER", "docker-dash-ha.gz_FAST_SHUTDOWN_BEFORE",
                     "docker-dash-ha.gz_FAST_SHUTDOWN_AFTER", "SONIC_INSTALL_DOCKER_IMAGES",
                     "SONIC_INSTALL_DOCKER_DBG_IMAGES"]
        recipe = "include rules/docker-dash-ha.mk\n.PHONY: result\nresult:\n\t@printf '%s\\n' "
        recipe += " ".join("'$(" + name + ")'" for name in variables) + "\n"
        output = subprocess.check_output(["make", "--no-print-directory", "-f", "-", "result",
            *[key + "=" + value for key, value in values.items()]], input=recipe, cwd=ROOT, text=True)
        return [line.strip() for line in output.splitlines()]

    def test_selected_image_targets_have_all_parent_manifests(self):
        result = self.evaluate()
        self.assertEqual(result[:2], ["docker-dash-ha.gz", "docker-dash-ha-dbg.gz"])
        self.assertEqual(set(result[2].split()), set(result[:2]))
        self.assertEqual(result[3:5], ["//dockers/docker-dash-ha:docker-dash-ha.gz", "//dockers/docker-dash-ha:docker-dash-ha-dbg.gz"])
        self.assertEqual(set(result[5].split()), {"target/bazel-manifests/" + name + "/manifest.json" for name in
            ("docker-base-trixie", "docker-config-engine-trixie", "docker-swss-layer-trixie", "docker-dash-ha", "docker-dash-ha-dbg")})

    def test_native_packages_mounts_and_shutdown_order_remain(self):
        result = self.evaluate()
        self.assertEqual(result[6:10], ["dash-ha_1.0.0_amd64.deb", "dash-ha-dbgsym.deb", "docker-swss-layer-trixie.gz", "dash-ha"])
        self.assertEqual(result[10].split(), ["-t", "-v", "/etc/sonic:/etc/sonic:ro", "-v", "/etc/localtime:/etc/localtime:ro"])
        self.assertEqual(result[11:15], ["syncd", "swss", "syncd", "swss"])

    def test_unsupported_profiles_keep_legacy_producers(self):
        baseline = self.evaluate()
        for changes in ({"BLDENV": "bookworm"}, {"CONFIGURED_ARCH": "arm64"}, {"CONFIGURED_ARCH": "armhf"},
                        {"CONFIGURED_PLATFORM": "broadcom"}, {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"},
                        {"INCLUDE_FIPS": "n"}, {"ENABLE_ASAN": "y"}, {"CROSS_BUILD_ENVIRON": "y"},
                        {"MULTIARCH_QEMU_ENVIRON": "y"}):
            with self.subTest(changes=changes):
                result = self.evaluate(**changes)
                self.assertEqual(result[:2], ["", ""])
                self.assertEqual(result[3:6], ["", "", ""])
                self.assertEqual(result[6:], baseline[6:])
                if changes.get("BLDENV", "trixie") == "trixie":
                    self.assertIn("docker-dash-ha.gz", result[2])

    def test_include_flag_controls_installer_selection(self):
        self.assertEqual(self.evaluate()[15:], ["docker-dash-ha.gz", "docker-dash-ha-dbg.gz"])
        self.assertEqual(self.evaluate(INCLUDE_DASH_HA="n")[15:], ["", ""])


if __name__ == "__main__":
    unittest.main()
