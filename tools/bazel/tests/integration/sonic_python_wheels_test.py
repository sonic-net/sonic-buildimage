"""Install both SONiC wheels and check their setup.py package contracts."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import unittest

from tools.bazel.tests.integration.installed_wheel_fixture import installed_wheels, write_wheel_receipt
from tools.bazel.tests.integration.wheel_contract import assert_wheel_contract, expected_sources, setup_contract


parser = argparse.ArgumentParser()
for option in ("common-wheel", "config-wheel", "yang-wheel", "common-setup", "config-setup"):
    parser.add_argument("--" + option, type=Path, required=True)
INPUTS, TEST_ARGS = parser.parse_known_args()
RECEIPT = {}


class InstalledWheelsTest(unittest.TestCase):
    receipt_wheels = None
    receipt_metadata = {}

    @classmethod
    def setUpClass(cls):
        cls.packages = []
        for kind in ("common", "config"):
            wheel = getattr(INPUTS, kind + "_wheel").absolute()
            setup = getattr(INPUTS, kind + "_setup").absolute()
            contract = setup_contract(setup)
            cls.packages.append((wheel, setup, contract))
        own_roots = [Path(__file__).absolute().parents[4], Path(__file__).resolve().parents[4]]
        # Config-engine imports YANG from its installed wheel, with owner source
        # trees excluded by the shared fixture. Retain that dependency's hash too.
        cls.receipt_wheels = [INPUTS.yang_wheel.absolute(), *[wheel for wheel, _, _ in cls.packages]]
        installed = cls.enterClassContext(installed_wheels(
            cls.receipt_wheels, exclude_source_roots=own_roots))
        cls.installed = installed
        cls.root, cls.environment = installed.root, installed.environment
        cls.python, cls.site, cls.scripts = installed.python, installed.site, installed.scripts
        cls.env = dict(installed.env, PLATFORM="sonic-bazel-build", NAMESPACE_ID="")
        for key in ("CFGGEN_UNIT_TESTING", "CFGGEN_UNIT_TESTING_TOPOLOGY"):
            cls.env.pop(key, None)
        cls.env["PATH"] = str(cls.scripts) + os.pathsep + cls.env.get("PATH", "")
        cls.receipt_metadata = {wheel: {"distribution": contract["name"], "version": contract["version"]}
                                for wheel, _, contract in cls.packages}

    def test_metadata_and_payload_match_setup(self):
        """Ship every declared module, command and template with matching metadata."""
        for wheel, setup, contract in self.packages:
            with self.subTest(distribution=contract["name"]):
                assert_wheel_contract(wheel, setup, contract, self.installed)

    def test_installed_imports_and_commands(self):
        """Run both installed packages with their source directories unavailable."""
        modules = []
        for _, setup, contract in self.packages:
            modules.extend(name.removesuffix(".py").replace("/", ".").removesuffix(".__init__")
                           for name in expected_sources(contract, setup.parent))
        probe = """import hashlib,importlib,importlib.metadata,json,pathlib,sys
site=pathlib.Path(sys.argv[1]).resolve()
result={}
for distribution in ("sonic-py-common", "sonic-config-engine", "sonic-yang-mgmt"):
    metadata=importlib.metadata.distribution(distribution)
    assert metadata.version=="1.0", (distribution,metadata.version)
    assert pathlib.Path(metadata.locate_file("")).resolve()==site, distribution
import sonic_yang
assert pathlib.Path(sonic_yang.__file__).resolve().is_relative_to(site)
for name in json.loads(sys.argv[2]):
    module=importlib.import_module(name)
    path=pathlib.Path(module.__file__).resolve()
    assert path.is_relative_to(site), (name,str(path),str(site))
    result[name]=str(path.relative_to(site))
from sonic_grpc.gnoi import system_pb2
value=system_pb2.RebootRequest(method=system_pb2.HALT)
assert system_pb2.RebootRequest.FromString(value.SerializeToString()).method==system_pb2.HALT
import redisdl
# SONiC's existing redis-dump-load patch adds this batched pipeline reader.
# Empty input exercises that entry point without connecting to a Redis server.
assert list(redisdl._read_keys(None, [], pretty=False, encoding="utf-8"))==[]
redis_source=pathlib.Path(redisdl.__file__).read_bytes()
print(json.dumps({"modules": result, "redis_dump_load": {
    "version": importlib.metadata.version("redis-dump-load"),
    "source_sha256": hashlib.sha256(redis_source).hexdigest(),
    "pipeline_reader": True,
}},sort_keys=True))
"""
        process = subprocess.run([str(self.python), "-I", "-c", probe, str(self.site), json.dumps(sorted(set(modules)))],
                                 cwd=self.root, env=self.env, capture_output=True, text=True, timeout=60)
        self.assertEqual(process.returncode, 0, process.stdout + process.stderr)
        installed = json.loads(process.stdout)
        RECEIPT["installed_modules"] = installed["modules"]
        RECEIPT["redis_dump_load"] = installed["redis_dump_load"]
        for name, expected in (("sonic-cfggen", "--from-db"), ("sonic-db-load", "Load data from FILE"), ("sonic-db-dump", "Dump data from specified")):
            # Use the installed entrypoint (including its installer-selected
            # interpreter) from a directory with neither package's source tree.
            result = subprocess.run([str(self.scripts / name), "--help"], cwd=self.root,
                                    env=self.env, capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn(expected, result.stdout)
        generated = subprocess.run([str(self.scripts / "sonic-cfggen"), "-a", '{"hostname":"installed-wheel"}', "-v", "hostname"],
                                   cwd=self.root, env=self.env, capture_output=True, text=True, timeout=30)
        self.assertEqual(generated.returncode, 0, generated.stdout + generated.stderr)
        self.assertEqual(generated.stdout.strip(), "installed-wheel")
        RECEIPT["commands"] = ["sonic-cfggen", "sonic-db-load", "sonic-db-dump"]


if __name__ == "__main__":
    result = unittest.main(argv=[__file__, *TEST_ARGS], exit=False).result
    write_wheel_receipt(InstalledWheelsTest.receipt_wheels,
                        status="passed" if result.wasSuccessful() else "failed",
                        wheel_metadata=InstalledWheelsTest.receipt_metadata,
                        details=dict(RECEIPT, tests_run=result.testsRun))
    raise SystemExit(0 if result.wasSuccessful() else 1)
