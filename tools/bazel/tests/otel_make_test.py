"""Exercise the real OTEL Make selector and preserve its host contracts."""

from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[3]


class OtelMakeTest(unittest.TestCase):
    def evaluate(self, **changes):
        values = dict(BLDENV="trixie", CONFIGURED_ARCH="amd64", CONFIGURED_PLATFORM="vs",
                      INCLUDE_FIPS="y", INCLUDE_SYSTEM_OTEL="y", DBG_IMAGE_MARK="dbg",
                      BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", DOCKERS_PATH="dockers", TARGET_PATH="target",
                      DOCKER_CONFIG_ENGINE_TRIXIE="docker-config-engine-trixie.gz")
        values.update(changes)
        names = ["SONIC_BAZEL_DOCKER_IMAGES", "SONIC_BAZEL_DBG_DOCKER_IMAGES",
                 "SONIC_BAZEL_SWITCHABLE_IMAGES", "docker-sonic-otel.gz_BAZEL_TARGET",
                 "docker-sonic-otel-$(DBG_IMAGE_MARK).gz_BAZEL_TARGET",
                 "docker-sonic-otel-$(DBG_IMAGE_MARK).gz_BAZEL_DEPENDS",
                 "docker-sonic-otel.gz_DEPENDS", "docker-sonic-otel.gz_LOAD_DOCKERS",
                 "docker-sonic-otel.gz_RUN_OPT", "docker-sonic-otel.gz_BASE_IMAGE_FILES",
                 "SONIC_INSTALL_DOCKER_IMAGES", "SONIC_INSTALL_DOCKER_DBG_IMAGES"]
        recipe = "include rules/docker-otel.mk\n.PHONY: result\nresult:\n\t@printf '%s\\n' "
        recipe += " ".join("'$(" + name + ")'" for name in names) + "\n"
        return subprocess.check_output(["make", "--no-print-directory", "-f", "-", "result",
                                        *[key + "=" + value for key, value in values.items()]],
                                       input=recipe, cwd=ROOT, text=True).splitlines()

    def test_manifest_handoff_preserves_host_contract(self):
        result = self.evaluate()
        self.assertEqual(result[:2], ["docker-sonic-otel.gz", "docker-sonic-otel-dbg.gz"])
        self.assertEqual(set(result[2].split()), set(result[:2]))
        self.assertEqual(result[3:5], ["//dockers/docker-sonic-otel:docker-sonic-otel.gz",
                                     "//dockers/docker-sonic-otel:docker-sonic-otel-dbg.gz"])
        self.assertEqual(set(result[5].split()), {
            "target/bazel-manifests/" + name + "/manifest.json" for name in
            ("docker-base-trixie", "docker-config-engine-trixie", "docker-sonic-otel", "docker-sonic-otel-dbg")} |
            {"target/bazel-manifests/docker-sonic-otel/image.env"})
        self.assertEqual(result[6:8], ["", "docker-config-engine-trixie.gz"])
        self.assertEqual(result[8].split(), ["-t", "-v", "/etc/sonic:/etc/sonic:ro",
            "-v", "/etc/localtime:/etc/localtime:ro", "-v", "/:/mnt/host:ro", "-v", "/tmp:/mnt/host/tmp:rw",
            "-v", "/var/tmp:/mnt/host/var/tmp:rw", "--pid=host", "--userns=host"])
        self.assertEqual(result[9:], ["monit_otel:/etc/monit/conf.d", "docker-sonic-otel.gz", "docker-sonic-otel-dbg.gz"])

    def test_excluded_profiles_retain_legacy_inputs(self):
        baseline = self.evaluate()
        for settings in ({"BLDENV": "bookworm"}, {"CONFIGURED_ARCH": "arm64"},
                         {"CONFIGURED_ARCH": "armhf"}, {"CONFIGURED_PLATFORM": "broadcom"},
                         {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"}, {"INCLUDE_FIPS": "n"},
                         {"ENABLE_ASAN": "y"}, {"CROSS_BUILD_ENVIRON": "y"}, {"MULTIARCH_QEMU_ENVIRON": "y"}):
            with self.subTest(settings=settings):
                result = self.evaluate(**settings)
                self.assertEqual(result[:2], ["", ""])
                self.assertEqual(result[3:6], ["", "", ""])
                self.assertEqual(result[6:], baseline[6:])
                if settings.get("BLDENV", "trixie") == "trixie":
                    self.assertIn("docker-sonic-otel.gz", result[2])

    def test_install_selection_and_debug_name_remain_explicit(self):
        self.assertEqual(self.evaluate(INCLUDE_SYSTEM_OTEL="n")[10:], ["", ""])
        renamed = self.evaluate(DBG_IMAGE_MARK="debug")
        self.assertEqual(renamed[:2], ["", ""])
        self.assertEqual(renamed[10:], ["docker-sonic-otel.gz", "docker-sonic-otel-debug.gz"])


if __name__ == "__main__":
    unittest.main()
