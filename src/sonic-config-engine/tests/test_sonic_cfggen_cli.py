"""Exercise the real cfggen command and its native imports without a switch."""

import importlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


CFGGEN = os.environ.get("SONIC_CFGGEN")


def cfggen_command():
    if CFGGEN:
        return [str(Path(CFGGEN).resolve())]
    return [sys.executable, str(Path(__file__).resolve().parents[1] / "sonic-cfggen")]


class CfggenCliTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)

    def write(self, name, contents):
        path = self.root / name
        path.write_text(contents)
        return str(path)

    def cfggen(self, *args, check=True, namespace=""):
        env = os.environ.copy()
        # This is the documented platform environment override, not a mocked
        # import. Build-time rendering does not consume live switch metadata.
        env.update(PLATFORM="sonic-bazel-build", NAMESPACE_ID=namespace)
        env.pop("CFGGEN_UNIT_TESTING", None)
        result = subprocess.run(
            cfggen_command() + list(args),
            env=env,
            text=True,
            capture_output=True,
            timeout=30,
        )
        if check:
            self.assertEqual(result.returncode, 0, result.stderr)
        return result

    def test_help_imports_the_real_common_and_yang_bindings(self):
        result = self.cfggen("--help")
        self.assertIn("--from-db", result.stdout)
        self.assertIn("--yang", result.stdout)

    def test_installed_library_modules_import(self):
        # openconfig_acl is a public setup.py module even though this CLI does
        # not import it. Exercise its bitarray/pyangbind dependency closure too.
        for name in (
            "asic_sensors_config", "config_samples", "minigraph", "minigraph_ext",
            "openconfig_acl", "portconfig", "smartswitch_config", "sonic_yang_cfg_generator",
        ):
            with self.subTest(module=name):
                importlib.import_module(name)

    def test_json_yaml_and_additional_data_merge(self):
        initial = self.write(
            "initial.json",
            json.dumps({"DEVICE_METADATA": {"localhost": {"hostname": "leaf", "type": "ToR"}}}),
        )
        extra = self.write(
            "extra.yml", "DEVICE_METADATA:\n  localhost:\n    type: LeafRouter\n    hwsku: test-sku\n"
        )
        result = self.cfggen(
            "-j", initial,
            "-y", extra,
            "-a", '{"DEVICE_METADATA":{"localhost":{"hostname":"leaf-final"}}}',
            "--print-data",
        )
        self.assertEqual(
            json.loads(result.stdout),
            {"DEVICE_METADATA": {"localhost": {
                "hostname": "leaf-final", "type": "LeafRouter", "hwsku": "test-sku",
            }}},
        )

    def test_template_include_and_sonic_filters(self):
        self.write("included.j2", "{{ prefix|network }}/{{ prefix|prefixlen }}")
        template = self.write(
            "main.j2",
            '{% include "included.j2" %} {{ prefix|ipv4 }} {{ text|b64encode }} '
            '{{ argument|shellquote }}',
        )
        result = self.cfggen(
            "-a", json.dumps({"prefix": "192.0.2.9/24", "text": "SONiC", "argument": "two words"}),
            "-t", template,
        )
        self.assertEqual(result.stdout, "192.0.2.0/24 True U09OaUM= 'two words'\n")

    def test_template_output_file_and_staged_config(self):
        first = self.write(
            "config.j2", '{"DEVICE_METADATA":{"localhost":{"hostname":"{{ hostname }}"}}}'
        )
        second = self.write("hostname.j2", "{{ DEVICE_METADATA.localhost.hostname }}")
        output = self.root / "hostname.txt"
        result = self.cfggen(
            "-a", '{"hostname":"leaf-1"}',
            "-t", first + ",config-db",
            "-t", second + "," + str(output),
        )
        self.assertEqual(result.stdout, "")
        self.assertEqual(output.read_text(), "leaf-1\n")

    def test_runtime_namespace_is_available_to_the_cli(self):
        result = self.cfggen("--var-json", "DEVICE_METADATA", namespace="2")
        self.assertEqual(json.loads(result.stdout), {"localhost": {"namespace_id": "2"}})

    def test_invalid_interface_name_fails_rendering(self):
        template = self.write("interface.j2", "{{ interface|validate_interface_name }}")
        result = self.cfggen(
            "-a", '{"interface":"Ethernet0; touch /tmp/unexpected"}',
            "-t", template,
            check=False,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Invalid interface name", result.stderr)

    def test_invalid_input_fails_without_emitting_config(self):
        initial = self.write("broken.json", '{"DEVICE_METADATA":')
        result = self.cfggen("-j", initial, "--print-data", check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, "")


if __name__ == "__main__":
    if len(sys.argv) > 1 and not sys.argv[1].startswith("-"):
        CFGGEN = sys.argv.pop(1)
    unittest.main()
