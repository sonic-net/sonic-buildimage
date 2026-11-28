"""Check the real LLDP selection and environment handoff."""

import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location(
    "lldp_version", ROOT / "dockers/docker-lldp/bazel/prepare_version.py")
VERSION = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(VERSION)


class LldpMakeTest(unittest.TestCase):
    def evaluate(self, **overrides):
        settings = dict(BLDENV="trixie", CONFIGURED_PLATFORM="vs", CONFIGURED_ARCH="amd64",
                        INCLUDE_FIPS="y", BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", DBG_IMAGE_MARK="dbg",
                        DOCKERS_PATH="dockers", TARGET_PATH="target", INCLUDE_LLDP="y",
                        DOCKER_CONFIG_ENGINE_TRIXIE="docker-config-engine-trixie.gz",
                        LLDPD="lldpd.deb", LIBSWSSCOMMON="libswsscommon.deb",
                        PYTHON3_SWSSCOMMON="python3-swsscommon.deb", DBSYNCD_PY3="sonic-d.whl")
        settings.update(overrides)
        names = ["SONIC_BAZEL_DOCKER_IMAGES", "SONIC_BAZEL_DBG_DOCKER_IMAGES",
                 "$(DOCKER_LLDP)_BAZEL_DEPENDS", "$(DOCKER_LLDP_DBG)_BAZEL_DEPENDS",
                 "SONIC_INSTALL_DOCKER_IMAGES", "SONIC_INSTALL_DOCKER_DBG_IMAGES",
                 "$(DOCKER_LLDP)_RUN_OPT", "$(DOCKER_LLDP)_LOAD_DOCKERS",
                 "$(DOCKER_LLDP)_DEPENDS", "$(DOCKER_LLDP)_PYTHON_WHEELS"]
        with tempfile.TemporaryDirectory() as temporary:
            recipe = Path(temporary) / "Makefile"
            recipe.write_text("include rules/docker-lldp.mk\nresult:\n\t@printf '%s\\n' " +
                              " ".join("'$(" + name + ")'" for name in names) + "\n")
            return subprocess.check_output(
                ["make", "--no-print-directory", "-f", str(recipe), "result",
                 *[key + "=" + value for key, value in settings.items()]], cwd=ROOT, text=True).splitlines()

    def test_native_handoff_and_install_selection(self):
        values = self.evaluate()
        images = ["docker-lldp.gz", "docker-lldp-dbg.gz"]
        self.assertEqual(values[:2], images)
        self.assertIn("target/bazel-manifests/docker-lldp/image.env", values[2])
        self.assertIn("target/bazel-manifests/docker-lldp/manifest.json", values[3])
        self.assertIn("target/bazel-manifests/docker-lldp-dbg/manifest.json", values[3])
        self.assertEqual(values[4:6], images)
        self.assertIn("/etc/sonic:/etc/sonic:ro", values[6])
        self.assertEqual(values[7], "docker-config-engine-trixie.gz")
        self.assertIn("--cap-add=NET_ADMIN", values[6])
        self.assertEqual(values[8].split(), ["lldpd.deb", "libswsscommon.deb", "python3-swsscommon.deb"])
        self.assertEqual(values[9], "sonic-d.whl")
        disabled = self.evaluate(INCLUDE_LLDP="n")
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
                self.assertEqual(values[7], "docker-config-engine-trixie.gz")

    def test_version_change_noop_and_failed_preparation(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "image.env"
            VERSION.publish(output, "master.1-test")
            original = output.stat().st_mtime_ns
            VERSION.publish(output, "master.1-test")
            self.assertEqual(output.stat().st_mtime_ns, original)
            self.assertEqual(output.read_text(), "DEBIAN_FRONTEND=noninteractive\nIMAGE_VERSION=master.1-test\n")
            VERSION.publish(output, "master.2-test")
            expected = "DEBIAN_FRONTEND=noninteractive\nIMAGE_VERSION=master.2-test\n"
            self.assertEqual(output.read_text(), expected)
            for bad in ("", "master\nEXTRA=value", "master\rvalue", "master\0value", "build-$PATH", "build-${HOME}"):
                with self.subTest(value=repr(bad)), self.assertRaises(ValueError):
                    VERSION.publish(output, bad)
                self.assertEqual(output.read_text(), expected)


if __name__ == "__main__":
    unittest.main()
