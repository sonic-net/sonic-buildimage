"""Check Database's real Make selector and declared image-version input."""

import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import database_image

SPEC = importlib.util.spec_from_file_location(
    "database_version", ROOT / "dockers/docker-database/bazel/prepare_environment.py")
VERSION = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(VERSION)


class DatabaseMakeTest(unittest.TestCase):
    def evaluate(self, **overrides):
        settings = dict(BLDENV="trixie", CONFIGURED_PLATFORM="vs", CONFIGURED_ARCH="amd64",
                        INCLUDE_FIPS="y", BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", DBG_IMAGE_MARK="dbg",
                        DOCKERS_PATH="dockers", TARGET_PATH="target", INCLUDE_SYSTEM_EVENTD="y",
                        LIBSWSSCOMMON="common.deb", SONIC_DB_CLI="cli.deb", LIB_SONIC_DASH_API="dash-api.deb",
                        SYSCTL_NET_CONFIG="90-sonic.conf", UPDATE_CHASSISDB_CONFIG_SCRIPT="update_chassisdb_config")
        settings.update(overrides)
        names = ["SONIC_BAZEL_DOCKER_IMAGES", "SONIC_BAZEL_DBG_DOCKER_IMAGES",
                 "$(DOCKER_DATABASE)_BAZEL_DEPENDS", "$(DOCKER_DATABASE_DBG)_BAZEL_DEPENDS",
                 "$(DOCKER_DATABASE)_DEPENDS", "$(DOCKER_DATABASE)_BASE_IMAGE_FILES",
                 "$(DOCKER_DATABASE)_FILES", "$(DOCKER_DATABASE)_RUN_OPT",
                 "$(DOCKER_DATABASE)_BAZEL_ARGS", "$(DOCKER_DATABASE_DBG)_BAZEL_ARGS"]
        with tempfile.TemporaryDirectory() as temporary:
            recipe = Path(temporary) / "Makefile"
            recipe.write_text("include rules/docker-database.mk\nresult:\n\t@printf '%s\\n' " +
                              " ".join("'$(" + name + ")'" for name in names) + "\n")
            return subprocess.check_output(
                ["make", "--no-print-directory", "-f", str(recipe), "result",
                 *[key + "=" + value for key, value in settings.items()]], cwd=ROOT, text=True).splitlines()

    def test_make_passes_the_selected_version_to_both_archives(self):
        values = self.evaluate(SONIC_IMAGE_VERSION="202705.1")
        expected = "--//dockers/docker-database:image_version=202705.1"
        self.assertEqual(values[-2:], [expected, expected])

    def test_native_handoff_retains_legacy_and_host_inputs(self):
        values = self.evaluate()
        self.assertEqual(values[:2], ["docker-database.gz", "docker-database-dbg.gz"])
        self.assertIn("target/bazel-manifests/docker-database/image.env", values[2])
        self.assertIn("target/bazel-manifests/docker-database/manifest.json", values[3])
        self.assertIn("target/bazel-manifests/docker-database-dbg/manifest.json", values[3])
        self.assertEqual(values[4:7], ["common.deb cli.deb dash-api.deb", "redis-cli:/usr/bin/redis-cli", "90-sonic.conf update_chassisdb_config"])
        self.assertIn("/etc/sonic:/etc/sonic:ro", values[7])

    def test_component_labels_for_installer_dependencies(self):
        with tempfile.TemporaryDirectory() as temporary:
            target = Path(temporary)
            parent, runtime = [target / "bazel-manifests" / name / "components.labels"
                               for name in database_image.COMPONENT_LABEL_IMAGES]
            command = [
                "make", "--no-print-directory", "-f", "tools/bazel/prepare_manifests.mk", str(parent), str(runtime),
                "TARGET_PATH=" + str(target), "CONFIGURED_ARCH=amd64", "CONFIGURED_PLATFORM=vs",
                "BUILD_WITH_BAZEL_WHEN_AVAILABLE=y", "INCLUDE_FIPS=y", "BLDENV=trixie", "DBG_IMAGE_MARK=dbg",
                "SONIC_BAZEL_COMPONENT_LABELS=" + " ".join(database_image.COMPONENT_LABEL_IMAGES),
                "MANIFEST_METADATA=" + database_image.MANIFEST_METADATA,
            ]
            common = "com.azure.sonic.versions.libswsscommon="
            supervisor = "com.azure.sonic.versions.sonic-supervisord-utilities-rs="
            subprocess.run(command, cwd=ROOT, check=True)
            self.assertEqual(set(parent.read_text().splitlines()), {common + "1.0.0", supervisor + "1.0.0"})
            self.assertEqual(set(runtime.read_text().splitlines()), {common + "1.0.0"})
            stamps = [file.stat().st_mtime_ns for file in (parent, runtime)]
            subprocess.run(command, cwd=ROOT, check=True)
            self.assertEqual([file.stat().st_mtime_ns for file in (parent, runtime)], stamps)
            subprocess.run(command + ["LIBSWSSCOMMON_VERSION=1.2.3", "SONIC_SUPERVISORD_UTILITIES_RS_VERSION=2.0.0"],
                           cwd=ROOT, check=True)
            self.assertEqual(set(parent.read_text().splitlines()), {common + "1.2.3", supervisor + "2.0.0"})
            self.assertEqual(set(runtime.read_text().splitlines()), {common + "1.2.3"})

    def test_other_profiles_keep_make_producers(self):
        for settings in ({"CONFIGURED_ARCH": "arm64"}, {"CONFIGURED_PLATFORM": "broadcom"},
                         {"BLDENV": "bookworm"}, {"INCLUDE_FIPS": "n"}, {"ENABLE_ASAN": "y"},
                         {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"}, {"CROSS_BUILD_ENVIRON": "y"},
                         {"MULTIARCH_QEMU_ENVIRON": "y"}, {"DBG_IMAGE_MARK": "custom"}):
            with self.subTest(settings=settings):
                values = self.evaluate(**settings)
                self.assertEqual(values[:4], ["", "", "", ""])
                self.assertEqual(values[4:7], ["common.deb cli.deb dash-api.deb", "redis-cli:/usr/bin/redis-cli", "90-sonic.conf update_chassisdb_config"])

    def test_environment_change_noop_and_failed_preparation(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "image.env"
            VERSION.publish(output, "y")
            original = output.stat().st_mtime_ns
            VERSION.publish(output, "y")
            self.assertEqual(output.stat().st_mtime_ns, original)
            self.assertEqual(output.read_text(), "DEBIAN_FRONTEND=noninteractive\nINCLUDE_SYSTEM_EVENTD=y\n")
            VERSION.publish(output, "n")
            self.assertEqual(output.read_text(), "DEBIAN_FRONTEND=noninteractive\nINCLUDE_SYSTEM_EVENTD=n\n")
            for bad in ("", "master\nEXTRA=value", "master\rvalue", "master\0value", "build-$PATH", "build-${HOME}"):
                with self.subTest(value=repr(bad)), self.assertRaises(ValueError):
                    VERSION.publish(output, bad)
                self.assertEqual(output.read_text(), "DEBIAN_FRONTEND=noninteractive\nINCLUDE_SYSTEM_EVENTD=n\n")


if __name__ == "__main__":
    unittest.main()
