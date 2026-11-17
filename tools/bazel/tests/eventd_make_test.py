"""Check Eventd's real Make selector and declared image-version input."""

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import eventd_image

class EventdMakeTest(unittest.TestCase):
    def evaluate(self, **overrides):
        settings = dict(BLDENV="trixie", CONFIGURED_PLATFORM="vs", CONFIGURED_ARCH="amd64",
                        INCLUDE_FIPS="y", BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", DBG_IMAGE_MARK="dbg",
                        SONIC_BUILD_TARGET="target/sonic-vs.bin",
                        DOCKERS_PATH="dockers", TARGET_PATH="target", INCLUDE_SYSTEM_EVENTD="y",
                        SONIC_EVENTD="eventd.deb", SONIC_EVENTD_DBG="eventd-dbgsym.deb",
                        SONIC_UTILITIES_PY3="utilities.whl", PYTHON3_SWSSCOMMON="bindings.deb")
        settings.update(overrides)
        names = ["SONIC_BAZEL_DOCKER_IMAGES", "SONIC_BAZEL_DBG_DOCKER_IMAGES",
                 "$(DOCKER_EVENTD)_BAZEL_DEPENDS", "$(DOCKER_EVENTD_DBG)_BAZEL_DEPENDS",
                 "$(DOCKER_EVENTD)_DEPENDS", "$(DOCKER_EVENTD)_INSTALL_PYTHON_WHEELS",
                 "$(DOCKER_EVENTD)_INSTALL_DEBS", "$(DOCKER_EVENTD)_RUN_OPT",
                 "SONIC_BAZEL_IMAGE_ENVIRONMENTS", "$(DOCKER_EVENTD)_PATH",
                 "$(DOCKER_EVENTD_DBG)_PATH", "SONIC_BAZEL_VS_PAIR_OUTPUTS"]
        with tempfile.TemporaryDirectory() as temporary:
            recipe = Path(temporary) / "Makefile"
            recipe.write_text("include rules/docker-eventd.mk\ninclude tools/bazel/docker.mk\nresult:\n\t@printf '%s\\n' " +
                              " ".join("'$(" + name + ")'" for name in names) + "\n")
            return subprocess.check_output(
                ["make", "--no-print-directory", "-f", str(recipe), "result",
                 *[key + "=" + value for key, value in settings.items()]], cwd=ROOT, text=True).splitlines()

    def test_native_handoff_retains_legacy_and_builder_inputs(self):
        values = self.evaluate()
        self.assertEqual(values[:2], ["docker-eventd.gz", "docker-eventd-dbg.gz"])
        self.assertIn("target/bazel-manifests/docker-eventd/image.env", values[2])
        self.assertIn("target/bazel-manifests/docker-eventd/manifest.json", values[3])
        self.assertIn("target/bazel-manifests/docker-eventd-dbg/manifest.json", values[3])
        self.assertEqual(values[4:7], ["eventd.deb", "utilities.whl", "bindings.deb"])
        self.assertIn("/etc/sonic:/etc/sonic:ro", values[7])
        self.assertEqual(values[8], "docker-eventd")
        self.assertEqual(values[9:11], ["dockers/docker-eventd", "dockers/docker-eventd"])
        self.assertEqual(values[11].split(), ["docker-eventd.gz", "docker-eventd-dbg.gz"])

    def test_other_profiles_keep_make_producers(self):
        for settings in ({"CONFIGURED_ARCH": "arm64"}, {"CONFIGURED_PLATFORM": "broadcom"},
                         {"BLDENV": "bookworm"}, {"INCLUDE_FIPS": "n"}, {"ENABLE_ASAN": "y"},
                         {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"}, {"CROSS_BUILD_ENVIRON": "y"},
                         {"MULTIARCH_QEMU_ENVIRON": "y"}, {"DBG_IMAGE_MARK": "custom"}):
            with self.subTest(settings=settings):
                values = self.evaluate(**settings)
                self.assertEqual(values[:4], ["", "", "", ""])
                self.assertEqual(values[8:], ["", "dockers/docker-eventd", "", ""])
                self.assertEqual(values[4:7], ["eventd.deb", "utilities.whl", "bindings.deb"])

    def test_incremental_source_guard_rejects_uncommitted_probe_input(self):
        source = ROOT / "dockers/docker-eventd/host_events_info.json"
        with mock.patch.object(eventd_image.subprocess, "check_output",
                               return_value=" M dockers/docker-eventd/host_events_info.json\n"):
            with self.assertRaisesRegex(ValueError, "uncommitted changes in incremental probe input"):
                eventd_image.require_clean_incremental_source(source, "deadbeef")


if __name__ == "__main__":
    unittest.main()
