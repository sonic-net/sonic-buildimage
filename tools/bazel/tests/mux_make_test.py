"""Exercise MUX's real Make selection, prerequisites and runtime contract."""

from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class MuxMakeTest(unittest.TestCase):
    def evaluate(self, **overrides):
        settings = dict(BLDENV="trixie", CONFIGURED_PLATFORM="vs", CONFIGURED_ARCH="amd64",
                        INCLUDE_FIPS="y", BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", DBG_IMAGE_MARK="dbg",
                        DOCKERS_PATH="dockers", TARGET_PATH="target", INCLUDE_MUX="y",
                        SONIC_LINKMGRD="linkmgrd.deb", LIBSWSSCOMMON="common.deb",
                        SONIC_LINKMGRD_DBG="linkmgrd-dbgsym.deb", LIBSWSSCOMMON_DBG="common-dbgsym.deb",
                        DOCKER_CONFIG_ENGINE_TRIXIE="docker-config-engine-trixie.gz")
        settings.update(overrides)
        names = ["SONIC_BAZEL_DOCKER_IMAGES", "SONIC_BAZEL_DBG_DOCKER_IMAGES",
                 "$(DOCKER_MUX)_BAZEL_DEPENDS", "$(DOCKER_MUX_DBG)_BAZEL_DEPENDS",
                 "SONIC_INSTALL_DOCKER_IMAGES", "SONIC_INSTALL_DOCKER_DBG_IMAGES",
                 "$(DOCKER_MUX)_RUN_OPT", "$(DOCKER_MUX)_LOAD_DOCKERS",
                 "$(DOCKER_MUX)_DEPENDS", "$(DOCKER_MUX)_DBG_DEPENDS",
                 "$(DOCKER_MUX)_WARM_SHUTDOWN_BEFORE", "$(DOCKER_MUX)_FAST_SHUTDOWN_BEFORE",
                 "SONIC_BAZEL_MANIFESTS", "$(DOCKER_MUX)_BAZEL_TARGET", "$(DOCKER_MUX_DBG)_BAZEL_TARGET",
                 "$(DOCKER_MUX)_PATH", "$(DOCKER_MUX_DBG)_PATH"]
        with tempfile.TemporaryDirectory() as temporary:
            recipe = Path(temporary) / "Makefile"
            recipe.write_text("include rules/functions rules/docker-mux.mk rules/docker-mux.dep tools/bazel/docker.mk\n"
                              "result:\n\t@printf '%s\\n' " +
                              " ".join("'$(" + name + ")'" for name in names) + "\n")
            return subprocess.check_output(
                ["make", "--no-print-directory", "-f", str(recipe), "result",
                 *[key + "=" + value for key, value in settings.items()]], cwd=ROOT, text=True).splitlines()

    def test_native_profile_keeps_contract_and_declares_manifests(self):
        values = self.evaluate()
        images = ["docker-mux.gz", "docker-mux-dbg.gz"]
        self.assertEqual(values[:2], images)
        expected = ["target/bazel-manifests/" + name + "/manifest.json" for name in
                    ("docker-base-trixie", "docker-config-engine-trixie", "docker-mux")]
        self.assertEqual(values[2].split(), expected)
        self.assertEqual(values[3].split(), expected + ["target/bazel-manifests/docker-mux-dbg/manifest.json"])
        self.assertEqual(values[4:6], images)
        self.assertEqual(values[6].split(), ["-t", "-v", "/etc/sonic:/etc/sonic:ro", "-v", "/etc/localtime:/etc/localtime:ro"])
        self.assertEqual(values[7], "docker-config-engine-trixie.gz")
        self.assertEqual(values[8].split(), ["linkmgrd.deb", "common.deb"])
        self.assertEqual(values[9].split(), ["linkmgrd-dbgsym.deb", "common-dbgsym.deb"])
        self.assertEqual(values[10:12], ["swss", "swss"])
        self.assertEqual(values[12].split(), ["docker-mux", "docker-mux-dbg"])
        self.assertEqual(values[13:15], ["//dockers/docker-mux:" + name for name in images])
        self.assertEqual(values[15:], ["dockers/docker-mux", "dockers/docker-mux"])

    def test_excluded_mux_produces_no_selected_archives(self):
        values = self.evaluate(INCLUDE_MUX="n")
        self.assertEqual(values[:6], [""] * 6)
        self.assertEqual(values[12:15], [""] * 3)

    def test_other_profiles_keep_make_producers(self):
        for settings in ({"CONFIGURED_ARCH": "arm64"}, {"CONFIGURED_PLATFORM": "broadcom"},
                         {"BLDENV": "bookworm"}, {"INCLUDE_FIPS": "n"}, {"ENABLE_ASAN": "y"},
                         {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"}, {"CROSS_BUILD_ENVIRON": "y"},
                         {"MULTIARCH_QEMU_ENVIRON": "y"}, {"DBG_IMAGE_MARK": "custom"}):
            with self.subTest(settings=settings):
                values = self.evaluate(**settings)
                self.assertEqual(values[:4], [""] * 4)
                self.assertEqual(values[4], "docker-mux.gz")
                self.assertEqual(values[7], "docker-config-engine-trixie.gz")
                self.assertEqual(values[8].split(), ["linkmgrd.deb", "common.deb"])
                self.assertEqual(values[12:15], [""] * 3)


if __name__ == "__main__":
    unittest.main()
