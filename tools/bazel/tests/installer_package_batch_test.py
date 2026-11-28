"""Execute the production package blocks without installing into the host."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]
SOURCE = (ROOT / "files/build_templates/sonic_debian_extension.j2").read_text()


class InstallerPackageBatchTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.work = Path(temporary.name)
        self.filesystem = self.work / "root"
        self.filesystem.mkdir()
        self.wheels = self.work / "wheels"
        self.wheels.mkdir()
        for name in ("redis_dump_load.whl", "sonic_py_common.whl"):
            (self.wheels / name).write_bytes(b"wheel fixture")
        self.commands = self.work / "commands"
        self.commands.mkdir()
        sudo = self.commands / "sudo"
        sudo.write_text("#!" + sys.executable + "\n" + r'''
import json, os, pathlib, shutil, sys
args = sys.argv[1:]
while args and "=" in args[0]:
    args.pop(0)
with pathlib.Path(os.environ["CALLS"]).open("a") as output:
    output.write(json.dumps(args) + "\n")
if args[0] == "cp":
    shutil.copyfile(args[1], pathlib.Path(args[2]) / pathlib.Path(args[1]).name)
elif args[0] == "rm":
    for name in args[1:]:
        if not name.startswith("-"):
            pathlib.Path(name).unlink(missing_ok=True)
elif args[0] == "chroot" and args[2] == "pip3":
    sys.exit(int(os.environ.get("FAIL_PIP", "0")))
''')
        sudo.chmod(0o755)

    def run_blocks(self, wheel_paths=None, **settings):
        wheel_paths = wheel_paths if wheel_paths is not None else [
            str(self.wheels / "redis_dump_load.whl"), str(self.wheels / "sonic_py_common.whl")]
        selection = SOURCE[SOURCE.index("batch_packages=n"):SOURCE.index("\n. functions.sh")]
        helpers = SOURCE[SOURCE.index("install_pip_package() {"):SOURCE.index("install_deb_package() {")]
        efi = SOURCE[SOURCE.index("# Install efitools"):SOURCE.index("# Install sedutil")]
        packages = SOURCE[SOURCE.index("# Install dependencies for SONiC config engine"):
                          SOURCE.index("# Install dependency pkgs for SONiC config engine Python 2")]
        script = selection + helpers + efi + packages
        for name, path in zip(("redis_dump_load_py3_wheel_path", "sonic_py_common_py3_wheel_path"),
                              wheel_paths):
            script = script.replace("{{" + name + "}}", path)
        environment = {
            "PATH": str(self.commands) + os.pathsep + os.defpath,
            "CALLS": str(self.work / "calls.jsonl"), "FILESYSTEM_ROOT": str(self.filesystem),
            "CONFIGURED_PLATFORM": "vs", "CONFIGURED_ARCH": "amd64", "https_proxy": "",
            **settings,
        }
        result = subprocess.run(["bash", "-e", "-c", script + "\necho completed"],
                                env=environment, cwd=self.work, capture_output=True,
                                text=True, timeout=10)
        calls = [json.loads(line) for line in (self.work / "calls.jsonl").read_text().splitlines()]
        return result, calls

    def installs(self, calls, tool):
        return [args[5:] if tool == "apt-get" else args[4:]
                for args in calls if args[0] == "chroot" and args[2] == tool]

    def test_native_vs_batches_adjacent_packages_and_cleans_staged_wheels(self):
        result, calls = self.run_blocks(SONIC_VS_BATCH_PACKAGES="1")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.installs(calls, "apt-get"),
                         [["efitools", "mokutil"], ["python3-dev", "j2cli"]])
        self.assertEqual(self.installs(calls, "pip3"), [
            ["redis==3.5.3", "redis_dump_load.whl"],
            ["psutil", "blkinfo", "ipaddr", "grpcio==1.71.0", "grpcio-tools==1.71.0",
             "smbus2", "python-ipmi", "telnetlib3", "sonic_py_common.whl"],
        ])
        self.assertEqual(list(self.filesystem.iterdir()), [])

    def test_defaults_and_non_native_builds_keep_serial_installation(self):
        for settings in ({}, {"SONIC_VS_BATCH_PACKAGES": "0"},
                         {"CONFIGURED_PLATFORM": "generic"}, {"CONFIGURED_ARCH": "arm64"},
                         {"CONFIGURED_ARCH": "armhf"}, {"CROSS_BUILD_ENVIRON": "y"},
                         {"MULTIARCH_QEMU_ENVIRON": "y"}):
            with self.subTest(settings=settings):
                (self.work / "calls.jsonl").unlink(missing_ok=True)
                env = {"SONIC_VS_BATCH_PACKAGES": "1", **settings}
                if not settings:
                    env.pop("SONIC_VS_BATCH_PACKAGES")
                result, calls = self.run_blocks(**env)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(self.installs(calls, "apt-get"),
                                 [["efitools"], ["mokutil"], ["python3-dev"], ["j2cli"]])
                self.assertEqual(self.installs(calls, "pip3"), [
                    ["redis==3.5.3"], ["redis_dump_load.whl"], ["psutil"], ["blkinfo"],
                    ["ipaddr"], ["grpcio==1.71.0"], ["grpcio-tools==1.71.0"], ["smbus2"],
                    ["python-ipmi"], ["telnetlib3"], ["sonic_py_common.whl"],
                ])

    def test_missing_optional_wheels_keep_runtime_requirements(self):
        result, calls = self.run_blocks(wheel_paths=["", ""], SONIC_VS_BATCH_PACKAGES="1")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.installs(calls, "pip3"), [
            ["redis==3.5.3"],
            ["psutil", "blkinfo", "ipaddr", "grpcio==1.71.0", "grpcio-tools==1.71.0",
             "smbus2", "python-ipmi", "telnetlib3"],
        ])
        self.assertFalse(any(args[0] in ("cp", "rm") for args in calls))

    def test_batched_wheel_staging_handles_paths_with_spaces(self):
        self.filesystem = self.work / "root with spaces"
        self.filesystem.mkdir()
        moved = self.work / "wheels with spaces"
        self.wheels.rename(moved)
        self.wheels = moved
        result, _ = self.run_blocks(SONIC_VS_BATCH_PACKAGES="1")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(list(self.filesystem.iterdir()), [])

    def test_failed_batch_stops_assembly_and_preserves_exit_status(self):
        result, calls = self.run_blocks(SONIC_VS_BATCH_PACKAGES="1", FAIL_PIP="17")
        self.assertEqual(result.returncode, 17, result.stderr)
        self.assertNotIn("completed", result.stdout)
        self.assertEqual(len(self.installs(calls, "pip3")), 1)
        self.assertTrue((self.filesystem / "redis_dump_load.whl").is_file())


if __name__ == "__main__":
    unittest.main()
