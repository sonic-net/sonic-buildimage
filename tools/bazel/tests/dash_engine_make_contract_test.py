"""Exercise native DASH engine selection through its actual Make recipe."""

from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class DashEngineMakeContractTest(unittest.TestCase):
    def evaluate(self, **changes):
        values = dict(BLDENV="trixie", CONFIGURED_PLATFORM="vs", CONFIGURED_ARCH="amd64",
                      BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", INCLUDE_FIPS="y", INCLUDE_VS_DASH_SAI="y",
                      PLATFORM_PATH="platform/vs", TARGET_PATH="target")
        values.update(changes)
        names = ["SONIC_DOCKER_IMAGES", "SONIC_INSTALL_DOCKER_IMAGES", "SONIC_BAZEL_DOCKER_IMAGES",
                 "SONIC_BAZEL_MANIFESTS", "SONIC_DOCKER_DBG_IMAGES", "SONIC_INSTALL_DOCKER_DBG_IMAGES",
                 "docker-dash-engine.gz_VERSION", "docker-dash-engine.gz_PACKAGE_NAME",
                 "docker-dash-engine.gz_CONTAINER_NAME", "docker-dash-engine.gz_CONTAINER_PRIVILEGED",
                 "docker-dash-engine.gz_RUN_OPT", "docker-dash-engine.gz_BAZEL_TARGET",
                 "docker-dash-engine.gz_BAZEL_DEPENDS"]
        text = "\n".join(f"{k} := {v}" for k, v in values.items()) + "\n"
        text += f"include {ROOT}/platform/vs/docker-dash-engine.mk\n"
        text += "\n".join(f"$(info {name}=$({name}))" for name in names)
        text += "\n.PHONY: check\ncheck:\n\t@:\n"
        with tempfile.TemporaryDirectory() as directory:
            makefile = Path(directory) / "Makefile"
            makefile.write_text(text)
            output = subprocess.check_output(["make", "--no-print-directory", "-f", str(makefile), "check"], text=True)
        return dict(line.split("=", 1) for line in output.splitlines() if "=" in line)

    def assert_legacy_metadata(self, values):
        self.assertEqual(values["SONIC_DOCKER_IMAGES"], "docker-dash-engine.gz")
        self.assertEqual(values["SONIC_INSTALL_DOCKER_IMAGES"], "docker-dash-engine.gz")
        for key, expected in (("VERSION", "1.0.0"), ("PACKAGE_NAME", "dash-engine"),
                              ("CONTAINER_NAME", "dash_engine"), ("CONTAINER_PRIVILEGED", "true"),
                              ("RUN_OPT", "--privileged -t")):
            self.assertEqual(values["docker-dash-engine.gz_" + key], expected)
        self.assertEqual(values["SONIC_DOCKER_DBG_IMAGES"], "")
        self.assertEqual(values["SONIC_INSTALL_DOCKER_DBG_IMAGES"], "")

    def test_native_selection_preserves_only_original_installer_family(self):
        values = self.evaluate()
        self.assert_legacy_metadata(values)
        self.assertEqual(values["SONIC_BAZEL_DOCKER_IMAGES"], "docker-dash-engine.gz")
        self.assertEqual(values["docker-dash-engine.gz_BAZEL_TARGET"], "//platform/vs/docker-dash-engine:docker-dash-engine.gz")
        self.assertEqual(values["SONIC_BAZEL_MANIFESTS"], "docker-dash-engine")
        self.assertEqual(values["docker-dash-engine.gz_BAZEL_DEPENDS"].split(), [
            "target/bazel-manifests/docker-base-trixie/manifest.json",
            "target/bazel-manifests/docker-dash-engine/manifest.json"])

    def test_other_profiles_keep_original_docker_path_and_metadata(self):
        for changes in ({"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"}, {"BLDENV": "bookworm"},
                        {"CONFIGURED_ARCH": "arm64"}, {"CONFIGURED_PLATFORM": "broadcom"},
                        {"INCLUDE_FIPS": "n"}, {"INCLUDE_VS_DASH_SAI": "n"},
                        {"CROSS_BUILD_ENVIRON": "y"}, {"MULTIARCH_QEMU_ENVIRON": "y"},
                        {"ENABLE_ASAN": "y"}, {"ENABLE_SYNCD_RPC": "y"}):
            with self.subTest(changes=changes):
                values = self.evaluate(**changes)
                self.assert_legacy_metadata(values)
                self.assertEqual(values["SONIC_BAZEL_DOCKER_IMAGES"], "")
                self.assertEqual(values["docker-dash-engine.gz_BAZEL_TARGET"], "")
                self.assertEqual(values["docker-dash-engine.gz_BAZEL_DEPENDS"], "")


if __name__ == "__main__":
    unittest.main()
