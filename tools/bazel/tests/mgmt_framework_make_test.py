"""Check the real Framework Make handoff and unsupported-profile fallback."""

from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class FrameworkMakeTest(unittest.TestCase):
    def evaluate(self, **overrides):
        settings = dict(BLDENV="trixie", CONFIGURED_PLATFORM="vs", CONFIGURED_ARCH="amd64",
                        INCLUDE_FIPS="y", BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", DBG_IMAGE_MARK="dbg",
                        DOCKERS_PATH="dockers", TARGET_PATH="target", INCLUDE_MGMT_FRAMEWORK="y",
                        SONIC_MGMT_COMMON="common.deb", SONIC_MGMT_FRAMEWORK="framework.deb",
                        SONIC_MGMT_FRAMEWORK_DBG="framework-dbgsym.deb")
        settings.update(overrides)
        with tempfile.TemporaryDirectory() as temporary:
            recipe = Path(temporary) / "Makefile"
            fields = {"runtime": "SONIC_BAZEL_DOCKER_IMAGES", "debug": "SONIC_BAZEL_DBG_DOCKER_IMAGES",
                      "legacy": "$(DOCKER_MGMT_FRAMEWORK)_DEPENDS",
                      "prerequisites": "$(DOCKER_MGMT_FRAMEWORK_DBG)_BAZEL_DEPENDS",
                      "host_files": "$(DOCKER_MGMT_FRAMEWORK)_BASE_IMAGE_FILES",
                      "install": "SONIC_INSTALL_DOCKER_IMAGES", "flags": "$(DOCKER_MGMT_FRAMEWORK)_RUN_OPT"}
            expressions = " ".join("'$(" + name + ")'" for name in fields.values())
            # Inspect Make's public variables without invoking any producer.
            recipe.write_text("include rules/docker-sonic-mgmt-framework.mk\nresult:\n\t@printf '%s\\n' " + expressions + "\n")
            output = subprocess.check_output(["make", "--no-print-directory", "-f", str(recipe), "result",
                        *[key + "=" + value for key, value in settings.items()]], cwd=ROOT, text=True)
            return dict(zip(fields, output.splitlines()))

    def test_native_tar_handoff_retains_legacy_inputs_and_host_wrapper(self):
        values = self.evaluate()
        self.assertEqual(values["runtime"], "docker-sonic-mgmt-framework.gz")
        self.assertEqual(values["debug"], "docker-sonic-mgmt-framework-dbg.gz")
        text = "\n".join(values.values())
        for expected in ("docker-sonic-mgmt-framework.gz", "docker-sonic-mgmt-framework-dbg.gz",
                         "common.deb framework.deb", "sonic-cli:/usr/bin/sonic-cli",
                         "target/bazel-manifests/docker-base-trixie/manifest.json",
                         "target/bazel-manifests/docker-config-engine-trixie/manifest.json",
                         "target/bazel-manifests/docker-sonic-mgmt-framework-dbg/manifest.json",
                         "/etc/sonic:/etc/sonic:ro", "/var/run/dbus:/var/run/dbus:rw"):
            self.assertIn(expected, text)

    def test_other_profiles_keep_make_producers(self):
        for settings in ({"CONFIGURED_ARCH": "arm64"}, {"CONFIGURED_PLATFORM": "broadcom"},
                         {"BLDENV": "bookworm"}, {"INCLUDE_FIPS": "n"}, {"ENABLE_ASAN": "y"},
                         {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"}, {"CROSS_BUILD_ENVIRON": "y"},
                         {"MULTIARCH_QEMU_ENVIRON": "y"}, {"DBG_IMAGE_MARK": "custom"}):
            with self.subTest(settings=settings):
                values = self.evaluate(**settings)
                self.assertEqual(values["runtime"], "")
                self.assertEqual(values["debug"], "")
                text = "\n".join(values.values())
                self.assertIn("common.deb framework.deb", text)
                self.assertIn("sonic-cli:/usr/bin/sonic-cli", text)


if __name__ == "__main__":
    unittest.main()
