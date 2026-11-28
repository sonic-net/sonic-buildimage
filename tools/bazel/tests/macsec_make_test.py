"""Exercise MACsec's real Make selector without executing image/package recipes."""

from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[3]


class MacsecMakeTest(unittest.TestCase):
    def evaluate(self, *, include_bridge=False, **changes):
        values = dict(BLDENV="trixie", CONFIGURED_ARCH="amd64", CONFIGURED_PLATFORM="vs",
                      INCLUDE_FIPS="y", INCLUDE_MACSEC="y", INSTALL_DEBUG_TOOLS="n", DBG_IMAGE_MARK="dbg",
                      BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", DOCKERS_PATH="dockers", TARGET_PATH="target",
                      WPASUPPLICANT="wpasupplicant.deb", SONIC_UTILITIES_PY3="sonic_utilities.whl",
                      PYTHON3_SWSSCOMMON="python3-swsscommon.deb", LIBYANG3_PY3="python3-yang.deb",
                      DOCKER_SWSS_LAYER_TRIXIE="docker-swss-layer-trixie.gz")
        values.update(changes)
        variables = ["SONIC_BAZEL_DOCKER_IMAGES", "SONIC_BAZEL_DBG_DOCKER_IMAGES",
                     "SONIC_BAZEL_SWITCHABLE_IMAGES", "docker-macsec.gz_BAZEL_TARGET",
                     "docker-macsec-$(DBG_IMAGE_MARK).gz_BAZEL_TARGET",
                     "docker-macsec-$(DBG_IMAGE_MARK).gz_BAZEL_DEPENDS",
                     "docker-macsec.gz_DEPENDS", "docker-macsec.gz_LOAD_DOCKERS",
                     "docker-macsec.gz_INSTALL_PYTHON_WHEELS", "docker-macsec.gz_INSTALL_DEBS",
                     "docker-macsec.gz_SERVICE_REQUIRES", "docker-macsec.gz_SERVICE_AFTER",
                     "docker-macsec.gz_CONTAINER_PRIVILEGED", "docker-macsec.gz_CONTAINER_VOLUMES",
                     "docker-macsec.gz_CLI_CONFIG_PLUGIN", "docker-macsec.gz_CLI_SHOW_PLUGIN",
                     "docker-macsec.gz_CLI_CLEAR_PLUGIN", "SONIC_PACKAGES_LOCAL"]
        recipe = "include rules/docker-macsec.mk\n"
        if include_bridge:
            recipe += "include tools/bazel/docker.mk\n"
        recipe += ".PHONY: result\nresult:\n\t@printf '%s\\n' "
        recipe += " ".join("'$(" + variable + ")'" for variable in variables) + "\n"
        return subprocess.check_output(["make", "--no-print-directory", "-f", "-", "result",
                                        *[key + "=" + value for key, value in values.items()]],
                                       input=recipe, cwd=ROOT, text=True).splitlines()

    def test_selected_runtime_and_debug_use_manifest_prerequisites(self):
        result = self.evaluate()
        self.assertEqual(result[:2], ["docker-macsec.gz", "docker-macsec-dbg.gz"])
        self.assertEqual(set(result[2].split()), set(result[:2]))
        self.assertEqual(result[3:5], ["//dockers/docker-macsec:docker-macsec.gz",
                                     "//dockers/docker-macsec:docker-macsec-dbg.gz"])
        self.assertEqual(set(result[5].split()), {
            "target/bazel-manifests/" + name + "/manifest.json" for name in
            ("docker-base-trixie", "docker-config-engine-trixie", "docker-swss-layer-trixie",
             "docker-macsec", "docker-macsec-dbg")})
        self.assertEqual(result[6:13], ["wpasupplicant.deb", "docker-swss-layer-trixie.gz",
                                       "sonic_utilities.whl", "python3-swsscommon.deb python3-yang.deb",
                                       "config-setup", "swss syncd", "false"])
        self.assertEqual(result[13].split(), ["/etc/sonic:/etc/sonic:ro", "/etc/localtime:/etc/localtime:ro"])
        self.assertEqual(result[14:17], ["/cli/config/plugins/macsec.py", "/cli/show/plugins/show_macsec.py",
                                        "/cli/clear/plugins/clear_macsec_counter.py"])
        self.assertEqual(result[17], "docker-macsec.gz")

    def test_excluded_profiles_preserve_legacy_inputs_and_service_metadata(self):
        baseline = self.evaluate()
        for settings in ({"BLDENV": "bookworm"}, {"CONFIGURED_ARCH": "arm64"},
                         {"CONFIGURED_ARCH": "armhf"}, {"CONFIGURED_PLATFORM": "broadcom"},
                         {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"}, {"INCLUDE_FIPS": "n"},
                         {"DBG_IMAGE_MARK": "debug"}, {"ENABLE_ASAN": "y"},
                         {"CROSS_BUILD_ENVIRON": "y"}, {"MULTIARCH_QEMU_ENVIRON": "y"}):
            with self.subTest(settings=settings):
                result = self.evaluate(**settings)
                self.assertEqual(result[:2], ["", ""])
                self.assertEqual(result[3:6], ["", "", ""])
                self.assertEqual(result[6:], baseline[6:])
                if settings.get("BLDENV", "trixie") == "trixie":
                    self.assertIn("docker-macsec.gz", result[2])

    def test_existing_install_flags_select_local_image_variant(self):
        self.assertEqual(self.evaluate(INCLUDE_MACSEC="n")[17], "")
        self.assertEqual(self.evaluate(INSTALL_DEBUG_TOOLS="y")[17], "docker-macsec-dbg.gz")

    def test_selected_local_packages_reach_shared_docker_bridge(self):
        for debug, archive in (("n", "docker-macsec.gz"), ("y", "docker-macsec-dbg.gz")):
            with self.subTest(debug=debug):
                self.assertEqual(self.evaluate(include_bridge=True, INSTALL_DEBUG_TOOLS=debug)[17], archive)


if __name__ == "__main__":
    unittest.main()
