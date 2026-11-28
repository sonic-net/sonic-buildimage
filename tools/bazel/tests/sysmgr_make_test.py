"""Exercise the real Sysmgr Make handoff without invoking package producers."""

from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[3]


class SysmgrMakeTest(unittest.TestCase):
    def evaluate(self, **changes):
        values = dict(
            BLDENV="trixie", CONFIGURED_PLATFORM="vs", CONFIGURED_ARCH="amd64",
            INCLUDE_FIPS="y", DBG_IMAGE_MARK="dbg", BUILD_WITH_BAZEL_WHEN_AVAILABLE="y",
            DOCKERS_PATH="dockers", TARGET_PATH="target", SYSMGR="sysmgr.deb",
            SYSMGR_DBG="sysmgr-dbg.deb", LIBSWSSCOMMON_DBG="common-dbg.deb",
            DOCKER_CONFIG_ENGINE_TRIXIE="docker-config-engine-trixie.gz",
        )
        values.update(changes)
        variables = [
            "SONIC_BAZEL_DOCKER_IMAGES", "SONIC_BAZEL_DBG_DOCKER_IMAGES",
            "SONIC_BAZEL_SWITCHABLE_IMAGES", "docker-sysmgr.gz_BAZEL_TARGET",
            "docker-sysmgr-$(DBG_IMAGE_MARK).gz_BAZEL_TARGET",
            "docker-sysmgr-$(DBG_IMAGE_MARK).gz_BAZEL_DEPENDS",
            "docker-sysmgr.gz_DEPENDS", "docker-sysmgr.gz_DBG_DEPENDS",
            "docker-sysmgr.gz_LOAD_DOCKERS", "docker-sysmgr.gz_CONTAINER_NAME",
            "docker-sysmgr.gz_RUN_OPT", "SONIC_INSTALL_DOCKER_IMAGES",
            "SONIC_INSTALL_DOCKER_DBG_IMAGES",
        ]
        recipe = "include rules/docker-sysmgr.mk\n.PHONY: result\nresult:\n\t@printf '%s\\n' "
        recipe += " ".join("'$(" + name + ")'" for name in variables) + "\n"
        output = subprocess.check_output(
            ["make", "--no-print-directory", "-f", "-", "result",
             *[key + "=" + value for key, value in values.items()]],
            input=recipe, cwd=ROOT, text=True,
        )
        return [line.strip() for line in output.splitlines()]

    def test_selected_targets_include_parent_and_variant_manifests(self):
        result = self.evaluate()
        self.assertEqual(result[:2], ["docker-sysmgr.gz", "docker-sysmgr-dbg.gz"])
        self.assertEqual(set(result[2].split()), set(result[:2]))
        self.assertEqual(result[3:5], [
            "//dockers/docker-sysmgr:docker-sysmgr.gz",
            "//dockers/docker-sysmgr:docker-sysmgr-dbg.gz",
        ])
        self.assertEqual(set(result[5].split()), {
            "target/bazel-manifests/" + name + "/manifest.json" for name in
            ("docker-base-trixie", "docker-config-engine-trixie", "docker-sysmgr", "docker-sysmgr-dbg")
        })

    def test_legacy_prerequisites_mounts_and_installer_selection_remain(self):
        result = self.evaluate()
        self.assertEqual(result[6:10], [
            "sysmgr.deb", "sysmgr-dbg.deb common-dbg.deb", "docker-config-engine-trixie.gz", "sysmgr",
        ])
        self.assertEqual(result[10].split(), [
            "-v", "/var/run/dbus:/var/run/dbus:rw", "-v", "/etc/sonic:/etc/sonic:ro",
        ])
        self.assertEqual(result[11:], ["docker-sysmgr.gz", "docker-sysmgr-dbg.gz"])

    def test_unsupported_profiles_keep_legacy_producers(self):
        baseline = self.evaluate()
        for changes in (
            {"BLDENV": "bookworm"}, {"CONFIGURED_ARCH": "arm64"}, {"CONFIGURED_ARCH": "armhf"},
            {"CONFIGURED_PLATFORM": "broadcom"}, {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"},
            {"INCLUDE_FIPS": "n"}, {"ENABLE_ASAN": "y"}, {"CROSS_BUILD_ENVIRON": "y"},
            {"MULTIARCH_QEMU_ENVIRON": "y"}, {"DBG_IMAGE_MARK": "debug"},
        ):
            with self.subTest(changes=changes):
                result = self.evaluate(**changes)
                self.assertEqual(result[:2], ["", ""])
                self.assertEqual(result[3:6], ["", "", ""])
                self.assertEqual(result[6:12], baseline[6:12])
                self.assertEqual(result[12], "docker-sysmgr-" + changes.get("DBG_IMAGE_MARK", "dbg") + ".gz")


if __name__ == "__main__":
    unittest.main()
