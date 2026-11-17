"""Exercise the real sFlow selector and preserve legacy consumer metadata."""

from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[3]


class SflowMakeTest(unittest.TestCase):
    def evaluate(self, **changes):
        values = dict(BLDENV="trixie", CONFIGURED_ARCH="amd64", CONFIGURED_PLATFORM="vs",
                      INCLUDE_FIPS="y", INCLUDE_SFLOW="y", DBG_IMAGE_MARK="dbg",
                      BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", DOCKERS_PATH="dockers", TARGET_PATH="target",
                      SWSS="swss.deb", HSFLOWD="hsflowd.deb", SFLOWTOOL="sflowtool.deb", PSAMPLE="psample.deb",
                      DOCKER_SWSS_LAYER_TRIXIE="docker-swss-layer-trixie.gz")
        values.update(changes)
        variables = ["SONIC_BAZEL_DOCKER_IMAGES", "SONIC_BAZEL_DBG_DOCKER_IMAGES",
                     "SONIC_BAZEL_SWITCHABLE_IMAGES", "docker-sflow.gz_BAZEL_TARGET",
                     "docker-sflow-$(DBG_IMAGE_MARK).gz_BAZEL_TARGET",
                     "docker-sflow-$(DBG_IMAGE_MARK).gz_BAZEL_DEPENDS",
                     "docker-sflow.gz_DEPENDS", "docker-sflow.gz_LOAD_DOCKERS",
                     "docker-sflow.gz_RUN_OPT", "docker-sflow.gz_BASE_IMAGE_FILES",
                     "docker-sflow.gz_WARM_SHUTDOWN_BEFORE", "docker-sflow.gz_FAST_SHUTDOWN_BEFORE",
                     "SONIC_INSTALL_DOCKER_IMAGES", "SONIC_INSTALL_DOCKER_DBG_IMAGES"]
        recipe = "include rules/docker-sflow.mk\n.PHONY: result\nresult:\n\t@printf '%s\\n' "
        recipe += " ".join("'$(" + variable + ")'" for variable in variables) + "\n"
        return subprocess.check_output(["make", "--no-print-directory", "-f", "-", "result",
                                        *[key + "=" + value for key, value in values.items()]],
                                       input=recipe, cwd=ROOT, text=True).splitlines()

    def test_manifest_handoff_preserves_package_and_host_consumers(self):
        result = self.evaluate()
        self.assertEqual(result[:2], ["docker-sflow.gz", "docker-sflow-dbg.gz"])
        self.assertEqual(set(result[2].split()), set(result[:2]))
        self.assertEqual(result[3:5], ["//dockers/docker-sflow:docker-sflow.gz",
                                     "//dockers/docker-sflow:docker-sflow-dbg.gz"])
        self.assertEqual(set(result[5].split()), {
            "target/bazel-manifests/" + name + "/manifest.json" for name in
            ("docker-base-trixie", "docker-config-engine-trixie", "docker-swss-layer-trixie",
             "docker-sflow", "docker-sflow-dbg")})
        self.assertEqual(result[6:8], ["swss.deb hsflowd.deb sflowtool.deb psample.deb",
                                      "docker-swss-layer-trixie.gz"])
        self.assertEqual(result[8].split(), ["-t", "--cap-add=NET_ADMIN", "--cap-add=SYS_ADMIN",
                                            "-v", "/etc/sonic:/etc/sonic:ro", "-v", "/etc/localtime:/etc/localtime:ro"])
        self.assertEqual(result[9].split(), ["psample:/usr/bin/psample", "sflowtool:/usr/bin/sflowtool"])
        self.assertEqual(result[10:], ["swss", "swss", "docker-sflow.gz", "docker-sflow-dbg.gz"])

    def test_excluded_profiles_retain_legacy_producer_inputs(self):
        baseline = self.evaluate()
        for settings in ({"BLDENV": "bookworm"}, {"CONFIGURED_ARCH": "arm64"},
                         {"CONFIGURED_ARCH": "armhf"}, {"CONFIGURED_PLATFORM": "broadcom"},
                         {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"}, {"INCLUDE_FIPS": "n"},
                         {"ENABLE_ASAN": "y"}, {"CROSS_BUILD_ENVIRON": "y"},
                         {"MULTIARCH_QEMU_ENVIRON": "y"}):
            with self.subTest(settings=settings):
                result = self.evaluate(**settings)
                self.assertEqual(result[:2], ["", ""])
                self.assertEqual(result[3:6], ["", "", ""])
                self.assertEqual(result[6:], baseline[6:])
                if settings.get("BLDENV", "trixie") == "trixie":
                    self.assertIn("docker-sflow.gz", result[2])

    def test_install_selection_and_debug_name_remain_explicit(self):
        self.assertEqual(self.evaluate(INCLUDE_SFLOW="n")[12:], ["", ""])
        renamed = self.evaluate(DBG_IMAGE_MARK="debug")
        self.assertEqual(renamed[:2], ["", ""])
        self.assertEqual(renamed[12:], ["docker-sflow.gz", "docker-sflow-debug.gz"])


if __name__ == "__main__":
    unittest.main()
