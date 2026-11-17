"""Exercise Teamd's real Make selector while retaining host service metadata."""

from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[3]


class TeamdMakeTest(unittest.TestCase):
    def evaluate(self, **changes):
        values = dict(BLDENV="trixie", CONFIGURED_ARCH="amd64", CONFIGURED_PLATFORM="vs",
                      INCLUDE_FIPS="y", INCLUDE_TEAMD="y", DBG_IMAGE_MARK="dbg",
                      BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", DOCKERS_PATH="dockers",
                      TARGET_PATH="target", LIBTEAMDCTL="libteamdctl.deb",
                      LIBTEAM_UTILS="libteam-utils.deb",
                      DOCKER_SWSS_LAYER_TRIXIE="docker-swss-layer-trixie.gz")
        values.update(changes)
        variables = ["SONIC_BAZEL_DOCKER_IMAGES", "SONIC_BAZEL_DBG_DOCKER_IMAGES",
                     "SONIC_BAZEL_SWITCHABLE_IMAGES", "docker-teamd.gz_BAZEL_TARGET",
                     "docker-teamd-$(DBG_IMAGE_MARK).gz_BAZEL_TARGET",
                     "docker-teamd-$(DBG_IMAGE_MARK).gz_BAZEL_DEPENDS",
                     "docker-teamd.gz_DEPENDS", "docker-teamd.gz_LOAD_DOCKERS",
                     "docker-teamd.gz_BASE_IMAGE_FILES", "docker-teamd.gz_RUN_OPT",
                     "docker-teamd.gz_WARM_SHUTDOWN_BEFORE", "docker-teamd.gz_WARM_SHUTDOWN_AFTER",
                     "SONIC_INSTALL_DOCKER_IMAGES", "SONIC_INSTALL_DOCKER_DBG_IMAGES",
                     "docker-teamd.gz_BAZEL_ARGS", "docker-teamd-dbg.gz_BAZEL_ARGS", "docker-teamd-dbg.gz_PATH"]
        recipe = "include rules/docker-teamd.mk\n.PHONY: result\nresult:\n\t@printf '%s\\n' "
        recipe += " ".join("'$(" + variable + ")'" for variable in variables) + "\n"
        return subprocess.check_output(["make", "--no-print-directory", "-f", "-", "result",
                                        *[key + "=" + value for key, value in values.items()]],
                                       input=recipe, cwd=ROOT, text=True).splitlines()

    def test_selected_runtime_and_debug_use_only_manifest_prerequisites(self):
        result = self.evaluate(SONIC_IMAGE_VERSION="202705.42")
        self.assertEqual(result[:2], ["docker-teamd.gz", "docker-teamd-dbg.gz"])
        self.assertEqual(set(result[2].split()), set(result[:2]))
        self.assertEqual(result[3:5], ["//dockers/docker-teamd:docker-teamd.gz",
                                     "//dockers/docker-teamd:docker-teamd-dbg.gz"])
        self.assertEqual(set(result[5].split()), {
            "target/bazel-manifests/" + name + "/manifest.json" for name in
            ("docker-base-trixie", "docker-config-engine-trixie", "docker-swss-layer-trixie",
             "docker-teamd", "docker-teamd-dbg")})
        self.assertEqual(result[6].split(), ["libteamdctl.deb", "libteam-utils.deb"])
        self.assertEqual(result[7], "docker-swss-layer-trixie.gz")
        self.assertEqual(result[8], "teamdctl:/usr/bin/teamdctl")
        self.assertIn("--cap-add=NET_ADMIN", result[9])
        self.assertIn("-v /etc/sonic:/etc/sonic:ro", result[9])
        self.assertEqual(result[10:12], ["syncd", "swss"])
        self.assertEqual(result[12:14], result[:2])
        self.assertEqual(result[14:16], ["--//dockers/docker-teamd:image_version=202705.42"] * 2)
        self.assertEqual(result[16], "dockers/docker-teamd")

    def test_excluded_profiles_keep_legacy_inputs_and_service_selection(self):
        for settings in ({"BLDENV": "bookworm"}, {"CONFIGURED_ARCH": "arm64"},
                         {"CONFIGURED_ARCH": "armhf"}, {"CONFIGURED_PLATFORM": "broadcom"},
                         {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"}, {"INCLUDE_FIPS": "n"},
                         {"DBG_IMAGE_MARK": "debug"}, {"ENABLE_ASAN": "y"},
                         {"CROSS_BUILD_ENVIRON": "y"}, {"MULTIARCH_QEMU_ENVIRON": "y"}):
            with self.subTest(settings=settings):
                result = self.evaluate(**settings)
                self.assertEqual(result[:2], ["", ""])
                self.assertEqual(result[3:6], ["", "", ""])
                self.assertEqual(result[6].split(), ["libteamdctl.deb", "libteam-utils.deb"])
                self.assertEqual(result[8], "teamdctl:/usr/bin/teamdctl")
                self.assertEqual(result[10:12], ["syncd", "swss"])
                if settings.get("BLDENV", "trixie") == "trixie":
                    self.assertIn("docker-teamd.gz", result[2])

    def test_include_flag_controls_installer_membership(self):
        result = self.evaluate(INCLUDE_TEAMD="n")
        self.assertEqual(result[:2], ["docker-teamd.gz", "docker-teamd-dbg.gz"])
        self.assertEqual(result[12:14], ["", ""])


if __name__ == "__main__":
    unittest.main()
