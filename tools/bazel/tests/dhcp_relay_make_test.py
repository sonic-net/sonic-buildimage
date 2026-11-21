"""Check DHCP's real Make selection, legacy prerequisites and version handoff."""

import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location(
    "image_version", ROOT / "tools/bazel/oci/prepare_image_version.py"
)
VERSION = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(VERSION)


class DhcpRelayMakeTest(unittest.TestCase):
    def evaluate(self, **overrides):
        settings = dict(
            BLDENV="trixie", CONFIGURED_PLATFORM="vs", CONFIGURED_ARCH="amd64",
            INCLUDE_FIPS="y", BUILD_WITH_BAZEL_WHEN_AVAILABLE="y", DBG_IMAGE_MARK="dbg",
            DOCKERS_PATH="dockers", TARGET_PATH="target", INCLUDE_DHCP_RELAY="y",
            INSTALL_DEBUG_TOOLS="n", DOCKER_CONFIG_ENGINE_TRIXIE="docker-config-engine-trixie.gz",
            ISC_DHCP_RELAY="isc-dhcp-relay.deb", SONIC_DHCPMON="sonic-dhcpmon.deb",
            SONIC_DHCP4RELAY="sonic-dhcp4relay.deb", SONIC_DHCPRELAY="sonic-dhcp6relay.deb",
            LIBSWSSCOMMON="libswsscommon.deb", PYTHON3_SWSSCOMMON="python3-swsscommon.deb",
            SONIC_UTILITIES_PY3="sonic_utilities.whl", SONIC_DHCP_UTILITIES_PY3="sonic_dhcp_utilities.whl",
        )
        settings.update(overrides)
        names = {
            "runtime": "SONIC_BAZEL_DOCKER_IMAGES",
            "debug": "SONIC_BAZEL_DBG_DOCKER_IMAGES",
            "runtime_target": "$(DOCKER_DHCP_RELAY)_BAZEL_TARGET",
            "debug_target": "$(DOCKER_DHCP_RELAY_DBG)_BAZEL_TARGET",
            "runtime_inputs": "$(DOCKER_DHCP_RELAY)_BAZEL_DEPENDS",
            "debug_inputs": "$(DOCKER_DHCP_RELAY_DBG)_BAZEL_DEPENDS",
            "selected": "SONIC_PACKAGES_LOCAL",
            "parent": "$(DOCKER_DHCP_RELAY)_LOAD_DOCKERS",
            "debs": "$(DOCKER_DHCP_RELAY)_DEPENDS",
            "wheels": "$(DOCKER_DHCP_RELAY)_PYTHON_WHEELS",
            "builder_wheels": "$(DOCKER_DHCP_RELAY)_INSTALL_PYTHON_WHEELS",
            "builder_debs": "$(DOCKER_DHCP_RELAY)_INSTALL_DEBS",
            "container": "$(DOCKER_DHCP_RELAY)_CONTAINER_NAME",
            "privileged": "$(DOCKER_DHCP_RELAY)_CONTAINER_PRIVILEGED",
            "volumes": "$(DOCKER_DHCP_RELAY)_CONTAINER_VOLUMES",
            "tmpfs": "$(DOCKER_DHCP_RELAY)_CONTAINER_TMPFS",
            "config": "$(DOCKER_DHCP_RELAY)_CLI_CONFIG_PLUGIN",
            "show": "$(DOCKER_DHCP_RELAY)_CLI_SHOW_PLUGIN",
            "clear": "$(DOCKER_DHCP_RELAY)_CLI_CLEAR_PLUGIN",
        }
        with tempfile.TemporaryDirectory() as temporary:
            recipe = Path(temporary) / "Makefile"
            recipe.write_text(
                "include rules/docker-dhcp-relay.mk\nresult:\n\t@printf '%s\\n' "
                + " ".join("'$(" + name + ")'" for name in names.values()) + "\n"
            )
            values = subprocess.check_output(
                ["make", "--no-print-directory", "-f", str(recipe), "result",
                 *[key + "=" + value for key, value in settings.items()]],
                cwd=ROOT, text=True,
            ).splitlines()
        self.assertEqual(len(values), len(names))
        return dict(zip(names, values))

    def test_native_routing_and_selected_variant(self):
        values = self.evaluate()
        self.assertEqual(values["runtime"], "docker-dhcp-relay.gz")
        self.assertEqual(values["debug"], "docker-dhcp-relay-dbg.gz")
        self.assertEqual(values["runtime_target"], "//dockers/docker-dhcp-relay:docker-dhcp-relay.gz")
        self.assertEqual(values["debug_target"], "//dockers/docker-dhcp-relay:docker-dhcp-relay-dbg.gz")
        self.assertIn("target/bazel-manifests/docker-dhcp-relay/image.env", values["runtime_inputs"])
        self.assertIn("target/bazel-manifests/docker-dhcp-relay/manifest.json", values["debug_inputs"])
        self.assertIn("target/bazel-manifests/docker-dhcp-relay-dbg/manifest.json", values["debug_inputs"])
        self.assertEqual(values["selected"], "docker-dhcp-relay.gz")
        self.assertEqual(self.evaluate(INSTALL_DEBUG_TOOLS="y")["selected"], "docker-dhcp-relay-dbg.gz")
        self.assertEqual(self.evaluate(INCLUDE_DHCP_RELAY="n")["selected"], "")

    def test_legacy_prerequisites_and_service_contract_are_preserved(self):
        values = self.evaluate()
        self.assertEqual(values["parent"], "docker-config-engine-trixie.gz")
        self.assertEqual(values["debs"].split(), [
            "isc-dhcp-relay.deb", "sonic-dhcpmon.deb", "sonic-dhcp4relay.deb",
            "sonic-dhcp6relay.deb", "libswsscommon.deb",
        ])
        self.assertEqual(values["wheels"], "sonic_dhcp_utilities.whl")
        self.assertEqual(values["builder_wheels"], "sonic_utilities.whl")
        self.assertEqual(values["builder_debs"], "python3-swsscommon.deb")
        self.assertEqual(values["container"], "dhcp_relay")
        self.assertEqual(values["privileged"], "false")
        self.assertIn("/etc/sonic:/etc/sonic:ro", values["volumes"])
        self.assertEqual(values["tmpfs"].split(), ["/tmp/", "/var/tmp/"])
        self.assertEqual(values["config"], "/cli/config/plugins/dhcp_relay.py")
        self.assertEqual(values["show"], "/cli/show/plugins/show_dhcp_relay.py")
        self.assertEqual(values["clear"], "/cli/clear/plugins/clear_dhcp_relay.py")

    def test_unsupported_profiles_keep_original_make_producers(self):
        baseline = self.evaluate()
        for settings in (
            {"CONFIGURED_ARCH": "arm64"}, {"CONFIGURED_PLATFORM": "broadcom"},
            {"BLDENV": "bookworm"}, {"INCLUDE_FIPS": "n"}, {"ENABLE_ASAN": "y"},
            {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"}, {"CROSS_BUILD_ENVIRON": "y"},
            {"MULTIARCH_QEMU_ENVIRON": "y"}, {"DBG_IMAGE_MARK": "custom"},
        ):
            with self.subTest(settings=settings):
                values = self.evaluate(**settings)
                for field in ["runtime", "debug", "runtime_target", "debug_target", "runtime_inputs", "debug_inputs"]:
                    self.assertEqual(values[field], "")
                for field in ["parent", "debs", "wheels", "builder_wheels", "builder_debs"]:
                    self.assertEqual(values[field], baseline[field])

    def test_version_noop_change_and_failed_preparation(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "image.env"
            VERSION.publish(output, "master.1-dhcp")
            timestamp = output.stat().st_mtime_ns
            VERSION.publish(output, "master.1-dhcp")
            self.assertEqual(output.stat().st_mtime_ns, timestamp)
            VERSION.publish(output, "master.2-dhcp")
            expected = "DEBIAN_FRONTEND=noninteractive\nIMAGE_VERSION=master.2-dhcp\n"
            self.assertEqual(output.read_text(), expected)
            for bad in ("", "master\nEXTRA=value", "master\rvalue", "master\0value", "build-$PATH", "build-${HOME}"):
                with self.subTest(value=repr(bad)), self.assertRaises(ValueError):
                    VERSION.publish(output, bad)
                self.assertEqual(output.read_text(), expected)


if __name__ == "__main__":
    unittest.main()
