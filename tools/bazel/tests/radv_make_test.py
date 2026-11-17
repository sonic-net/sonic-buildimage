"""Check the real Router Advertiser selection and environment handoff."""

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))

from tools.bazel.tests.docker_test import ArchiveRecipeFixture

class RadvMakeTest(unittest.TestCase):
    def evaluate(self, **overrides):
        settings = dict(BLDENV="trixie", CONFIGURED_PLATFORM="vs", CONFIGURED_ARCH="amd64",
                        INCLUDE_FIPS="y", BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", DBG_IMAGE_MARK="dbg",
                        DOCKERS_PATH="dockers", TARGET_PATH="target", INCLUDE_ROUTER_ADVERTISER="y",
                        DOCKER_CONFIG_ENGINE_TRIXIE="docker-config-engine-trixie.gz")
        settings.update(overrides)
        names = ["SONIC_BAZEL_DOCKER_IMAGES", "SONIC_BAZEL_DBG_DOCKER_IMAGES",
                 "$(DOCKER_ROUTER_ADVERTISER)_BAZEL_DEPENDS", "$(DOCKER_ROUTER_ADVERTISER_DBG)_BAZEL_DEPENDS",
                 "SONIC_INSTALL_DOCKER_IMAGES", "SONIC_INSTALL_DOCKER_DBG_IMAGES",
                 "$(DOCKER_ROUTER_ADVERTISER)_RUN_OPT", "$(DOCKER_ROUTER_ADVERTISER)_LOAD_DOCKERS",
                 "SONIC_BAZEL_IMAGE_ENVIRONMENTS", "$(DOCKER_ROUTER_ADVERTISER)_BAZEL_ARGS",
                 "$(DOCKER_ROUTER_ADVERTISER_DBG)_BAZEL_ARGS"]
        with tempfile.TemporaryDirectory() as temporary:
            recipe = Path(temporary) / "Makefile"
            recipe.write_text("include rules/docker-router-advertiser.mk\nresult:\n\t@printf '%s\\n' " +
                              " ".join("'$(" + name + ")'" for name in names) + "\n")
            return subprocess.check_output(
                ["make", "--no-print-directory", "-f", str(recipe), "result",
                 *[key + "=" + value for key, value in settings.items()]], cwd=ROOT, text=True).splitlines()

    def test_native_handoff_and_install_selection(self):
        values = self.evaluate(SONIC_IMAGE_VERSION="radv-parity-2")
        images = ["docker-router-advertiser.gz", "docker-router-advertiser-dbg.gz"]
        self.assertEqual(values[:2], images)
        self.assertIn("target/bazel-manifests/docker-router-advertiser/image.env", values[2])
        self.assertIn("target/bazel-manifests/docker-router-advertiser/manifest.json", values[3])
        self.assertIn("target/bazel-manifests/docker-router-advertiser-dbg/manifest.json", values[3])
        self.assertEqual(values[4:6], images)
        self.assertIn("/etc/sonic:/etc/sonic:ro", values[6])
        self.assertEqual(values[7], "docker-config-engine-trixie.gz")
        self.assertEqual(values[8], "docker-router-advertiser")
        self.assertEqual(values[9:11], ["--//dockers/docker-router-advertiser:image_version=radv-parity-2"] * 2)
        disabled = self.evaluate(INCLUDE_ROUTER_ADVERTISER="n")
        self.assertEqual(disabled[:2], images)
        self.assertEqual(disabled[4:6], ["", ""])

    def test_other_profiles_keep_make_producers(self):
        for settings in ({"CONFIGURED_ARCH": "arm64"}, {"CONFIGURED_PLATFORM": "broadcom"},
                         {"BLDENV": "bookworm"}, {"INCLUDE_FIPS": "n"}, {"ENABLE_ASAN": "y"},
                         {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"}, {"CROSS_BUILD_ENVIRON": "y"},
                         {"MULTIARCH_QEMU_ENVIRON": "y"}, {"DBG_IMAGE_MARK": "custom"}):
            with self.subTest(settings=settings):
                values = self.evaluate(**settings)
                self.assertEqual(values[:4], ["", "", "", ""])
                self.assertEqual(values[8], "")
                self.assertEqual(values[7], "docker-config-engine-trixie.gz")

class RadvArchiveSwitchTest(ArchiveRecipeFixture, unittest.TestCase):
    archives = ("docker-router-advertiser.gz", "docker-router-advertiser-dbg.gz")
    labels = {name: "//dockers/docker-router-advertiser:" + name for name in archives}

    def test_parallel_existing_archives_switch_producers_and_restore(self):
        (self.root / "rules").symlink_to(ROOT / "rules", target_is_directory=True)
        self.prerequisites = {name: [] for name in self.archives}
        self.makefile += """
BLDENV = trixie
CONFIGURED_PLATFORM = vs
INCLUDE_FIPS = y
DBG_IMAGE_MARK = dbg
DOCKERS_PATH = dockers
# Keep the recipe and dependency paths; supply prepared Make input files below.
include rules/docker-router-advertiser.mk
SONIC_BAZEL_MANIFESTS :=
SONIC_BAZEL_IMAGE_ENVIRONMENTS :=
include tools/bazel/docker.mk
target/bazel-manifests/%:
	@mkdir -p "$(@D)" && touch "$@"
legacy := $(filter-out $(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES),$(SONIC_BAZEL_SWITCHABLE_IMAGES))
$(addprefix target/,$(legacy)): target/%.gz:
	@printf 'make %s' '$(@F)' > "$@"
"""
        for mode in ("n", "y", "n", "y"):
            with self.subTest(mode=mode):
                result = self.run_make(*self.archives, parallel=True,
                                       environment={"BUILD_WITH_BAZEL_WHEN_AVAILABLE": mode})
                self.assert_success(result)
                for name in self.archives:
                    archive = self.target / name
                    expected = ("new " if mode == "y" else "make ") + name
                    self.assertEqual(archive.read_text(), expected)
                    stamp = self.target / ".container-build-method" / name
                    self.assertEqual(stamp.read_text().strip(), "bazel" if mode == "y" else "make")


if __name__ == "__main__":
    unittest.main()
